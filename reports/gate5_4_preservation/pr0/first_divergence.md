# First Divergence

```text
FIRST_FAIL_SEED=0
FIRST_FAIL_CYCLE=7
```

The diagnostic used an instrumented copy only under `/tmp/boom_hls/pr0/current/`; the repository test was not edited. It ran the unmodified failure conditions and stopped at their first true mismatch.

## Pre-Step State

- Reset/redirect: runtime reset false, architectural redirect false, branch redirect false, generic flush false.
- Decode ready: false.
- Response injection this cycle: no matching or stale response; `response_received=0`.
- Last response fields: instruction `65537` (`0x00010001`), fetch ID 1, no exception.
- Carry: invalid, PC 0, value 0.
- Pending packet: valid, mask 3.
- Packet lane 0: PC 65602 (`0x10042`), instruction 65683 (`0x10093`), fetch ID 1.
- Packet lane 1: PC 65606 (`0x10046`), instruction 19 (`0x13`), original parcel 1, fetch ID 1.
- Outstanding request: valid, pending fetch ID 2, pending epoch 0.
- Old B2 scoreboard decision: `expected_pop=0`, `expected_push=1`.

## Expected Queue

| Index | PC | Instruction | Original | Fetch ID | RVC |
|---:|---:|---:|---:|---:|---:|
| 0 | 65600 | 19 | 1 | 0 | 1 |
| 1 | 65602 | 65683 | 0 | 1 | 0 |

## Actual Queue

| Index | PC | Instruction | Original | Fetch ID | RVC |
|---:|---:|---:|---:|---:|---:|
| 0 | 65600 | 19 | 1 | 0 | 1 |
| 1 | 65602 | 65683 | 0 | 1 | 0 |
| 2 | 65606 | 19 | 1 | 1 | 1 |

## Explanation

The B2 scoreboard observes `producer_uop`, which mirrors only pending packet lane 0, and predicts one enqueue. The accepted B3I product atomically admits both valid lanes, correctly adding PC 65606 as the third queue entry. The first `drop` is therefore an oracle cardinality error, not a product drop. Subsequent expected/actual queue phase displacement creates the aggregate `ordering_error` count.

Raw diagnostic: `/tmp/boom_hls/pr0/logs/first_divergence.log`, SHA-256 `5dd678d3bd663c334078ef92557c0d67405721ad8583199ce0ddf2cbad4a67be`.
