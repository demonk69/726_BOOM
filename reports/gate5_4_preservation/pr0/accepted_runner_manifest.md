# Accepted Gate 5.3 Random Runner Manifest

## Provenance

- Commit: `a48e527f78e42945969e945c63f501641eae179c`.
- Runner: `scripts/gate5_3/run_b3i_random.sh`.
- Runner SHA-256: `0d4800b85d1023d04611c835dccc96f8015b6f830aa62acabfdf8bce80f4981c`.
- Test: `tb/differential/fetch_packet_2lane_random_tests.cpp`.
- Test SHA-256: `0702fc56894b7be439270147bb00edc202de93d9ab05c5bd01348ed76e555464`.
- Accepted evidence: `reports/gate5_3_fetch_buffer/b3i/logs/random.log`.
- Accepted/reproduced log SHA-256: `5736b09d9eb241c580d2a60bcb4d68a7c7d2099ba877ca8a2ce6c702a4664920`.

## Compile And Run

```text
g++ -std=c++11 -O2 -Wall -Wextra -Werror \
  -Wno-error=misleading-indentation -Wno-error=unused-label \
  -Wno-unknown-pragmas -I$ROOT/include \
  $ROOT/tb/differential/fetch_packet_2lane_random_tests.cpp \
  $ROOT/src/frontend.cpp $ROOT/src/fetch_buffer.cpp \
  $ROOT/src/fetch_packet.cpp $ROOT/src/rvc.cpp \
  $ROOT/src/decode.cpp $ROOT/src/divider.cpp -o $BUILD/random
$BUILD/random
```

There are no command-line `-D` defines. Compile-time dimensions come from the accepted headers: packet width 2 and Fetch Buffer depth 8.

## Campaign And Oracle

- Seed list: integer seeds `0..255` inclusive.
- RNG initial state: `(0xd1b54a32d192ed03ULL ^ seed) | 1`.
- Cycle count: 4096 per seed; 1,048,576 total cycles.
- Input generation: deterministic memory-word patterns, delayed responses, stale ID/address/epoch responses, access faults, decode stalls, reset, architectural redirect, branch redirect, global flush, and ROB exception fence.
- Scoreboard: independent packet-aware `Oracle` with carry state, response-waiting state, pending packet mask/lanes, and FIFO of complete `FetchInstruction` entries.
- Required coverage: matched/stale responses, carry, zero/one/two-entry packets, stalls, resets, redirects, faults, illegal encodings, full buffer, atomic wait, produced/consumed/killed.
- Expected error counters: `drop=0`, `duplicate=0`, `packet_mask_error=0`, `partial_enqueue=0`, `bad_pc=0`, `order_error=0`, `stale_side_effect=0`, `atomicity_error=0`, `mirror_error=0`.

## Source SHA-256

| Path | SHA-256 |
|---|---|
| `src/frontend.cpp` | `a0640d73462ddbd8235db710f6f8cf435c6c8ef0b3d7b88dc09841298f95ff49` |
| `src/fetch_buffer.cpp` | `c4a2c2ea3692802ef3a69ab316f574cf2242c84752b4dcb92b5129a998f2b34d` |
| `src/fetch_packet.cpp` | `63bd6563e95c165ac7f2162bc7b788523c46cde485ca606e965fdd191716108f` |
| `src/rvc.cpp` | `987f69a8f6670e58deba1b0b2145ffb99ff79bd12b6f3b96a2362fa3e46a7c8e` |
| `src/decode.cpp` | `84498d9d38485d2b63562bffde1883ebac22ebcabc0526954b1159762d460dd4` |
| `src/divider.cpp` | `eb4514f11ab347649309142df54aae679dcdf15d7b7dd2b71583bec106c9991e` |
| `include/boom_state.hpp` | `66f066263a9fcdfdc76b320b7b8b2529ebe522d23a098e59479e525ca6e536dd` |
| `include/boom_config.hpp` | `8a76c0b4437fe7e3be597a8ad28d924bb79bd8e8306ee8ad1474ebf71a6cc682` |
