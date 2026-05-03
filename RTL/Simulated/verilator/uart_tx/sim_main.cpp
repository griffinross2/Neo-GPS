// Include common routines
#include <verilated.h>
#include <verilated_vcd_c.h>

// Include model header, generated from Verilating "top.v"
#include "Vuart_tx.h"

double sc_time_stamp()
{
    return 0;
}

int main(int argc, char **argv)
{
    // See a similar example walkthrough in the verilator manpage.

    // This is intended to be a minimal example.  Before copying this to start a
    // real project, it is better to start with a more complete example,
    // e.g. examples/c_tracing.

    // Construct a VerilatedContext to hold simulation time, etc.
    VerilatedContext *const contextp = new VerilatedContext;

    // Pass arguments so Verilated code can see them, e.g. $value$plusargs
    // This needs to be called before you create any model
    contextp->commandArgs(argc, argv);

    contextp->traceEverOn(true);
    contextp->threads(1);

    VerilatedVcdC *tfp = new VerilatedVcdC;

    // Construct the Verilated model, from Vtop.h generated from Verilating "top.v"
    Vuart_tx *const top = new Vuart_tx{contextp};

    top->trace(tfp, 99);
    tfp->open("waveform.vcd");

    // Simulate until $finish
    while (!contextp->gotFinish())
    {
        contextp->timeInc(1);

        // Evaluate model
        top->eval();

        tfp->dump(contextp->time());
    }

    // Final model cleanup
    top->final();

    // Destroy model
    delete top;

    // Final simulation summary
    contextp->statsPrintSummary();

    // Return good completion status
    return 0;
}
