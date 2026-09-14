#include "../rvc_fetch/common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option norvc
  addi s0, zero, 11
.global pf5_bim_probe_0
pf5_bim_probe_0:
  beq s0, zero, 1f
  addi s1, zero, 21
  jal zero, 2f
1:
  addi s1, zero, 99
2:
  addi s2, s0, 20
  TOHOST_PASS
