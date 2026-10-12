.macro LOAD_BASE reg
  addi \reg, zero, 1
  slli \reg, \reg, 31
  addi \reg, \reg, 256
.endm

.macro TOHOST_PASS
  addi t2, zero, 1
  slli t2, t2, 31
  addi t2, t2, 128
  addi t3, zero, 1
  sd t3, 0(t2)
1: jal zero, 1b
.endm

.section .text.init,"ax"
.global _start
_start:
  LOAD_BASE t0

  ld s0, 0(t0)
  ld s1, 8(t0)
  ld s2, 16(t0)
  ld s3, 24(t0)
  ld s4, 32(t0)
  ld s5, 40(t0)

  add a0, s0, s1
  add a0, a0, s2
  add a0, a0, s3
  add a0, a0, s4
  add a0, a0, s5
  sd a0, 64(t0)

  TOHOST_PASS
