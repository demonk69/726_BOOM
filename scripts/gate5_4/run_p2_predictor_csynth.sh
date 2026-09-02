#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}

BOOM_HLS_GATE=gate5_4_p2_depth \
  "$ROOT/scripts/run_module_csynth.sh" \
  synth_predictor_foundation_64_top \
  synth_predictor_foundation_128_top \
  synth_predictor_foundation_256_top \
  synth_predictor_foundation_512_top \
  synth_predictor_foundation_full_reset_top

BOOM_HLS_GATE=gate5_4_p2_lutram \
BOOM_HLS_CFLAGS_EXTRA=-DBOOM_PREDICTOR_STORAGE_LUTRAM \
  "$ROOT/scripts/run_module_csynth.sh" synth_predictor_foundation_top

BOOM_HLS_GATE=gate5_4_p2_bram \
BOOM_HLS_CFLAGS_EXTRA=-DBOOM_PREDICTOR_STORAGE_BRAM \
  "$ROOT/scripts/run_module_csynth.sh" synth_predictor_foundation_top

printf '%s\n' GATE5_4_P2_PREDICTOR_CSYNTH_COMPLETE
