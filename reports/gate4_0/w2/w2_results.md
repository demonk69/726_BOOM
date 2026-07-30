# Gate 4.0 W2 Results

Status: `W2_DUAL_SELECTION_VERIFIED`.

## Change

The implemented shared eight-entry IQ now classifies supported uops and independently selects the oldest ready MEM-compatible entry for fixed lane 0 and the oldest ready INT-compatible entry for fixed lane 1. Lane 2 remains reserved for the unsupported FP queue. Branch kill and resolve-mask clearing occur before selection, and per-lane readiness controls acceptance without dropping a generated grant. Dispatch-origin grants are generated only when they can be accepted or preserved.

W2 deliberately preserves the conservative acceptance budget: at most one generated grant is copied to `issued_uops[0]` per cycle. Therefore peak generated grants are two, peak accepted grants are one, and this result is not dual execution or `PARTIAL_WIDE_ISSUE_MAX2`. Zero-drop accounting applies to generated grants; broader frontend/rename retry under a full blocked IQ remains outside this checkpoint.

## Verification

- Frozen W1 source regressions: 149/149 assertions PASS.
- W2 directed dual-grant tests: 28/28 PASS, including exception removal and full-blocked-IQ dispatch grant gating.
- W2 random differential: 64 seeds, 2048 cycles, 63 dual-grant cycles, and all four lane-ready masks covered.
- Generated-grant accounting: 382 accepted, 424 retained, 0 dropped.
- Frozen HLS C++ and csim traces: 10/10 byte-identical.
- Full-program architectural comparison: 10/10 PASS.
- Partial-order evidence: 8 legal reorder events and 0 real exposed violations.
- Dedicated synthesized issue-selection RTL: 5/5 PASS.
- Full generated-core XSim matrix: 49/49 PASS.

## Synthesis

| Top | Scope | Estimated period | LUT | FF | BRAM_18K | DSP |
|---|---|---:|---:|---:|---:|---:|
| W1 `boom_core_top` | Frozen baseline | 5.898 ns | 51558 | 12802 | 12 | 3 |
| W2 `synth_issue_top` | Diagnostic | 4.570 ns | 15079 | 4608 | 0 | 0 |
| W2 `synth_core_step_top` | Diagnostic | 5.898 ns | 54667 | 15123 | 12 | 3 |
| W2 `boom_core_top` | Product checkpoint | 5.898 ns | 61760 | 15213 | 12 | 3 |

The W2 product checkpoint adds 10202 LUT (19.79%) and 2411 FF (18.83%) relative to W1, with unchanged estimated period, BRAM, and DSP. `CORE_CYCLE` remains unpipelined. The integrated 5.89 ns limiter remains LSU load-response extraction; standalone issue selection is 4.57 ns.

This area increase is evidence for the W2 selection checkpoint, not acceptance of W2 as the product PPA configuration.

## Decision

W2 closes simultaneous MEM/INT grant generation for the supported subset. W3 may evaluate raising acceptance and execute intake to two, but must preserve independent backpressure, generated-grant retention, branch recovery, and the FP-lane prohibition. Unsupported non-exception classes remain queued for an unavailable unit, and the serialized frontend/rename path still lacks a retry handshake when dispatch meets a full IQ with no accepted grant.

| Decision | Value |
|---|---|
| `READY_FOR_DUAL_EXECUTE_ACCEPTANCE_EXPERIMENT` | true |
| `READY_FOR_PARTIAL_WIDE_ISSUE_MAX2` | false |
| `READY_FOR_OFFICIAL_GATE_3` | false |
| `M009` | `PARTIALLY_VERIFIED` |
| `M014` | `VERIFIED` |

Official Gate 3 remains blocked by the unavailable Chipyard/FESVR/DRAMSim environment. No strict BOOM cycle-equivalence claim is made.
