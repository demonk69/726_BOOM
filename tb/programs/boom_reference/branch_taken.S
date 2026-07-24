.option norvc
.include "common.S"
.section .text.init
.global _start
_start:
  addi x1, x0, 5
  addi x2, x0, 5
  beq x1, x2, 1f
  addi x3, x0, 99
1:
  addi x3, x0, 10
  li t2, 10
  bne x3, t2, 9f
  PASS
9: FAIL 4
