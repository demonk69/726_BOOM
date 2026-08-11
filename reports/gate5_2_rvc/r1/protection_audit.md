# Gate 5.2 R1 Protection Audit

- `src/frontend.cpp`: hash-identical, `c6821379...107ac2`.
- `src/decode.cpp`: hash-identical, `4be756ba...27904`.
- All frozen state/config/backend/directive/reference hashes match the R1 baseline.
- `src/boom_all.cpp`: hash `d6f88563...2c76c`, unchanged across this repair,
  excluded from generation, compilation, synthesis, tests, and acceptance.
- `src/synth_module_tops.cpp` remains unchanged; RVC uses a separate top file.
- `scripts/generate_merged.sh` contains no `boom_all.cpp` binding.
- Final merged source contains `rvc.cpp`, `decompress_rvc`, and `synth_rvc_top`
  exactly once and contains no `boom_all.cpp` reference.
- Decode/dispatch/commit widths remain 1/1/1; ROB/IQ/PRF capacities are unchanged.
- No `DATAFLOW`, false-dependence, or complete-partition directive was added.
- `CORE_CYCLE` and both required csynth tops remain `Pipelined=no`.
- No Frontend integration, PC+2, halfword/cross-word state, Fetch Buffer, or
  backend widening is present.

Protection result: **PASS**.
