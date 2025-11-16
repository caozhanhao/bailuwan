ENTRY(_fsbl)

MEMORY {
  mrom : ORIGIN = 0x20000000, LENGTH = 4K
  sram : ORIGIN = 0x0f000000, LENGTH = 8K
  /* only low 24-bit is taken in APBSPI, so flash size should be 2^24 bytes = 16 M */
  flash : ORIGIN = 0x30000000, LENGTH = 16M
  psram : ORIGIN = 0x80000000, LENGTH = 16M
}

SECTIONS {
  .fsbl : ALIGN(8) {
    *(fsbl)
  } > flash AT> flash

  .ssbl : ALIGN(8) {
    _ssbl_lma = LOADADDR(.ssbl);
    _sssbl = .;
    *(ssbl)
    _essbl = .;
  } > sram AT> flash

  .text : ALIGN(8) {
    _text_lma = LOADADDR(.text);
    _stext = .;
    *(.entry)
    *(.text*)
    _etext = .;
  } > psram AT> flash

  .rodata : ALIGN(8) {
    _rodata_lma = LOADADDR(.rodata);
    _srodata = .;
    *(.rodata*)
    _erodata = .;
  } > psram AT> flash

  .data : {
    _data_lma = LOADADDR(.data);
    _sdata = .;
    *(.data*)
    *(.sdata*)
    _edata = .;
  } > psram AT> flash

  .bss (NOLOAD): {
    _sbss = .;
    *(.bss*)
    *(.sbss*)
    *(.scommon)
    _ebss = .;
  } > psram

  .stack (NOLOAD) : {
    _stack_top   = ORIGIN(sram) + LENGTH(sram);
    _stack_start = _stack_top - 0x800; /* 2 KB */
    . = _stack_start;
    . = . + 0x800;
    _stack_pointer = _stack_top;
  } > sram

  _heap_start = ALIGN(_ebss, 8);
  _heap_end = ORIGIN(psram) + LENGTH(psram);
}
