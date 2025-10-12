#include <iostream>
#include <cstdint>
#include <cstdio>

#include "config.hpp"
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
    auto ret = sim_handle.get_memory().read(raddr);
    IFDEF(CONFIG_MTRACE, printf("read addr = 0x%08x, data = 0x%08x\n", raddr, ret));
    return ret;
}

void pmem_write(int waddr, int wdata, char wmask)
{
    IFDEF(CONFIG_MTRACE, printf("write addr = 0x%08x, data = 0x%08x, mask = 0x%x\n", waddr, wdata, wmask));
    sim_handle.get_memory().write(waddr, wdata, wmask);
}
}
