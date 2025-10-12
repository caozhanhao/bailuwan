#ifndef DPIC_H
#define DPIC_H

#include <cstdint>

// Byte * 1024 * 1024 * 512 Byte -> 512 MB
#define DUT_MEMORY_MAXSIZE (1024 * 1024 * 512)

extern "C" {
    extern uint32_t* dut_memory;
    extern size_t dut_memory_size;
}

void init_memory(const char *filename);
#endif