# Provisional Gate 3 Results

Date: 2026-07-24

Provisional Gate 3: PARTIAL PASS - architectural loaded-program prefix only.

READY_FOR_GATE_3=false

STRICT_CYCLE_EQUIVALENCE=false

## Verdict

| Check | Result | Evidence |
|---|---|---|
| HLS C++ vs Vitis HLS csim architectural prefix | PASS | `reports/equivalence/provisional_gate3/hls_cpp_vs_hls_csim_arch_diff.csv` |
| HLS C++ vs Vitis HLS csim event order | PASS | `reports/equivalence/provisional_gate3/hls_cpp_vs_hls_csim_event_diff.csv` |
| HLS C++ vs Vitis HLS csim normalized cycles | PASS | `reports/equivalence/provisional_gate3/hls_cpp_vs_hls_csim_cycle_diff.csv` |
| BOOM vs HLS csim architectural prefix | PASS | `reports/equivalence/provisional_gate3/boom_vs_hls_csim_arch_diff.csv` |
| BOOM vs HLS csim event order | FAIL | `reports/equivalence/provisional_gate3/boom_vs_hls_csim_event_diff.csv` |
| BOOM vs HLS csim normalized cycles | FAIL | `reports/equivalence/provisional_gate3/boom_vs_hls_csim_cycle_diff.csv` |
| Complete program comparison | BLOCKED | HLS `lsu_module` is a no-op, so the BOOM `SD` to `tohost` cannot be retired in HLS. |
| Official Gate 3 / strict cycle equivalence | BLOCKED | Official Chipyard/FESVR/DRAMSim simulator and noninterference path remain unavailable. |

## Scope

This run uses `PREFIX` mode. BOOM traces are normalized to the loaded program starting at `0x80000000`; boot/reset ROM commits are excluded because HLS does not execute that path and the standalone BOOM trace records those instructions as unavailable. The comparison stops before the first unsupported dynamic `SD` to `tohost` at `0x80000080`.

The common dynamic subset is generated at `reports/equivalence/provisional_gate3/common_instruction_subset.csv`.

## Architectural Prefix Results

| Program | BOOM prefix commits | HLS csim prefix commits | Result |
|---|---:|---:|---|
| independent_alu | 8 | 8 | PASS |
| raw_chain | 8 | 8 | PASS |
| branch_taken | 9 | 9 | PASS |
| branch_not_taken | 10 | 10 | PASS |
| nested_branch | 10 | 10 | PASS |

Total BOOM-vs-HLS architectural commits compared: 45. All compared PCs, instructions, architectural destination registers, destination values, and exception flags match.

## Event And Cycle Results

BOOM-vs-HLS event-order and normalized-cycle checks fail for all five programs. The first mismatches are caused by BOOM resolving branches ahead of older commit events, while the HLS prefix trace is serialized.

Examples:

- `independent_alu`: BOOM records the `BNE` branch event at `0x80000010` before the commit of `0x80000004`; HLS commits `0x80000004` before resolving that branch.
- `branch_taken`: BOOM records the taken `BEQ` branch event at `0x80000008` at normalized cycle `-1`, before the first loaded-program commit; HLS records the first event as the commit at `0x80000000`.

This is expected with the current HLS implementation because it is a serialized integer/control subset and does not model BOOM's speculative event interleaving. These failures prevent any strict cycle-equivalence claim.

## Generated Artifacts

| Artifact | Path |
|---|---|
| HLS C++ traces | `reference/hls_traces/*_hls_cpp.jsonl` |
| HLS csim traces | `reference/hls_traces/*_hls_csim.jsonl` |
| Normalized traces | `reports/equivalence/provisional_gate3/normalized/*.jsonl` |
| Per-program diff details | `reports/equivalence/provisional_gate3/diffs/*.json` |
| HLS C++ trace log | `reports/equivalence/provisional_gate3/hls_cpp_trace.log` |
| Vitis HLS csim trace log | `reports/equivalence/provisional_gate3/hls_csim_trace.log` |
| Trace schema | `reference/equivalence_trace_schema.json` |
| HLS prefix trace testbench | `tb/differential/hls_prefix_trace_tb.cpp` |
| C++ normalized-trace helper sources | `tb/differential/trace_compare.cpp`, `tb/differential/event_matcher.cpp`, `tb/differential/cycle_normalizer.cpp` |
| Termination-scope note | `docs/provisional_gate3_termination.md` |

## Commands Run

```text
python3 scripts/generate_provisional_gate3_subset.py
bash scripts/run_hls_prefix_traces.sh
bash scripts/run_architectural_diff.sh hls_cpp hls_csim
bash scripts/run_event_order_diff.sh hls_cpp hls_csim
bash scripts/run_normalized_cycle_diff.sh hls_cpp hls_csim
bash scripts/run_architectural_diff.sh boom hls_csim
bash scripts/run_event_order_diff.sh boom hls_csim
bash scripts/run_normalized_cycle_diff.sh boom hls_csim
```

## Final Status

Provisional Gate 3 establishes `ARCHITECTURAL_PREFIX_PASS` for the five loadmem-backed programs only. It does not establish full-program equivalence, event-order equivalence, normalized-cycle equivalence, official Gate 3, or strict cycle equivalence.
