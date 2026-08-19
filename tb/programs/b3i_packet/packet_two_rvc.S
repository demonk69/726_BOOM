#include "../rvc_fetch/common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option rvc
  c.li s0, 5
  c.addi s0, 7
  .option norvc
  addi s1, s0, 9
  TOHOST_PASS
