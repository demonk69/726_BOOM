# RTL Provenance Audit

## Compared Evidence

| Item | PPA4 compact physical | PPA5 B3I RTL | PPA6 end-to-end RTL | L3R6 capacity RTL |
| --- | --- | --- | --- | --- |
| Product source | `src/boom_core_merged.cpp` | `src/boom_core_merged.cpp` | `src/boom_core_merged.cpp` | `src/boom_core_merged.cpp` |
| Source SHA-256 | `589f5b5c90bbbbaadc4e2d30c14205c5dc8e5c4c6a904e27de94a4255cd9a5f7` | `589f5b5c90bbbbaadc4e2d30c14205c5dc8e5c4c6a904e27de94a4255cd9a5f7` | `589f5b5c90bbbbaadc4e2d30c14205c5dc8e5c4c6a904e27de94a4255cd9a5f7` | `589f5b5c90bbbbaadc4e2d30c14205c5dc8e5c4c6a904e27de94a4255cd9a5f7` |
| Config | `LQ_DEPTH=4 SQ_DEPTH=4` | `LQ_DEPTH=4 SQ_DEPTH=4` | `LQ_DEPTH=4 SQ_DEPTH=4` | `LQ_DEPTH=4 SQ_DEPTH=4` |
| Generated RTL dir | `/home/lab_726/opencode_tmp/g6_l3r2_r5_preflight_4_4_20261009_155117/hls_project/solution/syn/verilog` | same | same | same |
| RTL top SHA-256 | `9adbf00aa02cc89def9f73d34f860e2865bef0c1771106f94bce97f6e5459659` | same | same | same |
| RTL aggregate SHA-256 | `7fb7061044620afddeda70abf0dae0c77bbd95583a982f42e2abe5c7fd251a44` | same | same | same |
| Tool | Vivado/Vitis 2021.2 | Vivado xsim 2021.2 | Vivado xsim 2021.2 | Vivado xsim 2021.2 |
| Top | `boom_l3_ppa4_system_top` | `boom_core_top` | `boom_core_top` | `boom_core_top` |
| Adapter/model | Compact on-chip adapter; IMEM returns NOP | B3I IMEM/DMEM models | Reorder-capable program IMEM/DMEM | Capacity DMEM that holds responses until 4 outstanding |

## Physical Evidence Boundary

PPA4 routed the full generated core inside `core_u`, not an adapter-only shell: `core_u=48458 LUT / 29567 FF`, compact total `49452 LUT / 31832 FF`, black boxes `0`, WNS `+0.434 ns`, TNS `0.000 ns`.

That physical result is still not a true-program PPA PASS because the PPA4 compact adapter's IMEM responder returns deterministic NOP instruction data. PPA6 and L3R6 functional/capacity runs prove real program execution on the same generated core RTL, but they do not add board XDC, bitstream DRC, or board timing signoff.

## Result

`L3_R6_RTL_PROVENANCE=PASS_SAME_CANONICAL_SOURCE_SAME_GENERATED_CORE_DIFFERENT_TOPS_AND_ADAPTERS`

`L3_R6_PRODUCT_CODE_MODIFIED=false`

`PPA_GATE_STATUS=UNRESOLVED`
