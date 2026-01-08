// Copyright (c) 2025-2026 caozhanhao
// SPDX-License-Identifier: MulanPSL-2.0

#ifndef BAILUWAN_DISASM_HPP
#define BAILUWAN_DISASM_HPP

#include <string>
#include <cstdint>

std::string rv32_disasm(uint32_t pc, uint32_t inst);
void init_disasm();

#endif
