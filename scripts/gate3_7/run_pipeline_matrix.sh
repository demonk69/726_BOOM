#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
RUNNER="$ROOT/scripts/gate3_7/run_pipeline_variant.sh"
ANALYZER="$ROOT/scripts/gate3_7/analyze_pipeline_log.py"
P0_TIMEOUT=${GATE37_P0_TIMEOUT:-300}
P1_P4_TIMEOUT=${GATE37_P1_P4_TIMEOUT:-900}
P5_P6_TIMEOUT=${GATE37_P5_P6_TIMEOUT:-1800}

"$RUNNER" P0_BASELINE none "$P0_TIMEOUT"
"$RUNNER" P1_PIPELINE_NO_II auto "$P1_P4_TIMEOUT"

if [ ! -f "$ROOT/reports/gate3_7/variants/P1_PIPELINE_NO_II/boom_core_top_csynth.rpt" ]; then
  for spec in P2_PIPELINE_II_16:16 P3_PIPELINE_II_8:8 P4_PIPELINE_II_4:4 P5_PIPELINE_II_2:2 P6_PIPELINE_II_1:1; do
    variant=${spec%%:*}
    ii=${spec##*:}
    python3 "$ANALYZER" --root "$ROOT" --variant "$variant" --requested-ii "$ii" \
      --status NOT_RUN_BLOCKED_BY_P1 --exit-code 0 --timeout-seconds 0
  done
  python3 "$ANALYZER" --root "$ROOT" --aggregate
  exit 0
fi

"$RUNNER" P2_PIPELINE_II_16 16 "$P1_P4_TIMEOUT"
"$RUNNER" P3_PIPELINE_II_8 8 "$P1_P4_TIMEOUT"
"$RUNNER" P4_PIPELINE_II_4 4 "$P1_P4_TIMEOUT"
"$RUNNER" P5_PIPELINE_II_2 2 "$P5_P6_TIMEOUT"

all_reports=1
for variant in P1_PIPELINE_NO_II P2_PIPELINE_II_16 P3_PIPELINE_II_8 P4_PIPELINE_II_4 P5_PIPELINE_II_2; do
  if [ ! -f "$ROOT/reports/gate3_7/variants/$variant/boom_core_top_csynth.rpt" ]; then all_reports=0; fi
done

if [ "$all_reports" -eq 1 ]; then
  "$RUNNER" P6_PIPELINE_II_1 1 "$P5_P6_TIMEOUT"
else
  python3 "$ANALYZER" --root "$ROOT" --variant P6_PIPELINE_II_1 --requested-ii 1 \
    --status NOT_RUN_PRIOR_MATRIX_INCOMPLETE --exit-code 0 --timeout-seconds 0
fi

python3 "$ANALYZER" --root "$ROOT" --aggregate
