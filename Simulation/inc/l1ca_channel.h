#pragma once

#include "l1ca_code.h"
#include "constants.h"
#include "tools.h"
#include "filters.h"
#include "l1ca_ephemeris.h"

#include <array>
#include <cstdint>

#define DEBUG_FILE

#ifdef DEBUG_FILE
#include <fstream>
#endif

typedef enum
{
    L1CA_CHANNEL_STATE_IDLE = 0,
    L1CA_CHANNEL_STATE_PULL_IN_FLL = 1,
    L1CA_CHANNEL_STATE_PULL_IN_PLL = 2,
    L1CA_CHANNEL_STATE_TRACKING = 3,
    L1CA_CHANNEL_STATE_TRACKING_BIT_SYNCED = 4,
} L1CAChannelState_t;

class L1CAChannel
{
public:
    L1CAChannel() = delete;
    L1CAChannel(const double freq_sample_hz, const double freq_if_hz, L1CAEphemeris& ephemeris)
        : m_freq_sample_hz(freq_sample_hz), m_freq_if_hz(freq_if_hz), m_ephemeris(ephemeris) {};

    void start(int sv, double doppler, double code_phase);
    void stop();
    bool is_active() const { return m_state != L1CA_CHANNEL_STATE_IDLE; }
    void update(int sample);

    int get_sv() const { return m_sv; }
    int get_last_ip() const { return m_prev_ip; }
    int get_last_qp() const { return m_prev_qp; }
    double get_cn0() const { return m_cn0; }
    double get_corrected_gps_time_of_week() const;
    void get_satellite_ecef(double& x, double& y, double& z) const
    {
        m_ephemeris.get_satellite_ecef(m_sv, get_corrected_gps_time_of_week(), x, y, z);
    }
    bool can_solve() const
    {
        return m_nav_valid && m_ephemeris.is_ephemeris_valid(m_sv) && m_state == L1CA_CHANNEL_STATE_TRACKING_BIT_SYNCED;
    }

private:
    const std::array<int, 4> m_lo_sin = {1, 1, -1, -1};
    const std::array<int, 4> m_lo_cos = {1, -1, -1, 1};

    int m_sv = 0;
    L1CAChannelState_t m_state = L1CA_CHANNEL_STATE_IDLE;

    const double m_freq_sample_hz = 0.0;
    const double m_freq_if_hz = 0.0;

    // Code NCO
    double m_code_phase = 0.0;
    double m_code_rate = 0.0;

    // LO (carrier) NCO
    double m_lo_phase = 0.0;
    double m_lo_rate = 0.0;

    // Code generator
    L1CACode m_code_gen;

    // Code outputs
    int m_code_early = 0;
    int m_code_prompt = 0;
    int m_code_late = 0;

    // Accumulators
    int m_ie = 0;
    int m_qe = 0;
    int m_ip = 0;
    int m_qp = 0;
    int m_il = 0;
    int m_ql = 0;

    // Start of a new epoch?
    bool m_epoch = false;

    // CN0
    CN0Estimator<L1CA_CN0_ESTIMATOR_LENGTH> m_cn0_estimator;
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

    // Bit synchronization
    std::array<size_t, 20> m_bit_sync_histogram{0};
    size_t m_bit_ms = 0;
    size_t m_bit_sync_transition_count = 0;

    // Bit recovery
    int m_bit_total = 0;
    std::array<uint8_t, 300> m_bit_buffer{0};
    size_t m_bit_count = 0;
    int m_last_time_of_week = -2;
    bool m_nav_valid = false;

    // Ephemeris
    size_t m_epochs_since_last_nav_message = 0;
    L1CAEphemeris& m_ephemeris;

    // Private functions
    inline void update_sample(int sample);
    inline void update_epoch();
    inline void update_filter_bandwidths();
    inline void loop_filter();
    inline void update_cn0();
    inline double code_phase_discriminator();
    inline double lo_phase_discriminator();
    inline double lo_freq_discriminator();
    inline void update_bit_sync();
    inline void reset_bit_sync();
    inline void update_nav();
    inline bool check_parity(uint8_t flip, std::array<bool, 10>& subword_flips);
};