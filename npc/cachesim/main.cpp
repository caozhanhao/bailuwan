// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

#include <iostream>
#include <cstdint>
#include <cassert>
#include <cstring>
#include <functional>
#include <map>
#include <algorithm>

#include <dlfcn.h>

#include "cachesim.hpp"

constexpr auto RESET_VECTOR = 0x30000000;
constexpr auto MAX_IMAGE_SIZE = 32 * 1024 * 1024;
constexpr auto BATCH_SIZE = 8192;

enum { DIFFTEST_TO_DUT, DIFFTEST_TO_REF };

using difftest_memcpy_t = void (*)(uint32_t addr, void* buf, size_t n, bool direction);
using difftest_regcpy_t = void (*)(void* dut, bool direction);
using difftest_exec_t = void (*)(uint64_t n);
using difftest_raise_intr_t = void (*)(uint64_t NO);

using difftest_cachesim_init_t = void (*)(uint32_t batch_size);
using difftest_cachesim_step_t = void (*)(void* batch);

difftest_memcpy_t ref_difftest_memcpy;
difftest_regcpy_t ref_difftest_regcpy;
difftest_exec_t ref_difftest_exec;
difftest_raise_intr_t ref_difftest_raise_intr;

difftest_cachesim_init_t ref_difftest_cachesim_init;
difftest_cachesim_step_t ref_difftest_cachesim_step;

struct diff_context_t
{
    uint32_t gpr[32];
    uint32_t pc;
    uint32_t csr[4096];
};

void init(void* img, size_t img_size)
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

    ref_difftest_cachesim_init = reinterpret_cast<difftest_cachesim_init_t>(dlsym(handle, "difftest_cachesim_init"));
    assert(ref_difftest_cachesim_init);

    ref_difftest_cachesim_step = reinterpret_cast<difftest_cachesim_step_t>(dlsym(handle, "difftest_cachesim_step"));
    assert(ref_difftest_cachesim_step);

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

    // Initialize cachesim
    ref_difftest_cachesim_init(BATCH_SIZE);
}

struct cachesim_batch
{
    // PC stream
    uint32_t* i_stream;
    uint32_t i_size;

    struct dcache_entry
    {
        bool is_read;
        uint32_t addr;
    } * d_stream;

    uint32_t d_size;
};

uint32_t i_buffer[BATCH_SIZE];
cachesim_batch::dcache_entry d_buffer[BATCH_SIZE];

void drain_stream(const std::function<void(uint32_t)>& pc_consumer,
                  const std::function<void(bool, uint32_t)>& ldstr_consumer)
{
    cachesim_batch batch{};
    batch.i_stream = i_buffer;
    batch.d_stream = d_buffer;

    while (true)
    {
        ref_difftest_cachesim_step(&batch);

        for (uint32_t i = 0; i < batch.i_size; i++)
            pc_consumer(i_buffer[i]);

        for (uint32_t i = 0; i < batch.d_size; i++)
            ldstr_consumer(d_buffer[i].is_read, d_buffer[i].addr);

        if (batch.i_size != BATCH_SIZE)
            break;
    }
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: cachesim image_path\n");
        return -1;
    }

    // 64 MB
    auto image = malloc(MAX_IMAGE_SIZE);
    memset(image, 0, MAX_IMAGE_SIZE);

    printf("Initializing from %s\n", argv[1]);
    FILE* fp = fopen(argv[1], "rb");

    auto bytes_read = fread(image, 1, MAX_IMAGE_SIZE, fp);

    init(image, bytes_read);

    std::vector<CacheSim> icache_sims;
    std::vector<CacheSim> dcache_sims;

    // Block size (byte) -> Average miss cycles (miss_penalty)
    std::vector<std::pair<size_t, double>> block_info = {
        {4, 24.014169},
        {8, 45.429843},
        {16, 86.215061},
    };

    std::vector replace_policies = {
        ReplacementPolicy::FIFO,
        ReplacementPolicy::LRU,
        ReplacementPolicy::RANDOM
    };

    std::vector write_policies = {
        WritePolicy::WRITE_BACK,
        WritePolicy::WRITE_THROUGH
    };

    std::vector alloc_policies = {
        AllocationPolicy::WRITE_ALLOCATE,
        AllocationPolicy::NO_WRITE_ALLOCATE
    };

    // Bytes
    std::vector<size_t> i_cache_sizes = {32, 64, 128};
    std::vector<size_t> i_set_sizes = {1, 2}; // n-way
    // ICache
    for (auto policy : replace_policies)
    {
        for (auto cache_size : i_cache_sizes)
        {
            for (auto [block_size, miss_penalty] : block_info)
            {
                for (auto set_size : i_set_sizes)
                {
                    if (set_size * block_size > cache_size)
                        continue;

                    icache_sims.emplace_back(cache_size, block_size, set_size, miss_penalty, policy);
                }
            }
        }
    }

    // DCache
    std::vector<size_t> d_cache_sizes = {32, 64};
    std::vector<size_t> d_set_sizes = {1, 2}; // n-way

    for (auto replace_policy : replace_policies)
    {
        for (auto write_policy : write_policies)
        {
            for (auto alloc_policy : alloc_policies)
            {
                for (auto cache_size : d_cache_sizes)
                {
                    for (auto [block_size, miss_penalty] : block_info)
                    {
                        for (auto set_size : d_set_sizes)
                        {
                            if (set_size * block_size > cache_size)
                                continue;

                            dcache_sims.emplace_back(cache_size, block_size, set_size, miss_penalty,
                                                     replace_policy, write_policy, alloc_policy);
                        }
                    }
                }
            }
        }
    }

    drain_stream([&](uint32_t pc)
                 {
                     for (auto& sim : icache_sims)
                         sim.access(pc, AccessType::READ);
                 },
                 [&](bool is_read, uint32_t addr)
                 {
                     for (auto& sim : dcache_sims)
                         sim.access(addr, is_read ? AccessType::READ : AccessType::WRITE);
                 });

    std::sort(icache_sims.begin(), icache_sims.end(), [](const auto& a, const auto& b)
    {
        return a.get_AMAT() < b.get_AMAT();
    });

    std::sort(dcache_sims.begin(), dcache_sims.end(), [](const auto& a, const auto& b)
    {
        return a.get_AMAT() < b.get_AMAT();
    });


    printf("---------------------------------------------------------------\n");
    printf("                         ICache Sim                            \n");
    printf("---------------------------------------------------------------\n");

    for (auto& sim : icache_sims)
        sim.dump(stdout);

    printf("---------------------------------------------------------------\n");
    printf("                         DCache Sim                            \n");
    printf("---------------------------------------------------------------\n");

    for (auto& sim : dcache_sims)
        sim.dump(stdout);


    printf("-------------------------------------------------------------------------------------------\n");

    // ICache Best AMAT
    for (auto sz : i_cache_sizes)
    {
        double best_AMAT = std::numeric_limits<double>::max();
        const CacheSim* best_sim = nullptr;

        for (auto& sim : icache_sims)
        {
            if (sim.get_cache_size() == sz && sim.get_AMAT() < best_AMAT)
            {
                best_AMAT = sim.get_AMAT();
                best_sim = &sim;
            }
        }

        printf("ICache Best AMAT for %ldB cache:\n", sz);
        best_sim->dump(stdout);
    }

    // DCache Best AMAT
    for (auto sz : d_cache_sizes)
    {
        double best_AMAT = std::numeric_limits<double>::max();
        const CacheSim* best_sim = nullptr;

        for (auto& sim : dcache_sims)
        {
            if (sim.get_cache_size() == sz && sim.get_AMAT() < best_AMAT)
            {
                best_AMAT = sim.get_AMAT();
                best_sim = &sim;
            }
        }

        printf("DCache Best AMAT for %ldB cache:\n", sz);
        best_sim->dump(stdout);
    }


    free(image);
    return 0;
}
