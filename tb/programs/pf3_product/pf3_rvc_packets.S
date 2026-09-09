#include "../rvc_fetch/common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option rvc
  c.li s0, 3
  c.li s1, 4
  c.add s0, s1
  c.addi s0, 5
  c.li s2, 9
  c.add s2, s0
  .option norvc
  add s3, s0, s2
  TOHOST_PASS
