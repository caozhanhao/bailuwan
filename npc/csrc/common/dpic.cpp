#include <cstdint>
#include <cstdio>

#include "common.h"
#include "trace.h"
#include "dpic.h"

extern "C" {
uint32_t* memory;

void dump_registers()
{
}

void ebreak_handler()
{
    printf("ebreak\n");
    trace_cleanup();
    dump_registers();
    exit(dut.io_registers_0);
}

int pmem_read(int raddr)
{
    return memory[((unsigned)raddr & ~0x3u) / 4];
}

void pmem_write(int waddr, int wdata, char wmask)
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

void init_memory(const char* filename)
{
    printf("Initializing memory from %s\n", filename);
    FILE* fp = fopen(filename, "rb");
    assert(fp);

    memory = static_cast<uint32_t*>(malloc(MEMORY_SIZE));
    memset(memory, 0, MEMORY_SIZE);

    size_t bytes_read = fread(memory, 1, MEMORY_SIZE, fp);
    if (bytes_read == 0)
    {
        if (ferror(stdin))
        {
            perror("fread");
            free(memory);
            exit(-1);
        }
    }

    printf("Read %zu bytes from %s\n", bytes_read, filename);
    fclose(fp);
}
