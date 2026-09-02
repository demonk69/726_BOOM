# Frontend CFI Predecode Requirements

## Decision

`PREDECODE_REQUIRED=true`

The predictor cannot make a useful conditional-direction decision until it knows which packet instruction is the CFI and its exact PC. IMEM issue has only an aligned request address; RVC boundaries, a possible carry PC, CFI type, and static target are not known. Full information first exists after `build_fetch_packet` has completed/decompressed emitted instructions (`src/fetch_packet.cpp:73-135`). Performing CFI work before that point would duplicate the accepted carry and RVC parser.

Gate 5.3 already provides, for every emitted slot:

| Property | Available after packet construction | Evidence |
|---|---|---|
| Full canonical 32-bit instruction | Yes | normal/cross-word assembly at `fetch_packet.cpp:86-101,104-117`; RVC expansion at `31-45` |
| `is_rvc` | Yes | `fetch_packet.cpp:31-45` |
| exact instruction PC | Yes | `FetchInstruction.pc`; carry starts at `carry_pc` |
| length | Yes | `is_rvc ? 2 : 4` |
| opcode | Yes | canonical instruction bits; current Decode uses bits 6:0 at `decode.cpp:129-159` |
| branch/JAL/JALR type | Derivable, not stored | current Decode proves classifications at `decode.cpp:142-159` |
| B/J immediate | Derivable, not stored | current Decode extracts both immediates |
| compressed CFI class | Derivable from canonical expansion plus `is_rvc` | C.J/C.BEQZ/C.BNEZ/C.JR/C.JALR expansions at `rvc.cpp:157-170,190-201` |

Cross-word lane 0 is complete before predecode. Optional lane 1 is independently complete. A carry-only parser result has no packet and must not request prediction or allocate FTQ state.

## Minimal Logical Output

These are interface requirements, not fields added by P0:

```text
predecode_valid
instruction_pc[63:0]
instruction_length[2:0]     // 2 or 4 bytes
is_cfi
cfi_type                    // NONE, CONDITIONAL, JAL, JALR, RETURN
is_conditional
is_jal
is_jalr
is_call
is_return
static_target_valid
static_target[63:0]
```

`is_call` recognizes link writes by JAL/JALR with `rd=x1` or `x5`. `RETURN` is the selected subtype for JALR with `rd=x0`, `rs1=x1` or `x5`, and immediate zero; `is_jalr` and `is_return` are both true. C.JALR expands to `rd=x1` and is a call, while C.JR using `x1/x5` can be a return. The minimum response does not need raw opcode/immediate fields because predecode consumes them locally.

Faulting/illegal slots set `predecode_valid=false`. A packet with no valid CFI, including a fault-only packet, bypasses the predictor with no latency: it retains the parser mask/next PC and proceeds to atomic enqueue when ready. Predecode must preserve current instruction/fault delivery and must not become architectural Decode.
