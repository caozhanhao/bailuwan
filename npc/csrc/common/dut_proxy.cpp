#include "dut_proxy.hpp"
#include <iostream>
#include <iomanip>

TOP_NAME dut;
CPUProxy cpu;

void CPUProxy::bind(TOP_NAME* this_dut)
{
#define BIND(reg) register_bindings[reg] = &this_dut->io_registers_##reg;
    BIND(0)
    BIND(1)
    BIND(2)
    BIND(3)
    BIND(4)
    BIND(5)
    BIND(6)
    BIND(7)
    BIND(8)
    BIND(9)
    BIND(10)
    BIND(11)
    BIND(12)
    BIND(13)
    BIND(14)
    BIND(15)
#undef BIND

    pc_binding = &dut.io_pc;
    inst_binding = &dut.io_inst;
}

uint32_t CPUProxy::curr_inst()
{
    return *inst_binding;
}

uint32_t CPUProxy::pc()
{
    return *pc_binding;
}

uint32_t CPUProxy::reg(uint32_t idx)
{
    return *register_bindings[idx];
}

void CPUProxy::dump_registers(std::ostream& os)
{
    for (int i = 0; i < 32; i++)
        os << "x" << i << ": " << std::hex << std::setfill('0') << std::setw(8) << reg(i) << std::endl;
}



