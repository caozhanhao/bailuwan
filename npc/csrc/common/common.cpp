#include "common.h"

TOP_NAME dut;

void reset(int n)
{
    dut.reset = 1;
    while (n-- > 0) single_cycle();
    dut.reset = 0;
}
