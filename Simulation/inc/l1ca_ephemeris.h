#pragma once

#include <cstdint>
#include <array>
#include <print>

class L1CAEphemeris
{
public:
    L1CAEphemeris() = default;

    void update(std::array<uint8_t, 300>& data);

    uint16_t get_week_number() const { return week_number; }
    bool is_ephemeris_valid() const
    {
        return sv_healthy && subframe1_valid && subframe2_valid && subframe3_valid && (iodc & 0xFF) == iode_2 &&
               iode_2 == iode_3;
    }
    void get_satellite_ecef(double t, double& x, double& y, double& z) const;
    double get_clock_correction(double t) const;

private:
    // m_ notation is omitted here because there are so many variables with weird names and I would get confused.

    // Subframe 1
    bool subframe1_valid = false;
    uint16_t week_number = 0;
    uint8_t ura = 0;
    bool sv_healthy = false;
    uint16_t iodc = 0;
    double T_GD = 0.0;
    double t_oc = 0.0;
    double a_f2 = 0.0;
    double a_f1 = 0.0;
    double a_f0 = 0.0;

    // Subframe 2
    bool subframe2_valid = false;
    uint8_t iode_2 = 0;
    double C_rs = 0.0;
    double delta_n = 0.0;
    double M_0 = 0.0;
    double C_uc = 0.0;
    double e = 0.0;
    double C_us = 0.0;
    double sqrt_A = 0.0;
    double t_oe = 0.0;

    // Subframe 3
    bool subframe3_valid = false;
    double C_ic = 0.0;
    double Omega_0 = 0.0;
    double C_is = 0.0;
    double i_0 = 0.0;
    double C_rc = 0.0;
    double omega = 0.0;
    double Omega_dot = 0.0;
    uint8_t iode_3 = 0;
    double idot = 0.0;

    // Private functions
    double time_from_epoch(double t, double t_epoch) const;
    double eccentric_anomaly(double t_k) const;
};