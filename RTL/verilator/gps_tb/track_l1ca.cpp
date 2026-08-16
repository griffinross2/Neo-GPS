#include "track_l1ca.h"
#include "math.h"

#include <string.h>
#include <stdio.h>

#define CHIP_RATE 1.023e6
#define CODE_LENGTH 1023
#define FREQ_L1CA 1.57542e9
#define GNSS_PI 3.14159265358979323846
#define BIT_DESYNC_THRESHOLD 36.0
#define BIT_SYNC_THRESHOLD 34.0
#define BIT_SYNC_MS 1000

// FLL pull-in: for the first FLL_PULLIN_MS of tracking, run the loop as FLL-only
// (wide bandwidth, no phase term) to resolve large residual Doppler error left over
// from acquisition before handing off to phase tracking. FLL_ASSIST_BW is a small
// residual FLL term kept active afterward to help the Costas loop through dynamics.
#define FLL_PULLIN_MS 500
#define FLL_PULLIN_BW 20.0
#define FLL_ASSIST_BW 2.0
#define FLL_ASSIST_CN0_ENABLE 28.0  // (re-)enable FLL assist once smoothed cn0 drops below this
#define FLL_ASSIST_CN0_DISABLE 33.0 // disable FLL assist once smoothed cn0 climbs above this
                                    // (gap between the two is hysteresis, so the gate can't
                                    // chatter every epoch cn0 sits near a single threshold)

// The PLL/DLL bandwidth schedule and the FLL-assist gate are both closed loops through cn0
// (cn0 -> bandwidth/gate -> tracking noise -> cn0). Reacting to a single noisy instantaneous
// cn0 sample every epoch turns that into a self-sustaining oscillation, so control decisions
// use a slow EMA of cn0 instead, and the bandwidth schedule is only re-evaluated periodically.
#define CN0_SMOOTHING_TAU_MS 500.0
#define BW_SCHEDULE_UPDATE_MS 500

// 1-bit carrier LUTs
const uint8_t carrier_sin[] = {1, 1, 0, 0};
const uint8_t carrier_cos[] = {1, 0, 0, 1};

struct BandwidthPoint
{
    double cn0;    // dB-Hz
    double bn_pll; // Hz
    double bn_dll; // Hz
};

static const BandwidthPoint BW_SCHEDULE[] = {
    {25.0, 20.0, 0.6}, // weak signal: still needs enough bandwidth to reject real
                       // automotive dynamics, not just noise -- too narrow here traps
                       // the loop (can't correct the error that's suppressing cn0)
    {35.0, 25.0, 0.8}, // nominal automotive open-sky signal
    {45.0, 32.0, 1.3}, // strong signal: buy back dynamic margin and snap back quickly once
                       // a fade clears
};

static const int BW_SCHEDULE_LEN = sizeof(BW_SCHEDULE) / sizeof(BW_SCHEDULE[0]);

static void bandwidth_from_cn0(double cn0, double *bn_pll, double *bn_dll)
{
    if (cn0 <= BW_SCHEDULE[0].cn0)
    {
        *bn_pll = BW_SCHEDULE[0].bn_pll;
        *bn_dll = BW_SCHEDULE[0].bn_dll;
        return;
    }
    if (cn0 >= BW_SCHEDULE[BW_SCHEDULE_LEN - 1].cn0)
    {
        *bn_pll = BW_SCHEDULE[BW_SCHEDULE_LEN - 1].bn_pll;
        *bn_dll = BW_SCHEDULE[BW_SCHEDULE_LEN - 1].bn_dll;
        return;
    }
    for (int i = 0; i < BW_SCHEDULE_LEN - 1; i++)
    {
        if (cn0 >= BW_SCHEDULE[i].cn0 && cn0 <= BW_SCHEDULE[i + 1].cn0)
        {
            double frac = (cn0 - BW_SCHEDULE[i].cn0) / (BW_SCHEDULE[i + 1].cn0 - BW_SCHEDULE[i].cn0);
            *bn_pll = BW_SCHEDULE[i].bn_pll + frac * (BW_SCHEDULE[i + 1].bn_pll - BW_SCHEDULE[i].bn_pll);
            *bn_dll = BW_SCHEDULE[i].bn_dll + frac * (BW_SCHEDULE[i + 1].bn_dll - BW_SCHEDULE[i].bn_dll);
            return;
        }
    }
}

uint8_t check_parity(uint8_t *bits, uint8_t *p, uint8_t D29, uint8_t D30);

GPSL1CATracker::GPSL1CATracker(int sv, double fs, double fc, double doppler)
{
    this->sv = sv;
    this->fs = fs;
    this->fc = fc;
    this->doppler = doppler;

    // NCO rates, in the channel's 32-bit phase-accumulator units. These are what
    // get_code_rate()/get_carrier_rate() hand back, so seed them from the acquired
    // Doppler rather than leaving them indeterminate until the first epoch.
    code_rate = (CHIP_RATE + doppler * CHIP_RATE / FREQ_L1CA) / fs * 4294967296.0;
    carrier_rate = (fc + doppler) / fs * 4294967296.0;

    // Code generator outputs
    // Figure out the start chip and increment it if we overflowed
    // the code_phase when we defined it.
    code_early = 0;
    code_prompt = 0;
    code_late = 0;

    // Accumulators
    ie_longint = 0;
    qe_longint = 0;
    ip_longint = 0;
    qp_longint = 0;
    il_longint = 0;
    ql_longint = 0;
    longint_en = false;

    // DLL filter
    dll = new SecondOrderPLL(0.2, doppler * CHIP_RATE / FREQ_L1CA);

    // PLL filter (starts FLL-only, wide bandwidth, to pull in residual acquisition
    // frequency error before the phase term is enabled)
    pll = new ThirdOrderFLLAssistedPLL(FLL_PULLIN_BW, 0.0, doppler);
    fll_pull_in = true;
    fll_pullin_deadline_ms = FLL_PULLIN_MS;
    fll_assist_active = true;

    // Time
    ms_elapsed = 0;

    // Prompt buffers
    prompt_len = 0;
    prompt_idx = 0;

    // Bit sync
    last_ip = 0;
    bit_sync_count = 0;
    memset(bit_hist, 0, sizeof(bit_hist));
    bit_synced = false;
    bit_ms = 0;
    bit_sum = 0;

    // Bit buffer
    nav_count = 0;

    // Nav
    nav_valid = false;
    last_z_count = -2; // So we don't think we incremented on the first one

    // SNR
    cn0 = 0;
    cn0_smoothed = 0;

    carrier_aiding_smoothed = doppler * CHIP_RATE / FREQ_L1CA;
    carrier_aiding_alpha = 1;

    debug_file = std::ofstream("sv" + std::to_string(sv) + ".csv");
    debug_file << "ms,cn0,cn0_smoothed,carrier_error,code_error,longint_en,fll_pull_in,fll_assist_active,bit_synced,ip,qp" << std::endl;
}

GPSL1CATracker::~GPSL1CATracker()
{
    delete dll;
    delete pll;
}

// Update the tracker with a new epoch
void GPSL1CATracker::update_epoch(int32_t ip, int32_t qp, int32_t ie, int32_t qe, int32_t il, int32_t ql)
{
    // if (sv == 7)
    // {
    //     printf("%d,%d\n", ip, qp);
    // }

    ip_longint += ip;
    qp_longint += qp;
    ie_longint += ie;
    qe_longint += qe;
    il_longint += il;
    ql_longint += ql;

    // SNR
    ip_buffer[prompt_idx] = ip;
    qp_buffer[prompt_idx] = qp;
    prompt_len += (prompt_len >= 100) ? 0 : 1;
    prompt_idx = (prompt_idx + 1) % 100;

    cn0 = cn0_svn_estimator(ip_buffer, qp_buffer, prompt_len, 0.001);

    // printf("PRN %d: C/N0 = %.2f dB-Hz\n", sv, cn0);

    // Slow EMA of cn0 for control decisions (bandwidth schedule, FLL gate). The bandwidth
    // schedule and FLL gate are both closed loops through cn0 (cn0 -> bandwidth/gate ->
    // tracking noise -> cn0); driving them off a single noisy epoch's cn0 sample turns that
    // into a self-sustaining oscillation, so they react to this smoothed value instead.
    if (prompt_len < 20)
    {
        // cn0_svn_estimator is essentially noise on tiny windows (at prompt_len==1 it
        // reduces to I[0]^2/Q[0]^2) -- track the raw estimate directly rather than
        // anchoring the smoother on a near-arbitrary single-sample value.
        cn0_smoothed = cn0;
    }
    else
    {
        double cn0_smoothing_alpha = 1.0 - exp(-1.0 / CN0_SMOOTHING_TAU_MS);
        cn0_smoothed += cn0_smoothing_alpha * (cn0 - cn0_smoothed);
    }

    // Exit FLL pull-in once it has had time to resolve residual frequency error
    if (fll_pull_in && ms_elapsed >= fll_pullin_deadline_ms)
    {
        fll_pull_in = false;
        printf("L1 C/A PRN %d: FLL pull-in complete at %lld ms, enabling phase tracking\n", sv, ms_elapsed);
    }

    if (prompt_len == 100 && ms_elapsed % BW_SCHEDULE_UPDATE_MS == 0)
    {
        if (fll_pull_in)
        {
            // Stay FLL-only (no phase term) while pulling in
            pll->set_bandwidth(FLL_PULLIN_BW, 0.0);
            dll->set_bandwidth(BW_SCHEDULE[0].bn_dll);
        }
        else
        {
            double bw_pll = 0;
            double bw_dll = 0;
            bandwidth_from_cn0(cn0_smoothed, &bw_pll, &bw_dll);

            pll->set_bandwidth(FLL_ASSIST_BW, bw_pll);
            dll->set_bandwidth(bw_dll);
        }
    }

    // Gate the FLL assist with hysteresis (Schmitt trigger) on the smoothed cn0, so it can't
    // chatter on/off every epoch cn0 sits near a single threshold.
    if (!fll_pull_in)
    {
        if (fll_assist_active && cn0_smoothed > FLL_ASSIST_CN0_DISABLE)
        {
            fll_assist_active = false;
        }
        else if (!fll_assist_active && cn0_smoothed < FLL_ASSIST_CN0_ENABLE)
        {
            fll_assist_active = true;
        }
    }

    // Compute the FLL discriminator from the phase difference between consecutive
    // 1 ms prompt correlations (cheap and short-baseline enough to rarely straddle
    // a 20 ms data bit edge, even before bit sync). Once locked at good C/N0, this
    // 1 ms-baseline estimate is noisier than the phase discriminator, so it is only
    // used during pull-in or while the FLL assist gate is active.
    double carrier_discriminator_fll = 0;
    if (fll_pull_in || fll_assist_active)
    {
        int prev_idx = (prompt_idx - 2 + 100) % 100;
        if (prompt_len >= 2)
        {
            // Four-quadrant cross/dot discriminator (Kaplan). This doubles the
            // unambiguous frequency range of a single-quadrant atan-difference
            // (resolves a full +/-pi phase change per 1 ms baseline instead of
            // +/-pi/2, i.e. up to ~500 Hz instead of ~250 Hz before aliasing) --
            // this signal's residual Doppler sits right at that old limit, which
            // was causing the loop to settle into a false lock instead of true
            // zero error. A data-bit edge landing between the two samples can
            // still flip this by pi (~1/20 epochs before/without bit sync), but
            // the loop filter sees that as a rare outlier, not a systematic bias.
            double ip_prev = ip_buffer[prev_idx];
            double qp_prev = qp_buffer[prev_idx];
            double cross = ip_prev * qp - ip * qp_prev;
            double dot = ip_prev * ip + qp_prev * qp;
            carrier_discriminator_fll = atan2(cross, dot) / (2.0 * GNSS_PI * 0.001); // Hz
        }
    }

    double carrier_error = 0;
    double code_error = 0;

    // The "coherent" branch below is disabled: it fires 4 times per 20 ms bit period
    // (bit_ms == 3, 8, 13, 18, from the %5 condition) but the longint accumulators it
    // reads only reset once per bit, so each firing actually averages a different
    // window length (4/9/14/19 ms) while always being called with a fixed int_time of
    // 1 ms. That mismatch measurably hurts tracking (confirmed empirically: re-enabling
    // it drops mean cn0 from ~38.5 to ~32.2 dB-Hz on the reference recording) versus
    // just running the plain per-epoch discriminator below every time. Re-enable only
    // after fixing the accumulation-length/int_time mismatch (e.g. track the true
    // elapsed window and pass it as int_time, or make the branch fire exactly once per
    // 20 ms bit using the full bit-length accumulation).
    if (false && longint_en && (bit_ms + 2) % 5 == 0)
    {
        // Compute the Costas loop discriminator
        double carrier_discriminator = 0;
        if (ip_longint != 0)
        {
            carrier_discriminator = atan((double)qp_longint / ip_longint) / (2.0 * GNSS_PI);
        }

        // Compute the normalized early-minus late power discriminator
        double power_early = sqrt(ie_longint * ie_longint + qe_longint * qe_longint);
        double power_late = sqrt(il_longint * il_longint + ql_longint * ql_longint);
        double code_discriminator = 0.0;
        if (power_early + power_late != 0)
        {
            code_discriminator = 0.5 * ((power_early - power_late) / (power_early + power_late));
        }

        // Filter the carrier discriminator. update_epoch() runs every 1 ms regardless
        // of which branch supplies the discriminator, so int_time (the elapsed time
        // between successive loop-filter calls) is always 1 ms here -- NOT the 5 ms
        // coherent accumulation length of this branch's discriminator. Passing 0.005
        // would tell the filter 5x too much time had elapsed since its last call,
        // over-integrating the discriminator's sign every 5th epoch and producing a
        // systematic one-directional drift instead of noise.
        carrier_error = pll->update(0.0, carrier_discriminator, 0.001); // Hz

        // Filter the code discriminator
        code_error = dll->update(code_discriminator, 0.001); // chips/s
    }
    else
    {
        // Compute the Costas loop discriminator
        double carrier_discriminator = 0;
        if (ip != 0)
        {
            carrier_discriminator = atan((double)qp / ip) / (2.0 * GNSS_PI);
        }

        // Compute the normalized early-minus late power discriminator
        double power_early = sqrt(ie * ie + qe * qe);
        double power_late = sqrt(il * il + ql * ql);
        double code_discriminator = 0.0;
        if (power_early + power_late != 0)
        {
            code_discriminator = 0.5 * ((power_early - power_late) / (power_early + power_late));
        }

        // Filter the carrier discriminator
        carrier_error = pll->update(carrier_discriminator_fll, carrier_discriminator, 0.001); // Hz

        // Filter the code discriminator
        code_error = dll->update(code_discriminator, 0.001); // chips/s
    }

    // printf("%d, %d\n", ip, qp);
    // printf("%.3f, %.3f\n", code_error, carrier_error * CHIP_RATE / FREQ_L1CA);
    debug_file << ms_elapsed << "," << cn0 << "," << cn0_smoothed << ","
               << carrier_error << "," << code_error << ","
               << (int)longint_en << "," << (int)fll_pull_in << "," << (int)fll_assist_active << ","
               << (int)bit_synced << "," << ip << "," << qp << std::endl;

    carrier_rate = (fc + carrier_error) / fs * 4294967296.0;
    carrier_aiding_smoothed += carrier_aiding_alpha * (carrier_error * CHIP_RATE / FREQ_L1CA - carrier_aiding_smoothed);
    code_rate = (CHIP_RATE + code_error + carrier_aiding_smoothed) / fs * 4294967296.0;
    printf("%.3f, %.3f\n", carrier_rate, code_rate);
    // code_rate = (CHIP_RATE + code_error) / fs  * 4294967296.0;

    // Bit sync and bit recovery
    if (bit_synced)
    {
        // Recover the last bit
        if (bit_ms == 0)
        {
            nav_buf[nav_count] = (bit_sum > 0) ? 1 : 0;
            nav_count++;
            bit_sum = 0;
        }
        bit_sum += ip;
    }
    // Start bit sync when above threshold
    else if (cn0 > BIT_SYNC_THRESHOLD || bit_sync_count != 0)
    {
        if (bit_sync_count >= BIT_SYNC_MS)
        {
            // Find maximum likelihood edge position
            int bit_off = 0;
            int bit_max = 0;
            for (int i = 0; i < 20; i++)
            {
                if (bit_hist[i] > bit_max)
                {
                    bit_max = bit_hist[i];
                    bit_off = i;
                }
            }

            // Synchronize
            bit_synced = true;
            bit_ms -= bit_off + 1;
            if (bit_ms < 0)
                bit_ms += 20;
        }
        // Update the bit sync histogram
        bit_hist[bit_ms % 20] += ((ip > 0) != (last_ip > 0)) ? 1 : 0;
        bit_sync_count++;
    }

    if (bit_synced && bit_ms == 0)
    {
        longint_en = true;
        carrier_aiding_alpha = 0.05;
    }
    if (cn0_smoothed < BIT_DESYNC_THRESHOLD)
    {
        longint_en = false;
        carrier_aiding_alpha = 1.0;
        bit_sync_count = 0;
    }

    // if (sv == 7 && this->ms_elapsed % 10 == 0)
    // {
    //     printf("L1 C/A PRN %d: C/N0 = %.2f dB-Hz\n", sv, cn0);
    // }

    // Increment bit counter
    bit_ms = (bit_ms + 1) % 20;

    // Process the bits
    if (nav_count >= 300)
    {
        update_nav();
    }

    // Reset the long integration time accumulators
    if ((bit_ms + 1) % 20 == 0)
    {
        ie_longint = 0;
        qe_longint = 0;
        ip_longint = 0;
        qp_longint = 0;
        il_longint = 0;
        ql_longint = 0;
    }

    // Set the flag to indicate that this epoch has been processed
    ms_elapsed++;
}

void GPSL1CATracker::update_nav()
{
    uint8_t preamble_norm[] = {1, 0, 0, 0, 1, 0, 1, 1};
    uint8_t preamble_inv[] = {0, 1, 1, 1, 0, 1, 0, 0};
    uint8_t p[6];

    if (memcmp(nav_buf, preamble_norm, 8) == 0)
        p[4] = p[5] = 0;
    else if (memcmp(nav_buf, preamble_inv, 8) == 0)
        p[4] = p[5] = 1;
    else
    {
        // No preamble found
        memmove(nav_buf, nav_buf + 1, 299);
        nav_count--;
        return;
    }

    // Check 10 word parities
    for (int i = 0; i < 10; i++)
    {
        if (!check_parity(nav_buf + i * 30, p, p[4], p[5]))
        {
            // Invalid parity
            memmove(nav_buf, nav_buf + 1, 299);
            nav_count--;
            return;
        }
    }

    // Verify HOW subframe ID
    int subframe_id = (nav_buf[29 + 20] << 2) | (nav_buf[29 + 21] << 1) | nav_buf[29 + 22];
    if (subframe_id < 1 || subframe_id > 5)
    {
        // Invalid subframe ID
        memmove(nav_buf, nav_buf + 1, 299);
        nav_count--;
        return;
    }

    // Final verification is the Z count incrementing by 1
    int this_z_count = 0;
    for (int i = 30; i < 30 + 17; i++)
    {
        this_z_count = (this_z_count << 1) | nav_buf[i];
    }
    if ((last_z_count + 1) % 100800 == this_z_count)
    {
        nav_valid = true;
    }
    else
    {
        nav_valid = false;
    }
    last_z_count = this_z_count;

    // Success
    printf("GPS L1 PRN %d found subframe %d at %lld ms\n", sv, subframe_id, ms_elapsed);
    ephm.process_message(nav_buf);
    nav_count = 0;
    return;
}

double GPSL1CATracker::get_tx_time()
{
    // double t = (last_z_count * 6.0) +
    //            (nav_count / 50.0) +
    //            ((bit_ms + 1) / 1000.0) +
    //            (code_gen->chip / 1023000.0) +
    //            (code_phase / 1023000.0);
    double t = 0;

    return t;
}

void GPSL1CATracker::get_satellite_ecef(double t, double *x, double *y, double *z)
{
    ephm.get_satellite_ecef(t, x, y, z);
}

double GPSL1CATracker::get_clock_correction(double t)
{
    return ephm.get_clock_correction(t);
}

bool GPSL1CATracker::ready_to_solve()
{
    return ephm.ephm_valid();
}

uint8_t check_parity(uint8_t *bits, uint8_t *p, uint8_t D29, uint8_t D30)
{
    // D29 and D30 are the bits from the previous word
    uint8_t *d = bits - 1;
    for (uint8_t i = 1; i < 25; i++)
        d[i] ^= D30; // Flip to correct polarity as we go
    p[0] = D29 ^ d[1] ^ d[2] ^ d[3] ^ d[5] ^ d[6] ^ d[10] ^ d[11] ^ d[12] ^ d[13] ^ d[14] ^ d[17] ^ d[18] ^ d[20] ^ d[23];
    p[1] = D30 ^ d[2] ^ d[3] ^ d[4] ^ d[6] ^ d[7] ^ d[11] ^ d[12] ^ d[13] ^ d[14] ^ d[15] ^ d[18] ^ d[19] ^ d[21] ^ d[24];
    p[2] = D29 ^ d[1] ^ d[3] ^ d[4] ^ d[5] ^ d[7] ^ d[8] ^ d[12] ^ d[13] ^ d[14] ^ d[15] ^ d[16] ^ d[19] ^ d[20] ^ d[22];
    p[3] = D30 ^ d[2] ^ d[4] ^ d[5] ^ d[6] ^ d[8] ^ d[9] ^ d[13] ^ d[14] ^ d[15] ^ d[16] ^ d[17] ^ d[20] ^ d[21] ^ d[23];
    p[4] = D30 ^ d[1] ^ d[3] ^ d[5] ^ d[6] ^ d[7] ^ d[9] ^ d[10] ^ d[14] ^ d[15] ^ d[16] ^ d[17] ^ d[18] ^ d[21] ^ d[22] ^ d[24];
    p[5] = D29 ^ d[3] ^ d[5] ^ d[6] ^ d[8] ^ d[9] ^ d[10] ^ d[11] ^ d[13] ^ d[15] ^ d[19] ^ d[22] ^ d[23] ^ d[24];

    return (memcmp(d + 25, p, 6) == 0);
}