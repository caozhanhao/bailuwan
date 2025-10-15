/***************************************************************************************
 * Copyright (c) 2014-2024 Zihao Yu, Nanjing University
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan
 *PSL v2. You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 *KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 *NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

#include "local-include/csr.h"
#include <isa.h>
#include <stdlib.h>

#define CSR_TABLE_ENTRY(name, idx) [idx] = #name,
const char *csr_names[4096] = {CSR_TABLE};
#undef CSR_TABLE_ENTRY

void isa_csr_display() {
  for (int i = 0; i < 4096; i++) {
    if (csr_names[i] == NULL)
      continue;
    printf("%-10s  0x%08x  %11d\n", csr_names[i], cpu_csr(i), cpu_csr(i));
  }
}

word_t isa_csr_str2val(const char *s, bool *success) {
  for (int i = 0; i < 4096; i++) {
    if (csr_names[i] == NULL)
      continue;

    if (strcmp(s, csr_names[i]) == 0) {
      *success = true;
      return cpu_csr(i);
    }
  }
  *success = false;
  return 0;
}
