# PPA3 Constraint Audit

## Scope

This audit re-opened the PPA2 out-of-context routed checkpoint and replaced the original 10.000 ns clock with the L3 target period of 6.450 ns. The purpose was to confirm that the observed 6.450 ns timing failure was not caused by missing internal clocks, hidden timing exceptions, or unconstrained internal endpoints.

| Field | Value |
|---|---|
| PPA2 evidence commit | `cf4d4ab59f3150896ecd8fd2b6a2af2f87c732bc` |
| Script | `reports/gate6_0_full_lsu/l3/timing_closure_analysis/ppa3_timing_audit.tcl` |
| Input checkpoint | `/home/lab_726/opencode_tmp/g6_l3r5_ppa2_ooc_4_4_rerun_20261009/ooc_post_route.dcp` |
| Audit output root | `/home/lab_726/opencode_tmp/g6_l3r5_ppa3_timing_audit_20261009` |
| Tool | Vivado `2021.2`, SW Build `3367213`, IP Build `3369179` |
| Part | `xczu7ev-ffvc1156-2-e` |
| Top | `boom_core_top` |
| Product source modified | `false` |

## Clock Model

`report_clocks_6p450.rpt` shows a single propagated user clock:

| Clock | Period | Waveform | Source |
|---|---:|---|---|
| `ap_clk` | `6.450 ns` | `{0.000 3.225}` | `{ap_clk}` |

No generated clocks, user uncertainty, or user jitter constraints were reported.

## Timing Coverage

`check_timing_6p450.rpt` reported no missing clocks and no unconstrained internal endpoints:

| Check | Count | Interpretation |
|---|---:|---|
| `no_clock` | 0 | All sequential elements have a clock. |
| `constant_clock` | 0 | No sequential elements are clocked by constants. |
| `pulse_width_clock` | 0 | No pulse-width-only clock issues. |
| `unconstrained_internal_endpoints` | 0 | Internal max-delay paths are constrained. |
| `multiple_clock` | 0 | No multi-clock ambiguity. |
| `generated_clocks` | 0 | No disconnected generated clocks. |
| `loops` | 0 | No combinational loops. |
| `partial_input_delay` | 0 | No partial input-delay constraints. |
| `partial_output_delay` | 0 | No partial output-delay constraints. |
| `latch_loops` | 0 | No latch-loop timing issue. |

The same report also lists `520` input ports with no input delay and `1034` output ports with no output delay. These are expected for the current OOC/raw-HLS top context because the package-valid wrapper and board-level interface constraints do not exist yet. They do not explain the internal register-to-register setup failure, but they do mean the raw `boom_core_top` is not a final system timing-constraint environment.

## Timing Exceptions

`report_exceptions_6p450.rpt` reports:

```text
No valid timing exceptions found.
```

No false paths, multicycle paths, or clock groups were applied to hide timing.

## Methodology

`report_methodology_6p450.rpt` reports `1452` warnings:

| Rule | Count | Meaning |
|---|---:|---|
| `TIMING-16` | 452 | Large setup violation in the old routed checkpoint when re-analyzed at 6.450 ns. |
| `TIMING-18` | 1000 | Missing input or output delay on the raw/OOC top-level ports. |

The large setup violations align with the failing timing summary from the old 10 ns route re-analysis. The missing I/O delay warnings align with the known lack of a final wrapper and board-level constraints.

## Conclusion

For OOC internal timing analysis, the 6.450 ns clock is valid and internal sequential endpoints are constrained. The PPA2 6.450 ns failure is a real internal setup failure in the old route, not an artifact of false-path exceptions or unconstrained internal endpoints. System-level timing constraints remain incomplete until a deployable wrapper defines real external I/O timing.

| Field | Value |
|---|---|
| `L3_PPA3_CONSTRAINTS_VALID` | `OOC_INTERNAL_PASS_SYSTEM_IO_PENDING` |
| `L3_PPA3_PRODUCT_CODE_MODIFIED` | `false` |
