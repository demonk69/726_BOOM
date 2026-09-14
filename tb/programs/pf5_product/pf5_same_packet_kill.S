#include "../rvc_fetch/common.inc"
.section .text.init,"ax"
.global _start
_start:
  .option rvc
  c.li s0, 0
  .balign 4
.global pf5_bim_probe_0
pf5_bim_probe_0:
  c.beqz s0, 1f
  c.li s1, 9
  c.li s2, 10
1:
  c.li s1, 26
  c.li s2, 13
  .option norvc
  add s3, s1, s2
  TOHOST_PASS
