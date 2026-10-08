#include "l1ca_code.h"
#include "e1_code.h"

#include <print>

constexpr double FS = 19.2e6;
constexpr double IF = 4.02e6;

void l1ca_first_10_chips(int prn)
{
    L1CACode code(prn);
    std::print("First 10 chips - PRN{}: ", prn);
    for (int i = 0; i < 10; i++)
    {
        std::print("{} ", code.get_chip() > 0 ? 1 : 0);
        code.clock_chip();
    }
    std::println("");
}

void e1b_first_10_chips(int prn)
{
    E1Code code(prn);
    std::print("First 10 chips - PRN{}: ", prn);
    for (int i = 0; i < 10; i++)
    {
        std::print("{} ", code.get_chip_b() > 0 ? 1 : 0);
        code.clock_chip();
    }
    std::println("");
}

void e1c_first_10_chips(int prn)
{
    E1Code code(prn);
    std::print("First 10 chips - PRN{}: ", prn);
    for (int i = 0; i < 10; i++)
    {
        std::print("{} ", code.get_chip_c() > 0 ? 1 : 0);
        code.clock_chip();
    }
    std::println("");
}

int main()
{
    std::println("L1CA First 10 Chips:");

    for (int prn = 1; prn <= 63; prn++)
    {
        l1ca_first_10_chips(prn);
    }

    for (int prn = 120; prn <= 210; prn++)
    {
        l1ca_first_10_chips(prn);
    }

    std::println("E1B First 10 Chips:");

    for (int prn = 1; prn <= 36; prn++)
    {
        e1b_first_10_chips(prn);
    }

    std::println("E1C First 10 Chips:");

    for (int prn = 1; prn <= 36; prn++)
    {
        e1c_first_10_chips(prn);
    }

    return 0;
}