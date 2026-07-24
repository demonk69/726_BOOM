.option norvc
.include "common.S"
.section .text.init
.global _start
_start:
  .word 0x00000000
  FAIL 8
