#pragma once

#include "types.h"

GPS_Status_t l1ca_search_ac_pca(uint8_t *samples, size_t num_samples, GPS_Config_t &gps_conf, int sv, double &code_phase, double &doppler, double &power);