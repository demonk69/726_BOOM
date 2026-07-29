# Gate 3.8 RTL Reset Semantics Analysis

Date: 2026-07-28

## Verdict

Status: `RTL_RESET_MISMATCH`.

The accepted conservative RTL does not implement a coherent runtime processor reset. `ap_rst_n` resets generated control FSMs, AXIS register slices, FIFO occupancy, selected frontend-private registers, branch-update valid flags, and the CSR cycle counter. Most architectural state is only initialized at elaboration and survives a later assertion of `ap_rst_n`.

## Generated RAM Behavior

Architectural arrays have a `reset` input, but their clocked process does not use it. The representative ROB-valid RAM in `reports/gate3_8/baseline_artifacts/conservative_rtl/boom_core_top_boom_core_cycle_io_state_rob_entries_valid_RAM_AUTO_1R1W.v:17-35` initializes storage with `$readmemh` and subsequently changes it only on write enable. The same pattern is used for ROB payload, IQ, issued uops, rename maps, free/busy state, branch snapshots, allocation lists, RF, execute results, and load/store queues.

Therefore `.reset(ap_rst)` connectivity is not evidence that those arrays support runtime reset. Their initialization files establish simulation/power-on state only.

## Failure Analysis

### R2_RESET_ROB_NONEMPTY

The test asserts reset while `state_rob_head != state_rob_tail`. After reset, the RTL accepts 26 instruction fetches but produces zero commits and times out at 40000 cycles.

The reset restarts the HLS control FSMs while retaining ROB pointers, ROB valids and payloads, IQ and issued-uop valids, rename maps, free/busy state, execute-result valids, queue state, and pending memory bookkeeping. This can leave stale ownership and dependencies with no live producer. The evidence establishes an incoherent post-reset machine and explains the retirement deadlock; identifying the first blocking retained bit would require waveform-level diagnosis and is not necessary to classify the reset contract as failed.

Evidence: `reports/gate3_8/logs/raw_chain_R2_RESET_ROB_NONEMPTY.log` and `reports/gate3_8/rtl_test_matrix.csv`.

### R6_RESET_BRANCH_RECOVERY and P0_RESET_AND_BRANCH_MISPREDICT

Both tests report the first fetch after mid-run reset as `0x0000000080000000`, while the required reset vector is `0x0000000000010040`.

The direct cause is the frontend PC. It is initialized to decimal 65600 (`0x10040`) in `boom_core_top_boom_core_cycle_io.v:8718`, but its update block at lines 23308-23314 has no reset branch. Runtime reset consequently retains the redirected pre-reset PC. Retained active branch masks, snapshots, allocation lists, and execute-result valids are additional recovery hazards, but the first-fetch mismatch alone proves the reset failure.

Evidence: `reports/gate3_8/logs/branch_taken_R6_RESET_BRANCH_RECOVERY.log`, `reports/gate3_8/logs/branch_taken_P0_RESET_AND_BRANCH_MISPREDICT.log`, and `reports/gate3_8/rtl_test_matrix.csv`.

## Passing Reset Cases

R0, R1, R3, R4, R5, R7, I6, D7, P1, and P2 pass. These cases prove specific observed interactions, not complete reset coverage. A valid-based or control-only reset cannot be generalized to the whole machine because R2, R6, and P0 provide concrete counterexamples.

## Required Closure

Gate 3.8 cannot pass until generated RTL provides a coherent runtime reset for all architecturally live state, including the frontend PC and validity, ROB/IQ/rename/branch/execute/LSU state, or until an explicitly specified external reset protocol recreates the core and is verified at the integration boundary. Removing the whole-state source reset to reduce LUT remains invalid because that would weaken, not repair, the required behavior.
