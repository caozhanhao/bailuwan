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

#ifndef __RISCV_CSR_H__
#define __RISCV_CSR_H__

#include <common.h>

static inline int check_csr_idx(int idx) {
  IFDEF(CONFIG_RT_CHECK, assert(idx >= 0 && idx < 4096));
  return idx;
}

#define cpu_csr(idx) (cpu.csr[check_csr_idx(idx)])

#define CSR_TABLE                                                                                                      \
  CSR_TABLE_ENTRY(mstatus, 0x300)                                                                                      \
  CSR_TABLE_ENTRY(mtvec, 0x305)                                                                                        \
  CSR_TABLE_ENTRY(mepc, 0x341)                                                                                         \
  CSR_TABLE_ENTRY(mcause, 0x342)

// Name to Index
// CSR_mstatus = 0x300, ...
enum {
#define CSR_TABLE_ENTRY(name, idx) CSR_##name = idx,
  CSR_TABLE
#undef CSR_TABLE_ENTRY
};

// Index to Name
// csr_name(0x300) == "mstatus", ...
static inline const char *csr_name(int idx) {
  extern const char* csr_names[];
  const char *ret = csr_names[check_csr_idx(idx)];
  if (ret == NULL)
    ret = "unknown";
  return ret;
}

#endif
