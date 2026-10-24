#include "pico/stdlib.h"

#include "gps.h"

uint64_t last_report_time = 0;

int main()
{
    stdio_init_all();

    gps_init();

    while (true)
    {
        gps_task_kill();
        gps_task_search();
        gps_task_decode();

        if (time_us_64() - last_report_time > 1000000)
        {
            last_report_time = time_us_64();
            gps_task_report();
            gps_task_solve();
        }
    }

    return 0;
}