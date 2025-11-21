#ifndef AM_YSYXSOC_UART_H
#define AM_YSYXSOC_UART_H

#define UART_BASE 0x10000000L
#define UART_RXBUF 0 // r  8
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

#endif