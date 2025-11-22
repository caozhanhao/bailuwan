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
    fprintf(stderr, "Unexpected MROM Read at addr=0x%x\n", addr);
    SIM.cpu().dump();
    assert(0);
    // *data = SIM.mem().read<int32_t>(addr);
    // printf("MROM Read | addr=0x%x, data=0x%x\n", addr, *data);
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
// To make the x command in sdb work, we convert the address to the original address issued in NPC.
//
// First, in the SDRAM controller, addr[1:0] is ignored and addr[12] is used as a chip select.
// The address layout (MSB -> LSB) is:
//
//    MSB (SDRAM_ADDR_W)                                          LSB (0)
//    +---------------------+-------------+---------------+-----------+
//    |     ROW Address     | BANK Select |  COL Address  |  Ignored  |
//    +---------------------+-------------+---------------+-----------+
//    |   [ADDR_W : C+B+1]  | [C+B : C+1] |    [C : 2]    |  [1 : 0]  |
//    |   [     25:    13]  | [12  :  10] |    [9 : 2]    |  [1 : 0]  |
//    +----------+----------+------+------+-------+-------+-----+-----+
//
// Additionally, the SDRAM controller pads one 0 to the column address, and the SDRAM chip pads
// another 0 before calling the DPI-C function. Therefore, the address seen here is actually:
//
//                    addr[25:13] ## addr[11:2] ## 0.U(2.W)
//
// Besides, ID is a 2-bit wire assigned as [wordext id, bitext id], meaning:
//           Word Extend     Bit Extend
//   - 00: addr[12] = low,  low  16 bits
//   - 01: addr[12] = low,  high 16 bits
//   - 10: addr[12] = high, low  16 bits
//   - 11: addr[12] = high, high 16 bits
//
// Thus the original NPC address can be reconstructed as:
//
//         addr(DPI-C)[24:12] ## id[1] ## addr(DPI-C)[11:2] ## 0.U(2.W)
//
// If id[0] is high, the access targets the high 16 bits; otherwise it targets the low 16 bits.
static uint32_t convert_sdram_addr(int addr, char id)
{
    auto npc_addr = (BITS(addr, 24, 12) << 13) | (BITS(id, 1, 1) << 12) | (BITS(addr, 11, 2) << 2);

    // little-endian
    npc_addr += BITS(id, 0, 0) * 2;

    // same as flash
    npc_addr += CONFIG_SDRAM_BASE;
    return npc_addr;
}

int16_t sdram_read(int raddr, char id)
{
    auto npc_addr = convert_sdram_addr(raddr, id);
    auto data = SIM.mem().read<int16_t>(npc_addr);
    IFDEF(CONFIG_MTRACE, printf("SDRAM Read | id=%d, addr=0x%x, data=0x%x\n",
              id, raddr, data));
    return data;
}

void sdram_write(int waddr, int16_t wdata, char mask, char id)
{
    IFDEF(CONFIG_MTRACE, printf("SDRAM Write | id=%d, addr=0x%x, data=0x%x, mask=0x%x\n",
              id, waddr, wdata, mask));
    auto npc_addr = convert_sdram_addr(waddr, id);
    SIM.mem().write<int16_t>(npc_addr, wdata, mask);
}

void ebreak_handler()
{
    printf("Ebreak after %lu cycles\n", SIM.cycles());
    printf("Statistics:\n");
    SIM.dump_statistics(stdout);

    auto a0 = SIM.cpu().reg(10);
    if (a0 == 0)
        printf("\33[1;32mHIT GOOD TRAP\33[0m\n");
    else
        printf("\33[1;41mHIT BAD TRAP\33[0m, a0=%d\n", a0);

    throw EBreakException(static_cast<int>(a0));
}
}
