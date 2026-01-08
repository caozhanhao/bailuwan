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

#include <dlfcn.h>

#include <iostream>

#include "utils/disasm.hpp"

enum { DIFFTEST_TO_DUT, DIFFTEST_TO_REF };

using difftest_memcpy_t = void (*)(uint32_t addr, void* buf, size_t n, bool direction);
using difftest_regcpy_t = void (*)(void* dut, bool direction);
using difftest_exec_t = void (*)(uint64_t n);
using difftest_raise_intr_t = void (*)(uint64_t NO);

difftest_memcpy_t ref_difftest_memcpy;
difftest_regcpy_t ref_difftest_regcpy;
difftest_exec_t ref_difftest_exec;
difftest_raise_intr_t ref_difftest_raise_intr;

struct diff_context_t
{
    word_t gpr[32];
    word_t pc;
    word_t csr[4096];
};

#ifdef CONFIG_DIFFTEST

static void sync_regs_to_ref(uint32_t pc)
{
    auto& cpu = SIM.cpu();

    diff_context_t ctx{};
    for (int i = 0; i < 16; i++)
        ctx.gpr[i] = cpu.reg(i);
    for (int i = 0; i < 4096; i++)
    {
        if (cpu.is_csr_valid(i))
            ctx.csr[i] = cpu.csr(i);
        else
            ctx.csr[i] = 0;
    }
    ctx.pc = pc;
    ref_difftest_regcpy(&ctx, DIFFTEST_TO_REF);
}

void init_difftest(size_t img_size)
{
    const char* ref_so_file = "sim/common/lib/riscv32-nemu-interpreter-so";

    auto handle = dlopen(ref_so_file, RTLD_LAZY);
    assert(handle);

    ref_difftest_memcpy = reinterpret_cast<difftest_memcpy_t>(dlsym(handle, "difftest_memcpy"));
    assert(ref_difftest_memcpy);

    ref_difftest_regcpy = reinterpret_cast<difftest_regcpy_t>(dlsym(handle, "difftest_regcpy"));
    assert(ref_difftest_regcpy);

    ref_difftest_exec = reinterpret_cast<difftest_exec_t>(dlsym(handle, "difftest_exec"));
    assert(ref_difftest_exec);

    ref_difftest_raise_intr = reinterpret_cast<difftest_raise_intr_t>(dlsym(handle, "difftest_raise_intr"));
    assert(ref_difftest_raise_intr);

    using difftest_init_t = void (*)(int);
    auto ref_difftest_init = reinterpret_cast<difftest_init_t>(dlsym(handle, "difftest_init"));
    assert(ref_difftest_init);

    Log("Differential testing: %s", ANSI_FMT("ON", ANSI_FG_GREEN));
    Log("The result of every instruction will be compared with %s. "
        "This will help you a lot for debugging, but also significantly reduce the performance. "
        "If it is not necessary, you can turn it off in menuconfig.", ref_so_file);

    ref_difftest_init(0);
    auto& mem = SIM.mem();

    Log("Initializing memory. RESET_VECTOR=0x%x, img_size=0x%lx", RESET_VECTOR, img_size);
    ref_difftest_memcpy(RESET_VECTOR, mem.guest_to_host(RESET_VECTOR), img_size, DIFFTEST_TO_REF);

    // Initialize registers
    // Don't use cpu.pc() here, since pc is a binding in EXU, and might be invalid at the beginning.
    sync_regs_to_ref(RESET_VECTOR);
}

// After each `difftest_step`, ref's pc is updated to `dnpc`, but difftest_pc() is the
// current instruction's pc. And getting the dut's `dnpc` is not easy here.
// So we delay the pc check one instruction: After each difftest_step, we save the ref's pc,
// which is actually dnpc. At the next call to difftest_step, we check the dut's pc with the
// saved ref's pc.
static uint32_t expected_pc = RESET_VECTOR;

static void check_regs(diff_context_t* ref)
{
    auto& cpu = SIM.cpu();
    bool match = true;

    if (cpu.difftest_pc() != expected_pc)
    {
        Log("pc: expected " FMT_WORD ", but got " FMT_WORD "\n", expected_pc, cpu.difftest_pc());
        match = false;
    }

    for (int i = 0; i < 16; i++)
    {
        if (cpu.reg(i) != ref->gpr[i])
        {
            Log("reg: x%d, expected " FMT_WORD ", but got " FMT_WORD "\n", i, ref->gpr[i], cpu.reg(i));
            match = false;
        }
    }

    for (int i = 0; i < 4096; i++)
    {
        // Don't check mcycle
        if (i == CSR_mcycle)
            continue;

        if (cpu.is_csr_valid(i) && cpu.csr(i) != ref->csr[i])
        {
            Log("csr: addr=%d, name=%s, expected " FMT_WORD ", but got " FMT_WORD "\n", i,
                csr_names[i] ? csr_names[i] : "unknown", ref->csr[i], cpu.csr(i));
            match = false;
        }
    }

    if (!match)
    {
        sdb_state = SDBState::Abort;
        printf("Test failed after difftest_pc=" FMT_WORD ", difftest_inst=" FMT_WORD "\n",
               cpu.difftest_pc(), cpu.difftest_inst());
        cpu.dump();
    }
}

static bool is_accessing_device()
{
    auto& cpu = SIM.cpu();
    auto inst = cpu.difftest_inst();

    bool is_store = BITS(inst, 6, 0) == 0b0100011;
    bool is_load = BITS(inst, 6, 0) == 0b0000011;
    word_t imm;
    // Store
    if (is_store)
        imm = (SEXT(BITS(inst, 31, 25), 7) << 5) | BITS(inst, 11, 7);
    else if (is_load)
        imm = SEXT(BITS(inst, 31, 20), 12);
    else
        return false;

    auto rs1 = BITS(inst, 19, 15);
    auto src1 = cpu.reg(rs1);

    auto addr = src1 + imm;

    // See if it is accessing devices.
    if (DUTMemory::in_device(addr))
    {
        // printf("Accessing device at addr: " FMT_WORD ", inst: 0x%x\n", addr, inst);
        return true;
    }

    return false;
}

void difftest_step()
{
    auto difftest_pc = SIM.cpu().difftest_pc();

    IFDEF(CONFIG_DIFFTEST_TRACE,
          fprintf(stderr, "DIFF_STEP, 0x%x: %s\n", difftest_pc,
              rv32_disasm(difftest_pc, SIM.cpu().difftest_inst()).c_str())
    );

    if (is_accessing_device())
    {
        // ATTENTION: difftest_pc + 4
        //   `is_accessing_device` can only be true in store or load, thus the dnpc
        //   is always difftest_pc + 4. We can NOT use dnpc here because they are bindings in EXU.
        auto dnpc = difftest_pc + 4;
        sync_regs_to_ref(dnpc);
        expected_pc = dnpc;
        return;
    }

    ref_difftest_exec(1);

    diff_context_t ref_r{};
    ref_difftest_regcpy(&ref_r, DIFFTEST_TO_DUT);

    check_regs(&ref_r);

    // Update pc
    expected_pc = ref_r.pc;
}
#else
void init_difftest()
{
}
void difftest_step()
{
}
#endif
