#include <VTop.h>

#include "dut_proxy.hpp"
#include "utils/disasm.hpp"

int main(int argc, char* argv[])
{
    // INIT
    if (argc != 3)
    {
        printf("Usage: %s [number of cycles] [filename]\n", argv[0]);
        return -1;
    }

    int cycles = atoi(argv[1]);
    bool no_cycle_limit = (cycles <= 0);

    sim_handle.init_sim(argv[2]);
    sim_handle.reset(10);

    Disassembler disasm{};
    disasm.init();

    // Simulate
    printf("Simulation started...\n");
    while (no_cycle_limit || cycles-- > 0)
    {
        sim_handle.single_cycle();

        printf("%s\n", disasm.disassemble(sim_handle.get_cpu().pc(), sim_handle.get_cpu().curr_inst()).c_str());
    }

    printf("Simulation terminated after %lu cycles\n", sim_handle.get_cycles());

    sim_handle.cleanup();
    return 0;
}
