#include <am.h>
#include <klib-macros.h>
#include <klib.h>

extern char _heap_start;
int main(const char *args);

extern char _pmem_start;
#define PMEM_SIZE (128 * 1024 * 1024)
#define PMEM_END  ((uintptr_t)&_pmem_start + PMEM_SIZE)
#define SERIAL_PORT_MMIO 0x10000000

Area heap = RANGE(&_heap_start, PMEM_END);
static const char mainargs[MAINARGS_MAX_LEN] = TOSTRING(MAINARGS_PLACEHOLDER); // defined in CFLAGS

void putch(char ch) {
  *(volatile uint8_t*)SERIAL_PORT_MMIO = ch;
}

void halt(int code) {
  asm volatile ("ebreak");
  while (1);
}

void _trm_init() {
  uint32_t mvendorid, marchid;
  asm volatile ("csrr %0, mvendorid" : "=r"(mvendorid));
  asm volatile ("csrr %0, marchid" : "=r"(marchid));

  char buf[5];
  buf[0] = (char)((mvendorid >> 24) & 0xFF);
  buf[1] = (char)((mvendorid >> 16) & 0xFF);
  buf[2] = (char)((mvendorid >>  8) & 0xFF);
  buf[3] = (char)( mvendorid        & 0xFF);
  buf[4] = '\0';

  printf("[_trm_init]: %s_%d caozhanhao\n", buf, marchid);

  int ret = main(mainargs);
  halt(ret);
}
