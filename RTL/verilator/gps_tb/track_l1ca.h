#ifndef TRACK_L1CA_H
#define TRACK_L1CA_H

#include <stdint.h>
#include "tools.h"
#include "filters.h"
#include "ephm_l1ca.h"
#include <fstream>

class GPSL1CATracker
{
public:
    GPSL1CATracker(
        int sv,
        double fs = 19.2e6,
        double fc = 4.02e6,
        double doppler = 0);

    ~GPSL1CATracker();

    void update_epoch(int32_t ip, int32_t qp, int32_t ie, int32_t qe, int32_t il, int32_t ql);

    uint32_t get_code_rate() { return (uint32_t)(code_rate); }
    uint32_t get_carrier_rate() { return (uint32_t)(carrier_rate); }
    double get_tx_time();
    void get_satellite_ecef(double t, double *x, double *y, double *z);
    double get_clock_correction(double t);
    bool ready_to_solve();
    double get_cn0() { return cn0; }
    int get_sv() { return sv; }

private:
    int sv;
    double fs;
    double fc;
    double doppler;

    double code_rate;
    double carrier_rate;

    // Code generator outputs
    uint8_t code_early;
    uint8_t code_prompt;
    uint8_t code_late;

    // Accumulators (early, prompt, late for I and Q)
    long long ie_longint;
    long long qe_longint;
    long long ip_longint;
    long long qp_longint;
    long long il_longint;
    long long ql_longint;
    bool longint_en;

    // DLL filter
    PLL *dll;

    // PLL filter
    PLL *pll;
    double carrier_aiding_smoothed;
    double carrier_aiding_alpha;
    bool fll_pull_in;
    long long fll_pullin_deadline_ms;
    bool fll_assist_active;

    // Time
    long long ms_elapsed;

    // Prompt buffers
    double ip_buffer[100];
    double qp_buffer[100];
    int prompt_len;
    int prompt_idx;

    // Bit sync
    int last_ip;
    int bit_sync_count;
    int bit_hist[20];
    bool bit_synced;
    int bit_ms;
    int bit_sum;

    // Bit buffer
    uint8_t nav_buf[300];
    int nav_count;

    // Nav
    bool nav_valid;
    int last_z_count;

    // Ephemeris
    EphemerisL1CA ephm;

    // SNR
    double cn0;
    double cn0_smoothed; // slow EMA of cn0, used for bandwidth/FLL-gate decisions

    std::ofstream debug_file;

    // Private functions
    void update_nav();
};

#endif // TRACK_L1CA_H
