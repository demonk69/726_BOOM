#include "../rvc_fetch/common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option rvc
  c.li s0, 0
.global pf5_bim_probe_0
pf5_bim_probe_0:
  c.beqz s0, 1f
  c.li s1, 9
  c.li s2, 10
1:
  c.li s1, 25
  c.addi s0, 6
  .option norvc
  addi s2, s1, 12
  TOHOST_PASS
