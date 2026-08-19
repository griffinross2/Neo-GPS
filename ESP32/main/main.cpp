/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "gps_if.h"

extern "C"
{
    void app_main(void);
}

void app_main(void)
{
    vTaskDelay(10 / portTICK_PERIOD_MS);

    gps_if_init();

    for (uint8_t sv = 0; sv <= 31; sv++)
    {
        gps_if_start_search(0, 0, sv);
        while (!gps_if_search_done(0))
        {
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        uint32_t acc = gps_if_accumulator(0);
        double code = gps_if_code(0);
        double doppler = gps_if_doppler(0);

        printf("SV%d: Accumulator = %lu, Code = %0.1f, Doppler = %0.0f\n", sv, acc, code, doppler);
    }

    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
