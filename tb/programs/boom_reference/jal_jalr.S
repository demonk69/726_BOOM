.option norvc
.include "common.S"
.section .text.init
.global _start
_start:
  jal x0, 1f
  addi x3, x0, 99
1:
  auipc x2, 0
  addi x2, x2, 16
  jalr x1, 0(x2)
  addi x3, x0, 88
  addi x3, x0, 5
  li t2, 5
  bne x3, t2, 9f
  PASS
9: FAIL 6
