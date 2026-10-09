#include "l1ca_search_ac_pca.h"
#include "l1ca_channel.h"
#include "l1ca_ephemeris.h"
#include "e1_search_ac_pca.h"
#include "e1_channel.h"
#include "e1_ephemeris.h"
#include "solve.h"
#include "tools.h"

#include <print>
#include <chrono>
#include <cmath>
#include <numbers>
#include <fstream>

constexpr double FS = 19.2e6;
constexpr double IF = 4.02e6;

int8_t* samples = nullptr;
size_t num_samples = 0;

L1CAEphemeris l1ca_ephemeris;
std::array<L1CAChannel, 6> l1ca_channels{
    L1CAChannel(FS, IF, l1ca_ephemeris), L1CAChannel(FS, IF, l1ca_ephemeris), L1CAChannel(FS, IF, l1ca_ephemeris),
    L1CAChannel(FS, IF, l1ca_ephemeris), L1CAChannel(FS, IF, l1ca_ephemeris), L1CAChannel(FS, IF, l1ca_ephemeris),
};

E1Ephemeris e1_ephemeris;
std::array<E1Channel, 6> e1_channels{
    E1Channel(FS, IF, e1_ephemeris), E1Channel(FS, IF, e1_ephemeris), E1Channel(FS, IF, e1_ephemeris),
    E1Channel(FS, IF, e1_ephemeris), E1Channel(FS, IF, e1_ephemeris), E1Channel(FS, IF, e1_ephemeris),
};

void acquire_l1ca(int8_t* samples);
void acquire_e1(int8_t* samples);
bool get_samples();

int main()
{
    if (!get_samples())
    {
        return 1;
    }

    auto solve_log = std::ofstream("solve.csv");
    solve_log << "time,num_sats,x,y,z,lat,lon,alt\n";
    solve_log << std::fixed << std::setprecision(10);

    // TRACK

    Solver<8> solver;

    for (size_t i = 0; i < num_samples; i++)
    {
        // Try to acquire new satellites every 5 seconds
        if (i % (size_t)(FS * 5) == 0)
        {
            for (size_t j = 0; j < l1ca_channels.size(); j++)
            {
                L1CAChannel& channel = l1ca_channels[j];
                if (channel.get_cn0() < 30.0 && channel.is_active())
                {
                    std::println("Stopping L1CA channel {} (SV {})", j, channel.get_sv());
                    channel.stop();
                }
            }
            for (size_t j = 0; j < e1_channels.size(); j++)
            {
                E1Channel& channel = e1_channels[j];
                if (channel.get_cn0() < 30.0 && channel.is_active())
                {
                    std::println("Stopping E1 channel {} (SV {})", j, channel.get_sv());
                    channel.stop();
                }
            }

            acquire_l1ca(samples + i);
            acquire_e1(samples + i);
        }

        // L1CA Channels
        for (auto& channel : l1ca_channels)
        {
            channel.update(samples[i]);

            if (i % (size_t)(FS * 1) == 0 && channel.is_active())  // Print every 100ms
            {
                std::println("L1CA SV {}: cn0 = {:.2f} dB-Hz", channel.get_sv(), channel.get_cn0());
            }
        }

        // L1CA Channels
        for (auto& channel : e1_channels)
        {
            channel.update(samples[i]);

            if (i % (size_t)(FS * 1) == 0 && channel.is_active())  // Print every 100ms
            {
                std::println("E1 SV {}: cn0 = {:.2f} dB-Hz", channel.get_sv(), channel.get_cn0());
            }
        }

        // Try to solve every 2 seconds
        if (i % (size_t)(FS * 2) == 0)
        {
            double x, y, z, t_rx;
            size_t num_sats = 0;
            bool solved = solver.solve(l1ca_channels, e1_channels, x, y, z, t_rx, num_sats);
            if (solved)
            {
                std::println(
                    "Solved position with {} satellites: x = {:12.3f}, y = {:12.3f}, z = {:12.3f}, t_rx = {:20.10f}",
                    num_sats, x, y, z, t_rx);

                double lat, lon, alt;
                ecef_to_coords(x, y, z, lat, lon, alt);

                std::println("Latitude: {:12.8f} deg, Longitude: {:12.8f} deg, Altitude: {:12.3f} m", lat, lon, alt);

                solve_log << (i / FS) << "," << num_sats << "," << x << "," << y << "," << z << "," << lat << "," << lon
                          << "," << alt << "\n";
            }
            else
            {
                std::println("Not enough satellites to solve");
            }
        }
    }

    solve_log.close();
    delete[] samples;

    return 0;
}

void acquire_l1ca(int8_t* samples)
{
    const std::chrono::time_point<std::chrono::high_resolution_clock> start_time =
        std::chrono::high_resolution_clock::now();

    for (int sv = 1; sv <= 32; sv++)
    {
        double code_phase = 0.0;
        double doppler = 0.0;
        double power = 0.0;

        bool already_acquired = false;
        for (auto& channel : l1ca_channels)
        {
            if (channel.get_sv() == sv && channel.is_active())
            {
                already_acquired = true;
                break;
            }
        }

        if (already_acquired)
        {
            continue;
        }

        l1ca_search_ac_pca(samples, IF, FS, sv, code_phase, doppler, power);

        if (power > 20.0)
        {
            for (size_t j = 0; j < l1ca_channels.size(); j++)
            {
                L1CAChannel& channel = l1ca_channels[j];
                if (!channel.is_active())
                {
                    std::println("Starting L1CA SV {} on channel {}", sv, j);
                    channel.start(sv, doppler, code_phase);
                    break;
                }
            }
        }
    }

    const std::chrono::time_point<std::chrono::high_resolution_clock> end_time =
        std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> elapsed_time = end_time - start_time;
    std::println("L1CA search completed in {:.3f} seconds", elapsed_time.count());
}

void acquire_e1(int8_t* samples)
{
    const std::chrono::time_point<std::chrono::high_resolution_clock> start_time =
        std::chrono::high_resolution_clock::now();

    for (int sv = 1; sv <= 32; sv++)
    {
        double code_phase = 0.0;
        double doppler = 0.0;
        double power = 0.0;

        bool already_acquired = false;
        for (auto& channel : e1_channels)
        {
            if (channel.get_sv() == sv && channel.is_active())
            {
                already_acquired = true;
                break;
            }
        }

        if (already_acquired)
        {
            continue;
        }

        e1_search_ac_pca(samples, IF, FS, sv, code_phase, doppler, power);

        if (power > 15.0)
        {
            for (size_t j = 0; j < e1_channels.size(); j++)
            {
                E1Channel& channel = e1_channels[j];
                if (!channel.is_active())
                {
                    std::println("Starting E1 SV {} on channel {}", sv, j);
                    channel.start(sv, doppler, code_phase);
                    break;
                }
            }
        }
    }

    const std::chrono::time_point<std::chrono::high_resolution_clock> end_time =
        std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> elapsed_time = end_time - start_time;
    std::println("E1 search completed in {:.3f} seconds", elapsed_time.count());
}

bool get_samples()
{
    FILE* f = fopen("signal.bin", "rb");
    if (!f)
    {
        std::println("Failed to open signal.bin");
        return false;
    }

    fseek(f, 0, SEEK_END);
    num_samples = size_t(ftell(f)) * size_t(8);
    num_samples = std::min(num_samples, (size_t)(size_t(FS) * size_t(60 * 9)));  // 9 min of samples
    fseek(f, 0, SEEK_SET);
    samples = new int8_t[num_samples];

    int nbit = 0;
    uint8_t byte = 0;
    for (size_t i = 0; i < num_samples; i++)
    {
        // Top of byte
        if (nbit == 0)
        {
            if (fread(&byte, sizeof(char), 1, f) != 1)
                break;
        }

        samples[i] = ((byte >> nbit) & 0x1) ? 1 : -1;
        nbit = (nbit + 1) % 8;
    }

    fclose(f);

    return true;
}