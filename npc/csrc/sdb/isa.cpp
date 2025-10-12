#include "dut_proxy.hpp"
#include "sdb.hpp"

const char* regs[] = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1", "a0",
    "a1", "a2", "a3", "a4", "a5", "a6", "a7", "s2", "s3", "s4", "s5",
    "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

void isa_reg_display()
{
    auto& cpu = sim_handle.get_cpu();
    for (int i = 0; i < 32; i++)
        printf("x%-2d %-5s  0x%08x  %11d\n", i, regs[i], cpu.reg(i), cpu.reg(i));
}

word_t isa_reg_str2val(const char* s, bool* success)
{
    auto& cpu = sim_handle.get_cpu();

    if (strcmp(s, "pc") == 0)
    {
        *success = true;
        return cpu.pc();
    }

    if (s[0] == 'x' || s[0] == 'X')
    {
        char* endptr;
        word_t idx = strtol(s + 1, &endptr, 10);
        if (endptr == s + 1 || idx >= 32)
        {
            *success = false;
            return 0;
        }

        *success = true;
        return cpu.reg(idx);
    }

    for (int i = 0; i < 32; i++)
    {
        if (strcmp(s, regs[i]) == 0)
        {
            *success = true;
            return cpu.reg(i);
        }
    }
    *success = false;
    return 0;
}
