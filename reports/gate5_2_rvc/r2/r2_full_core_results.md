# Gate 5.2 R2 full-core mixed RVC coverage

## Results

| Mode | Result | Evidence |
|---|---:|---|
| Program build/raw parcel audit | 10/10 PASS | `logs/r2_program_build.log`, generated dumps and byte images |
| Native canonical `boom_core_step` | 10/10 PASS | `logs/r2_native.log` |
| Vitis HLS 2021.2 csim | 10/10 PASS | `logs/r2_csim.log` |
| Generated canonical `boom_core_top` XSim | 10/10 PASS | `logs/r2_rtl/*.log`, `r2_rtl_traces/*.jsonl`, `r2_full_core_program_matrix.csv` |

Native and csim each passed all independently specified register signatures, observed the data-memory `tohost` store with value 1, found the corresponding committed store, and observed `state.tohost == 1`. Every raw image was parsed as a byte stream before execution and required at least one legal compressed parcel and one 32-bit instruction. The parser explicitly rejects `C.EBREAK` and `C.SRLI shamt[5]=1`.

`rvc_cross_boundary` begins with bytes `29 44 93 04 94 00 89 04`: the 32-bit `addi` starts at byte offset 2 and is split across aligned IMEM words `04934429` and `04890094`. `rvc_redirect_halfword` has a taken 32-bit branch to byte offset `0x0a`, exercising a target whose address is 2 modulo 4.

## Artifacts

- Sources and generated ELF/binary/dumps/loadmem: `tb/programs/rvc_fetch/`
- Native/csim harness: `tb/differential/gate5_2_r2_full_core_rvc.cpp`
- R2 RTL wrapper: `tb/differential/gate5_2_r2_rtl_harness.sv`
- Build and runners: `scripts/gate5_2/`
- Program and signature matrix: `r2_full_core_program_matrix.csv`

The builder emits `.words.hex` as aligned 32-bit little-endian memory values and `.hex` as packed 64-bit readmemh beats for the existing RTL IMEM model. Compressed instructions remain two bytes in both formats; no pre-expansion is performed.

## Commands

```text
bash scripts/gate5_2/build_rvc_programs.sh
bash scripts/gate5_2/run_r2_native.sh
bash scripts/gate5_2/run_r2_csim.sh
bash scripts/gate5_2/run_r2_full_core_rtl.sh
```

All four completed successfully. The final RTL was generated from `/home/lab_726/boom/hls_boom/boom_hls_gate5_2_rvc_r2_repair_core_boom_core_top/solution_module/syn/verilog`; each program reached the expected final architectural register values and committed `tohost=1` at address `0x80000080`, with a final trace under `r2_rtl_traces/`. The earlier timeout note is historical and superseded by this final run.
