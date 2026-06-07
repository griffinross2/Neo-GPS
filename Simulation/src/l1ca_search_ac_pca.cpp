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

#include "l1ca_search_ac_pca.h"

#include "constants.h"
#include "l1ca_code.h"
#include "fftw3.h"

static void correlate(fftw_complex *code, fftw_complex *signal, int len, GPS_Config_t &gps_conf, double doppler_range, unsigned int &code_phase_idx, int &doppler_idx, double &snr)
{
    // Now that we have a frequency domain representation of the signal
    // we can easily find the correct code phase and doppler. The doppler
    // shift is performed by a simple translation of the FFT and the code
    // phase will be revealed by the point of maximum power in the time
    // domain after inversely transforming the signal.

    // First create a buffer for the output data and a
    // plan for the inverse transform
    fftw_complex *correlation = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * len);
    fftw_plan plan = fftw_plan_dft_1d(len, correlation, correlation, FFTW_BACKWARD, FFTW_ESTIMATE);

    int max_snr_idx = 0;
    int max_snr_dop = 0;
    double max_snr = 0.0;

    // Search for doppler shifts from -doppler_range to +doppler_range each bin is len/CODE_RATE Hz wide
    for (int dop_shift = int(-1.0 * doppler_range * len / GPS_L1CA_CODE_RATE_CPS); dop_shift <= int(doppler_range * len / GPS_L1CA_CODE_RATE_CPS); dop_shift++)
    {
        int max_corr_idx = 0;
        double max_corr = 0.0;
        double total_corr = 0.0;

        for (int i = 0; i < len; i++)
        {
            // Create index accounting for roll-over
            int idx = (i - dop_shift + len) % len;
            correlation[i][0] = code[idx][0] * signal[i][0] + code[idx][1] * signal[i][1];
            correlation[i][1] = code[idx][1] * signal[i][0] - code[idx][0] * signal[i][1];
        }

        // Perform the inverse FFT
        fftw_execute(plan);

        // Look through the result for the maximum power point (only 1ms)
        int i;
        for (i = 0; i < 1023; i++)
        {
            double power = correlation[i][0] * correlation[i][0] + correlation[i][1] * correlation[i][1];
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
    fftw_destroy_plan(plan);
    fftw_free(correlation);
}

GPS_Status_t l1ca_search_ac_pca(uint8_t *samples, size_t num_samples, GPS_Config_t &gps_conf, int sv, double &code_phase, double &doppler, double &power)
{
    // Constants
    constexpr size_t FFT_SIZE = 4096;

    // This is the number of offsets to perform the correlation on as well as
    // the approximate number of samples that are averaged together. For a longer
    // integration time, more samples are average together but more offsets must be searched.
    const size_t NUM_OFFSETS = num_samples / FFT_SIZE;
    constexpr uint8_t carrier_sin[] = {1, 1, 0, 0};
    constexpr uint8_t carrier_cos[] = {1, 0, 0, 1};

    // Variables
    fftw_complex *sample_buf = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * FFT_SIZE);
    fftw_complex *sample_fft_buf = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * FFT_SIZE);
    fftw_plan plan_samples = fftw_plan_dft_1d(FFT_SIZE, sample_buf, sample_fft_buf, FFTW_FORWARD, FFTW_ESTIMATE);

    L1CACode code(l1ca_taps[sv][0], l1ca_taps[sv][1]);
    double *code_buf = (double *)fftw_malloc(sizeof(double) * FFT_SIZE);
    fftw_complex *code_fft_buf = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * FFT_SIZE);
    fftw_plan plan_code = fftw_plan_dft_r2c_1d(FFT_SIZE, code_buf, code_fft_buf, FFTW_ESTIMATE);

    double carrier_nco = 0.0;
    double code_nco = 0.0;

    size_t best_offset = 0;
    double best_power = 0.0;

    // Get code FFT
    for (size_t i = 0; i < FFT_SIZE; i++)
    {
        code_buf[i] = code.get_chip() ? 1.0 : -1.0;
        code.clock_chip();
    }
    fftw_execute(plan_code);
    fftw_destroy_plan(plan_code);

    for (size_t offset = 0; offset < NUM_OFFSETS; offset++)
    {
        // Average samples with this offset
        size_t dest_idx = 0;
        for (size_t i = 0; i < num_samples; i++)
        {
            sample_buf[dest_idx][0] += (samples[offset + i] ^ carrier_sin[(int)carrier_nco % 4]) ? -1.0 : 1.0;
            sample_buf[dest_idx][1] += (samples[offset + i] ^ carrier_cos[(int)carrier_nco % 4]) ? -1.0 : 1.0;

            // Increment code phase, and take the average at the end of each chip
            code_nco += GPS_L1CA_CODE_RATE_CPS / gps_conf.sample_rate_sps;
            if (code_nco >= 1.0)
            {
                sample_buf[dest_idx][0] = sample_buf[dest_idx][0] > 0 ? 1.0 : -1.0;
                sample_buf[dest_idx][1] = sample_buf[dest_idx][1] > 0 ? 1.0 : -1.0;
                dest_idx = (dest_idx + 1) % FFT_SIZE;
                code_nco -= 1.0;
            }

            // Increment carrier phase
            carrier_nco += 4 * gps_conf.if_freq_hz / gps_conf.sample_rate_sps;
            if (carrier_nco >= 4.0)
            {
                carrier_nco -= 4.0;
            }
        }

        // Perform FFT on samples
        fftw_execute(plan_samples);

        // Correlation
        unsigned int code_phase_idx = 0;
        int doppler_idx = 0;
        double this_power = 0.0;

        correlate(code_fft_buf, sample_fft_buf, FFT_SIZE, gps_conf, 5000.0, code_phase_idx, doppler_idx, this_power);

        if (this_power > best_power)
        {
            best_power = this_power;
            best_offset = offset;

            code_phase = code_phase_idx - (offset * GPS_L1CA_CODE_RATE_CPS / gps_conf.sample_rate_sps);
            doppler = doppler_idx * GPS_L1CA_CODE_RATE_CPS / FFT_SIZE;
            power = this_power;
        }
    }

    // Clean up
    fftw_destroy_plan(plan_samples);
    fftw_free(sample_buf);
    fftw_free(sample_fft_buf);
    fftw_free(code_buf);
    fftw_free(code_fft_buf);

    return OK;
}