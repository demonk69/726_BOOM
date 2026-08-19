#include "../rvc_fetch/common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option rvc
  c.li s0, 1
  c.j redirect_target
  c.addi s0, 10
  c.li s1, 31
redirect_target:
  c.addi s0, 2
  c.li s1, 7
  .option norvc
  add a0, s0, s1
  TOHOST_PASS
