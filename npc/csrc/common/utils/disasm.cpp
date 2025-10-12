#include <dlfcn.h>
#include <capstone/capstone.h>
#include <cassert>

#include "disasm.hpp"

void Disassembler::init()
{
    void *dl_handle;
    dl_handle = dlopen("csrc/common/utils/libcapstone.so.5", RTLD_LAZY);
    assert(dl_handle);

    opendl_t cs_open_dl = NULL;
    cs_open_dl = reinterpret_cast<opendl_t>(dlsym(dl_handle, "cs_open"));
    assert(cs_open_dl);

    cs_disasm_dl = reinterpret_cast<decltype(cs_disasm_dl)>(dlsym(dl_handle, "cs_disasm"));
    assert(cs_disasm_dl);

    cs_free_dl = reinterpret_cast<decltype(cs_free_dl)>(dlsym(dl_handle, "cs_free"));
    assert(cs_free_dl);

    int ret = cs_open_dl(CS_ARCH_RISCV, CS_MODE_RISCV32, &handle);
    assert(ret == CS_ERR_OK);
}

std::string Disassembler::disassemble(uint32_t pc, uint32_t inst) {
    char buffer[128];

    cs_insn *insn;
    uint8_t* code = reinterpret_cast<uint8_t *>(&inst);
    size_t count = cs_disasm_dl(handle, code, 4, pc, 0, &insn);
    assert(count == 1);
    int ret = snprintf(buffer, 128, "%s", insn->mnemonic);
    if (insn->op_str[0] != '\0')
        snprintf(buffer + ret, 128 - ret, "\t%s", insn->op_str);
    cs_free_dl(insn, count);

    return std::string(buffer);
}