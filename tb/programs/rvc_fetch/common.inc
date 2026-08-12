.macro TOHOST_PASS
  .option push
  .option norvc
  addi t0, zero, 1
  slli t0, t0, 31
  addi t0, t0, 128
  addi t1, zero, 1
  sd t1, 0(t0)
1: jal zero, 1b
  .option pop
.endm

.macro DATA_ADDRESS reg
  .option push
  .option norvc
  addi \reg, zero, 1
  slli \reg, \reg, 31
  addi \reg, \reg, 256
  .option pop
.endm
