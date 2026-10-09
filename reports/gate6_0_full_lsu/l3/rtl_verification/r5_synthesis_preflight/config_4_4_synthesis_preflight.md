# CONFIG_4_4 Synthesis Preflight

R4 evidence commit: `61f6d57ef54c7b9cb6e9cebd34e30e26c1a04637`.

Command:

```bash
HLS_BOOM_ROOT=/home/lab_726/boom/hls_boom G6_L2_SYNTH_WORK=/home/lab_726/opencode_tmp/g6_l3r2_r5_preflight_4_4_20261009_155117 LQ_DEPTH=4 SQ_DEPTH=4 timeout 7200 /home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls -f /home/lab_726/boom/hls_boom/scripts/gate6_0/l2_full_core_csynth.tcl
```

Result: `CONFIG_4_4_SYNTHESIS_PREFLIGHT=PASS`.

Exit status: `0`.

Runtime: `510.90s` real, `506.85s` user, `5.33s` sys.

Build root: `/home/lab_726/opencode_tmp/g6_l3r2_r5_preflight_4_4_20261009_155117`.

Log: `reports/gate6_0_full_lsu/l3/rtl_verification/r5_synthesis_preflight/config_4_4_csynth.log`.

## Synthesis Summary

| Field | Value |
| --- | --- |
| Tool | Vitis HLS 2021.2, Build 3367213 |
| Top | `boom_core_top` |
| Part | `xczu7ev-ffvc1156-2-e` |
| Clock target | `10.00 ns` |
| Estimated clock | `6.071 ns` |
| Clock uncertainty | `2.70 ns` |
| Top latency / interval | `?` / `?` |
| Pipeline | `no` |

The unknown top latency and interval are expected for the free-running `CORE_CYCLE` loop with `ap_ctrl_none` top-level control.

The HLS Tcl clock constraint is `create_clock -period 10 -name default`; Vitis renders this target as `10.00 ns` in the raw CSynth report.

## Utilization

| Resource | Used | Available | Utilization |
| --- | ---: | ---: | ---: |
| BRAM_18K | 16 | 624 | 2% |
| DSP | 3 | 1728 | ~0% |
| FF | 47985 | 460800 | 10% |
| LUT | 211204 | 230400 | 91% |
| URAM | 0 | 96 | 0% |

Timing preflight passes against the 10 ns target. LUT utilization remains high at 91%, so larger configuration synthesis should be treated as capacity-sensitive.

## Interface Check

| Interface | Mode | Width | Register mode |
| --- | --- | ---: | --- |
| `imem_req_out` | AXIS out | 192 | both |
| `imem_resp_in` | AXIS in | 256 | both |
| `dmem_req_out` | AXIS out | 320 | both |
| `dmem_resp_in` | AXIS in | 384 | both |
| `commit_trace_out` | AXIS out | 768 | both |
| `ap_ctrl` | control |  | `ap_ctrl_none` |

The `dmem_req_out` register mode remains `both`, matching the R3/R4 backpressure reconciliation basis.

## Log Audit

| Marker | Count |
| --- | ---: |
| `ERROR:` | 0 |
| `CRITICAL WARNING` | 0 |
| `WARNING:` | 736 |

Representative warnings are existing HLS warnings for internal stream sizing, the infinite `CORE_CYCLE` loop, function-name legalization, AXIS reset mode, and `ap_none` output ports. No new product source was edited for this preflight.

## Provenance

The synthesis input file is `src/boom_core_merged.cpp`, whose worktree hash is `589f5b5c90bbbbaadc4e2d30c14205c5dc8e5c4c6a904e27de94a4255cd9a5f7` and matches the clean RTL source provenance. The same hash is present at commit `61f6d57ef54c7b9cb6e9cebd34e30e26c1a04637`.

HLS Tcl SHA256: `51071bbd8e083876454ada347786f856cf399e5d09d78d32b717cc130e4c9be0`.

Directive file SHA256: `4e37fa6f60d80b71056dd966f48c92ad03b10e122dd1fcb367def3c0d14ececd`.

The pre-existing dirty `src/boom_all.cpp` was not used by this HLS Tcl.

The regenerated CONFIG_4_4 RTL aggregate hash is `2c1d683260694cb949658c15ce59564cf721e371d25a176a658489c0aadeb906`, matching the R4 CONFIG_4_4 RTL hash.
