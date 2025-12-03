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
            line.last_access_time = clock;
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
            line.last_access_time = clock;
            return;
        }
    }

    auto lru_it = std::min_element(current_set.begin(), current_set.end(),
                                   [](const CacheLine& a, const CacheLine& b)
                                   {
                                       return a.last_access_time < b.last_access_time;
                                   });

    lru_it->tag = tag;
    lru_it->last_access_time = clock;
}

void ICacheSim::dump(FILE* stream) const
{
    uint64_t total = hit + miss;
    double rate = (total > 0) ? static_cast<double>(hit) / static_cast<double>(total) * 100.0 : 0.0;

    fprintf(stream, "--- ICache Statistics ---\n");
    fprintf(stream, "cache_size=%u, block_size=%u, set_size=%u\n",
            cache_size, block_size, set_size);
    fprintf(stream, "Total: %lu\n", total);
    fprintf(stream, "Hits:           %lu\n", hit);
    fprintf(stream, "Misses:         %lu\n", miss);
    fprintf(stream, "Hit Rate:       %.2f%%\n", rate);
    fprintf(stream, "-------------------------\n");
}
