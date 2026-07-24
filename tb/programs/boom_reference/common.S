# Common bare-metal helpers for BOOM reference trace programs.
.macro PASS
  la t0, tohost
  li t1, 1
  sd t1, 0(t0)
1: j 1b
.endm

.macro FAIL code
  la t0, tohost
  li t1, ((\code << 1) | 1)
  sd t1, 0(t0)
1: j 1b
.endm

.section .tohost,"aw",@progbits
.align 6
.global tohost
tohost: .dword 0
.global fromhost
fromhost: .dword 0
