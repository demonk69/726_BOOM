# Remaining Gate Blockers

## Current Status Taxonomy

| Status Class | Result | Notes |
| --- | --- | --- |
| `ADAPTER_ONLY_PASS` | PASS | PPA4 adapter scoreboard passed. |
| `PARTIAL_CORE_PASS` | NOT_USED | Compact route is not adapter-only; generated core netlist is present. |
| `FULL_CORE_FUNCTIONAL_PASS` | PARTIAL | B3I full-core programs pass, but required multi-outstanding/out-of-order workload is not complete. |
| `FULL_CORE_PHYSICAL_PASS` | PARTIAL | PPA4 compact system routes and meets `6.450 ns`; board constraints pending. |
| `BOARD_BITSTREAM_PASS` | NOT_RUN | Blocked by missing board definition/XDC and DRC critical warnings. |

## Blocking Items

| ID | Blocker | Status | Required Next Action |
| --- | --- | --- | --- |
| PPA5-B1 | Single end-to-end workload covering multiple independent outstanding loads | OPEN | Build or reuse a legal program-level harness that measures actual outstanding load depth without internal state seeding. |
| PPA5-B2 | Out-of-order external response workload | OPEN | Extend memory model to support multiple pending responses and intentional reordering while preserving TXID. |
| PPA5-B3 | Store-to-load forwarding in program-level workload | OPEN | Add executable program and architectural/memory checks; current PPA5 B3I run does not cover it. |
| PPA5-B4 | Final memory image comparison | OPEN | Add final memory signature extraction and expected-state comparison. |
| PPA5-B5 | Board XDC | OPEN | Obtain board model, revision, pinout, bank voltages, clock/reset source, I/O standards. |
| PPA5-B6 | Bitstream signoff | OPEN | Rerun DRC and `write_bitstream` only after real board XDC exists. |
| PPA5-B7 | Reproducible committed PPA4 evidence | PARTIAL | PPA4 scripts/reports can be committed; generated RTL/DCP remain external and hash-recorded. |

## Final Fields

`L3_PPA4_EVIDENCE_COMMIT=42dca0373e132f9d87a7ea006893fbf0c6e30062`

`L3_PPA5_PPA4_IMPLEMENTATION_CLASS=FULL_CORE_WITH_COMPACT_ONCHIP_ADAPTER_PHYSICAL_EVIDENCE`

`L3_PPA5_FULL_CORE_PRESENT=PASS_NETLIST_PRESENT`

`L3_PPA5_PRODUCT_RTL_HASH_VERIFIED=PASS_FOR_R5_GENERATED_RTL_HASH`

`L3_PPA5_CRITICAL_LOGIC_PRESERVED=PASS_NETLIST_HIERARCHY_PRESENT_FUNCTIONAL_MULTI_OUTSTANDING_PROGRAM_PENDING`

`L3_PPA5_FULL_CORE_ELABORATION=PASS_PPA4_COMPACT_AND_B3I_RTL_XSIM_BUILD`

`L3_PPA5_FULL_CORE_WORKLOAD=PARTIAL_PASS_B3I_PROGRAMS_MULTI_OUTSTANDING_PROGRAM_PENDING`

`L3_PPA5_FULL_CORE_LOAD_STORE=PARTIAL_PASS_SINGLE_LOAD_STORE_PROGRAM_PLUS_R4_PROTOCOL_STRESS`

`L3_PPA5_FULL_CORE_RVC=PASS`

`L3_PPA5_FULL_CORE_COMMIT_TRACE=PASS_FOR_B3I_PROGRAMS`

`L3_PPA5_MAX_OBSERVED_OUTSTANDING=4_R4_STRESS_NOT_PPA5_PROGRAM_WORKLOAD`

`L3_PPA5_BOARD_XDC_STATUS=PENDING_BOARD_DEFINITION`

`L3_PPA5_NSTD_1_STATUS=CRITICAL_WARNING_UNRESOLVED`

`L3_PPA5_UCIO_1_STATUS=CRITICAL_WARNING_UNRESOLVED`

`L3_PPA5_FULL_CORE_LUT=48458_CORE_49452_TOTAL_COMPACT_SYSTEM`

`L3_PPA5_FULL_CORE_FF=29567_CORE_31832_TOTAL_COMPACT_SYSTEM`

`L3_PPA5_FULL_CORE_BRAM=28_TOTAL_BRAM_TILES_COMPACT_SYSTEM`

`L3_PPA5_FULL_CORE_DSP=16_CORE_16_TOTAL_COMPACT_SYSTEM`

`L3_PPA5_FULL_CORE_ROUTE=PASS_FOR_PPA4_COMPACT_SYSTEM_NOT_BOARD_SIGNOFF`

`L3_PPA5_FULL_CORE_WNS=0.434_COMPACT_SYSTEM_ONLY`

`L3_PPA5_FULL_CORE_TNS=0.000_COMPACT_SYSTEM_ONLY`

`L3_PPA5_BITSTREAM_STATUS=NOT_RUN_BOARD_XDC_PENDING`

`PPA_GATE_STATUS=UNRESOLVED`

`G6_0_L3_MULTI_OUTSTANDING_LOADS_VERIFIED=false`
