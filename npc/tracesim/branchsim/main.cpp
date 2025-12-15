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

    drain_stream([&](uint32_t pc)
                 {
                     // pass
                 },
                 [&](bool is_read, uint32_t addr)
                 {
                     // pass
                 }, [&](uint32_t pc, uint32_t target, bool taken)
                 {
                     printf("Branch: %08x -> %08x (%s)\n", pc, target, taken ? "taken" : "not taken");
                 });

    free(image);
    return 0;
}
