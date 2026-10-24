#include "parallel_interface.h"

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/sync.h"

#include "parallel_interface.pio.h"

const PIO pio = pio0;
const unsigned sm = 0;

void parallel_interface_init()
{
    const unsigned offset = pio_add_program(pio, &parallel_interface_program);

    pio_sm_config cfg = parallel_interface_program_get_default_config(offset);

    sm_config_set_out_pins(&cfg, 2, 16);
    sm_config_set_in_pins(&cfg, 2);
    sm_config_set_sideset_pins(&cfg, 0);
    sm_config_set_in_shift(&cfg, true, false, 32);
    sm_config_set_out_shift(&cfg, false, false, 32);
    sm_config_set_clkdiv(&cfg, 10.0f);

    for (unsigned i = 0; i < 18; i++)
    {
        pio_gpio_init(pio, i);
        if (i >= 2)
        {
            gpio_pull_up(i);
        }
    }
    pio_sm_set_consecutive_pindirs(pio, sm, 0, 18, true);

    pio_sm_init(pio, sm, offset, &cfg);

    sleep_ms(10);
}

void parallel_interface_write(uint16_t addr, uint32_t data)
{
    uint32_t int_status = save_and_disable_interrupts();

    uint32_t addr_word = (uint32_t)(addr & 0x7FFF) << 16;

    pio_sm_set_consecutive_pindirs(pio, sm, 2, 16, true);
    pio_sm_put_blocking(pio, sm, 0xFFFF0000);  // Pin Dirs
    pio_sm_put_blocking(pio, sm, addr_word);   // Address
    pio_sm_put_blocking(pio, sm, data);        // Data
    pio_sm_set_enabled(pio, sm, true);
    pio_sm_get_blocking(pio, sm);  // Dummy read
    pio_sm_set_enabled(pio, sm, false);

    restore_interrupts(int_status);
}

uint32_t parallel_interface_read(uint16_t addr)
{
    uint32_t int_status = save_and_disable_interrupts();

    uint32_t data = 0x0;
    uint32_t addr_word = (uint32_t)(addr & 0x7FFF) << 16 | 0x80000000;

    pio_sm_set_consecutive_pindirs(pio, sm, 2, 16, true);
    pio_sm_put_blocking(pio, sm, 0x00000000);  // Pin Dirs
    pio_sm_put_blocking(pio, sm, addr_word);   // Address
    pio_sm_put_blocking(pio, sm, 0x00000000);  // Dummy write
    pio_sm_set_enabled(pio, sm, true);
    data = pio_sm_get_blocking(pio, sm);  // Read data
    pio_sm_set_enabled(pio, sm, false);

    // Need to exchange the 16-bit words in the data
    data = (data << 16) | (data >> 16);

    restore_interrupts(int_status);

    return data;
}