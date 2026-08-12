#include "common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option rvc
  c.li s0, 2
  c.j 1f
  c.addi s0, 20
1:
  c.addi s0, 5
  .option norvc
  addi s1, s0, 11
  TOHOST_PASS
