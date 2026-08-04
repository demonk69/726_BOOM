#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BOOM_W4_STAGE=w4d_oracle \
BOOM_W4_GATE=gate4_0_w4d_oracle \
BOOM_W4_VARIANT=W4D_RTL_ORACLE_PREP \
BOOM_W4_TOPS=synth_w4d_oracle_top \
  "$ROOT/scripts/gate4_0/run_w4b_csynth.sh"
