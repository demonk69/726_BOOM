# Gate 4.1 M1 Baseline Manifest

- Frozen commit: `c3da95934265428a36c1ffc6994d30329a3ee252`
- Branch: `gate3.8-rtl-verification`
- Remote parity at freeze: local HEAD equals `origin/gate3.8-rtl-verification`.
- Accepted predecessor: Gate 4.0 W4, `W4_MULTI_WRITEBACK_VERIFIED=true`.
- Canonical implementation: modular `include/` and `src/` files plus generated `src/boom_core_merged.cpp`.
- Explicit exclusion: `src/boom_all.cpp` is legacy, dirty, and is not read, generated, compiled, synthesized, hashed, or accepted by Gate 4.1.

## Pre-existing Dirty Scope

`git status --porcelain` contained 370 entries before M1. They are preserved and excluded:

- tracked: `reports/equivalence/provisional_gate3/hls_csim_trace.log`, `src/boom_all.cpp`, `vitis_hls.log`;
- work products: `build/`, `reports/gate3_4/hls_prefix_trace_tb`, `xvlog.log`, `xvlog.pb`;
- prior generated evidence not staged at W3/W4: `reports/gate4_0/w3/focused_rtl_work/`, `reports/gate4_0/w3/full_core_rtl/`, and related hash files;
- HLS/XSim intermediates under `reports/gate4_0/w4/full_core_rtl/`, excluding the already committed selected Verilog;
- timestamped `*.backup.log` files under Gate 3.8, Gate 4.0 W2, and Gate 4.0 W4.

Gate 4.1 outputs use `reports/gate4_1/`; temporary binaries and HLS projects use `build/gate4_1/`. No pre-existing dirty path is modified or treated as evidence.

## Frozen Architecture

- `DISPATCH_WIDTH=1`, `COMMIT_WIDTH=1`.
- Fixed lanes: MEM 0, INT 1, FP-reserved 2.
- W4 completion/writeback topology: three retained completion sources, three wakeups/bypasses, and two integer PRF writes.
- `CORE_CYCLE` is unpipelined.
- Existing 8-bit uop and FU fields already contain all RV64M identifiers and `FU_MUL`/`FU_DIV`.
