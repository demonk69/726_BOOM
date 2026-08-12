#include "common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option rvc
  c.li s0, -1
  c.srli s0, 32
  .option norvc
  la t0, 1f
  la s2, jalr_site
  .option rvc
jalr_site:
  c.jalr t0
  c.addi s0, 1
1:
  .option norvc
  sub s1, ra, s2
  TOHOST_PASS
