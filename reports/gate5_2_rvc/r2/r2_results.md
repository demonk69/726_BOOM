# Gate 5.2 R2 RV64C Fetch Results

## Final Status

- `GATE5_2_R2_RVC_FETCH_VERIFIED=true`
- `READY_FOR_GATE5_2_R3_DECODE_GAP_CLOSURE=true`
- `GATE5_2_RVC_VERIFIED=false`
- `READY_FOR_FETCH_BUFFER=false`
- `GATE5_2_R2_PPA_BLOCKER=false`
- `M009=PARTIALLY_VERIFIED`
- `M014=VERIFIED`
- `READY_FOR_OFFICIAL_GATE_3=false`
- `READY_FOR_FULL_LSU_IMPLEMENTATION=false`

R2 verifies the implemented one-wide mixed 16/32-bit fetch path. It does not verify complete RV64C because 288 legal forms remain deliberately protected as illegal pending R3: one `C.EBREAK`, 256 `C.SRLI shamt[5]=1`, and 31 `C.JALR` forms.

## Required Answers 1-32

1. **Canonical IMEM payload:** 32 data bits, four little-endian bytes per exact 4-byte-aligned response; no byte mask and no 64-bit fetch payload.
2. **Parcel selection:** PC bit 1 selects lower or upper 16-bit parcel; PC bit 0 must be zero.
3. **Compressed progression:** accepted compressed instruction advances architectural PC by 2.
4. **Base progression:** accepted 32-bit instruction advances architectural PC by 4.
5. **Cross-word assembly:** an upper-position low half is retained and combined as `{next_word[15:0], saved_halfword}`.
6. **Retained state:** one response word and one cross-word halfword carry only, listed in `frontend_state_delta.csv`.
7. **Request ownership:** one logical outstanding request with exact `{fetch_id, epoch, address}` response matching remains enforced.
8. **Stale handling:** wrong ID, epoch, or address is drained with no publication or retained-state side effect.
9. **Backpressure:** Decode hold preserves PC, expanded instruction, original compressed bits, fault, and attribution until accepted.
10. **Redirects:** even targets, including addresses 2 modulo 4, are supported; redirects clear retained word/carry and advance the epoch.
11. **Misalignment:** odd targets publish a precise instruction-address-misaligned fault and issue no request.
12. **Fetch faults:** a cross-word upper access fault is attributed once to the saved instruction PC; no partial instruction is decoded.
13. **Long encodings:** parcel patterns denoting instructions longer than 32 bits are explicit illegal instructions; 48/64/80+ assembly is absent.
14. **Protected Decode gaps:** `C.EBREAK` 1 form, `C.SRLI shamt5` 256 forms, and `C.JALR` 31 forms.
15. **C.JALR reason:** frozen Execute writes PC+4 as link, while compressed JALR requires PC+2; R2 therefore does not claim support.
16. **Protected behavior:** all three classes produce explicit illegal-instruction cause 2 and retain original compressed bits and PC.
17. **Focused native:** 4,111 assertions across 414 cases, 288 protected cases, zero failures; `logs/rvc_fetch_tests.log`.
18. **Persistent random:** 256 seeds x 2,048 cycles = 524,288 cycles, zero errors; `logs/rvc_fetch_random_tests.log`.
19. **Focused generated RTL:** 58/58 PASS with 24 requests and 24 responses; `logs/xsim.log` and `rtl_test_matrix.csv`.
20. **Mixed full-core:** program build, native, Vitis csim, and canonical generated `boom_core_top` RTL each pass 10/10; `r2_full_core_results.md` and `r2_full_core_program_matrix.csv`.
21. **Gate 5.1 preservation:** focused generated RTL passes 33/33; `logs/gate5_1_preservation/xsim.stdout.log`.
22. **Older software preservation:** repaired-source W3 400/400; W4 directed 95/95 and persistent random 128/128; full-program 10/10 and partial-order 7/7 PASS under `old_regression/current_w4e/`.
23. **Older generated RTL preservation:** repaired canonical RTL Gate 3.9 passes 49/49; current-source M3C focused, W3 focused, and W4 focused pass 30/30, 11/11, and 20/20 under `old_regression/current_*`.
24. **RV64M preservation:** repaired-source native, Vitis csim, and repaired canonical full-core RTL pass 15/15 under `old_regression/current_m3c/`, `current_m3c_csim/`, and `current_m3c_full_rtl/`.
25. **Canonical synthesis:** all ten requested final tops PASS; exact LUT/FF/BRAM/DSP/period/latency/interval/`PipelineType` and absolute XML paths are in `resource_summary.csv`.
26. **Full-core PPA:** `boom_core_top` is 126,798 LUT, 28,492 FF, 16 BRAM_18K, 3 DSP, 6.341 ns; versus the frozen Gate 5.1 baseline this is +2,481 LUT, +979 FF, unchanged BRAM/DSP/period.
27. **Critical path/PPA blocker:** Execute remains the 6.341 ns full-core child path; RVC Frontend is not critical and the 10 ns target closes, so `GATE5_2_R2_PPA_BLOCKER=false`.
28. **Scheduling/directives:** `CORE_CYCLE Pipelined=no`; no DATAFLOW, false-dependence, or complete-array-partition directive is accepted.
29. **Widths/topology:** fetch/decode/dispatch/commit widths remain one; backend issue/execute/completion/PRF topology is unchanged.
30. **Deferred frontend structures:** no Fetch Buffer, FTQ, predictor, RAS, BTB, or ICache is implemented; `READY_FOR_FETCH_BUFFER=false`.
31. **Acceptance boundary:** R2 fetch is verified, but the 288 protected legal forms prevent full Gate 5.2 verification; R3 Decode-gap closure is next.
32. **Global status:** `M009=PARTIALLY_VERIFIED`, `M014=VERIFIED`, `READY_FOR_OFFICIAL_GATE_3=false`, and `READY_FOR_FULL_LSU_IMPLEMENTATION=false` are preserved; `src/boom_all.cpp` is excluded.

## Canonical PPA

| Top | LUT | FF | BRAM | DSP | Period ns | Latency | Interval | PipelineType |
|---|---:|---:|---:|---:|---:|---|---|---|
| `boom_core_top` | 126798 | 28492 | 16 | 3 | 6.341 | undef | undef | no |
| `synth_core_step_top` | 119210 | 28085 | 16 | 3 | 6.341 | undef | undef | no |
| `synth_completion_top` | 38034 | 10811 | 8 | 0 | 5.474 | undef | undef | no |
| `synth_divider_top` | 3429 | 344 | 0 | 0 | 5.734 | 1-2 | 2-3 | no |
| `synth_execute_top` | 6773 | 916 | 8 | 3 | 6.411 | 11-222 | 12-223 | no |
| `synth_frontend_top` | 1479 | 718 | 0 | 0 | 4.379 | 4 | 5 | no |
| `synth_issue_top` | 18001 | 4930 | 0 | 0 | 3.567 | undef | undef | no |
| `synth_mul_top` | 709 | 264 | 0 | 3 | 4.717 | 1 | 2 | no |
| `synth_rob_top` | 29641 | 8155 | 1 | 0 | 3.746 | undef | undef | no |
| `synth_rvc_top` | 1022 | 0 | 0 | 0 | 1.845 | 0 | 1 | no |

The authoritative machine-readable table is `resource_summary.csv`; each row records the absolute final XML path. These are HLS estimates, not post-route utilization or STA.

## Evidence Root

`/home/lab_726/boom/hls_boom/reports/gate5_2_rvc/r2/`

Key final logs are `logs/rvc_fetch_tests.log`, `logs/rvc_fetch_random_tests.log`, `logs/rvc_throughput.log`, `logs/rvc_decompress_tests.log`, `logs/rvc_decode_cross_tests.log`, `logs/xsim.log`, `logs/r2_native.log`, `logs/r2_csim.log`, `logs/r2_rtl/*.log`, `logs/gate5_1_preservation/xsim.stdout.log`, and `logs/canonical_csynth/`. Regression paths are enumerated in `regression_after.md`; artifacts and hashes are enumerated in `artifact_manifest.csv` and `source_hashes_after.txt`.
