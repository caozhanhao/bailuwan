// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

#include "trace.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>
#include <string>

enum { DIFFTEST_TO_DUT, DIFFTEST_TO_REF };

using difftest_memcpy_t = void (*)(uint32_t addr, void* buf, size_t n, bool direction);
using difftest_regcpy_t = void (*)(void* dut, bool direction);
using difftest_exec_t = void (*)(uint64_t n);
using difftest_raise_intr_t = void (*)(uint64_t NO);

using difftest_tracesim_init_t = void (*)(uint32_t batch_size);
using difftest_tracesim_step_t = void (*)(void* batch);

difftest_memcpy_t ref_difftest_memcpy;
difftest_regcpy_t ref_difftest_regcpy;
difftest_exec_t ref_difftest_exec;
difftest_raise_intr_t ref_difftest_raise_intr;

difftest_tracesim_init_t ref_difftest_tracesim_init;
difftest_tracesim_step_t ref_difftest_tracesim_step;

struct diff_context_t
{
    uint32_t gpr[32];
    uint32_t pc;
    uint32_t csr[4096];
};

void init_tracesim(void* img, size_t img_size)
{
    auto npc_home = getenv("NPC_HOME");
    assert(npc_home);

    auto ref_so_path = std::string(npc_home) + "/sim/common/lib/riscv32-nemu-interpreter-so";

    printf("Loading %s\n", ref_so_path.c_str());

    auto handle = dlopen(ref_so_path.c_str(), RTLD_LAZY);
    if (!handle)
    {
        fprintf(stderr, "dlopen error: %s\n", dlerror());
        assert(false);
    }

    ref_difftest_memcpy = reinterpret_cast<difftest_memcpy_t>(dlsym(handle, "difftest_memcpy"));
    assert(ref_difftest_memcpy);

    ref_difftest_regcpy = reinterpret_cast<difftest_regcpy_t>(dlsym(handle, "difftest_regcpy"));
    assert(ref_difftest_regcpy);

    ref_difftest_exec = reinterpret_cast<difftest_exec_t>(dlsym(handle, "difftest_exec"));
    assert(ref_difftest_exec);

    ref_difftest_raise_intr = reinterpret_cast<difftest_raise_intr_t>(dlsym(handle, "difftest_raise_intr"));
    assert(ref_difftest_raise_intr);

    ref_difftest_tracesim_init = reinterpret_cast<difftest_tracesim_init_t>(dlsym(handle, "difftest_tracesim_init"));
    assert(ref_difftest_tracesim_init);

    ref_difftest_tracesim_step = reinterpret_cast<difftest_tracesim_step_t>(dlsym(handle, "difftest_tracesim_step"));
    assert(ref_difftest_tracesim_step);

    using difftest_init_t = void (*)(int);
    auto ref_difftest_init = reinterpret_cast<difftest_init_t>(dlsym(handle, "difftest_init"));
    assert(ref_difftest_init);

    // Initialize DiffTest
    ref_difftest_init(0);

    // Initialize Memory
    ref_difftest_memcpy(RESET_VECTOR, img, img_size, DIFFTEST_TO_REF);

    // Initialize PC
    diff_context_t ctx{};
    ctx.pc = RESET_VECTOR;
    ref_difftest_regcpy(&ctx, DIFFTEST_TO_REF);

    // Initialize tracesim
    ref_difftest_tracesim_init(BATCH_SIZE);
}

struct tracesim_batch
{
    // PC stream
    uint32_t* i_stream;
    uint32_t i_size;

    // ldstr stream
    struct dcache_entry
    {
        bool is_read;
        uint32_t addr;
    } * d_stream;

    uint32_t d_size;

    // branch stream
    struct branch_entry
    {
        uint32_t pc;
        uint32_t target;
        bool is_uncond;
        bool taken;
    } * b_stream;

    uint32_t b_size;
};

uint32_t i_buffer[BATCH_SIZE];
tracesim_batch::dcache_entry d_buffer[BATCH_SIZE];
tracesim_batch::branch_entry b_buffer[BATCH_SIZE];

void drain_stream(
    const std::function<void(uint32_t)>& pc_consumer,
    const std::function<void(bool, uint32_t)>& ldstr_consumer,
    const std::function<void(uint32_t, uint32_t, bool, bool)>& branch_consumer)
{
    tracesim_batch batch{};
    batch.i_stream = i_buffer;
    batch.d_stream = d_buffer;
    batch.b_stream = b_buffer;

    while (true)
    {
        ref_difftest_tracesim_step(&batch);

        for (uint32_t i = 0; i < batch.i_size; i++)
            pc_consumer(i_buffer[i]);

        for (uint32_t i = 0; i < batch.d_size; i++)
            ldstr_consumer(d_buffer[i].is_read, d_buffer[i].addr);

        for (uint32_t i = 0; i < batch.b_size; i++)
        {
            branch_consumer(b_buffer[i].pc, b_buffer[i].target,
                            b_buffer[i].is_uncond, b_buffer[i].taken);
        }

        if (batch.i_size != BATCH_SIZE)
            break;
    }
}
