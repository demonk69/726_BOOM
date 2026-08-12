#include "common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option rvc
  c.li s0, 6
  .option norvc
  addi s1, s0, 7
  .option rvc
  c.add s0, s1
  .option norvc
  xor a0, s0, s1
  TOHOST_PASS
