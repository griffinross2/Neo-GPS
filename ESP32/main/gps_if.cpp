#include "gps_if.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

#include <stdint.h>
#include <memory.h>

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

uint32_t gps_if_accumulator(uint8_t search)
{
    uint32_t acc_low = gps_if_spi_read(GPS_SEARCH_OFFSET[search] + GPS_SEARCH_ACC_LOW);
    uint32_t acc_high = gps_if_spi_read(GPS_SEARCH_OFFSET[search] + GPS_SEARCH_ACC_HIGH);

    return (acc_high << 16) | acc_low;
}

double gps_if_code(uint8_t search)
{
    uint16_t code_data = gps_if_spi_read(GPS_SEARCH_OFFSET[search] + GPS_SEARCH_CODE_START);
    double code = (double)(code_data & 0x3FF);
    code -= (double)((code_data >> 11) & 0x1F) * (1.023e6 / 19.2e6);

    return code;
}

double gps_if_doppler(uint8_t search)
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