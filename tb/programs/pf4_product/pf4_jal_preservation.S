#include "../rvc_fetch/common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option norvc
  addi s0, zero, 29
  jal ra, 1f
  addi s1, zero, 99
1:
  addi s1, zero, 39
  addi s2, s0, 20
  TOHOST_PASS
