#include "l1ca_search_ac_pca.h"

#include <print>

constexpr double FS = 19.2e6;
constexpr double IF = 4.02e6;

int main()
{
    std::println("Starting search...");

    GPS_Config_t gps_conf;
    gps_conf.sample_rate_sps = FS;
    gps_conf.if_freq_hz = IF;

    constexpr size_t num_samples = FS * 0.05; // 50ms of samples
    uint8_t *samples = new uint8_t[num_samples]{0};

    FILE *f = fopen("signal.bin", "rb");
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

        samples[i] = ((byte >> nbit) & 0x1);
        nbit = (nbit + 1) % 8;
    }

    fclose(f);

    double code_phase = 0.0;
    double doppler = 0.0;
    double power = 0.0;

    for (int sv = 0; sv < 32; sv++)
    {
        l1ca_search_ac_pca(samples, (size_t)(gps_conf.sample_rate_sps * 0.004), gps_conf, sv, code_phase, doppler, power);
        std::string power_str = "";
        for (int i = 0; i < int(power / 10.0); i++)
        {
            power_str += "*";
        }
        std::println("SV{:3}: code_phase={:10.3f} chips, doppler={:12.3f} Hz, power: {}", sv + 1, code_phase, doppler, power_str);
    }

    delete[] samples;
    return 0;
}