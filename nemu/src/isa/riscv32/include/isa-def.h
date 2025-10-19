/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#ifndef __ISA_RISCV_H__
#define __ISA_RISCV_H__

#include <common.h>

enum {
  PRIV_LEVEL_U = 0b00,
  PRIV_LEVEL_S = 0b01,
  PRIV_LEVEL_M = 0b11,
};

typedef struct {
  word_t gpr[MUXDEF(CONFIG_RVE, 16, 32)];
  vaddr_t pc;
  word_t csr[4096];
  uint8_t priv_level;
} MUXDEF(CONFIG_RV64, riscv64_CPU_state, riscv32_CPU_state);

// decode
typedef struct {
  uint32_t inst;
} MUXDEF(CONFIG_RV64, riscv64_ISADecodeInfo, riscv32_ISADecodeInfo);

#define CSR_TABLE                                                                                                      \
  CSR_TABLE_ENTRY(mstatus, 0x300)                                                                                      \
  CSR_TABLE_ENTRY(mtvec, 0x305)                                                                                        \
  CSR_TABLE_ENTRY(mepc, 0x341)                                                                                         \
  CSR_TABLE_ENTRY(mcause, 0x342)                                                                                       \
  CSR_TABLE_ENTRY(mcycle, 0xb00)                                                                                       \
  CSR_TABLE_ENTRY(mcycleh, 0xb80)                                                                                      \
  CSR_TABLE_ENTRY(mvendorid, 0xf11)                                                                                    \
  CSR_TABLE_ENTRY(marchid, 0xf12)

// Name to Index
// CSR_mstatus = 0x300, ...
enum {
#define CSR_TABLE_ENTRY(name, idx) CSR_##name = idx,
  CSR_TABLE
#undef CSR_TABLE_ENTRY
};

#define isa_mmu_check(vaddr, len, type) (MMU_DIRECT)

#endif
