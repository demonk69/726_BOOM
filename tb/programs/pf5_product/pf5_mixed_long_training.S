#include "../rvc_fetch/common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option norvc
  addi s0, zero, 0
  addi s1, zero, 12
1:
  addi s0, s0, 1
.global pf5_bim_probe_0
pf5_bim_probe_0:
  blt s0, s1, 1b
  jal ra, 2f
  addi s2, zero, 99
2:
  auipc t2, 0
  addi t2, t2, 16
  jalr ra, 0(t2)
  addi s2, zero, 98
  addi s2, s0, 40
  addi s3, s2, 10
  TOHOST_PASS
