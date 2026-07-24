#!/usr/bin/env bash
set -euo pipefail

HLS_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SIM="${BOOM_TRACE_SIM:-${HLS_ROOT}/reference/verilator_trace/simulator-chipyard-SmallBoomConfig-trace}"
REPORT_DIR="${HLS_ROOT}/reports/equivalence/gate2"
ELF=""
LOADMEM=""
OUT=""
MAX_CYCLES="100000"
LOADMEM_ADDR="80000000"
TOHOST_ADDR="80000080"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --elf) ELF="$2"; shift 2 ;;
    --loadmem) LOADMEM="$2"; shift 2 ;;
    --loadmem-addr) LOADMEM_ADDR="$2"; shift 2 ;;
    --tohost-addr) TOHOST_ADDR="$2"; shift 2 ;;
    --output) OUT="$2"; shift 2 ;;
    --max-cycles) MAX_CYCLES="$2"; shift 2 ;;
    *) echo "ERROR: unknown argument $1" >&2; exit 2 ;;
  esac
done

mkdir -p "${REPORT_DIR}"
LOG="${REPORT_DIR}/run_boom_trace.log"
{
  echo "Run started: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "Simulator: ${SIM}"
  echo "ELF: ${ELF}"
  echo "Loadmem: ${LOADMEM}"
  echo "Loadmem addr: ${LOADMEM_ADDR}"
  echo "Tohost addr: ${TOHOST_ADDR}"
  echo "Output: ${OUT}"
  echo "Max cycles: ${MAX_CYCLES}"
  if [[ -z "${ELF}" && -z "${LOADMEM}" || -z "${OUT}" ]]; then
    echo "ERROR: --elf/--loadmem and --output are required"
    exit 2
  fi
  if [[ ! -x "${SIM}" ]]; then
    echo "ERROR: trace simulator not executable: ${SIM}"
    exit 3
  fi
  mkdir -p "$(dirname "${OUT}")"
  TMP_OUT="${OUT}.tmp.$$"
  rm -f "${TMP_OUT}"
  if [[ -n "${LOADMEM}" ]]; then
    if [[ ! -f "${LOADMEM}" ]]; then
      echo "ERROR: loadmem image not found: ${LOADMEM}"
      exit 3
    fi
    "${SIM}" "+boom_equiv_trace=${TMP_OUT}" "+loadmem_addr=${LOADMEM_ADDR}" "+loadmem=${LOADMEM}" "+tohost_addr=${TOHOST_ADDR}" "+max-cycles=${MAX_CYCLES}" "+standalone_wake_hart0=1" "+standalone_disable_assert_stops=1"
  else
    if [[ ! -f "${ELF}" ]]; then
      echo "ERROR: ELF not found: ${ELF}"
      exit 3
    fi
    "${SIM}" +permissive "+boom_equiv_trace=${TMP_OUT}" +permissive-off "+max-cycles=${MAX_CYCLES}" "${ELF}"
  fi
  mv "${TMP_OUT}" "${OUT}"
} > "${LOG}" 2>&1
