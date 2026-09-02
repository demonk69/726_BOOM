#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BOOM_HLS_GATE=gate5_4_p1 "$ROOT/scripts/run_module_csynth.sh" \
  synth_predecode_top synth_predecode_packet_top synth_frontend_top
printf '%s\n' 'GATE5_4_P1_PREDECODE_CSYNTH_COMPLETE'
