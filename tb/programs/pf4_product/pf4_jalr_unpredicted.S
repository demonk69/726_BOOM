#include "../rvc_fetch/common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option norvc
  addi s0, zero, 30
  auipc t2, 0
  addi t2, t2, 16
  jalr ra, 0(t2)
  addi s1, zero, 99
  addi s1, zero, 40
  addi s2, s0, 20
  TOHOST_PASS
