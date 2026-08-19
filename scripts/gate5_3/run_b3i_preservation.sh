#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
REPORT="$ROOT/reports/gate5_3_fetch_buffer/b3i"
BUILD_ROOT="$ROOT/build/gate5_3_fetch_buffer/b3i/rtl"

mapfile -t CPP_INPUTS < <(printf '%s\n' "$ROOT"/src/*.cpp | sort)
mapfile -t HEADER_INPUTS < <(printf '%s\n' "$ROOT"/include/*.hpp | sort)
INPUTS=()
for source in "${CPP_INPUTS[@]}"; do
    case "$source" in
        */src/boom_all.cpp|*/src/boom_core_merged.cpp) continue ;;
    esac
    INPUTS+=("$source")
done
INPUTS+=("${HEADER_INPUTS[@]}")

SOURCE_HASH=$(python3 - "$ROOT" "${INPUTS[@]}" <<'PY'
import hashlib
import sys
from pathlib import Path
root = Path(sys.argv[1])
digest = hashlib.sha256()
for item in sys.argv[2:]:
    path = Path(item)
    digest.update(str(path.relative_to(root)).encode("utf-8"))
    digest.update(b"\0")
    digest.update(hashlib.sha256(path.read_bytes()).digest())
print(digest.hexdigest())
PY
)
RTL="$BUILD_ROOT/${SOURCE_HASH:0:16}/boom_core_top_hls/solution_b3i/syn/verilog"

[[ -s "$RTL/boom_core_top.v" && -s "$RTL/.b3i_source_hash" ]] || {
    printf 'Current-source B3I generated RTL is unavailable: %s\n' "$RTL" >&2
    exit 3
}
[[ "$(<"$RTL/.b3i_source_hash")" == "$SOURCE_HASH" ]] || {
    printf '%s\n' 'Current-source B3I RTL freshness stamp mismatch' >&2
    exit 3
}

GATE5_3_REGRESSION_REPORT="$REPORT/regression" \
GATE5_3_REGRESSION_BUILD="$ROOT/build/gate5_3_fetch_buffer/b3i/regression" \
GATE5_3_REGRESSION_RTL="$RTL" \
GATE5_3_REGRESSION_FRESHNESS_MANIFEST="$REPORT/rtl/source_freshness_manifest.csv" \
GATE5_3_REGRESSION_SUMMARY="$REPORT/regression_after.md" \
GATE5_3_REGRESSION_FRONTEND_TAG=gate5_3_b3i_preservation_frontend \
GATE5_3_REGRESSION_SUMMARY_TITLE="Gate 5.3 B3I Preservation Regressions" \
GATE5_3_REGRESSION_SUMMARY_SCOPE="current B3I modular-source run" \
GATE5_3_REGRESSION_SUMMARY_GATE="B3I preservation" \
GATE5_3_REGRESSION_SUMMARY_FOLLOWUP="Canonical csynth status is recorded separately in b3i_ppa.csv." \
    "$ROOT/scripts/gate5_3/run_b2_phase_e_regressions.sh"

printf 'GATE5_3_B3I_PRESERVATION_PASS source_hash=%s\n' "$SOURCE_HASH"
