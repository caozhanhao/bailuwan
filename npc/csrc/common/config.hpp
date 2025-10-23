#ifndef CONFIG_HPP
#define CONFIG_HPP

#define RTC_MMIO 0xa0000048
#define SERIAL_PORT_MMIO 0x10000000

#define CONFIG_MBASE 0x80000000
#define CONFIG_MSIZE 0x8000000

#define CONFIG_MTRACE 1

// Only available in sdb
#define CONFIG_ITRACE 1
// #define CONFIG_FTRACE 1
#define CONFIG_DIFFTEST 1

#endif
