#include "../rvc_fetch/common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option norvc
  addi s0, zero, 7
  jal ra, 1f
  addi s0, s0, 40
  addi s1, zero, 31
1:
  addi s0, s0, 2
  addi s1, zero, 13
  TOHOST_PASS
