#include "test_program.h"
#include "sim_sched.h"
#include "parallel_interface_host.h"
#include "track_l1ca.h"

#include <cmath>
#include <iostream>

#include "verilated.h"
#include "Vgps_tb_gps_tb.h"

GPSL1CATracker *ch0_track;

static int32_t get_ip(Vgps_tb *const top)
{
    uint32_t ip_raw = pi_read(top, 0x0027);
    int32_t ip = std::bit_cast<int32_t>(ip_raw);
    return ip;
}

static int32_t get_qp(Vgps_tb *const top)
{
    uint32_t qp_raw = pi_read(top, 0x0028);
    int32_t qp = std::bit_cast<int32_t>(qp_raw);
    return qp;
}

static int32_t get_ie(Vgps_tb *const top)
{
    uint32_t ie_raw = pi_read(top, 0x0029);
    int32_t ie = std::bit_cast<int32_t>(ie_raw);
    return ie;
}

static int32_t get_qe(Vgps_tb *const top)
{
    uint32_t qe_raw = pi_read(top, 0x002A);
    int32_t qe = std::bit_cast<int32_t>(qe_raw);
    return qe;
}

static int32_t get_il(Vgps_tb *const top)
{
    uint32_t il_raw = pi_read(top, 0x002B);
    int32_t il = std::bit_cast<int32_t>(il_raw);
    return il;
}

static int32_t get_ql(Vgps_tb *const top)
{
    uint32_t ql_raw = pi_read(top, 0x002C);
    int32_t ql = std::bit_cast<int32_t>(ql_raw);
    return ql;
}

// Runs on the test thread: every pi_* call below blocks until the transaction
// has completed on the wire, while the simulation keeps advancing.
static void test_program_body(Vgps_tb *const top)
{
    sim_wait_until_time(1e6 * 1);

    pi_write(top, 0x0020, 0x40000006); // Set clear for channel 0
    pi_write(top, 0x0020, 0x00000006); // Clear clear for channel 0 and set sv=6
    pi_write(top, 0x0010, 0x40000006); // Write start bit, sv=6, ch=0
    uint64_t start_time = sim_time();
    std::cout << "Starting search at time " << start_time << std::endl;

    // Poll the status register until the search completes
    while (pi_read(top, 0x0010) & 0x80000000)
    {
    }

    std::cout << "Search done at time " << sim_time() << std::endl;

    uint32_t accumulator = pi_read(top, 0x0011);

    uint16_t code_start_doppler = pi_read(top, 0x0012);
    double code = (double)(code_start_doppler & 0x3FF);
    code -= (double)((code_start_doppler >> 10) & 0x1F) * (1.023e6 / 19.2e6); // Start position in signal sample
    uint16_t dop_data = (code_start_doppler >> 15) & 0x3F;
    uint16_t doppler_sign = ((dop_data >> 5) & 0x1);
    int8_t doppler_int = (int8_t)(dop_data + (doppler_sign << 6) + (doppler_sign << 7)); // Sign extend to 8 bits then convert to int
    double doppler = (double)doppler_int * 250.0;                                        // Doppler in Hz

    std::cout << "Search results at time " << sim_time() << std::endl;
    std::cout << "Accumulator: " << accumulator << std::endl;
    std::cout << "Code: " << code << std::endl;
    std::cout << "Doppler: " << doppler << std::endl;

    uint64_t end_time = sim_time();

    // Represents the amount of chips by which the signal is advanced relative to the code
    double signal_advance = code + doppler * ((end_time - start_time) / 1.0e9) * (1.0 / 1540.0);

    // Represents the amount of chips to delay the code to resolve the signal advance
    double delay_chips = 1023.0 - signal_advance - 0.5;
    delay_chips = fmod(fmod(delay_chips, 1023.0) + 1023.0, 1023.0);

    ch0_track = new GPSL1CATracker(7, 19.2e6, 4.02e6, doppler);

    uint32_t delay_cycles = (uint32_t)llround(delay_chips * 19.2e6 / 1.023e6);
    std::cout << "Signal advance: " << signal_advance << " chips, delay: " << delay_chips
              << " chips (" << delay_cycles << " cycles)" << std::endl;

    // Initial code and carrier rates based on the search results
    uint32_t code_rate0 = (uint32_t)llround((1.023e6 + doppler * 1.023e6 / 1.57542e9) / 19.2e6 * 4294967296.0);
    uint32_t carrier_rate0 = (uint32_t)llround((4.02e6 + doppler) / 19.2e6 * 4294967296.0);
    pi_write(top, 0x0022, code_rate0);
    pi_write(top, 0x0023, carrier_rate0);

    pi_write(top, 0x0021, delay_cycles); // Write delay
    pi_read(top, 0x0020);                // Read to clear epoch signal
    while ((pi_read(top, 0x0020) & 0x80000000) == 0)
    {
    } // Wait and ditch first epoch

    for (int i = 0; i < 30000; i++)
    {
        while ((pi_read(top, 0x0020) & 0x80000000) == 0)
        {
        } // Wait for epoch

        // Read the I and Q values
        int32_t ip = get_ip(top);
        int32_t qp = get_qp(top);
        int32_t ie = get_ie(top);
        int32_t qe = get_qe(top);
        int32_t il = get_il(top);
        int32_t ql = get_ql(top);

        // std::cout << "Epoch " << i << " at time " << sim_time() << std::endl;
        // std::cout << "I: " << ip << ", Q: " << qp << ", I_E: " << ie << ", Q_E: " << qe << ", I_L: " << il << ", Q_L: " << ql << std::endl;

        ch0_track->update_epoch(ip, qp, ie, qe, il, ql);
        uint32_t code_rate = ch0_track->get_code_rate();
        uint32_t carrier_rate = ch0_track->get_carrier_rate();
        pi_write(top, 0x0022, code_rate);    // Write code rate
        pi_write(top, 0x0023, carrier_rate); // Write carrier rate

        // std::cout << "C/N0: " << ch0_track->get_cn0() << " dB-Hz" << std::endl;
    }
}

void test_program_init(VerilatedContext *const contextp, Vgps_tb *const top)
{
    pi_init(top);
    sim_start(contextp, [top]
              { test_program_body(top); });
}
