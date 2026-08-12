#include "common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option rvc
  c.li s0, 10
  .option norvc
  addi s1, s0, 9
  .option rvc
  c.addi s1, 2
  .option norvc
  add a0, s0, s1
  TOHOST_PASS
