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

#include "isa.h"
#include "memory/paddr.h"
#include "sdb.h"

typedef struct breakpoint {
  int NO;
  int pool_index;
  struct breakpoint *next;

  word_t addr;
} BP;

static BP bp_pool[NR_BP] = {};
static BP *head = NULL, *free_ = NULL;

void init_bp_pool() {
  int i;
  for (i = 0; i < NR_BP; i++) {
    // To avoid conflict with watch points, the NO of breakpoints starts from NR_WP.
    bp_pool[i].NO = NR_WP + i;
    bp_pool[i].next = (i == NR_BP - 1 ? NULL : &bp_pool[i + 1]);
  }

  head = NULL;
  free_ = bp_pool;
}

BP *new_bp() {
  Assert(free_, "No more breakpoints available");
  BP *p = free_;
  free_ = free_->next;
  p->next = head;
  head = p;
  return p;
}

void free_bp(BP *bp) {
  Assert(bp, "Breakpoint is NULL");

  if (bp == head) {
    head = head->next;
    bp->next = free_;
    free_ = bp;
    return;
  }

  for (BP *p = head; p != NULL; p = p->next) {
    if (p->next == bp) {
      p->next = bp->next;
      bp->next = free_;
      free_ = bp;
      return;
    }
  }

  panic("Breakpoint not found");
}

void bp_update_one(BP *p) {
  if (cpu.pc == p->addr) {
    Log("Breakpoint hit at 0x%x.", p->addr);

    if (nemu_state.state == NEMU_RUNNING)
      nemu_state.state = NEMU_STOP;
  }
}

void bp_update() {
  for (BP *p = head; p != NULL; p = p->next)
    bp_update_one(p);
}

const char* ftrace_search(uint32_t pc);
void bp_display() {
  BP* buffer[NR_BP] = {};
  int buffer_pos = 0;
  for (BP *p = head; p != NULL; p = p->next)
    buffer[buffer_pos++] = p;

  printf("%-6s %-10s %s\n", "Num", "Func", "What");
  for (int i = buffer_pos - 1; i >= 0; --i) {
    BP *p = buffer[i];

    const char* func = ftrace_search(p->addr);
    printf("%-6d %-10s " FMT_PADDR "\n", p->NO, func ? func : "???", p->addr);
  }
}

void bp_create(word_t addr) {
  if (!in_pmem(addr)) {
    printf("Bad address to watch: " FMT_WORD "\n", addr);
    return;
  }

  BP *p = new_bp();
  p->addr = addr;

  const char* func = ftrace_search(addr);
  printf("Breakpoint %d: " FMT_WORD " (func: @%s)\n", p->NO, addr, func ? func : "???");
  bp_update_one(p);
}

void bp_delete(int NO) { free_bp(&bp_pool[NO]); }
