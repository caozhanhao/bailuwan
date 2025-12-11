// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

// SDB is adapted from sdb in NEMU.

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

static void sync_regs_to_ref()
{
    auto& cpu = SIM.cpu();

    diff_context_t ctx;
    for (int i = 0; i < 16; i++)
        ctx.gpr[i] = cpu.reg(i);
    for (int i = 0; i < 4096; i++)
    {
        if (cpu.is_csr_valid(i))
            ctx.csr[i] = cpu.csr(i);
        else
            ctx.csr[i] = 0;
    }
    ctx.pc = cpu.pc();
    ref_difftest_regcpy(&ctx, DIFFTEST_TO_REF);
    // Log("Syncing to ref at pc: " FMT_WORD "\n", cpu.pc());
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

    sync_regs_to_ref();
}

static void checkregs(diff_context_t* ref)
{
    auto& cpu = SIM.cpu();
    bool match = true;
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

    if (cpu.pc() != ref->pc)
    {
        Log("pc: expected " FMT_WORD ", but got " FMT_WORD "\n", ref->pc, cpu.pc());
        match = false;
    }

    if (!match)
    {
        sdb_state = SDBState::Abort;
        printf("Test failed at pc = " FMT_WORD "\n", cpu.pc());
        cpu.dump();
    }
}

static bool is_accessing_device()
{
    auto& cpu = SIM.cpu();
    auto inst = cpu.curr_inst();

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

// Difftest happens after each cycle, and before the rising edge of the next cycle.
//
//              _____       _____
//   clock     |     |_____|     |_____
//              cycle 1        cycle 2
//                     ^
//                     |
//          difftest_step is called here
void difftest_step()
{
    // If this cycle is ready for difftest,
    // skip this cycle but do NOT sync registers.
    auto& cpu = SIM.cpu();
    if (!cpu.is_ready_for_difftest())
        return;

    if (is_accessing_device())
    {
        // printf("Skipped 0x%x\n", SIM.cpu().curr_inst());
        sync_regs_to_ref();
        return;
    }

    ref_difftest_exec(1);

    diff_context_t ref_r;
    ref_difftest_regcpy(&ref_r, DIFFTEST_TO_DUT);
    checkregs(&ref_r);
}
#else
void init_difftest()
{
}
void difftest_step()
{
}
#endif
