#ifndef SDB_HPP
#define SDB_HPP

#include <cstdint>
#include <cstdio>
#include <cassert>
#include "utils/macro.hpp"

#define word_t uint32_t
#define sword_t int32_t

#define FMT_WORDx "0x%08" PRIx32

// CPU
void cpu_exec(uint64_t n);

// ISA
void isa_reg_display();
word_t isa_reg_str2val(const char *s, bool *success);

// Expr
void init_regex();
bool syntax_check(char* e);
word_t expr(char *e, bool *success);

// Watch point
void init_wp_pool();
void wp_update();
void wp_display();
void wp_create(char* expr);
void wp_delete(int NO);

enum class SDBState
{
    Running, Stop, End, Quit, Abort
};

SDBState sdb_state;

#define Log(format, ...) \
    printf(ANSI_FMT("[%s:%d %s] " format, ANSI_FG_BLUE) "\n", \
        __FILE__, __LINE__, __func__, ## __VA_ARGS__)

#define Assert(cond, format, ...) \
  do { \
    if (!(cond)) { \
      fflush(stdout);\
      fprintf(stderr, ANSI_FMT(format, ANSI_FG_RED) "\n", ##  __VA_ARGS__); \
      assert(cond); \
    } \
  } while (0)

#define panic(format, ...) Assert(0, format, ## __VA_ARGS__)

#endif
