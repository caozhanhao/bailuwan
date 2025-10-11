#ifndef NPC_CSRC_DISASM_HPP
#define NPC_CSRC_DISASM_HPP

#include <cstdint>
#include <string>

void init_disasm();
std::string disassemble(uint32_t pc, uint32_t inst);

#endif