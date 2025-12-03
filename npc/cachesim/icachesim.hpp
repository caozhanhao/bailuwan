// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

#ifndef BAILUWAN_ICACHE_SIM
#define BAILUWAN_ICACHE_SIM

#include <cstdint>
#include <vector>
#include <cstdio>
#include <random>
#include <cmath>
#include <algorithm>
#include <iostream>

struct CacheLine
{
    bool valid = false;
    uint32_t tag = 0;

    // For LRU:  Represents the last time this line was accessed.
    // For FIFO: Represents the time this line was inserted into the cache.
    // For RANDOM: Not used.
    uint64_t timestamp = 0;
};

enum class ReplacementPolicy
{
    FIFO, LRU, RANDOM
};

class ICacheSim
{
private:
    uint64_t hit;
    uint64_t miss;

    uint32_t cache_size;
    uint32_t block_size;
    uint32_t set_size;

    // Calculated
    uint32_t num_sets;
    uint32_t index_bits;
    uint32_t offset_bits;
    uint32_t index_mask;

    // Replacement Policy
    ReplacementPolicy replacement_policy;

    std::vector<std::vector<CacheLine>> sets;

    // For LRU, FIFO
    uint64_t clock = 0;

    // For RANDOM
    std::mt19937 rng;

    static uint32_t log2_u32(uint32_t n)
    {
        uint32_t r = 0;
        while ((n >>= 1) != 0) r++;
        return r;
    }

public:
    ICacheSim(uint32_t cache_size_, uint32_t block_size_, uint32_t set_size_,
              ReplacementPolicy policy_)
        : hit(0), miss(0), cache_size(cache_size_), block_size(block_size_), set_size(set_size_),
          replacement_policy(policy_)
    {
        num_sets = cache_size / (block_size * set_size);

        sets.resize(num_sets);
        for (auto& set : sets)
            set.resize(set_size);

        offset_bits = log2_u32(block_size);
        index_bits = log2_u32(num_sets);

        index_mask = num_sets - 1;

        // Initialize random
        std::random_device rd;
        rng.seed(rd());
    }

    [[nodiscard]] uint64_t get_hit() const { return hit; }
    [[nodiscard]] uint64_t get_miss() const { return miss; }

    void step(uint32_t pc);
    void dump(FILE* stream) const;
};
#endif
