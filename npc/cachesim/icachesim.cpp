#include "icachesim.hpp"

void ICacheSim::step(uint32_t pc)
{
    clock++;

    uint32_t index = (pc >> offset_bits) & index_mask;
    uint32_t tag = pc >> (offset_bits + index_bits);

    auto& current_set = sets[index];

    for (auto& line : current_set)
    {
        if (line.valid && line.tag == tag)
        {
            hit++;

            if (replacement_policy == ReplacementPolicy::LRU)
                line.timestamp = clock;
            return;
        }
    }

    miss++;

    for (auto& line : current_set)
    {
        if (!line.valid)
        {
            line.valid = true;
            line.tag = tag;

            // Both LRU and FIFO updates the timestamp here.
            line.timestamp = clock;
            return;
        }
    }

    std::vector<CacheLine>::iterator victim_it;

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

    // Replace the victim
    victim_it->tag = tag;

    // Reset timestamp for the new line
    victim_it->timestamp = clock;
}

void ICacheSim::dump(FILE* stream) const
{
    uint64_t total = hit + miss;

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

    fprintf(stream, "Storage: cache_size=%u, block_size=%u, set_size=%u\n",
            cache_size, block_size, set_size);
    fprintf(stream, "Policy:   %s\n", policy_str);
    fprintf(stream, "Total:    %lu\n", total);
    fprintf(stream, "Hits:     %lu\n", hit);
    fprintf(stream, "Misses:   %lu\n", miss);
    fprintf(stream, "Hit Rate: %.2f%%\n", get_hit_rate());
}
