#ifndef DPIC_H
#define DPIC_H

#include <cstdint>

// Byte * 1024 * 1024 * 512 Byte -> 512 MB
#define MEMORY_SIZE (1024 * 1024 * 512)

extern "C" {
    extern uint32_t* memory;
}

void init_memory(const char *filename);
#endif