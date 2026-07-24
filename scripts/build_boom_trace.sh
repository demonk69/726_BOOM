#!/usr/bin/env bash
set -euo pipefail

HLS_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ROOT="$(cd "${HLS_ROOT}/.." && pwd)"
BOOM_GEN="${BOOM_GEN:-${ROOT}/chipyard.TestHarness.SmallBoomConfig}"
OBJ_DIR="${BOOM_GEN}/chipyard.TestHarness.SmallBoomConfig"
REPORT_DIR="${HLS_ROOT}/reports/equivalence/gate2"
LOG="${REPORT_DIR}/build_boom_trace.log"
OUT="${HLS_ROOT}/reference/verilator_trace/simulator-chipyard-SmallBoomConfig-trace"

mkdir -p "${REPORT_DIR}" "$(dirname "${OUT}")"
{
  echo "Build started: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "HLS root: ${HLS_ROOT}"
  echo "BOOM generated root: ${BOOM_GEN}"
  echo "Object dir: ${OBJ_DIR}"
  echo "Output simulator: ${OUT}"
  if [[ ! -d "${BOOM_GEN}" || ! -d "${OBJ_DIR}" ]]; then
    echo "ERROR: BOOM generated/object directory missing"
    exit 2
  fi
  if [[ ! -f "${OBJ_DIR}/VTestHarness.mk" ]]; then
    echo "ERROR: VTestHarness.mk missing"
    exit 2
  fi
  if ! command -v verilator >/dev/null 2>&1; then
    echo "ERROR: verilator not found"
    exit 2
  fi
  verilator --version
  if [[ ! -f "${OBJ_DIR}/VTestHarness__ALL.a" ]]; then
    echo "ERROR: VTestHarness__ALL.a missing"
    exit 2
  fi
  if ! command -v g++ >/dev/null 2>&1; then
    echo "ERROR: g++ not found"
    exit 2
  fi
  RUNNER_CPP="${HLS_ROOT}/reference/verilator_trace/standalone_boom_trace.cpp"
  if [[ ! -f "${RUNNER_CPP}" ]]; then
    echo "ERROR: standalone runner missing: ${RUNNER_CPP}"
    exit 2
  fi
  VERILATOR_INC="${VERILATOR_INCLUDE:-}"
  if [[ -z "${VERILATOR_INC}" ]]; then
    for candidate in \
      "$(verilator --getenv VERILATOR_ROOT 2>/dev/null || true)/include" \
      "/usr/local/share/verilator/include" \
      "/usr/share/verilator/include" \
      "/tmp/opencode/verilator-4.038-pkg/usr/share/verilator/include"; do
      if [[ -f "${candidate}/verilated_heavy.h" ]]; then
        VERILATOR_INC="${candidate}"
        break
      fi
    done
  fi
  if [[ -z "${VERILATOR_INC}" || ! -f "${VERILATOR_INC}/verilated_heavy.h" ]]; then
    echo "ERROR: verilated_heavy.h not found. Set VERILATOR_INCLUDE to a compatible Verilator include directory."
    exit 2
  fi
  echo "Verilator include: ${VERILATOR_INC}"
  echo "Building standalone trace runner against existing generated Verilator archive."
  g++ -O2 -std=c++11 \
    -I"${OBJ_DIR}" \
    -I"${BOOM_GEN}" \
    -I"${VERILATOR_INC}" \
    -I"${VERILATOR_INC}/vltstd" \
    "${RUNNER_CPP}" \
    "${OBJ_DIR}/VTestHarness__ALL.a" \
    "${OBJ_DIR}/verilated.o" \
    "${OBJ_DIR}/verilated_dpi.o" \
    "${OBJ_DIR}/verilated_vpi.o" \
    -pthread \
    -o "${OUT}"
  chmod +x "${OUT}"
  echo "Built ${OUT}"
} > "${LOG}" 2>&1
