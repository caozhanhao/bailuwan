ENTRY(_start)

MEMORY {
  mrom : ORIGIN = 0x20000000, LENGTH = 4K
  sram : ORIGIN = 0x0f000000, LENGTH = 8K
}

SECTIONS {
  . = ORIGIN(mrom);

  .text : ALIGN(8) {
    *(entry)
    *(.text*)
  } > mrom AT> mrom
  etext = .;
  _etext = .;

  .rodata : ALIGN(8) {
    *(.rodata*)
  } > mrom AT> mrom

  .data : {
    _sidata = LOADADDR(.data);
    _sdata = .;
    *(.data*)
    _edata = .;
  } > sram AT> mrom

  .bss (NOLOAD): {
    _sbss = .;
    *(.bss*)
    *(.sbss*)
    *(.scommon)
    _ebss = .;
  } > sram

  .stack (NOLOAD) : {
    _stack_top   = ORIGIN(sram) + LENGTH(sram);
    _stack_start = _stack_top - 0x800; /* 2 KB */
    . = _stack_start;
    . = . + 0x800;
    _stack_pointer = _stack_top;
  } > sram

  _heap_start = ORIGIN(sram);
  _heap_end = _stack_start;

  end = .;
  _end = .;
}
