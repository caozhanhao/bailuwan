#include "../riscv.h"
#include "uart.h"
#include <am.h>
#include <klib-macros.h>
#include <klib.h>
#include <stddef.h>

#include <stdint.h>

extern char _heap_start;
extern char _heap_end;

int main(const char* args);

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

// Initialize UART 16550
void init_uart16550()
{
    // 1. Set the Line Control Register to the desired line control parameters.
    //    Set bit 7 to ‘1’ to allow access to the Divisor Latches.
    uint8_t orignal_LCR = inb(UART_BASE + UART_LCR);
    outb(UART_BASE + UART_LCR, orignal_LCR | (1 << 7));

    // 2. Set the Divisor Latches, MSB first, LSB next.
    //    The value set should be equal to (system clock speed) / (16 x desired baud rate).
    //
    // Note for NVBoard:
    //   Quoted from ysyx v24.07 Study Handouts B2 SoC Computer System:
    //     "As for the baud rate, because there is no concept of clock frequency in NVBoard,
    //     it is described using a divisor, meaning the number of cycles needed to maintain
    //     one bit during data transmission."
    //   In other words, NVBoard divisor = the number of cycles per bit
    //   Since one bit lasts `1 / baud` seconds and one clock cycle lasts `1 / clk_freq` seconds,
    //   the number of cycles per bit is:
    //       (1 / baud) / (1 / clk_freq) = clk_freq / baud
    //   Therefore:
    //       NVBoard divisor = clk_freq / baud
    //   For UART16550, the Divisor Latch value (DL) should be set to `clk_freq / 16*baud`.
    //   Hence:
    //       NVBoard divisor = 16 * DL
    //   The default divisor in NVBoard is 16, so we set DL to 1 here.
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

    // NVBoard 7-Segment
    *(volatile uint32_t *)(0x10002000 + 0x8) = 0x25100251;
}

void _trm_init()
{
    init_uart16550();
    print_welcome_msg();
    int ret = main(mainargs);
    halt(ret);
}
