#include "../rvc_fetch/common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option rvc
  c.li s0, 6
  .option norvc
  addi s1, s0, 9
  .option rvc
  c.addi s1, 2
  .option norvc
  addi a0, s1, 1
  TOHOST_PASS
