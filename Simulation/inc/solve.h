#pragma once

#include "constants.h"
#include "l1ca_channel.h"
#include "e1_channel.h"

#include "Eigen/Dense"

#include <algorithm>
#include <numbers>
#include <array>
#include <cmath>

constexpr double SPEED_OF_LIGHT = 299792458.0;       // m/s
constexpr double EARTH_ROTATION_RATE = 7.292115e-5;  // rad/s
constexpr double DELTA_THRESHOLD = 1e-6;             // Convergence threshold for the solver
constexpr size_t MAX_ITERATIONS = 50;                // Maximum number of iterations for the solver

/*

Let f_i(x, y, z, t_rx) = sqrt((x - x_i)^2 + (y - y_i)^2 + (z - z_i)^2) - c*(t_rx - t_tx_i)

where (x_i, y_i, z_i) is the ECEF position of satellite i, t_tx_i is the transmission time of satellite i, and c is the
speed of light.

Using the Gauss-Newton, we can iteratively update the receiver position estimate to minimize the sum of squares of f_i:

x_n+1 = x_n - J+ * F

Where J+ the the Moore-Penrose pseudo-inverse of the Jacobian J formed from the partial derivatives of f_i and evaluated
at x_n, F is the vector of f_i evaluated at x_n, and x_n is the current estimate of (x, y, z, t_rx).

We proceed until the change in every element of x_n is less than DELTA_THRESHOLD or a maximum number of iterations is
reached.

*/

template <size_t MAX_SATS>
class Solver
{
public:
    Solver() = default;

    template <size_t N_L1CA, size_t N_E1>
    bool solve(const std::array<L1CAChannel, N_L1CA>& l1ca_channels, const std::array<E1Channel, N_E1>& e1_channels,
               double& x, double& y, double& z, double& t_rx, size_t& num_sats)
    {
        size_t sv_index = 0;

        for (size_t i = 0; i < N_L1CA; i++)
        {
            if (l1ca_channels[i].can_solve() && sv_index < MAX_SATS)
            {
                double t_tx = l1ca_channels[i].get_corrected_gps_time_of_week();
                l1ca_channels[i].get_satellite_ecef(m_x_SV_0(sv_index, 0), m_x_SV_0(sv_index, 1),
                                                    m_x_SV_0(sv_index, 2));
                m_x_SV_0(sv_index, 3) = t_tx;
                sv_index++;
            }
        }

        for (size_t i = 0; i < N_E1; i++)
        {
            if (e1_channels[i].can_solve() && sv_index < MAX_SATS)
            {
                double t_tx = e1_channels[i].get_corrected_gps_time_of_week();
                e1_channels[i].get_satellite_ecef(m_x_SV_0(sv_index, 0), m_x_SV_0(sv_index, 1), m_x_SV_0(sv_index, 2));
                m_x_SV_0(sv_index, 3) = t_tx;
                sv_index++;
            }
        }

        if (sv_index < 4)
        {
            return false;  // Not enough satellites to solve
        }

        num_sats = sv_index;

        // Initial guess for solution
        double average_t_tx = 0.0;
        for (size_t i = 0; i < sv_index; i++)
        {
            average_t_tx += m_x_SV_0(i, 3);
        }
        average_t_tx /= sv_index;

        m_dist.setZero();
        m_F.setZero();
        m_J.setZero();

        // Perform Gauss-Newton Solving
        double delta = 10000.0;
        size_t iterations = 0;
        while (delta > DELTA_THRESHOLD && iterations < MAX_ITERATIONS)
        {
            // Apply correction for Earth's rotation during signal travel
            for (size_t i = 0; i < sv_index; i++)
            {
                const double theta = EARTH_ROTATION_RATE * (m_x(3) - m_x_SV_0(i, 3));
                const double cos_theta = std::cos(theta);
                const double sin_theta = std::sin(theta);

                m_x_SV(i, 0) = cos_theta * m_x_SV_0(i, 0) + sin_theta * m_x_SV_0(i, 1);
                m_x_SV(i, 1) = -sin_theta * m_x_SV_0(i, 0) + cos_theta * m_x_SV_0(i, 1);
                m_x_SV(i, 2) = m_x_SV_0(i, 2);
                m_x_SV(i, 3) = m_x_SV_0(i, 3);
            }

            // Calculate the distance from the current estimate to each satellite
            for (size_t i = 0; i < sv_index; i++)
            {
                // Get distance to satellite we will use later
                const double dx = m_x_SV(i, 0) - m_x(0);
                const double dy = m_x_SV(i, 1) - m_x(1);
                const double dz = m_x_SV(i, 2) - m_x(2);
                m_dist(i) = std::sqrt(dx * dx + dy * dy + dz * dz);
            }

            // Calculate the Jacobian
            for (size_t i = 0; i < sv_index; i++)
            {
                const double dfdx = 2.0 * (m_x(0) - m_x_SV(i, 0)) / m_dist(i);
                const double dfdy = 2.0 * (m_x(1) - m_x_SV(i, 1)) / m_dist(i);
                const double dfdz = 2.0 * (m_x(2) - m_x_SV(i, 2)) / m_dist(i);
                const double dfdt = -1.0 * SPEED_OF_LIGHT;

                m_J(i, 0) = dfdx;
                m_J(i, 1) = dfdy;
                m_J(i, 2) = dfdz;
                m_J(i, 3) = dfdt;
            }

            // Calculate the pseudo-inverse of the Jacobian
            m_J_inv = m_J.completeOrthogonalDecomposition().pseudoInverse();

            // Calculate the current function values
            for (size_t i = 0; i < sv_index; i++)
            {
                m_F(i) = m_dist(i) - (SPEED_OF_LIGHT * (m_x(3) - m_x_SV(i, 3)));
            }

            // Update using the Gauss-Newton method
            m_x_next = m_x - m_J_inv * m_F;

            // Calculate the delta
            delta = 0.0;
            for (size_t i = 0; i < 4; i++)
            {
                delta = std::max(delta, std::abs(m_x_next(i) - m_x(i)));
            }

            m_x = m_x_next;
            iterations++;
        }

        x = m_x(0);
        y = m_x(1);
        z = m_x(2);
        t_rx = m_x(3);

        double sse = 0.0;
        for (size_t i = 0; i < sv_index; i++)
        {
            sse += std::pow(m_F(i), 2);
        }
        std::println("Solver converged in {} iterations with delta {}, sse {}", iterations, delta, sse);

        return true;
    }

private:
    Eigen::Vector<double, 4> m_x;                 // Estimated solution [x, y, z, t_rx]
    Eigen::Vector<double, MAX_SATS> m_dist;       // norm(x - x_SV) for each satellite
    Eigen::Vector<double, 4> m_x_next;            // Temporary variable for the next estimate
    Eigen::Matrix<double, MAX_SATS, 4> m_x_SV_0;  // [x, y, z, t_tx] for each satellite before iteration
    Eigen::Matrix<double, MAX_SATS, 4> m_x_SV;    // [x, y, z, t_tx] for each satellite during iteration
    Eigen::Matrix<double, MAX_SATS, 1> m_F;       // Output of the navigation equations for each satellite
    Eigen::Matrix<double, MAX_SATS, 4> m_J;       // Jacobian of the navigation equations
    Eigen::Matrix<double, 4, MAX_SATS> m_J_inv;   // Moore-Penrose pseudo-inverse of the Jacobian
};