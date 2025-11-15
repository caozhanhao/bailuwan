.section entry, "ax"
.globl _start
.type _start, @function

_bootloader:
  call _start

_start:
  mv s0, zero
  la sp, _stack_pointer
  call _trm_init

.size _start, . - _start
