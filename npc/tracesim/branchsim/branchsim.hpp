#ifndef BAILUWAN_TRACESIM_BRANCHSIM_H
#define BAILUWAN_TRACESIM_BRANCHSIM_H

#include <cstdint>
#include <cstdio>
#include <vector>

class BranchSim
{
private:
    struct BTBEntry
    {
        bool valid;
        uint64_t tag;
        uint64_t target;
        bool is_uncond;

        BTBEntry() : valid(false), tag(0), target(0), is_uncond(false)
        {
        }
    };

    const int entries;
    const int index_bits;
    const uint64_t index_mask;

    std::vector<BTBEntry> storage;

    uint64_t total_branches = 0;
    uint64_t mispredictions = 0;


    static uint32_t log2_u32(uint32_t n)
    {
        uint32_t r = 0;
        while ((n >>= 1) != 0) r++;
        return r;
    }

public:
    BranchSim(int btb_entries = 32)
        : entries(btb_entries),
          index_bits(log2_u32(btb_entries)),
          index_mask(btb_entries - 1),
          storage(btb_entries)
    {
    }

    void step(uint32_t pc, uint32_t target, bool is_uncond, bool taken)
    {
        total_branches++;

        uint32_t r_idx = (pc >> 2) & index_mask;
        uint64_t r_tag = pc >> (index_bits + 2);

        const auto& entry = storage[r_idx];
        bool btb_hit = entry.valid && (entry.tag == r_tag);

        bool predict_taken = false;
        uint32_t predict_target = 0;

        if (btb_hit)
        {
            if (entry.is_uncond || entry.target < pc)
                predict_taken = true;
            predict_target = static_cast<uint32_t>(entry.target);
        }

        bool mispredicted = false;
        if (predict_taken != target)
            mispredicted = true;
        else if (predict_taken && (predict_target != target))
            mispredicted = true;

        if (mispredicted)
            mispredictions++;

        uint32_t w_idx = (pc >> 2) & index_mask;
        uint64_t w_tag = pc >> (index_bits + 2);

        storage[w_idx].valid = true;
        storage[w_idx].tag = w_tag;
        storage[w_idx].target = target;
        storage[w_idx].is_uncond = is_uncond;
    }

    void dump()
    {
        printf("BranchSim Results:\n");
        printf("  BTB Entries: %d\n", entries);
        printf("  Total branches: %lu\n", total_branches);
        printf("  Mispredictions: %lu\n", mispredictions);
        printf("  Misprediction rate: %.2f%%\n",
               static_cast<double>(mispredictions) / static_cast<double>(total_branches) * 100.0);
    }
};


#endif
