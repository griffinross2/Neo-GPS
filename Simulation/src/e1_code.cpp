#include "e1_code.h"

#include <string.h>
#include <print>

E1Code::E1Code(int sv, int chip_start)
{
    this->chip = 0;
    m_sv = sv;
    m_current_byte_b = E1B_PRIMARY_CODE[sv - 1][0];
    m_current_byte_c = E1C_PRIMARY_CODE[sv - 1][0];

    // Wind to the initial chip
    set_chip(chip_start);
}

void E1Code::clock_chip()
{
    // Shift the current byte to the left by 1
    m_current_byte_b <<= 1;
    m_current_byte_c <<= 1;

    // Update chip
    chip = (chip + 1) % 4092;

    if ((chip % 8) == 0)
    {
        m_current_byte_b = E1B_PRIMARY_CODE[m_sv - 1][chip / 8];
        m_current_byte_c = E1C_PRIMARY_CODE[m_sv - 1][chip / 8];
    }
}

int E1Code::get_chip_b()
{
    return (m_current_byte_b & 0x80) ? 1 : -1;
}

int E1Code::get_chip_c()
{
    return (m_current_byte_c & 0x80) ? 1 : -1;
}

void E1Code::set_chip(int chip)
{
    while (this->chip != chip)
    {
        clock_chip();
    }
}