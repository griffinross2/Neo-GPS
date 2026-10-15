#include "l1ca_channel.h"

#include "constants.h"

#include <cmath>
#include <numbers>
#include <string>

void L1CAChannel::start(int sv, double doppler, double code_phase)
{
    m_sv = sv;

    // Accumulators
    m_ie = 0;
    m_qe = 0;
    m_ip = 0;
    m_qp = 0;
    m_il = 0;
    m_ql = 0;

    m_cn0_estimator = CN0Estimator<L1CA_CN0_ESTIMATOR_LENGTH>(0.001);
    m_cn0 = -100.0;

    float code_doppler = doppler * GPS_L1CA_CODE_RATE_CPS / GPS_L1CA_FREQ_HZ;
    m_code_pll = SecondOrderPLL(L1CA_PULLIN_CODE_PLL_BANDWIDTH_HZ, code_doppler);
    m_lo_pll = ThirdOrderFLLAssistedPLL(L1CA_PULLIN_LO_FLL_BANDWIDTH_HZ, L1CA_TRACKING_LO_PLL_BANDWIDTH_HZ, doppler);

    m_prev_ip = 0;
    m_prev_qp = 0;

    m_ms_elapsed = 0;

    m_code_error = code_doppler;
    m_lo_error = doppler;

#ifdef DEBUG_FILE
    m_debug_file = std::make_unique<std::ofstream>("l1ca_sv" + std::to_string(sv) + ".csv");
    *m_debug_file << "ms_elapsed,bit_ms,ip,qp,ie,qe,il,ql,cn0,lo_error,code_error,lo_rate,code_rate\n";
#endif

    m_bit_sync_histogram.fill(0);
    m_bit_sync_transition_count = 0;

    m_bit_total = 0;
    m_bit_buffer.fill(0);

    m_bit_count = 0;
    m_last_time_of_week = -2;
    m_nav_valid = false;

    m_epochs_since_last_nav_message = 0;

    m_state = L1CA_CHANNEL_STATE_PULL_IN_FLL;
}

void L1CAChannel::stop()
{
    m_state = L1CA_CHANNEL_STATE_IDLE;
}

void L1CAChannel::update(int ip, int qp, int ie, int qe, int il, int ql)
{
    if (m_state == L1CA_CHANNEL_STATE_IDLE)
    {
        return;
    }
    m_ie = ie;
    m_qe = qe;
    m_ip = ip;
    m_qp = qp;
    m_il = il;
    m_ql = ql;

    update_epoch();
}

inline void L1CAChannel::update_epoch()
{
    update_cn0();
    update_filter_bandwidths();
    loop_filter();

    // Try to transition from pull-in to tracking
    if (m_state == L1CA_CHANNEL_STATE_PULL_IN_FLL && m_ms_elapsed >= L1CA_PULLIN_TIME_MS)
    {
        printf("SV %d: FLL pull-in complete\n", m_sv);
        m_state = L1CA_CHANNEL_STATE_PULL_IN_PLL;
    }

    // Try to transition from pull-in to tracking
    if (m_state == L1CA_CHANNEL_STATE_PULL_IN_PLL && m_cn0 >= L1CA_PULLIN_TO_TRACKING_CN0_THRESHOLD)
    {
        printf("SV %d: Tracking\n", m_sv);
        m_state = L1CA_CHANNEL_STATE_TRACKING;
        reset_bit_sync();
        m_code_pll.reset();
    }

    // Bit sync
    if (m_state == L1CA_CHANNEL_STATE_TRACKING)
    {
        update_bit_sync();
    }

    m_epochs_since_last_nav_message++;

    // Bit recovery
    if (m_state == L1CA_CHANNEL_STATE_TRACKING_BIT_SYNCED)
    {
        update_nav();
    }

#ifdef DEBUG_FILE
    *m_debug_file << m_ms_elapsed << "," << m_bit_ms << "," << m_ip << "," << m_qp << "," << m_ie << "," << m_qe << ","
                  << m_il << "," << m_ql << "," << m_cn0 << "," << m_lo_error << "," << m_code_error << "," << m_lo_rate
                  << "," << m_code_rate << "\n";
#endif

    m_bit_ms = (m_bit_ms + 1) % 20;
    m_ms_elapsed += 1;

    // Reset accumulators
    m_prev_ip = m_ip;
    m_prev_qp = m_qp;
}

inline void L1CAChannel::update_filter_bandwidths()
{
    switch (m_state)
    {
        case L1CA_CHANNEL_STATE_PULL_IN_FLL:
        case L1CA_CHANNEL_STATE_PULL_IN_PLL:
            m_lo_pll.set_bandwidth(L1CA_PULLIN_LO_FLL_BANDWIDTH_HZ, L1CA_TRACKING_LO_PLL_BANDWIDTH_HZ);
            m_code_pll.set_bandwidth(L1CA_PULLIN_CODE_PLL_BANDWIDTH_HZ);
            break;

        case L1CA_CHANNEL_STATE_TRACKING:
            m_lo_pll.set_bandwidth(L1CA_PULLIN_LO_FLL_BANDWIDTH_HZ, L1CA_TRACKING_LO_PLL_BANDWIDTH_HZ);
            m_code_pll.set_bandwidth(L1CA_TRACKING_CODE_PLL_BANDWIDTH_HZ);
            break;

        default:
            break;
    }
}

inline void L1CAChannel::loop_filter()
{
    switch (m_state)
    {
        case L1CA_CHANNEL_STATE_PULL_IN_FLL:
            m_lo_error = m_lo_pll.update(lo_freq_discriminator(), 0.0, 0.001);
            break;

        default:
            m_lo_error = m_lo_pll.update(0.0, lo_phase_discriminator(), 0.001);
            break;
    }

    m_code_error = m_code_pll.update(code_phase_discriminator(), 0.001);

    m_lo_rate = (4 * (m_freq_if_hz + m_lo_error)) / m_freq_sample_hz;

    float carrier_aiding =
        (m_state >= L1CA_CHANNEL_STATE_TRACKING) ? (m_lo_error * GPS_L1CA_CODE_RATE_CPS / GPS_L1CA_FREQ_HZ) : 0.0;
    m_code_rate = (GPS_L1CA_CODE_RATE_CPS + m_code_error + carrier_aiding) / m_freq_sample_hz;
}

inline void L1CAChannel::update_cn0()
{
    m_cn0_estimator.update(m_ip, m_qp);
    if (m_cn0_estimator.is_full())
    {
        m_cn0 = m_cn0_estimator.get_cn0();
    }
}

inline double L1CAChannel::code_phase_discriminator()
{
    // Normalized early-late power discriminator
    float code_discriminator = 0.0;
    float power_early = std::sqrt(m_ie * m_ie + m_qe * m_qe);
    float power_late = std::sqrt(m_il * m_il + m_ql * m_ql);
    if (power_early + power_late != 0)
    {
        code_discriminator = 0.5 * ((power_early - power_late) / (power_early + power_late));
    }

    return code_discriminator;
}

inline double L1CAChannel::lo_phase_discriminator()
{
    float lo_discriminator = 0;
    if (m_ip != 0)
    {
        lo_discriminator = std::atan(static_cast<float>(m_qp) / static_cast<float>(m_ip)) / (2.0 * std::numbers::pi);
    }

    return lo_discriminator;
}

inline double L1CAChannel::lo_freq_discriminator()
{
    float cross = m_prev_ip * m_qp - m_ip * m_prev_qp;
    float dot = m_prev_ip * m_ip + m_prev_qp * m_qp;

    float lo_discriminator_fll = atan2(cross, dot) / (2.0 * std::numbers::pi * 0.001);  // Hz

    return lo_discriminator_fll;
}

inline void L1CAChannel::update_bit_sync()
{
    // Check for a bit transition
    if (m_prev_ip * m_ip < 0)
    {
        m_bit_sync_histogram[m_bit_ms]++;
        m_bit_sync_transition_count++;
    }

    // Check if the bit synchronization has finished
    if (m_bit_sync_transition_count >= L1CA_BIT_SYNC_TRANSITIONS)
    {
        // Look through the histogram to find the max and 2nd to max
        int highest_index = -1;
        int second_highest_index = -1;

        for (size_t i = 0; i < 20; i++)
        {
            if (highest_index < 0 || m_bit_sync_histogram[i] > m_bit_sync_histogram[highest_index])
            {
                second_highest_index = highest_index;
                highest_index = i;
            }
            else if (second_highest_index < 0 || m_bit_sync_histogram[i] > m_bit_sync_histogram[second_highest_index])
            {
                second_highest_index = i;
            }
        }

        // Check that the ratio is at least 3
        if (m_bit_sync_histogram[highest_index] >= 3 * m_bit_sync_histogram[second_highest_index])
        {
            // Want to move bit_ms back by the highest index
            // = adding (20 - highest_index) then mod 20
            m_bit_ms = (m_bit_ms + (20 - highest_index)) % 20;
            m_state = L1CA_CHANNEL_STATE_TRACKING_BIT_SYNCED;
            // printf("SV %d: Bit synchronization complete\n", m_sv);

            for (size_t i = 0; i < 20; i++)
            {
                // printf("%d ", m_bit_sync_histogram[i]);
            }
            // printf("\n");
        }
        else
        {
            m_state = L1CA_CHANNEL_STATE_TRACKING;
            // printf("SV %d: Bit synchronization failed\n", m_sv);
        }

        reset_bit_sync();
    }
}

inline void L1CAChannel::reset_bit_sync()
{
    m_bit_sync_histogram.fill(0);
    m_bit_sync_transition_count = 0;
}

inline bool L1CAChannel::check_parity(uint8_t flip, std::array<bool, 10>& subword_flips)
{
    // Reference: IS-GPS-200N, Table 20-XIV, pg. 139
    uint8_t D30 = flip;
    uint8_t D29 = flip;
    subword_flips[0] = (flip == 1);

    std::array<uint8_t, 24> d{0};
    std::array<uint8_t, 6> p{0};

    // Go through each of the 10 subwords and check the parity
    for (size_t subword = 0; subword < 10; subword++)
    {
        // Recover the 24 data bits
        for (size_t i = 0; i < 24; i++)
        {
            d[i] = m_bit_buffer[subword * 30 + i] ^ D30;
        }

        // Calculate the expected parity bits
        p[0] = D29 ^ d[0] ^ d[1] ^ d[2] ^ d[4] ^ d[5] ^ d[9] ^ d[10] ^ d[11] ^ d[12] ^ d[13] ^ d[16] ^ d[17] ^ d[19] ^
               d[22];
        p[1] = D30 ^ d[1] ^ d[2] ^ d[3] ^ d[5] ^ d[6] ^ d[10] ^ d[11] ^ d[12] ^ d[13] ^ d[14] ^ d[17] ^ d[18] ^ d[20] ^
               d[23];
        p[2] = D29 ^ d[0] ^ d[2] ^ d[3] ^ d[4] ^ d[6] ^ d[7] ^ d[11] ^ d[12] ^ d[13] ^ d[14] ^ d[15] ^ d[18] ^ d[19] ^
               d[21];
        p[3] = D30 ^ d[1] ^ d[3] ^ d[4] ^ d[5] ^ d[7] ^ d[8] ^ d[12] ^ d[13] ^ d[14] ^ d[15] ^ d[16] ^ d[19] ^ d[20] ^
               d[22];
        p[4] = D30 ^ d[0] ^ d[2] ^ d[4] ^ d[5] ^ d[6] ^ d[8] ^ d[9] ^ d[13] ^ d[14] ^ d[15] ^ d[16] ^ d[17] ^ d[20] ^
               d[21] ^ d[23];
        p[5] = D29 ^ d[2] ^ d[4] ^ d[5] ^ d[7] ^ d[8] ^ d[9] ^ d[10] ^ d[12] ^ d[14] ^ d[18] ^ d[21] ^ d[22] ^ d[23];

        // Check the parity bits
        if (!std::equal(p.cbegin(), p.cend(), m_bit_buffer.cbegin() + subword * 30 + 24,
                        m_bit_buffer.cbegin() + subword * 30 + 30))
        {
            return false;
        }

        // Update D29 and D30 for the next subword
        D29 = p[4];
        D30 = p[5];

        // Save the D30 needed to flip the next subword
        if (subword < 9)
        {
            subword_flips[subword + 1] = (D30 == 1);
        }
    }

    return true;
}

inline void L1CAChannel::update_nav()
{
    if (m_bit_ms == 0)
    {
        m_bit_buffer.at(m_bit_count) = m_bit_total > 0 ? 1 : 0;
        m_bit_count++;
        m_bit_total = 0;
    }
    m_bit_total += m_ip;

    if (m_bit_count >= 300)
    {
        constexpr std::array<uint8_t, 8> preamble_norm = {1, 0, 0, 0, 1, 0, 1, 1};
        constexpr std::array<uint8_t, 8> preamble_inv = {0, 1, 1, 1, 0, 1, 0, 0};
        uint8_t flip = 0;

        if (std::equal(m_bit_buffer.cbegin(), m_bit_buffer.cbegin() + 8, preamble_norm.cbegin(), preamble_norm.cend()))
            flip = 0;
        else if (std::equal(m_bit_buffer.cbegin(), m_bit_buffer.cbegin() + 8, preamble_inv.cbegin(),
                            preamble_inv.cend()))
            flip = 1;
        else
        {
            // No preamble found
            std::copy(m_bit_buffer.begin() + 1, m_bit_buffer.end(), m_bit_buffer.begin());
            m_bit_count--;
            return;
        }

        // Check parity
        std::array<bool, 10> subword_flips{0};
        if (!check_parity(flip, subword_flips))
        {
            // Parity check failed
            std::copy(m_bit_buffer.begin() + 1, m_bit_buffer.end(), m_bit_buffer.begin());
            m_bit_count--;
            // printf("par\n");
            return;
        }

        // Flip the bits if necessary
        for (size_t i = 0; i < 300; i++)
        {
            if (subword_flips[i / 30])
            {
                m_bit_buffer[i] = 1 - m_bit_buffer[i];
            }
        }

        // Verify HOW subframe ID
        int subframe_id = (m_bit_buffer[29 + 20] << 2) | (m_bit_buffer[29 + 21] << 1) | m_bit_buffer[29 + 22];
        if (subframe_id < 1 || subframe_id > 5)
        {
            // Invalid subframe ID
            std::copy(m_bit_buffer.begin() + 1, m_bit_buffer.end(), m_bit_buffer.begin());
            m_bit_count--;
            // printf("sfid\n");
            return;
        }

        // Final verification is the TOW incrementing by 1
        int time_of_week = 0;
        for (int i = 30; i < 30 + 17; i++)
        {
            time_of_week = (time_of_week << 1) | m_bit_buffer[i];
        }
        if ((m_last_time_of_week + 1) % 100800 == time_of_week)
        {
            m_nav_valid = true;
        }
        else
        {
            m_nav_valid = false;
        }
        m_last_time_of_week = time_of_week;

        // The TOW refers to the start of the next subframe
        // We are currently one 1ms past the start of the next subframe, so we reset to 1
        m_epochs_since_last_nav_message = 1;

        // Success
        printf("GPS L1 SV %d found subframe %d at %d ms. TOW = %d\n", m_sv, subframe_id, m_ms_elapsed, time_of_week);

        m_ephemeris.update(m_sv, m_bit_buffer);

        m_bit_count = 0;
        return;
    }
}

double L1CAChannel::get_corrected_gps_time_of_week() const
{
    // Code phase is referenced to early chip, so the prompt is L1CA_CHIP_SPACING chips behind
    // double code_phase_chips = m_code_gen.chip + m_code_phase - L1CA_CHIP_SPACING;

    // double t_sv = (m_last_time_of_week * 6.0) + (m_epochs_since_last_nav_message / 1000.0) +
    //               (code_phase_chips / GPS_L1CA_CODE_RATE_CPS);

    // double t = t_sv - m_ephemeris.get_clock_correction(m_sv, t_sv);
    double t = 0.0;

    return t;
}