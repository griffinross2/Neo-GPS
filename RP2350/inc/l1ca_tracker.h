#pragma once

#include "constants.h"
#include "filters.h"
#include "tools.h"

typedef enum
{
    L1CA_TRACKING_WIDE_FLL,
    L1CA_TRACKING_NARROW_FLL,
    L1CA_TRACKING_WIDE_PLL,
    L1CA_TRACKING_NARROW_PLL,
} L1CATrackingMode;

class L1CATracker
{
public:
    L1CATracker(float freq_sample_hz, float freq_if_hz);

    void reset(size_t sv, float doppler);
    void update(int ie, int qe, int ip, int qp, int il, int ql, float int_time);
    uint32_t get_lo_rate() const { return static_cast<uint32_t>(m_lo_rate * std::powf(2, 32)); }
    uint32_t get_code_rate() const { return static_cast<uint32_t>(m_code_rate * std::powf(2, 32)); }
    L1CATrackingMode get_mode() const { return m_last_mode; }
    size_t get_sv() const { return m_sv; }
    float get_cn0() const { return m_cn0; }
    float get_elapsed_time_s() const { return m_elapsed_time_s; }
    bool is_bit_synced() const { return m_bit_synced; }
    bool is_message_ready() const { return m_message_ready; }
    bool is_epochs_synced() const { return m_epochs_synced; }
    void sync_epoch_counter();
    const std::array<uint8_t, 300>& get_waiting_message_buffer()
    {
        m_message_ready = false;
        return m_waiting_message_buffer;
    }
    bool can_solve() const { return m_nav_valid && m_last_mode == L1CA_TRACKING_NARROW_PLL && m_bit_synced; }

private:
    float code_phase_discriminator();
    float lo_phase_discriminator();
    float lo_freq_discriminator();
    void update_bit_sync();
    void reset_bit_sync();
    void nav_decode();
    bool check_parity(uint8_t flip, std::array<bool, 10>& subword_flips);

    size_t m_sv = 0;

    int m_ie;
    int m_qe;
    int m_ip;
    int m_qp;
    int m_il;
    int m_ql;

    int m_prev_ip;
    int m_prev_qp;

    const float m_freq_sample_hz;
    const float m_freq_if_hz;
    float m_lo_error;
    float m_code_error;
    float m_lo_rate;
    float m_code_rate;
    float m_cn0;
    float m_elapsed_time_s;

    L1CATrackingMode m_last_mode;
    SecondOrderPLL m_code_pll;
    ThirdOrderFLLAssistedPLL m_lo_pll;
    CN0Estimator<L1CA_CN0_ESTIMATOR_LENGTH> m_cn0_estimator;

    // Bit synchronization
    bool m_bit_synced = false;
    std::array<size_t, 20> m_bit_sync_histogram{0};
    size_t m_bit_ms = 0;
    size_t m_bit_sync_transition_count = 0;

    // Bit recovery
    int m_bit_total = 0;
    std::array<uint8_t, 300> m_bit_buffer;
    size_t m_bit_buffer_count = 0;

    int m_last_time_of_week = -2;
    bool m_nav_valid = false;
    int m_epochs_since_last_nav_message = 0;
    bool m_epochs_synced = false;

    std::array<uint8_t, 300> m_waiting_message_buffer;
    bool m_message_ready = false;
};