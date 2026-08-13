#include "verilated.h"
#include "Vgps_tb_gps_tb.h"

#include "spi_host.h"

int spi_bit_counter = 0;
int spi_clock_counter = 0;
constexpr int SPI_CLOCK_DIVIDER = 1000;
bool read_write = false; // false for write, true for read
uint8_t spi_wdata = 0;
uint8_t spi_rdata = 0;
uint8_t spi_addr = 0;

typedef enum
{
    SPI_IDLE,
    SPI_ADDR,
    SPI_DATA,
    SPI_CS_TIME,
    SPI_END
} spi_state_t;

spi_state_t spi_state = SPI_IDLE;

void spi_init(Vgps_tb *const top)
{
    top->gps_tb->sck = 0;
    top->gps_tb->cs = 1;
    top->gps_tb->sdi = 0;
}

void spi_write(Vgps_tb *const top, uint8_t addr, uint8_t data)
{
    spi_wdata = data;
    spi_addr = addr;
    spi_state = SPI_ADDR;
    top->gps_tb->cs = 0;  // Assert chip select
    read_write = false;   // Set to write mode
    top->gps_tb->sdi = 0; // Send read/write bit first
}

void spi_read(Vgps_tb *const top, uint8_t addr)
{
    spi_addr = addr;
    spi_state = SPI_ADDR;
    top->gps_tb->cs = 0;  // Assert chip select
    read_write = true;    // Set to read mode
    top->gps_tb->sdi = 1; // Send read/write bit first
}

void spi_task(Vgps_tb *const top)
{

    switch (spi_state)
    {
    case SPI_IDLE:

        break;
    case SPI_ADDR:
        if (spi_clock_counter >= SPI_CLOCK_DIVIDER - 1)
        {
            spi_clock_counter = 0;
            top->gps_tb->sck = !top->gps_tb->sck; // Toggle clock

            if (top->gps_tb->sck == 0)
            {
                top->gps_tb->sdi = (spi_addr >> (6 - spi_bit_counter)) & 0x1; // Send address bit
                spi_bit_counter++;

                if (spi_bit_counter > 6)
                {
                    spi_bit_counter = 0;
                    spi_state = SPI_DATA;
                }
            }
        }
        else
        {
            spi_clock_counter++;
        }

        break;
    case SPI_DATA:
        if (spi_clock_counter >= SPI_CLOCK_DIVIDER - 1)
        {
            spi_clock_counter = 0;
            top->gps_tb->sck = !top->gps_tb->sck; // Toggle clock

            if (!read_write && top->gps_tb->sck == 0)
            {
                // Write bits at each negedge
                top->gps_tb->sdi = (spi_wdata >> (7 - spi_bit_counter)) & 0x1; // Send data bit
                spi_bit_counter++;

                if (spi_bit_counter > 7)
                {
                    spi_bit_counter = 0;
                    spi_state = SPI_CS_TIME;
                }
            }

            if (read_write && top->gps_tb->sck == 1)
            {
                // Read bits at each posedge
                spi_rdata = (spi_rdata << 1) | top->gps_tb->sdo; // Read data bit
                spi_bit_counter++;

                if (spi_bit_counter > 8)
                {
                    spi_bit_counter = 0;
                    spi_state = SPI_CS_TIME;
                }
            }
        }
        else
        {
            spi_clock_counter++;
        }

        break;
    case SPI_CS_TIME:

        if (spi_clock_counter >= SPI_CLOCK_DIVIDER - 1)
        {
            spi_clock_counter = 0;
            top->gps_tb->sck = !top->gps_tb->sck; // Toggle clock

            if (top->gps_tb->sck == 0)
            {
                top->gps_tb->cs = 1; // Deassert chip select
                spi_bit_counter = 0;
                spi_state = SPI_END;
            }
        }
        else
        {
            spi_clock_counter++;
        }

        break;

    case SPI_END:
        if (spi_clock_counter >= 2 * SPI_CLOCK_DIVIDER - 1)
        {
            spi_bit_counter = 0;
            spi_state = SPI_IDLE;
        }
        else
        {
            spi_clock_counter++;
        }
    }
}

bool spi_is_idle()
{
    return spi_state == SPI_IDLE;
}

uint8_t spi_get_read_data()
{
    return spi_rdata;
}