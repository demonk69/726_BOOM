#include "../rvc_fetch/common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option norvc
  addi s0, zero, 5
.global pf5_bim_wt_0
pf5_bim_wt_0:
  beq s0, zero, 1f
  addi s1, zero, 24
  jal zero, 2f
1:
  addi s1, zero, 99
2:
  addi s2, s1, 11
  TOHOST_PASS
