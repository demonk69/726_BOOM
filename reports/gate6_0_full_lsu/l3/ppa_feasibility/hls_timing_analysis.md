# HLS Timing Analysis

## CSynth Timing

All five configurations report the same top-level timing estimate:

| Config | Target period | Estimated period | Clock uncertainty | Latency | Interval | Pipeline |
|---|---:|---:|---:|---|---|---|
| `4_4` | 10.00 ns | 6.071 ns | 2.70 ns | `undef` | `undef` | no |
| `4_16` | 10.00 ns | 6.071 ns | 2.70 ns | `undef` | `undef` | no |
| `8_8` | 10.00 ns | 6.071 ns | 2.70 ns | `undef` | `undef` | no |
| `16_4` | 10.00 ns | 6.071 ns | 2.70 ns | `undef` | `undef` | no |
| `16_16` | 10.00 ns | 6.071 ns | 2.70 ns | `undef` | `undef` | no |

The L3 planning period limit is `6.450 ns`; all CSynth estimates are under that limit by `0.379 ns`. The top function is non-pipelined and contains an unbounded `CORE_CYCLE` loop, so HLS reports top-level latency and interval as undefined.

## Synthesized Netlist Timing From 4_4 Implementation Attempt

The `CONFIG_4_4` Vivado implementation attempt reached synthesized timing before failing placement.

| Stage | Result |
|---|---|
| XDC clock | `create_clock -period 10.000 -name ap_clk [get_ports ap_clk]` |
| Setup endpoints | 0 failing |
| Setup WNS | 4.415 ns |
| Setup TNS | 0.000 ns |
| Hold endpoints | 30265 failing |
| Hold WHS | -0.082 ns |
| Hold THS | -1244.281 ns |
| Pulse width endpoints | 0 failing |

The worst setup path in the synthesized report is from `grp_boom_core_cycle_io_fu_6197/grp_execute_module_fu_7402/rhs_reg_877_reg[31]/C` to `grp_boom_core_cycle_io_fu_6197/grp_execute_module_fu_7402/high_reg_6457_reg[62]/D`, with `5.430 ns` data path delay and `4.415 ns` slack against a `10.000 ns` requirement.

## Timing Conclusion

CSynth timing is stable across the five depth configurations and below the L3 planned period limit. Post-place and post-route timing are unavailable because placement fails before routing. The synthesized hold violations cannot be treated as final route timing; they remain a physical implementation risk to revisit after placement is made feasible.
