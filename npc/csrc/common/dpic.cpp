#include <iostream>
#include <cstdint>
#include <cstdio>

#include "config.hpp"
#include "dut_proxy.hpp"

extern "C" {
void flash_read(int32_t addr, int32_t* data) { assert(0); }

void mrom_read(int32_t addr, int32_t* data)
{
    unsigned char chartest_bin[] = {
        0x41, 0x11, 0x06, 0xe4, 0x22, 0xe0, 0x00, 0x08, 0xb7, 0x07, 0x00, 0x10,
        0x13, 0x07, 0x10, 0x04, 0x23, 0x80, 0xe7, 0x00, 0xb7, 0x07, 0x00, 0x10,
        0x29, 0x47, 0x23, 0x80, 0xe7, 0x00, 0x01, 0xa0
      };
    unsigned int chartest_bin_len = 32;

    assert(addr >= 0x20000000 && (addr - 0x20000000) < chartest_bin_len);
    *data = reinterpret_cast<uint32_t*>(chartest_bin)[(addr - 0x20000000) / 4];
}

void ebreak_handler()
{
    auto cycles = sim_handle.get_cycles();
    auto elapsed_time = sim_handle.elapsed_time();

    printf("ebreak after %lu cycles\n", cycles);
    printf("elapsed time: %lu us\n", elapsed_time);

    double cycle_per_us = static_cast<double>(elapsed_time) / static_cast<double>(cycles);
    printf("cycle per us: %f\n", cycle_per_us);

    auto a0 = 0;
    // auto a0 = sim_handle.get_cpu().reg(10);
    // if (a0 == 0)
    //     printf("\33[1;32mHIT GOOD TRAP\33[0m\n");
    // else
    //     printf("\33[1;41mHIT BAD TRAP\33[0m, a0=%d\n", a0);

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
