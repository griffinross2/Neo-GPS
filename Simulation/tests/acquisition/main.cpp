#include "l1ca_search_ac_pca.h"
#include "e1_search_ac_pca.h"
#include "b1c_search_ac_pca.h"
#include "l1c_search_ac_pca.h"

#include <print>
#include <chrono>
#include <cmath>

constexpr double FS = 19.2e6;
constexpr double IF = 4.02e6;

int main()
{
    constexpr size_t num_samples = FS * 0.05;  // 50ms of samples
    int8_t* samples = new int8_t[num_samples]{0};

    FILE* f = fopen("signal.bin", "rb");
    if (!f)
    {
        std::println("Failed to open signal.bin");
        delete[] samples;
        return 1;
    }

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

    double code_phase = 0.0;
    double doppler = 0.0;
    double power = 0.0;

    // GPS L1 C/A

    std::println("Starting GPS search...");

    std::chrono::time_point<std::chrono::high_resolution_clock> start_time = std::chrono::high_resolution_clock::now();

    for (int sv = 1; sv <= 32; sv++)
    {
        l1ca_search_ac_pca(samples, IF, FS, sv, code_phase, doppler, power);
        std::string power_str = "*";
        for (int i = 0; i < int(power / 10.0); i++)
        {
            power_str += "*";
        }
        std::println("SV{:3}: code_phase={:10.3f} chips, doppler={:12.3f} Hz, SNR: {:4.0f} {}", sv, code_phase, doppler,
                     power, power_str);
    }
    std::println("");
    int waas_svs[3] = {131, 133, 135};
    for (int i = 0; i < 3; i++)
    {
        int sv = waas_svs[i];
        l1ca_search_ac_pca(samples, IF, FS, sv, code_phase, doppler, power);
        std::string power_str = "*";
        for (int i = 0; i < int(power / 10.0); i++)
        {
            power_str += "*";
        }
        std::println("SV{:3}: code_phase={:10.3f} chips, doppler={:12.3f} Hz, SNR: {:4.0f} {}", sv, code_phase, doppler,
                     power, power_str);
    }

    std::chrono::time_point<std::chrono::high_resolution_clock> end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_time = end_time - start_time;
    std::println("Search completed in {:.3f} seconds", elapsed_time.count());
    std::println("");

    // Galileo E1

    std::println("Starting Galileo search...");
    start_time = std::chrono::high_resolution_clock::now();

    for (int sv = 1; sv <= 36; sv++)
    {
        e1_search_ac_pca(samples, IF, FS, sv, code_phase, doppler, power);
        std::string power_str = "*";
        for (int i = 0; i < int(power / 10.0); i++)
        {
            power_str += "*";
        }
        std::println("SV{:3}: code_phase={:10.3f} chips, doppler={:12.3f} Hz, SNR: {:4.0f} {}", sv, code_phase, doppler,
                     power, power_str);
    }

    end_time = std::chrono::high_resolution_clock::now();
    elapsed_time = end_time - start_time;
    std::println("Search completed in {:.3f} seconds", elapsed_time.count());

    // // BeiDou B1C

    // std::println("Starting BeiDou search...");
    // start_time = std::chrono::high_resolution_clock::now();

    // for (int sv = 1; sv <= 63; sv++)
    // {
    //     b1c_search_ac_pca(samples, IF, FS, sv, code_phase, doppler, power);
    //     std::string power_str = "*";
    //     for (int i = 0; i < int(power / 10.0); i++)
    //     {
    //         power_str += "*";
    //     }
    //     std::println("SV{:3}: code_phase={:10.3f} chips, doppler={:12.3f} Hz, SNR: {:4.0f} {}", sv, code_phase,
    //     doppler,
    //                  power, power_str);
    // }

    // end_time = std::chrono::high_resolution_clock::now();
    // elapsed_time = end_time - start_time;
    // std::println("Search completed in {:.3f} seconds", elapsed_time.count());

    // // GPS L1C

    // std::println("Starting GPS L1C search...");
    // start_time = std::chrono::high_resolution_clock::now();

    // for (int sv = 1; sv <= 37; sv++)
    // {
    //     l1c_search_ac_pca(samples, IF, FS, sv, code_phase, doppler, power);
    //     std::string power_str = "*";
    //     for (int i = 0; i < int(power / 10.0); i++)
    //     {
    //         power_str += "*";
    //     }
    //     std::println("SV{:3}: code_phase={:10.3f} chips, doppler={:12.3f} Hz, SNR: {:4.0f} {}", sv, code_phase,
    //     doppler,
    //                  power, power_str);
    // }

    // end_time = std::chrono::high_resolution_clock::now();
    // elapsed_time = end_time - start_time;
    // std::println("Search completed in {:.3f} seconds", elapsed_time.count());

    delete[] samples;

    return 0;
}