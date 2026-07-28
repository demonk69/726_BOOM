# Full-Program Architectural Diff

Scope: loaded-program commits through the retired `SD` to `tohost`.

Result: 10/10 PASS

| Program | HLS Source | Status | Commits | Store PC Match | Tohost Match |
|---|---|---|---:|---|---|
| independent_alu | hls_cpp | PASS | 9 | TRUE | TRUE |
| raw_chain | hls_cpp | PASS | 9 | TRUE | TRUE |
| branch_taken | hls_cpp | PASS | 10 | TRUE | TRUE |
| branch_not_taken | hls_cpp | PASS | 11 | TRUE | TRUE |
| nested_branch | hls_cpp | PASS | 11 | TRUE | TRUE |
| independent_alu | hls_csim | PASS | 9 | TRUE | TRUE |
| raw_chain | hls_csim | PASS | 9 | TRUE | TRUE |
| branch_taken | hls_csim | PASS | 10 | TRUE | TRUE |
| branch_not_taken | hls_csim | PASS | 11 | TRUE | TRUE |
| nested_branch | hls_csim | PASS | 11 | TRUE | TRUE |

This is still not official Gate 3 equivalence: the official Chipyard/FESVR/DRAMSim simulator path remains unavailable, and the HLS LSU remains a minimal integer LSU path rather than a full BOOM LSU/cache/MMU implementation.
