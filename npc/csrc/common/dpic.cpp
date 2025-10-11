#include <cstdint>
#include <cstdio>

#include "common.h"
#include "trace.h"
#include "dpic.h"

extern "C" {
uint32_t* memory;

void dump_registers()
{
#define DUMP(reg) printf("x%d = 0x%08x\n", reg, dut.io_registers_##reg);
    DUMP(0)
    DUMP(1)
    DUMP(2)
    DUMP(3)
    DUMP(4)
    DUMP(5)
    DUMP(6)
    DUMP(7)
    DUMP(8)
    DUMP(9)
    DUMP(10)
    DUMP(11)
    DUMP(12)
    DUMP(13)
    DUMP(14)
    DUMP(15)
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
    uint32_t idx = (static_cast<uint32_t>(raddr) & ~0x3u) / 4u;
    assert(idx < MEMORY_SIZE);
    return memory[idx];
}

void pmem_write(int waddr, int wdata, char wmask)
{
    uint32_t idx = (static_cast<uint32_t>(waddr) & ~0x3u) / 4;
    assert(idx < MEMORY_SIZE);

    uint32_t cur = memory[idx];
    uint32_t newv = cur;
    uint32_t wd = static_cast<uint32_t>(wdata);
    uint8_t mask = static_cast<uint8_t>(wmask);

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
