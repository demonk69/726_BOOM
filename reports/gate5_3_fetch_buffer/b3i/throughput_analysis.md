# Gate 5.3 B3I Native Packet Utilization

This audit calls the canonical `boom::frontend_module` and then the canonical `boom::decode_module` once per counted native call. IMEM responses are returned on the native call after their request. A native call is a C++ state transition, not an HLS transaction clock or physical `ap_clk`; this report makes no RTL throughput claim.

The B2 comparison is the accepted one-token scalar-producer schedule under the same zero-downstream-stall assumption. B3I still has one-wide Decode, so two packet lanes improve response unpacking and buffer admission rather than backend width. Therefore B3I claims no end-to-end native-call speedup; startup effects make the measured rate ratio slightly below one in these finite campaigns.

| Scenario | Instructions | RVC | Native calls | Masks 00/01/11 | Packet slot util. | Fetch byte util. | B2 calls | B3I inst/call |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| all-C | 512 | 512 | 515 | 0/0/256 | 1.000 | 0.977 | 514 | 0.994 |
| all-32 | 256 | 0 | 259 | 0/256/0 | 0.500 | 0.988 | 258 | 0.988 |
| alternating | 384 | 192 | 387 | 0/192/96 | 0.667 | 0.980 | 386 | 0.992 |
| 75-percent-RVC | 512 | 384 | 515 | 0/128/192 | 0.800 | 0.982 | 514 | 0.994 |
| cross-word-heavy | 256 | 0 | 260 | 1/256/0 | 0.500 | 0.985 | 258 | 0.985 |
| branch-heavy | 384 | 288 | 387 | 0/96/144 | 0.800 | 0.976 | 386 | 0.992 |

Packet masks and valid slots include only lanes whose PCs are within the intended program range. Trailing frontend prefetch remains included in request, response, and fetched-byte totals but cannot inflate packet production. `mask_00` records in-range carry-only responses, `mask_01` one complete instruction, and `mask_11` two complete instructions. No `mask_10` is legal. `useful_bytes` is the architectural stream size; fetched bytes include aligned and trailing native requests.
