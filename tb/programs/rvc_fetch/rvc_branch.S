#include "common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option rvc
  c.li s0, 0
  c.beqz s0, 1f
  c.li s1, 3
1:
  c.li s1, 9
  .option norvc
  addi a0, s1, 4
  TOHOST_PASS
