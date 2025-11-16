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
#include <utils.h>

/* http://en.wikibooks.org/wiki/Serial_Programming/8250_UART_Programming */
// NOTE: this is compatible to 16550

#define CH_OFFSET 0

static uint8_t *mrom_base = nullptr;
static uint8_t *sram_base = nullptr;
static uint8_t *flash_base = nullptr;
static uint8_t *sdram_base = nullptr;

void init_ysyxsoc() {
  mrom_base = new_space(CONFIG_MROM_SIZE);
  sram_base = new_space(CONFIG_SRAM_SIZE);
  flash_base = new_space(CONFIG_FLASH_SIZE);
  sdram_base = new_space(CONFIG_SDRAM_SIZE);

  // Don't use a handler to assert !write, because we need to init difftest.
  add_mmio_map("ysyxsoc_mrom", CONFIG_MROM_BASE, mrom_base, CONFIG_MROM_SIZE, nullptr);
  add_mmio_map("ysyxsoc_sram", CONFIG_SRAM_BASE, sram_base, CONFIG_SRAM_SIZE, nullptr);
  add_mmio_map("ysyxsoc_flash", CONFIG_FLASH_BASE, flash_base, CONFIG_FLASH_SIZE, nullptr);
  add_mmio_map("ysyxsoc_sdram", CONFIG_SDRAM_BASE, sdram_base, CONFIG_SDRAM_SIZE, nullptr);
}