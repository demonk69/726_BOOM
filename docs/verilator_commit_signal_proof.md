# Verilator Commit Signal Proof

Date: 2026-07-24

Status: VERIFIED_FOR_STANDALONE_GENERATED_MODEL_COMMIT_LANE_0

## Signal Chain

| Layer | Signal | Width | Evidence |
|---|---|---:|---|
| Chisel source intent | BOOM ROB `io.commit.valids(0)` | 1 | Generated annotations reference `rob.scala`; source checkout is unavailable. |
| Generated Verilog ROB output | `io_commit_valids_0` | 1 | `chipyard.TestHarness.SmallBoomConfig.top.v:260809` declares the ROB commit output. |
| Generated Verilog condition | `io_commit_valids_0 = can_commit_0 & ~can_throw_exception_0 & ~block_commit` | 1 | `chipyard.TestHarness.SmallBoomConfig.top.v:265813`. |
| ROB head valid condition | `rob_head_vals_0` | 1 | `chipyard.TestHarness.SmallBoomConfig.top.v:261805`. |
| ROB head not-busy condition | `can_commit_0 = rob_head_vals_0 & ~_GEN_6900 & ~io_csr_stall` | 1 | `chipyard.TestHarness.SmallBoomConfig.top.v:261869`; `_GEN_6900` selects the current ROB head busy bit. |
| Exception block condition | `can_throw_exception_0 = rob_head_vals_0 & _GEN_6868` | 1 | `chipyard.TestHarness.SmallBoomConfig.top.v:261933`; `_GEN_6868` selects the current ROB head exception bit. |
| Commit block condition | `block_commit` | 1 | `chipyard.TestHarness.SmallBoomConfig.top.v:261936`. |
| BoomCore wiring | `.io_commit_valids_0(rob_io_commit_valids_0)` | 1 | `chipyard.TestHarness.SmallBoomConfig.top.v:278677`. |
| Verilator C++ field | `...boom_tile__DOT__core__DOT__rob_io_commit_valids_0` | 1 | `VTestHarness.h:8778`. |
| Runner condition | `if (CORE_SIG(rob_io_commit_valids_0))` | 1 | `reference/verilator_trace/standalone_boom_trace.cpp`. |

## Commit Uop Fields

| Field | Verilator field | Width | Use |
|---|---|---:|---|
| FTQ index | `rob_io_commit_uops_0_ftq_idx` | 4 | Reconstruct commit PC from FTQ base. |
| PC low bits | `rob_io_commit_uops_0_pc_lob` | 6 | Reconstruct commit PC low offset. |
| Physical destination | `rob_io_commit_uops_0_pdst` | 6 | Read integer physical register file for committed rd value. |
| Stale physical destination | `rob_io_commit_uops_0_stale_pdst` | 6 | Record stale destination for rename/free-list evidence. |
| Architectural destination | `rob_io_commit_uops_0_ldst` | 6 | Record architectural rd and maintain the standalone architectural shadow. |
| Architectural destination valid | `rob_io_commit_uops_0_ldst_val` | 1 | Record rd validity and update architectural shadow only for valid non-x0 writes. |
| Architectural commit valid | `rob_io_commit_arch_valids_0` | 1 | Record BOOM architectural-valid sideband. |
| Exception valid | `rob_io_com_xcpt_valid` | 1 | Record exception sideband. |

These fields are declared in `VTestHarness.h:8778` through `VTestHarness.h:8794` and wired from the ROB instance at `chipyard.TestHarness.SmallBoomConfig.top.v:278677` through `chipyard.TestHarness.SmallBoomConfig.top.v:278711`.

## Why This Represents Retirement

The runner records commit records only after reset deassertion and only once per simulated full cycle. `full_cycle()` calls `eval()` at clock 0 and clock 1, then calls `trace_cycle()` once. `trace_cycle()` has an additional `g_last_recorded_cycle` guard, so accidental duplicate calls for the same `g_cycle` are ignored.

The commit predicate is the ROB commit output, not decode valid, ROB enqueue, ROB entry valid, writeback, or LSU commit sideband. The generated ROB equation requires a valid ROB head, not busy, not exception-throwing, and not blocked. Therefore a `commit` event corresponds to BOOM lane-0 retirement for this SmallBoomConfig generated model.

## Noninterference

The standalone runner does not assign to BOOM pipeline control, ready/valid, ROB, rename, or issue state. Trace variables such as `trace_sequence_id`, metadata state, and the architectural register shadow are local C++ runner state. The only intentional standalone injections remain hart wake/assert-stop controls and the local DPI memory/serial/JTAG/UART environment, which are documented as standalone-environment differences.

## Termination Detection

The original memory-only `tohost` watch did not trigger because BOOM's cache/store path can retain the store internally in the standalone environment. Gate 2.5 therefore added a commit-time `tohost` monitor: when a real retired store instruction is observed, the runner decodes the store, uses a local architectural shadow built only from previous committed `rd_value`s, computes the store address, and emits a `tohost` event only if the retired store targets `0x80000080` with a nonzero value.

This termination detector depends on the same verified commit predicate above. It does not mark a program complete based on fetch/decode, predicted control flow, or expected test outcome.

## Remaining Limitations

- Only commit lane 0 is currently traced; this generated SmallBoomConfig exposes one ROB commit lane in the used trace path.
- Commit instruction is reconstructed from the loadmem image when PC maps into the standalone program; boot ROM instruction encodings remain `null`.
- Commit rd value is read from the exposed physical integer register file rather than from an explicit BOOM trace port.
- Rename source physical mappings are not exposed as stable trace ports.
- The proof applies to the existing generated `VTestHarness` model and does not replace official Chipyard/FESVR/DRAMSim noninterference.
