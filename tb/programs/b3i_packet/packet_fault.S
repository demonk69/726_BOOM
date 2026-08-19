#include "../rvc_fetch/common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option rvc
  c.li s0, 13
  .2byte 0x0000
  c.li s1, 29
  .option norvc
  addi a0, s1, 1
  TOHOST_PASS
