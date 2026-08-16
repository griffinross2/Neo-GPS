#include "test_program.h"
#include "spi_host.h"

#include <iostream>

#include "verilated.h"
#include "Vgps_tb_gps_tb.h"

typedef enum
{
    TEST_IDLE,
    TEST_WAIT_FOR_CHANNEL_CONFIG,
    TEST_WAIT_FOR_SEARCH,
    TEST_WAIT_FOR_SEARCH_READ_STATUS,
    TEST_WAIT_FOR_SEARCH_READ_ACC0,
    TEST_WAIT_FOR_SEARCH_READ_ACC1,
    TEST_WAIT_FOR_SEARCH_READ_CODE_START,
    TEST_WAIT_FOR_SEARCH_READ_DOP,
    TEST_DONE,
} test_state_t;

test_state_t test_state = TEST_IDLE;

uint32_t accumulator = 0;
double code = 0;
double doppler = 0;

void test_program_init(Vgps_tb *const top)
{
    spi_init(top);
    test_state = TEST_IDLE;
}

void test_program_task(VerilatedContext *const contextp, Vgps_tb *const top)
{
    spi_task(top);
    switch (test_state)
    {
    case TEST_IDLE:
        if (contextp->time() == 1e6 * 1)
        {
            // Start a search
            spi_write(top, 0x0020, 0x4006); // Clear channel 0 bit and sv=6
            test_state = TEST_WAIT_FOR_CHANNEL_CONFIG;
        }
        break;
    case TEST_WAIT_FOR_CHANNEL_CONFIG:
        if (spi_is_idle())
        {
            // Start a search
            spi_write(top, 0x0010, 0x4006); // Write start bit, sv=6, ch=0

            // Previous search start finished
            std::cout << "Starting search at time " << contextp->time() << std::endl;
            test_state = TEST_WAIT_FOR_SEARCH;
        }
        break;
    case TEST_WAIT_FOR_SEARCH:
        if (spi_is_idle())
        {
            // Start a read of the status register
            spi_read(top, 0x0010);
            test_state = TEST_WAIT_FOR_SEARCH_READ_STATUS;
        }
        break;
    case TEST_WAIT_FOR_SEARCH_READ_STATUS:
        if (spi_is_idle())
        {
            uint16_t read_data = spi_get_read_data();
            if (read_data & 0x8000)
            {
                // Check again
                test_state = TEST_WAIT_FOR_SEARCH_READ_STATUS;
            }
            else
            {
                // Search is done
                spi_read(top, 0x0011);
                test_state = TEST_WAIT_FOR_SEARCH_READ_ACC0;
                std::cout << "Search done at time " << contextp->time() << std::endl;
            }
        }
        break;
    case TEST_WAIT_FOR_SEARCH_READ_ACC0:
        if (spi_is_idle())
        {
            uint16_t read_data = spi_get_read_data();
            spi_read(top, 0x0012);
            accumulator = (uint32_t)read_data;
            test_state = TEST_WAIT_FOR_SEARCH_READ_ACC1;
        }
        break;
    case TEST_WAIT_FOR_SEARCH_READ_ACC1:
        if (spi_is_idle())
        {
            uint16_t read_data = spi_get_read_data();
            spi_read(top, 0x0013);
            accumulator |= (uint32_t)read_data << 16;
            test_state = TEST_WAIT_FOR_SEARCH_READ_CODE_START;
        }
        break;
    case TEST_WAIT_FOR_SEARCH_READ_CODE_START:
        if (spi_is_idle())
        {
            uint16_t read_data = spi_get_read_data();
            spi_read(top, 0x0014);
            code = (double)(read_data & 0x3FF);
            code -= (double)((read_data >> 11) & 0x1F) * (1.023e6 / 19.2e6); // Start position in signal sample
            test_state = TEST_WAIT_FOR_SEARCH_READ_DOP;
        }
        break;
    case TEST_WAIT_FOR_SEARCH_READ_DOP:
        if (spi_is_idle())
        {
            uint16_t read_data = spi_get_read_data();
            uint16_t doppler_sign = ((read_data >> 5) & 0x1);
            int8_t doppler_int = (int8_t)(read_data + (doppler_sign << 6) + (doppler_sign << 7)); // Sign extend to 8 bits then convert to int
            doppler = (double)doppler_int * 250.0;                                                // Doppler in Hz

            std::cout << "Search results at time " << contextp->time() << std::endl;
            std::cout << "Accumulator: " << accumulator << std::endl;
            std::cout << "Code: " << code << std::endl;
            std::cout << "Doppler: " << doppler << std::endl;

            test_state = TEST_DONE;
        }
        break;
    case TEST_DONE:
        break;
    default:
        break;
    }
}