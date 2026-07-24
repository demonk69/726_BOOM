#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
HLS_ROOT="${ROOT}/hls_boom"
PATCH="${HLS_ROOT}/patches/boom_equivalence_trace.patch"
TARGET="${ROOT}/chipyard.TestHarness.SmallBoomConfig/emulator.cc"

if [[ ! -f "${TARGET}" ]]; then
  echo "ERROR: target not found: ${TARGET}" >&2
  exit 2
fi
if ! grep -q 'BOOM_EQUIV_TRACE' "${TARGET}"; then
  echo "Trace patch is not applied."
  exit 0
fi

patch --reverse -p0 -d "${ROOT}" < "${PATCH}"
