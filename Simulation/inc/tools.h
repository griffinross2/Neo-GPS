#pragma once

#include <array>
#include <cmath>
#include <span>

template <size_t N>
double cn0_svn_estimator(std::array<int, N>& ip_buffer, std::array<int, N>& qp_buffer, size_t length, double int_time)
{
    // Modified from GNSS-SDR

    float SNR = 0.0;
    float SNR_dB_Hz = 0.0;
    float Psig = 0.0;
    float Ptot = 0.0;
    if (length == 0 || int_time == 0.0)
    {
        return -100.0;
    }
    for (int i = 0; i < length; i++)
    {
        Psig += std::abs(ip_buffer.at(i));
        Ptot += qp_buffer.at(i) * qp_buffer.at(i) + ip_buffer.at(i) * ip_buffer.at(i);
    }
    Psig /= static_cast<float>(length);
    Psig = Psig * Psig;
    Ptot /= static_cast<float>(length);
    float aux = Ptot - Psig;
    if (aux == 0.0)
    {
        return -100.0;
    }
    SNR = Psig / aux;
    SNR_dB_Hz = 10.0 * std::log10(SNR) - 10.0 * std::log10(int_time);

    return SNR_dB_Hz;
}

template <size_t N>
double cn0_m2m4_estimator(std::array<int, N>& ip_buffer, std::array<int, N>& qp_buffer, size_t length, double int_time)
{
    // Modified from GNSS-SDR

    double SNR_aux = 0.0;
    double SNR_dB_Hz = 0.0;
    double Psig = 0.0;
    double m_2 = 0.0;
    double m_4 = 0.0;
    double aux;
    const auto n = static_cast<double>(length);
    if (length == 0 || int_time == 0.0)
    {
        return -100.0;
    }
    for (int i = 0; i < length; i++)
    {
        Psig += std::abs(ip_buffer.at(i));
        aux = qp_buffer.at(i) * qp_buffer.at(i) + ip_buffer.at(i) * ip_buffer.at(i);
        m_2 += aux;
        m_4 += (aux * aux);
    }
    Psig /= n;
    Psig = Psig * Psig;
    m_2 /= n;
    m_4 /= n;
    aux = std::sqrt(2.0 * m_2 * m_2 - m_4);
    double denominator;
    if (std::isnan(aux))
    {
        denominator = m_2 - Psig;
        if (denominator == 0)
        {
            return -100.0;
        }
        SNR_aux = Psig / denominator;
    }
    else
    {
        denominator = m_2 - aux;
        if (denominator == 0)
        {
            return -100.0;
        }
        SNR_aux = aux / denominator;
    }

    if (SNR_aux == 0)
    {
        return -100.0;
    }
    SNR_dB_Hz = 10.0 * std::log10(SNR_aux) - 10.0 * std::log10(int_time);

    return SNR_dB_Hz;
}

template <size_t N>
class CN0Estimator
{
public:
    CN0Estimator() = default;

    CN0Estimator(double int_time) : m_int_time(int_time)
    {
        m_cn0 = -100.0;
        m_length = 0;
        m_index = 0;
    }

    void update(int ip, int qp)
    {
        // Push new values into the buffers
        m_ip_buffer[m_index] = static_cast<int>(ip);
        m_qp_buffer[m_index] = static_cast<int>(qp);
        if (m_length < N)
        {
            m_length++;
        }
        m_index = (m_index + 1) % N;

        m_cn0 = cn0_svn_estimator<N>(m_ip_buffer, m_qp_buffer, m_length, m_int_time);
    }

    double get_cn0() { return m_cn0; }
    bool is_full() { return m_length >= N; }

private:
    double m_int_time = 0.0;
    std::array<int, N> m_ip_buffer;
    std::array<int, N> m_qp_buffer;
    double m_cn0 = -100.0;
    size_t m_length = 0;
    size_t m_index = 0;
};

static uint32_t hamming_dist2(uint32_t val1, uint32_t val2)
{
    uint32_t dist = 0;
    for (int i = 0; i < 2; i++)
    {
        dist += (val1 & 0x1) ^ (val2 & 0x1);
        val1 >>= 1;
        val2 >>= 1;
    }
    return dist;
}

static uint8_t parity(uint8_t val)
{
    uint8_t parity = 0;
    for (int i = 0; i < 8; i++)
    {
        parity ^= (val >> i) & 0x1;
    }
    return parity;
}

static uint8_t get_conv_out(uint8_t state, uint8_t input)
{
    static uint8_t G1 = 0b1111001;
    static uint8_t G2 = 0b1011011;

    uint8_t conv_in = state | (input << 6);

    // [7:2] res, [1:0] output bits
    uint8_t result = 0;
    result |= (parity(conv_in & G1) << 1);
    result |= (parity(conv_in & G2) == 0);

    return result;
}

template <size_t N>
uint32_t viterbi_decode(std::array<uint8_t, N>& input, std::array<uint8_t, N / 2>& result)
{
    constexpr uint8_t NSTATES = 64;
    constexpr uint8_t RATE = 2;

    // Path metric array
    std::array<std::array<uint32_t, N / RATE + 1>, NSTATES> path_metric;

    // Setup initial state
    for (int i = 0; i < NSTATES; i++)
    {
        path_metric[i].fill(0xFFFFFFFF);
    }
    path_metric[0][0] = 0;

    // Create trellis
    for (int j = 0; j < N / RATE; j++)
    {
        for (uint8_t i = 0; i < NSTATES; i++)
        {
            // skip unaccessible states
            if (path_metric[i][j] == 0xFFFFFFFF)
                continue;

            // input bit 0
            uint8_t next_state = (i >> 1);
            uint32_t conv_out = get_conv_out(i, 0);
            uint8_t rcvd = ((input[j * RATE]) << 1) | (input[j * RATE + 1]);
            uint32_t dist = hamming_dist2(rcvd, conv_out);
            if (path_metric[next_state][j + 1] > path_metric[i][j] + dist)
            {
                path_metric[next_state][j + 1] = path_metric[i][j] + dist;
            }
            // input bit 1
            next_state |= 0b100000;
            conv_out = get_conv_out(i, 1);
            dist = hamming_dist2(rcvd, conv_out);
            if (path_metric[next_state][j + 1] > path_metric[i][j] + dist)
            {
                path_metric[next_state][j + 1] = path_metric[i][j] + dist;
            }
        }
    }

    // Find best ending
    uint32_t best_ending = 0xFFFFFFFF;
    uint8_t best_state = 0;
    uint32_t best_metric = 0xFFFFFFFF;
    for (int i = 0; i < NSTATES; i++)
    {
        if (path_metric[i][N / RATE] < best_metric)
        {
            best_metric = path_metric[i][N / RATE];
            best_state = i;
        }
    }

    // Save best ending
    best_ending = best_state;

    // Traceback through best path
    for (int i = N / RATE - 1; i >= 0; i--)
    {
        result[i] = (best_state >> 5) & 1;

        // Zero path
        uint8_t previous_state = (best_state << 1) & 0b111111;
        best_metric = path_metric[previous_state][i];
        best_state = previous_state;

        // One path
        previous_state = (best_state | 1);
        if (path_metric[previous_state][i] < best_metric)
        {
            best_metric = path_metric[previous_state][i];
            best_state = previous_state;
        }
    }

    return best_ending;
}

template <size_t R, size_t C>
void deinterleave_and_flip(uint8_t flip, std::span<uint8_t, R * C>& input, std::array<uint8_t, R * C>& output)
{
    // Deinterleave
    for (int i = 0; i < R; i++)
    {
        for (int j = 0; j < C; j++)
        {
            output[i + j * R] = input[i * C + j] ^ flip;
        }
    }
}

void bytes_to_number(void* dest, const uint8_t* buf, size_t dest_size, size_t src_size, bool is_signed);

void ecef_to_coords(double x, double y, double z, double& lat, double& lon, double& alt);