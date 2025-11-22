#ifndef __AMDEV_H__
#define __AMDEV_H__

// **MAY SUBJECT TO CHANGE IN THE FUTURE**

#define AM_DEVREG(id, reg, perm, ...) \
  enum { AM_##reg = (id) }; \
  typedef struct { __VA_ARGS__; } AM_##reg##_T;

AM_DEVREG( 1, UART_CONFIG,  RD, bool present);
AM_DEVREG( 2, UART_TX,      WR, char data);
AM_DEVREG( 3, UART_RX,      RD, char data);
AM_DEVREG( 4, TIMER_CONFIG, RD, bool present, has_rtc);
AM_DEVREG( 5, TIMER_RTC,    RD, int year, month, day, hour, minute, second);
AM_DEVREG( 6, TIMER_UPTIME, RD, uint64_t us);
AM_DEVREG( 7, INPUT_CONFIG, RD, bool present);
AM_DEVREG( 8, INPUT_KEYBRD, RD, bool keydown; int keycode);
AM_DEVREG( 9, GPU_CONFIG,   RD, bool present, has_accel; int width, height, vmemsz);
AM_DEVREG(10, GPU_STATUS,   RD, bool ready);
AM_DEVREG(11, GPU_FBDRAW,   WR, int x, y; void *pixels; int w, h; bool sync);
AM_DEVREG(12, GPU_MEMCPY,   WR, uint32_t dest; void *src; int size);
AM_DEVREG(13, GPU_RENDER,   WR, uint32_t root);
AM_DEVREG(14, AUDIO_CONFIG, RD, bool present; int bufsize);
AM_DEVREG(15, AUDIO_CTRL,   WR, int freq, channels, samples);
AM_DEVREG(16, AUDIO_STATUS, RD, int count);
AM_DEVREG(17, AUDIO_PLAY,   WR, Area buf);
AM_DEVREG(18, DISK_CONFIG,  RD, bool present; int blksz, blkcnt);
AM_DEVREG(19, DISK_STATUS,  RD, bool ready);
AM_DEVREG(20, DISK_BLKIO,   WR, bool write; void *buf; int blkno, blkcnt);
AM_DEVREG(21, NET_CONFIG,   RD, bool present);
AM_DEVREG(22, NET_STATUS,   RD, int rx_len, tx_len);
AM_DEVREG(23, NET_TX,       WR, Area buf);
AM_DEVREG(24, NET_RX,       WR, Area buf);

#define AM_KEY_TABLE(NORMAL, EXT, arg) \
  NORMAL(ESCAPE, 0x76, arg) \
  NORMAL(F1, 0x5, arg) \
  NORMAL(F2, 0x6, arg) \
  NORMAL(F3, 0x4, arg) \
  NORMAL(F4, 0x0C, arg) \
  NORMAL(F5, 0x3, arg) \
  NORMAL(F6, 0x0B, arg) \
  NORMAL(F7, 0x83, arg) \
  NORMAL(F8, 0x0A, arg) \
  NORMAL(F9, 0x1, arg) \
  NORMAL(F10, 0x9, arg) \
  NORMAL(F11, 0x78, arg) \
  NORMAL(F12, 0x7, arg) \
  NORMAL(GRAVE, 0x0E, arg) \
  NORMAL(1, 0x16, arg) \
  NORMAL(2, 0x1E, arg) \
  NORMAL(3, 0x26, arg) \
  NORMAL(4, 0x25, arg) \
  NORMAL(5, 0x2E, arg) \
  NORMAL(6, 0x36, arg) \
  NORMAL(7, 0x3D, arg) \
  NORMAL(8, 0x3E, arg) \
  NORMAL(9, 0x46, arg) \
  NORMAL(0, 0x45, arg) \
  NORMAL(MINUS, 0x4E, arg) \
  NORMAL(EQUALS, 0x55, arg) \
  NORMAL(BACKSPACE, 0x66, arg) \
  NORMAL(TAB, 0x0D, arg) \
  NORMAL(LEFTBRACKET, 0x54, arg) \
  NORMAL(RIGHTBRACKET, 0x5B, arg) \
  NORMAL(BACKSLASH, 0x5D, arg) \
  NORMAL(CAPSLOCK, 0x58, arg) \
  NORMAL(SEMICOLON, 0x4C, arg) \
  NORMAL(APOSTROPHE, 0x52, arg) \
  NORMAL(RETURN, 0x5A, arg) \
  NORMAL(LSHIFT, 0x12, arg) \
  NORMAL(COMMA, 0x41, arg) \
  NORMAL(PERIOD, 0x49, arg) \
  NORMAL(SLASH, 0x4A, arg) \
  NORMAL(RSHIFT, 0x59, arg) \
  NORMAL(LCTRL, 0x14, arg) \
  NORMAL(LALT, 0x11, arg) \
  NORMAL(SPACE, 0x29, arg) \
  NORMAL(A, 0x1C, arg) \
  NORMAL(B, 0x32, arg) \
  NORMAL(C, 0x21, arg) \
  NORMAL(D, 0x23, arg) \
  NORMAL(E, 0x24, arg) \
  NORMAL(F, 0x2B, arg) \
  NORMAL(G, 0x34, arg) \
  NORMAL(H, 0x33, arg) \
  NORMAL(I, 0x43, arg) \
  NORMAL(J, 0x3B, arg) \
  NORMAL(K, 0x42, arg) \
  NORMAL(L, 0x4B, arg) \
  NORMAL(M, 0x3A, arg) \
  NORMAL(N, 0x31, arg) \
  NORMAL(O, 0x44, arg) \
  NORMAL(P, 0x4D, arg) \
  NORMAL(Q, 0x15, arg) \
  NORMAL(R, 0x2D, arg) \
  NORMAL(S, 0x1B, arg) \
  NORMAL(T, 0x2C, arg) \
  NORMAL(U, 0x3C, arg) \
  NORMAL(V, 0x2A, arg) \
  NORMAL(W, 0x1D, arg) \
  NORMAL(X, 0x22, arg) \
  NORMAL(Y, 0x35, arg) \
  NORMAL(Z, 0x1A, arg) \
  EXT(APPLICATION, 0x2F, arg) \
  EXT(RALT, 0x11, arg) \
  EXT(RCTRL, 0x14, arg) \
  EXT(UP, 0x75, arg) \
  EXT(DOWN, 0x72, arg) \
  EXT(LEFT, 0x6B, arg) \
  EXT(RIGHT, 0x74, arg) \
  EXT(INSERT, 0x70, arg) \
  EXT(DELETE, 0x71, arg) \
  EXT(HOME, 0x6C, arg) \
  EXT(END, 0x69, arg) \
  EXT(PAGEUP, 0x7D, arg) \
  EXT(PAGEDOWN, 0x7A, arg)

#define AM_KEYS_HELPER(name, code, cb) cb(name)
// Compatible with original `AM_KEYS`
#define AM_KEYS(cb)  AM_KEY_TABLE(AM_KEYS_HELPER, AM_KEYS_HELPER, cb)

#define AM_KEY_NAMES(key) AM_KEY_##key,
enum {
  AM_KEY_NONE = 0,
  AM_KEYS(AM_KEY_NAMES)
};

// GPU

#define AM_GPU_TEXTURE  1
#define AM_GPU_SUBTREE  2
#define AM_GPU_NULL     0xffffffff

typedef uint32_t gpuptr_t;

struct gpu_texturedesc {
  uint16_t w, h;
  gpuptr_t pixels;
} __attribute__((packed));

struct gpu_canvas {
  uint16_t type, w, h, x1, y1, w1, h1;
  gpuptr_t sibling;
  union {
    gpuptr_t child;
    struct gpu_texturedesc texture;
  };
} __attribute__((packed));

#endif
