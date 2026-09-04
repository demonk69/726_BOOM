# Current Preservation Runner Manifest

## Identification

PF1 did not preserve a dedicated runner path or compile log for the failing invocation. The exact output marker uniquely identifies the selected test as `tb/differential/fetch_buffer_integration_random_tests.cpp`, originally introduced for Gate 5.3 B2. The nearest checked-in owner is `scripts/gate5_3/run_b2_native.sh`, but that historical script is not a valid B3I standalone random runner because its `FRONTEND` source list predates and omits `src/fetch_packet.cpp`. The current preservation invocation was therefore ad hoc/unversioned.

- Selected test SHA-256: `bef9c5f54bb38594e790fa6dfe04b0d70334bd63106a0dae8a203564872bbaf4`.
- Historical owner SHA-256: `b6add874ef0559157b070bc9a624027aba5a9f6ca6e5b24f73cb9defb097c1ba`.
- Reconstructed baseline-equivalent result SHA-256: `babd4f2be1f6ceadb46aabeb0c4ad5758e3bc6626039574943a068fdb380d21b`.
- Result: `drop=425388`, `ordering_error=174170`; duplicate, stale-side-effect, and post-flush errors zero.

## Reconstructed Compile And Run

```text
g++ -std=c++11 -O2 -Wall -Wextra -Werror \
  -Wno-error=misleading-indentation -Wno-error=unused-label \
  -Wno-unknown-pragmas -I$SOURCE/include \
  $TEST/tb/differential/fetch_buffer_integration_random_tests.cpp \
  $SOURCE/src/frontend.cpp $SOURCE/src/fetch_buffer.cpp \
  $SOURCE/src/fetch_packet.cpp $SOURCE/src/rvc.cpp \
  $SOURCE/src/decode.cpp $SOURCE/src/divider.cpp -o random
random
```

There are no command-line `-D` defines.

## Campaign And Scoreboard

- Seed list: integer seeds `0..255` inclusive.
- RNG initial state: `(0x9e3779b97f4a7c15ULL ^ seed) | 1`.
- Cycle count: 4096 per seed.
- Input generation: deterministic four-pattern memory, delayed/stale responses, faults, stalls, reset, debug redirect, branch redirect, and generic flush.
- Scoreboard: B2 scalar-producer queue. It observes only pre-step `producer_uop`/`producer_fetch_id` and predicts at most one enqueue per cycle.
- Expected counters encoded by this test: zero drop, duplicate, ordering, stale-side-effect, and post-flush-old-entry errors.

## Source Binding

The current committed baseline `490788d8...` has byte-identical relevant sources and tests to `a48e527...`. PF1 changes only relevant Frontend control ownership/state:

| Path | Baseline SHA-256 | PF1 SHA-256 |
|---|---|---|
| `src/frontend.cpp` | `a0640d73462ddbd8235db710f6f8cf435c6c8ef0b3d7b88dc09841298f95ff49` | `dc4a191c0929632436c48ea2179a5320877d8131ea6a221cd0d7deff0c8eccb6` |
| `include/boom_state.hpp` | `66f066263a9fcdfdc76b320b7b8b2529ebe522d23a098e59479e525ca6e536dd` | `29884300de11ea706894612d5c38b67f8548884a3c257852ba98276549f6020d` |
| `src/fetch_packet.cpp` | `63bd6563e95c165ac7f2162bc7b788523c46cde485ca606e965fdd191716108f` | same |
| `src/fetch_buffer.cpp` | `c4a2c2ea3692802ef3a69ab316f574cf2242c84752b4dcb92b5129a998f2b34d` | same |

The accepted packet-aware test passes unchanged against the PF1 hashes with all error counters zero.
