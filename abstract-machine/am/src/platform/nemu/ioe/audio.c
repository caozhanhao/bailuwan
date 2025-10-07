#include <am.h>
#include <nemu.h>

#define AUDIO_FREQ_ADDR      (AUDIO_ADDR + 0x00)
#define AUDIO_CHANNELS_ADDR  (AUDIO_ADDR + 0x04)
#define AUDIO_SAMPLES_ADDR   (AUDIO_ADDR + 0x08)
#define AUDIO_SBUF_SIZE_ADDR (AUDIO_ADDR + 0x0c)
#define AUDIO_INIT_ADDR      (AUDIO_ADDR + 0x10)
#define AUDIO_WPTR_ADDR     (AUDIO_ADDR + 0x14)
#define AUDIO_RPTR_ADDR     (AUDIO_ADDR + 0x18)

void __am_audio_init() {
}

void __am_audio_config(AM_AUDIO_CONFIG_T *cfg) {
  cfg->present = false;
  cfg->bufsize = inl(AUDIO_SBUF_SIZE_ADDR);
}

void __am_audio_ctrl(AM_AUDIO_CTRL_T *ctrl) {
  outl(AUDIO_FREQ_ADDR, ctrl->freq);
  outl(AUDIO_CHANNELS_ADDR, ctrl->channels);
  outl(AUDIO_SAMPLES_ADDR, ctrl->samples);
  outl(AUDIO_INIT_ADDR, 1);
}

void __am_audio_status(AM_AUDIO_STATUS_T *stat) {
  uint32_t wptr = inl(AUDIO_WPTR_ADDR);
  uint32_t rptr = inl(AUDIO_RPTR_ADDR);
  uint32_t sbuf_size = inl(AUDIO_SBUF_SIZE_ADDR);
  uint32_t cnt = (wptr + sbuf_size - rptr) % sbuf_size;
  stat->count = cnt;
}

void __am_audio_play(AM_AUDIO_PLAY_T *ctl) {
  int len = ctl->buf.end - ctl->buf.start;
  int sbuf_size = inl(AUDIO_SBUF_SIZE_ADDR);

  // wait the buffer to be bigger enough
  while (inl(AUDIO_WPTR_ADDR) + len > sbuf_size)
    ;

  uint32_t wptr = inl(AUDIO_WPTR_ADDR);
  uint32_t first_chunk = sbuf_size - wptr;
  uint8_t *src = (uint8_t *)ctl->buf.start;

  if ((uint32_t)len <= first_chunk) {
    for (int i = 0; i < len; i++)
      outb(AUDIO_SBUF_ADDR + wptr + i, src[i]);
  } else {
    uint32_t n1 = first_chunk;
    uint32_t n2 = len - n1;
    for (uint32_t i = 0; i < n1; i++)
      outb(AUDIO_SBUF_ADDR + wptr + i, src[i]);

    for (uint32_t i = 0; i < n2; i++) {
      outb(AUDIO_SBUF_ADDR + i, src[n1 + i]);
    }
  }

  outl(AUDIO_WPTR_ADDR, (wptr + len) % sbuf_size);
}
