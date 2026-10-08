# Source Scope Audit

## Baseline

| Field | Value |
| --- | --- |
| Branch | `gate3.8-rtl-verification` |
| HEAD | `2c6c5067274d9c310c637d20752bc271590bcded` |
| Index | empty |
| Protected file | `src/boom_all.cpp` |
| Protected SHA256 | `d6f885632ddd445729adda8148ea256e67683ccc8e7f2b10c9951e915d92c76c` |

`SRC_BOOM_ALL_HASH_UNCHANGED=true`

## Tracked Product Source Diff Against Baseline

| File | Category | Scope |
| --- | --- | --- |
| `include/boom_state.hpp` | LQ state | adds `outstanding_load_count`, constructor initializes to 0 |
| `src/lsu.cpp` | LSU/load issue ownership | per-LQ active txid ownership, bounded txid allocation, one external issue per cycle |
| `src/completion.cpp` | response ownership | scans active LQ owners for out-of-order response matching |
| `src/branch.cpp` | branch/flush LSU cleanup | clears killed per-entry load response ownership |
| `src/reset.cpp` | reset | clears `outstanding_load_count` |
| `src/commit.cpp` | completion/reclaim | store txid allocation integration |
| `src/synth_module_tops.cpp` | generated top seed alignment | L3 seed-state alignment |
| `src/boom_core_merged.cpp` | generated merged source | generated source changed with modular source |
| `src/boom_all.cpp` | protected generated aggregate | dirty before this triage; protected hash unchanged |

## Explicit No-Diff Areas

No tracked current-vs-baseline diff exists in:

| Area | Files Checked | Result |
| --- | --- | --- |
| frontend | `src/frontend.cpp`, `src/fetch_buffer.cpp`, `include/fetch_buffer.hpp` | unchanged |
| fetch packet | `src/fetch_packet.cpp`, `include/fetch_packet.hpp` | unchanged |
| RVC decode/decompress | `src/rvc.cpp`, `include/rvc.hpp`, `src/decode.cpp` | unchanged |
| predecode/predictor/FTQ frontend path | `src/predecode.cpp`, `src/predictor.cpp`, related headers | unchanged |
| RVC test and runner | `tb/differential/rvc_fetch_tests.cpp`, `scripts/gate5_2/run_r2_native_tests.sh` | unchanged |

`L3_SCOPE_VIOLATION=false`

## Reset And Bounds Audit

`src/reset.cpp` keeps independent bounds:

| Queue | Bound |
| --- | --- |
| LQ cleanup | `LQ_DEPTH` |
| SQ cleanup | `SQ_DEPTH` |

The L3 addition clears `state.lsu.outstanding_load_count = 0` on reset. `src/lsu.cpp` also clears the count and pending ownership summary in `clear_lsu_queues()`.

## No-Memory Activity Invariant

`rvc_fetch_tests` compiles only frontend/fetch/RVC/decode/branch/divider sources. It does not compile `src/lsu.cpp` or `src/completion.cpp`; the clean baseline and current runs are identical. Therefore the observed RVC failure is not caused by L3 load-issue or response matching logic in this focused binary.

`NO_MEMORY_ACTIVITY_L3_BEHAVIOR_EQUIVALENT_TO_L2=true`
