#pragma once

#include <cstdint>
#include <array>
#include <print>

typedef struct L1CASubframe1
{
    // Subframe 1
    bool valid = false;
    uint16_t week_number = 0;
    uint8_t ura = 0;
    bool sv_healthy = false;
    uint16_t iodc = 0;
    double T_GD = 0.0;
    double t_oc = 0.0;
    double a_f2 = 0.0;
    double a_f1 = 0.0;
    double a_f0 = 0.0;
} L1CASubframe1_t;

typedef struct L1CASubframe2
{
    // Subframe 2
    bool valid = false;
    uint8_t iode_2 = 0;
    double C_rs = 0.0;
    double delta_n = 0.0;
    double M_0 = 0.0;
    double C_uc = 0.0;
    double e = 0.0;
    double C_us = 0.0;
    double sqrt_A = 0.0;
    double t_oe = 0.0;
} L1CASubframe2_t;

typedef struct L1CASubframe3
{
    // Subframe 3
    bool valid = false;
    double C_ic = 0.0;
    double Omega_0 = 0.0;
    double C_is = 0.0;
    double i_0 = 0.0;
    double C_rc = 0.0;
    double omega = 0.0;
    double Omega_dot = 0.0;
    uint8_t iode_3 = 0;
    double idot = 0.0;
} L1CASubframe3_t;

class L1CAEphemeris
{
public:
    L1CAEphemeris() = default;

    void update(size_t sv, const std::array<uint8_t, 300>& data);

    uint16_t get_week_number(size_t sv) const { return m_current_subframe1_data[sv - 1].week_number; }
    bool is_ephemeris_valid(size_t sv) const
    {
        return m_current_subframe1_data[sv - 1].sv_healthy && m_current_subframe1_data[sv - 1].valid &&
               m_current_subframe2_data[sv - 1].valid && m_current_subframe3_data[sv - 1].valid &&
               (m_current_subframe1_data[sv - 1].iodc & 0xFF) == m_current_subframe2_data[sv - 1].iode_2 &&
               m_current_subframe2_data[sv - 1].iode_2 == m_current_subframe3_data[sv - 1].iode_3;
    }
    void get_satellite_ecef(size_t sv, double t, double& x, double& y, double& z) const;
    double get_clock_correction(size_t sv, double t) const;

private:
    // Ephemeris data
    std::array<L1CASubframe1_t, 32> m_current_subframe1_data;
    std::array<L1CASubframe2_t, 32> m_current_subframe2_data;
    std::array<L1CASubframe3_t, 32> m_current_subframe3_data;

    std::array<L1CASubframe2_t, 32> m_next_subframe2_data;
    std::array<L1CASubframe3_t, 32> m_next_subframe3_data;

    // Private functions
    double time_from_epoch(double t, double t_epoch) const;
    double eccentric_anomaly(size_t sv, double t_k) const;
};