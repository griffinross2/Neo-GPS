#pragma once

#include <cstdint>
#include <array>
#include <print>

typedef struct E1Word1
{
    bool valid = false;
    uint16_t iod1 = 0;
    double t_0e = 0.0;
    double M_0 = 0.0;
    double e = 0.0;
    double sqrt_A = 0.0;
} E1Word1_t;

typedef struct E1Word2
{
    bool valid = false;
    uint16_t iod2 = 0;
    double Omega_0 = 0.0;
    double i_0 = 0.0;
    double omega = 0.0;
    double i_dot = 0.0;
} E1Word2_t;

typedef struct E1Word3
{
    bool valid = false;
    uint16_t iod3 = 0;
    double Omega_dot = 0.0;
    double delta_n = 0.0;
    double C_uc = 0.0;
    double C_us = 0.0;
    double C_rc = 0.0;
    double C_rs = 0.0;
} E1Word3_t;

typedef struct E1Word4
{
    bool valid = false;
    uint16_t iod4 = 0;
    double C_ic = 0.0;
    double C_is = 0.0;
    double t_0c = 0.0;
    double a_f0 = 0.0;
    double a_f1 = 0.0;
    double a_f2 = 0.0;
} E1Word4_t;

typedef struct E1Word5
{
    bool valid = false;
    double BGD = 0.0;
    bool data_valid = false;
    bool signal_healthy = false;
    size_t WN = 0;
    size_t TOW = 0;
} E1Word5_t;

typedef struct E1Word10
{
    bool valid = false;
    double A_0G = 0.0;
    double A_1G = 0.0;
    size_t t_0G = 0;
    size_t WN_0G = 0;
} E1Word10_t;

class E1Ephemeris
{
public:
    E1Ephemeris() = default;

    void update(size_t sv, std::array<uint8_t, 128>& data);

    size_t get_TOW(size_t sv) const { return m_current_word5_data[sv - 1].TOW; }
    bool is_ephemeris_valid(size_t sv) const
    {
        const E1Word1_t& word1 = m_current_word1_data[sv - 1];
        const E1Word2_t& word2 = m_current_word2_data[sv - 1];
        const E1Word3_t& word3 = m_current_word3_data[sv - 1];
        const E1Word4_t& word4 = m_current_word4_data[sv - 1];
        const E1Word5_t& word5 = m_current_word5_data[sv - 1];
        const E1Word10_t& word10 = m_current_word10_data[sv - 1];

        return word1.valid && word2.valid && word3.valid && word4.valid && word5.valid && word10.valid &&
               word1.iod1 == word2.iod2 && word2.iod2 == word3.iod3 && word3.iod3 == word4.iod4 && word5.data_valid &&
               word5.signal_healthy;
    }
    void get_satellite_ecef(size_t sv, double t, double& x, double& y, double& z) const;
    double get_clock_correction(size_t sv, double t) const;
    double gal_time_to_gps_time(size_t sv, double t) const;

private:
    // Ephemeris data
    std::array<E1Word1_t, 36> m_current_word1_data;
    std::array<E1Word2_t, 36> m_current_word2_data;
    std::array<E1Word3_t, 36> m_current_word3_data;
    std::array<E1Word4_t, 36> m_current_word4_data;
    std::array<E1Word5_t, 36> m_current_word5_data;
    std::array<E1Word10_t, 36> m_current_word10_data;

    std::array<E1Word1_t, 36> m_next_word1_data;
    std::array<E1Word2_t, 36> m_next_word2_data;
    std::array<E1Word3_t, 36> m_next_word3_data;
    std::array<E1Word4_t, 36> m_next_word4_data;

    // Private functions
    double time_from_epoch(double t, double t_epoch) const;
    double eccentric_anomaly(size_t sv, double t_k) const;
};