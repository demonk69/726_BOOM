#include "common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option norvc
  beq zero, zero, target
  .option rvc
  c.li s0, 1
  .option norvc
  addi s0, zero, 2
target:
  .option rvc
  c.li s0, 23
  .option norvc
  addi s1, s0, 4
  TOHOST_PASS
