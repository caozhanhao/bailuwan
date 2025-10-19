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

#include "dut_proxy.hpp"
#include "sdb.hpp"

typedef struct breakpoint {
  int NO;
  int pool_index;
  struct breakpoint *next;

  word_t addr;
} BP;

static BP bp_pool[NR_BP] = {};
static BP *head = nullptr, *free_ = nullptr;

void init_bp_pool() {
  int i;
  for (i = 0; i < NR_BP; i++) {
    // To avoid conflict with watch points, the NO of breakpoints starts from NR_WP.
    bp_pool[i].NO = NR_WP + i;
    bp_pool[i].next = (i == NR_BP - 1 ? nullptr : &bp_pool[i + 1]);
  }

  head = nullptr;
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
  Assert(bp, "Breakpoint is nullptr");

  if (bp == head) {
    head = head->next;
    bp->next = free_;
    free_ = bp;
    return;
  }

  for (BP *p = head; p != nullptr; p = p->next) {
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
  if (sim_handle.get_cpu().pc() == p->addr) {
    Log("Breakpoint hit at 0x%x.", p->addr);

    if (sdb_state == SDBState::Running)
      sdb_state = SDBState::Stop;
  }
}

void bp_update() {
  for (BP *p = head; p != nullptr; p = p->next)
    bp_update_one(p);
}

const char* ftrace_search(uint32_t pc);
void bp_display() {
  BP* buffer[NR_BP] = {};
  int buffer_pos = 0;
  for (BP *p = head; p != nullptr; p = p->next)
    buffer[buffer_pos++] = p;

  printf("%-6s %-10s %s\n", "Num", "Func", "What");
  for (int i = buffer_pos - 1; i >= 0; --i) {
    BP *p = buffer[i];

    const char* func = ftrace_search(p->addr);
    printf("%-6d %-10s " FMT_WORD "\n", p->NO, func ? func : "???", p->addr);
  }
}

void bp_create(word_t addr) {
  if (!sim_handle.get_memory().in_pmem(addr)) {
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
