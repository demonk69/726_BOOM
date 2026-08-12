#include "common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option rvc
  c.li s0, 7
  c.li s1, 6
  .option norvc
  mul a0, s0, s1
  div a1, a0, s1
  .option rvc
  c.addi a1, 1
  .option norvc
  TOHOST_PASS
