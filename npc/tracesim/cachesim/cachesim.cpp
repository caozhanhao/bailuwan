// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MulanPSL-2.0

#include "cachesim.hpp"

#include <cassert>

void CacheSim::access(uint32_t addr, AccessType type)
{
    clock++;

    uint32_t index = (addr >> offset_bits) & index_mask;
    uint32_t tag = addr >> (offset_bits + index_bits);

    auto& current_set = sets[index];

    for (auto& line : current_set)
    {
        if (line.valid && line.tag == tag)
        {
            if (type == AccessType::READ)
                read_hits++;
            else
            {
                write_hits++;

                if (write_policy == WritePolicy::WRITE_BACK)
                    line.dirty = true;
                else
                    write_throughs++;
            }

            if (replacement_policy == ReplacementPolicy::LRU)
                line.timestamp = clock;
            return;
        }
    }

    if (type == AccessType::READ)
        read_misses++;
    else
    {
        write_misses++;
        if (alloc_policy == AllocationPolicy::NO_WRITE_ALLOCATE)
        {
            write_throughs++;
            return;
        }
    }

    // Prefer empty line
    auto victim_it = std::find_if(current_set.begin(), current_set.end(),
                                  [](const CacheLine& l) { return !l.valid; });

    if (victim_it == current_set.end())
    {
        if (replacement_policy == ReplacementPolicy::RANDOM)
        {
            // Randomly select a victim index between 0 and set_size - 1
            std::uniform_int_distribution<uint32_t> dist(0, set_size - 1);
            uint32_t idx = dist(rng);
            victim_it = current_set.begin() + idx;
        }
        else
        {
            // For LRU:  The smallest timestamp -> Least Recently Used.
            // For FIFO: The smallest timestamp -> the Earliest Inserted.
            victim_it = std::min_element(current_set.begin(), current_set.end(),
                                         [](const CacheLine& a, const CacheLine& b)
                                         {
                                             return a.timestamp < b.timestamp;
                                         });
        }
    }

    // Evicting a dirty line under WB, write it back
    if (victim_it->valid && victim_it->dirty && write_policy == WritePolicy::WRITE_BACK)
        write_backs++;

    // Replace the victim
    victim_it->valid = true;
    victim_it->tag = tag;

    // Reset timestamp for the new line
    victim_it->timestamp = clock;

    if (type == AccessType::WRITE)
    {
        if (write_policy == WritePolicy::WRITE_BACK)
            victim_it->dirty = true;
        else
        {
            victim_it->dirty = false;
            write_throughs++;
        }
    }
    else
        victim_it->dirty = false;
}

void CacheSim::dump(FILE* stream) const
{
    uint64_t total = get_total_accesses();

    auto policy_str = "Unknown";
    switch (replacement_policy)
    {
    case ReplacementPolicy::LRU:
        policy_str = "LRU";
        break;
    case ReplacementPolicy::FIFO:
        policy_str = "FIFO";
        break;
    case ReplacementPolicy::RANDOM:
        policy_str = "RANDOM";
        break;
    }

    auto w_policy_str = (write_policy == WritePolicy::WRITE_BACK) ? "Write-Back" : "Write-Through";
    auto a_policy_str = (alloc_policy == AllocationPolicy::WRITE_ALLOCATE) ? "Write-Allocate" : "No-Write-Allocate";

    auto accesses = read_misses + write_throughs + write_backs;

    if (alloc_policy == AllocationPolicy::WRITE_ALLOCATE)
        accesses += write_misses;

    fprintf(stream, "Config: Size=%uB, Block=%uB, Ways=%u\n", cache_size, block_size, set_size);
    fprintf(stream, "Policies: Repl=%s, Write=%s, Alloc=%s\n", policy_str, w_policy_str, a_policy_str);
    fprintf(stream, "Accesses:      %lu\n", total);
    fprintf(stream, "  Read Hits:   %lu\n", read_hits);
    fprintf(stream, "  Read Miss:   %lu\n", read_misses);
    fprintf(stream, "  Write Hits:  %lu\n", write_hits);
    fprintf(stream, "  Write Miss:  %lu\n", write_misses);
    fprintf(stream, "Hit Rate:      %.2f%%\n", get_hit_rate());
    fprintf(stream, "AMAT:          %.2f cycles\n", get_AMAT());
    fprintf(stream, "TMT:           %.2f cycles\n", static_cast<double>(accesses) * miss_penalty);
    fprintf(stream, "Memory Stats:\n");
    fprintf(stream, "  Write Backs:   %lu\n", write_backs);
    fprintf(stream, "  Writes Throughs: %lu\n", write_throughs);
    fprintf(stream, "============================================================\n");
}
