.option norvc
.include "common.S"
.section .text.init
.global _start
_start:
  addi x1, x0, 5
  addi x2, x1, 2
  addi x3, x2, 4
  li t2, 11
  bne x3, t2, 9f
  PASS
9: FAIL 2
