#ifndef NPC_CSRC_DISASM_HPP
#define NPC_CSRC_DISASM_HPP

#include <string>
#include <capstone/capstone.h>

class Disassembler
{
    using disasm_dl_t = size_t (*)(csh handle, const uint8_t *code,
                              size_t code_size, uint64_t address, size_t count, cs_insn **insn);
    using free_dl_t = void (*)(cs_insn *insn, size_t count);
    using opendl_t = cs_err (*)(cs_arch arch, cs_mode mode, csh *handle);

    disasm_dl_t cs_disasm_dl;
    free_dl_t cs_free_dl;
    csh handle;
public:
    Disassembler() = default;

    void init();

    std::string disassemble(uint32_t pc, uint32_t inst);
};

#endif