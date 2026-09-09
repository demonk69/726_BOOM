#include "../rvc_fetch/common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option norvc
  addi s0, zero, 1
  addi s1, zero, 2
  blt s0, s1, 1f
  addi s2, zero, 55
  addi s3, zero, 54
1:
  addi s2, zero, 21
  addi s3, s2, 8
  TOHOST_PASS
