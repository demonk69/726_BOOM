#include "../rvc_fetch/common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option norvc
  addi s0, zero, 27
.global pf5_fault_site
pf5_fault_site:
  addi s1, zero, 99
  addi s2, zero, 99
  .org 0xc0
pf5_trap_handler:
  addi s1, zero, 37
  addi s2, s0, 20
  TOHOST_PASS
