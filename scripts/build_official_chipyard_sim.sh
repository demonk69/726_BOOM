#!/usr/bin/env bash
set -euo pipefail

HLS_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ROOT="$(cd "${HLS_ROOT}/.." && pwd)"
REPORT_DIR="${HLS_ROOT}/reports/equivalence/gate2_5"
BUILD_LOG="${REPORT_DIR}/chipyard_build.log"
GEN_ROOT="${BOOM_GEN:-${ROOT}/chipyard.TestHarness.SmallBoomConfig}"
OBJ_DIR="${GEN_ROOT}/chipyard.TestHarness.SmallBoomConfig"
MK="${OBJ_DIR}/VTestHarness.mk"

mkdir -p "${REPORT_DIR}"
{
  printf 'Official Chipyard simulator build attempt\n'
  date -u +%Y-%m-%dT%H:%M:%SZ
  printf 'GEN_ROOT=%s\n' "${GEN_ROOT}"
  printf 'OBJ_DIR=%s\n' "${OBJ_DIR}"
  printf 'MAKEFILE=%s\n' "${MK}"
  if [[ ! -f "${MK}" ]]; then
    printf 'FIRST_BLOCKER: generated VTestHarness.mk is missing\n'
    exit 2
  fi
  target="$(sed -n 's/^default: //p' "${MK}" | sed -n '1p')"
  printf 'TARGET=%s\n' "${target}"
  if [[ -n "${target}" ]]; then
    target_dir="$(dirname "${target}")"
    if [[ ! -d "${target_dir}" ]]; then
      printf 'FIRST_BLOCKER: generated makefile target directory does not exist: %s\n' "${target_dir}"
    fi
  fi
  printf 'Running make from generated object directory.\n'
  make -C "${OBJ_DIR}" -f "${MK}"
} > "${BUILD_LOG}" 2>&1
