#include "../rvc_fetch/common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option norvc
  addi s0, zero, 31
  .word 0xffffffff
  addi s1, zero, 99
  addi s2, zero, 99
  .org 0xc0
pf4_trap_handler:
  addi s1, zero, 41
  addi s2, s0, 20
  TOHOST_PASS
