#include <iostream>
#include <cstdint>
#include <cstdio>

#include "dut_proxy.hpp"

extern "C" {
void ebreak_handler()
{
    printf("ebreak after %lu cycles\n", sim_handle.get_cycles());
    sim_handle.cleanup();

    auto a0 = sim_handle.get_cpu().reg(10);
    if (a0 == 0)
        printf("\33[1;32mHIT GOOD TRAP\33[0m\n");
    else
        printf("\33[1;41mHIT BAD TRAP\33[0m, a0=%d\n", a0);

    throw EBreakException(static_cast<int>(a0));
}

int pmem_read(int raddr)
{
    return sim_handle.get_memory().read(raddr);
}

void pmem_write(int waddr, int wdata, char wmask)
{
    sim_handle.get_memory().write(waddr, wdata, wmask);
}
}
