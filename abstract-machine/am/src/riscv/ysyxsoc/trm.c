#include <am.h>
#include <klib-macros.h>
#include <klib.h>

extern char _heap_start;
extern char _heap_end;
int main(const char *args);

#define UART_BASE 0x10000000L
#define UART_TX   0

Area heap = RANGE(&_heap_start, &_heap_end);
static const char mainargs[MAINARGS_MAX_LEN] = TOSTRING(MAINARGS_PLACEHOLDER); // defined in CFLAGS

void putch(char ch) {
  *(volatile char *)(UART_BASE + UART_TX) = ch;
}

__attribute__((noinline)) void halt(int code) {
  asm volatile ("ebreak");
  while (1);
}

void _trm_init() {
  // uint32_t mvendorid, marchid;
  // asm volatile ("csrr %0, mvendorid" : "=r"(mvendorid));
  // asm volatile ("csrr %0, marchid" : "=r"(marchid));

  // char buf[5];
  // buf[0] = (char)((mvendorid >> 24) & 0xFF);
  // buf[1] = (char)((mvendorid >> 16) & 0xFF);
  // buf[2] = (char)((mvendorid >>  8) & 0xFF);
  // buf[3] = (char)( mvendorid        & 0xFF);
  // buf[4] = '\0';

  // printf("[_trm_init]: %s_%d caozhanhao\n", buf, marchid);

  int ret = main(mainargs);
  halt(ret);
}
