# Gate 5.2 R2 Historical Full-Core RTL Timeout

Final status: **SUPERSEDED; generated canonical RTL 10/10 PASS**

This file records the earlier timed-out attempt only. Final canonical RTL was subsequently generated at `/home/lab_726/boom/hls_boom/boom_hls_gate5_2_rvc_r2_repair_core_boom_core_top/solution_module/syn/verilog`, and all ten XSim programs passed. The authoritative final evidence is `r2_full_core_results.md`, `r2_full_core_program_matrix.csv`, `r2_full_core_rtl_matrix.csv`, `logs/r2_rtl/*.log`, and `r2_rtl_traces/*.jsonl`.

At the time of this historical attempt, the workspace contained no generated Gate 5.2 canonical `boom_core_top` RTL. Gate 5.1 RTL was correctly not used as R2 product evidence.

The R2 runner invoked canonical synthesis from the modular production sources with `set_top boom_core_top`:

```text
bash scripts/gate5_2/run_r2_full_core_rtl.sh
```

The command ran for 1,200 seconds and was terminated by the execution timeout while Vitis HLS was still in `csynth_design`. The last completed log action was scheduling `try_issue_load` with final II 66. At termination:

- `tb/differential/gate5_2_r2_rtl_build/r2_boom_core_top/solution_r2_rtl/syn/verilog/boom_core_top.v` did not exist.
- No `*csynth.rpt` existed under the R2 build.
- XSim compilation and all 10 XSim program runs were therefore not started.

Primary evidence: `reports/gate5_2_rvc/r2/logs/r2_rtl/csynth.log`, lines 60 and 2811-2835.

Exact continuation command, with a sufficiently long external timeout:

```text
bash scripts/gate5_2/run_r2_full_core_rtl.sh
```

The runner rejects a missing canonical `boom_core_top.v`, builds an R2-only wrapper after successful synthesis, runs all ten images in XSim, and independently checks register signatures and committed `tohost=1` from JSONL commit traces.
