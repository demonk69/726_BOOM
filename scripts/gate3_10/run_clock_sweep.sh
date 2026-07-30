#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
OUT="$ROOT/reports/gate3_10/clock_target_sweep.csv"
printf 'configuration,variant,requested_period,estimated_period,timing_met,critical_path,lut,ff,bram,dsp,runtime_seconds\n' > "$OUT"
for config in P0 R1; do
  for period in 6.0 5.5 5.0 4.5; do
    suffix=${period/./_}
    variant="SWEEP_${config}_${suffix}"
    "$ROOT/scripts/gate3_10/run_local_variant.sh" "$variant" "$period"
    python3 "$ROOT/scripts/gate3_10/summarize_clock_point.py" \
      --configuration "$config" --variant "$variant" --requested "$period" \
      --report "$ROOT/reports/gate3_10/variants/$variant/boom_core_top_csynth.xml" \
      --time "$ROOT/reports/gate3_10/variants/$variant/csynth.time" >> "$OUT"
  done
done
