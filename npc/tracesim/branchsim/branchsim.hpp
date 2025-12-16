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

    void step(uint32_t pc, uint32_t target, bool is_uncond, bool taken);

    void dump();
};


#endif
