#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
ROOT=$(cd -- "$SCRIPT_DIR/../.." && pwd -P)
[[ -n "$ROOT" && "$ROOT" != / && -d "$ROOT/.git" ]] || {
  printf 'unsafe repository root: %q\n' "$ROOT" >&2
  exit 2
}

warning_gb=${WORKSPACE_SIZE_WARNING_GB:-20}
severe_gb=${WORKSPACE_SIZE_SEVERE_GB:-50}
build_root=${BOOM_BUILD_ROOT:-/tmp/boom_hls}
total_bytes=$(du -sxB1 -- "$ROOT" | cut -f1)
build_bytes=$(du -sxB1 -- "$build_root" 2>/dev/null | cut -f1 || printf '0\n')
reports_bytes=$(du -sxB1 -- "$ROOT/reports" 2>/dev/null | cut -f1 || printf '0\n')

printf 'REPOSITORY_TOTAL_BYTES=%s REPOSITORY_TOTAL=%s\n' "$total_bytes" "$(numfmt --to=iec --suffix=B "$total_bytes")"
printf 'BUILD_TOTAL_BYTES=%s BUILD_TOTAL=%s\n' "$build_bytes" "$(numfmt --to=iec --suffix=B "$build_bytes")"
printf 'REPORTS_TOTAL_BYTES=%s REPORTS_TOTAL=%s\n' "$reports_bytes" "$(numfmt --to=iec --suffix=B "$reports_bytes")"
printf 'LARGEST_20_DIRECTORIES\n'
du -xB1 --max-depth=4 "$ROOT" | sort -nr | sed -n '1,20p' | while IFS=$'\t' read -r bytes path; do
  printf '%s\t%s\t%s\n' "$bytes" "$(numfmt --to=iec --suffix=B "$bytes")" "$path"
done
printf 'LARGEST_20_FILES\n'
find "$ROOT" -xdev -type f -printf '%s\t%p\n' | sort -nr | sed -n '1,20p' | while IFS=$'\t' read -r bytes path; do
  printf '%s\t%s\t%s\n' "$bytes" "$(numfmt --to=iec --suffix=B "$bytes")" "$path"
done

if (( total_bytes >= severe_gb * 1024 * 1024 * 1024 )); then
  printf 'WORKSPACE_SIZE_WARNING=SEVERE threshold_gb=%s\n' "$severe_gb" >&2
elif (( total_bytes >= warning_gb * 1024 * 1024 * 1024 )); then
  printf 'WORKSPACE_SIZE_WARNING=WARNING threshold_gb=%s\n' "$warning_gb" >&2
else
  printf 'WORKSPACE_SIZE_WARNING=NONE warning_threshold_gb=%s severe_threshold_gb=%s\n' "$warning_gb" "$severe_gb"
fi
if (( build_bytes >= warning_gb * 1024 * 1024 * 1024 )); then
  printf 'Suggested cleanup: bash scripts/maintenance/cleanup_workspace.sh --build-root %q\n' "$build_root"
fi
