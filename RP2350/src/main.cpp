#include "parallel_interface.h"

#include "pico/stdlib.h"
#include "tusb.h"
#include "gps_if.h"
#include "track_l1ca.h"

#include <cmath>
#include <stdio.h>

int main()
{
    tusb_init();
    stdio_init_all();

    gps_if_init();

    printf("Starting GPS search...\n");

    uint8_t sv;
    uint64_t start_time = 0;
    uint32_t acc = 0;
    double code = 0;
    double doppler = 0;

    for (sv = 0; sv <= 31; sv++)
    {
        gps_if_clear_channel(0);
        gps_if_set_channel_sv(0, sv);

        printf("Trying SV%d...\n", sv + 1);

        start_time = time_us_64();

        gps_if_start_search(0, 0, sv);
        while (!gps_if_search_done(0))
        {
            tud_task();
        }

        acc = gps_if_search_accumulator(0);
        gps_if_search_code_doppler(0, code, doppler);

        printf("Tracking SV%d: Accumulator = %lu, Code = %0.1f, Doppler = %0.0f\n", sv + 1, acc, code, doppler);

        if (acc > 25000)
        {
            break;
        }
    }

    if (sv <= 31)
    {
        double elapsed_time_s = (time_us_64() - start_time) / 1000000.0;

        // Represents the amount of chips by which the signal is advanced relative to the code
        double signal_advance = code + doppler * elapsed_time_s * (1.0 / 1540.0);

        // Represents the amount of chips to delay the code to resolve the signal advance
        double delay_chips = 1023.0 - signal_advance - 0.5;
        delay_chips = fmod(fmod(delay_chips, 1023.0) + 1023.0, 1023.0);

        GPSL1CATracker ch0_track(sv + 1, 19.2e6, 4.02e6, doppler);

        uint16_t delay_cycles = (uint32_t)llround(delay_chips * 19.2e6 / 1.023e6);
        printf("Signal advance: %.6f chips, delay: %.6f chips (%u cycles)\n", signal_advance, delay_chips,
               delay_cycles);

        // Initial code and carrier rates based on the search results
        uint32_t code_rate0 = (uint32_t)llround((1.023e6 + doppler * 1.023e6 / 1.57542e9) / 19.2e6 * 4294967296.0);
        uint32_t carrier_rate0 = (uint32_t)llround((4.02e6 + doppler) / 19.2e6 * 4294967296.0);
        gps_if_set_channel_code_rate(0, code_rate0);
        gps_if_set_channel_lo_rate(0, carrier_rate0);

        gps_if_channel_pause(0, delay_cycles);
        gps_if_channel_is_epoch(0);  // Ensure the epoch flag is clear

        while (!gps_if_channel_is_epoch(0))
        {
            tud_task();
        }  // Wait and ditch first epoch

        while (1)
        {
            while (!gps_if_channel_is_epoch(0))
            {
                tud_task();
                // Wait for epoch
            }

            // Read the I and Q values
            int32_t ip = gps_if_channel_ip(0);
            int32_t qp = gps_if_channel_qp(0);
            int32_t ie = gps_if_channel_ie(0);
            int32_t qe = gps_if_channel_qe(0);
            int32_t il = gps_if_channel_il(0);
            int32_t ql = gps_if_channel_ql(0);

            ch0_track.update_epoch(ip, qp, ie, qe, il, ql);
            uint32_t code_rate = ch0_track.get_code_rate();
            uint32_t carrier_rate = ch0_track.get_carrier_rate();
            gps_if_set_channel_code_rate(0, code_rate);
            gps_if_set_channel_lo_rate(0, carrier_rate);

            if (ch0_track.get_ms_elapsed() % 1000 == 0)
            {
                printf("C/N0: %.2f dB-Hz, C/N0 (smoothed): %.2f dB-Hz\n", ch0_track.get_cn0(),
                       ch0_track.get_cn0_smoothed());
            }
        }
    }
}