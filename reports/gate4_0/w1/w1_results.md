# Gate 4.0 W1 Results

Status: `W1_FIXED_LANE_INTERFACE_VERIFIED`.

## Change

The issue interface remains `ISSUE_WIDTH=3`, and the execute-result interface now has the same fixed lane count. The implemented issue/result cap remains `DISPATCH_WIDTH=1`, so only lane 0 can become valid in W1. Branch recovery, LSU intake, ROB completion, and reset now consume or clear every fixed result lane.

## Verification

- Dedicated lane interface test: PASS; lanes 1 and 2 remain invalid and the unissued IQ entry is retained.
- Existing directed, IQ, LSU, branch snapshot, and 21-seed randomized branch regressions: PASS.
- Reset architecture: 14/14 PASS.
- Gate 3.9 frozen C++ and C-sim traces: 10/10 byte-identical.
- Full-program C++ and C-sim architectural checks: 10/10 PASS.
- Conservative top-level synthesis: PASS; `CORE_CYCLE` remains unpipelined.

## Synthesis

| Variant | Estimated period | LUT | FF | BRAM_18K | DSP |
|---|---:|---:|---:|---:|---:|
| Gate 3.9 accepted | 5.898 ns | 47999 | 12134 | 12 | 3 |
| Gate 4.0 W1 | 5.898 ns | 51558 | 12802 | 12 | 3 |

W1 adds 3559 LUT and 668 FF while preserving the 5.898 ns estimate, 12 BRAM_18K, and 3 DSP. This is an interface-expansion checkpoint, not an accepted performance configuration.

## Decision

W1 passes its functional and synthesis gate. W2 may experiment with at most two simultaneous grants, constrained to one MEM-compatible uop and one INT-compatible uop. The FP queue remains out of scope, so peak integer issue 3 remains prohibited.
