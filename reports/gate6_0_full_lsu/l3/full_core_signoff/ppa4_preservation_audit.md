# PPA4 Preservation Audit

Scope: Gate 6.0 L3R5-PPA5 audit of the compact PPA4 package-level system integration evidence.

## PPA4 Evidence Status

| Item | Status | Evidence |
| --- | --- | --- |
| PPA3 timing evidence independently committed | PASS | `5a2ce0bc6b4f245747b9c0b4b13a1a2bfb6b5db6`, subject `Gate 6.0 L3R5: close 6p450 OOC timing analysis`. |
| PPA4 nine-report set present | PASS | `reports/gate6_0_full_lsu/l3/system_integration/`. |
| PPA4 raw tool artifacts present | PASS | `/home/lab_726/opencode_tmp/g6_l3r5_ppa4_*`. |
| Adapter scoreboard executed | PASS | `PPA4_SCOREBOARD,PASS,all_checks` in `simulate.log`. |
| Vivado top explicit | PASS | `boom_l3_ppa4_system_top`. |
| FPGA part explicit | PASS | `xczu7ev-ffvc1156-2-e`. |
| WNS/TNS sourced from routed DCP timing | PASS | `ppa4_timing_summary_from_dcp.rpt`, WNS `0.434 ns`, TNS `0.000 ns`. |
| Reproducible from repo alone | NOT_CLAIMED | Generated RTL and routed DCP are external artifacts under `/home/lab_726/opencode_tmp`. |

PPA4 evidence is valid compact system evidence, but it is not marked `REPRODUCIBLE_PASS` because generated RTL, DCPs, and Vivado reports are not all versioned in the repository.

## Artifact Manifest

| Artifact Class | Path | SHA-256 | Versioned |
| --- | --- | --- | --- |
| Product HLS input used by R5 preflight | `src/boom_core_merged.cpp` | `589f5b5c90bbbbaadc4e2d30c14205c5dc8e5c4c6a904e27de94a4255cd9a5f7` | yes |
| Product source/header working-tree aggregate | `/home/lab_726/opencode_tmp/g6_l3r5_ppa5_product_source_hashes_20261010.txt` | `a394e0a8586f8fca1787bd396acd730a84d0d2fc9d30a64c12cdf3520609cd0f` | external manifest |
| Product source/header HEAD tree aggregate | `/home/lab_726/opencode_tmp/g6_l3r5_ppa5_product_source_head_tree_20261010.txt` | `6104543372c6609ab9f76352037f5e1384de384e5ce11407b3fce869bf8f3602` | external manifest |
| R5 HLS Tcl runner | `scripts/gate6_0/l2_full_core_csynth.tcl` | `51071bbd8e083876454ada347786f856cf399e5d09d78d32b717cc130e4c9be0` | yes |
| R5 HLS directives | `directives/baseline_directives.tcl` | `4e37fa6f60d80b71056dd966f48c92ad03b10e122dd1fcb367def3c0d14ececd` | yes |
| Generated RTL aggregate | `/home/lab_726/opencode_tmp/g6_l3r5_ppa5_generated_rtl_hashes_20261010.txt` | `20ec3d0e65ef5c419fa6082d8a3d341580a2144b2c1553dd26e05139d2fa6e12` | no |
| PPA4 memory adapter RTL | `reports/gate6_0_full_lsu/l3/system_integration/rtl/boom_l3_ppa4_memory_system.v` | `e93bc70ecf0cc393fbe9be003ff3102acb93d5b04af2a1163a2e38eef0de037a` | PPA4 commit candidate |
| PPA4 system wrapper RTL | `reports/gate6_0_full_lsu/l3/system_integration/rtl/boom_l3_ppa4_system_top.v` | `75034bacd56202737e5868c35aaefcff699377e2d22d32019499381246dd0c21` | PPA4 commit candidate |
| PPA4 adapter testbench | `reports/gate6_0_full_lsu/l3/system_integration/tb/ppa4_memory_system_scoreboard_tb.sv` | `c368a78d8db543bdcaa4b89671cc42f89e755a769b00b00de06fea52f30484d9` | PPA4 commit candidate |
| PPA4 scoreboard runner | `reports/gate6_0_full_lsu/l3/system_integration/ppa4_adapter_scoreboard_sim.tcl` | `cc54d4957bb21c0e91f99a8d77a2ff3bd3e1fb75db56adc901065a0e1b95d96a` | PPA4 commit candidate |
| PPA4 synthesis runner | `reports/gate6_0_full_lsu/l3/system_integration/ppa4_system_synth.tcl` | `ec9984a1b342b16cc154dc4b13458149d38352973d7d3ac8c91496e1742a6dcd` | PPA4 commit candidate |
| PPA4 implementation runner | `reports/gate6_0_full_lsu/l3/system_integration/ppa4_system_impl.tcl` | `6a266785ecc3c6e2c986fa79bcbd1835a0bbcd5348a34853eb71cb053ace8b7b` | PPA4 commit candidate |
| Board XDC | none found | none | no |
| PPA4 routed DCP | `/home/lab_726/opencode_tmp/g6_l3r5_ppa4_system_impl_compact_20261010/ppa4_post_route.dcp` | `740693b8631ebd45faeb1eeae12a86fd974789a9247ce562ef3f8a62f9e5bdb3` | no |

## Source Control Caveats

| Path | Status | Reason |
| --- | --- | --- |
| `src/boom_all.cpp` | modified before this task | Not used by the R5 HLS Tcl; left untouched. |
| PPA4 wrapper/scripts/reports | untracked before optional PPA4 commit | Experimental evidence assets produced after PPA3. |
| Generated RTL directory | not versioned | Tool output under `/home/lab_726/opencode_tmp`; use aggregate hash above. |
| Routed/synth DCP and raw Vivado reports | not versioned | Large tool artifacts under `/home/lab_726/opencode_tmp`; hash recorded. |

## PPA4 Classification

`PPA4_IMPLEMENTATION_CLASS=FULL_CORE_WITH_COMPACT_ONCHIP_ADAPTER_PHYSICAL_EVIDENCE`

This classification means the routed netlist includes the generated `boom_core_top` plus the compact adapter. It does not mean the compact adapter provides a real executable program memory. The PPA4 adapter returns NOPs for IMEM, so PPA4 adapter scoreboard evidence cannot be used as full-core workload pass evidence.

## Final Fields

`L3_PPA4_EVIDENCE_COMMIT=42dca0373e132f9d87a7ea006893fbf0c6e30062`

`L3_PPA5_PPA4_IMPLEMENTATION_CLASS=FULL_CORE_WITH_COMPACT_ONCHIP_ADAPTER_PHYSICAL_EVIDENCE`

`L3_PPA5_PRODUCT_RTL_HASH_VERIFIED=PASS_FOR_R5_GENERATED_RTL_HASH`

`L3_PPA5_FULL_CORE_PRESENT=PASS_NETLIST_PRESENT`

`L3_PPA5_CRITICAL_LOGIC_PRESERVED=PASS_NETLIST_HIERARCHY_PRESENT_FUNCTIONAL_MULTI_OUTSTANDING_PROGRAM_PENDING`
