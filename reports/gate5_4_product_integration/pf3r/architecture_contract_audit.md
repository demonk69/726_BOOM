# PF3R Architecture Contract Audit

PF3R preserves the accepted PF3 contract:

- FTQ entry payload remains 211 bits at depth 32 and maps to LUTRAM.
- Reset remains control-only; payload arrays are not reset-cleared.
- References remain 40 bits: 5-bit index, 32-bit generation, 1-bit lane, and
  2-bit halfword offset, with `is_rvc` separate.
- Fetch Buffer enqueue and FTQ allocation remain atomic for each nonempty final
  packet.
- The FTQ may reclaim at most one zero-live-lane head and allocate in the same
  step.
- Stale safety remains index plus 32-bit generation and lane ownership.
- Conditional prediction steering, predicted-vs-actual recovery, and Commit BIM
  training remain disabled.
- No `CORE_CYCLE` pipeline or prohibited scheduling directive was introduced.

Canonical `boom_core_step()` now directly selects the compile-time product
specialization. The runtime-selectable `frontend_module()` remains available to
focused preservation tests, but its mode condition is not synthesized into the
canonical product path.

`src/boom_all.cpp` remains excluded from the modular build and its SHA-256 is
`d6f885632ddd445729adda8148ea256e67683ccc8e7f2b10c9951e915d92c76c`.
