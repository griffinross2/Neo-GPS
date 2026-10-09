#include "e1_ephemeris.h"

#include "tools.h"

#include <print>
#include <numbers>

constexpr double E1_F = -4.442807309e-10;
constexpr double E1_OMEGA_E = 7.2921151467e-5;  // rad/s
constexpr double E1_MU = 3.986004418e14;        // m^3/s^2
constexpr size_t E_K_MAX_ITER = 100;

void E1Ephemeris::update(size_t sv, std::array<uint8_t, 128>& data)
{
    uint8_t word_id;
    bytes_to_number(&word_id, data.data(), sizeof(word_id) * 8, 6, false);

    if (word_id == 1)
    {
        E1Word1_t& word1 = m_next_word1_data[sv - 1];

        bytes_to_number(&word1.iod1, data.data() + 6, sizeof(word1.iod1) * 8, 10, false);
        uint16_t t_0e_raw = 0;
        bytes_to_number(&t_0e_raw, data.data() + 16, sizeof(t_0e_raw) * 8, 14, false);
        word1.t_0e = t_0e_raw * 60.0;
        int32_t M_0_raw = 0;
        bytes_to_number(&M_0_raw, data.data() + 30, sizeof(M_0_raw) * 8, 32, true);
        word1.M_0 = M_0_raw * std::pow(2.0, -31) * std::numbers::pi;
        uint32_t e_raw = 0;
        bytes_to_number(&e_raw, data.data() + 62, sizeof(e_raw) * 8, 32, false);
        word1.e = e_raw * std::pow(2.0, -33);
        uint32_t sqrt_A_raw = 0;
        bytes_to_number(&sqrt_A_raw, data.data() + 94, sizeof(sqrt_A_raw) * 8, 32, false);
        word1.sqrt_A = sqrt_A_raw * std::pow(2.0, -19);

        word1.valid = true;
    }
    else if (word_id == 2)
    {
        E1Word2_t& word2 = m_next_word2_data[sv - 1];

        bytes_to_number(&word2.iod2, data.data() + 6, sizeof(word2.iod2) * 8, 10, false);
        int32_t Omega_0_raw = 0;
        bytes_to_number(&Omega_0_raw, data.data() + 16, sizeof(Omega_0_raw) * 8, 32, true);
        word2.Omega_0 = Omega_0_raw * std::pow(2.0, -31) * std::numbers::pi;
        int32_t i_0_raw = 0;
        bytes_to_number(&i_0_raw, data.data() + 48, sizeof(i_0_raw) * 8, 32, true);
        word2.i_0 = i_0_raw * std::pow(2.0, -31) * std::numbers::pi;
        int32_t omega_raw = 0;
        bytes_to_number(&omega_raw, data.data() + 80, sizeof(omega_raw) * 8, 32, true);
        word2.omega = omega_raw * std::pow(2.0, -31) * std::numbers::pi;
        int16_t i_dot_raw = 0;
        bytes_to_number(&i_dot_raw, data.data() + 112, sizeof(i_dot_raw) * 8, 14, true);
        word2.i_dot = i_dot_raw * std::pow(2.0, -43) * std::numbers::pi;

        word2.valid = true;
    }
    else if (word_id == 3)
    {
        E1Word3_t& word3 = m_next_word3_data[sv - 1];

        bytes_to_number(&word3.iod3, data.data() + 6, sizeof(word3.iod3) * 8, 10, false);
        int32_t Omega_dot_raw = 0;
        bytes_to_number(&Omega_dot_raw, data.data() + 16, sizeof(Omega_dot_raw) * 8, 24, true);
        word3.Omega_dot = Omega_dot_raw * std::pow(2.0, -43) * std::numbers::pi;
        int16_t delta_n_raw = 0;
        bytes_to_number(&delta_n_raw, data.data() + 40, sizeof(delta_n_raw) * 8, 16, true);
        word3.delta_n = delta_n_raw * std::pow(2.0, -43) * std::numbers::pi;
        int16_t C_uc_raw = 0;
        bytes_to_number(&C_uc_raw, data.data() + 56, sizeof(C_uc_raw) * 8, 16, true);
        word3.C_uc = C_uc_raw * std::pow(2.0, -29);
        int16_t C_us_raw = 0;
        bytes_to_number(&C_us_raw, data.data() + 72, sizeof(C_us_raw) * 8, 16, true);
        word3.C_us = C_us_raw * std::pow(2.0, -29);
        int16_t C_rc_raw = 0;
        bytes_to_number(&C_rc_raw, data.data() + 88, sizeof(C_rc_raw) * 8, 16, true);
        word3.C_rc = C_rc_raw * std::pow(2.0, -5);
        int16_t C_rs_raw = 0;
        bytes_to_number(&C_rs_raw, data.data() + 104, sizeof(C_rs_raw) * 8, 16, true);
        word3.C_rs = C_rs_raw * std::pow(2.0, -5);

        word3.valid = true;
    }
    else if (word_id == 4)
    {
        E1Word4_t& word4 = m_next_word4_data[sv - 1];

        bytes_to_number(&word4.iod4, data.data() + 6, sizeof(word4.iod4) * 8, 10, false);
        int16_t C_ic_raw = 0;
        bytes_to_number(&C_ic_raw, data.data() + 22, sizeof(C_ic_raw) * 8, 16, true);
        word4.C_ic = C_ic_raw * std::pow(2.0, -29);
        int16_t C_is_raw = 0;
        bytes_to_number(&C_is_raw, data.data() + 38, sizeof(C_is_raw) * 8, 16, true);
        word4.C_is = C_is_raw * std::pow(2.0, -29);
        uint16_t t_0c_raw = 0;
        bytes_to_number(&t_0c_raw, data.data() + 54, sizeof(t_0c_raw) * 8, 14, false);
        word4.t_0c = t_0c_raw * 60.0;
        int32_t a_f0_raw = 0;
        bytes_to_number(&a_f0_raw, data.data() + 68, sizeof(a_f0_raw) * 8, 31, true);
        word4.a_f0 = a_f0_raw * std::pow(2.0, -34);
        int32_t a_f1_raw = 0;
        bytes_to_number(&a_f1_raw, data.data() + 99, sizeof(a_f1_raw) * 8, 21, true);
        word4.a_f1 = a_f1_raw * std::pow(2.0, -46);
        int8_t a_f2_raw = 0;
        bytes_to_number(&a_f2_raw, data.data() + 120, sizeof(a_f2_raw) * 8, 6, true);
        word4.a_f2 = a_f2_raw * std::pow(2.0, -59);

        word4.valid = true;
    }
    else if (word_id == 5)
    {
        E1Word5_t& word5 = m_current_word5_data[sv - 1];

        int16_t BGD_raw = 0;
        bytes_to_number(&BGD_raw, data.data() + 57, sizeof(BGD_raw) * 8, 10, true);
        word5.BGD = BGD_raw * std::pow(2.0, -32);
        uint8_t signal_health_raw = 0;
        bytes_to_number(&signal_health_raw, data.data() + 69, sizeof(signal_health_raw) * 8, 2, false);
        word5.signal_healthy = (signal_health_raw == 0);
        uint8_t data_valid_raw = 0;
        bytes_to_number(&data_valid_raw, data.data() + 72, sizeof(data_valid_raw) * 8, 1, false);
        word5.data_valid = (data_valid_raw == 0);
        bytes_to_number(&word5.WN, data.data() + 73, sizeof(word5.WN) * 8, 12, false);
        bytes_to_number(&word5.TOW, data.data() + 85, sizeof(word5.TOW) * 8, 20, false);

        word5.valid = true;
    }
    else if (word_id == 10)
    {
        E1Word10_t& word10 = m_current_word10_data[sv - 1];

        int16_t A_0G_raw = 0;
        bytes_to_number(&A_0G_raw, data.data() + 86, sizeof(A_0G_raw) * 8, 16, true);
        word10.A_0G = A_0G_raw * std::pow(2.0, -35);
        int16_t A_1G_raw = 0;
        bytes_to_number(&A_1G_raw, data.data() + 102, sizeof(A_1G_raw) * 8, 12, true);
        word10.A_1G = A_1G_raw * std::pow(2.0, -51);
        bytes_to_number(&word10.t_0G, data.data() + 114, sizeof(word10.t_0G) * 8, 8, false);
        word10.t_0G *= 3600;
        bytes_to_number(&word10.WN_0G, data.data() + 122, sizeof(word10.WN_0G) * 8, 6, false);

        if (A_0G_raw == -1 && A_1G_raw == -1 && word10.t_0G == 0xFF && word10.WN_0G == 0x3F)
        {
            word10.valid = false;
        }
        else
        {
            word10.valid = true;
        }
    }

    // Move next ephemeris data as a group
    if (m_next_word1_data[sv - 1].valid && m_next_word2_data[sv - 1].valid && m_next_word3_data[sv - 1].valid &&
        m_next_word4_data[sv - 1].valid && m_next_word1_data[sv - 1].iod1 == m_next_word2_data[sv - 1].iod2 &&
        m_next_word2_data[sv - 1].iod2 == m_next_word3_data[sv - 1].iod3 &&
        m_next_word3_data[sv - 1].iod3 == m_next_word4_data[sv - 1].iod4)
    {
        m_current_word1_data[sv - 1] = m_next_word1_data[sv - 1];
        m_current_word2_data[sv - 1] = m_next_word2_data[sv - 1];
        m_current_word3_data[sv - 1] = m_next_word3_data[sv - 1];
        m_current_word4_data[sv - 1] = m_next_word4_data[sv - 1];

        m_next_word1_data[sv - 1].valid = false;
        m_next_word2_data[sv - 1].valid = false;
        m_next_word3_data[sv - 1].valid = false;
        m_next_word4_data[sv - 1].valid = false;
    }
}

void E1Ephemeris::get_satellite_ecef(size_t sv, double t, double& x, double& y, double& z) const
{
    const E1Word1_t& word1 = m_current_word1_data[sv - 1];
    const E1Word2_t& word2 = m_current_word2_data[sv - 1];
    const E1Word3_t& word3 = m_current_word3_data[sv - 1];
    const E1Word4_t& word4 = m_current_word4_data[sv - 1];

    double A = word1.sqrt_A * word1.sqrt_A;
    double t_k = time_from_epoch(t, word1.t_0e);
    double E_k = eccentric_anomaly(sv, t_k);
    double v_k = std::atan2(std::sqrt(1 - word1.e * word1.e) * std::sin(E_k), std::cos(E_k) - word1.e);
    double phi_k = v_k + word2.omega;
    double delta_u_k = word3.C_us * std::sin(2 * phi_k) + word3.C_uc * std::cos(2 * phi_k);
    double delta_r_k = word3.C_rs * std::sin(2 * phi_k) + word3.C_rc * std::cos(2 * phi_k);
    double delta_i_k = word4.C_is * std::sin(2 * phi_k) + word4.C_ic * std::cos(2 * phi_k);
    double u_k = phi_k + delta_u_k;
    double r_k = A * (1 - word1.e * std::cos(E_k)) + delta_r_k;
    double i_k = word2.i_0 + delta_i_k + word2.i_dot * t_k;
    double Omega_k = word2.Omega_0 + (word3.Omega_dot - E1_OMEGA_E) * t_k - E1_OMEGA_E * word1.t_0e;
    double x_k_prime = r_k * std::cos(u_k);
    double y_k_prime = r_k * std::sin(u_k);

    x = x_k_prime * std::cos(Omega_k) - y_k_prime * std::cos(i_k) * std::sin(Omega_k);
    y = x_k_prime * std::sin(Omega_k) + y_k_prime * std::cos(i_k) * std::cos(Omega_k);
    z = y_k_prime * std::sin(i_k);
}

double E1Ephemeris::get_clock_correction(size_t sv, double t) const
{
    const E1Word1_t& word1 = m_current_word1_data[sv - 1];
    const E1Word4_t& word4 = m_current_word4_data[sv - 1];
    const E1Word5_t& word5 = m_current_word5_data[sv - 1];

    double t_k = time_from_epoch(t, word1.t_0e);
    double E_k = eccentric_anomaly(sv, t_k);
    double t_r = E1_F * word1.e * word1.sqrt_A * std::sin(E_k);
    t = time_from_epoch(t, word4.t_0c);

    return word4.a_f0 + word4.a_f1 * t + word4.a_f2 * (t * t) + t_r - word5.BGD;
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

double E1Ephemeris::eccentric_anomaly(size_t sv, double t_k) const
{
    const E1Word1_t& word1 = m_current_word1_data[sv - 1];
    const E1Word3_t& word3 = m_current_word3_data[sv - 1];

    double A = word1.sqrt_A * word1.sqrt_A;
    double n_0 = std::sqrt(E1_MU / (A * A * A));
    double n = n_0 + word3.delta_n;
    double M_k = word1.M_0 + n * t_k;
    double E_k = M_k;

    size_t iter = 0;
    while (iter++ < E_K_MAX_ITER)
    {
        double E_k_new = M_k + word1.e * std::sin(E_k);
        if (std::fabs(E_k_new - E_k) < 1e-10)
        {
            E_k = E_k_new;
            break;
        }
        E_k = E_k_new;
    }
    return E_k;
}

double E1Ephemeris::gal_time_to_gps_time(size_t sv, double t) const
{
    const E1Word5_t& word5 = m_current_word5_data[sv - 1];
    const E1Word10_t& word10 = m_current_word10_data[sv - 1];

    int week_delta = (word5.WN % 64) - (word10.WN_0G);
    if (week_delta > 31)
    {
        week_delta -= 64;
    }
    if (week_delta < -31)
    {
        week_delta += 64;
    }
    double dt = word10.A_0G + word10.A_1G * (t - word10.t_0G + 604800.0 * static_cast<double>(week_delta));

    double t_gps = t - (word10.A_0G + word10.A_1G * (t - word10.t_0G));

    return t_gps;
}
