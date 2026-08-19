#include "../rvc_fetch/common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option rvc
  c.li s0, 3
  c.li s1, 4
  .option norvc
  add a0, s0, s1
  TOHOST_PASS
