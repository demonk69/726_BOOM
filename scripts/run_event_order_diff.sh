#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"}
REF_SOURCE=${1:-boom}
DUT_SOURCE=${2:-hls_csim}

python3 "$ROOT/scripts/run_trace_diff.py" \
  --kind event \
  --ref-source "$REF_SOURCE" \
  --dut-source "$DUT_SOURCE" \
  --root "$ROOT"
