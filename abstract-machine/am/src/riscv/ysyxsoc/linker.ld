ENTRY(_start)
PHDRS { text PT_LOAD; data PT_LOAD; }

MEMORY {
  MROM (rx)  : ORIGIN = 0x20000000, LENGTH = 0x00001000
  SRAM (rwx) : ORIGIN = 0x0f000000, LENGTH = 0x01000000
}

SECTIONS {
  . = ORIGIN(MROM);

  .text : ALIGN(8) {
    KEEP(*(entry))
    *(.text*)
    *(.rodata*)
  } > MROM
  etext = .;
  _etext = .;

  .rodata : ALIGN(8) {
    *(.rodata*)
  } > MROM
  .data : {
    *(.data*)
  } : data
  edata = .;
  _data = .;

  .bss : {
	_bss_start = .;
    *(.bss*)
    *(.sbss*)
    *(.scommon)
  }

  .stack (NOLOAD) : {
    _stack_start = ORIGIN(SRAM);
    . = . + LENGTH(SRAM);
    _stack_end = .;
    _stack_top = ALIGN(_stack_end);
    _stack_pointer = _stack_top;
  } > SRAM

  end = .;
  _end = .;
  _heap_start = ALIGN(0x1000);
}
