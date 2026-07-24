#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"}

python3 "$ROOT/scripts/build_dynamic_uop_map.py" \
  --root "$ROOT" \
  --out-dir "$ROOT/reports/equivalence/provisional_gate3_1"
