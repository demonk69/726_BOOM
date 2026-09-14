#include "../rvc_fetch/common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option norvc
  addi s0, zero, 7
.global pf5_bim_st_0
pf5_bim_st_0:
  bne s0, zero, 1f
  addi s1, zero, 99
1:
  addi s1, zero, 23
  addi s2, s0, 27
  TOHOST_PASS
