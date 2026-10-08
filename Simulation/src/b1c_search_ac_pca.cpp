/* clang-format off */
/**************************************************************************************/
/*                        Averaging Correlation PCA Search                            */
/*                                   Based on:                                        */
/*                                                                                    */
/*                             J. A. Starzyk and Z. Zhu,                              */
/* "Averaging correlation for C/A code acquisition and tracking in frequency domain," */
/*    Proceedings of the 44th IEEE 2001 Midwest Symposium on Circuits and Systems.    */
/*                 MWSCAS 2001 (Cat. No.01CH37257), Dayton, OH, USA,                  */
/*                            2001, pp. 905-908 vol.2,                                */
/*                        doi: 10.1109/MWSCAS.2001.986334.                            */
/**************************************************************************************/
/* clang-format on */

#include "b1c_search_ac_pca.h"

#include "constants.h"
#include "b1c_code.h"

#include "fftw3.h"

#include <print>
#include <cstring>

static void correlate(fftw_complex* code_data, fftw_complex* code_pilot, fftw_complex* signal, int len,
                      double freq_if_hz, double freq_sample_hz, double doppler_range, unsigned int& code_phase_idx,
                      int& doppler_idx, double& snr)
{
    // Now that we have a frequency domain representation of the signal
    // we can easily find the correct code phase and doppler. The doppler
    // shift is performed by a simple translation of the FFT and the code
    // phase will be revealed by the point of maximum power in the time
    // domain after inversely transforming the signal.

    // First create a buffer for the output data and a
    // plan for the inverse transform
    fftw_complex* correlation_data = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * len);
    fftw_complex* correlation_pilot = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * len);
    fftw_plan plan_data = fftw_plan_dft_1d(len, correlation_data, correlation_data, FFTW_BACKWARD, FFTW_ESTIMATE);
    fftw_plan plan_pilot = fftw_plan_dft_1d(len, correlation_pilot, correlation_pilot, FFTW_BACKWARD, FFTW_ESTIMATE);

    int max_snr_idx = 0;
    int max_snr_dop = 0;
    double max_snr = 0.0;

    // Search for doppler shifts from -doppler_range to +doppler_range each bin
    // is len/CODE_RATE Hz wide
    for (int dop_shift = int(-1.0 * doppler_range * len / BEIDOU_B1C_CODE_RATE_CPS);
         dop_shift <= int(doppler_range * len / BEIDOU_B1C_CODE_RATE_CPS); dop_shift++)
    {
        int max_corr_idx = 0;
        double max_corr = 0.0;
        double total_corr = 0.0;

        for (int i = 0; i < len; i++)
        {
            // Create index accounting for roll-over
            int idx = (i - dop_shift + len) % len;
            correlation_data[i][0] = code_data[idx][0] * signal[i][0] + code_data[idx][1] * signal[i][1];
            correlation_data[i][1] = code_data[idx][1] * signal[i][0] - code_data[idx][0] * signal[i][1];
            correlation_pilot[i][0] = code_pilot[idx][0] * signal[i][0] + code_pilot[idx][1] * signal[i][1];
            correlation_pilot[i][1] = code_pilot[idx][1] * signal[i][0] - code_pilot[idx][0] * signal[i][1];
        }

        // Perform the inverse FFT
        fftw_execute(plan_data);
        fftw_execute(plan_pilot);

        // Look through the result for the maximum power point (only 10ms)
        int i;
        for (i = 0; i < 10230; i++)
        {
            double power =
                correlation_data[i][0] * correlation_data[i][0] + correlation_data[i][1] * correlation_data[i][1] +
                correlation_pilot[i][0] * correlation_pilot[i][0] + correlation_pilot[i][1] * correlation_pilot[i][1];
            if (power > max_corr)
            {
                max_corr = power;
                max_corr_idx = i;
            }
            total_corr += power;
        }

        // Calculate the SNR
        double snr = max_corr / (total_corr / i);
        if (snr > max_snr)
        {
            max_snr = snr;
            max_snr_idx = max_corr_idx;
            max_snr_dop = dop_shift;
        }
    }

    // Return the results
    code_phase_idx = max_snr_idx;
    doppler_idx = max_snr_dop;
    snr = max_snr;

    // Clean up
    fftw_destroy_plan(plan_data);
    fftw_destroy_plan(plan_pilot);
    fftw_free(correlation_data);
    fftw_free(correlation_pilot);
}

int b1c_search_ac_pca(int8_t* samples, double freq_if_hz, double freq_sample_hz, int sv, double& code_phase,
                      double& doppler, double& power)
{
    // Constants
    constexpr size_t FFT_SIZE = 16384;

    const size_t NUM_OFFSETS = static_cast<size_t>(freq_sample_hz / BEIDOU_B1C_CODE_RATE_CPS);
    constexpr int8_t carrier_sin[] = {1, 1, -1, -1};
    constexpr int8_t carrier_cos[] = {1, -1, -1, 1};
    constexpr int8_t boc1_sin[] = {1, -1};

    // Variables
    fftw_complex* sample_buf = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * FFT_SIZE);
    fftw_complex* sample_fft_buf = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * FFT_SIZE);
    fftw_plan plan_samples = fftw_plan_dft_1d(FFT_SIZE, sample_buf, sample_fft_buf, FFTW_FORWARD, FFTW_ESTIMATE);

    B1CCode code(sv);
    fftw_complex* code_data_buf = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * FFT_SIZE);
    fftw_complex* code_pilot_buf = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * FFT_SIZE);
    fftw_complex* code_data_fft_buf = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * FFT_SIZE);
    fftw_complex* code_pilot_fft_buf = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * FFT_SIZE);
    fftw_plan plan_data_code =
        fftw_plan_dft_1d(FFT_SIZE, code_data_buf, code_data_fft_buf, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_plan plan_pilot_code =
        fftw_plan_dft_1d(FFT_SIZE, code_pilot_buf, code_pilot_fft_buf, FFTW_FORWARD, FFTW_ESTIMATE);

    double carrier_nco = 0.0;
    double code_nco = 0.0;

    double best_power = 0.0;

    // Get code FFT
    for (size_t i = 0; i < FFT_SIZE; i++)
    {
        code_data_buf[i][0] = static_cast<double>(code.get_chip_data());
        code_data_buf[i][1] = 0.0;
        code_pilot_buf[i][0] = static_cast<double>(code.get_chip_pilot());
        code_pilot_buf[i][1] = 0.0;
        code.clock_chip();
    }
    fftw_execute(plan_data_code);
    fftw_execute(plan_pilot_code);
    fftw_destroy_plan(plan_data_code);
    fftw_destroy_plan(plan_pilot_code);

    code_nco = 0.0;
    carrier_nco = 0.0;

    for (size_t offset = 0; offset < NUM_OFFSETS; offset++)
    {
        memset(sample_buf, 0, sizeof(fftw_complex) * FFT_SIZE);

        // Average samples with this offset
        size_t dest_idx = 0;
        size_t i = offset;
        while (dest_idx < FFT_SIZE)
        {
            sample_buf[dest_idx][0] += static_cast<double>(
                (samples[i] * carrier_cos[(int)carrier_nco % 4] * boc1_sin[(int)(code_nco * 2) % 2]));
            sample_buf[dest_idx][1] += static_cast<double>((samples[i] * carrier_sin[(int)carrier_nco % 4]) * -1.0 *
                                                           boc1_sin[(int)(code_nco * 2) % 2]);

            // Increment code phase, and take the average at the end of each chip
            code_nco += BEIDOU_B1C_CODE_RATE_CPS / freq_sample_hz;
            if (code_nco >= 1.0)
            {
                sample_buf[dest_idx][0] = ((sample_buf[dest_idx][0] > 0) ? 1.0 : -1.0);
                sample_buf[dest_idx][1] = ((sample_buf[dest_idx][1] > 0) ? 1.0 : -1.0);
                dest_idx++;
                code_nco -= 1.0;
            }

            // Increment carrier phase
            carrier_nco += 4 * freq_if_hz / freq_sample_hz;
            if (carrier_nco >= 4.0)
            {
                carrier_nco -= 4.0;
            }

            i++;
        }

        // Perform FFT on samples
        fftw_execute(plan_samples);

        // Correlation
        unsigned int code_phase_idx = 0;
        int doppler_idx = 0;
        double this_power = 0.0;

        correlate(code_data_fft_buf, code_pilot_fft_buf, sample_fft_buf, FFT_SIZE, freq_if_hz, freq_sample_hz, 5000.0,
                  code_phase_idx, doppler_idx, this_power);

        if (this_power > best_power)
        {
            best_power = this_power;

            code_phase = (code_phase_idx * 10230.0 / FFT_SIZE) - (offset * BEIDOU_B1C_CODE_RATE_CPS / freq_sample_hz);
            doppler = doppler_idx * BEIDOU_B1C_CODE_RATE_CPS / FFT_SIZE;
            power = this_power;
        }
    }

    // Clean up
    fftw_destroy_plan(plan_samples);
    fftw_free(sample_buf);
    fftw_free(sample_fft_buf);
    fftw_free(code_data_buf);
    fftw_free(code_data_fft_buf);
    fftw_free(code_pilot_buf);
    fftw_free(code_pilot_fft_buf);

    return 0;
}