// Include common routines
#include <verilated.h>
#include <verilated_fst_c.h>
#include <iostream>

// Include model header, generated from Verilating "top.v"
#include "Vl1ca_ac_pca_search_tb.h"

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

    VerilatedFstC *tfp = new VerilatedFstC;
    tfp->set_time_unit("ns");
    tfp->set_time_resolution("ns");

    // Construct the Verilated model, from Vtop.h generated from Verilating "top.v"
    Vl1ca_ac_pca_search_tb *const top = new Vl1ca_ac_pca_search_tb{contextp};

    top->trace(tfp, 99);
    tfp->open("outputs/l1ca_ac_pca_search_tb/waveform.fst");

    // Simulate until $finish
    while (!contextp->gotFinish())
    {
        contextp->timeInc(1);

        // Evaluate model
        top->eval();

        tfp->dump(contextp->time());

        if (contextp->time() % 10000000 == 0)
        {
            std::cout << "Simulation time: " << (contextp->time() / 1000000) << " ms" << std::endl;
        }
    }

    // Final model cleanup
    top->final();
    tfp->close();

    // Destroy model
    delete top;
    delete tfp;

    // Final simulation summary
    contextp->statsPrintSummary();

    // Return good completion status
    return 0;
}
