.option norvc
.include "common.S"
.section .text.init
.global _start
_start:
  ecall
  PASS
