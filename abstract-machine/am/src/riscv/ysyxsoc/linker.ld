ENTRY(_start)

MEMORY {
  mrom : ORIGIN = 0x20000000, LENGTH = 4K
  sram : ORIGIN = 0x0f000000, LENGTH = 8K
  /* only low 24-bit is taken in APBSPI, so flash size should be 2^24 bytes = 16 M */
  flash : ORIGIN = 0x30000000, LENGTH = 16M
  psram : ORIGIN = 0x80000000, LENGTH = 16M
}

SECTIONS {
  .text : ALIGN(8) {
    _sitext = LOADADDR(.text);
    _stext = .;
    *(entry)
    *(.text*)
    _etext = .;
  } > sram AT> flash

  .rodata : ALIGN(8) {
    _sirodata = LOADADDR(.rodata);
    _srodata = .;
    *(.rodata*)
    _erodata = .;
  } > sram AT> flash

  .data : {
    _sidata = LOADADDR(.data);
    _sdata = .;
    *(.data*)
    *(.sdata*)
    _edata = .;
  } > sram AT> flash

  .bss (NOLOAD): {
    _sbss = .;
    *(.bss*)
    *(.sbss*)
    *(.scommon)
    _ebss = .;

    /* heap starts immediately after bss */
    _heap_start = .;
    _heap_start = ALIGN(_heap_start, 8);
  } > sram

  .stack (NOLOAD) : {
    _stack_top   = ORIGIN(sram) + LENGTH(sram);
    _stack_start = _stack_top - 0x800; /* 2 KB */
    . = _stack_start;
    . = . + 0x800;
    _stack_pointer = _stack_top;
  } > sram

  /* heap ends at the end of psram */
  _heap_end = ORIGIN(psram) + LENGTH(psram);

  end = .;
  _end = .;
}
