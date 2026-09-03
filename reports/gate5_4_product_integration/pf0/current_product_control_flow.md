# Current Product Control Flow

This is the canonical separately compiled source path. `src/boom_all.cpp` is excluded.

| Stage | Input and output | State/PC owner | Metadata and lifetime | Flush behavior |
|---|---|---|---|---|
| IMEM request | `fe.pc`, carry state -> aligned address, fetch ID, epoch | Frontend owns PC and one pending request tuple | No branch prediction/FTQ metadata | Redirect clears logical outstanding state; physical stale response is drained |
| IMEM response | response tuple -> held response | Frontend owns response payload | Exact address/fetch ID/epoch match rejects stale data | Redirect dominates same-step response |
| RV64C/cross-word | response + optional parcel carry -> 0/1/2 complete canonical instructions | Frontend owns carry PC/epoch; each instruction owns full PC | Fetch fault, original instruction, canonical instruction, `is_rvc`; carry has no architectural token | Redirect/reset clears carry and response |
| Fetch Packet | complete instructions -> mask `01` or `11` and next PC | `build_fetch_packet`; Frontend commits next PC | No predecode, prediction, generation, or FTQ ref | Packet pending is discarded |
| Fetch Buffer | atomic packet enqueue -> scalar dequeue | 8-entry FIFO owns `FetchInstruction` payload | Full PC/fault survive; packet grouping is lost after compacted enqueue | CONTROL_ONLY clears head/tail/count |
| Decode | scalar FetchInstruction -> MicroOp | MicroOp receives `debug_pc` | Fetch fault overrides decode exception; fetch ID is dropped; inert `ftq_idx` is not produced | Decode valid is cleared on frontend redirect |
| Rename/dispatch | MicroOp -> mapped uop and branch snapshot | Rename map/free-list/branch allocator | Control uop gets branch tag; younger uops carry branch mask | Mispredict restores owner snapshot and kills dependents |
| ROB | renamed uop -> entry with 32-bit allocation ID | ROB owns in-order lifetime and copied full uop | Full PC and exception survive; no actual outcome/target or prediction fields | Branch recovery retains owner, removes younger suffix |
| Execute | operands/uop -> execution result | Execute transient result | Branch computes actual condition and target; `mispredict` is overloaded as actual-taken under implicit NT; JAL/JALR always true | Killed by branch mask/owner checks |
| Completion/branch | result -> ROB completion and transient BranchUpdate | Completion validates ROB allocation ID | Actual outcome/target do not get written into ROB | Taken branch/JAL/JALR triggers immediate recovery |
| Commit | ROB head -> trace/store/exception terminal state | ROB head and committed rename map | No predictor update; normal trace forces branch_mispredict false | Non-ECALL exception remains at head and enters `ROB_EXCEPTION` |
| Redirect/reset | architectural redirect, branch update, flush -> frontend reset of transient state | Frontend priority arbitration; branch recovery also mutates frontend | Frontend epoch protects IMEM/carry only; branch path increments it twice | Priority: runtime reset/initial redirect, architectural redirect, branch recovery, generic/local flush |

## Exact Current Sequence

`IMEM request -> matched response -> canonical RV64C/cross-word packet -> pending packet -> atomic Fetch Buffer enqueue -> scalar Decode -> Rename/Dispatch -> ROB -> Issue/Execute -> Completion/branch resolution -> Commit -> redirect or terminal exception fence`.

The core step services completion and branch recovery before Commit and Frontend, then Decode/Rename/ROB/Issue/Execute. There is one logical outstanding IMEM request, but a request, a captured response, a pending packet, and older Fetch Buffer entries can coexist. No current product Predictor or FTQ is instantiated.

## Canonical Source Evidence

- Core order: `src/boom_core_step.cpp:22-43`.
- Request/response matching, redirect priority, pending packet and request issuance: `src/frontend.cpp:67-225`.
- Canonical RVC/cross-word packet construction: `src/fetch_packet.cpp:73-135`.
- Atomic FIFO admission: `src/fetch_buffer.cpp:46-80`.
- Full-PC and fault transfer to Decode: `src/decode.cpp:103-127,268-272`.
- ROB allocation identity and copied uop: `src/rob.cpp:29-54`.
- Actual branch computation: `src/execute.cpp:155-164`.
- Recovery and transient BranchUpdate: `src/branch.cpp:263-319`.
- Terminal exception/normal Commit: `src/commit.cpp:41-115`.
