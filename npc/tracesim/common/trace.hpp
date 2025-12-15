// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MIT

#ifndef BAILUWAN_TRACESIM_COMMON_TRACE_HPP
#define BAILUWAN_TRACESIM_COMMON_TRACE_HPP

#include <cstdint>
#include <cstring>
#include <functional>

constexpr auto RESET_VECTOR = 0x30000000;
constexpr auto MAX_IMAGE_SIZE = 32 * 1024 * 1024;
constexpr auto BATCH_SIZE = 8192;

void init_tracesim(void* img, size_t img_size);

void drain_stream(
    // PC
    const std::function<void(uint32_t)>& pc_consumer,
    // (is_read, addr)
    const std::function<void(bool, uint32_t)>& ldstr_consumer,
    // (pc, branch target, is taken)
    const std::function<void(uint32_t, uint32_t, bool)>& branch_consumer);

#endif
