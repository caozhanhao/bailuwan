#include <am.h>
#include <klib-macros.h>
#include <klib.h>

extern char _heap_start;
extern char _heap_end;
extern char _sdata;
extern char _sidata;
extern char _edata;
extern char _sbss;
extern char _ebss;

int main(const char* args);

#define UART_BASE 0x10000000L
#define UART_TX   0

Area heap = RANGE(&_heap_start, &_heap_end);
static const char mainargs[MAINARGS_MAX_LEN] = TOSTRING(MAINARGS_PLACEHOLDER); // defined in CFLAGS

void putch(char ch)
{
    *(volatile char*)(UART_BASE + UART_TX) = ch;
}

__attribute__((noinline)) void halt(int code)
{
    asm volatile ("ebreak");
    while (1);
}

void _trm_init()
{
    size_t data_size = (uintptr_t) & _edata - (uintptr_t) & _sdata;
    for (size_t i = 0; i < data_size; ++i)
        (&_sdata)[i] = (&_sidata)[i];

    unsigned int bss_size = (uintptr_t) & _ebss - (uintptr_t) & _sbss;
    for (size_t i = 0; i < bss_size; ++i)
        (&_sbss)[i] = 0;


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
