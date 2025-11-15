#include "../riscv.h"
#include <am.h>
#include <klib-macros.h>
#include <klib.h>
#include <stddef.h>

#include <stdint.h>

extern char _heap_start;
extern char _heap_end;
extern char _data_lma;
extern char _sdata;
extern char _edata;
extern char _rodata_lma;
extern char _srodata;
extern char _erodata;
extern char _text_lma;
extern char _stext;
extern char _etext;
extern char _sbss;
extern char _ebss;

int main(const char* args);

#define UART_BASE 0x10000000L
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
    while (!(inb(UART_BASE + UART_LSR) & (1 << 5)))
    {
        // LSR Bit 5 - Transmit FIFO is empty.
        // ‘1’ – The transmitter FIFO is empty. Generates Transmitter
        //       Holding Register Empty interrupt. The bit is cleared when data is
        //       being been written to the transmitter FIFO.
        // ‘0’ – Otherwise
    }

    outb(UART_BASE + UART_THR, (uint8_t)ch);
}

__attribute__((noinline)) void halt(int code)
{
    asm volatile ("ebreak");
    while (1);
}

void init_memory()
{
#define INIT_SECTION(section_name) \
    size_t section_name##_size = (uintptr_t)&_e##section_name - (uintptr_t)&_s##section_name; \
    memcpy(&_s##section_name, &_##section_name##_lma, section_name##_size);

    // rodata, data and text
    INIT_SECTION(rodata)
    INIT_SECTION(data)
    INIT_SECTION(text)

#undef INIT_SECTION

    // bss
    unsigned int bss_size = (uintptr_t)&_ebss - (uintptr_t)&_sbss;
    memset(&_sbss, 0, bss_size);
}

// Initialize UART 16550
void init_uart16550()
{
    // 1. Set the Line Control Register to the desired line control parameters.
    //    Set bit 7 to ‘1’ to allow access to the Divisor Latches.
    uint8_t orignal_LCR = inb(UART_BASE + UART_LCR);
    outb(UART_BASE + UART_LCR, orignal_LCR | (1 << 7));

    // 2. Set the Divisor Latches, MSB first, LSB next.
    outb(UART_BASE + UART_DIVISOR_LATCH_2, 0x00);
    outb(UART_BASE + UART_DIVISOR_LATCH_1, 0x01);

    // 3. Set bit 7 of LCR to ‘0’ to disable access to Divisor Latches.
    //    At this time the transmission engine starts working and data can be sent and received.
    outb(UART_BASE + UART_LCR, orignal_LCR);

    // 4. Set the FIFO trigger level.
    //    Generally, higher trigger level values produce less interrupt to the system,
    //    so setting it to 14 bytes is recommended if the system responds fast enough.
    // The Reset Value is 11000000b, which defines FIFO trigger level as 14 bytes, so we
    // just skip this step.

    // 5. Enable desired interrupts by setting appropriate bits in the Interrupt Enable register.
    // The Reset Value is already 00h.
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
