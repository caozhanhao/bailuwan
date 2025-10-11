#ifndef COMMON_H
#define COMMON_H

#include "VTop.h"

extern TOP_NAME dut;

class CPUProxy
{
    uint32_t* pc_binding;
    uint32_t* inst_binding;
    uint32_t* register_bindings[16];

public:
    CPUProxy() = default;

    void bind(TOP_NAME* dut);
    void dump_registers(std::ostream& os);
    uint32_t pc();
    uint32_t curr_inst();
    uint32_t reg(uint32_t idx);
};

extern CPUProxy cpu;
#endif

