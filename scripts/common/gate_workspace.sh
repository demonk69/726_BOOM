#!/usr/bin/env bash

_gate_workspace_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
# shellcheck source=build_env.sh
source "$_gate_workspace_dir/build_env.sh"

gate_begin() {
  GATE_WORKSPACE_GATE=${1:?gate is required}
  GATE_WORKSPACE_STAGE=${2:?stage is required}
  export GATE_WORKSPACE_GATE GATE_WORKSPACE_STAGE
}

gate_build_dir() {
  local top=${1:?top is required}
  [[ -n "${GATE_WORKSPACE_GATE:-}" && -n "${GATE_WORKSPACE_STAGE:-}" ]] || {
    printf 'gate_begin must be called first\n' >&2
    return 2
  }
  create_build_dir "$GATE_WORKSPACE_GATE" "$GATE_WORKSPACE_STAGE" "$top"
}

gate_preserve_failure() {
  keep_build_dir
}

gate_cleanup_success() {
  cleanup_build_dir "${1:-${BOOM_BUILD_DIR:-}}"
}
