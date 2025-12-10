// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

#ifndef BAILUWAN_CACHE_SIM
#define BAILUWAN_CACHE_SIM

#include <cstdint>
#include <vector>
#include <cstdio>
#include <random>
#include <cmath>
#include <algorithm>
#include <iostream>

enum class AccessType
{
    READ, // Load / Instruction Fetch
    WRITE // Store
};

enum class ReplacementPolicy
{
    FIFO, LRU, RANDOM
};

// Write hit
enum class WritePolicy
{
    WRITE_BACK,
    WRITE_THROUGH
};

// Write miss
enum class AllocationPolicy
{
    WRITE_ALLOCATE,
    NO_WRITE_ALLOCATE
};

struct CacheLine
{
    bool valid = false;
    bool dirty = false; // For Write Back
    uint32_t tag = 0;

    // For LRU:  Represents the last time this line was accessed.
    // For FIFO: Represents the time this line was inserted into the cache.
    // For RANDOM: Not used.
    uint64_t timestamp = 0;
};

class CacheSim
{
private:
    uint64_t read_hits;
    uint64_t read_misses;
    uint64_t write_hits;
    uint64_t write_misses;
    uint64_t write_backs;
    uint64_t write_throughs;

    uint32_t cache_size;
    uint32_t block_size;
    uint32_t set_size;
    double miss_penalty;

    // Calculated
    uint32_t num_sets;
    uint32_t index_bits;
    uint32_t offset_bits;
    uint32_t index_mask;

    ReplacementPolicy replacement_policy;
    WritePolicy write_policy;
    AllocationPolicy alloc_policy;

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
    CacheSim(uint32_t cache_size_, uint32_t block_size_, uint32_t set_size_,
             double miss_penalty_, ReplacementPolicy repl_policy_,
             WritePolicy write_policy_ = WritePolicy::WRITE_BACK,
             AllocationPolicy alloc_policy_ = AllocationPolicy::WRITE_ALLOCATE)
        : read_hits(0), read_misses(0), write_hits(0), write_misses(0),
          write_backs(0), write_throughs(0),
          cache_size(cache_size_), block_size(block_size_), set_size(set_size_),
          miss_penalty(miss_penalty_), replacement_policy(repl_policy_),
          write_policy(write_policy_), alloc_policy(alloc_policy_),
          rng(std::random_device()())
    {
        num_sets = cache_size / (block_size * set_size);

        sets.resize(num_sets);
        for (auto& set : sets)
            set.resize(set_size);

        offset_bits = log2_u32(block_size);
        index_bits = log2_u32(num_sets);

        index_mask = num_sets - 1;
    }

    void access(uint32_t addr, AccessType type);

    [[nodiscard]] uint64_t get_total_hits() const { return read_hits + write_hits; }
    [[nodiscard]] uint64_t get_total_misses() const { return read_misses + write_misses; }
    [[nodiscard]] uint64_t get_total_accesses() const { return get_total_hits() + get_total_misses(); }

    [[nodiscard]] double get_hit_rate() const
    {
        auto total = static_cast<double>(get_total_accesses());
        return total == 0 ? 0.0 : (static_cast<double>(get_total_hits()) / total) * 100.0;
    }

    [[nodiscard]] double get_AMAT() const
    {
        // AMAT = p * access_time + (1 - p) * (access_time + miss_penalty) = access_time + (1 - p) * miss_penalty
        auto access_time = 1;
        auto hit_rate = get_hit_rate() / 100.0;
        auto AMAT = hit_rate * access_time + (1.0 - hit_rate) * (access_time + miss_penalty);
        return AMAT;
    }

    void dump(FILE* stream) const;
};

#endif
