/***************************************************************************************
 * Copyright (c) 2014-2024 Zihao Yu, Nanjing University
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan
 *PSL v2. You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 *KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 *NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

#include "dut_proxy.hpp"
#include "sdb.hpp"

#include <iostream>

void isa_reg_display()
{
    SIM.cpu().dump_gprs(stdout);
}

void isa_csr_display()
{
    SIM.cpu().dump_csrs(stdout);
}

word_t isa_csr_str2val(const char* s, bool* success)
{
    auto& cpu = SIM.cpu();
    for (int i = 0; i < 4096; i++)
    {
        if (csr_names[i] == nullptr)
            continue;

        if (strcmp(s, csr_names[i]) == 0)
        {
            *success = true;
            return cpu.csr(i);
        }
    }
    *success = false;
    return 0;
}

word_t isa_reg_str2val(const char* s, bool* success)
{
    auto& cpu = SIM.cpu();

    if (strcmp(s, "pc") == 0)
    {
        *success = true;
        return cpu.exu_pc();
    }

    if (s[0] == 'x' || s[0] == 'X')
    {
        char* endptr;
        word_t idx = strtol(s + 1, &endptr, 10);
        if (endptr == s + 1 || idx >= 16)
        {
            *success = false;
            return 0;
        }

        *success = true;
        return cpu.reg(idx);
    }

    for (int i = 0; i < 16; i++)
    {
        if (strcmp(s, gpr_names[i]) == 0)
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

    auto pc = SIM.cpu().exu_pc();
    auto dnpc = SIM.cpu().exu_dnpc();

    if (is_call)
    {
        uint32_t entry_addr;
        const char* callee = ftrace_search(dnpc, &entry_addr);
        if (callee == nullptr)
        {
            Log("ftrace: Unknown jump at " FMT_WORD, dnpc);
            return -1;
        }

        // Not a function call
        if (dnpc != entry_addr)
            return -1;

        snprintf(buf, buf_size, FMT_WORD ": %s [%s@" FMT_WORD "]",
                 pc, (rd == 1 ? "call" : "tail"), callee, entry_addr);
    }
    else if (is_ret)
    {
        const char* callee = ftrace_search(pc, nullptr);
        if (callee == nullptr)
        {
            Log("ftrace: Unknown function at " FMT_WORD, pc);
            return -1;
        }
        snprintf(buf, buf_size, FMT_WORD ": ret [%s]", pc, callee);
    }
    else
    {
        panic("Unreachable");
    }

    return 0;
}

int isa_ftrace_dump(char* buf, size_t buf_size)
{
    auto inst = SIM.cpu().exu_inst();


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
