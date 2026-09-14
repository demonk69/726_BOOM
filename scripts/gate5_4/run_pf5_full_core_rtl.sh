#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
PF5_TRAINING_EXPECTED=1 \
GATE5_4_PF4_RTL_FORCE_CSYNTH=${GATE5_4_PF5_RTL_FORCE_CSYNTH:-1} \
"$ROOT/scripts/gate5_4/run_pf4_full_core_rtl.sh"
