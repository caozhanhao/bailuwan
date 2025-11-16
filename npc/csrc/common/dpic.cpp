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

    *data = SIM.mem().read<uint32_t>(addr + CONFIG_FLASH_BASE);
}

void mrom_read(int32_t addr, int32_t* data)
{
    assert(0);
    // auto& mem = SIM.mem();
    // assert(mem.in_mrom(addr));
    // *data = SIM.mem().read(addr);
}

char psram_read(int raddr)
{
    return SIM.mem().read<char>(raddr + CONFIG_PSRAM_BASE /* same as flash*/);
}

void psram_write(int waddr, char wdata)
{
    return SIM.mem().write<char>(waddr + CONFIG_PSRAM_BASE /* same as flash*/, wdata, 0b1);
}

int16_t sdram_read(int raddr)
{
    auto data = SIM.mem().read<int16_t>(raddr + CONFIG_SDRAM_BASE /* same as flash*/);
    IFDEF(CONFIG_MTRACE, printf("SDRAM Read | addr=0x%x, data=0x%x\n",
              raddr, data));
    return data;
}

void sdram_write(int waddr, int16_t wdata, char mask)
{
    IFDEF(CONFIG_MTRACE, printf("SDRAM Write | addr=0x%x, data=0x%x, mask=0x%x\n",
              waddr, wdata, mask));
    SIM.mem().write<int16_t>(waddr + CONFIG_SDRAM_BASE /* same as flash*/, wdata, mask);
}

void ebreak_handler()
{
    auto cycles = SIM.cycles();
    auto elapsed_time = SIM.elapsed_time();

    printf("ebreak after %lu cycles\n", cycles);
    printf("elapsed time: %lu us\n", elapsed_time);

    double cycle_per_us = static_cast<double>(elapsed_time) / static_cast<double>(cycles);
    printf("cycle per us: %f\n", cycle_per_us);

    auto a0 = SIM.cpu().reg(10);
    if (a0 == 0)
        printf("\33[1;32mHIT GOOD TRAP\33[0m\n");
    else
        printf("\33[1;41mHIT BAD TRAP\33[0m, a0=%d\n", a0);

    throw EBreakException(static_cast<int>(a0));
}
}
