# PPA4 System Timing Report

Scope: compact PPA4 system top, post-route, `CONFIG_4_4`, `xczu7ev-ffvc1156-2-e`.

## Implementation Status

| Stage | Status | Evidence |
| --- | --- | --- |
| Setup | PASS | `/home/lab_726/opencode_tmp/g6_l3r5_ppa4_system_impl_compact_20261010/ppa4_impl_status.csv` |
| Opt | PASS | `/home/lab_726/opencode_tmp/g6_l3r5_ppa4_system_impl_compact_20261010/ppa4_post_opt.dcp` |
| Place | PASS | `/home/lab_726/opencode_tmp/g6_l3r5_ppa4_system_impl_compact_20261010/ppa4_post_place.dcp` |
| Phys opt | PASS | `/home/lab_726/opencode_tmp/g6_l3r5_ppa4_system_impl_compact_20261010/ppa4_post_phys_opt.dcp` |
| Route | PASS | `/home/lab_726/opencode_tmp/g6_l3r5_ppa4_system_impl_compact_20261010/ppa4_post_route.dcp` |

## Post-Route Timing

| Metric | Value |
| --- | ---: |
| Clock | `ap_clk` |
| Period | `6.450 ns` |
| Frequency | `155.039 MHz` |
| Setup WNS | `0.434 ns` |
| Setup TNS | `0.000 ns` |
| Setup failing endpoints | `0` |
| Hold WHS | `0.010 ns` |
| Hold THS | `0.000 ns` |
| Hold failing endpoints | `0` |
| Pulse width failing endpoints | `0` |
| Timing conclusion | `All user specified timing constraints are met.` |

## Route Status

| Metric | Value |
| --- | ---: |
| Logical nets | `108709` |
| Routable nets | `72881` |
| Fully routed nets | `72881` |
| Nets with routing errors | `0` |

## Worst Setup Path

| Field | Value |
| --- | --- |
| Slack | `0.434 ns` |
| Source | `core_u/grp_boom_core_cycle_io_fu_6197/grp_execute_module_fu_7402/rhs_reg_877_reg[32]/C` |
| Destination | `core_u/grp_boom_core_cycle_io_fu_6197/grp_execute_module_fu_7402/reg_761_reg[63]/D` |
| Requirement | `6.450 ns` |
| Data path delay | `5.891 ns` |
| Logic delay | `4.278 ns (72.619%)` |
| Route delay | `1.613 ns (27.381%)` |
| Logic levels | `18` |
| Dominant logic | DSP chain plus carry/LUT tail in execute module |

## Congestion

Post-route design analysis reports no placer congestion windows above level 5 and no effective router congestion windows above level 5.

## DRC

Post-route DRC has no errors. Remaining warnings:

| Rule | Severity | Count | Disposition |
| --- | --- | ---: | --- |
| `NSTD-1` | Critical Warning | 1 | Board XDC must assign I/O standards before bitstream. |
| `UCIO-1` | Critical Warning | 1 | Board XDC must assign pin LOC constraints before bitstream. |
| `DPIP-2` | Warning | 24 | Existing DSP input pipelining warning. |
| `DPOP-3` | Warning | 10 | Existing DSP PREG warning. |
| `DPOP-4` | Warning | 10 | Existing DSP MREG warning. |

## Artifact Hashes

| Artifact | SHA-256 |
| --- | --- |
| Post-synth DCP | `24eaad0fc098fabd260e59f5c27ae20f6ae20e9e02c57f9422db4f01e008c81e` |
| Post-route DCP | `740693b8631ebd45faeb1eeae12a86fd974789a9247ce562ef3f8a62f9e5bdb3` |
| Post-route timing summary | `1a3706caf76f0942afc9056184b734c24e485e0cdd6d44b092a82fc99a6267a2` |
| Post-route utilization | `ceff062308c83cd0e60673f5b5afcc1f89abf5cbdabc036ddb804a2fcb08ef93` |
| Post-route status | `8b62208f8f09c8ea30ba958ecbecc37e2ada0cf1440e001ba602ea2a70117e7c` |
| Post-route DRC | `1bf169de999114a9ff99017a8ee3170c90800ef5d7828c177180188822ff6e34` |

## Conclusion

`L3_PPA4_SYSTEM_TIMING=PASS_FOR_USER_SPECIFIED_CLOCK_CONSTRAINTS`

`L3_PPA4_BOARD_IO_SIGNOFF=PENDING_BOARD_XDC`
