# REF1 Transaction Boundary Contract

## Interfaces Are Not Pin-Compatible

The reference hierarchy reaches TileLink cache/uncached request/response channels in BoomTile and then the ChipTop/DigitalTop bus and AXI/DRAM simulation models. HLS RTL exposes separate decoupled IMEM request/response and DMEM request/response streams. There is no direct same-pin comparison.

## First-Stage Semantic Adapter

| Semantic operation | Reference observation in current standalone runner | HLS RTL observation | REF1 comparison |
|---|---|---|---|
| Fetch | Program image loaded at `0x80000000`; committed PCs/instructions are traced. No ICache TileLink request cycles are recorded. | IMEM stream driven with same word image; requests/responses recorded in execution trace. | instruction bytes and retirement semantics compared; fetch handshakes not compared |
| Load/store acceptance | The runner uses a simple AXI memory model. Current reference JSONL records retired tohost store only; general AXI request handshakes are not logged. | DMEM request trace records address, txid, load/store, commit, mask/data; responses recorded for loads. | no full request protocol equivalence claim; compare only observable retired memory effects for these programs |
| Transaction identity | Reference runner does not expose architectural ownership ID mapping in its trace. | HLS LSU txid/response ownership is logged. | raw txid bit patterns are not compared; owner map unavailable on reference side |
| Memory result | Reference `tohost` event detects retired store value; it does not dump a complete memory image. | HLS final memory report plus commit trace. | compare the common touched location `0x80000080`; report full-memory comparison unavailable |
| Fault/exception | Reference metadata reports trap_seen and commit exception; exception-cause field is nullable in this runner. | HLS commit exception/cause and `io_trap`. | compare observed exception boolean; cause only when both sources expose it |

## Initial State and Normalization

Both sides use the exact same five original 64-bit word images loaded at `0x80000000`, reset vector `0x10040`, tohost address `0x80000080`, and initial machine-mode execution intent. The REF1-only HLS testbench is derived from the prior stream adapter with its five PPA6 sentinel writes removed; it initializes all DMEM bytes to zero, matching the reference runner's sparse-memory default. The derivation and source/testbench hashes are in `ref1_hls_tb_manifest.json`. HLS uses a three-instruction reset adapter instead of the reference boot ROM. Raw traces preserve the reset/ROM commits. Normalized comparisons filter only commits with PC below the common program entry `0x80000000`; no commit inside the common program interval is skipped. The selected five programs issue no data loads and only their tohost store. The HLS final-state comparison is therefore scoped to tohost, not claimed as a complete address-space dump.

Cycle counts are reported side by side but are not an equality criterion. Commit order and architectural payload are compared exactly.
