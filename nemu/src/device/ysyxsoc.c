/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <device/map.h>

static uint8_t *mrom_base = nullptr;
static uint8_t *sram_base = nullptr;
static uint8_t *flash_base = nullptr;
static uint8_t *psram_base = nullptr;
static uint8_t *sdram_base = nullptr;
static uint8_t *uart_base = nullptr;

#define UART_LSR 5
// We simulate LSR to avoid CacheSim stuck at `putch`.
static void uart_io_handler(uint32_t offset, int len, bool is_write) {
  assert(len == 1);
  switch (offset) {
  case UART_LSR:
    // LSR Bit 5 - Transmit FIFO is empty.
    // ‘1’ – The transmitter FIFO is empty. Generates Transmitter
    //       Holding Register Empty interrupt. The bit is cleared when data is
    //       being been written to the transmitter FIFO.
    // ‘0’ – Otherwise
    uart_base[UART_LSR] = 1 << 5;
    break;
  default:
    // just pass
    break;
  }
}

static void psram_handler(uint32_t offset, int len, bool is_write) {
  printf("psram offset=0x%x, len=%d, is_write=%d\n", offset, len, is_write);
}

void init_ysyxsoc() {
  mrom_base = new_space(CONFIG_MROM_SIZE);
  sram_base = new_space(CONFIG_SRAM_SIZE);
  flash_base = new_space(CONFIG_FLASH_SIZE);
  psram_base = new_space(CONFIG_PSRAM_SIZE);
  sdram_base = new_space(CONFIG_SDRAM_SIZE);
  uart_base = new_space(CONFIG_UART_SIZE);

  // Don't use a handler to assert !write, because we need to init difftest.
  add_mmio_map("ysyxsoc_mrom", CONFIG_MROM_BASE, mrom_base, CONFIG_MROM_SIZE, nullptr);
  add_mmio_map("ysyxsoc_sram", CONFIG_SRAM_BASE, sram_base, CONFIG_SRAM_SIZE, nullptr);
  add_mmio_map("ysyxsoc_flash", CONFIG_FLASH_BASE, flash_base, CONFIG_FLASH_SIZE, nullptr);
  add_mmio_map("ysyxsoc_psram", CONFIG_PSRAM_BASE, psram_base, CONFIG_PSRAM_SIZE, psram_handler);
  add_mmio_map("ysyxsoc_sdram", CONFIG_SDRAM_BASE, sdram_base, CONFIG_SDRAM_SIZE, nullptr);
  add_mmio_map("ysyxsoc_uart", CONFIG_UART_BASE, uart_base, CONFIG_UART_SIZE, uart_io_handler);
}