#include "e1_ephemeris.h"

#include "tools.h"

#include <print>
#include <numbers>

constexpr double E1_F = -4.442807309e-10;
constexpr double E1_OMEGA_E = 7.2921151467e-5;  // rad/s
constexpr double E1_MU = 3.986004418e14;        // m^3/s^2
constexpr size_t E_K_MAX_ITER = 100;

void E1Ephemeris::update(std::array<uint8_t, 128>& data)
{
    uint8_t word_id;
    bytes_to_number(&word_id, data.data(), sizeof(word_id) * 8, 6, false);

    if (word_id == 1)
    {
        bytes_to_number(&iod1, data.data() + 6, sizeof(iod1) * 8, 10, false);
        uint16_t t_0e_raw = 0;
        bytes_to_number(&t_0e_raw, data.data() + 16, sizeof(t_0e_raw) * 8, 14, false);
        t_0e = t_0e_raw * 60.0;
        int32_t M_0_raw = 0;
        bytes_to_number(&M_0_raw, data.data() + 30, sizeof(M_0_raw) * 8, 32, true);
        M_0 = M_0_raw * std::pow(2.0, -31) * std::numbers::pi;
        uint32_t e_raw = 0;
        bytes_to_number(&e_raw, data.data() + 62, sizeof(e_raw) * 8, 32, false);
        e = e_raw * std::pow(2.0, -33);
        uint32_t sqrt_A_raw = 0;
        bytes_to_number(&sqrt_A_raw, data.data() + 94, sizeof(sqrt_A_raw) * 8, 32, false);
        sqrt_A = sqrt_A_raw * std::pow(2.0, -19);

        word1_valid = true;
    }
    else if (word_id == 2)
    {
        bytes_to_number(&iod2, data.data() + 6, sizeof(iod2) * 8, 10, false);
        int32_t Omega_0_raw = 0;
        bytes_to_number(&Omega_0_raw, data.data() + 16, sizeof(Omega_0_raw) * 8, 32, true);
        Omega_0 = Omega_0_raw * std::pow(2.0, -31) * std::numbers::pi;
        int32_t i_0_raw = 0;
        bytes_to_number(&i_0_raw, data.data() + 48, sizeof(i_0_raw) * 8, 32, true);
        i_0 = i_0_raw * std::pow(2.0, -31) * std::numbers::pi;
        int32_t omega_raw = 0;
        bytes_to_number(&omega_raw, data.data() + 80, sizeof(omega_raw) * 8, 32, true);
        omega = omega_raw * std::pow(2.0, -31) * std::numbers::pi;
        int16_t i_dot_raw = 0;
        bytes_to_number(&i_dot_raw, data.data() + 112, sizeof(i_dot_raw) * 8, 14, true);
        i_dot = i_dot_raw * std::pow(2.0, -43) * std::numbers::pi;

        word2_valid = true;
    }
    else if (word_id == 3)
    {
        bytes_to_number(&iod3, data.data() + 6, sizeof(iod3) * 8, 10, false);
        int32_t Omega_dot_raw = 0;
        bytes_to_number(&Omega_dot_raw, data.data() + 16, sizeof(Omega_dot_raw) * 8, 24, true);
        Omega_dot = Omega_dot_raw * std::pow(2.0, -43) * std::numbers::pi;
        int16_t delta_n_raw = 0;
        bytes_to_number(&delta_n_raw, data.data() + 40, sizeof(delta_n_raw) * 8, 16, true);
        delta_n = delta_n_raw * std::pow(2.0, -43) * std::numbers::pi;
        int16_t C_uc_raw = 0;
        bytes_to_number(&C_uc_raw, data.data() + 56, sizeof(C_uc_raw) * 8, 16, true);
        C_uc = C_uc_raw * std::pow(2.0, -29);
        int16_t C_us_raw = 0;
        bytes_to_number(&C_us_raw, data.data() + 72, sizeof(C_us_raw) * 8, 16, true);
        C_us = C_us_raw * std::pow(2.0, -29);
        int16_t C_rc_raw = 0;
        bytes_to_number(&C_rc_raw, data.data() + 88, sizeof(C_rc_raw) * 8, 16, true);
        C_rc = C_rc_raw * std::pow(2.0, -5);
        int16_t C_rs_raw = 0;
        bytes_to_number(&C_rs_raw, data.data() + 104, sizeof(C_rs_raw) * 8, 16, true);
        C_rs = C_rs_raw * std::pow(2.0, -5);

        word3_valid = true;
    }
    else if (word_id == 4)
    {
        bytes_to_number(&iod4, data.data() + 6, sizeof(iod4) * 8, 10, false);
        int16_t C_ic_raw = 0;
        bytes_to_number(&C_ic_raw, data.data() + 22, sizeof(C_ic_raw) * 8, 16, true);
        C_ic = C_ic_raw * std::pow(2.0, -29);
        int16_t C_is_raw = 0;
        bytes_to_number(&C_is_raw, data.data() + 38, sizeof(C_is_raw) * 8, 16, true);
        C_is = C_is_raw * std::pow(2.0, -29);
        uint16_t t_0c_raw = 0;
        bytes_to_number(&t_0c_raw, data.data() + 54, sizeof(t_0c_raw) * 8, 14, false);
        t_0c = t_0c_raw * 60.0;
        int32_t a_f0_raw = 0;
        bytes_to_number(&a_f0_raw, data.data() + 68, sizeof(a_f0_raw) * 8, 31, true);
        a_f0 = a_f0_raw * std::pow(2.0, -34);
        int32_t a_f1_raw = 0;
        bytes_to_number(&a_f1_raw, data.data() + 99, sizeof(a_f1_raw) * 8, 21, true);
        a_f1 = a_f1_raw * std::pow(2.0, -46);
        int8_t a_f2_raw = 0;
        bytes_to_number(&a_f2_raw, data.data() + 120, sizeof(a_f2_raw) * 8, 6, true);
        a_f2 = a_f2_raw * std::pow(2.0, -59);

        word4_valid = true;
    }
    else if (word_id == 5)
    {
        int16_t BGD_raw = 0;
        bytes_to_number(&BGD_raw, data.data() + 57, sizeof(BGD_raw) * 8, 10, true);
        BGD = BGD_raw * std::pow(2.0, -32);
        uint8_t signal_health_raw = 0;
        bytes_to_number(&signal_health_raw, data.data() + 69, sizeof(signal_health_raw) * 8, 2, false);
        signal_healthy = (signal_health_raw == 0);
        uint8_t data_valid_raw = 0;
        bytes_to_number(&data_valid_raw, data.data() + 72, sizeof(data_valid_raw) * 8, 1, false);
        data_valid = (data_valid_raw == 0);
        bytes_to_number(&WN, data.data() + 73, sizeof(WN) * 8, 12, false);
        bytes_to_number(&TOW, data.data() + 85, sizeof(TOW) * 8, 20, false);

        word5_valid = true;
    }
    else if (word_id == 10)
    {
        int16_t A_0G_raw = 0;
        bytes_to_number(&A_0G_raw, data.data() + 86, sizeof(A_0G_raw) * 8, 16, true);
        A_0G = A_0G_raw * std::pow(2.0, -35);
        int16_t A_1G_raw = 0;
        bytes_to_number(&A_1G_raw, data.data() + 102, sizeof(A_1G_raw) * 8, 12, true);
        A_1G = A_1G_raw * std::pow(2.0, -51);
        bytes_to_number(&t_0G, data.data() + 114, sizeof(t_0G) * 8, 8, false);
        t_0G *= 3600;
        bytes_to_number(&WN_0G, data.data() + 122, sizeof(WN_0G) * 8, 6, false);

        if (A_0G_raw == -1 && A_1G_raw == -1 && t_0G == 0xFF && WN_0G == 0x3F)
        {
            word10_valid = false;
        }
        else
        {
            word10_valid = true;
        }
    }
}

void E1Ephemeris::get_satellite_ecef(double t, double& x, double& y, double& z) const
{
    double A = sqrt_A * sqrt_A;
    double t_k = time_from_epoch(t, t_0e);
    double E_k = eccentric_anomaly(t_k);
    double v_k = std::atan2(std::sqrt(1 - e * e) * std::sin(E_k), std::cos(E_k) - e);
    double phi_k = v_k + omega;
    double delta_u_k = C_us * std::sin(2 * phi_k) + C_uc * std::cos(2 * phi_k);
    double delta_r_k = C_rs * std::sin(2 * phi_k) + C_rc * std::cos(2 * phi_k);
    double delta_i_k = C_is * std::sin(2 * phi_k) + C_ic * std::cos(2 * phi_k);
    double u_k = phi_k + delta_u_k;
    double r_k = A * (1 - e * std::cos(E_k)) + delta_r_k;
    double i_k = i_0 + delta_i_k + i_dot * t_k;
    double Omega_k = Omega_0 + (Omega_dot - E1_OMEGA_E) * t_k - E1_OMEGA_E * t_0e;
    double x_k_prime = r_k * std::cos(u_k);
    double y_k_prime = r_k * std::sin(u_k);

    x = x_k_prime * std::cos(Omega_k) - y_k_prime * std::cos(i_k) * std::sin(Omega_k);
    y = x_k_prime * std::sin(Omega_k) + y_k_prime * std::cos(i_k) * std::cos(Omega_k);
    z = y_k_prime * std::sin(i_k);
}

double E1Ephemeris::get_clock_correction(double t) const
{
    double t_k = time_from_epoch(t, t_0e);
    double E_k = eccentric_anomaly(t_k);
    double t_r = E1_F * e * sqrt_A * std::sin(E_k);
    t = time_from_epoch(t, t_0c);

    return a_f0 + a_f1 * t + a_f2 * (t * t) + t_r - BGD;
}

double E1Ephemeris::time_from_epoch(double t, double t_epoch) const
{
    t -= t_epoch;
    if (t > 302400.0)
    {
        t -= 604800.0;
    }
    else if (t < -302400.0)
    {
        t += 604800.0;
    }
    return t;
}

double E1Ephemeris::eccentric_anomaly(double t_k) const
{
    double A = sqrt_A * sqrt_A;
    double n_0 = std::sqrt(E1_MU / (A * A * A));
    double n = n_0 + delta_n;
    double M_k = M_0 + n * t_k;
    double E_k = M_k;

    size_t iter = 0;
    while (iter++ < E_K_MAX_ITER)
    {
        double E_k_new = M_k + e * std::sin(E_k);
        if (std::fabs(E_k_new - E_k) < 1e-10)
        {
            E_k = E_k_new;
            break;
        }
        E_k = E_k_new;
    }
    return E_k;
}

double E1Ephemeris::gal_time_to_gps_time(double t) const
{
    int week_delta = (WN % 64) - (WN_0G);
    if (week_delta > 31)
    {
        week_delta -= 64;
    }
    if (week_delta < -31)
    {
        week_delta += 64;
    }
    double dt = A_0G + A_1G * (t - t_0G + 604800.0 * static_cast<double>(week_delta));

    double t_gps = t - (A_0G + A_1G * (t - t_0G));

    return t_gps;
}
