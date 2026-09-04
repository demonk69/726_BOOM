# PF2 Regression After

- W3: 15/15 suites, 400/400 checks, 197 runs PASS; C++/CSim normalized 7/7 and full programs 10/10.
- W4 multi-writeback: 13/13 PASS.
- RV64M: directed 1458 checks, random 256 x 2048, programs 15/15 PASS.
- RVC full-core native and Vitis CSim: 11/11 PASS in each mode after adding the four canonical PF2 Frontend dependencies to the existing CSim manifest.
- Gate 5.3 accepted B3I random: 256 x 4096, all counters zero.
- PF1: directed 1817, random 256 x 8192, native programs 8/8, CSim 8/8, focused RTL 64/64, full-core RTL 8/8 PASS.
- Fresh PF2 full-core generated RTL RVC matrix: 11/11 PASS.
- Legacy focused RVC harness: `NON_VERDICT_DIAGNOSTIC` lifecycle-version mismatch, not an accepted failure and not reported as PASS; see `preservation_runner_selection.md`.
- A fresh rebuild of the historical Gate 3.9 snapshot with the current HEAD harness is not a valid preservation run: Vivado reports a 192-bit RTL versus 128-bit harness IMEM request-port mismatch and all 49 cases then time out at zero commits. The diagnostic is retained under `gate3_9_preservation/` as `NON_VERDICT_DIAGNOSTIC`; historical accepted Gate 3.9 and B3I preservation artifacts remain 49/49. Current-source reset behavior is covered by fresh PF2 full-core exception RTL 8/8 plus PF2 focused reset/redirect cases.
