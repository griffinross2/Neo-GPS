#pragma once

#include <stdint.h>

int b1c_search_ac_pca(int8_t* samples, double freq_if_hz, double freq_sample_hz, int sv, double& code_phase,
                      double& doppler, double& power);