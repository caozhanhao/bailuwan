// Copyright (c) 2025 caozhanhao
// SPDX-License-Identifier: MulanPSL-2.0

#include <dlfcn.h>
#include <capstone/capstone.h>
#include <cassert>

#include "disasm.hpp"

#include <iostream>

#include "dut_proxy.hpp"

using disasm_dl_t = size_t (*)(csh handle, const uint8_t* code,
                               size_t code_size, uint64_t address, size_t count, cs_insn** insn);
using free_dl_t = void (*)(cs_insn* insn, size_t count);
using opendl_t = cs_err (*)(cs_arch arch, cs_mode mode, csh* handle);

static disasm_dl_t cs_disasm_dl;
static free_dl_t cs_free_dl;
static csh handle;

std::string rv32_disasm(uint32_t pc, uint32_t inst)
{
    if (inst == 0)
        return "<null>";

    char buffer[128];

    cs_insn* insn;
    auto* code = reinterpret_cast<uint8_t*>(&inst);
    size_t count = cs_disasm_dl(handle, code, 4, pc, 0, &insn);
    if (count != 1)
    {
        fprintf(stderr, "Disassembler error at inst: 0x%x\n", inst);
        SIM.cpu().dump();
        SIM.cleanup();
        exit(-1);
    }
    int ret = snprintf(buffer, 128, "%s", insn->mnemonic);
    if (insn->op_str[0] != '\0')
        snprintf(buffer + ret, 128 - ret, "\t%s", insn->op_str);
    cs_free_dl(insn, count);

    return {buffer};
}

void init_disasm()
{
    auto dl_handle = dlopen("sim/common/lib/libcapstone.so.5", RTLD_LAZY);
    assert(dl_handle);

    auto cs_open_dl = reinterpret_cast<opendl_t>(dlsym(dl_handle, "cs_open"));
    assert(cs_open_dl);

    cs_disasm_dl = reinterpret_cast<decltype(cs_disasm_dl)>(dlsym(dl_handle, "cs_disasm"));
    assert(cs_disasm_dl);

    cs_free_dl = reinterpret_cast<decltype(cs_free_dl)>(dlsym(dl_handle, "cs_free"));
    assert(cs_free_dl);

    int ret = cs_open_dl(CS_ARCH_RISCV, CS_MODE_RISCV32, &handle);
    assert(ret == CS_ERR_OK);
}
