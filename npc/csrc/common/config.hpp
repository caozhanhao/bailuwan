#ifndef CONFIG_HPP
#define CONFIG_HPP

#define RTC_MMIO 0xa0000048
#define SERIAL_PORT_MMIO 0x10000000

#define CONFIG_MROM_BASE 0x20000000
#define CONFIG_MROM_SIZE 0x1000
#define CONFIG_SRAM_BASE 0x0f000000
#define CONFIG_SRAM_SIZE 0x01000000
#define CONFIG_FLASH_BASE 0x30000000

// FLASH's address space is 0x10000000 bytes, but only low 24-bit in address
// is used in APBSPI, so access to address that can't be contained in 24-bit should also be
// considered invalid. Thus, we set the FLASH_SIZE as 0x1000000(= 2^24).
#define CONFIG_FLASH_SIZE 0x1000000

#define CONFIG_PSRAM_BASE 0x80000000
#define CONFIG_PSRAM_SIZE 0x1000000 // Same as flash

#define CONFIG_SDRAM_BASE 0xa0000000
// 4 bank * 8192 row * 512 col * 16 bit = 32 MB
#define CONFIG_SDRAM_CHIP_SIZE 0x2000000
// 2 chip -> 64 MB
#define CONFIG_SDRAM_SIZE (CONFIG_SDRAM_CHIP_SIZE * 4)

#define PMEM_LEFT  ((uint32_t)CONFIG_FLASH_BASE)
#define PMEM_RIGHT ((uint32_t)CONFIG_FLASH_BASE + CONFIG_FLASH_SIZE - 1)
#define RESET_VECTOR PMEM_LEFT

// Available in sdb/fast/nvboard
// #define CONFIG_MTRACE 1

// Only available in sdb:
// #define CONFIG_ITRACE 1
// #define CONFIG_FTRACE 1
#define CONFIG_WP_BP
#define CONFIG_DIFFTEST 1

#endif
