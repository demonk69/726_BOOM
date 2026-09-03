# Gate 5.4 PF0 Baseline Manifest

- Requested repository path: `/home/lab_726/boom`
- Actual nested Git worktree: `/home/lab_726/boom/hls_boom`
- Branch: `gate3.8-rtl-verification`
- HEAD: `a905d65d00f3c27f6bb2f1cfda7bbfc83c72edac`
- Accepted HEAD is the current HEAD and therefore is in its history.
- Gate 5.3 final: `ef051ca4e3673d663f9d10b55029c956fbc0052a`
- P1 CFI Predecode: `fb1f34719dd09332e54cbc0766dd68df81bd545a`
- P2 Predictor Foundation: `5bc5644b1b47697a439ce1605b87068b76eee68d`
- Repository Hygiene R1: `7b6a7a66bf190f8d8faa990f3f88bcb99de99c8a`
- F1 Standalone FTQ: `a905d65d00f3c27f6bb2f1cfda7bbfc83c72edac`
- Default build root: `/tmp/boom_hls`
- Review mode: read-only architecture audit; no build or synthesis was run.

## Frozen Product Baseline

```text
GATE5_3_FETCH_BUFFER_VERIFIED=true
FETCH_BUFFER_DEPTH=8
FETCH_BUFFER_STORAGE=AUTO
FETCH_BUFFER_RESET_POLICY=CONTROL_ONLY
FETCH_PACKET_WIDTH=2
IMEM_RESPONSE_BITS=32
PARCELS_PER_RESPONSE=2
ONE_LOGICAL_OUTSTANDING_IMEM_REQUEST=true
MULTI_RESPONSE_PACKET_AGGREGATION=false
DECODE_WIDTH=1
DISPATCH_WIDTH=1
COMMIT_WIDTH=1
FULL_CORE_BASELINE_LUT=135953
FULL_CORE_BASELINE_FF=33373
FULL_CORE_BASELINE_BRAM=16
FULL_CORE_BASELINE_DSP=3
FULL_CORE_BASELINE_PERIOD_NS=6.341
CORE_CYCLE_PIPELINED=false
```

Historical dirty and retained evidence paths were not cleaned, reset, restored, stashed, or deleted. `src/boom_all.cpp` was excluded.
