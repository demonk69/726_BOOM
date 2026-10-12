# OOC vs System Comparison

This report compares the committed PPA3 OOC route with the PPA4 experimental system wrapper route.

## Summary

| Metric | PPA3 OOC core | PPA4 compact system top |
| --- | ---: | ---: |
| Evidence commit | `5a2ce0bc6b4f245747b9c0b4b13a1a2bfb6b5db6` | Experimental system-integration reports |
| Top | `boom_core_top` OOC | `boom_l3_ppa4_system_top` |
| Clock target | `6.450 ns` | `6.450 ns` |
| Placement | PASS | PASS |
| Routing | PASS | PASS |
| Post-route setup WNS | `0.086 ns` | `0.434 ns` |
| Post-route setup TNS | `0.000 ns` | `0.000 ns` |
| Setup failing endpoints | `0` | `0` |
| Route errors | `0` | `0` |
| Bonded IOB | `0/360` | `265/360` |
| CLB LUTs | `52806/230400` | `49452/230400` |
| CLB registers | `35419/460800` | `31832/460800` |
| BRAM tiles | `30/312` | `28/312` |
| DSPs | `16/1728` | `16/1728` |

## Interpretation

PPA3 proved the generated core fabric can close at 6.450 ns in OOC mode but intentionally excluded package I/O. PPA4 adds a package-level system top that internalizes the wide HLS streams and routes with real package IOB insertion.

The compact PPA4 wrapper fixes the raw-top I/O overutilization and avoids the initial byte-array adapter LUT explosion. The resulting system top routes at the target clock and uses 73.61 percent of bonded IOB capacity.

## Remaining Difference

PPA4 is still not board bitstream signoff. The route used no board-level pin LOC assignments, I/O standards, input delays, or output delays. Vivado therefore reports `NSTD-1` and `UCIO-1` critical warnings. Those are expected until a board XDC exists, but they prevent claiming bitstream-ready board signoff.

## Status

`L3_PPA3_OOC_TIMING=PASS`

`L3_PPA4_PACKAGE_LEVEL_ROUTE=PASS`

`L3_PPA4_BOARD_BITSTREAM_SIGNOFF=PENDING_BOARD_XDC`
