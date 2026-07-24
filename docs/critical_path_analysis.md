# Critical Path Analysis

Gate 1 did not perform timing optimization. The previous Vitis HLS 2021.2 csynth evidence remains the timing reference.

## Current Status

- Target clock: 10.00 ns (100 MHz)
- Previous estimated period: 10.854 ns (92.13 MHz)
- Previous slack: violated
- Gate 1 build evidence: merged C++ compile PASS, Vitis HLS csim PASS 5/5

## Known Critical Areas

| Area | Cause | Gate 1 Change |
|---|---|---|
| `rename_module` | Sequential map/free-list loops rather than parallel hardware reads | Functional source-map semantics fixed only |
| `rob_commit_module` | Loop-based ROB processing | No timing change |
| `issue_module` | Loop-based IQ select/compact | Grant cap added to prevent dropped uops; no directive applied |
| `execute_module` | ALU/branch path plus single implemented result lane | No timing change |
| `boom_core_step` | Large `next_state = state` copy and serialized module calls | No timing change |

## Deferred Safe Optimizations

Do not apply these until Gate 2 and Gate 3 provide trace evidence:

- `ARRAY_PARTITION` or `ARRAY_RESHAPE` for map tables, busy table, PRF, ROB, and IQ arrays.
- `UNROLL` for real lane-parallel fixed loops.
- Tree priority encoder for IQ selection.
- `PIPELINE II=1` only after state dependencies are proven safe.

Cycle equivalence remains INSUFFICIENT_EVIDENCE, so no timing optimization result is claimed in Gate 1.
