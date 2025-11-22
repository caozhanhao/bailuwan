#include "../riscv.h"

#include <am.h>
#include <amdev.h>
#include <klib-macros.h>
#include <klib.h>

#define PS2_KEYBOARD_BASE 0x10011000

#define AM_KEY_ENTRY(name, code, arg) [code] = AM_KEY_##name,
#define AM_EXT_KEY_ENTRY(name, code, arg)
static int keymap[256] = {
  AM_KEY_TABLE(AM_KEY_ENTRY, AM_EXT_KEY_ENTRY, 0)
};
#undef AM_KEY_ENTRY
#undef AM_EXT_KEY_ENTRY

#define AM_EXT_KEY_ENTRY(name, code, arg) [code] = AM_KEY_##name,
#define AM_KEY_ENTRY(name, code, arg)
static int keymap_ext[256] = {
  AM_KEY_TABLE(AM_KEY_ENTRY, AM_EXT_KEY_ENTRY, 0)
};
#undef AM_KEY_ENTRY
#undef AM_EXT_KEY_ENTRY

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  // Attention: Initialize kbd first in case `inb(PS2_KEYBOARD_BASE)` is 0.
  kbd->keycode = AM_KEY_NONE;
  kbd->keydown = 0;

  bool keydown = true;
  bool ext = false;
  uint8_t code;
  while ((code = inb(PS2_KEYBOARD_BASE)))
  {
    if (code == 0xF0)
      keydown = false;
    else if (code == 0xE0)
      ext = true;
    else
    {
      kbd->keycode = ext ? keymap_ext[code] : keymap[code];
      kbd->keydown = keydown;
      return;
    }
  }
}
