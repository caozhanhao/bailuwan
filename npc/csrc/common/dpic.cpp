#include <iostream>
#include <cstdint>
#include <cstdio>

#include "config.hpp"
#include "dut_proxy.hpp"

extern "C" {
void flash_read(int32_t addr, int32_t* data)
{
    static uint32_t flashd[1024];
    static bool inited = false;
    if (!inited) {
        inited = true;
        FILE* file = fopen("/home/caozhanhao/ysyx/workspace/tmp/chartest.bin", "rb");
        fread(flashd, sizeof(flashd), 1, file);
    }
    printf("read addr = 0x%08x",  addr);
    *data = flashd[(addr - 0x30000000) / 4];
}

static uint32_t mrom_data[1024];
void mrom_read(int32_t addr, int32_t* data)
{
    // printf("read addr = 0x%08x, data = 0x%08x\n", addr, sim_handle.get_memory().read(addr));
    *data = sim_handle.get_memory().read(addr);
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
