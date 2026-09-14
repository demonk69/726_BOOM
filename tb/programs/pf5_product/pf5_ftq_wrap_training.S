#include "../rvc_fetch/common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option norvc
  addi s0, zero, 0
  .rept 80
  addi s0, s0, 1
  .endr
  addi s1, zero, 1
.global pf5_bim_probe_0
pf5_bim_probe_0:
  beq s1, s1, 1f
  addi s2, zero, 99
1:
  addi s2, s0, 8
  TOHOST_PASS
