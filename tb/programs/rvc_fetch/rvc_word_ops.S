#include "common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option rvc
  c.li s0, 20
  c.addiw s0, -3
  c.li s1, 5
  c.subw s0, s1
  .option norvc
  addiw a0, s0, -20
  TOHOST_PASS
