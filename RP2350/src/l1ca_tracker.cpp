#include "l1ca_tracker.h"

#include "gps_if.h"

#include "pico/stdlib.h"

L1CATracker::L1CATracker(const float freq_sample_hz, const float freq_if_hz)
    : m_freq_sample_hz(freq_sample_hz), m_freq_if_hz(freq_if_hz)
{
    reset(0, 0.0f);
}

void L1CATracker::reset(size_t sv, float doppler)
{
    m_sv = sv;
    m_lo_rate = m_freq_if_hz / m_freq_sample_hz;
    m_code_rate = GPS_L1CA_CODE_RATE_CPS / m_freq_sample_hz;
    m_cn0 = -100.0f;
    m_last_mode = L1CA_TRACKING_WIDE_FLL;
    m_lo_error = doppler;
    m_code_error = doppler * GPS_L1CA_CODE_RATE_CPS / GPS_L1CA_FREQ_HZ;
    m_elapsed_time_s = 0.0f;

    m_ie = 0;
    m_qe = 0;
    m_ip = 0;
    m_qp = 0;
    m_il = 0;
    m_ql = 0;
    m_prev_ip = 0;
    m_prev_qp = 0;

    m_code_pll = SecondOrderPLL(L1CA_PULLIN_CODE_PLL_BANDWIDTH_HZ, m_code_error);
    m_lo_pll = ThirdOrderFLLAssistedPLL(L1CA_PULLIN_LO_FLL_BANDWIDTH_HZ, 0.0f, m_lo_error);
    m_cn0_estimator.reset(0.001f);

    m_bit_synced = false;
    m_bit_ms = 0;
    reset_bit_sync();

    m_bit_total = 0;
    m_bit_buffer_count = 0;
    m_bit_buffer.fill(0);

    m_last_time_of_week = -2;
    m_nav_valid = false;
    m_epochs_since_last_nav_message = 0;
    m_epochs_synced = false;
    m_message_ready = false;
    m_waiting_message_buffer.fill(0);
}

void __not_in_flash_func(L1CATracker::update)(int ie, int qe, int ip, int qp, int il, int ql, float int_time)
{
    m_ie = ie;
    m_qe = qe;
    m_ip = ip;
    m_qp = qp;
    m_il = il;
    m_ql = ql;

    L1CATrackingMode mode = m_last_mode;

    // Decide if we should change the mode
    if (m_last_mode == L1CA_TRACKING_WIDE_FLL && m_elapsed_time_s > L1CA_PULLIN_TIME_S)
    {
        mode = L1CA_TRACKING_WIDE_PLL;
    }
    else if (m_last_mode == L1CA_TRACKING_WIDE_PLL && m_cn0 > L1CA_PULLIN_TO_TRACKING_CN0_THRESHOLD)
    {
        mode = L1CA_TRACKING_NARROW_PLL;
    }

    if (mode == L1CA_TRACKING_WIDE_FLL && mode != m_last_mode)
    {
        m_lo_pll.set_bandwidth(L1CA_PULLIN_LO_FLL_BANDWIDTH_HZ, 0.0f);
        m_code_pll.set_bandwidth(L1CA_PULLIN_CODE_PLL_BANDWIDTH_HZ);
    }
    else if (mode == L1CA_TRACKING_NARROW_FLL && mode != m_last_mode)
    {
        m_lo_pll.set_bandwidth(L1CA_PULLIN_LO_FLL_BANDWIDTH_HZ, 0.0f);
        m_code_pll.set_bandwidth(L1CA_PULLIN_CODE_PLL_BANDWIDTH_HZ);
    }
    else if (mode == L1CA_TRACKING_WIDE_PLL && mode != m_last_mode)
    {
        m_lo_pll.set_bandwidth(0.0f, L1CA_TRACKING_LO_PLL_BANDWIDTH_HZ);
        m_code_pll.set_bandwidth(L1CA_PULLIN_CODE_PLL_BANDWIDTH_HZ);
    }
    else if (mode == L1CA_TRACKING_NARROW_PLL && mode != m_last_mode)
    {
        m_lo_pll.set_bandwidth(0.0f, L1CA_TRACKING_LO_PLL_BANDWIDTH_HZ);
        m_code_pll.set_bandwidth(L1CA_TRACKING_CODE_PLL_BANDWIDTH_HZ);
        m_code_pll.reset();  // Switch to carrier aided mode
    }

    switch (mode)
    {
        case L1CA_TRACKING_WIDE_FLL:
        case L1CA_TRACKING_NARROW_FLL:
            m_lo_error = m_lo_pll.update(lo_freq_discriminator(), 0.0f, int_time);
            m_code_error = m_code_pll.update(code_phase_discriminator(), int_time);
            break;
        case L1CA_TRACKING_WIDE_PLL:
        case L1CA_TRACKING_NARROW_PLL:
            m_lo_error = m_lo_pll.update(0.0f, lo_phase_discriminator(), int_time);
            m_code_error = m_code_pll.update(code_phase_discriminator(), int_time);
            break;
    }

    m_lo_rate = (m_freq_if_hz + m_lo_error) / m_freq_sample_hz;
    double carrier_aiding =
        (mode == L1CA_TRACKING_NARROW_PLL) ? (m_lo_error * GPS_L1CA_CODE_RATE_CPS / GPS_L1CA_FREQ_HZ) : 0.0;
    m_code_rate = (GPS_L1CA_CODE_RATE_CPS + m_code_error + carrier_aiding) / m_freq_sample_hz;

    m_cn0_estimator.update(m_ip, m_qp);
    m_cn0 = m_cn0_estimator.get_cn0();

    if (mode == L1CA_TRACKING_NARROW_PLL && !m_bit_synced)
    {
        update_bit_sync();
    }

    // Recover a bit
    if (m_bit_synced && m_bit_ms == 0)
    {
        m_bit_buffer.at(m_bit_buffer_count) = m_bit_total > 0 ? 1 : 0;
        m_bit_buffer_count++;
        m_bit_total = 0;
    }
    m_bit_total += m_ip;
    m_epochs_since_last_nav_message = (m_epochs_since_last_nav_message + 1) % 6000;

    // Decode
    nav_decode();

    m_last_mode = mode;
    m_prev_ip = m_ip;
    m_prev_qp = m_qp;
    m_elapsed_time_s += int_time;
    m_bit_ms = (m_bit_ms + 1) % 20;
}

float __not_in_flash_func(L1CATracker::code_phase_discriminator)()
{
    // Normalized early-late power discriminator
    float code_discriminator = 0.0f;
    float power_early = std::sqrt(m_ie * m_ie + m_qe * m_qe);
    float power_late = std::sqrt(m_il * m_il + m_ql * m_ql);
    if (power_early + power_late != 0.0f)
    {
        code_discriminator = 0.5f * ((power_early - power_late) / (power_early + power_late));
    }

    return code_discriminator;
}

float __not_in_flash_func(L1CATracker::lo_phase_discriminator)()
{
    float lo_discriminator = 0.0f;
    if (m_ip != 0)
    {
        lo_discriminator = std::atan(static_cast<float>(m_qp) / static_cast<float>(m_ip)) / (2.0f * std::numbers::pi);
    }

    return lo_discriminator;
}

float __not_in_flash_func(L1CATracker::lo_freq_discriminator)()
{
    float cross = m_prev_ip * m_qp - m_ip * m_prev_qp;
    float dot = m_prev_ip * m_ip + m_prev_qp * m_qp;

    float lo_discriminator_fll = atan2(cross, dot) / (2.0f * std::numbers::pi * 0.001f);  // Hz

    return lo_discriminator_fll;
}

void __not_in_flash_func(L1CATracker::update_bit_sync)()
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
            m_bit_synced = true;
        }
        else
        {
            m_bit_synced = false;
        }

        reset_bit_sync();
    }
}

void __not_in_flash_func(L1CATracker::reset_bit_sync)()
{
    m_bit_sync_histogram.fill(0);
    m_bit_sync_transition_count = 0;
}

bool __not_in_flash_func(L1CATracker::check_parity)(uint8_t flip, std::array<bool, 10>& subword_flips)
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

void __not_in_flash_func(L1CATracker::nav_decode)()
{
    if (m_bit_buffer_count >= 300)
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
            m_bit_buffer_count--;
            return;
        }

        // Check parity
        std::array<bool, 10> subword_flips{0};
        if (!check_parity(flip, subword_flips))
        {
            // Parity check failed
            std::copy(m_bit_buffer.begin() + 1, m_bit_buffer.end(), m_bit_buffer.begin());
            m_bit_buffer_count--;
            return;
        }

        // Flip the bits if necessary
        for (size_t i = 0; i < m_bit_buffer.size(); i++)
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
            m_bit_buffer_count--;
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

        // Copy the decoded message to the waiting message buffer
        std::copy(m_bit_buffer.begin(), m_bit_buffer.end(), m_waiting_message_buffer.begin());
        m_message_ready = true;

        m_bit_buffer_count = 0;
        return;
    }
}

void L1CATracker::sync_epoch_counter()
{
    m_epochs_synced = true;
}