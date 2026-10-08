#include "l1c_code.h"

#include <string.h>
#include <print>

L1CCode::L1CCode(int sv, int chip_start)
{
    this->chip = 0;
    m_sv = sv;
    m_current_byte_data = L1C_PRIMARY_CODE_DATA[sv - 1][0];
    m_current_byte_pilot = L1C_PRIMARY_CODE_PILOT[sv - 1][0];

    // Wind to the initial chip
    set_chip(chip_start);
}

void L1CCode::clock_chip()
{
    // Shift the current byte to the left by 1
    m_current_byte_data <<= 1;
    m_current_byte_pilot <<= 1;

    // Update chip
    chip = (chip + 1) % 10230;

    if ((chip % 8) == 0)
    {
        m_current_byte_data = L1C_PRIMARY_CODE_DATA[m_sv - 1][chip / 8];
        m_current_byte_pilot = L1C_PRIMARY_CODE_PILOT[m_sv - 1][chip / 8];
    }
}

int L1CCode::get_chip_data()
{
    return (m_current_byte_data & 0x80) ? 1 : -1;
}

int L1CCode::get_chip_pilot()
{
    return (m_current_byte_pilot & 0x80) ? 1 : -1;
}

void L1CCode::set_chip(int chip)
{
    while (this->chip != chip)
    {
        clock_chip();
    }
}