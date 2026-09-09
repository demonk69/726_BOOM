#include "../rvc_fetch/common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option norvc
  addi s0, zero, 4
  addi s1, zero, 9
  beq s0, s1, 3f
  addi s2, zero, 7
  blt s0, s1, 1f
  addi s2, zero, 50
1:
  jal ra, 2f
  addi s3, zero, 51
2:
  addi s3, s2, 8
  bne s0, s0, 3f
  addi s4, s3, 2
3:
  TOHOST_PASS
