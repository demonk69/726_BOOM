# Exception Recovery Integration Audit

```text
CURRENT_EXCEPTION_RECOVERY_IMPLEMENTED=false
EXCEPTION_RECOVERY_BLOCKS_PRODUCT_INTEGRATION=true
```

Current fetch/decode/LSU faults reach the ROB. A non-ECALL exception at the head emits one report, enters `ROB_EXCEPTION`, leaves the owner at head, clears/stops the Frontend, and repeatedly asserts trap. It does not capture `mepc`, `mcause`, or `mtval`; calculate `mtvec`; generate an architectural redirect; squash all younger backend state; restore committed rename state; remove the owner; or support MRET/post-trap execution. ECALL is a separate host-exit behavior, not architectural trap entry.

## Minimum Foundation Gap

- Capture EPC/cause/tval from the validated ROB-head owner before invalidation.
- Define trap CSR/status/privilege update and direct/vectored `mtvec` target.
- Atomically squash younger ROB/IQ/execute/completion/LSU/rename/branch state.
- Generate a validated architectural Frontend redirect and resume fetching.
- Define fault-owner removal and MRET return semantics.
- Gate younger allocation/issue while trap transition is pending.
- Correct fetch-fault/breakpoint classifications required by the supported contract.

For FTQ, the faulting owner entry must remain readable until EPC/cause capture and redirect ownership are complete. All younger FTQ entries are killed. The faulting lane is then retired/reclaimed exactly once according to explicit trap ordering. Integrating FTQ first would bake terminal-exception behavior into references and reclaim, causing recovery rework.

Therefore the unique next gate is Exception Recovery Foundation. Predictor frontend integration can follow after redirect/generation ownership is stable.

Canonical evidence: `src/commit.cpp:41-65` enters terminal `ROB_EXCEPTION`; `src/frontend.cpp:141-150` fences fetch; `src/csr.cpp:7-9` only increments cycle; `src/reset.cpp:227-241` only initializes trap CSRs; `src/frontend.cpp:11-18` requires redirect-owner validation; and `src/decode.cpp:247-253` decodes ECALL/EBREAK/MRET without a complete trap/return path.
