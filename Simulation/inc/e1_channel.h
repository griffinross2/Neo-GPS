#pragma once

#include "e1_code.h"
#include "e1_ephemeris.h"
#include "constants.h"
#include "tools.h"
#include "filters.h"

#include <array>
#include <cstdint>

#define DEBUG_FILE

#ifdef DEBUG_FILE
#include <fstream>
#endif

typedef enum
{
    E1_CHANNEL_STATE_IDLE = 0,
    E1_CHANNEL_STATE_PULL_IN_PLL = 1,
    E1_CHANNEL_STATE_TRACKING = 2,
} E1ChannelState_t;

class E1Channel
{
public:
    E1Channel() = delete;
    E1Channel(const double freq_sample_hz, const double freq_if_hz);

    void start(int sv, double doppler, double code_phase);
    void stop();
    bool is_active() const { return m_state != E1_CHANNEL_STATE_IDLE; }
    void update(int sample);
    double get_corrected_gps_time_of_week() const;
    void get_satellite_ecef(double& x, double& y, double& z) const
    {
        m_ephemeris.get_satellite_ecef(get_corrected_gps_time_of_week(), x, y, z);
    }
    bool can_solve() const { return m_ephemeris.is_ephemeris_valid() && m_state == E1_CHANNEL_STATE_TRACKING; }

    int get_sv() const { return m_sv; }
    int get_last_ip() const { return m_prev_ip; }
    int get_last_qp() const { return m_prev_qp; }
    double get_cn0() const { return m_cn0; }

private:
    const std::array<int, 4> m_lo_sin = {1, 1, -1, -1};
    const std::array<int, 4> m_lo_cos = {1, -1, -1, 1};

    int m_sv = 0;
    E1ChannelState_t m_state = E1_CHANNEL_STATE_IDLE;

    const double m_freq_sample_hz = 0.0;
    const double m_freq_if_hz = 0.0;

    // Code NCO
    double m_code_phase = 0.0;
    double m_code_rate = 0.0;

    // LO (carrier) NCO
    double m_lo_phase = 0.0;
    double m_lo_rate = 0.0;

    // Code generator
    E1Code m_code_gen;

    // Code outputs
    int m_code_early_pilot = 0;
    int m_code_prompt_pilot = 0;
    int m_code_late_pilot = 0;

    int m_code_prompt_data = 0;

    // Subcarrier outputs
    int m_subcarrier_early = 0;
    int m_subcarrier_prompt = 0;
    int m_subcarrier_late = 0;

    // Accumulators
    int m_ie_pilot = 0;
    int m_qe_pilot = 0;
    int m_ip_pilot = 0;
    int m_qp_pilot = 0;
    int m_il_pilot = 0;
    int m_ql_pilot = 0;

    int m_ip_pilot_1ms = 0;
    int m_qp_pilot_1ms = 0;

    int m_ie_pilot_no_sc = 0;
    int m_qe_pilot_no_sc = 0;
    int m_ip_pilot_no_sc = 0;
    int m_qp_pilot_no_sc = 0;
    int m_il_pilot_no_sc = 0;
    int m_ql_pilot_no_sc = 0;

    int m_ip_data = 0;

    // Start of a new epoch?
    bool m_epoch = false;
    bool m_epoch_1ms = false;

    // CN0
    CN0Estimator<E1_CN0_ESTIMATOR_LENGTH> m_cn0_estimator;
    double m_cn0 = -100.0;

    // Loop filters
    SecondOrderPLL m_code_pll;
    ThirdOrderFLLAssistedPLL m_lo_pll;

    // Previous accumulator values
    int m_prev_ip = 0;
    int m_prev_qp = 0;

    // Elapsed time
    uint64_t m_ms_elapsed = 0;

    // Code and LO error (doppler)
    double m_lo_error = 0.0;
    double m_code_error = 0.0;

    // Debug file
#ifdef DEBUG_FILE
    std::unique_ptr<std::ofstream> m_debug_file;
#endif

    // Nav data
    size_t m_bit_count = 0;
    std::array<uint8_t, 250> m_bit_buffer;
    std::array<uint8_t, 128> m_page_buffer;
    bool m_even_received = false;
    size_t m_epochs_since_last_tow_sync = 0;

    // Ephemeris
    E1Ephemeris m_ephemeris;

    // Private functions
    inline void update_sample(int sample);
    inline void update_epoch();
    inline void update_epoch_1ms();
    inline void update_filter_bandwidths();
    inline void loop_filter(bool epoch_1ms);
    inline void update_state();
    inline void update_cn0();
    inline double code_phase_discriminator();
    inline double lo_phase_discriminator();
    inline double lo_phase_discriminator_1ms();
    inline void update_nav();
};