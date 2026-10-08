#include "pico/stdlib.h"

void force_runtime(double &value)
{
    asm volatile("" : "+g"(value));
}

int main()
{
    double a = 4.7;
    double b = 2.3;

    force_runtime(a);
    force_runtime(b);

    double c = a * b;

    return int(c);
}