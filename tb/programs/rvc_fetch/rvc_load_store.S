#include "common.inc"
.section .text.init,"ax"
.global _start
_start:
  DATA_ADDRESS s0
  .option rvc
  c.li s1, 31
  c.addi s1, 15
  c.sd s1, 0(s0)
  c.ld a0, 0(s0)
  .option norvc
  addi a1, a0, 1
  TOHOST_PASS
