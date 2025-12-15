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
#include <memory/vaddr.h>

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

struct tracesim_batch {
  // PC stream
  uint32_t *i_stream;
  uint32_t i_size;

  // ldstr stream
  struct dcache_entry {
    bool is_read;
    uint32_t addr;
  } *d_stream;

  uint32_t d_size;

  // branch stream
  struct branch_entry {
    uint32_t pc;
    uint32_t target;
    bool is_uncond;
    bool taken;
  } *b_stream;

  uint32_t b_size;
};

bool in_difftest_tracesim;
static uint32_t tracesim_batch_size;
__EXPORT void difftest_tracesim_init(uint32_t batch_size) {
  tracesim_batch_size = batch_size;
  in_difftest_tracesim = true;
}

// `batch_` should be a pointer to `tracesim_batch`, and the `stream` field in it
// MUST allocate at least `tracesim_batch_size * sizeof(uint32_t/dcache_entry)` bytes.
__EXPORT void difftest_tracesim_step(void *batch_) {
  struct tracesim_batch *batch = (struct tracesim_batch *)batch_;
  int i = 0;
  int d = 0;
  int b = 0;
  while (i < tracesim_batch_size) {
    batch->i_stream[i++] = cpu.pc;

    auto inst = vaddr_ifetch(cpu.pc, 4);

    bool is_read;
    word_t addr;
    if (isa_decode_ldstr(inst, &is_read, &addr) == 0) {
      batch->d_stream[d].is_read = is_read;
      batch->d_stream[d].addr = addr;
      d++;
    }

    word_t target_addr;
    bool is_uncond;
    bool taken;
    if (isa_decode_branch(cpu.pc, inst, &target_addr, &is_uncond, &taken) == 0) {
      batch->b_stream[i].pc = cpu.pc;
      batch->b_stream[i].target = target_addr;
      batch->b_stream[i].is_uncond = is_uncond;
      batch->b_stream[i].taken = taken;
      b++;
      printf("1NEMU pc: 0x%x\n", batch->b_stream[i].pc);
    }

    cpu_exec(1);

    if (nemu_state.state == NEMU_END || nemu_state.state == NEMU_ABORT || nemu_state.state == NEMU_QUIT)
      break;
  }
  batch->i_size = i;
  batch->d_size = d;
  batch->b_size = b;

  for (i = 0; i < b; i++)
    printf("NEMU pc: 0x%x\n", batch->b_stream[i].pc);
}