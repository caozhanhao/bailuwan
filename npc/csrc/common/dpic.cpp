#include <cstdint>
#include <cstdio>
#include "trace.h"

extern "C" {
// 00000000 <_start>:
//      0:	01400513          	addi	a0,zero,20
//      4:	010000e7          	jalr	ra,16(zero) # 10 <fun>
//      8:	00c000e7          	jalr	ra,12(zero) # c <halt>
//
//   0000000c <halt>:
//      c:  00100073            ebreak
//
//   00000010 <fun>:
//     10:	00a50513          	addi	a0,a0,10
//     14:	00008067          	jalr	zero,0(ra)
uint32_t memory[65536] = {
    0x01400513, // addi a0, zero, 20
    0x010000e7, // jalr ra, 16(zero)
    0x00c000e7, // jalr ra, 12(zero)
    0x00100073, // ebreak
    0x00a50513, // addi a0, a0, 10
    0x00008067, // jalr zero,0(ra)
};


void ebreak_handler()
{
    printf("ebreak\n");
    trace_cleanup();
    exit(0);
}

extern "C" int pmem_read(int raddr)
{
    return memory[((unsigned)raddr & ~0x3u) / 4];
}

extern "C" void pmem_write(int waddr, int wdata, char wmask)
{
    unsigned idx = ((unsigned)waddr & ~0x3u) / 4u;

    uint32_t cur = memory[idx];
    uint32_t newv = cur;
    uint32_t wd = (uint32_t)wdata;
    uint8_t mask = (uint8_t)wmask;

    for (int i = 0; i < 4; ++i)
    {
        if (mask & (1u << i))
        {
            uint32_t byte_mask = 0xFFu << (i * 8);
            uint32_t src_byte = (wd >> (i * 8)) & 0xFFu;
            newv = (newv & ~byte_mask) | (src_byte << (i * 8));
        }
    }

    memory[idx] = newv;
}
}
