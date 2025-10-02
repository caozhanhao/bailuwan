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

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  char *expr;
  word_t last_val;
  bool last_val_valid;
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

WP *new_wp() {
  Assert(free_, "No more watchpoints available");
  WP *p = free_;
  free_ = free_->next;
  p->next = head;
  head = p;
  return p;
}

void free_wp(WP *wp) {
  Assert(wp, "Watchpoint is NULL");

  free(wp->expr);

  if (wp == head) {
    head = head->next;
    wp->next = free_;
    free_ = wp;
    return;
  }

  for (WP *p = head; p != NULL; p = p->next) {
    if (p->next == wp) {
      p->next = wp->next;
      wp->next = free_;
      free_ = wp;
      return;
    }
  }

  panic("Watchpoint not found");
}

void wp_update_one(WP *p) {
  if (!p->last_val_valid) {
    bool success;
    p->last_val = expr(p->expr, &success);
    p->last_val_valid = success;
    if (!success)
      Log("Failed to evaluate '%s' for watchpoint %d.", p->expr, p->NO);
    return;
  }

  bool success;
  word_t curr_val = expr(p->expr, &success);
  if (!success) {
    Log("Failed to evaluate '%s' for watchpoint %d.", p->expr, p->NO);
    return;
  }

  if (p->last_val != curr_val) {
    Log("Watchpoint %d: %s changed from %x to %x", p->NO, p->expr, p->last_val,
        curr_val);
    p->last_val = curr_val;
    nemu_state.state = NEMU_STOP;
  }
}

void wp_update() {
  for (WP *p = head; p != NULL; p = p->next)
    wp_update_one(p);
}

static int wp_no_cmp(const void* a, const void* b) {
  return ((WP*)a)->NO - ((WP*)b)->NO;
}

void wp_display() {
  // Sort to make the order pretty
  WP* buffer[NR_WP] = {};
  int buffer_pos = 0;
  for (WP *p = head; p != NULL; p = p->next)
    buffer[buffer_pos++] = p;

  qsort(buffer, buffer_pos, sizeof(WP*), wp_no_cmp);

  printf("%-6s %-15s %s\n", "Num", "LastValue", "What");
  for (int i = 0; i < buffer_pos; i++) {
    WP *p = buffer[i];
    if (p->last_val_valid)
      printf("%-6d 0x%-13x %s\n", p->NO, p->last_val, p->expr);
    else
      printf("%-6d %-15s %s\n", p->NO, "Not evaluated", p->expr);
  }
}

void wp_create(char *expr) {
  WP *p = new_wp();
  p->expr = strdup(expr);
  p->last_val_valid = false;
  p->last_val = 0;
  wp_update_one(p);
}

void wp_delete(int NO) { free_wp(&wp_pool[NO]); }
