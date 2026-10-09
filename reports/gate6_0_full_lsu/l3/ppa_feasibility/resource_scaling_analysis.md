# Resource Scaling Analysis

## Summary

All five measured configurations completed Vitis HLS CSynth with exit code `0`, no `ERROR:` lines, and no `CRITICAL WARNING:` lines. The LUT spread is small relative to the Gate 6 baseline, but absolute LUT utilization remains high: the worst measured HLS row is `4_16` at `215632 / 230400` LUTs, or `93.59%` of the target device LUT estimate.

## Five-Config Resource Matrix

| Config | LQ | SQ | LUT | LUT vs baseline | LUT util | FF | FF vs baseline | BRAM_18K | DSP | Estimated period |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| `4_4` | 4 | 4 | 211204 | -2232 (-1.05%) | 91.67% | 47985 | +648 (+1.37%) | 16 | 3 | 6.071 ns |
| `4_16` | 4 | 16 | 215632 | +2196 (+1.03%) | 93.59% | 50559 | +3222 (+6.81%) | 16 | 3 | 6.071 ns |
| `8_8` | 8 | 8 | 207950 | -5486 (-2.57%) | 90.26% | 46978 | -359 (-0.76%) | 16 | 3 | 6.071 ns |
| `16_4` | 16 | 4 | 208959 | -4477 (-2.10%) | 90.69% | 47473 | +136 (+0.29%) | 16 | 3 | 6.071 ns |
| `16_16` | 16 | 16 | 212856 | -580 (-0.27%) | 92.39% | 49733 | +2396 (+5.06%) | 16 | 3 | 6.071 ns |

## Observations

- The queue-depth sweep does not show monotonic LUT growth. `4_16` is the largest LUT row, while `8_8` is the smallest. This is consistent with HLS scheduling, inlining, and storage-binding effects dominating the marginal queue-depth changes at these sizes.
- BRAM_18K remains fixed at `16` and DSP remains fixed at `3` across all five configurations.
- FF count grows most in `4_16` and `16_16`, which is consistent with additional queue state and control retention.
- Every HLS row is below the L3 review threshold of `228375` LUT and below the historical global review threshold of `234780` LUT.
- HLS resource estimates are not post-place/post-route utilization. The implementation attempt shows that raw HLS top-level I/O can block placement even when core LUT/FF estimates are under budget.

## Resource Conclusion

Current HLS resource scaling is feasible against the L3 planned LUT threshold, but it is not sufficient for physical signoff. The design remains close to the LUT capacity of `xczu7ev-ffvc1156-2-e`, and physical implementation requires an integration wrapper or I/O reduction strategy before route timing can be measured.
