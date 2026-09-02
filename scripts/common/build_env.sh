#!/usr/bin/env bash

: "${BOOM_BUILD_ROOT:=/tmp/boom_hls}"
: "${BOOM_KEEP_BUILD:=0}"
export BOOM_BUILD_ROOT BOOM_KEEP_BUILD

_boom_safe_component() {
  case "$1" in
    ''|.|..|*/*|*[^A-Za-z0-9_.-]*) return 1 ;;
  esac
}

create_build_dir() {
  local gate=${1:?gate is required}
  local stage=${2:?stage is required}
  local top=${3:?top is required}
  _boom_safe_component "$gate" && _boom_safe_component "$stage" && _boom_safe_component "$top" || {
    printf 'unsafe build path component\n' >&2
    return 2
  }
  mkdir -p -- "$BOOM_BUILD_ROOT/$gate/$stage"
  BOOM_BUILD_DIR=$(mktemp -d "$BOOM_BUILD_ROOT/$gate/$stage/${top}.XXXXXX")
  export BOOM_BUILD_DIR
  printf '%s\n' "$BOOM_BUILD_DIR"
}

keep_build_dir() {
  BOOM_KEEP_BUILD=1
  export BOOM_KEEP_BUILD
  [[ -z "${BOOM_BUILD_DIR:-}" ]] || printf 'Preserving build workspace: %s\n' "$BOOM_BUILD_DIR" >&2
}

cleanup_build_dir() {
  local path=${1:-${BOOM_BUILD_DIR:-}}
  [[ -n "$path" ]] || return 0
  if [[ "$BOOM_KEEP_BUILD" == 1 ]]; then
    printf 'Preserving build workspace: %s\n' "$path" >&2
    return 0
  fi
  case "$path" in
    "$BOOM_BUILD_ROOT"/*) ;;
    *) printf 'refusing cleanup outside BOOM_BUILD_ROOT: %s\n' "$path" >&2; return 2 ;;
  esac
  [[ ! -L "$path" ]] || { printf 'refusing symlink cleanup: %s\n' "$path" >&2; return 2; }
  [[ ! -e "$path" ]] || rm -rf --one-file-system -- "$path"
}
