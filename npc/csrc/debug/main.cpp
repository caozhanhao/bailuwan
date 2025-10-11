#include <VTop.h>

#include "trace.hpp"
#include "macro.hpp"
#include "disasm.hpp"
#include "dut_proxy.hpp"
#include "dpic.hpp"
#include "minirvemu.hpp"

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

    reset(10);

    // Simulate
    printf("Simulation started...\n");
    MiniRVEmu emu;
    emu.load(dut_memory, DUT_MEMORY_SIZE / sizeof(uint32_t));
    int inconsistent_mem_cnt = 0;
    while (cycles-- > 0)
    {
        auto disasm = disassemble(cpu.pc(), cpu.curr_inst());
        printf("0x%08x: %s\n", cpu.pc(), disasm.c_str());

        // Diff test
        for (int i = 0; i < 16; i++)
        {
            if (emu.reg(i) != cpu.reg(i))
            {
                std::cerr << "Register mismatch at x" + std::to_string(i) +
                    ", expected " + std::to_string(emu.reg(i)) + ", got " + std::to_string(cpu.reg(i))
                    << std::endl;
                eassert(false, "Register mismatch");
            }
        }

        if (memcmp(dut_memory, emu.memory.data(), DUT_MEMORY_SIZE) != 0)
            inconsistent_mem_cnt++;
        else
            inconsistent_mem_cnt = 0;

        eassert(inconsistent_mem_cnt < 3, "Memory mismatch");

        emu.step();
        single_cycle();
    }

    // clean up
    trace_cleanup();
    return 0;
}
