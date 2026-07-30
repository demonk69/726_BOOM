# BOOM HLS Architecture

## Overview

This project implements a Vitis HLS synthesizable C++ model of the
BOOM (Berkeley Out-of-Order Machine) RISC-V processor, configured as
**SmallBoomConfig** within the Chipyard SoC framework.

## Configuration

| Parameter | Value | Source |
|---|---|---|
| ISA | RV64IMAFDC | core.config:96-98, DTS:32 |
| MMU | Sv39 | core.config:98, DTS:29 |
| Fetch Width | 4 | core.config:67 |
| Decode Width | 1 | core.config:68 |
| Issue Width | 3 | core.config:69 |
| Commit Width | 1 | core.config:59-60 |
| ROB Depth | 32 | core.config:60, FIRRTL Rob:259475 |
| Int Phys Regs | 52 | core.config:73, FIRRTL FreeList |
| FP Phys Regs | 48 | core.config:74 |
| IQ Mem/ALU/FPU | 8/8/8 | core.config:71 |
| LDQ/STQ | 8/8 | core.config:72, FIRRTL ldq_idx:3b |
| Max Branch Count | 8 | core.config:75, FIRRTL br_mask:8b |
| FTQ Depth | 16 | FIRRTL ftq_idx:4b |
| Fetch Buffer | 8 | top.v fb_uop_ram_0-7 |
| Branch Pred | TAGE 14KB | core.config:17-31 |
| ICache | 16KB 4-way | core.config:6-14, DTS:25-26 |
| DCache | 16KB 4-way | core.config:86-87, DTS:17-19 |
| L2 Cache | 512KB 8-way | DTS:62-65, l2.json |
| Phys Addr Bits | 32 | core.config:93 |
| Virt Addr Bits | 39 | core.config:94 |
| Reset Vector | 0x10040 | dromajo_params.h:4 |
| Clock | 100MHz | freq-summary files |
| Main Memory | 256MB @ 0x80000000 | DTS:48-50, memmap.json |

## Pipeline Structure

```
Cycle N:
  Frontend -> Decode -> Rename -> Dispatch -> Issue -> RegRead -> Execute -> Writeback -> Commit
                                    ^                                      |
                                    |----------- Wakeup ------------------+
                                    |
                                    +----------- Branch Resolution ------+
```

### State Machine
- All pipeline state is held in BoomCoreState
- Each cycle updates the persistent BoomCoreState in serialized module order; the Gate 3.2 baseline no longer copies the whole state into a `next_state` temporary
- Branch-mask snapshot recovery is implemented for the supported subset; exception/global flush behavior remains coarse

### Gate 4.0 Issue Lanes

| Lane | Class | W2 behavior |
|---:|---|---|
| 0 | MEM | Oldest ready supported integer load/store candidate |
| 1 | INT | Oldest ready supported integer ALU/multiply candidate |
| 2 | FP | Reserved and always invalid; FP queue/path is not implemented |

The reference `IssueWidth=3` does not imply three integer lanes. W2 has an integer selection width of two but retains an acceptance and execute intake width of one. A second generated grant is held in the IQ under the conservative acceptance budget; W2 makes no dual-execution claim.

## Module Hierarchy

```
boom_core_top (ap_ctrl_none, CORE_CYCLE loop)
  boom_core_step (one cycle)
    ├── frontend (ICache + FetchBuffer + BranchPredictor stub)
    ├── decode  (DecodeUnit + BranchMaskGeneration)
    ├── rename  (RenameMapTable + FreeList + BusyTable)
    ├── issue   (shared implemented IQ, fixed MEM/INT selection lanes + reserved FP lane)
    ├── execute (single accepted integer intake + FPU stub)
    ├── branch  (Branch resolution + brupdate generation)
    ├── lsu     (minimal integer Load/Store Queue subset)
    ├── commit  (ROB commit + Writeback)
    └── csr     (CSR File minimal)
```

## Memory Model

- Ideal instruction memory: 1-cycle response, valid/ready interface
- Ideal data memory: 1-cycle response, valid/ready interface
- Backpressure configurable for testing

## Verification

- Commit trace differential comparison vs Verilator reference model
- Format: cycle,pc,inst,rd,rd_val,exception per committed instruction
- Gate 3.2 conservative baseline Vitis HLS csynth passes for `boom_core_top`; strict cycle equivalence remains insufficient-evidence
- Gate 4.0 W2 dual MEM/INT selection passes source differential tests and generated-RTL verification; accepted execution remains single-uop

## Current Implementation Status

See docs/implementation_status.md
