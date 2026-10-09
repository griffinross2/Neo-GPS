#include "l1ca_ephemeris.h"

#include "tools.h"

#include <print>
#include <numbers>

constexpr double L1CA_F = -4.442807633e-10;
constexpr double L1CA_OMEGA_E = 7.2921151467e-5;  // rad/s
constexpr double L1CA_MU = 3.986005e14;           // m^3/s^2
constexpr size_t E_K_MAX_ITER = 100;

void L1CAEphemeris::update(size_t sv, const std::array<uint8_t, 300>& data)
{
    uint8_t subframe_id;
    bytes_to_number(&subframe_id, data.data() + 49, sizeof(subframe_id) * 8, 3, false);

    if (subframe_id == 1)
    {
        L1CASubframe1_t& subframe1 = m_current_subframe1_data[sv - 1];

        bytes_to_number(&subframe1.week_number, data.data() + 60, sizeof(subframe1.week_number) * 8, 10, false);
        bytes_to_number(&subframe1.ura, data.data() + 72, sizeof(subframe1.ura) * 8, 4, false);
        uint8_t sv_health = 0;
        bytes_to_number(&sv_health, data.data() + 76, sizeof(sv_health) * 8, 6, false);
        subframe1.sv_healthy = (sv_health == 0);
        uint16_t iodc_high = 0;
        bytes_to_number(&iodc_high, data.data() + 82, sizeof(iodc_high) * 8, 2, false);
        uint16_t iodc_low = 0;
        bytes_to_number(&iodc_low, data.data() + 210, sizeof(iodc_low) * 8, 8, false);
        subframe1.iodc = (iodc_high << 8) | iodc_low;
        int8_t T_GD_raw = 0;
        bytes_to_number(&T_GD_raw, data.data() + 196, sizeof(T_GD_raw) * 8, 8, true);
        subframe1.T_GD = T_GD_raw * std::pow(2.0, -31);
        uint16_t t_oc_raw = 0;
        bytes_to_number(&t_oc_raw, data.data() + 218, sizeof(t_oc_raw) * 8, 16, false);
        subframe1.t_oc = t_oc_raw * std::pow(2.0, 4);
        int8_t a_f2_raw = 0;
        bytes_to_number(&a_f2_raw, data.data() + 240, sizeof(a_f2_raw) * 8, 8, true);
        subframe1.a_f2 = a_f2_raw * std::pow(2.0, -55);
        int16_t a_f1_raw = 0;
        bytes_to_number(&a_f1_raw, data.data() + 248, sizeof(a_f1_raw) * 8, 16, true);
        subframe1.a_f1 = a_f1_raw * std::pow(2.0, -43);
        int32_t a_f0_raw = 0;
        bytes_to_number(&a_f0_raw, data.data() + 270, sizeof(a_f0_raw) * 8, 22, true);
        subframe1.a_f0 = a_f0_raw * std::pow(2.0, -31);

        subframe1.valid = true;
    }
    else if (subframe_id == 2)
    {
        L1CASubframe2_t& subframe2 = m_next_subframe2_data[sv - 1];

        bytes_to_number(&subframe2.iode_2, data.data() + 60, sizeof(subframe2.iode_2) * 8, 8, false);
        int16_t C_rs_raw = 0;
        bytes_to_number(&C_rs_raw, data.data() + 68, sizeof(C_rs_raw) * 8, 16, true);
        subframe2.C_rs = C_rs_raw * std::pow(2.0, -5);
        int16_t delta_n_raw = 0;
        bytes_to_number(&delta_n_raw, data.data() + 90, sizeof(delta_n_raw) * 8, 16, true);
        subframe2.delta_n = delta_n_raw * std::pow(2.0, -43) * std::numbers::pi;
        int32_t M_0_raw_high = 0;
        bytes_to_number(&M_0_raw_high, data.data() + 106, sizeof(M_0_raw_high) * 8, 8, true);
        int32_t M_0_raw_low = 0;
        bytes_to_number(&M_0_raw_low, data.data() + 120, sizeof(M_0_raw_low) * 8, 24, false);
        subframe2.M_0 = ((M_0_raw_high << 24) | M_0_raw_low) * std::pow(2.0, -31) * std::numbers::pi;
        int16_t C_uc_raw = 0;
        bytes_to_number(&C_uc_raw, data.data() + 150, sizeof(C_uc_raw) * 8, 16, true);
        subframe2.C_uc = C_uc_raw * std::pow(2.0, -29);
        uint32_t e_raw_high = 0;
        bytes_to_number(&e_raw_high, data.data() + 166, sizeof(e_raw_high) * 8, 8, false);
        uint32_t e_raw_low = 0;
        bytes_to_number(&e_raw_low, data.data() + 180, sizeof(e_raw_low) * 8, 24, false);
        subframe2.e = ((e_raw_high << 24) | e_raw_low) * std::pow(2.0, -33);
        int16_t C_us_raw = 0;
        bytes_to_number(&C_us_raw, data.data() + 210, sizeof(C_us_raw) * 8, 16, true);
        subframe2.C_us = C_us_raw * std::pow(2.0, -29);
        uint32_t sqrt_A_raw_high = 0;
        bytes_to_number(&sqrt_A_raw_high, data.data() + 226, sizeof(sqrt_A_raw_high) * 8, 8, false);
        uint32_t sqrt_A_raw_low = 0;
        bytes_to_number(&sqrt_A_raw_low, data.data() + 240, sizeof(sqrt_A_raw_low) * 8, 24, false);
        subframe2.sqrt_A = ((sqrt_A_raw_high << 24) | sqrt_A_raw_low) * std::pow(2.0, -19);
        uint16_t t_oe_raw = 0;
        bytes_to_number(&t_oe_raw, data.data() + 270, sizeof(t_oe_raw) * 8, 16, false);
        subframe2.t_oe = t_oe_raw * std::pow(2.0, 4);

        subframe2.valid = true;
    }
    else if (subframe_id == 3)
    {
        L1CASubframe3_t& subframe3 = m_next_subframe3_data[sv - 1];

        int16_t C_ic_raw = 0;
        bytes_to_number(&C_ic_raw, data.data() + 60, sizeof(C_ic_raw) * 8, 16, true);
        subframe3.C_ic = C_ic_raw * std::pow(2.0, -29);
        int32_t Omega_0_raw_high = 0;
        bytes_to_number(&Omega_0_raw_high, data.data() + 76, sizeof(Omega_0_raw_high) * 8, 8, true);
        int32_t Omega_0_raw_low = 0;
        bytes_to_number(&Omega_0_raw_low, data.data() + 90, sizeof(Omega_0_raw_low) * 8, 24, false);
        subframe3.Omega_0 = ((Omega_0_raw_high << 24) | Omega_0_raw_low) * std::pow(2.0, -31) * std::numbers::pi;
        int16_t C_is_raw = 0;
        bytes_to_number(&C_is_raw, data.data() + 120, sizeof(C_is_raw) * 8, 16, true);
        subframe3.C_is = C_is_raw * std::pow(2.0, -29);
        int32_t i_0_raw_high = 0;
        bytes_to_number(&i_0_raw_high, data.data() + 136, sizeof(i_0_raw_high) * 8, 8, true);
        int32_t i_0_raw_low = 0;
        bytes_to_number(&i_0_raw_low, data.data() + 150, sizeof(i_0_raw_low) * 8, 24, false);
        subframe3.i_0 = ((i_0_raw_high << 24) | i_0_raw_low) * std::pow(2.0, -31) * std::numbers::pi;
        int16_t C_rc_raw = 0;
        bytes_to_number(&C_rc_raw, data.data() + 180, sizeof(C_rc_raw) * 8, 16, true);
        subframe3.C_rc = C_rc_raw * std::pow(2.0, -5);
        int32_t omega_raw_high = 0;
        bytes_to_number(&omega_raw_high, data.data() + 196, sizeof(omega_raw_high) * 8, 8, true);
        int32_t omega_raw_low = 0;
        bytes_to_number(&omega_raw_low, data.data() + 210, sizeof(omega_raw_low) * 8, 24, false);
        subframe3.omega = ((omega_raw_high << 24) | omega_raw_low) * std::pow(2.0, -31) * std::numbers::pi;
        int32_t Omega_dot_raw = 0;
        bytes_to_number(&Omega_dot_raw, data.data() + 240, sizeof(Omega_dot_raw) * 8, 24, true);
        subframe3.Omega_dot = Omega_dot_raw * std::pow(2.0, -43) * std::numbers::pi;
        bytes_to_number(&subframe3.iode_3, data.data() + 270, sizeof(subframe3.iode_3) * 8, 8, false);
        int16_t idot_raw = 0;
        bytes_to_number(&idot_raw, data.data() + 278, sizeof(idot_raw) * 8, 14, true);
        subframe3.idot = idot_raw * std::pow(2.0, -43) * std::numbers::pi;

        subframe3.valid = true;
    }

    // Move subframe 2 and 3 to current as a unit
    if (m_next_subframe2_data[sv - 1].valid && m_next_subframe3_data[sv - 1].valid &&
        m_next_subframe2_data[sv - 1].iode_2 == m_next_subframe3_data[sv - 1].iode_3)
    {
        m_current_subframe2_data[sv - 1] = m_next_subframe2_data[sv - 1];
        m_current_subframe3_data[sv - 1] = m_next_subframe3_data[sv - 1];
        m_next_subframe2_data[sv - 1].valid = false;
        m_next_subframe3_data[sv - 1].valid = false;
    }
}

void L1CAEphemeris::get_satellite_ecef(size_t sv, double t, double& x, double& y, double& z) const
{
    const L1CASubframe2_t& subframe2 = m_current_subframe2_data[sv - 1];
    const L1CASubframe3_t& subframe3 = m_current_subframe3_data[sv - 1];

    double A = subframe2.sqrt_A * subframe2.sqrt_A;
    double t_k = time_from_epoch(t, subframe2.t_oe);
    double E_k = eccentric_anomaly(sv, t_k);
    double v_k = std::atan2(std::sqrt(1 - subframe2.e * subframe2.e) * std::sin(E_k), std::cos(E_k) - subframe2.e);
    double phi_k = v_k + subframe3.omega;
    double delta_u_k = subframe2.C_us * std::sin(2 * phi_k) + subframe2.C_uc * std::cos(2 * phi_k);
    double delta_r_k = subframe2.C_rs * std::sin(2 * phi_k) + subframe3.C_rc * std::cos(2 * phi_k);
    double delta_i_k = subframe3.C_is * std::sin(2 * phi_k) + subframe3.C_ic * std::cos(2 * phi_k);
    double u_k = phi_k + delta_u_k;
    double r_k = A * (1 - subframe2.e * std::cos(E_k)) + delta_r_k;
    double i_k = subframe3.i_0 + delta_i_k + subframe3.idot * t_k;
    double Omega_k = subframe3.Omega_0 + (subframe3.Omega_dot - L1CA_OMEGA_E) * t_k - L1CA_OMEGA_E * subframe2.t_oe;
    double x_k_prime = r_k * std::cos(u_k);
    double y_k_prime = r_k * std::sin(u_k);

    x = x_k_prime * std::cos(Omega_k) - y_k_prime * std::cos(i_k) * std::sin(Omega_k);
    y = x_k_prime * std::sin(Omega_k) + y_k_prime * std::cos(i_k) * std::cos(Omega_k);
    z = y_k_prime * std::sin(i_k);
}

double L1CAEphemeris::get_clock_correction(size_t sv, double t) const
{
    const L1CASubframe1_t& subframe1 = m_current_subframe1_data[sv - 1];
    const L1CASubframe2_t& subframe2 = m_current_subframe2_data[sv - 1];

    double t_k = time_from_epoch(t, subframe2.t_oe);
    double E_k = eccentric_anomaly(sv, t_k);
    double t_r = L1CA_F * subframe2.e * subframe2.sqrt_A * std::sin(E_k);
    t = time_from_epoch(t, subframe1.t_oc);

    return subframe1.a_f0 + subframe1.a_f1 * t + subframe1.a_f2 * (t * t) + t_r - subframe1.T_GD;
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

double L1CAEphemeris::eccentric_anomaly(size_t sv, double t_k) const
{
    const L1CASubframe2_t& subframe2 = m_current_subframe2_data[sv - 1];

    double A = subframe2.sqrt_A * subframe2.sqrt_A;
    double n_0 = std::sqrt(L1CA_MU / (A * A * A));
    double n = n_0 + subframe2.delta_n;
    double M_k = subframe2.M_0 + n * t_k;
    double E_k = M_k;

    size_t iter = 0;
    while (iter++ < E_K_MAX_ITER)
    {
        double E_k_new = M_k + subframe2.e * std::sin(E_k);
        if (std::fabs(E_k_new - E_k) < 1e-10)
        {
            E_k = E_k_new;
            break;
        }
        E_k = E_k_new;
    }
    return E_k;
}