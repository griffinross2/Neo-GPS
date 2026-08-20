#include "gps_if.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

#include <stdint.h>
#include <memory.h>
#include <bit>

constexpr gpio_num_t GPS_CS_PIN = GPIO_NUM_10;
constexpr gpio_num_t GPS_CLK_PIN = GPIO_NUM_12;
constexpr gpio_num_t GPS_MOSI_PIN = GPIO_NUM_13;
constexpr gpio_num_t GPS_MISO_PIN = GPIO_NUM_11;

static spi_device_handle_t s_gps_if_spi_handle;

static void gps_if_spi_write(uint16_t reg, uint16_t data)
{
    uint16_t rx_dummy;
    uint8_t tx_data[] = {(uint8_t)(data >> 8), (uint8_t)data};
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.addr = reg & 0x7FFF;
    t.length = 16;
    t.rxlength = 0;
    t.tx_buffer = &tx_data;
    t.rx_buffer = &rx_dummy;

    spi_device_transmit(s_gps_if_spi_handle, &t);
}

uint16_t gps_if_spi_read(uint16_t reg)
{
    uint16_t tx_dummy = 0;
    uint8_t rx_data[2];
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.addr = reg | 0x8000;
    t.length = 16;
    t.rxlength = 0;
    t.tx_buffer = &tx_dummy;
    t.rx_buffer = &rx_data;

    spi_device_transmit(s_gps_if_spi_handle, &t);

    return ((uint16_t)rx_data[0] << 8) | rx_data[1];
}

int gps_if_init()
{

    spi_bus_config_t buscfg;
    memset(&buscfg, 0, sizeof(buscfg));
    buscfg.sclk_io_num = GPIO_NUM_12;
    buscfg.mosi_io_num = GPIO_NUM_13;
    buscfg.miso_io_num = GPIO_NUM_11;
    buscfg.data2_io_num = -1;
    buscfg.data3_io_num = -1;
    buscfg.data4_io_num = -1;
    buscfg.data5_io_num = -1;
    buscfg.data6_io_num = -1;
    buscfg.data7_io_num = -1;
    buscfg.data_io_default_level = 0;
    buscfg.max_transfer_sz = 0;
    buscfg.flags = 0;
    buscfg.isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO;
    buscfg.intr_flags = 0;

    spi_device_interface_config_t devcfg;
    memset(&devcfg, 0, sizeof(devcfg));
    devcfg.address_bits = 16;
    devcfg.mode = 0;
    devcfg.clock_speed_hz = 5000000; // 5 MHz
    devcfg.spics_io_num = GPS_CS_PIN;
    devcfg.queue_size = 1;

    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_DISABLED);
    spi_bus_add_device(SPI2_HOST, &devcfg, &s_gps_if_spi_handle);

    return 0;
}

void gps_if_clear_channel(uint8_t channel)
{
    gps_if_spi_write(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_CONTROL, 0x4000); // set clear
    vTaskDelay(1);
    gps_if_spi_write(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_CONTROL, 0x0000); // clear clear
}

void gps_if_set_channel_sv(uint8_t channel, uint8_t sv)
{
    gps_if_spi_write(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_CONTROL, sv & 0x3F);
}

void gps_if_start_search(uint8_t search, uint8_t channel, uint8_t sv)
{
    gps_if_spi_write(GPS_SEARCH_OFFSET[search] + GPS_SEARCH_CONTROL, 0x4000 | ((channel & 0x3F) << 6) | (sv & 0x3F));
}

uint32_t gps_if_search_accumulator(uint8_t search)
{
    uint32_t acc_low = gps_if_spi_read(GPS_SEARCH_OFFSET[search] + GPS_SEARCH_ACC_LOW);
    uint32_t acc_high = gps_if_spi_read(GPS_SEARCH_OFFSET[search] + GPS_SEARCH_ACC_HIGH);

    return (acc_high << 16) | acc_low;
}

double gps_if_search_code(uint8_t search)
{
    uint16_t code_data = gps_if_spi_read(GPS_SEARCH_OFFSET[search] + GPS_SEARCH_CODE_START);
    double code = (double)(code_data & 0x3FF);
    code -= (double)((code_data >> 11) & 0x1F) * (1.023e6 / 19.2e6);

    return code;
}

double gps_if_search_doppler(uint8_t search)
{
    uint16_t dop_data = gps_if_spi_read(GPS_SEARCH_OFFSET[search] + GPS_SEARCH_DOPPLER);
    uint16_t doppler_sign = ((dop_data >> 5) & 0x1);
    int8_t doppler_int = (int8_t)(dop_data + (doppler_sign << 6) + (doppler_sign << 7)); // Sign extend to 8 bits then convert to int
    double doppler = (double)doppler_int * 250.0;

    return doppler;
}

bool gps_if_search_done(uint8_t search)
{
    uint16_t status = gps_if_spi_read(GPS_SEARCH_OFFSET[search] + GPS_SEARCH_CONTROL);
    if (status & 0x8000)
    {
        return false;
    }
    return true;
}

int32_t gps_if_channel_ip(uint8_t channel)
{
    uint32_t ip_lower = (uint32_t)gps_if_spi_read(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_IP_LOW);
    uint32_t ip_upper = (uint32_t)gps_if_spi_read(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_IP_HIGH);
    int32_t ip = std::bit_cast<int32_t>(ip_lower | (ip_upper << 16));
    return ip;
}

int32_t gps_if_channel_qp(uint8_t channel)
{
    uint32_t qp_lower = (uint32_t)gps_if_spi_read(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_QP_LOW);
    uint32_t qp_upper = (uint32_t)gps_if_spi_read(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_QP_HIGH);
    int32_t qp = std::bit_cast<int32_t>(qp_lower | (qp_upper << 16));
    return qp;
}

int32_t gps_if_channel_ie(uint8_t channel)
{
    uint32_t ie_lower = (uint32_t)gps_if_spi_read(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_IE_LOW);
    uint32_t ie_upper = (uint32_t)gps_if_spi_read(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_IE_HIGH);
    int32_t ie = std::bit_cast<int32_t>(ie_lower | (ie_upper << 16));
    return ie;
}

int32_t gps_if_channel_qe(uint8_t channel)
{
    uint32_t qe_lower = (uint32_t)gps_if_spi_read(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_QE_LOW);
    uint32_t qe_upper = (uint32_t)gps_if_spi_read(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_QE_HIGH);
    int32_t qe = std::bit_cast<int32_t>(qe_lower | (qe_upper << 16));
    return qe;
}

int32_t gps_if_channel_il(uint8_t channel)
{
    uint32_t il_lower = (uint32_t)gps_if_spi_read(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_IL_LOW);
    uint32_t il_upper = (uint32_t)gps_if_spi_read(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_IL_HIGH);
    int32_t il = std::bit_cast<int32_t>(il_lower | (il_upper << 16));
    return il;
}

int32_t gps_if_channel_ql(uint8_t channel)
{
    uint32_t ql_lower = (uint32_t)gps_if_spi_read(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_QL_LOW);
    uint32_t ql_upper = (uint32_t)gps_if_spi_read(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_QL_HIGH);
    int32_t ql = std::bit_cast<int32_t>(ql_lower | (ql_upper << 16));
    return ql;
}

void gps_if_set_channel_code_rate(uint8_t channel, uint32_t code_rate)
{
    gps_if_spi_write(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_CODE_RATE_LOW, (uint16_t)(code_rate & 0xFFFF));
    gps_if_spi_write(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_CODE_RATE_HIGH, (uint16_t)((code_rate >> 16) & 0xFFFF));
}

void gps_if_set_channel_lo_rate(uint8_t channel, uint32_t lo_rate)
{
    gps_if_spi_write(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_LO_RATE_LOW, (uint16_t)(lo_rate & 0xFFFF));
    gps_if_spi_write(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_LO_RATE_HIGH, (uint16_t)((lo_rate >> 16) & 0xFFFF));
}

bool gps_if_channel_is_epoch(uint8_t channel)
{

    uint16_t status = gps_if_spi_read(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_CONTROL);
    if (status & 0x8000)
    {
        return true;
    }
    return false;
}

void gps_if_channel_pause(uint8_t channel, uint16_t pause)
{
    gps_if_spi_write(GPS_CHANNEL_OFFSET[channel] + GPS_CHANNEL_PAUSE, pause);
}