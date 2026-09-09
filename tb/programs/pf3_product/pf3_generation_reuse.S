#include "../rvc_fetch/common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option norvc
  addi s0, zero, 0
  .rept 38
  addi s0, s0, 1
  .endr
  jal zero, 1f
  addi s1, zero, 60
1:
  .rept 38
  addi s0, s0, 1
  .endr
  addi s1, s0, 3
  TOHOST_PASS
