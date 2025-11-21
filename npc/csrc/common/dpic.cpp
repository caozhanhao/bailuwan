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

// The SDRAM addr only has low 25-bit valid.
// One chip -> 25-bit -> 32 MB
// id is used to tell which chip is being accessed.
//
// To let command `x` in sdb work, we convert the address to the original address issued in
// NPC by sw/lw/...
//
// First, in the SDRAM Controller, the addr[12] is used as chip select, and
//    MSB (SDRAM_ADDR_W)                                          LSB (0)
//    +---------------------+-------------+---------------+-----------+
//    |     ROW Address     | BANK Select |  COL Address  |  Ignored  |
//    +---------------------+-------------+---------------+-----------+
//    |   [ADDR_W : C+B+1]  | [C+B : C+1] |    [C : 2]    |  [1 : 0]  |
//    |   [     25:    13]  | [12  :  10] |    [9 : 2]       [1 : 0]  |
//    +----------+----------+------+------+-------+-------+-----+-----+
int16_t sdram_read(int raddr, char id)
{
    assert(id >= 0 && id < CONFIG_SDRAM_CHIP_NUM);
    auto addr = raddr + CONFIG_SDRAM_BASE /* same as flash*/;
    addr += id * CONFIG_SDRAM_CHIP_SIZE;
    auto data = SIM.mem().read<int16_t>(addr);
    IFDEF(CONFIG_MTRACE, printf("SDRAM Read | id=%d, addr=0x%x, data=0x%x\n",
              id, raddr, data));
    return data;
}

void sdram_write(int waddr, int16_t wdata, char mask, char id)
{
    assert(id >= 0 && id < CONFIG_SDRAM_CHIP_NUM);
    IFDEF(CONFIG_MTRACE, printf("SDRAM Write | id=%d, addr=0x%x, data=0x%x, mask=0x%x\n",
              id, waddr, wdata, mask));
    auto addr = waddr + CONFIG_SDRAM_BASE /* same as flash*/;
    addr += id * CONFIG_SDRAM_CHIP_SIZE;
    SIM.mem().write<int16_t>(addr, wdata, mask);
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
