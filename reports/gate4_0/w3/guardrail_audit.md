# Gate 4.0 W3 Synthesis Guardrail Audit

- CORE_CYCLE pipeline: unpipelined (pipeline pragma remains behind an undefined opt-in macro)
- DISPATCH_WIDTH: 1
- COMMIT_WIDTH: 1
- Capacity and field-width checks: PASS (27 exact checks)
- Forbidden pipeline/dataflow/array-partition/false-dependence overrides: none active
- Synthesis C flags: `-std=c++11 -I<root>/include` only; no extra feature defines

## Checked Configuration

| Macro | Required | Observed |
|---|---:|---:|
| FETCH_WIDTH | 4 | 4 |
| DECODE_WIDTH | 1 | 1 |
| DISPATCH_WIDTH | 1 | 1 |
| ISSUE_WIDTH | 3 | 3 |
| COMMIT_WIDTH | 1 | 1 |
| ROB_DEPTH | 32 | 32 |
| ROB_IDX_BITS | 5 | 5 |
| INT_PHYS_REGS | 52 | 52 |
| FP_PHYS_REGS | 48 | 48 |
| PHYS_REG_BITS | 6 | 6 |
| LOGICAL_REG_COUNT | 32 | 32 |
| ISSUE_QUEUE_MEM_DEPTH | 8 | 8 |
| ISSUE_QUEUE_ALU_DEPTH | 8 | 8 |
| ISSUE_QUEUE_FPU_DEPTH | 8 | 8 |
| ISSUE_QUEUE_IDX_BITS | 3 | 3 |
| LDQ_DEPTH | 8 | 8 |
| STQ_DEPTH | 8 | 8 |
| LDQ_IDX_BITS | 3 | 3 |
| STQ_IDX_BITS | 3 | 3 |
| MAX_BRANCH_COUNT | 8 | 8 |
| BR_MASK_BITS | 8 | 8 |
| BR_TAG_BITS | 3 | 3 |
| FTQ_DEPTH | 16 | 16 |
| FTQ_IDX_BITS | 4 | 4 |
| FETCH_BUFFER_DEPTH | 8 | 8 |
| PADDR_BITS | 32 | 32 |
| VADDR_BITS | 39 | 39 |

## Result

PASS

## Post-Synthesis Audit

- All five canonical XML reports identify Vitis HLS 2021.2, `xczu7ev-ffvc1156-2-e`, a 10 ns target, and `PipelineType=no`.
- `boom_core_top` contains `CORE_CYCLE` without a `PipelineII` element.
- Every generated preprocessed source and solution directive was checked for active pipeline, dataflow, array-partition, and dependence overrides; none was found.
- Production source hashes were unchanged across the five synthesis runs.
