#include <iostream>
#include <cstdint>
#include <cstdio>

#include "dut_proxy.hpp"
#include "trace.hpp"
#include "dpic.hpp"

extern "C" {
uint32_t* dut_memory;

void ebreak_handler()
{
    printf("ebreak\n");
    trace_cleanup();
    cpu.dump_registers(std::cerr);
    exit(cpu.reg(10));
}

int pmem_read(int raddr)
{
    uint32_t idx = (static_cast<uint32_t>(raddr) & ~0x3u) / 4u;
    if (idx >= DUT_MEMORY_SIZE)
    {
        printf("Out of bound memory access at PC = 0x%08x, raddr = 0x%08x\n", cpu.pc(), raddr);
        cpu.dump_registers(std::cerr);
        exit(-1);
    }
    return dut_memory[idx];
}

void pmem_write(int waddr, int wdata, char wmask)
{
    uint32_t idx = (static_cast<uint32_t>(waddr) & ~0x3u) / 4;

    if (idx >= DUT_MEMORY_SIZE)
    {
        printf("Out of bound memory access at PC = 0x%08x, waddr = 0x%08x\n", cpu.pc(), waddr);
        cpu.dump_registers(std::cerr);
        exit(-1);
    }

    uint32_t cur = dut_memory[idx];
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

    dut_memory[idx] = newv;
}
}

void init_memory(const char* filename)
{
    printf("Initializing memory from %s\n", filename);
    FILE* fp = fopen(filename, "rb");
    assert(fp);

    dut_memory = static_cast<uint32_t*>(malloc(DUT_MEMORY_SIZE));
    memset(dut_memory, 0, DUT_MEMORY_SIZE);

    size_t bytes_read = fread(dut_memory, 1, DUT_MEMORY_SIZE, fp);
    if (bytes_read == 0)
    {
        if (ferror(stdin))
        {
            perror("fread");
            free(dut_memory);
            exit(-1);
        }
    }

    printf("Read %zu bytes from %s\n", bytes_read, filename);

    printf("First 32 bytes:\n");
    for (int i = 0; i < 4; i++)
        printf("%08x: %08x\n", i * 4, dut_memory[i]);

    // ebreak for sum
    dut_memory[0x224 / 4] = 0b00000000000100000000000001110011;

    fclose(fp);
}
