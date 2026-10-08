#include "l1ca_ephemeris.h"

#include "tools.h"

#include <print>
#include <numbers>

constexpr double L1CA_F = -4.442807633e-10;
constexpr double L1CA_OMEGA_E = 7.2921151467e-5;  // rad/s
constexpr double L1CA_MU = 3.986005e14;           // m^3/s^2
constexpr size_t E_K_MAX_ITER = 100;

void L1CAEphemeris::update(std::array<uint8_t, 300>& data)
{
    uint8_t subframe_id;
    bytes_to_number(&subframe_id, data.data() + 49, sizeof(subframe_id) * 8, 3, false);

    if (subframe_id == 1)
    {
        bytes_to_number(&week_number, data.data() + 60, sizeof(week_number) * 8, 10, false);
        bytes_to_number(&ura, data.data() + 72, sizeof(ura) * 8, 4, false);
        uint8_t sv_health = 0;
        bytes_to_number(&sv_health, data.data() + 76, sizeof(sv_health) * 8, 6, false);
        sv_healthy = (sv_health == 0);
        uint16_t iodc_high = 0;
        bytes_to_number(&iodc_high, data.data() + 82, sizeof(iodc_high) * 8, 2, false);
        uint16_t iodc_low = 0;
        bytes_to_number(&iodc_low, data.data() + 210, sizeof(iodc_low) * 8, 8, false);
        iodc = (iodc_high << 8) | iodc_low;
        int8_t T_GD_raw = 0;
        bytes_to_number(&T_GD_raw, data.data() + 196, sizeof(T_GD_raw) * 8, 8, true);
        T_GD = T_GD_raw * std::pow(2.0, -31);
        uint16_t t_oc_raw = 0;
        bytes_to_number(&t_oc_raw, data.data() + 218, sizeof(t_oc_raw) * 8, 16, false);
        t_oc = t_oc_raw * std::pow(2.0, 4);
        int8_t a_f2_raw = 0;
        bytes_to_number(&a_f2_raw, data.data() + 240, sizeof(a_f2_raw) * 8, 8, true);
        a_f2 = a_f2_raw * std::pow(2.0, -55);
        int16_t a_f1_raw = 0;
        bytes_to_number(&a_f1_raw, data.data() + 248, sizeof(a_f1_raw) * 8, 16, true);
        a_f1 = a_f1_raw * std::pow(2.0, -43);
        int32_t a_f0_raw = 0;
        bytes_to_number(&a_f0_raw, data.data() + 270, sizeof(a_f0_raw) * 8, 22, true);
        a_f0 = a_f0_raw * std::pow(2.0, -31);

        subframe1_valid = true;
    }
    else if (subframe_id == 2)
    {
        bytes_to_number(&iode_2, data.data() + 60, sizeof(iode_2) * 8, 8, false);
        int16_t C_rs_raw = 0;
        bytes_to_number(&C_rs_raw, data.data() + 68, sizeof(C_rs_raw) * 8, 16, true);
        C_rs = C_rs_raw * std::pow(2.0, -5);
        int16_t delta_n_raw = 0;
        bytes_to_number(&delta_n_raw, data.data() + 90, sizeof(delta_n_raw) * 8, 16, true);
        delta_n = delta_n_raw * std::pow(2.0, -43) * std::numbers::pi;
        int32_t M_0_raw_high = 0;
        bytes_to_number(&M_0_raw_high, data.data() + 106, sizeof(M_0_raw_high) * 8, 8, true);
        int32_t M_0_raw_low = 0;
        bytes_to_number(&M_0_raw_low, data.data() + 120, sizeof(M_0_raw_low) * 8, 24, false);
        M_0 = ((M_0_raw_high << 24) | M_0_raw_low) * std::pow(2.0, -31) * std::numbers::pi;
        int16_t C_uc_raw = 0;
        bytes_to_number(&C_uc_raw, data.data() + 150, sizeof(C_uc_raw) * 8, 16, true);
        C_uc = C_uc_raw * std::pow(2.0, -29);
        uint32_t e_raw_high = 0;
        bytes_to_number(&e_raw_high, data.data() + 166, sizeof(e_raw_high) * 8, 8, false);
        uint32_t e_raw_low = 0;
        bytes_to_number(&e_raw_low, data.data() + 180, sizeof(e_raw_low) * 8, 24, false);
        e = ((e_raw_high << 24) | e_raw_low) * std::pow(2.0, -33);
        int16_t C_us_raw = 0;
        bytes_to_number(&C_us_raw, data.data() + 210, sizeof(C_us_raw) * 8, 16, true);
        C_us = C_us_raw * std::pow(2.0, -29);
        uint32_t sqrt_A_raw_high = 0;
        bytes_to_number(&sqrt_A_raw_high, data.data() + 226, sizeof(sqrt_A_raw_high) * 8, 8, false);
        uint32_t sqrt_A_raw_low = 0;
        bytes_to_number(&sqrt_A_raw_low, data.data() + 240, sizeof(sqrt_A_raw_low) * 8, 24, false);
        sqrt_A = ((sqrt_A_raw_high << 24) | sqrt_A_raw_low) * std::pow(2.0, -19);
        uint16_t t_oe_raw = 0;
        bytes_to_number(&t_oe_raw, data.data() + 270, sizeof(t_oe_raw) * 8, 16, false);
        t_oe = t_oe_raw * std::pow(2.0, 4);

        subframe2_valid = true;
    }
    else if (subframe_id == 3)
    {
        int16_t C_ic_raw = 0;
        bytes_to_number(&C_ic_raw, data.data() + 60, sizeof(C_ic_raw) * 8, 16, true);
        C_ic = C_ic_raw * std::pow(2.0, -29);
        int32_t Omega_0_raw_high = 0;
        bytes_to_number(&Omega_0_raw_high, data.data() + 76, sizeof(Omega_0_raw_high) * 8, 8, true);
        int32_t Omega_0_raw_low = 0;
        bytes_to_number(&Omega_0_raw_low, data.data() + 90, sizeof(Omega_0_raw_low) * 8, 24, false);
        Omega_0 = ((Omega_0_raw_high << 24) | Omega_0_raw_low) * std::pow(2.0, -31) * std::numbers::pi;
        int16_t C_is_raw = 0;
        bytes_to_number(&C_is_raw, data.data() + 120, sizeof(C_is_raw) * 8, 16, true);
        C_is = C_is_raw * std::pow(2.0, -29);
        int32_t i_0_raw_high = 0;
        bytes_to_number(&i_0_raw_high, data.data() + 136, sizeof(i_0_raw_high) * 8, 8, true);
        int32_t i_0_raw_low = 0;
        bytes_to_number(&i_0_raw_low, data.data() + 150, sizeof(i_0_raw_low) * 8, 24, false);
        i_0 = ((i_0_raw_high << 24) | i_0_raw_low) * std::pow(2.0, -31) * std::numbers::pi;
        int16_t C_rc_raw = 0;
        bytes_to_number(&C_rc_raw, data.data() + 180, sizeof(C_rc_raw) * 8, 16, true);
        C_rc = C_rc_raw * std::pow(2.0, -5);
        int32_t omega_raw_high = 0;
        bytes_to_number(&omega_raw_high, data.data() + 196, sizeof(omega_raw_high) * 8, 8, true);
        int32_t omega_raw_low = 0;
        bytes_to_number(&omega_raw_low, data.data() + 210, sizeof(omega_raw_low) * 8, 24, false);
        omega = ((omega_raw_high << 24) | omega_raw_low) * std::pow(2.0, -31) * std::numbers::pi;
        int32_t Omega_dot_raw = 0;
        bytes_to_number(&Omega_dot_raw, data.data() + 240, sizeof(Omega_dot_raw) * 8, 24, true);
        Omega_dot = Omega_dot_raw * std::pow(2.0, -43) * std::numbers::pi;
        bytes_to_number(&iode_3, data.data() + 270, sizeof(iode_3) * 8, 8, false);
        int16_t idot_raw = 0;
        bytes_to_number(&idot_raw, data.data() + 278, sizeof(idot_raw) * 8, 14, true);
        idot = idot_raw * std::pow(2.0, -43) * std::numbers::pi;

        subframe3_valid = true;
    }
}

void L1CAEphemeris::get_satellite_ecef(double t, double& x, double& y, double& z) const
{
    double A = sqrt_A * sqrt_A;
    double t_k = time_from_epoch(t, t_oe);
    double E_k = eccentric_anomaly(t_k);
    double v_k = std::atan2(std::sqrt(1 - e * e) * std::sin(E_k), std::cos(E_k) - e);
    double phi_k = v_k + omega;
    double delta_u_k = C_us * std::sin(2 * phi_k) + C_uc * std::cos(2 * phi_k);
    double delta_r_k = C_rs * std::sin(2 * phi_k) + C_rc * std::cos(2 * phi_k);
    double delta_i_k = C_is * std::sin(2 * phi_k) + C_ic * std::cos(2 * phi_k);
    double u_k = phi_k + delta_u_k;
    double r_k = A * (1 - e * std::cos(E_k)) + delta_r_k;
    double i_k = i_0 + delta_i_k + idot * t_k;
    double Omega_k = Omega_0 + (Omega_dot - L1CA_OMEGA_E) * t_k - L1CA_OMEGA_E * t_oe;
    double x_k_prime = r_k * std::cos(u_k);
    double y_k_prime = r_k * std::sin(u_k);

    x = x_k_prime * std::cos(Omega_k) - y_k_prime * std::cos(i_k) * std::sin(Omega_k);
    y = x_k_prime * std::sin(Omega_k) + y_k_prime * std::cos(i_k) * std::cos(Omega_k);
    z = y_k_prime * std::sin(i_k);
}

double L1CAEphemeris::get_clock_correction(double t) const
{
    double t_k = time_from_epoch(t, t_oe);
    double E_k = eccentric_anomaly(t_k);
    double t_r = L1CA_F * e * sqrt_A * std::sin(E_k);
    t = time_from_epoch(t, t_oc);

    return a_f0 + a_f1 * t + a_f2 * (t * t) + t_r - T_GD;
}

double L1CAEphemeris::time_from_epoch(double t, double t_epoch) const
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

double L1CAEphemeris::eccentric_anomaly(double t_k) const
{
    double A = sqrt_A * sqrt_A;
    double n_0 = std::sqrt(L1CA_MU / (A * A * A));
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