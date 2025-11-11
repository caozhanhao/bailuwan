ENTRY(_start)

MEMORY {
  mrom : ORIGIN = 0x20000000, LENGTH = 4K
  sram : ORIGIN = 0x0f000000, LENGTH = 8K
}

SECTIONS {
  . = ORIGIN(mrom);

  .text : ALIGN(8) {
    KEEP(*(entry))
    *(.text*)
    *(.rodata*)
  } > mrom AT> mrom
  etext = .;
  _etext = .;

  .rodata : ALIGN(8) {
    *(.rodata*)
  } > mrom AT> mrom

  .data : {
    *(.data*)
  } > sram AT> mrom

  edata = .;
  _data = .;

  .bss : {
	_bss_start = .;
    *(.bss*)
    *(.sbss*)
    *(.scommon)
  } > sram AT> mrom

  .stack (NOLOAD) : {
    _stack_start = ORIGIN(sram) + LENGTH(sram) - 0x800;
    . = . + 0x800; /* 2 KB */
    _stack_end = .;
    _stack_top = ALIGN(_stack_end);
    _stack_pointer = _stack_top;
  } > sram

  _heap_start = 0x0f000000;
  _heap_end = _stack_start;

  end = .;
  _end = .;
}
