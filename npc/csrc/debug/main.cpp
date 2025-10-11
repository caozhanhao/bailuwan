#include <VTop.h>

#include "trace.h"
#include "macro.h"
#include "common.h"

static uint64_t sim_time = 0;

static void single_cycle()
{
    IFDEF(TRACE, tfp->dump(sim_time++));

    dut.clock = 0;
    dut.eval();

    IFDEF(TRACE, tfp->dump(sim_time++));

    dut.clock = 1;
    dut.eval();
}

void reset(int n)
{
    dut.reset = 1;
    while (n-- > 0)
        single_cycle();
    dut.reset = 0;
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s [number of cycles]\n", argv[0]);
        return -1;
    }

    int cycles = atoi(argv[1]);

    trace_init();

    reset(10);

    while (cycles-- > 0)
    {
        single_cycle();
    }

    trace_cleanup();
    return 0;
}
