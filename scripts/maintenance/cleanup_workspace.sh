#!/usr/bin/env bash
set -euo pipefail

MODE=dry-run
OLDER_THAN_DAYS=7
BUILD_ROOT=${BOOM_BUILD_ROOT:-/tmp/boom_hls}
while (( $# )); do
  case "$1" in
    --execute) MODE=execute; shift ;;
    --older-than-days) OLDER_THAN_DAYS=${2:?missing day count}; shift 2 ;;
    --build-root) BUILD_ROOT=${2:?missing build root}; shift 2 ;;
    -h|--help)
      printf 'usage: %s [--execute] [--older-than-days N] [--build-root PATH]\n' "$0"
      exit 0
      ;;
    *) printf 'unknown argument: %s\n' "$1" >&2; exit 2 ;;
  esac
done
[[ "$OLDER_THAN_DAYS" =~ ^[0-9]+$ ]] || { printf 'days must be a nonnegative integer\n' >&2; exit 2; }

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
ROOT=$(cd -- "$SCRIPT_DIR/../.." && pwd -P)
[[ -n "$ROOT" && "$ROOT" != / ]] || { printf 'unsafe repository root: %q\n' "$ROOT" >&2; exit 2; }
[[ -d "$ROOT/.git" ]] || { printf 'not a Git worktree root: %s\n' "$ROOT" >&2; exit 2; }
[[ "$(git -C "$ROOT" rev-parse --show-toplevel)" == "$ROOT" ]] || {
  printf 'resolved path is not the Git root: %s\n' "$ROOT" >&2
  exit 2
}

[[ "$BUILD_ROOT" = /* && "$BUILD_ROOT" != / && "$BUILD_ROOT" != "$ROOT" ]] || {
  printf 'unsafe build root: %s\n' "$BUILD_ROOT" >&2
  exit 2
}
if [[ ! -d "$BUILD_ROOT" ]]; then
  printf 'MODE=%s\nBUILD_ROOT=%s\nCANDIDATE_BYTES=0\n' "$MODE" "$BUILD_ROOT"
  exit 0
fi
BUILD_ROOT=$(realpath -e -- "$BUILD_ROOT")
case "$BUILD_ROOT" in
  "$ROOT"/reports|"$ROOT"/reports/*|"$ROOT"/docs|"$ROOT"/docs/*|"$ROOT"/src|"$ROOT"/src/*|"$ROOT"/include|"$ROOT"/include/*|"$ROOT"/tb|"$ROOT"/rtl_tb) exit 2 ;;
esac

declare -a CANDIDATES=()
while IFS= read -r -d '' candidate; do
  CANDIDATES+=("$candidate")
done < <(find "$BUILD_ROOT" -mindepth 1 -maxdepth 3 -type d -mtime "+$OLDER_THAN_DAYS" -print0 2>/dev/null)

declare -a ACTIVE_PATHS=()
while IFS= read -r pid; do
  [[ -n "$pid" ]] || continue
  cwd=$(readlink -f "/proc/$pid/cwd" 2>/dev/null || true)
  [[ -n "$cwd" ]] && ACTIVE_PATHS+=("$cwd")
  while IFS= read -r fd_path; do
    [[ -n "$fd_path" ]] && ACTIVE_PATHS+=("$fd_path")
  done < <(find "/proc/$pid/fd" -maxdepth 1 -type l -printf '%l\n' 2>/dev/null || true)
done < <(pgrep -f 'vitis_hls|vivado|xsim|xelab|xvlog|csim|cosim' || true)

is_active() {
  local candidate=$1 active
  for active in "${ACTIVE_PATHS[@]:-}"; do
    case "$active" in
      "$candidate"|"$candidate"/*) return 0 ;;
    esac
  done
  return 1
}

validate_candidate() {
  local candidate=$1 resolved relative
  [[ -d "$candidate" && ! -L "$candidate" ]] || return 1
  resolved=$(realpath -e -- "$candidate")
  case "$resolved" in "$BUILD_ROOT"/*) ;; *) printf 'SKIP_OUTSIDE_BUILD_ROOT path=%s\n' "$resolved" >&2; return 1 ;; esac
  case "$resolved" in
    "$ROOT"|"$ROOT"/reports|"$ROOT"/reports/*|"$ROOT"/src|"$ROOT"/src/*|\
    "$ROOT"/include|"$ROOT"/include/*|"$ROOT"/docs|"$ROOT"/docs/*|\
    "$ROOT"/rtl_tb|"$ROOT"/rtl_tb/*|"$ROOT"/tb) return 1 ;;
  esac
  case "$resolved" in
    "$ROOT"/*)
      relative=${resolved#"$ROOT"/}
      if [[ -n "$(git -C "$ROOT" ls-files -- "$relative")" ]]; then
        printf 'KEEP_TRACKED path=%s\n' "$resolved"
        return 1
      fi
      if [[ -n "$(git -C "$ROOT" status --porcelain=v1 --untracked-files=no -- "$relative")" ]]; then
        printf 'KEEP_DIRTY path=%s\n' "$resolved"
        return 1
      fi
      ;;
  esac
  if [[ -n "$(find "$resolved" -xdev -type l -print -quit 2>/dev/null)" ]]; then
    printf 'KEEP_CONTAINS_SYMLINK path=%s\n' "$resolved"
    return 1
  fi
  if is_active "$resolved"; then
    printf 'SKIP_ACTIVE path=%s\n' "$resolved"
    return 1
  fi
}

total=0
deleted=0
printf 'MODE=%s\nROOT=%s\nBUILD_ROOT=%s\nOLDER_THAN_DAYS=%s\n' "$MODE" "$ROOT" "$BUILD_ROOT" "$OLDER_THAN_DAYS"
for candidate in "${CANDIDATES[@]:-}"; do
  validate_candidate "$candidate" || continue
  size=$(du -sxB1 -- "$candidate" | cut -f1)
  human=$(numfmt --to=iec --suffix=B "$size")
  printf 'SAFE_DELETE path=%s size_bytes=%s size=%s\n' "$candidate" "$size" "$human"
  total=$((total + size))
  if [[ "$MODE" == execute ]]; then
    rm -rf --one-file-system -- "$candidate"
    deleted=$((deleted + size))
  fi
done

printf 'CANDIDATE_BYTES=%s\nCANDIDATE_HUMAN=%s\n' "$total" "$(numfmt --to=iec --suffix=B "$total")"
if [[ "$MODE" == execute ]]; then
  printf 'FREED_BYTES=%s\nFREED_HUMAN=%s\n' "$deleted" "$(numfmt --to=iec --suffix=B "$deleted")"
else
  printf '%s\n' 'No files were deleted. Re-run with --execute after reviewing every SAFE_DELETE line.'
fi
