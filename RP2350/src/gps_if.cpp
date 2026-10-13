#include "gps_if.h"

#include "parallel_interface.h"
#include "pico/stdlib.h"

#include <stdint.h>
#include <memory.h>
#include <bit>

int gps_if_init()
{
    parallel_interface_init();
    return 0;
}

void gps_if_clear_channel(uint8_t channel)
{
    parallel_interface_write(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_CONTROL, 0x40000000);  // set clear
    sleep_us(100);
    parallel_interface_write(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_CONTROL, 0x40000000);  // clear clear
}

void gps_if_set_channel_sv(uint8_t channel, uint8_t sv)
{
    parallel_interface_write(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_CONTROL, sv & 0x3F);  // set clear
}

void gps_if_start_search(uint8_t search, uint8_t channel, uint8_t sv)
{
    parallel_interface_write(GPS_SEARCH_OFFSET[search] + GPS_SEARCH_CONTROL,
                             0x40000000 | (((uint32_t)channel & 0x3F) << 6) | (sv & 0x3F));
}

uint32_t gps_if_search_accumulator(uint8_t search)
{
    uint32_t acc = parallel_interface_read(GPS_SEARCH_OFFSET[search] + GPS_SEARCH_ACC);

    return acc;
}

void gps_if_search_code_doppler(uint8_t search, double& code, double& doppler)
{
    uint32_t code_doppler_data = parallel_interface_read(GPS_SEARCH_OFFSET[search] + GPS_SEARCH_CODE_START_DOPPLER);
    code = (double)(code_doppler_data & 0x3FF);
    code -= (double)((code_doppler_data >> 10) & 0x1F) * (1.023e6 / 19.2e6);

    uint32_t dop_data = (code_doppler_data >> 15) & 0x3F;
    uint32_t doppler_sign = ((dop_data >> 5) & 0x1);
    int8_t doppler_int =
        (int8_t)(dop_data + (doppler_sign << 6) + (doppler_sign << 7));  // Sign extend to 8 bits then convert to int
    doppler = (double)doppler_int * 250.0;
}

bool gps_if_search_done(uint8_t search)
{
    uint32_t status = parallel_interface_read(GPS_SEARCH_OFFSET[search] + GPS_SEARCH_CONTROL);
    if (status & 0x80000000)
    {
        return false;
    }
    return true;
}

int32_t gps_if_channel_ip(uint8_t channel)
{
    uint32_t ip_raw = parallel_interface_read(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_IP);
    int32_t ip = std::bit_cast<int32_t>(ip_raw);
    return ip;
}

int32_t gps_if_channel_qp(uint8_t channel)
{
    uint32_t qp_raw = parallel_interface_read(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_QP);
    int32_t qp = std::bit_cast<int32_t>(qp_raw);
    return qp;
}

int32_t gps_if_channel_ie(uint8_t channel)
{
    uint32_t ie_raw = parallel_interface_read(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_IE);
    int32_t ie = std::bit_cast<int32_t>(ie_raw);
    return ie;
}

int32_t gps_if_channel_qe(uint8_t channel)
{
    uint32_t qe_raw = parallel_interface_read(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_QE);
    int32_t qe = std::bit_cast<int32_t>(qe_raw);
    return qe;
}

int32_t gps_if_channel_il(uint8_t channel)
{
    uint32_t il_raw = parallel_interface_read(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_IL);
    int32_t il = std::bit_cast<int32_t>(il_raw);
    return il;
}

int32_t gps_if_channel_ql(uint8_t channel)
{
    uint32_t ql_raw = parallel_interface_read(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_QL);
    int32_t ql = std::bit_cast<int32_t>(ql_raw);
    return ql;
}

void gps_if_set_channel_code_rate(uint8_t channel, uint32_t code_rate)
{
    parallel_interface_write(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_CODE_RATE, code_rate);
}

void gps_if_set_channel_lo_rate(uint8_t channel, uint32_t lo_rate)
{
    parallel_interface_write(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_LO_RATE, lo_rate);
}

bool gps_if_channel_is_epoch(uint8_t channel)
{
    uint32_t status = parallel_interface_read(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_CONTROL);
    if (status & 0x80000000)
    {
        return true;
    }
    return false;
}

void gps_if_channel_pause(uint8_t channel, uint16_t pause)
{
    parallel_interface_write(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_PAUSE, (uint32_t)pause);
}