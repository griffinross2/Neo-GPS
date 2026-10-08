#pragma once

#include <cstdint>
#include <array>
#include <print>

class E1Ephemeris
{
public:
    E1Ephemeris() = default;

    void update(std::array<uint8_t, 128>& data);

    size_t get_TOW() const { return TOW; }
    bool is_ephemeris_valid() const
    {
        return word1_valid && word2_valid && word3_valid && word4_valid && word5_valid && word10_valid &&
               iod1 == iod2 && iod2 == iod3 && iod3 == iod4 && data_valid && signal_healthy;
    }
    void get_satellite_ecef(double t, double& x, double& y, double& z) const;
    double get_clock_correction(double t) const;
    double gal_time_to_gps_time(double t) const;

private:
    // m_ notation is omitted here because there are so many variables with weird names and I would get confused.

    // Word 1 - Ephemeris 1/4
    bool word1_valid = false;
    uint16_t iod1;
    double t_0e = 0.0;
    double M_0 = 0.0;
    double e = 0.0;
    double sqrt_A = 0.0;

    // Word 2 - Ephemeris 2/4
    bool word2_valid = false;
    uint16_t iod2;
    double Omega_0 = 0.0;
    double i_0 = 0.0;
    double omega = 0.0;
    double i_dot = 0.0;

    // Word 3 - Ephemeris 3/4
    bool word3_valid = false;
    uint16_t iod3;
    double Omega_dot = 0.0;
    double delta_n = 0.0;
    double C_uc = 0.0;
    double C_us = 0.0;
    double C_rc = 0.0;
    double C_rs = 0.0;

    // Word 4 - Ephemeris 4
    bool word4_valid = false;
    uint16_t iod4;
    double C_ic = 0.0;
    double C_is = 0.0;
    double t_0c = 0.0;
    double a_f0 = 0.0;
    double a_f1 = 0.0;
    double a_f2 = 0.0;

    // Word 5 - Ionospheric correction, BGD, signal health, data validity, GST
    bool word5_valid = false;
    double BGD = 0.0;
    bool data_valid = false;
    bool signal_healthy = false;
    size_t WN = 0;
    size_t TOW = 0;

    // Word 10 - Almanac for SVID3 (2/2) and GST-GPS correction parameters
    bool word10_valid = false;
    double A_0G = 0.0;
    double A_1G = 0.0;
    size_t t_0G = 0;
    size_t WN_0G = 0;

    // Private functions
    double time_from_epoch(double t, double t_epoch) const;
    double eccentric_anomaly(double t_k) const;
};