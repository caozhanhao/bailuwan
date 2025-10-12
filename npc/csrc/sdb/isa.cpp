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

static int ftrace_dump(int rd, int rs1, word_t imm, char* buf, size_t buf_size)
{
    // call:
    //   jal  ra, imm        ->  s->dnpc = s->pc + imm;
    //   jalr ra, rs1, imm   ->  s->dnpc = (src1 + imm) & ~1
    // tail:
    //   jalr x0, x6, imm
    bool is_call = rd == 1 || (rd == 0 && rs1 != 1);

    // ret:
    //   jalr x0, ra, 0
    bool is_ret = rd == 0 && rs1 == 1 && imm == 0;

    if (!is_call && !is_ret)
    {
        Log("Unrecognized jal/jalr: rd=%d, rs1=%d, imm=" FMT_WORD, rd, rs1, imm);
        return -1;
    }
    if (is_call && is_ret)
    {
        Log("Ambiguous jal/jalr: rd=%d, rs1=%d, imm=" FMT_WORD, rd, rs1, imm);
        return -1;
    }

    static int depth = 0;
    depth = is_call ? depth + 1 : depth - 1;

    auto pc = sim_handle.get_cpu().pc();
    auto dnpc = sim_handle.get_cpu().dnpc();

    if (is_call)
    {
        const char* callee = ftrace_search(dnpc);
        if (callee == nullptr)
        {
            Log("ftrace: Unknown function at " FMT_WORD, dnpc);
            return -1;
        }

        snprintf(buf, buf_size, FMT_WORD ": %*s%s [%s@" FMT_WORD "], depth=%d",
                 pc, depth * 2, "", rd == 1 ? "call" : "tail", callee, dnpc, depth);
    }
    else if (is_ret)
    {
        const char* callee = ftrace_search(pc);
        if (callee == nullptr)
        {
            Log("ftrace: Unknown function at " FMT_WORD, pc);
            return -1;
        }
        snprintf(buf, buf_size, FMT_WORD ": %*sret [%s], depth=%d",
                 pc, (depth + 1) * 2, "", callee, depth + 1);
    }
    else
    {
        panic("Unreachable");
    }

    return 0;
}

int isa_ftrace_dump(char* buf, size_t buf_size)
{
    auto inst = sim_handle.get_cpu().curr_inst();


    // jal
    if (BITS(inst, 6, 0) == 0b1101111)
    {
        int rd = BITS(inst, 11, 7);
        int rs1 = BITS(inst, 19, 15);
        word_t imm = (SEXT(BITS(inst, 31, 31), 1) << 20) | (BITS(inst, 19, 12) << 12) | (BITS(inst, 20, 20) << 11) | (
            BITS(inst, 30, 21) << 1);
        return ftrace_dump(rd, rs1, imm, buf, buf_size);
    }

    // jalr
    if (BITS(inst, 6, 0) == 0b1100111 && BITS(inst, 14, 12) == 0)
    {
        int rd = BITS(inst, 11, 7);
        int rs1 = BITS(inst, 19, 15);
        word_t imm = SEXT(BITS(inst, 31, 20), 12);
        return ftrace_dump(rd, rs1, imm, buf, buf_size);
    }

    return -2;
}
