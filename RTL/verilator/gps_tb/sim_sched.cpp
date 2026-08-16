#include "sim_sched.h"

#include <atomic>
#include <semaphore>
#include <thread>

namespace
{
    // Thrown into the test thread by sim_shutdown() to unwind it cleanly.
    struct Abort
    {
    };

    std::binary_semaphore to_test{0}; // released to let the test thread run
    std::binary_semaphore to_sim{0};  // released to let the sim thread run
    std::thread test_thread;

    // Written by the test thread while the sim thread is parked, read by the sim
    // thread while the test thread is parked. Null means "runnable now".
    std::function<bool()> wake_pred;

    std::atomic<bool> finished = false;
    bool stopping = false;

    VerilatedContext *ctxp = nullptr;
}

void sim_start(VerilatedContext *const contextp, std::function<void()> body)
{
    ctxp = contextp;

    test_thread = std::thread([body = std::move(body)]
                              {
        // Verilator keeps a thread-local context pointer that $finish, VL_PRINTF
        // and DPI go through, so the test thread needs its own copy.
        Verilated::threadContextp(ctxp);

        to_test.acquire(); // wait for the first handoff from the sim thread

        if (!stopping)
        {
            try
            {
                body();
            }
            catch (const Abort &)
            {
                // sim_shutdown() asked us to stop mid-wait
            }
        }

        finished = true;
        to_sim.release(); });
}

void sim_service()
{
    if (finished)
    {
        return;
    }

    if (wake_pred && !wake_pred())
    {
        return; // still blocked - cheap path, taken on nearly every tick
    }

    wake_pred = nullptr;
    to_test.release();
    to_sim.acquire(); // sim thread parks while the test program runs
}

bool sim_done()
{
    return finished;
}

void sim_shutdown()
{
    if (!test_thread.joinable())
    {
        return;
    }

    if (!finished)
    {
        // Test program is parked in sim_wait(); wake it so it can unwind.
        stopping = true;
        to_test.release();
        to_sim.acquire();
    }

    test_thread.join();
}

void sim_wait(std::function<bool()> wake)
{
    wake_pred = std::move(wake);
    to_sim.release();
    to_test.acquire(); // test thread parks while the simulation advances

    if (stopping)
    {
        throw Abort{};
    }
}

void sim_wait_until_time(uint64_t time)
{
    sim_wait([time]
             { return ctxp->time() >= time; });
}

void sim_wait_ticks(uint64_t ticks)
{
    sim_wait_until_time(ctxp->time() + ticks);
}

uint64_t sim_time()
{
    return ctxp->time();
}
