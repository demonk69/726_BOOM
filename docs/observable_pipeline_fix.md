# Observable Pipeline Fix Report

## Problem

Vitis HLS 2021.2 csynth eliminated the entire BOOM core pipeline (86 LUT, 6 FF) because no internal computation affected any top-level output.

## Root Cause

1. `io_success` always false → no observable success condition
2. No `commit_trace_out` write path → commit logic eliminated
3. No `imem_req_out` driven by real fetch → frontend eliminated
4. `io_cycle`/`io_instret` not exposed → CSR counters eliminated
5. All internal state had no path to any output port

## Fixes Applied

### Top-Level Interfaces (`boom_core_top.cpp`)
- `imem_req_out`: IMEM request stream (address, fetch_id)
- `imem_resp_in`: IMEM response stream (instruction, fetch_id)
- `commit_trace_out`: Commit event stream (pc, inst, rd, rd_value)
- `io_success`: ECALL with a0=0 auto-detected test pass
- `io_trap`: Exception / ECALL with a0!=0
- `io_cycle`: Live CSR cycle counter
- `io_instret`: Live CSR instret counter

### Observable Data Path
```
frontend.fetch_request → imem_req_out
imem_resp_in → frontend.fetch_packet
fetch_packet → decode → renamed_uop
renamed_uop → rob_allocate → rob_idx
renamed_uop → issue_queue → selected_uop
issued_uop → execute → alu_result → int_rf writeback
alu_result → rob_writeback → rob commit
rob commit → commit_trace_out
rob commit → instret++
csr cycle → io_cycle
```

### Pipeline Reorder
Moved `rob_allocate` BEFORE `issue/execute` so rob_idx propagates through the pipeline correctly for writeback matching.

### State Update
All modules use `next_state` copy, only committed at end of `boom_core_step`. Same-cycle events handled via sequential call order within `next_state`.

### ECALL Success Detection
ECALL reaching ROB head with a0=0 sets `io_success=true`. a0!=0 sets `io_trap=true`.

## Results

| Metric | Before | After |
|---|---|---|
| LUT | 86 (~0%) | 133,537 (57%) |
| FF | 6 (~0%) | 49,182 (10%) |
| BRAM | 0 | 57 (9%) |
| DSP | 0 | 3 (~0%) |
| Fmax | 611 MHz | 92 MHz |
| CORE_CYCLE pipelined | yes (II=1) | no |
| Test programs pass | 0/5 | 5/5 |

## Module Retention

| Module | Type | LUTs | FFs |
|---|---|---|---|
| frontend_module | pipelined | 1,746 | 1,260 |
| decode_module | pipelined | 2,196 | - |
| rename_module | II=18 | 4,312 | 7,111 |
| issue_module | loop-based | 35,065 | 9,246 |
| execute_module | loop-based | 6,132 | 1,903 |
| rob_allocate | pipelined | 112 | - |
| rob_commit_module | II=32 | 1,814 | 1,288 |

## Remaining Issues

- Timing violation (-3.55ns slack, Fmax 92 MHz vs target 10ns/100MHz)
- CORE_CYCLE not pipelined (single-cycle latency too large for II=1)
- rename_module II=18 due to sequential loop over map table
- rob_commit_module II=32 due to sequential ROB traversal
- Need ARRAY_PARTITION on map table, free list, busy table, IQ entries
- Need PIPELINE directives on execute and issue loops

## Next Steps

1. Add ARRAY_PARTITION for rename structures
2. Pipeline decode/rename/issue/execute/commit loops
3. Target CORE_CYCLE II=1 with timing closure
4. Implement branch predictor (stub currently)
5. Add backpressure handling for IMEM latency
