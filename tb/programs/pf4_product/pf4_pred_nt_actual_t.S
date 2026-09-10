#include "../rvc_fetch/common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option norvc
  addi s0, zero, 0
.global pf4_bim_probe_0
pf4_bim_probe_0:
  beq s0, zero, 1f
  addi s1, zero, 99
1:
  addi s1, zero, 22
  addi s2, s1, 10
  TOHOST_PASS
