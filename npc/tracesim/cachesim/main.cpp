// Copyright (c) 2025-2026 caozhanhao
// SPDX-License-Identifier: MulanPSL-2.0

#include "../common/trace.hpp"
#include "cachesim.hpp"

#include <cstdio>
#include <cstring>
#include <cassert>
#include <map>
#include <algorithm>

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

    init_tracesim(image, bytes_read);

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
                 }, [&](uint32_t pc, uint32_t target, bool taken)
                 {
                     // pass
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
