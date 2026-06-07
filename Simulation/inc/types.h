#pragma once

#include <stdint.h>

typedef enum
{
    OK = 0,
    ERR,
} GPS_Status_t;

typedef struct
{
    double sample_rate_sps;
    double if_freq_hz;
} GPS_Config_t;