# IMEM Packet Capacity

## Exact Interface

- `ImemRequest::address` is a 64-bit byte address. Request identity is 32-bit `fetch_id` plus 32-bit `epoch`; `kill` is one bit (`include/boom_types.hpp:223-228`).
- `ImemResponse::instruction` is the canonical data payload and is exactly 32 bits. The response also echoes address/identity and carries exception metadata (`include/boom_types.hpp:231-238`).
- The canonical streams use these structs directly (`include/boom_interfaces.hpp:26-31`). `ICACHE_FETCH_BYTES=8` is a future configuration constant, not the implemented IMEM response width.

HLS transport width must not be confused with data payload width. The semantic request struct is 129 bits including `kill`; focused generated RTL exposes request `[128:0]`. The semantic response struct is 225 bits including address, IDs, exception, and cause; focused RTL exposes response `[224:0]`. Full-core synthesis removes constant-false request `kill`, so the generated DUT request port is 128 bits (`boom_core_top.v:74` in the accepted full-core RTL). The existing full-core harness/model interconnect declares 192 request bits and zero-extends that 128-bit DUT port; only `[127:0]` is observed (`tb/differential/gate5_2_r2_rtl_harness.sv:11-15,28,34-39`, `rtl_tb/axis_imem_model.sv:3-10`). The response interconnect is padded to 256 bits. In that bus the instruction data remains only `[159:128]`, exactly 32 bits. `axis_imem_model.sv` drives the same 32-bit `instruction_at()` result plus metadata and padding (`rtl_tb/axis_imem_model.sv:40-61,143-169`). Zero exception cause used by that model masks a known placement difference versus the generated full-core cause field; it does not change packet capacity, but a future nonzero fetch-fault RTL test must reconcile the packing.

```text
IMEM_REQUEST_ADDRESS_BITS=64
IMEM_RESPONSE_BITS=32
IMEM_RESPONSE_BYTES=4
PARCEL_BITS=16
PARCELS_PER_RESPONSE=2
REQUEST_ALIGNMENT_BYTES=4
```

Requests use `pc & ~3` or, for carry completion, `(halfword_pc + 2) & ~3` (`src/frontend.cpp:240-254`). The echoed response address is therefore the four-byte-aligned containing-word base, not necessarily the architectural instruction PC. Acceptance requires exact `{fetch_id, epoch, address}` equality (`src/frontend.cpp:116-131`).

## Ordering And Selection

The models are little-endian: bits `[15:0]` are bytes at base+0/base+1 and bits `[31:16]` are bytes at base+2/base+3. The binary full-core model builds each word with byte `b` shifted by `8*b` (`tb/differential/gate5_2_r2_full_core_rvc.cpp:35-51`). Frontend selects `[15:0]` when `PC[1]=0` and `[31:16]` when `PC[1]=1` (`src/frontend.cpp:182-187`). PC bit zero is illegal; PC bit one is a valid parcel selector.

`axis_imem_model.sv` indexes 32-bit words by `address >> 2`, maps lower/upper words from a 64-bit image in increasing address order, and echoes request address, ID, and epoch (`rtl_tb/axis_imem_model.sv:40-61,156-170`). C++ full-core models use the same aligned word and identity semantics (`tb/differential/gate5_2_r2_full_core_rvc.cpp:87-107`). Latency differs by model but payload capacity does not.

One stream response is destructively read once, including stale responses (`src/frontend.cpp:116-132`). A matching word can be retained locally through `response_received/resp_instruction`; this lets lower and upper compressed parcels publish on separate calls without rereading the stream (`include/boom_state.hpp:18-27`, `src/frontend.cpp:182-213`). There is no arbitrary response replay queue. Cross-word completion currently consumes response B, discards its unused upper parcel, and may re-request the same aligned word externally.

## Complete Instructions Per Response

| Case | Maximum complete instructions attributable to one response | Reason |
|---|---:|---|
| All RVC, aligned at lower parcel | 2 | C+C fits in two parcels. |
| All 32-bit, aligned at lower parcel | 1 | One instruction consumes both parcels. |
| C+C | 2 | Both parcels independently complete. |
| C+32 | 1 | C completes; upper parcel only starts the 32-bit instruction. |
| 32+C | 1 in the first word | Aligned 32-bit consumes both parcels; C begins in the next word. |
| Existing cross-word carry + response B | Up to 2 mixed | Lower parcel completes the carried 32-bit instruction; upper parcel can additionally be C. |
| Start at `PC[1]=1`, upper C | 1 | Lower parcel is before the target and ignored. |
| Start at `PC[1]=1`, upper starts 32-bit | 0 initially | First word only creates carry; a later response completes it. |
| Redirect/reset coincident with response | 0 | Response is drained stale and retained state is killed. |

```text
MAX_COMPLETE_INSTRUCTIONS_PER_RESPONSE_ALL_RVC=2
MAX_COMPLETE_INSTRUCTIONS_PER_RESPONSE_ALL_32=1
MAX_COMPLETE_INSTRUCTIONS_PER_RESPONSE_MIXED=2
```

The mixed maximum of two is specifically carry completion plus an upper compressed instruction. A response-level access fault contributes one terminal fault entry, not parcel data.

Accepted measurements agree: all-C delivered 256 instructions from 128 responses, all-32 delivered 128/128, alternating C/32 delivered 192/192 under the current scalar parser, and upper-start all-32 delivered 128/256 (`reports/gate5_2_rvc/r2/throughput_analysis.md:5-18`).

The B1 four-lane enqueue API therefore exceeds the maximum producer information available from one current IMEM response. It is a storage-side capability, not evidence for a four-wide Frontend producer.
