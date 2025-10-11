#ifndef DPIC_H
#define DPIC_H

#include <cstdint>

extern "C" {
    extern uint32_t* memory;
}

void init_memory(const char *filename);
#endif