#pragma once

constexpr double WGS84_A = 6378137.0;
constexpr double WGS84_F_INV = 298.257223563;
constexpr double WGS84_B = 6356752.31424518;
constexpr double WGS84_E2 = 0.00669437999014132;

constexpr double GPS_L1CA_FREQ_HZ = 1575.42e6;
constexpr double GPS_L1CA_CODE_RATE_CPS = 1.023e6;
constexpr double L1CA_CHIP_SPACING = 0.5;
constexpr size_t L1CA_CN0_ESTIMATOR_LENGTH = 100;
constexpr size_t L1CA_PULLIN_TIME_MS = 500;
constexpr double L1CA_PULLIN_TO_TRACKING_CN0_THRESHOLD = 35.0;
constexpr double L1CA_PULLIN_CODE_PLL_BANDWIDTH_HZ = 0.8;
constexpr double L1CA_PULLIN_LO_FLL_BANDWIDTH_HZ = 20.0;
constexpr double L1CA_TRACKING_CODE_PLL_BANDWIDTH_HZ = 0.4;
constexpr double L1CA_TRACKING_LO_PLL_BANDWIDTH_HZ = 25.0;
constexpr size_t L1CA_BIT_SYNC_TRANSITIONS = 20;

constexpr double GALILEO_E1_FREQ_HZ = 1575.42e6;
constexpr double GALILEO_E1_CODE_RATE_CPS = 1.023e6;
constexpr double E1_CHIP_SPACING = 0.2;
constexpr double E1_ASPECT_BETA = 1.6;
constexpr size_t E1_CN0_ESTIMATOR_LENGTH = 25;
constexpr double E1_PULLIN_TO_TRACKING_CN0_THRESHOLD = 35.0;
constexpr double E1_PULLIN_CODE_PLL_BANDWIDTH_HZ = 0.8;
constexpr double E1_PULLIN_LO_FLL_BANDWIDTH_HZ = 20.0;
constexpr double E1_PULLIN_LO_PLL_BANDWIDTH_HZ = 25.0;
constexpr double E1_TRACKING_CODE_PLL_BANDWIDTH_HZ = 0.4;
constexpr double E1_TRACKING_LO_PLL_BANDWIDTH_HZ = 25.0;

constexpr double BEIDOU_B1C_FREQ_HZ = 1575.42e6;
constexpr double BEIDOU_B1C_CODE_RATE_CPS = 1.023e6;

constexpr double GPS_L1C_FREQ_HZ = 1575.42e6;
constexpr double GPS_L1C_CODE_RATE_CPS = 1.023e6;