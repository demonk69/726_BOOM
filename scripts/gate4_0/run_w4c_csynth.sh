#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BOOM_W4_STAGE=w4c \
BOOM_W4_GATE=gate4_0_w4c \
BOOM_W4_VARIANT=W4C_MULTI_WAKEUP \
  "$ROOT/scripts/gate4_0/run_w4b_csynth.sh"
