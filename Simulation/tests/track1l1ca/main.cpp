#include "l1ca_search_ac_pca.h"
#include "l1ca_channel.h"
#include "l1ca_ephemeris.h"

#include <print>
#include <chrono>

constexpr double FS = 19.2e6;
constexpr double IF = 4.02e6;

int main()
{
    FILE* f = fopen("signal.bin", "rb");
    if (!f)
    {
        std::println("Failed to open signal.bin");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    size_t num_samples = ftell(f) * 8;
    num_samples = std::min(num_samples, (size_t)(FS * 60));  // 60s of samples
    fseek(f, 0, SEEK_SET);
    int8_t* samples = new int8_t[num_samples];

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

    int best_sv = 0;
    double best_code_phase = 0.0;
    double best_doppler = 0.0;
    double best_power = 0.0;

    const std::chrono::time_point<std::chrono::high_resolution_clock> start_time =
        std::chrono::high_resolution_clock::now();

    std::println("Starting search...");

    for (int sv = 1; sv <= 32; sv++)
    {
        double code_phase = 0.0;
        double doppler = 0.0;
        double power = 0.0;

        l1ca_search_ac_pca(samples, IF, FS, sv, code_phase, doppler, power);

        if (power > best_power)
        {
            best_sv = sv;
            best_power = power;
            best_code_phase = code_phase;
            best_doppler = doppler;
        }
    }

    const std::chrono::time_point<std::chrono::high_resolution_clock> end_time =
        std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> elapsed_time = end_time - start_time;
    std::println("Search completed in {:.3f} seconds", elapsed_time.count());

    std::println("Best SV: sv={}, code_phase={:10.3f} chips, doppler={:12.3f} Hz, power: {:4.0f}", best_sv,
                 best_code_phase, best_doppler, best_power);

    L1CAEphemeris l1ca_ephemeris;
    L1CAChannel channel(FS, IF, l1ca_ephemeris);
    channel.start(best_sv, best_doppler, best_code_phase);

    for (size_t i = 0; i < num_samples; i++)
    {
        channel.update(samples[i]);

        if (i % (size_t)(FS * 0.1) == 0)  // Print every 100ms
        {
            std::println("ip = {:8}, qp = {:8}, cn0 = {:.2f} dB-Hz", channel.get_last_ip(), channel.get_last_qp(),
                         channel.get_cn0());
        }
    }

    delete[] samples;

    return 0;
}