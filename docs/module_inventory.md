# Module Inventory

| Module | File | Status | HLS Status | Notes |
|---|---|---|---|---|
| boom_core_top | src/boom_core_top.cpp | PARTIAL | PASS_GATE3_9_BASELINE | Gate 3.9 F1 fine-grain reset, 47999 LUT; `CORE_CYCLE` unpipelined; XSim 49/49 |
| reset controller | src/reset.cpp | IMPLEMENTED | PASS_LOCAL_PIPELINE_CHARACTERIZATION | Default 145-step Gate 3.9 reset accepted; R1 114-step source/II=1 loop rejected by RTL latency/cycle evidence |
| boom_core_step | src/boom_core_step.cpp | PARTIAL | PASS_STEP_TOP_CSYNTH | Gate 3.3 per-cycle orchestration passes csynth; whole-state copy removed in Gate 3.2 |
| boom_core_ncycle_n1/n2/n4/n8_top | src/boom_core_top.cpp | ANALYSIS_ONLY | PASS_ATTRIBUTION_CSYNTH | Gate 3.6 fixed-trip controls; one retained cycle wrapper, no pipeline/unroll, flat resources; not product replacements |
| frontend | src/frontend.cpp | PARTIAL | PASS_MODULE_CSYNTH | Ideal imem request/response subset; no ICache/FetchBuffer/BP |
| decode | src/decode.cpp | PARTIAL | PASS_MODULE_CSYNTH | DecodeUnit subset; no full decode/RVC coverage |
| rename | src/rename.cpp | PARTIAL | PASS_MODULE_CSYNTH | Integer RenameMapTable + FreeList subset; Gate 3.3 branch tag allocation, masks, snapshots, and allocation-list tracking implemented for supported subset |
| rob | src/rob.cpp | PARTIAL | PASS_MODULE_CSYNTH | 32-entry ROB allocation/completion/flush subset |
| issue | src/issue.cpp | PARTIAL | PASS_MODULE_CSYNTH | Integer IssueUnitCollapsing subset; one implemented ALU execute lane |
| execute | src/execute.cpp | PARTIAL | PASS_MODULE_CSYNTH | ALU/branch/minimal memory address path; no FPU/muldiv timing model |
| branch | src/branch.cpp | PARTIAL | COVERED_BY_STEP_TOP_CSYNTH | Gate 3.3 branch resolution/release, mispredict snapshot restore, free-list rollback, busy rebuild, mask clear, and younger-state kill implemented for supported subset; no predictor metadata |
| lsu | src/lsu.cpp | PARTIAL | PASS_MODULE_CSYNTH | Minimal integer LSU for current load/store subset; no cache/MMU/replay |
| commit | src/commit.cpp | PARTIAL | PASS_MODULE_CSYNTH | ROB commit and committed store request subset |
| csr | src/csr.cpp | PARTIAL | COVERED_BY_STEP_TOP_CSYNTH | CSRFile minimal subset |
| gate3_4_attribution_tops | src/synth_module_tops.cpp | ANALYSIS_ONLY | PASS_MODULE_CSYNTH | Gate 3.4 branch tag/mask/snapshot/rollback/busy/kill attribution tops; not architectural behavior |
| gate3_7_pipeline_solutions | scripts/gate3_7/ | ANALYSIS_ONLY | P0_PASS_P1_TIMEOUT | Independent solution-local pipeline directives; no product source directive and no accepted pipeline RTL |
| gate3_10_local_pipeline | scripts/gate3_10/ | ANALYSIS_ONLY | NO_ACCEPTED_CANDIDATE | Real critical-path extraction, R1 reset-only experiment, exact normal-cycle comparison, and clock sweep |

## Source: FIRRTL Module Line Numbers

| Component | FIRRTL Module | Line |
|---|---|---|
| ICache | ICache | 161443 |
| BranchPredictor | BranchPredictor | 174221 |
| BoomRAS | BoomRAS | 174507 |
| TLB | TLB | 176607 |
| FAMicroBTB | FAMicroBTBBranchPredictorBank | 170247 |
| BoomFrontend | BoomFrontend | 197118 |
| DecodeUnit | DecodeUnit | 231068 |
| RenameMapTable | RenameMapTable | 233063 |
| RenameFreeList | RenameFreeList | 233426 |
| RenameBusyTable | RenameBusyTable | 233891 |
| RenameStage | RenameStage | 233940 |
| RenameStage_1 | RenameStage_1 | 236136 |
| ALUExeUnit | ALUExeUnit | 199668 |
| ALUUnit | ALUUnit | 200154 |
| ALU | ALU | 199940 |
| IssueUnitCollapsing | IssueUnitCollapsing | 224348 |
| IssueSlot | IssueSlot | 219004 |
| Rob | Rob | 259441 |
| CSRFile | CSRFile | 264010 |
| BoomCore | BoomCore | 269856 |
| LSU | LSU | 284375 |
| BoomTile | BoomTile | 297181 |
| BoomWritebackUnit | BoomWritebackUnit | 144934 |
| BoomProbeUnit | BoomProbeUnit | 145175 |
| BoomNonBlockingDCache | BoomNonBlockingDCache | 157099 |
| ChipTop | ChipTop | 394301 |
