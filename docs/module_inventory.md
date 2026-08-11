# Module Inventory

| Module | File | Status | HLS Status | Notes |
|---|---|---|---|---|
| boom_core_top | src/boom_core_top.cpp | PARTIAL | PASS_GATE5_1R_CSYNTH | 124317 LUT, 27513 FF, 16 BRAM_18K, 3 DSP, 6.341 ns; `CORE_CYCLE` unpipelined; Gate 5.1R full regression accepted |
| reset controller | src/reset.cpp | IMPLEMENTED | PASS_LOCAL_PIPELINE_CHARACTERIZATION | Default 145-step Gate 3.9 reset accepted; R1 114-step source/II=1 loop rejected by RTL latency/cycle evidence |
| boom_core_step | src/boom_core_step.cpp | PARTIAL | PASS_GATE5_1R_CANONICAL_WRAPPER_CSYNTH | Raw function is not a product top; canonical `synth_core_step_top` is 116835 LUT, 27106 FF, 16 BRAM, 3 DSP, 6.341 ns |
| boom_core_ncycle_n1/n2/n4/n8_top | src/boom_core_top.cpp | ANALYSIS_ONLY | PASS_ATTRIBUTION_CSYNTH | Gate 3.6 fixed-trip controls; one retained cycle wrapper, no pipeline/unroll, flat resources; not product replacements |
| frontend | src/frontend.cpp | VERIFIED_GATE5_1R | PASS_MODULE_CSYNTH | One outstanding request with ID/epoch/address match, redirect ownership, fault/alignment handling, and one-entry handoff; verification RTL 33/33 and throughput repair accepted; no RVC/Fetch Buffer/FTQ/predictor/ICache |
| decode | src/decode.cpp | PARTIAL_M1_RV64M_VERIFIED | PASS_M1_CSYNTH | Exact decode for all 13 RV64M operations and strict OP/OP-32 legality; arithmetic, RVC, and full decode coverage remain incomplete |
| rename | src/rename.cpp | PARTIAL | PASS_MODULE_CSYNTH | Integer RenameMapTable + FreeList subset; Gate 3.3 branch tag allocation, masks, snapshots, and allocation-list tracking implemented for supported subset |
| rob | src/rob.cpp | PARTIAL | PASS_W4E_FINAL_CSYNTH | 32-entry ROB allocation/flush subset; up to three completion sources validated |
| issue | src/issue.cpp | PARTIAL_M3B_VERIFIED | PASS_M3B_CSYNTH | Fixed MEM/INT grants; busy DIV requests retain IQ ownership while ordinary INT work may proceed; FP lane inactive |
| execute | src/execute.cpp | PARTIAL_M3B_VERIFIED | PASS_M3B_CSYNTH | Concurrent fixed MEM/INT lanes; persistent radix-2 Divider publishes through the existing INT result; 6773 LUT, 916 FF, 8 BRAM, 3 DSP, 6.411 ns |
| divider | src/divider.cpp | IMPLEMENTED_M3B | PASS_M3B_CSYNTH | Unsigned restoring radix-2 core, 64/32 iterations, fast returns, held response; 3429 LUT, 344 FF, 0 BRAM/DSP, 5.734 ns |
| completion | src/completion.cpp | PARTIAL | PASS_W4E_FINAL_CSYNTH | Three named pending slots, three publications, and waiver-free two-bank/LVT PRF writeback at II=1 |
| branch | src/branch.cpp | PARTIAL | COVERED_BY_STEP_TOP_CSYNTH | Gate 3.3 branch resolution/release, mispredict snapshot restore, free-list rollback, busy rebuild, mask clear, and younger-state kill implemented for supported subset; no predictor metadata |
| lsu | src/lsu.cpp | PARTIAL | PASS_W4E_FINAL_CSYNTH | Minimal integer LSU for current load/store subset; no cache/MMU/replay or full ordering |
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
