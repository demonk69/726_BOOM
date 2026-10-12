# PPA6 RTL Provenance Comparison

| Item | PPA4 compact physical | PPA5 B3I functional | PPA6 multi-outstanding functional |
| --- | --- | --- | --- |
| Product source | `src/boom_core_merged.cpp` | `src/boom_core_merged.cpp` | `src/boom_core_merged.cpp` |
| Product source SHA-256 | `589f5b5c90bbbbaadc4e2d30c14205c5dc8e5c4c6a904e27de94a4255cd9a5f7` | `589f5b5c90bbbbaadc4e2d30c14205c5dc8e5c4c6a904e27de94a4255cd9a5f7` | `589f5b5c90bbbbaadc4e2d30c14205c5dc8e5c4c6a904e27de94a4255cd9a5f7` |
| HLS config | `LQ_DEPTH=4 SQ_DEPTH=4` | `LQ_DEPTH=4 SQ_DEPTH=4` | `LQ_DEPTH=4 SQ_DEPTH=4` |
| Tool | Vitis/Vivado 2021.2 | Vivado xsim 2021.2 | Vivado xsim 2021.2 |
| Generated RTL dir | `/home/lab_726/opencode_tmp/g6_l3r2_r5_preflight_4_4_20261009_155117/hls_project/solution/syn/verilog` | `/home/lab_726/opencode_tmp/g6_l3r2_r5_preflight_4_4_20261009_155117/hls_project/solution/syn/verilog` | `/home/lab_726/opencode_tmp/g6_l3r2_r5_preflight_4_4_20261009_155117/hls_project/solution/syn/verilog` |
| RTL top SHA-256 | `9adbf00aa02cc89def9f73d34f860e2865bef0c1771106f94bce97f6e5459659` | `9adbf00aa02cc89def9f73d34f860e2865bef0c1771106f94bce97f6e5459659` | `9adbf00aa02cc89def9f73d34f860e2865bef0c1771106f94bce97f6e5459659` |
| RTL aggregate SHA-256 | `7fb7061044620afddeda70abf0dae0c77bbd95583a982f42e2abe5c7fd251a44` | `7fb7061044620afddeda70abf0dae0c77bbd95583a982f42e2abe5c7fd251a44` | `7fb7061044620afddeda70abf0dae0c77bbd95583a982f42e2abe5c7fd251a44` |
| Top module | `boom_l3_ppa4_system_top` wrapping `boom_core_top` | `boom_core_top` | `boom_core_top` |
| Adapter/memory model | PPA4 compact adapter returns NOP IMEM | B3I program IMEM/DMEM models | PPA6 reorder-capable IMEM/DMEM testbench |
| Provenance class | `SAME_SOURCE_SAME_GENERATED_CORE_DIFFERENT_TOP` | `SAME_RTL_CORE_FUNCTIONAL_TOP` | `SAME_RTL_CORE_FUNCTIONAL_TOP` |

PPA4 physical route and PPA6 functional simulation use the same generated core RTL directory and hash, but not the same top-level wrapper. PPA4's compact adapter still returns NOP instructions, so PPA4 physical system evidence does not prove execution of the PPA6 program.
