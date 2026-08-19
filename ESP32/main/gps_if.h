#ifndef GPS_IF_H
#define GPS_IF_H

#include <stdint.h>

constexpr uint16_t GPS_SEARCH_GLOBAL_OFFSET = 0x00;

constexpr uint16_t GPS_SEARCH_OFFSET[] = {0x10};
constexpr uint16_t GPS_SEARCH_CONTROL = 0x0;
constexpr uint16_t GPS_SEARCH_ACC_LOW = 0x1;
constexpr uint16_t GPS_SEARCH_ACC_HIGH = 0x2;
constexpr uint16_t GPS_SEARCH_CODE_START = 0x3;
constexpr uint16_t GPS_SEARCH_DOPPLER = 0x4;

constexpr uint16_t GPS_CHANNEL_OFFSET[] = {0x20};
constexpr uint16_t GPS_CHANNEL_CONTROL = 0x0;
constexpr uint16_t GPS_CHANNEL_PAUSE = 0x1;
constexpr uint16_t GPS_CHANNEL_CODE_RATE_LOW = 0x2;
constexpr uint16_t GPS_CHANNEL_CODE_RATE_HIGH = 0x3;
constexpr uint16_t GPS_CHANNEL_LO_RATE_LOW = 0x4;
constexpr uint16_t GPS_CHANNEL_LO_RATE_HIGH = 0x5;
constexpr uint16_t GPS_CHANNEL_CHIP = 0x6;
constexpr uint16_t GPS_CHANNEL_CODE_PHASE_LOW = 0x8;
constexpr uint16_t GPS_CHANNEL_CODE_PHASE_HIGH = 0x9;
constexpr uint16_t GPS_CHANNEL_LO_PHASE_LOW = 0xA;
constexpr uint16_t GPS_CHANNEL_LO_PHASE_HIGH = 0xB;
constexpr uint16_t GPS_CHANNEL_IP_LOW = 0x10;
constexpr uint16_t GPS_CHANNEL_IP_HIGH = 0x11;
constexpr uint16_t GPS_CHANNEL_QP_LOW = 0x12;
constexpr uint16_t GPS_CHANNEL_QP_HIGH = 0x13;
constexpr uint16_t GPS_CHANNEL_IE_LOW = 0x14;
constexpr uint16_t GPS_CHANNEL_IE_HIGH = 0x15;
constexpr uint16_t GPS_CHANNEL_QE_LOW = 0x16;
constexpr uint16_t GPS_CHANNEL_QE_HIGH = 0x17;
constexpr uint16_t GPS_CHANNEL_IL_LOW = 0x18;
constexpr uint16_t GPS_CHANNEL_IL_HIGH = 0x19;
constexpr uint16_t GPS_CHANNEL_QL_LOW = 0x2A;
constexpr uint16_t GPS_CHANNEL_QL_HIGH = 0x2B;

int gps_if_init();
void gps_if_clear_channel(uint8_t channel);
void gps_if_set_channel_sv(uint8_t channel, uint8_t sv);
void gps_if_start_search(uint8_t search, uint8_t channel, uint8_t sv);
bool gps_if_search_done(uint8_t search);
uint32_t gps_if_accumulator(uint8_t search);
double gps_if_code(uint8_t search);
double gps_if_doppler(uint8_t search);

#endif // GPS_IF_H