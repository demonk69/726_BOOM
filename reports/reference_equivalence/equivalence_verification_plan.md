# BOOM HLS Reference RTL Equivalence Verification Plan

## Reference and Configuration Policy

The only primary reference is the generated design in `/home/lab_726/boom/chipyard.TestHarness.SmallBoomConfig/`. Its generated FIRRTL/Verilog and hashes are recorded in `reference_source_manifest.csv`. SmallBoomConfig's effective default is the primary target; establish its actual LQ/SQ and other effective parameters from generated RTL/FIRRTL before comparison. `8_8` is the main HLS configuration when confirmed against that reference. Other LQ/SQ depths (`4_4`, `16_16`, `4_16`, `16_4`) are parameterized HLS extension runs and do not replace or redefine the reference.

`PPA_GATE_STATUS`, HLS unit tests, RTL self-tests, and prior capacity reports do not establish reference equivalence. A PASS requires matched reference-derived input traces and an independent Verilog-vs-HLS differential result.

## Equivalence Classes

| Class | Required correspondence | Examples | Comparison |
| --- | --- | --- | --- |
| Architectural behavior | Same committed architectural state and externally visible outcomes for same instruction/environment stream | integer/FP registers, memory, exceptions, CSR/privilege, branch redirect architectural result | Retirement stream by PC/instruction/rd/value, memory transactions/results, final memory, trap/interrupt/CSR state |
| Transaction protocol | Same legal externally visible request/response contract at a declared abstraction boundary | fetch, load/store, AMO if in scope, TileLink if tile/SoC boundary included | Handshake cycle or latency window, address/opcode/size/mask/data/ID, response ownership/order, retry/fault, reset behavior |
| Microarchitectural state/timing | Corresponding internal state transitions when the implementation claims those structures | ROB, issue queues, FTQ, predictor, LQ/SQ, cache/TLB if in scope | Mapped state snapshots at agreed sample points plus cycle-level transition/occupancy traces; do not compare unrelated encoding bits |

Architectural equality is mandatory for behaviors supported by both sides. Transaction protocol equality is mandatory at the agreed interface boundary. Microarchitectural state/timing equality applies only to structures explicitly mapped as corresponding; state encoding differences require semantic mapping, not bitwise assumptions.

## Differential Harness Route

1. Freeze the manifest hashes and effective SmallBoomConfig parameters. Build a machine-readable reference module/instance path map from FIRRTL `inst`/connections and generated Verilog module declarations.
2. Choose the boundary explicitly. Initially drive the same cycle-indexed instruction-memory responses, data-memory responses, interrupts, reset and debug inputs into the reference `TestHarness/ChipTop` path and the HLS core's external IMEM/DMEM stream boundary. Add adapters only in the verification harness; do not alter product C++.
3. Use the same directed and constrained-random RV64 instruction streams. Include integer/control-flow/load-store, mispredict/recovery, CSR/trap/interrupt, FP, virtual memory, and AMO/LRSC only when the reference inventory confirms the behavior is actually instantiated and connected.
4. Capture reference Verilog commit/trace signals and HLS `commit_trace_out`; normalize fields (PC, instruction, destination register/value, memory op/address/data/mask, exception/privilege) into a canonical event schema.
5. Compare ordered architectural events and final state. Compare transaction traces at the chosen boundary with transaction IDs mapped by request owner, not presumed equal internal IDs.
6. Add semantic microarchitecture probes only where there is a one-to-one structure mapping: ROB occupancy/allocation/retire, LQ/SQ ownership, FTQ identity/generation and predictor request/update. Record cycle offset and allowed latency windows; strict cycle equality is a separate result.
7. Run negative controls: perturb one expected register, memory response, exception, transaction ID owner, and redirect result in the checker inputs. The differential checker must fail each mutation.
8. Report per-feature PASS/PARTIAL/FAIL/NOT_IN_REFERENCE with trace hashes, simulator/tool versions, source hashes and exact test seeds. Never promote HLS self-test PASS to reference-equivalence PASS.

## Scope Rules

- `reference_feature_inventory.csv` is the authority for existence, based on RTL module definitions plus instance/connection/state/interface evidence.
- A macro, parameter declaration, module name with no reachable instance, or HLS test result alone is insufficient evidence of a feature.
- L2 and SoC/TestHarness integration are outside the current HLS core boundary; retain as `REFERENCE_OUT_OF_SCOPE` unless the requested equivalence boundary is explicitly expanded.
- Where the reference has no feature, mark `REFERENCE_ABSENT` and do not create a new HLS requirement. Where evidence is incomplete, retain `UNRESOLVED`.
- This audit makes no product-source changes and does not claim equivalence PASS.

## Exit Criteria

1. Reference manifest and top/hierarchy are reviewed and immutable for the comparison run.
2. The effective HLS `8_8` parameters and build source hash are tied to SmallBoomConfig's actual reference parameters.
3. Every in-scope feature has a reproducible differential test set, normalized traces, checker results and negative-control PASS.
4. Architectural and transaction mismatches are zero for the selected suite; microarchitectural results are reported separately from architectural PASS.
5. Each remaining unsupported feature is explicitly classified as partial, missing, absent or out of scope. No gate-wide signoff flag is inferred from partial evidence.
