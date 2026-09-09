#include "../rvc_fetch/common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option norvc
  addi s0, zero, 11
  addi s1, zero, 17
  add s2, s0, s1
  addi s3, s2, 5
  xor s4, s3, s0
  TOHOST_PASS
