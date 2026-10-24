#include "gps.h"

#include "gps_if.h"
#include "l1ca_tracker.h"
#include "l1ca_search.h"
#include "l1ca_ephemeris.h"
#include "solve.h"

#include "pico/stdlib.h"
#include "hardware/sync.h"

#include <array>
#include <stdio.h>

constexpr size_t GPS_NUM_CHANNELS = 6;
constexpr unsigned int GPS_SEARCH_THRESHOLD = 10000;
constexpr float GPS_KILL_THRESHOLD = 30.0f;
constexpr float GPS_KILL_MIN_ELAPSED_TIME_S = 2.0f;

std::array<bool, GPS_NUM_CHANNELS> channel_active;
std::array<bool, 32> sv_status;

std::array<L1CATracker, GPS_NUM_CHANNELS> trackers = {
    L1CATracker(19.2e6f, 4.02e6f), L1CATracker(19.2e6f, 4.02e6f), L1CATracker(19.2e6f, 4.02e6f),
    L1CATracker(19.2e6f, 4.02e6f), L1CATracker(19.2e6f, 4.02e6f), L1CATracker(19.2e6f, 4.02e6f),
};

uint32_t channel0_epoch_counter_test = 0;
size_t channel0_skip_count = 0;

L1CAEphemeris ephemeris;

L1CASearch search(0);

Solver<GPS_NUM_CHANNELS> solver;

bool search_active = false;
unsigned int search_sv = 0;
unsigned int search_channel = 0;

uint64_t longest_tracker_update_time = 0;

void gps_init()
{
    channel_active.fill(false);
    sv_status.fill(false);

    gps_if_init();

    // Interrupt initialization
    gpio_init(18);
    gpio_set_dir(18, false);
    gpio_set_irq_enabled_with_callback(18, GPIO_IRQ_LEVEL_HIGH, true, &epoch_callback);
    irq_set_priority(IO_IRQ_BANK0, PICO_HIGHEST_IRQ_PRIORITY);
}

void gps_task_search()
{
    if (!search_active)
    {
        // Pick the first available channel
        search_channel = 0xFFFFFFFF;
        for (unsigned int i = 0; i < channel_active.size(); ++i)
        {
            if (!channel_active[i])
            {
                search_channel = i;
                break;
            }
        }

        // If none available, return
        if (search_channel == 0xFFFFFFFF)
        {
            return;
        }

        // Skip SVs already being tracked
        while (sv_status[search_sv] && search_sv < 32)
        {
            search_sv++;
        }

        if (search_sv >= 32)
        {
            search_sv = 0;
            return;
        }

        // Start a new search on the selected channel
        search.start(search_channel, search_sv);
        search_active = true;
    }

    if (search_active && search.is_finished())
    {
        // Search finished, check result
        search_active = false;

        unsigned int acc = 0;
        float doppler = 0.0f;
        float code_phase = 0.0f;
        uint64_t start_time_us = 0;
        search.result(acc, doppler, code_phase, start_time_us);

        printf("Search: SV%d acc=%ld\n", search_sv + 1, acc);

        if (acc > GPS_SEARCH_THRESHOLD)
        {
            float elapsed_time_s = (time_us_64() - start_time_us) / 1e6f;

            // Represents the amount of chips by which the signal is advanced relative to the code
            double signal_advance = code_phase + doppler * elapsed_time_s * (1.0 / 1540.0);

            // Represents the amount of chips to delay the code to resolve the signal advance
            double delay_chips = 1023.0 - signal_advance - 0.5;
            delay_chips = fmod(fmod(delay_chips, 1023.0) + 1023.0, 1023.0);
            uint16_t delay_cycles = (uint32_t)llround(delay_chips * 19.2e6 / 1.023e6);
            uint32_t code_rate0 = (uint32_t)llround((1.023e6 + doppler * 1.023e6 / 1.57542e9) / 19.2e6 * 4294967296.0);
            uint32_t carrier_rate0 = (uint32_t)llround((4.02e6 + doppler) / 19.2e6 * 4294967296.0);

            gps_if_set_channel_code_rate(search_channel, code_rate0);
            gps_if_set_channel_lo_rate(search_channel, carrier_rate0);
            gps_if_channel_pause(search_channel, delay_cycles);

            trackers[search_channel].reset(search_sv, doppler);
            channel_active[search_channel] = true;
            sv_status[search_sv] = true;
        }

        search_sv = (search_sv + 1) % 32;
    }
}

void gps_task_report()
{
    for (size_t i = 0; i < channel_active.size(); ++i)
    {
        if (channel_active[i])
        {
            printf("Ch%zu SV%d track_mode=%d, bit_synced=%d, CN0 %.2f\n", i, trackers[i].get_sv() + 1,
                   trackers[i].get_mode(), trackers[i].is_bit_synced(), trackers[i].get_cn0());
        }
    }
    printf("Longest update took %.3f ms\n", longest_tracker_update_time / 1000.0f);
    printf("Channel 0 skip count: %zu\n", channel0_skip_count);
}

void gps_task_kill()
{
    for (size_t i = 0; i < channel_active.size(); ++i)
    {
        if (channel_active[i] && trackers[i].get_elapsed_time_s() > GPS_KILL_MIN_ELAPSED_TIME_S &&
            trackers[i].get_cn0() < GPS_KILL_THRESHOLD)
        {
            channel_active[i] = false;
            sv_status[trackers[i].get_sv()] = false;
            gps_if_clear_channel(i);
        }
    }
}

void gps_task_decode()
{
    for (size_t i = 0; i < channel_active.size(); ++i)
    {
        if (channel_active[i] && trackers[i].is_message_ready())
        {
            const std::array<uint8_t, 300>& waiting_message_buffer = trackers[i].get_waiting_message_buffer();
            ephemeris.update(trackers[i].get_sv(), waiting_message_buffer);
        }
    }
}

void gps_task_solve()
{
    double x, y, z, t_rx;
    size_t num_sats = 0;
    if (solver.solve(trackers, ephemeris, x, y, z, t_rx, num_sats))
    {
        double lat, lon, alt;
        ecef_to_coords(x, y, z, lat, lon, alt);
        printf("Sats: %zu, Latitude: %12.8f deg, Longitude: %12.8f deg, Altitude: %12.3f m\n", num_sats, lat, lon, alt);

        for (size_t i = 0; i < channel_active.size(); ++i)
        {
            if (channel_active[i])
            {
                uint32_t epoch_counter = 0;
                uint32_t chip = 0;
                uint32_t code_phase = 0;
                gps_if_read_snapshot(i, epoch_counter, chip, code_phase);
                printf("sv: %zu, time: %.8f\n", trackers[i].get_sv() + 1,
                       ephemeris.get_corrected_gps_time_of_week(trackers[i].get_sv(), epoch_counter, chip, code_phase));
            }
        }
    }
}

void __not_in_flash_func(epoch_callback)(unsigned int gpio, uint32_t event_mask)
{
    for (size_t i = 0; i < channel_active.size(); ++i)
    {
        uint64_t start_time = time_us_64();
        uint32_t epoch_counter = 0;
        if (gps_if_channel_is_epoch(i, epoch_counter) && channel_active[i])
        {
            if (i == 0)
            {
                if (epoch_counter != (channel0_epoch_counter_test + 1) % 6000)
                {
                    channel0_skip_count++;
                }
                channel0_epoch_counter_test = epoch_counter;
            }

            int32_t ip = gps_if_channel_ip(i);
            int32_t qp = gps_if_channel_qp(i);
            int32_t ie = gps_if_channel_ie(i);
            int32_t qe = gps_if_channel_qe(i);
            int32_t il = gps_if_channel_il(i);
            int32_t ql = gps_if_channel_ql(i);

            trackers[i].update(ie, qe, ip, qp, il, ql, 0.001f);

            gps_if_set_channel_code_rate(i, trackers[i].get_code_rate());
            gps_if_set_channel_lo_rate(i, trackers[i].get_lo_rate());

            if (trackers[i].is_message_ready() && !trackers[i].is_epochs_synced())
            {
                trackers[i].sync_epoch_counter();
                gps_if_reset_epoch_counter(i, trackers[i].get_sv());
            }
        }
        uint64_t end_time = time_us_64();
        if (end_time - start_time > longest_tracker_update_time)
        {
            longest_tracker_update_time = end_time - start_time;
        }
    }
}