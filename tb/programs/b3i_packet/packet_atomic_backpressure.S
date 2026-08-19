#include "../rvc_fetch/common.inc"
.section .text.init,"ax"
.global _start
_start:
  DATA_ADDRESS t0
  ld s0, 0(t0)
  .option rvc
  .rept 40
  c.addi s0, 1
  .endr
  c.li s1, 19
  .option norvc
  add a0, s0, s1
  TOHOST_PASS
