.option norvc
.include "common.S"
.section .text.init
.global _start
_start:
  addi x1, x0, 3
  add x2, x1, x0
  addi x1, x0, 7
  li t2, 3
  bne x2, t2, 9f
  li t2, 7
  bne x1, t2, 9f
  PASS
9: FAIL 3
