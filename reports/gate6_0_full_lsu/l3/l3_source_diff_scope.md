# L3 Source Diff Scope

Intentional product source changes:

| Path | Purpose |
| --- | --- |
| `include/boom_state.hpp` | Add derived `outstanding_load_count` to LSU state |
| `src/lsu.cpp` | Per-LQ outstanding ownership, bounded active txid allocation, one-load-per-cycle issue |
| `src/completion.cpp` | Out-of-order txid lookup through active LQ owners and full tuple validation |
| `src/branch.cpp` | Per-entry squash invalidation for killed outstanding loads |
| `src/reset.cpp` | Clear outstanding count during reset |
| `src/commit.cpp` | Store requests use bounded txid allocator to avoid active load txid collisions |
| `src/synth_module_tops.cpp` | Keep focused wrapper seed state aligned with LQ owner truth |
| `src/boom_core_merged.cpp` | Regenerated from split source via `scripts/generate_merged.sh` |

Intentional test/script/report additions:

| Path | Purpose |
| --- | --- |
| `tb/differential/g6_l3_multi_outstanding_tests.cpp` | Native/CSim L3 directed, bounded exhaustive, and random oracle tests |
| `scripts/gate6_0/run_l3_multi_outstanding_native.sh` | Five-config native runner |
| `scripts/gate6_0/l3_multi_outstanding_csim.tcl` | Vitis CSim project flow |
| `scripts/gate6_0/run_l3_multi_outstanding_csim.sh` | Five-config CSim runner |
| `reports/gate6_0_full_lsu/l3/*` | Durable L3 evidence reports |

Explicitly not modified:

| Path | Status |
| --- | --- |
| `src/boom_all.cpp` | SHA256 unchanged: `d6f885632ddd445729adda8148ea256e67683ccc8e7f2b10c9951e915d92c76c` |

Pre-existing dirty files outside the scope above were left untouched.
