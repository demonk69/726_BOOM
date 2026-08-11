# Gate 5.1R R2 Results

Result: **PASS, 33/33 generated-RTL cases observable and passing**.

- `synth_frontend_verify_top` calls canonical `boom::frontend_module` and `boom::decode_module`; it does not duplicate Frontend next-state logic and is not a product synthesis entry.
- Explicit `runtime_reset` clears focused validity state and enters the canonical `reset_done=false` restart path. `ap_rst_control_only` remains a separate characterization.
- The original 20 cases and all 13 formerly blocked cases pass in `rtl_test_matrix.csv`.
- Redirect priority/response rejection, ROB allocation ownership, epoch rejection, stale drain under Decode stall, held instruction/fault stability, misalignment, and runtime reset are top-level observable.
- Final candidate rerun log: `logs/xsim.log`; terminal marker: `FRONTEND_VERIFY_RTL_PASS cases=33`.

No `boom_core_top` interface or architectural product behavior was changed by the wrapper.
