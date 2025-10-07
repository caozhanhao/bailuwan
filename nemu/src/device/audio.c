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

#include <common.h>
#include <device/map.h>
#include <SDL2/SDL.h>

enum {
  reg_freq = 0,
  reg_channels,
  reg_samples,
  reg_sbuf_size,
  reg_init,
  reg_count,
  reg_pos,
  nr_reg
};

static uint8_t *sbuf = NULL;
static uint32_t *audio_base = NULL;

static void sdl_audio_callback(void *userdata, uint8_t *stream, int len) {
  // We have `audio_base[reg_count]` bytes of data to play. But SDL allows
  // us to write at most `len` bytes.
  int written = (len < audio_base[reg_count]) ? len : audio_base[reg_count];

  // Resume from the previous position to play
  memcpy(stream, sbuf + audio_base[reg_pos], written);
  memset(stream + written, 0, len - written);

  // Advance pos for next play
  audio_base[reg_pos] += written;
  audio_base[reg_count] -= written;

  // Reset pos if we have played all data in sbuf
  if (audio_base[reg_pos] >= CONFIG_SB_SIZE) {
    audio_base[reg_pos] = 0;
    audio_base[reg_count] = 0;
  }
}

static void audio_io_handler(uint32_t offset, int len, bool is_write) {
  switch (offset) {
  case reg_freq << 2:
  case reg_channels << 2:
  case reg_samples << 2:
  case reg_count << 2:
    // pass
    break;

  case reg_sbuf_size << 2:
  case reg_pos << 2:
    Assert(!is_write, "write to read-only register.");
    break;

  case reg_init << 2: {
    SDL_AudioSpec s = {};

    s.format = AUDIO_S16SYS;  // 假设系统中音频数据的格式总是使用16位有符号数来表示
    s.userdata = NULL;        // 不使用
    s.freq = audio_base[reg_freq];
    s.channels = audio_base[reg_channels];
    s.samples = audio_base[reg_samples];
    s.callback = sdl_audio_callback;

    SDL_InitSubSystem(SDL_INIT_AUDIO);
    SDL_OpenAudio(&s, NULL);
    SDL_PauseAudio(0);
  }
    break;
  default:
    panic("audio: unknown offset = 0x%x", offset);
  }
}

void init_audio() {
  uint32_t space_size = sizeof(uint32_t) * nr_reg;
  audio_base = (uint32_t *)new_space(space_size);
  memset(audio_base, 0, space_size);
  audio_base[reg_sbuf_size] = CONFIG_SB_SIZE;

#ifdef CONFIG_HAS_PORT_IO
  add_pio_map ("audio", CONFIG_AUDIO_CTL_PORT, audio_base, space_size, audio_io_handler);
#else
  add_mmio_map("audio", CONFIG_AUDIO_CTL_MMIO, audio_base, space_size, audio_io_handler);
#endif

  sbuf = (uint8_t *)new_space(CONFIG_SB_SIZE);
  add_mmio_map("audio-sbuf", CONFIG_SB_ADDR, sbuf, CONFIG_SB_SIZE, NULL);
}
