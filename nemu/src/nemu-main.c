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

#include "monitor/sdb/sdb.h"

#include <common.h>
void init_monitor(int, char *[]);
void am_init_monitor();
void engine_start();
int is_exit_status_bad();

int test_expr_eval(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s input_file\n", argv[0]);
    return 1;
  }

  FILE *fp = fopen(argv[1], "r");
  if (fp == NULL) {
    perror("fopen");
    return 1;
  }

  char *line = NULL;
  size_t len = 0;
  ssize_t nread;

  int total = 0, passed = 0;

  while ((nread = getline(&line, &len, fp)) != -1) {
    if (nread <= 1) continue;

    char *p = strchr(line, ' ');
    if (p == NULL) continue;
    char* expr_str = p + 1;
    *p = '\0';

    char* endptr;
    unsigned expected = strtoul(line, &endptr, 10);
    Assert(line != endptr, "invalid number");

    printf("Testing: %s\n", expr_str);

    bool success = false;
    word_t got = expr(expr_str, &success);

    total++;
    if (success && got == expected) {
      passed++;
    } else {
      printf("[FAILED] expr: %s\n", expr_str);
      printf("  expected: %u, got: %u\n", expected, got);
    }
  }

  printf("Test finished: %d/%d passed.\n", passed, total);

  free(line);
  fclose(fp);
  return 0;
}

int main(int argc, char *argv[]) {
//   /* Initialize the monitor. */
// #ifdef CONFIG_TARGET_AM
//   am_init_monitor();
// #else
//   init_monitor(argc, argv);
// #endif
//
//   /* Start engine. */
//   engine_start();
//
//   return is_exit_status_bad();

  init_monitor(argc, argv);
  return test_expr_eval(argc, argv);
}