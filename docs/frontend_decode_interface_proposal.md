# Frontend to Decode Interface Proposal

Design-only draft; canonical types are unchanged in F0.

```cpp
struct FetchInstruction {
    bool valid;
    uint64_t pc;
    uint32_t instruction;
    bool is_rvc;
    bool exception;
    uint32_t exception_cause;
    uint32_t fetch_id;
};

template <unsigned MaxFetchWidth>
struct FetchPacket {
    bool valid;
    uint64_t base_pc;
    FetchInstruction slots[MaxFetchWidth];
    uint32_t valid_mask;
    uint32_t fetch_id;
};
```

Gate 5.1 uses `FetchInstruction` through a one-entry ready/valid holding stage and an adapter to the existing lane-0 decode input. Decode width and backend remain one. A later Fetch Buffer stores `FetchInstruction`, dequeues one item/cycle, and therefore does not require dispatch widening.

RVC detection, halfword assembly, and decompression belong in Frontend before enqueue/decode. Decode must always receive canonical 32-bit instructions while retaining original PC and `is_rvc`. Packet width should be a compile-time template parameter only when Gate 5.3 introduces packet lanes; depths remain separate architectural constants.

Prediction metadata should be owned by FTQ and referenced by a narrow `ftq_idx` in `MicroOp`. Existing `MicroOp.branch.ftq_idx`, `pc_lob`, branch mask/tag, and queue allocation ID are sufficient anchors (`include/boom_types.hpp:41-86`), but `ftq_idx` must be verified as at least four bits for 16 entries. Do not copy 120 predictor bits into every uop. Carry prediction taken/CFI classification only if execution needs it; retain bulk metadata in FTQ.

Width/PPA controls: adding epoch/fetch ID to every backend uop is not recommended. Consume identity at Fetch Buffer/decode boundary and retain only FTQ index plus architecturally required exception/RVC fields. Any widening of `MicroOp` multiplies across ROB 32, IQs, execute/completion registers, bypasses, and muxes and requires isolated synthesis.
