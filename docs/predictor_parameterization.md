# Predictor Parameterization

## Foundation Parameters

| Parameter | P0 recommendation |
|---|---|
| Algorithm | static direct-target predecode plus 2-bit BIM |
| BIM entries | sweep 64, 128, 256, 512; no entry count frozen in P0 |
| Counter width | 2 bits, saturating |
| Initial logical state | invalid entries read as weak not-taken; per-entry valid bitmap is reset, counter payload is not |
| Index source | exact selected conditional CFI PC, low aligned/index bits after dropping bit 0 |
| Lookup ports | one selected CFI read per cycle |
| Update ports | one commit-qualified write per cycle; same-index lookup receives forwarded updated value |
| Lookup latency | fixed one cycle |
| Pending requests | one |
| Generation | 32-bit Frontend epoch reuse plus pending token |
| Storage | AUTO initially; compare inferred registers/LUTRAM/BRAM in standalone P3 |

Raw counter capacity is 128, 256, 512, and 1024 bits for 64, 128, 256, and 512 entries. Index widths are 6, 7, 8, and 9 bits. Physical cost includes decode/mux, update bypass, initialization validity, pending response state, and ports; raw bits are not a PPA prediction.

On first commit to an invalid BIM entry, update from logical weak-not-taken, write the resulting counter, and set that entry valid. No background initialization is required.

No BTB/RAS/GHR parameter is part of foundation. Deferred BTB estimates in the P0 static model are planning bounds only. No directive, counter-payload full reset, complete partition, `DATAFLOW`, false dependency, or core-cycle pipeline is authorized.

P1 freezes no predictor-table parameter. Its predecode interface has fixed two-/four-byte instruction-length metadata and a four-value CFI enum (`NONE`, conditional, JAL, JALR). The optional packet helper is fixed at the accepted two-lane Fetch Packet width for interface proof only. No HLS directive was added.

## P2 Frozen Standalone Configuration

| Parameter | P2 selection |
|---|---|
| Algorithm | static direct-target predecode plus 2-bit BIM |
| BIM entries | 256 |
| Index width/formula | 8; `(pc >> 1) & 255` |
| Counter initialization | invalid reads as `01`, weak not-taken |
| Update collision | `UPDATE_FORWARD_NEW_VALUE` |
| Update qualification | committed conditional, active generation, matching PC-derived metadata index |
| Logical latency | request call N, response call N+1 |
| Pending requests | one; held response; no same-call turnover |
| Storage | LUTRAM |
| Reset | `LAZY_VALID_INIT`; separate valid bitmap, no counter payload reset |

The actual 256-entry storage comparison is AUTO 658 LUT/449 FF/1 BRAM/4.109 ns, LUTRAM 684 LUT/465 FF/0 BRAM/2.989 ns, and BRAM 652 LUT/449 FF/1 BRAM/4.109 ns. All report best-case HLS top latency 3, minimum II 4, and `PipelineType=no`. `BIND_STORAGE` selects storage only and is not a scheduling directive; `S0_NO_NEW_SCHEDULING_DIRECTIVE=true`.

The reset transaction contains a 256-iteration HLS loop, although logical software reset is one predictor step. Neither the logical N+1 response nor one reset step claims one physical clock. Measurements are standalone HLS estimates, not full-core PPA.
