#include "../rvc_fetch/common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option norvc
  addi s0, zero, 0
  .rept 48
  addi s0, s0, 1
  .endr
  addi s1, s0, 4
  TOHOST_PASS
