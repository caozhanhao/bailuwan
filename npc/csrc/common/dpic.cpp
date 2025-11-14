#include <iostream>
#include <cstdint>
#include <cstdio>

#include "config.hpp"
#include "dut_proxy.hpp"

extern "C" {
void flash_read(int32_t addr, int32_t* data)
{
    // Warning for FLASH:
    //   Note that the address we got exactly is 24-bit, since the read request
    //   we sent in APBSPI only takes the low 24-bit of the address.
    //   Therefore, the high 12-bit is always 0 and we add it back here.
    //   By the way, difftest in NEMU does not be affected by this, since the read request is emulated
    //   by NEMU itself, not a APBSPI in ysyxSoC.

    *data = sim_handle.get_memory().read(addr + CONFIG_FLASH_BASE);
}

static uint32_t mrom_data[1024];
void mrom_read(int32_t addr, int32_t* data)
{
    // printf("read addr = 0x%08x, data = 0x%08x\n", addr, sim_handle.get_memory().read(addr));
    // *data = sim_handle.get_memory().read(addr);
    assert(0);
}

void ebreak_handler()
{
    auto cycles = sim_handle.get_cycles();
    auto elapsed_time = sim_handle.elapsed_time();

    printf("ebreak after %lu cycles\n", cycles);
    printf("elapsed time: %lu us\n", elapsed_time);

    double cycle_per_us = static_cast<double>(elapsed_time) / static_cast<double>(cycles);
    printf("cycle per us: %f\n", cycle_per_us);

    auto a0 = sim_handle.get_cpu().reg(10);
    if (a0 == 0)
        printf("\33[1;32mHIT GOOD TRAP\33[0m\n");
    else
        printf("\33[1;41mHIT BAD TRAP\33[0m, a0=%d\n", a0);

    throw EBreakException(static_cast<int>(a0));
}

int pmem_read(int raddr)
{
    assert(0);
    // auto ret = sim_handle.get_memory().read(raddr);
    // IFDEF(CONFIG_MTRACE, printf("read addr = 0x%08x, data = 0x%08x\n", raddr, ret));
    // return ret;
}

void pmem_write(int waddr, int wdata, char wmask)
{
    assert(0);
    // IFDEF(CONFIG_MTRACE, printf("write addr = 0x%08x, data = 0x%08x, mask = 0x%x\n", waddr, wdata, wmask));
    // sim_handle.get_memory().write(waddr, wdata, wmask);
}
}
