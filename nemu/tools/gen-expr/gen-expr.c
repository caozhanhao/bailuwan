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

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// this should be enough
#define BUF_SIZE 65536

#define MAX_DEPTH 40
#define MAX_NUM 1000

static char* buf;
static size_t buf_size = BUF_SIZE;

static char *known_nonzero_exprs[BUF_SIZE] = {};
static size_t nonzero_exprs_size = 0;

static char* code_buf;
static size_t code_buf_size = BUF_SIZE + 128;
static char *code_format = "#include <stdio.h>\n"
                           "int main() { "
                           "  unsigned result = %s; "
                           "  printf(\"%%u\", result); "
                           "  return 0; "
                           "}";

static size_t pos = 0;

static void write_char(char c) {
  if (pos >= buf_size) {
    fprintf(stderr, "Reallocating buffer: %zu\n", buf_size * 2);
    buf = realloc(buf, buf_size * 2);
    buf_size *= 2;
  }

  buf[pos++] = c;
  buf[pos] = '\0';
}

static void write_str(const char *s) {
  while (*s) {
    if (pos >= buf_size) {
      fprintf(stderr, "Reallocating buffer: %zu\n", buf_size * 2);
      buf = realloc(buf, buf_size * 2);
      buf_size *= 2;
    }

    buf[pos++] = *s++;
  }
  buf[pos] = '\0';
}

static int choose(int n) {
  if (n <= 0)
    return 0;
  return rand() % n;
}

static void gen_num() {
  int num = rand() % MAX_NUM;

  char tmp[32];
  int n = snprintf(tmp, sizeof(tmp), "%du", num);
  assert(n > 0);

  write_str(tmp);
}

static void gen_non_zero_num() {
  int num = rand() % MAX_NUM;

  while (num == 0)
    num = rand() % MAX_NUM;

  char tmp[32];
  int n = snprintf(tmp, sizeof(tmp), "%d", num);
  assert(n > 0);

  write_str(tmp);
}

static char gen_op() {
  char ch;
  switch (choose(4)) {
  case 0:
    ch = '+';
    break;
  case 1:
    ch = '-';
    break;
  case 2:
    ch = '*';
    break;
  default:
    ch = '/';
    break;
  }
  write_char(ch);
  return ch;
}

static void gen_non_zero_expr() {
  if (nonzero_exprs_size > 0)
    write_str(known_nonzero_exprs[choose(nonzero_exprs_size)]);
  else
    gen_non_zero_num();
}

void gen_space() {
  if (choose(2)) {
    if (choose(3))
      write_char('\t');
    else
      write_char(' ');
  }
}

static void gen_rand_expr(int depth) {
  if (depth >= MAX_DEPTH) {
    gen_num();
    return;
  }

  gen_space();

  int r = choose(100);
  if (r < 30)
    gen_num();
  else if (r < 70)
    gen_rand_expr(depth + 1);
  else {
    write_char('(');

    gen_rand_expr(depth + 1);
    gen_space();
    if (gen_op() == '/')
      gen_non_zero_expr();
    else
      gen_rand_expr(depth + 1);

    gen_space();
    write_char(')');
  }
}

int main(int argc, char *argv[]) {
  buf = malloc(buf_size);
  code_buf = malloc(code_buf_size);

  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i++) {
    gen_rand_expr(0);

    if (code_buf_size < buf_size + 128) {
      fprintf(stderr, "Reallocating code_buf: %zu\n", buf_size + 256);
      code_buf = realloc(code_buf, buf_size + 256);
      code_buf_size = buf_size + 256;
    }

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc -Werror /tmp/.code.c -o /tmp/.expr");
    if (ret != 0)
      continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);

    if (result != 0 && nonzero_exprs_size < BUF_SIZE)
      known_nonzero_exprs[nonzero_exprs_size++] = strdup(buf);

    pos = 0;
    buf[0] = '\0';
  }

  for (i = 0; i < nonzero_exprs_size; i++)
    free(known_nonzero_exprs[i]);

  free(buf);

  return 0;
}
