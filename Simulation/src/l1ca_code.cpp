#include "l1ca_code.h"

#include <string.h>
#include <print>

L1CACode::L1CACode(int sv, int chip_start)
{
    this->chip = 0;
    m_sv = sv;

    // Initial conditionssa
    m_g1.fill(1);

    if (sv < 38)
    {
        m_g2.fill(1);
    }
    else if (sv < 64)
    {
        uint16_t g2_init = L1CA_G2_INIT_38_63[sv - 38];
        for (int i = 1; i < 11; i++)
        {
            m_g2[i] = (g2_init >> (i - 1)) & 0x1;
        }
    }
    else if (sv >= 120 && sv <= 210)
    {
        uint16_t g2_init = L1CA_G2_INIT_120_210[sv - 120];
        for (int i = 1; i < 11; i++)
        {
            m_g2[i] = (g2_init >> (i - 1)) & 0x1;
        }
    }
    else
    {
        std::println("Invalid SV number: {}", sv);
    }

    // Wind to the initial chip
    set_chip(chip_start);
}

void L1CACode::clock_chip()
{
    // Update G1
    m_g1[0] = m_g1[3] ^ m_g1[10];

    // Update G2
    m_g2[0] = m_g2[2] ^ m_g2[3] ^ m_g2[6] ^ m_g2[8] ^ m_g2[9] ^ m_g2[10];

    // Shift registers
    std::copy(m_g1.begin(), m_g1.end() - 1, m_g1.begin() + 1);
    std::copy(m_g2.begin(), m_g2.end() - 1, m_g2.begin() + 1);

    // Update chip
    chip = (chip + 1) % 1023;
}

int L1CACode::get_chip()
{
    if (m_sv > 0 && m_sv < 38)
    {
        uint8_t tap1 = L1CA_TAPS[m_sv - 1][0];
        uint8_t tap2 = L1CA_TAPS[m_sv - 1][1];
        return (m_g1[10] ^ m_g2[tap1] ^ m_g2[tap2]) ? 1 : -1;
    }
    else
    {
        return (m_g1[10] ^ m_g2[10]) ? 1 : -1;
    }
}

void L1CACode::set_chip(int chip)
{
    while (this->chip != chip)
    {
        clock_chip();
    }
}