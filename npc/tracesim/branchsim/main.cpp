// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

#include "../common/trace.hpp"

#include <cstdio>
#include <cstring>
#include <cassert>
#include <map>
#include <algorithm>

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: branchsim image_path\n");
        return -1;
    }

    // 64 MB
    auto image = malloc(MAX_IMAGE_SIZE);
    memset(image, 0, MAX_IMAGE_SIZE);

    printf("Initializing from %s\n", argv[1]);
    FILE* fp = fopen(argv[1], "rb");

    auto bytes_read = fread(image, 1, MAX_IMAGE_SIZE, fp);

    init_tracesim(image, bytes_read);

    uint64_t total_branches = 0;
    uint64_t btfn_mispredictions = 0;
    uint64_t always_not_taken_branches = 0;

    drain_stream([&](uint32_t pc)
                 {
                     // pass
                 },
                 [&](bool is_read, uint32_t addr)
                 {
                     // pass
                 }, [&](uint32_t pc, uint32_t target, bool taken)
                 {
                     // printf("Branch: %08x -> %08x (%s)\n", pc, target, taken ? "taken" : "not taken");
                     ++total_branches;

                     // BTFN (Backward Taken, Forward Not-taken)
                     if ((pc < target && !taken) || (pc > target && taken))
                         ++btfn_mispredictions;
                 });

    printf("Total branches: %lu\n", total_branches);
    printf("Mispredictions: %lu\n", btfn_mispredictions);
    printf("Misprediction rate: %.2f%%\n",
           static_cast<double>(btfn_mispredictions) / static_cast<double>(total_branches) * 100.0);

    free(image);
    return 0;
}
