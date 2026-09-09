#include "../rvc_fetch/common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option norvc
  addi s0, zero, 5
  addi s1, zero, 5
  beq s0, s1, 1f
  addi s2, zero, 63
  addi s3, zero, 62
1:
  addi s2, zero, 19
  addi s3, s2, 4
  TOHOST_PASS
