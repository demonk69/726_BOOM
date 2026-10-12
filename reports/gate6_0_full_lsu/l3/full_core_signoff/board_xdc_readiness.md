# Board XDC Readiness

## Part And Package

| Field | Value | Evidence |
| --- | --- | --- |
| FPGA exact part | `xczu7ev-ffvc1156-2-e` | PPA4 Tcl and DCP audit. |
| Device | `xczu7ev` | Vivado reports. |
| Package | `ffvc1156` | PPA4 IO reports. |
| Board model | unknown | No board definition found. |
| Board revision | unknown | No board definition found. |
| Board XDC | missing | No `*.xdc` files found under repo. |
| Clock source | unspecified board-level source | PPA4 creates only `create_clock -period 6.450 -name ap_clk`. |
| Reset source | unspecified board-level source | `ap_rst_n` is unconstrained for board location and I/O standard. |

## DRC Status

| Rule | Status | Evidence | Required Action |
| --- | --- | --- | --- |
| `NSTD-1` | CRITICAL_WARNING | 265 of 265 logical ports use `IOSTANDARD=DEFAULT`. | Add real board I/O standards. |
| `UCIO-1` | CRITICAL_WARNING | 265 of 265 logical ports have no `LOC`. | Add real board pin locations. |
| Bitstream | NOT_RUN | No board XDC and DRC critical warnings remain. | Do not run final signoff until board definition exists. |

No DRC severity was lowered. No guessed pin locations or guessed I/O standards were added.

## 719 vs 360 I/O Capacity

The earlier `719` number came from Vivado placer/package available user-I/O accounting in the raw-top placement failure. The `360` number comes from the Vivado utilization table for `Bonded IOB` resources on `xczu7ev-ffvc1156-2-e`.

Both views show the raw `1940`-I/O HLS top is impossible as a package top. For board signoff, the stricter practical requirement is not just capacity; every external port must have a board-valid package pin, bank voltage, and I/O standard.

## Status Fields

`L3_PPA5_BOARD_XDC_STATUS=PENDING_BOARD_DEFINITION`

`L3_PPA5_NSTD_1_STATUS=CRITICAL_WARNING_UNRESOLVED`

`L3_PPA5_UCIO_1_STATUS=CRITICAL_WARNING_UNRESOLVED`

`L3_PPA5_BITSTREAM_STATUS=NOT_RUN_BOARD_XDC_PENDING`
