#include "e1_channel.h"

#include "constants.h"

#include <cmath>
#include <numbers>
#include <print>
#include <string>
#include <algorithm>

void E1Channel::start(int sv, double doppler, double code_phase)
{
    m_sv = sv;

    // Initialize NCOs
    const double code_doppler = doppler * GALILEO_E1_CODE_RATE_CPS / GALILEO_E1_FREQ_HZ;
    const double code_fractional_offset = code_phase - std::floor(code_phase);
    m_code_phase =
        code_fractional_offset + E1_CHIP_SPACING;  // Increment by the chip spacing because code gen is the early chip
    m_code_rate = (GALILEO_E1_CODE_RATE_CPS + code_doppler) / m_freq_sample_hz;

    m_lo_phase = 0.0;
    m_lo_rate = 4 * (m_freq_if_hz + doppler) / m_freq_sample_hz;

    // Initialize the code generator
    m_code_gen = E1Code(sv, static_cast<int>(std::floor(code_phase)));

    // Code outputs
    m_code_early_pilot = 0;
    m_code_prompt_pilot = 0;
    m_code_late_pilot = 0;
    m_code_prompt_data = 0;

    // Subcarrier outputs
    m_subcarrier_early = 0;
    m_subcarrier_prompt = 0;
    m_subcarrier_late = 0;

    // Accumulators
    m_ie_pilot = 0;
    m_qe_pilot = 0;
    m_ip_pilot = 0;
    m_qp_pilot = 0;
    m_il_pilot = 0;
    m_ql_pilot = 0;
    m_ie_pilot_no_sc = 0;
    m_qe_pilot_no_sc = 0;
    m_ip_pilot_no_sc = 0;
    m_qp_pilot_no_sc = 0;
    m_il_pilot_no_sc = 0;
    m_ql_pilot_no_sc = 0;
    m_ip_pilot_1ms = 0;
    m_qp_pilot_1ms = 0;

    m_epoch = false;
    m_epoch_1ms = false;

    m_cn0_estimator = CN0Estimator<E1_CN0_ESTIMATOR_LENGTH>(0.004);
    m_cn0 = -100.0;

    m_code_pll = SecondOrderPLL(E1_PULLIN_CODE_PLL_BANDWIDTH_HZ, code_doppler);
    m_lo_pll = ThirdOrderFLLAssistedPLL(E1_PULLIN_LO_FLL_BANDWIDTH_HZ, E1_PULLIN_LO_PLL_BANDWIDTH_HZ, doppler);

    m_prev_ip = 0;
    m_prev_qp = 0;

    m_ms_elapsed = 0;

    m_code_error = code_doppler;
    m_lo_error = doppler;

#ifdef DEBUG_FILE
    m_debug_file = std::make_unique<std::ofstream>("e1_sv" + std::to_string(sv) + ".csv");
    *m_debug_file << "ms_elapsed,ip,qp,ie,qe,il,ql,cn0,lo_error,code_error,lo_rate,code_rate\n";
#endif

    m_bit_count = 0;
    m_even_received = false;
    m_epochs_since_last_tow_sync = 0;
    m_tow_synced = false;

    m_state = E1_CHANNEL_STATE_PULL_IN_PLL;
}

void E1Channel::stop()
{
    m_state = E1_CHANNEL_STATE_IDLE;
}

void E1Channel::update(int sample)
{
    if (m_state == E1_CHANNEL_STATE_IDLE)
    {
        return;
    }

    update_sample(sample);

    if (m_epoch)
    {
        update_epoch();

        m_epoch = false;
    }

    if (m_epoch_1ms)
    {
        update_epoch_1ms();

        m_epoch_1ms = false;
    }
}

inline void E1Channel::update_sample(int sample)
{
    // Get the local oscillator signals
    int lo_i = m_lo_sin.at(static_cast<size_t>(std::floor(m_lo_phase)));
    int lo_q = m_lo_cos.at(static_cast<size_t>(std::floor(m_lo_phase)));

    // Update the carrier NCO
    m_lo_phase += m_lo_rate;
    if (m_lo_phase >= 4.0)
    {
        m_lo_phase -= 4.0;
    }

    // Get the code chip

    // Early code chip (first to change)
    if (m_code_phase >= 1.0)
    {
        // Get new early code chip
        m_code_gen.clock_chip();
        m_code_early_pilot = m_code_gen.get_chip_c();

        m_code_phase -= 1.0;

        // If code chip is now 0, this is the start of a new epoch
        if (m_code_gen.chip == 0)
        {
            m_epoch = true;
        }

        if (m_code_gen.chip % 1023 == 0)
        {
            m_epoch_1ms = true;
        }
    }
    else if (m_code_phase >= 2 * E1_CHIP_SPACING)
    {
        // Late code chip (half chip after early chip)
        m_code_late_pilot = m_code_prompt_pilot;
    }
    else if (m_code_phase >= E1_CHIP_SPACING)
    {
        // Prompt code chip (quarter chip after early chip)
        m_code_prompt_data = m_code_gen.get_chip_b();
        m_code_prompt_pilot = m_code_early_pilot;
    }

    // Get the BOC subcarrier value
    // Early subcarrier
    int boc_early = (m_code_phase < 0.5) ? 1 : -1;
    // Prompt subcarrier
    double prompt_phase = std::fmod(1 + m_code_phase - E1_CHIP_SPACING, 1.0);
    int boc_prompt = (prompt_phase < 0.5) ? 1 : -1;
    // Late subcarrier
    double late_phase = std::fmod(1 + m_code_phase - 2 * E1_CHIP_SPACING, 1.0);
    int boc_late = (late_phase < 0.5) ? 1 : -1;

    // Update the code NCO
    m_code_phase += m_code_rate;

    // Update the accumulators
    m_ie_pilot += sample * lo_i * m_code_early_pilot * boc_early;
    m_qe_pilot += sample * lo_q * m_code_early_pilot * boc_early;
    m_ip_pilot += sample * lo_i * m_code_prompt_pilot * boc_prompt;
    m_ip_pilot_1ms += sample * lo_i * m_code_prompt_pilot * boc_prompt;
    m_qp_pilot += sample * lo_q * m_code_prompt_pilot * boc_prompt;
    m_qp_pilot_1ms += sample * lo_q * m_code_prompt_pilot * boc_prompt;
    m_il_pilot += sample * lo_i * m_code_late_pilot * boc_late;
    m_ql_pilot += sample * lo_q * m_code_late_pilot * boc_late;
    m_ie_pilot_no_sc += sample * lo_i * m_code_early_pilot;
    m_qe_pilot_no_sc += sample * lo_q * m_code_early_pilot;
    m_ip_pilot_no_sc += sample * lo_i * m_code_prompt_pilot;
    m_qp_pilot_no_sc += sample * lo_q * m_code_prompt_pilot;
    m_il_pilot_no_sc += sample * lo_i * m_code_late_pilot;
    m_ql_pilot_no_sc += sample * lo_q * m_code_late_pilot;

    m_ip_data += sample * lo_i * m_code_prompt_data * boc_prompt;
}

inline void E1Channel::update_epoch()
{
    update_cn0();
    update_state();
    update_filter_bandwidths();
    loop_filter(false);

    m_epochs_since_last_tow_sync++;

    if (m_state == E1_CHANNEL_STATE_TRACKING)
    {
        update_nav();
    }

#ifdef DEBUG_FILE
    *m_debug_file << m_ms_elapsed << "," << m_ip_pilot << "," << m_qp_pilot << "," << m_ie_pilot << "," << m_qe_pilot
                  << "," << m_il_pilot << "," << m_ql_pilot << "," << m_cn0 << "," << m_lo_error << "," << m_code_error
                  << "," << m_lo_rate << "," << m_code_rate << "\n";
#endif

    m_ms_elapsed += 4;

    // Reset accumulators
    m_prev_ip = m_ip_pilot;
    m_prev_qp = m_qp_pilot;

    m_ie_pilot = 0;
    m_qe_pilot = 0;
    m_ip_pilot = 0;
    m_qp_pilot = 0;
    m_il_pilot = 0;
    m_ql_pilot = 0;
    m_ie_pilot_no_sc = 0;
    m_qe_pilot_no_sc = 0;
    m_ip_pilot_no_sc = 0;
    m_qp_pilot_no_sc = 0;
    m_il_pilot_no_sc = 0;
    m_ql_pilot_no_sc = 0;

    m_ip_data = 0;
}

inline void E1Channel::update_epoch_1ms()
{
    loop_filter(true);

    m_ip_pilot_1ms = 0;
    m_qp_pilot_1ms = 0;
}

inline void E1Channel::update_state()
{
    // Try to transition from pull-in to tracking
    if (m_state == E1_CHANNEL_STATE_PULL_IN_PLL && m_cn0 >= E1_PULLIN_TO_TRACKING_CN0_THRESHOLD)
    {
        std::println("SV {}: Tracking", m_sv);
        m_state = E1_CHANNEL_STATE_TRACKING;
        m_code_pll.reset();
    }
}

inline void E1Channel::update_filter_bandwidths()
{
    switch (m_state)
    {
        case E1_CHANNEL_STATE_PULL_IN_PLL:
            m_lo_pll.set_bandwidth(E1_PULLIN_LO_FLL_BANDWIDTH_HZ, E1_PULLIN_LO_PLL_BANDWIDTH_HZ);
            m_code_pll.set_bandwidth(E1_PULLIN_CODE_PLL_BANDWIDTH_HZ);
            break;

        case E1_CHANNEL_STATE_TRACKING:
            m_lo_pll.set_bandwidth(E1_PULLIN_LO_FLL_BANDWIDTH_HZ, E1_TRACKING_LO_PLL_BANDWIDTH_HZ);
            m_code_pll.set_bandwidth(E1_TRACKING_CODE_PLL_BANDWIDTH_HZ);
            break;

        default:
            break;
    }
}

inline void E1Channel::loop_filter(bool epoch_1ms)
{
    if (m_state == E1_CHANNEL_STATE_PULL_IN_PLL && epoch_1ms)
    {
        m_lo_error = m_lo_pll.update(0.0, lo_phase_discriminator_1ms(), 0.001);
        m_lo_rate = (4 * (m_freq_if_hz + m_lo_error)) / m_freq_sample_hz;
    }
    else if (m_state == E1_CHANNEL_STATE_TRACKING && !epoch_1ms)
    {
        m_lo_error = m_lo_pll.update(0.0, lo_phase_discriminator(), 0.004);
        m_lo_rate = (4 * (m_freq_if_hz + m_lo_error)) / m_freq_sample_hz;
    }

    if (!epoch_1ms)
    {
        double carrier_aiding =
            (m_state >= E1_CHANNEL_STATE_TRACKING) ? (m_lo_error * GALILEO_E1_CODE_RATE_CPS / GALILEO_E1_FREQ_HZ) : 0.0;
        m_code_error = m_code_pll.update(code_phase_discriminator(), 0.004);
        m_code_rate = (GALILEO_E1_CODE_RATE_CPS + m_code_error + carrier_aiding) / m_freq_sample_hz;
    }
}

inline void E1Channel::update_cn0()
{
    m_cn0_estimator.update(m_ip_pilot, m_qp_pilot);
    if (m_cn0_estimator.is_full())
    {
        m_cn0 = m_cn0_estimator.get_cn0();
    }
}

inline double E1Channel::code_phase_discriminator()
{
    // ASPeCT Dot Product Discriminator
    double ie = static_cast<double>(m_ie_pilot);
    double qe = static_cast<double>(m_qe_pilot);
    double ip = static_cast<double>(m_ip_pilot);
    double qp = static_cast<double>(m_qp_pilot);
    double il = static_cast<double>(m_il_pilot);
    double ql = static_cast<double>(m_ql_pilot);
    double ie_no_sc = static_cast<double>(m_ie_pilot_no_sc);
    double qe_no_sc = static_cast<double>(m_qe_pilot_no_sc);
    double ip_no_sc = static_cast<double>(m_ip_pilot_no_sc);
    double qp_no_sc = static_cast<double>(m_qp_pilot_no_sc);
    double il_no_sc = static_cast<double>(m_il_pilot_no_sc);
    double ql_no_sc = static_cast<double>(m_ql_pilot_no_sc);

    double code_discriminator = 0.0;
    double num = ((ie - il) * ip + (qe - ql) * qp) -
                 E1_ASPECT_BETA * ((ie_no_sc - il_no_sc) * ip_no_sc + (qe_no_sc - ql_no_sc) * qp_no_sc);
    double denom = (6 + E1_ASPECT_BETA * 2 * E1_CHIP_SPACING) * (ip * ip + qp * qp);

    if (denom != 0)
    {
        code_discriminator = num / denom;
    }

    return code_discriminator;
}

inline double E1Channel::lo_phase_discriminator()
{
    double lo_discriminator = 0;
    if (m_ip_pilot != 0)
    {
        lo_discriminator =
            std::atan(static_cast<double>(m_qp_pilot) / static_cast<double>(m_ip_pilot)) / (2.0 * std::numbers::pi);
    }

    return lo_discriminator;
}

inline double E1Channel::lo_phase_discriminator_1ms()
{
    double lo_discriminator = 0;
    if (m_ip_pilot_1ms != 0)
    {
        lo_discriminator = std::atan(static_cast<double>(m_qp_pilot_1ms) / static_cast<double>(m_ip_pilot_1ms)) /
                           (2.0 * std::numbers::pi);
    }

    return lo_discriminator;
}

inline void E1Channel::update_nav()
{
    // Add latest nav bit to buffer
    m_bit_buffer[m_bit_count++] = (m_ip_data >= 0) ? 1 : 0;

    // Try to form a whole page half
    if (m_bit_count >= 250)
    {
        // First try to match the synchronization pattern
        constexpr std::array<uint8_t, 10> sync_norm = {0, 1, 0, 1, 1, 0, 0, 0, 0, 0};
        constexpr std::array<uint8_t, 10> sync_inv = {1, 0, 1, 0, 0, 1, 1, 1, 1, 1};
        uint8_t flip = 0;

        if (std::equal(m_bit_buffer.cbegin(), m_bit_buffer.cbegin() + 10, sync_norm.cbegin(), sync_norm.cend()))
        {
            flip = 0;
        }
        else if (std::equal(m_bit_buffer.cbegin(), m_bit_buffer.cbegin() + 10, sync_inv.cbegin(), sync_inv.cend()))
        {
            flip = 1;
        }
        else
        {
            // Synchronization pattern not found
            std::copy(m_bit_buffer.begin() + 1, m_bit_buffer.end(), m_bit_buffer.begin());
            m_bit_count--;
            m_even_received = false;
            return;
        }

        // Deinterleave and flip the bits
        auto interleaved_page_half = std::span(m_bit_buffer).last<240>();
        std::array<uint8_t, 240> deinterleaved_page_half;
        deinterleave_and_flip<8, 30>(flip, interleaved_page_half, deinterleaved_page_half);

        // Attempt to decode the page half
        std::array<uint8_t, 120> decoded_page_half;
        uint32_t metric = viterbi_decode(deinterleaved_page_half, decoded_page_half);

        if (metric > 0)
        {
            // Decoding failed
            std::copy(m_bit_buffer.begin() + 1, m_bit_buffer.end(), m_bit_buffer.begin());
            m_bit_count--;
            m_even_received = false;
            return;
        }

        // Check tail
        if (!std::all_of(decoded_page_half.cend() - 6, decoded_page_half.cend(), [](uint8_t bit) { return bit == 0; }))
        {
            // Tail check failed
            std::copy(m_bit_buffer.begin() + 1, m_bit_buffer.end(), m_bit_buffer.begin());
            m_bit_count--;
            m_even_received = false;
            return;
        }

        // Free the bit buffer for the next page half
        m_bit_count -= 250;

        // Page metadata
        bool odd_even = (decoded_page_half[0] == 1);
        bool page_nominal = (decoded_page_half[1] == 0);

        if (m_even_received && !odd_even)
        {
            // Even page was already received, and new page is even, should not happen
            std::println("SV {}: Unexpected even page half received", m_sv);
            m_even_received = false;
            return;
        }

        if (!m_even_received && odd_even)
        {
            // Even page was not received, new page is odd, should not happen
            std::println("SV {}: Unexpected odd page half received", m_sv);
            m_even_received = false;
            return;
        }

        if (!m_even_received && !odd_even && page_nominal)
        {
            // Even page was not yet received, and new page is even
            // Fill page buffer with the even half
            std::copy(decoded_page_half.begin() + 2, decoded_page_half.begin() + 114, m_page_buffer.begin());
            m_even_received = true;
            return;
        }

        if (m_even_received && odd_even && page_nominal)
        {
            // Even page was already received, and new page is odd
            // Fill page buffer with the odd half
            std::copy(decoded_page_half.begin() + 2, decoded_page_half.begin() + 18, m_page_buffer.begin() + 112);
            m_even_received = false;

            // Now pass to the ephemeris decoder
            uint8_t page_type = m_page_buffer[0] << 5 | m_page_buffer[1] << 4 | m_page_buffer[2] << 3 |
                                m_page_buffer[3] << 2 | m_page_buffer[4] << 1 | m_page_buffer[5];
            std::println("SV {}: Full page type {} received", m_sv, page_type);
            m_ephemeris.update(m_sv, m_page_buffer);

            if (page_type == 5)
            {
                // The TOW was updated, we can reset the epoch counter

                // The TOW is referenced to the start of the page, which was 500 symbols ago
                m_epochs_since_last_tow_sync = 500;
                m_tow_synced = true;
            }
        }
    }
}

double E1Channel::get_corrected_gps_time_of_week() const
{
    double tow = m_ephemeris.get_TOW(m_sv) + m_epochs_since_last_tow_sync * 0.004;
    // Account for rollover
    if (tow >= 604800.0)
    {
        tow -= 604800.0;
    }

    // Code phase is referenced to early chip, so the prompt is E1_CHIP_SPACING chips behind
    double code_phase_chips = m_code_gen.chip + m_code_phase - E1_CHIP_SPACING;

    double t = tow + (code_phase_chips / GALILEO_E1_CODE_RATE_CPS);
    t -= m_ephemeris.get_clock_correction(m_sv, t);
    t = m_ephemeris.gal_time_to_gps_time(m_sv, t);

    return t;
}