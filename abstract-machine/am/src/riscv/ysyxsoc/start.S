.macro COPY_SECTION_WORD src, dst, end
    la   t0, \src       /* source (LMA) */
    la   t1, \dst       /* dest   (VMA) */
    la   t2, \end       /* end    (VMA) */
.Lcp\@:
    beq  t1, t2, .Lcp_done\@
    lw   x10, 0(t0)
    sw   x10, 0(t1)
    addi t0, t0, 4
    addi t1, t1, 4
    j    .Lcp\@
.Lcp_done\@:
.endm

/* entry section is placed in flash */
.section entry, "ax"
.global _bootloader
.type _bootloader, @function

_bootloader:
    COPY_SECTION_WORD _text_lma, _stext, _etext
    COPY_SECTION_WORD _rodata_lma, _srodata, _erodata
    COPY_SECTION_WORD _data_lma, _sdata, _edata

    /* ---- clear .bss: zero _sbss .. _ebss ---- */
    la   t1, _sbss
    la   t2, _ebss
7:
    beq  t1, t2, 8f
    sw   x0, 0(t1)
    addi t1, t1, 4
    j    7b
8:
    la   t0, _start
    jr   t0

.size _bootloader, . - _bootloader

.globl _start
.type _start, @function
_start:
  mv s0, zero
  la sp, _stack_pointer
  call _trm_init

.size _start, . - _start
