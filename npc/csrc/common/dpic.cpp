#include <cstdint>
#include <cstdio>
#include "trace.h"

extern "C" {
uint32_t memory[65536];

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
