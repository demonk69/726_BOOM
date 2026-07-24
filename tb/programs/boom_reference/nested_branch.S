.option norvc
.include "common.S"
.section .text.init
.global _start
_start:
  addi x1, x0, 1
  addi x2, x0, 1
  beq x1, x2, 1f
  addi x3, x0, 11
1:
  beq x1, x2, 2f
  addi x3, x0, 22
2:
  addi x3, x0, 33
  li t2, 33
  bne x3, t2, 9f
  PASS
9: FAIL 7
