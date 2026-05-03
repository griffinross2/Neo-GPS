// Include common routines
#include <verilated.h>

// Include model header, generated from Verilating "top.v"
#include "Vuart_rx.h"

unsigned int main_time = 0;

double sc_time_stamp()
{
    return (double)main_time;
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

    // Construct the Verilated model, from Vtop.h generated from Verilating "top.v"
    Vuart_rx *const top = new Vuart_rx{contextp};

    // Simulate until $finish
    while (!contextp->gotFinish())
    {
        main_time++;

        // Evaluate model
        top->eval();
    }

    // Final model cleanup
    top->final();

    // Destroy model
    delete top;

    // Return good completion status
    return 0;
}
