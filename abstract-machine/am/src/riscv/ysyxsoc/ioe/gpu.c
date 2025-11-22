#include "../uart.h"
#include "../../riscv.h"

#include <am.h>
#include <klib-macros.h>

#define VGA_WIDTH 640
#define VGA_HEIGHT 480
#define VGA_FB_ADDR 0x21000000

void __am_gpu_init() {
    int i;
    uint32_t *fb = (uint32_t *)(uintptr_t)VGA_FB_ADDR;
    for (i = 0; i < VGA_WIDTH * VGA_HEIGHT; i ++)
        fb[i] = i;
}

void __am_gpu_config(AM_GPU_CONFIG_T * cfg)
{
    *cfg = (AM_GPU_CONFIG_T) {
        .present = true, .has_accel = false,
        .width = VGA_WIDTH, .height = VGA_HEIGHT,
        .vmemsz = 0x200000 // 0x2100_0000~0x211f_ffff
      };
}

void __am_gpu_status(AM_GPU_STATUS_T *status)
{
    status->ready = true;
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *draw)
{
    uint32_t *fb = (uint32_t *)(uintptr_t)VGA_FB_ADDR;
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            fb[(draw->y + y) * VGA_WIDTH + draw->x + x] = ((uint32_t *)draw->pixels)[y * draw->w + x];
        }
    }
}

