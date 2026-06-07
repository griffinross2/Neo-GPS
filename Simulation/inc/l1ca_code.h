#pragma once

#include <stdint.h>

class L1CACode
{
public:
    L1CACode(uint8_t tap1, uint8_t tap2, int chip_start = 0);

    void clock_chip();
    uint8_t get_chip();

    int chip;

private:
    uint8_t tap1;
    uint8_t tap2;
    uint8_t g1[11]; // First bit is space for new bit
    uint8_t g2[11]; // First bit is space for new bit
};