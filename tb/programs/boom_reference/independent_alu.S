.option norvc
.include "common.S"
.section .text.init
.global _start
_start:
  addi x1, x0, 5
  addi x2, x0, 7
  add x3, x1, x2
  li t2, 12
  bne x3, t2, 9f
  PASS
9: FAIL 1
