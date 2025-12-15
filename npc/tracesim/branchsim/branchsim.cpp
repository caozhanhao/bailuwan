#include "branchsim.hpp"
#include <cstdint>
#include <cstdio>
#include <vector>

void BranchSim::step(uint32_t pc, uint32_t target, bool is_uncond, bool taken)
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

    bool mispredicted = (predict_taken != taken) || (predict_taken && (predict_target != target));

    if (mispredicted)
        mispredictions++;

    uint32_t w_idx = (pc >> 2) & index_mask;
    uint64_t w_tag = pc >> (index_bits + 2);

    storage[w_idx].valid = true;
    storage[w_idx].tag = w_tag;
    storage[w_idx].target = target;
    storage[w_idx].is_uncond = is_uncond;
}

void BranchSim::dump()
{
    printf("BranchSim Results:\n");
    printf("  BTB Entries: %d\n", entries);
    printf("  Total branches: %lu\n", total_branches);
    printf("  Mispredictions: %lu\n", mispredictions);
    printf("  Misprediction rate: %.2f%%\n",
           static_cast<double>(mispredictions) / static_cast<double>(total_branches) * 100.0);
}
