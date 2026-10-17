# REF1 Reference Simulator Rebuild

## Rebuild Attempt

- Frozen reference hashes: all manifest entries and the required top, harness, full FIRRTL and core.config hashes match (`REF1_REFERENCE_MANIFEST_VALID=true`).
- Verilator command reports `/usr/local/bin/verilator` version 5.030; VTestHarness object archive timestamp predates this REF1 run and is not rebuilt here.
- Re-linking the standalone runner in the isolated Build Root failed. Full compiler log: `/home/lab_726/opencode_tmp/ref1_smallboom_20261017/reference/logs/standalone_runner_build.log`.
- Minimal blocker: generated `VTestHarness.h` includes `verilated_heavy.h`, but neither the recorded Verilator 4.038 include bundle nor current `/usr/local/share/verilator/include` contains that header. No reference RTL/Verilog was altered to bypass the failure.

## Fresh Runs Using Existing Generated Model

The existing standalone executable was run anew during REF1 against each of the five current reference program images. It is not an old trace replay. Existing generated model files have timestamps after the locked source files, but no per-source build manifest was embedded in the binary; therefore simulator source-to-model identity is marked `PREBUILT_MODEL_PROVENANCE_NOT_REBUILT`, not a clean rebuild PASS. Fresh output traces/logs and their hashes are in `reference_program_matrix.csv` and the external Build Root.

All five fresh invocations terminated through a retired tohost store with value 1, no max-cycle truncation and no trap (`reference_program_matrix.csv`). Existing clean build remains blocked until compatible `verilated_heavy.h` is restored or generated model is rebuilt in an isolated environment.
