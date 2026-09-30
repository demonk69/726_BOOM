# Gate 6.0 L2R BRAM Mapping Audit

## Scope

This audit first compares the pre-repair L2 full-core synthesis results for `LQ_DEPTH` / `SQ_DEPTH` configurations 4/4, 8/8, and 16/16. That historical 24-BRAM measurement is the required object-level diagnostic baseline for `L2R_PARAMETER_16_16_BRAM_REPAIR`; only the later Repair Result table is accepted as final PPA, and no parameter exception is granted.

Evidence:

- `/home/lab_726/opencode_tmp/g6_l2/full_core/4_4/hls_project/solution/syn/report/boom_core_top_csynth.rpt`
- `/home/lab_726/opencode_tmp/g6_l2/full_core/8_8/hls_project/solution/syn/report/boom_core_top_csynth.rpt`
- `/home/lab_726/opencode_tmp/g6_l2/full_core/16_16/hls_project/solution/syn/report/boom_core_top_csynth.rpt`
- `/home/lab_726/opencode_tmp/g6_l2/full_core/16_16/hls_project/solution/.autopilot/db/design.bindinfo.xml`
- `reports/gate6_0_full_lsu/l2/l2r_bram_object_matrix.csv`

## Configuration Summary

| Configuration | LUT | FF | BRAM18K | DSP | Estimated period |
|---|---:|---:|---:|---:|---:|
| 4/4 | 208928 | 47341 | 16 | 3 | 6.071 ns |
| 8/8 | 207214 | 46677 | 16 | 3 | 6.071 ns |
| 16/16 | 208057 | 46492 | 24 | 3 | 6.071 ns |

The 16/16 configuration adds exactly eight BRAM18Ks relative to both smaller configurations. No timing, DSP, or broad resource discontinuity accompanies the increase.

## Exact Delta

Only two generated memories account for the increase:

| Generated object | Logical field | 8/8 mapping | 16/16 mapping | BRAM delta |
|---|---|---|---|---:|
| `state_lsu_stq_address_U` | `StoreQueueEntry.address` | 8 x 64, auto 1R1W, 0 BRAM | 16 x 64, auto 1R1W, 4 BRAM | +4 |
| `state_lsu_stq_data_U` | `StoreQueueEntry.data` | 8 x 64, auto 1R1W, 0 BRAM | 16 x 64, auto 1R1W, 4 BRAM | +4 |

The 16/16 binding database identifies both objects as latency-one `ram_t2p` storage with `IMPL="auto"`. The generated memory table reports four BRAM18Ks per object. This audit uses the synthesis report totals as the resource authority and the binding database for storage type and implementation selection.

The address array is searchable forwarding metadata. The data array is forwarding payload. They are not LQ storage, queue identity, generation, validity, count, or control state. Both remain logically 16 entries wide at 16/16 and must retain all 64 bits.

## Unchanged BRAM Baseline

The following objects account for the 16 BRAM18Ks present in every measured configuration:

| Object or object group | BRAM18K |
|---|---:|
| `state_int_rf_bank0_U` | 2 |
| `state_int_rf_bank1_U` | 2 |
| `state_rename_int_map_table_br_snapshots_U` | 1 |
| `state_rob_entries_uop_inst_U` | 1 |
| `state_rob_entries_uop_imm_packed_4_U` | 1 |
| `state_rob_entries_uop_debug_pc_U` | 2 |
| `state_rob_entries_uop_rename_U` | 2 |
| `state_rob_entries_memory_address_U` | 2 |
| `state_rob_entries_memory_data_U` | 2 |
| `state_rob_entries_memory_transaction_id_U` | 1 |
| Total | 16 |

No LQ object consumes BRAM in any measured configuration. All generated SQ control and identity arrays also remain distributed at 16/16.

## Port Pressure

The two failing objects are inferred as true dual-port storage because full-core state updates and forwarding inspection require independent accesses. The generated RTL/binding evidence describes a latency-one `ram_t2p` implementation, while the report groups the implementation under an auto 1R1W memory module. The problem is therefore not raw capacity: each logical memory contains only 1024 bits. It is the auto-selected primitive and port replication at the depth-16 threshold.

## Candidate Screening Order

1. `SOURCE_LAYOUT_SPLIT`: separate searchable SQ address metadata from SQ data payload and verify whether auto mapping changes. A split is acceptable only if all 16 entries and all 64 payload bits remain represented and forwarding ownership checks are unchanged.
2. `NARROW_PAYLOAD_STORAGE`: rejected as a direct repair because the architecture requires the full 64-bit store payload and byte-mask semantics. No field may be dropped or truncated.
3. Existing safe mapping idiom: the project already uses targeted `RAM_2P` LUTRAM storage for persistent arrays such as `state.rob.ftq_generations`; an SQ-specific use may be screened after layout evidence.
4. Targeted `bind_storage`: allowed only for the two isolated SQ arrays, with measured 4/4, 8/8, and 16/16 effects. Broad state binding is not acceptable.

## Constraints

- The true 16/16 depths must remain 16/16.
- Store data remains 64 bits and store addresses remain 64 bits.
- Youngest-older overlapping single-store full-cover forwarding remains enabled.
- Stale generation and ROB allocation ownership checks remain enabled.
- Search scope remains the older ROB interval; no outstanding-load or semantic reduction is allowed.
- No `DATAFLOW`, broad partition, broad unroll, dependence suppression, false path, or core-cycle pipeline directive may be introduced.

## Audit Verdict

`L2R_ROOT_CAUSE=DEPTH_16_AUTO_T2P_MAPPING_OF_TWO_16X64_SQ_ARRAYS`

`L2R_BASELINE_BRAM=16`

`L2R_16_16_BRAM=24`

`L2R_REPAIR_REQUIRED=8_BRAM18K`

## Repair Result

The accepted repair moves the two 64-bit SQ payload arrays out of the entry
aggregate and applies `ARRAY_PARTITION cyclic factor=2 dim=1` only when
`SQ_DEPTH >= 16`. At 16/16, each logical 16 x 64 array is represented as two
8 x 64 auto RAM banks. No depth, width, search interval, or ownership check is
reduced.

| Configuration | LUT | FF | BRAM18K | DSP | Estimated period |
|---|---:|---:|---:|---:|---:|
| 4/4 | 208959 | 47345 | 16 | 3 | 6.071 ns |
| 8/8 | 207247 | 46664 | 16 | 3 | 6.071 ns |
| 16/16 | 211984 | 49382 | 16 | 3 | 6.071 ns |

Authoritative reports are under
`/home/lab_726/opencode_tmp/g6_l2/l2r_bank2_full_core/`. The temporary runner
CSVs incorrectly retained the old `full_core` report path; the durable
`synthesis/parameter_csynth.csv` records the corrected paths.

`L2R_REPAIR_RESULT=PASS_16_BRAM18K`

The BRAM blocker is closed. Overall release remains subject to all functional
and preservation gates.
