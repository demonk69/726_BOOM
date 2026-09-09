#include "../rvc_fetch/common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option norvc
  addi s0, zero, 21
  addi s1, zero, 6
  mul s2, s0, s1
  addi s3, s2, 5
  mul s4, s3, s1
  TOHOST_PASS
