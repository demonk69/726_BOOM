#include "common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option rvc
  c.li s0, 14
  c.addi s0, 1
  .option norvc
  addi s1, s0, 16
  TOHOST_PASS
