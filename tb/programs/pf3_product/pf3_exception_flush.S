#include "../rvc_fetch/common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option norvc
  addi s0, zero, 27
  .word 0xffffffff
  addi s1, zero, 61
  TOHOST_PASS
