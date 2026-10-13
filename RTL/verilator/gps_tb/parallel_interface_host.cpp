#include "verilated.h"
#include "Vgps_tb_gps_tb.h"

#include "parallel_interface_host.h"
#include "sim_sched.h"

// Ticks per if_clk half period. Each PIO instruction is one half period, so this
// is the PIO clock divider expressed in sim ticks (1 ns): 50 -> 10 MHz if_clk.
//
// The device's read path needs ~5 core_clk cycles between the address being
// latched (rising edge 1) and the first read word being launched (falling edge
// 2), which is 3 half periods away; keep 3 * PI_HALF_PERIOD above that.
constexpr int PI_HALF_PERIOD = 50;

// Half periods CS is held high after a transaction before the next can start.
// Matches the pull / mov / pull at the top of the PIO program.
constexpr int PI_CS_IDLE_HALF_PERIODS = 3;

// One phase per PIO instruction with CS low, plus the trailing CS high time.
// The pins for a phase are applied when the phase is entered.
typedef enum
{
    PI_IDLE,
    PI_ADDR,          // clk low, drive {rw, addr}
    PI_ADDR_LATCH,    // clk high, device latches the command
    PI_TURNAROUND,    // clk low, host releases the bus if reading
    PI_DUMMY,         // clk high
    PI_DATA_HI,       // clk low, drive data[31:16]
    PI_DATA_HI_LATCH, // clk high, sample data[31:16]
    PI_DATA_LO,       // clk low, drive data[15:0]
    PI_DATA_LO_LATCH, // clk high, sample data[15:0]
    PI_CS_HIGH,       // CS high, clk low, bus released
} pi_state_t;

static pi_state_t pi_state = PI_IDLE;
static int pi_tick_counter = 0;
static int pi_idle_counter = 0;
static bool pi_read_write = false; // false for write, true for read
static uint16_t pi_addr = 0;
static uint32_t pi_wdata = 0;
static uint32_t pi_rdata = 0;

void pi_init(Vgps_tb *const top)
{
    top->gps_tb->if_clk = 0;
    top->gps_tb->if_cs = 1;
    top->gps_tb->if_host_oe = 0;
    top->gps_tb->if_host_dat = 0;
}

// Drive the pins for the phase being entered.
static void pi_enter(Vgps_tb *const top, pi_state_t state)
{
    pi_state = state;

    switch (state)
    {
    case PI_IDLE:
        break;
    case PI_ADDR:
        top->gps_tb->if_cs = 0;
        top->gps_tb->if_clk = 0;
        top->gps_tb->if_host_oe = 1;
        top->gps_tb->if_host_dat = (pi_read_write ? 0x8000 : 0x0000) | (pi_addr & 0x7FFF);
        break;
    case PI_TURNAROUND:
        top->gps_tb->if_clk = 0;
        top->gps_tb->if_host_oe = !pi_read_write;
        break;
    case PI_DATA_HI:
        top->gps_tb->if_clk = 0;
        top->gps_tb->if_host_dat = (pi_wdata >> 16) & 0xFFFF;
        break;
    case PI_DATA_LO:
        top->gps_tb->if_clk = 0;
        top->gps_tb->if_host_dat = pi_wdata & 0xFFFF;
        break;
    case PI_ADDR_LATCH:
    case PI_DUMMY:
        top->gps_tb->if_clk = 1;
        break;
    case PI_DATA_HI_LATCH:
        // Sample before raising the clock: the bus still holds what was set up
        // on the previous falling edge, as it would at the rising edge.
        pi_rdata = (uint32_t)(top->gps_tb->if_dat_in & 0xFFFF) << 16;
        top->gps_tb->if_clk = 1;
        break;
    case PI_DATA_LO_LATCH:
        pi_rdata |= top->gps_tb->if_dat_in & 0xFFFF;
        top->gps_tb->if_clk = 1;
        break;
    case PI_CS_HIGH:
        top->gps_tb->if_cs = 1;
        top->gps_tb->if_clk = 0;
        top->gps_tb->if_host_oe = 0;
        pi_idle_counter = 0;
        break;
    }
}

// Kick off a transaction. pi_task() carries it out over the following ticks.
static void pi_start(Vgps_tb *const top, bool read_write, uint16_t addr, uint32_t data)
{
    pi_read_write = read_write;
    pi_addr = addr;
    pi_wdata = read_write ? 0 : data;
    pi_rdata = 0;
    pi_tick_counter = 0;
    pi_enter(top, PI_ADDR);
}

void pi_task(Vgps_tb *const top)
{
    if (pi_state == PI_IDLE)
    {
        return;
    }

    if (pi_tick_counter < PI_HALF_PERIOD - 1)
    {
        pi_tick_counter++;
        return;
    }
    pi_tick_counter = 0;

    if (pi_state == PI_CS_HIGH)
    {
        if (++pi_idle_counter >= PI_CS_IDLE_HALF_PERIODS)
        {
            pi_enter(top, PI_IDLE);
        }
        return;
    }

    pi_enter(top, (pi_state_t)(pi_state + 1));
}

static bool pi_is_idle()
{
    return pi_state == PI_IDLE;
}

void pi_write(Vgps_tb *const top, uint16_t addr, uint32_t data)
{
    pi_start(top, false, addr, data);
    sim_wait([]
             { return pi_is_idle(); });
}

uint32_t pi_read(Vgps_tb *const top, uint16_t addr)
{
    pi_start(top, true, addr, 0);
    sim_wait([]
             { return pi_is_idle(); });
    return pi_rdata;
}
