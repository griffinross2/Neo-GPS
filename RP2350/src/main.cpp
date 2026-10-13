#include "parallel_interface.h"

#include "pico/stdlib.h"

int main()
{
    parallel_interface_init();

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, true);
    while (true)
    {
        gpio_put(PICO_DEFAULT_LED_PIN, true);
        sleep_ms(500);
        gpio_put(PICO_DEFAULT_LED_PIN, false);
        sleep_ms(500);

        // parallel_interface_write(0xdead, 0x12345678);
        uint32_t data = parallel_interface_read(0xdead);
    }
}