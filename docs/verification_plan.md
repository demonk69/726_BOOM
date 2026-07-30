# Verification Plan

## Strategy

1. **Commit trace differential**: Compare HLS model commit trace vs Verilator reference trace
2. **Unit tests**: Per-module directed tests
3. **Random instruction tests**: Random sequences, compare architectural state
4. **RISC-V ISA tests**: Standard compliance tests (rv64ui, rv64um, etc.)

## Current Gate Status

| Gate | Status | Evidence |
|---|---|---|
| Gate 1 integer/control subset | PASS | directed `25/25`, Gate 1 regressions `13/13` |
| Gate 2.5 standalone BOOM traces | PARTIAL PASS | finite loadmem-backed traces; official simulator path blocked |
| Gate 3.1C minimal LSU architectural diff | PASS for supported subset | LSU `14/14`, full loaded-program diff `10/10` |
| Gate 3.2 conservative baseline csynth | PASS | `boom_core_top` csynth 45.56s, 5.898 ns estimated period |
| Gate 3.2 performance pipeline csynth | BLOCKED | `BOOM_HLS_ENABLE_CORE_PIPELINE=1` timed out after 15 minutes |
| Gate 3.8 generated RTL | FAIL: `RTL_RESET_MISMATCH` | XSim 42/45; all tested AXIS backpressure passes, three mid-run reset interactions fail |
| Gate 3.9 generated RTL | PASS: `RTL_RESET_VERIFIED` | XSim 49/49; reset architecture 14/14; M014 verified |
| Gate 3.10 local pipeline | COMPLETE: no accepted candidate | R1 II=1 and XSim 49/49, rejected latency/cycles; L1-L5 illegal |
| Official Gate 3 | BLOCKED | original Chipyard/FESVR/DRAMSim path unavailable |

## Trace Format

CSV format, one entry per committed instruction:
```
cycle,commit_slot,pc,instruction,privilege,rd_valid,rd,rd_value,exception,exception_cause,memory_valid,memory_address,memory_data,memory_mask,branch_mispredict
```

## Reference Trace Generation

- Use Verilator model of SmallBoomConfig
- Add commit-logging VPI/DPI module or use existing commit interface signals
- Output cycle-by-cycle commit information

## Differential Comparison

For each commit event from reference:
1. Wait for HLS model to produce same commit event
2. Compare PC, instruction, rd, rd_value
3. Compare architectural register file state at key checkpoints
4. Report mismatches with cycle/instruction context

## Test Programs

### M0 Tests
- Empty program (infinite NOP loop or wfi)
- Verify C++ compilation and HLS flow

### M1 Tests
- rv64ui-p-add, rv64ui-p-addi, rv64ui-p-addiw
- rv64ui-p-sub, rv64ui-p-slt, rv64ui-p-sltu
- rv64ui-p-sll, rv64ui-p-srl, rv64ui-p-sra
- rv64ui-p-and, rv64ui-p-or, rv64ui-p-xor
- rv64ui-p-lui, rv64ui-p-auipc
- rv64ui-p-jal, rv64ui-p-jalr
- rv64ui-p-beq, rv64ui-p-bne, rv64ui-p-blt
- rv64um-p-mul, rv64um-p-mulh
- Random instruction sequences (100-1000 instructions)

### M2 Tests
- rv64ui-p-simple (uses all basic instructions)
- rv64ui-p-ma_data (load/store to test LDQ/STQ)
- Short programs with branches (test ROB flush, branch recovery)
- Exception test (ecall in M-mode)
- CSR read/write test

## Quality Gates

### Gate 3.8 RTL
- [x] Build accepted conservative generated RTL with XSim
- [x] Compare seven normal-program C++/csim/RTL architectural traces
- [x] Exercise commit, IMEM, and DMEM AXIS backpressure
- [x] Exercise power-on and targeted mid-run reset scenarios
- [ ] Restore the reset vector after every mid-run reset
- [ ] Empty or coherently invalidate all in-flight architectural state on runtime reset

Gate 3.8 remains failed until the last two requirements pass. See `reports/gate3_8/gate3_8_results.md`.

### Gate 3.10 Local Pipeline
- [x] Freeze Gate 3.9 commit and evidence hashes
- [x] Extract current HLS state-local critical paths
- [x] Classify state recurrence, handshake, and recovery legality
- [x] Run R1 through source regressions, csim, csynth, and XSim 49/49
- [x] Compare normal external event cycles
- [x] Scan 6.0/5.5/5.0/4.5 ns requested targets
- [x] Keep `CORE_CYCLE` unpipelined
- [x] Reject all cycle-inexact candidates

### Gate M0
- [ ] boom_core_step compiles with g++
- [ ] boom_core_top compiles with g++
- [ ] Vitis HLS csim_design passes
- [ ] Vitis HLS csynth_design passes

### Gate M1
- [ ] All decode unit tests pass
- [ ] All ALU unit tests pass
- [ ] 100 random RV64I instructions produce correct results
- [ ] Vitis HLS csim_design passes
- [ ] Vitis HLS csynth_design passes

### Gate M2
- [ ] Rename unit tests pass (map, free list, busy table, snapshots)
- [ ] ROB unit tests pass (empty, full, allocate+commit, wrap, flush, exception)
- [ ] Issue queue unit tests pass (wakeup, select, grant, compress)
- [ ] CSR unit tests pass (mstatus, misa, mtvec, mepc, mcause)
- [ ] Short program (10-50 instructions) produces correct commit trace
- [ ] Vitis HLS csim_design passes
- [ ] Vitis HLS csynth_design passes
