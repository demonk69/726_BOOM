# M3C Source Scope

M3C is a verification and acceptance stage. No core structural change is planned. Canonical implementation is the modular `include/` and `src/` source merged by `scripts/generate_merged.sh` into `src/boom_core_merged.cpp`.

In scope:

- RV64M joint directed and random differential tests.
- Joint focused generated-RTL testbench and runner.
- Joint native, Vitis csim, and generated-RTL full-core program evidence.
- Existing regression runners and eight canonical synthesis tops.
- Reports, manifests, hashes, guardrail review, and status documentation.

Explicitly excluded:

- `src/boom_all.cpp`.
- Frontend, Full LSU, FPU, Cache, MMU, and TileLink implementation.
- New completion sources, PRF write ports, wakeup/bypass ports, or capacity changes.
- `CORE_CYCLE` pipeline, `DATAFLOW`, false dependence, and complete array partition.
- Raw `boom_core_step` as a product synthesis entry.
- Changes to frozen expected traces or earlier evidence.
