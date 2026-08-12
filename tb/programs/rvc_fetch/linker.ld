OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS
{
  . = 0;
  .text : { *(.text.init) *(.text*) }
  /DISCARD/ : { *(.riscv.attributes) *(.comment) }
}
