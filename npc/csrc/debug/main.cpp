#include <VTop.h>

#include "trace.hpp"
#include "macro.hpp"
#include "disasm.hpp"
#include "dut_proxy.hpp"
#include "dpic.hpp"

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
    // INIT
    if (argc != 3)
    {
        printf("Usage: %s [number of cycles] [filename]\n", argv[0]);
        return -1;
    }

    int cycles = atoi(argv[1]);

    init_memory(argv[2]);
    init_disasm();
    trace_init();
    cpu.bind(&dut);
    cycle_counter = 0;

    reset(10);

    // Simulate
    printf("Simulation started...\n");
    while (cycles-- > 0)
    {
        auto disasm = disassemble(cpu.pc(), cpu.curr_inst());
        printf("0x%08x: %s\n", cpu.pc(), disasm.c_str());

        single_cycle();
        cycle_counter++;
    }

    printf("Simulation terminated after %lu cycles\n", cycle_counter);

    // clean up
    trace_cleanup();
    return 0;
}
