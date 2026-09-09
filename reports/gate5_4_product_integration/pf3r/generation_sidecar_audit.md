# PF3R ROB Generation Sidecar Audit

## Verdict

No missing generation restoration or practical stale/reused ROB-slot defect was
found in the canonical synthesis path.

## Data Flow

- `frontend.cpp` stamps the FTQ allocation `{idx, generation, lane, offset}` on
  every admitted packet lane, and the Fetch Buffer preserves those fields.
- `decode.cpp`, `rename.cpp`, `issue.cpp`, and `execute.cpp` preserve the live
  `MicroOp`; branch completion therefore retains its generation without a ROB
  reconstruction.
- `rob.cpp` captures `uop.ftq_generation` in
  `rob.ftq_generations[rob.tail]` before clearing only the synthesis copy stored
  inside the ROB entry.
- `completion.cpp` restores the generation for load responses only after the
  transaction, ROB validity, allocation ID, and LDQ-owner checks pass.
- `commit.cpp` restores from `rob.ftq_generations[rob.head]` for exception
  ownership and uses that same current-head sidecar entry for normal retirement.
- Branch redirect uses the generation carried by the validated live completion
  `MicroOp`; the FTQ subsequently validates both index and generation.

## Reuse And Reset Safety

The sidecar is overwritten on every successful ROB allocation. Stale payload may
remain after reset, branch squash, or global cleanup, but all reads are guarded by
current ROB validity/ownership, or occur at the valid current head before it is
invalidated. Logical cycle ordering commits before frontend FTQ processing and
performs ROB allocation afterward, so same-cycle retirement and reuse cannot
replace the generation before it is consumed.

The formal alias bound remains 32-bit generation and allocation-ID rollover.
This is the frozen PF3 contract and not changed by PF3R.

## Residual Verification Gap

Native PF3 tests exercise the non-synthesis `MicroOp` field, and focused PF3 RTL
tests exercise the FTQ wrapper rather than the complete ROB/completion/commit
sidecar path. Full-core generated RTL scenarios remain required before complete
PF3 acceptance.
