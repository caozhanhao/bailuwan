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

#include "../../isa/riscv32/local-include/csr.h"

#include <cpu/cpu.h>
#include <difftest-def.h>
#include <isa.h>
#include <memory/paddr.h>

__EXPORT void difftest_memcpy(paddr_t addr, void *buf, size_t n, bool direction) {
  if (direction == DIFFTEST_TO_REF)
    copy_to_paddr(addr, buf, n);
  else
    copy_from_paddr(buf, addr, n);
}

struct diff_context_t {
  word_t gpr[32];
  word_t pc;
  word_t csr[4096];
};

__EXPORT void difftest_regcpy(void *dut, bool direction) {
  struct diff_context_t *ctx = (struct diff_context_t *)dut;
  if (direction == DIFFTEST_TO_REF) {
    cpu.pc = ctx->pc;
    for (int i = 0; i < 32; i++)
      cpu.gpr[i] = ctx->gpr[i];
    for (int i = 0; i < 4096; i++) {
      if (csr_name(i) != NULL)
        cpu.csr[i] = ctx->csr[i];
      else
        cpu.csr[i] = 0;
    }
  } else {
    ctx->pc = cpu.pc;
    for (int i = 0; i < 32; i++)
      ctx->gpr[i] = cpu.gpr[i];
    for (int i = 0; i < 4096; i++) {
      if (csr_name(i) != NULL)
        ctx->csr[i] = cpu.csr[i];
      else
        ctx->csr[i] = 0;
    }
  }
}

__EXPORT void difftest_exec(uint64_t n) { cpu_exec(n); }

__EXPORT void difftest_raise_intr(word_t NO) { assert(0); }

__EXPORT void difftest_init(int port) {
  void init_mem();
  init_mem();
  /* Perform ISA dependent initialization. */
  init_isa();

  void init_device();
  init_device();
}

struct cachesim_batch {
  // PC stream
  word_t* data;
  uint32_t size;
};

bool in_difftest_cachesim;
static uint32_t cachesim_batch_size;
__EXPORT void difftest_cachesim_init(uint32_t batch_size) {
  cachesim_batch_size = batch_size;
  in_difftest_cachesim = true;
}

// `batch_` should be a pointer to `cachesim_batch`, and the data field in it
// MUST allocate at least `cachesim_batch_size * sizeof(uint32_t)` bytes.
__EXPORT void difftest_cachesim_step(void* batch_) {
  struct cachesim_batch *batch = (struct cachesim_batch *)batch_;
  int i = 0;
  for (; i < cachesim_batch_size; i++) {
    batch->data[i] = cpu.pc;
    cpu_exec(1);

    if (nemu_state.state == NEMU_END || nemu_state.state == NEMU_ABORT || nemu_state.state == NEMU_QUIT)
      break;
  }
  batch->size = i;
}