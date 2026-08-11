# Frontend Redirect Contract Proposal

This is a Gate 5.0 design contract only. Canonical types are unchanged.

```cpp
enum RedirectCause {
    REDIRECT_RESET,
    REDIRECT_DEBUG,
    REDIRECT_EXCEPTION,
    REDIRECT_INTERRUPT,
    REDIRECT_ERET,
    REDIRECT_FENCEI,
    REDIRECT_BRANCH_MISPREDICT
};

struct FrontendRedirect {
    bool valid;
    uint64_t target_pc;
    RedirectCause cause;
    uint32_t rob_idx;
    uint32_t allocation_id;
    uint32_t branch_mask;
};
```

Priority is reset, debug (if implemented), exception/interrupt, ERET, FENCE.I/refetch, branch/JAL/JALR, then sequential response. Reset is highest. Exception wins over a simultaneous branch redirect because it represents an older precise architectural event; ownership must be validated by ROB index and allocation ID before arbitration.

Every accepted redirect invalidates the pending frontend transaction and increments a frontend epoch modulo its width. A response is accepted only if valid pending state and response identity, epoch, and expected address all match. Unmatched responses are drained without state change. Redirect wins over response in the same cycle. Epoch wrap safety requires no request from an old epoch to remain externally outstanding across a full wrap; Gate 5.1 uses one outstanding request and a width selected/tested against the environment's response lifetime.

Redirect flushes any future Fetch Buffer and younger FTQ entries. It does not clear learned predictor state; predictor speculative history/RAS repair is a later contract. Reset may clear predictor implementation state according to its own reset policy. `target_pc` must be 2-byte aligned when RVC is enabled and 4-byte aligned in Gate 5.1's non-RVC mode. Misaligned targets become an instruction-address exception rather than silent masking.

Gate 5.1 consumes only reset and existing branch redirects plus a reserved architectural redirect input. Predictor and ICache are not dependencies. Exception target generation remains outside Frontend unless a concrete CSR contract is supplied.
