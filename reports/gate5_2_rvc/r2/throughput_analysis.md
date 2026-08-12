# Gate 5.2 RVC R2 Native Throughput Audit

The harness calls canonical `boom::frontend_module` followed by canonical `boom::decode_module`. Decode's one-entry output is consumed before every native call. Every aligned-word request is accepted immediately and its exact matching response is presented on the next call. Thus a cycle below is one native architectural call, not an `ap_clk`.

| Scenario | Architectural instructions | Cycles | Bytes fetched | Requests | Responses | Request interval mean/min/max | Instruction interval mean/min/max | Cycles/instruction |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| all-C | 256 | 257 | 516 | 129 | 128 | 2.000000/2/2 | 1.000000/1/1 | 1.003906 |
| all-32 | 128 | 129 | 516 | 129 | 128 | 1.000000/1/1 | 1.000000/1/1 | 1.007812 |
| alternating-C-32 | 192 | 241 | 772 | 193 | 192 | 1.250000/1/2 | 1.251309/1/2 | 1.255208 |
| cross-boundary-heavy | 128 | 257 | 1028 | 257 | 256 | 1.000000/1/1 | 2.000000/2/2 | 2.007812 |

`Bytes fetched` counts all accepted 4-byte requests, including a trailing sequential request if the canonical frontend emits one with the final publication. Intervals are differences between event call numbers; cycles/instruction includes the initial request call.

## Gates

- **Retained C parcels: PASS.** In `all-C`, every upper compressed parcel is published one call after its lower partner without a response on that second call; the response word is reused locally and there is no publication bubble.
- **Aligned all-32 contract: PASS.** Logical requests and publications each have mean/min/max interval 1/1/1 after startup. The implementation retains the current one-request-per-word cadence.
- **Cross-boundary distinction:** the cross-boundary-heavy stream starts every 32-bit instruction at PC modulo 4 equal to 2. Its carry-only calls are parcel-local assembly work, not extra IMEM response latency.

## Protocol Boundary

This native test measures C++ call-level state transitions. It does not instantiate an HLS `ap_ctrl_hs` wrapper and makes no physical-clock throughput claim. HLS call latency/II must be reported from synthesis or RTL separately; the one-call IMEM latency here is explicit in `cycle_trace.csv` as request call N and response call N+1. Parcel-local reuse is visible when publication occurs with `response_valid=0`, and carry assembly is visible in `carry_before/after`.
