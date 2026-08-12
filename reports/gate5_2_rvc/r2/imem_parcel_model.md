# IMEM Parcel Model

## Canonical Contract

- `ImemResponse::instruction` is 32 bits and contains exactly four valid instruction bytes. There is no byte-valid mask or 64-bit fetch payload.
- Requests and accepted response addresses identify a 4-byte-aligned word. R2 aligns the request address with `pc & ~3`; response address must exactly echo that value.
- Acceptance retains Gate 5.1's exact `{fetch_id, epoch, address}` match. Unmatched responses are drained without changing word or carry state.
- Memory order is little-endian: response `[15:0]` is the lower-address parcel and `[31:16]` the higher-address parcel.
- A cross-word 32-bit instruction is `{next_response[15:0], saved_previous_response[31:16]}`.

## Source Evidence

- Interface fields: `include/boom_types.hpp:223-238`.
- Exact response matching: `src/frontend.cpp`.
- RTL model word selection: `rtl_tb/axis_imem_model.sv:40-61`.
- Hex loader low-word/high-word and little-endian expansion: `tb/differential/hls_prefix_trace_tb.cpp:50-77`.
- Full generated AXIS ports round the 225-bit semantic response to 256 transport bits; instruction remains bits `[159:128]`.

`ICACHE_FETCH_BYTES=8` is nominal configuration only and does not describe the implemented response payload. R2 therefore does not assume multiple 32-bit instructions per response and does not create a Fetch Buffer.

Final R2 evidence does not change this model: generated focused RTL passes 58/58 and mixed full-core generated RTL passes 10/10 using four-byte aligned words containing native 16-bit/32-bit byte streams, including a 32-bit instruction split across words and a redirect target at address 2 modulo 4.

## AXIS Cause Packing Audit

The full-core generated port places `exc_cause` in transport bits `[255:192]`, while the existing `axis_imem_model.sv` concatenation places a zero cause in semantic bits `[224:161]`. Zero-cause legacy tests mask this pre-existing discrepancy. R2 fault-capable RTL stimulus must use the generated transport layout.
