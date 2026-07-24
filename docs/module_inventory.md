# Module Inventory

| Module | File | Status | HLS Status | Notes |
|---|---|---|---|---|
| boom_core_top | src/boom_core_top.cpp | STUB | PENDING | Top-level wrapper with CORE_CYCLE loop |
| boom_core_step | src/boom_core_step.cpp | STUB | PENDING | Per-cycle orchestration |
| frontend | src/frontend.cpp | STUB | PENDING | ICache + FetchBuffer + BP stub |
| decode | src/decode.cpp | STUB | PENDING | DecodeUnit stub |
| rename | src/rename.cpp | STUB | PENDING | RenameMapTable + FreeList stub |
| rob | src/rob.cpp | STUB | PENDING | ROB 32-entry stub |
| issue | src/issue.cpp | STUB | PENDING | IssueUnitCollapsing stub |
| execute | src/execute.cpp | STUB | PENDING | ALUExeUnit stub |
| branch | src/branch.cpp | STUB | PENDING | Branch resolution stub |
| lsu | src/lsu.cpp | STUB | PENDING | LSU stub (M4+) |
| commit | src/commit.cpp | STUB | PENDING | ROB commit stub |
| csr | src/csr.cpp | STUB | PENDING | CSRFile minimal stub |

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
