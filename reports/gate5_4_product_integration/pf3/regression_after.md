# PF3 Regression After

- PF3 directed: 481322 checks, zero failures.
- PF3 bounded exhaustive: depth 2 length 6 and depth 4 length 4, zero errors.
- PF3 random: 256 x 8192, all 16 named counters zero.
- PF3 long run: 1000000 steps, zero leaks, maximum occupancy 32.
- PF3 CSim and focused generated RTL 100/100 cases (700 checks): PASS.
- PF2 current-source directed native/CSim 2239/2239 and random 256 x 8192: PASS.
- B3I packet-aware random 256 x 4096: PASS with all error counters zero.
- PF1 current-source directed 1817, random 256 seeds, 8 programs, and CSim 8: PASS.
- Backend W3 400/400, W4 multi-writeback 13/13, and RV64M directed/random/15 programs: PASS.
- Required PF3 product programs, PF3 full-core RTL, PF2 focused/full-core RTL, and
  PF1 focused/full-core RTL were not completed and remain acceptance blockers.
- Legacy R2 focused RVC failures remain the accepted PF2 non-verdict harness
  lifecycle mismatch; they are not counted as a PF3 PASS.
