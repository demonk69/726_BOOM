# Gate 4.0 W4 Final Source Scope

The canonical final product is the modular implementation listed in `product_source_hashes.txt`. It contains seven public headers and fourteen product source files, including `boom_core_step.cpp` and `boom_core_top.cpp`, but excludes diagnostic wrappers.

The no-partition focused diagnostic is separately listed in `diagnostic_source_hashes.txt`. Its top in `synth_module_tops.cpp` invokes two real `boom_core_step` transitions and is verified by C++/csim/generated RTL. Diagnostic-only additions do not alter the modular product source.

`source_hashes_after.txt` binds the complete final verification and synthesis input set. `csynth_source_binding.json` binds that manifest, the modular product manifest, current merged source, diagnostic wrapper, and all seven clean final csynth targets.

The preserved 49/49 full-core RTL was not rerun because `product_rtl_binding.json` proves all 21 modular product files match the generation source and all 87 generated product RTL files verify unchanged. `full_core_source_hashes.txt` now carries the current modular product scope; diagnostic and merged hashes are intentionally separate.

`src/boom_all.cpp` is explicitly excluded. It is not canonical source and was not read, modified, compiled, used, or hashed for W4 signoff.
