#ifndef SDB_HPP
#define SDB_HPP

#include <cstdint>
#include <cstdio>
#include <cassert>

#include "utils/macro.hpp"
#include "../common/config.hpp"

#define word_t uint32_t
#define sword_t int32_t

#define FMT_WORD "0x%08" PRIx32

// CPU
void cpu_exec(uint64_t n);

// Difftest
void difftest_step();
void init_difftest(size_t img_size);

// ISA
void isa_reg_display();
void isa_csr_display();
word_t isa_reg_str2val(const char* s, bool* success);
word_t isa_csr_str2val(const char* s, bool* success);
int isa_ftrace_dump(char* buf, size_t buf_size);

// Expr
void init_regex();
bool syntax_check(char* e);
word_t expr(char* e, bool* success);


#define NR_WP 32
#define NR_BP 32

// Watch point
void init_wp_pool();
void wp_update();
void wp_display();
void wp_create(char* expr);
void wp_delete(int NO);

// Break point
void init_bp_pool();
void bp_update();
void bp_display();
void bp_create(word_t addr);
void bp_delete(int NO);


// FTrace
void init_ftrace(const char* elf_file);
const char* ftrace_search(uint32_t pc);
word_t ftrace_get_address_of(const char* name);

enum class SDBState
{
    Running, Stop, End, Quit, Abort
};

extern SDBState sdb_state;
extern int sdb_halt_ret;

#define Log(format, ...) \
    printf(ANSI_FMT("[%s:%d %s] " format, ANSI_FG_BLUE) "\n", \
        __FILE__, __LINE__, __func__, ## __VA_ARGS__)

#define Assert(cond, format, ...) \
  do { \
    if (!(cond)) { \
      fflush(stdout);\
      fprintf(stderr, ANSI_FMT(format, ANSI_FG_RED) "\n", ##  __VA_ARGS__); \
      sim_handle.cleanup(); \
      assert(cond); \
    } \
  } while (0)

#define panic(format, ...) Assert(0, format, ## __VA_ARGS__)
#endif
