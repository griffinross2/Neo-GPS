#pragma once

#include <cstddef>

constexpr double WGS84_A = 6378137.0;
constexpr double WGS84_F_INV = 298.257223563;
constexpr double WGS84_B = 6356752.31424518;
constexpr double WGS84_E2 = 0.00669437999014132;

constexpr float GPS_L1CA_FREQ_HZ = 1575.42e6f;
constexpr float GPS_L1CA_CODE_RATE_CPS = 1.023e6f;
constexpr float L1CA_CHIP_SPACING = 0.5f;
constexpr size_t L1CA_CN0_ESTIMATOR_LENGTH = 100;
constexpr float L1CA_PULLIN_TIME_S = 0.5f;
constexpr float L1CA_PULLIN_TO_TRACKING_CN0_THRESHOLD = 35.0f;
constexpr float L1CA_PULLIN_CODE_PLL_BANDWIDTH_HZ = 0.8f;
constexpr float L1CA_PULLIN_LO_FLL_BANDWIDTH_HZ = 20.0f;
constexpr float L1CA_TRACKING_CODE_PLL_BANDWIDTH_HZ = 0.4f;
constexpr float L1CA_TRACKING_LO_PLL_BANDWIDTH_HZ = 25.0f;
constexpr size_t L1CA_BIT_SYNC_TRANSITIONS = 20;

constexpr float GALILEO_E1_FREQ_HZ = 1575.42e6f;
constexpr float GALILEO_E1_CODE_RATE_CPS = 1.023e6f;
constexpr float E1_CHIP_SPACING = 0.2f;
constexpr float E1_ASPECT_BETA = 1.6f;
constexpr size_t E1_CN0_ESTIMATOR_LENGTH = 25;
constexpr float E1_PULLIN_TO_TRACKING_CN0_THRESHOLD = 35.0f;
constexpr float E1_PULLIN_CODE_PLL_BANDWIDTH_HZ = 0.8f;
constexpr float E1_PULLIN_LO_FLL_BANDWIDTH_HZ = 20.0f;
constexpr float E1_PULLIN_LO_PLL_BANDWIDTH_HZ = 25.0f;
constexpr float E1_TRACKING_CODE_PLL_BANDWIDTH_HZ = 0.4f;
constexpr float E1_TRACKING_LO_PLL_BANDWIDTH_HZ = 25.0f;

constexpr float BEIDOU_B1C_FREQ_HZ = 1575.42e6f;
constexpr float BEIDOU_B1C_CODE_RATE_CPS = 1.023e6f;

constexpr float GPS_L1C_FREQ_HZ = 1575.42e6f;
constexpr float GPS_L1C_CODE_RATE_CPS = 1.023e6f;