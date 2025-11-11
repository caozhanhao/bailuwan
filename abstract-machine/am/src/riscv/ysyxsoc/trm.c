#include "../riscv.h"
#include <am.h>
#include <klib-macros.h>
#include <klib.h>

#include <stdint.h>

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
#define UART_THR 0 // w  8
#define UART_IER 1 // rw 8
#define UART_IIR 2 // r  8
#define UART_FCR 2 // w  8
#define UART_LCR 3 // rw 8
#define UART_MCR 4 // w  8
#define UART_LSR 5 // r  8
#define UART_MSR 6 // r  8
#define UART_DIVISOR_LATCH_1 0 // rw 8
#define UART_DIVISOR_LATCH_2 1 // rw 8

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

void init_memory()
{
    // Initialize .data section
    size_t data_size = (uintptr_t)&_edata - (uintptr_t)&_sdata;
    for (size_t i = 0; i < data_size; ++i)
        (&_sdata)[i] = (&_sidata)[i];

    // Initialize .bss section
    unsigned int bss_size = (uintptr_t)&_ebss - (uintptr_t)&_sbss;
    for (size_t i = 0; i < bss_size; ++i)
        (&_sbss)[i] = 0;
}

// Initialize UART 16550
void init_uart16550()
{
    // 1. Set the Line Control Register to the desired line control parameters.
    //    Set bit 7 to ‘1’ to allow access to the Divisor Latches.
    uint8_t orignal_LCR = inb(UART_BASE + UART_LCR);
    outb(UART_BASE + UART_LCR, orignal_LCR | (1 << 7));

    // 2. Set the Divisor Latches, MSB first, LSB next.
    outb(UART_BASE + UART_DIVISOR_LATCH_2, 0xf0);
    outb(UART_BASE + UART_DIVISOR_LATCH_1, 0x00);

    // 3. Set bit 7 of LCR to ‘0’ to disable access to Divisor Latches.
    //    At this time the transmission engine starts working and data can be sent and received.
    outb(UART_BASE + UART_LCR, orignal_LCR);

    // 4. Set the FIFO trigger level.
    //    Generally, higher trigger level values produce less interrupt to the system,
    //    so setting it to 14 bytes is recommended if the system responds fast enough.
    // The Reset Value is 11000000b, which defines FIFO trigger level as 14 bytes, so we
    // just skip this step.

    // 5. Enable desired interrupts by setting appropriate bits in the Interrupt Enable register.
    // We don't need any interrupts. and the Reset Value is already 00h.
}

// Print Welcome Message
// We don't use printf here to reduce code size.
void print_welcome_msg()
{
    putstr("[trm]: ");

    // mvendorid: ysyx
    uint32_t mvendorid;
    asm volatile ("csrr %0, mvendorid" : "=r"(mvendorid));
    for (int i = 0; i < 4; i++)
        putch((mvendorid >> ((3 - i) * 8)) & 0xFF);

    putch('_');

    // marchid: 25100251
    uint32_t marchid;
    asm volatile ("csrr %0, marchid" : "=r"(marchid));
    char buf[10];
    int i = 0;
    while (marchid)
    {
        buf[i++] = (char)('0' + (marchid % 10));
        marchid /= 10;
    }
    while (i-- > 0)
        putch(buf[i]);

    putstr(" caozhanhao\n");
}

void _trm_init()
{
    init_memory();
    init_uart16550();
    print_welcome_msg();
    int ret = main(mainargs);
    halt(ret);
}
