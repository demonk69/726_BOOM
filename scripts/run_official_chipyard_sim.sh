#!/usr/bin/env bash
set -euo pipefail

HLS_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ROOT="$(cd "${HLS_ROOT}/.." && pwd)"
REPORT_DIR="${HLS_ROOT}/reports/equivalence/gate2_5"
LOG="${REPORT_DIR}/official_run.log"
SIM="${OFFICIAL_BOOM_SIM:-/root/chipyard/sims/verilator/simulator-chipyard-SmallBoomConfig}"
ELF=""
TRACE=""
MAX_CYCLES="100000"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --sim) SIM="$2"; shift 2 ;;
    --elf) ELF="$2"; shift 2 ;;
    --trace) TRACE="$2"; shift 2 ;;
    --max-cycles) MAX_CYCLES="$2"; shift 2 ;;
    *) echo "ERROR: unknown argument $1" >&2; exit 2 ;;
  esac
done

mkdir -p "${REPORT_DIR}"
{
  printf 'Official Chipyard simulator run attempt\n'
  date -u +%Y-%m-%dT%H:%M:%SZ
  printf 'SIM=%s\n' "${SIM}"
  printf 'ELF=%s\n' "${ELF}"
  printf 'TRACE=%s\n' "${TRACE}"
  printf 'MAX_CYCLES=%s\n' "${MAX_CYCLES}"
  if [[ ! -x "${SIM}" ]]; then
    printf 'ERROR: official simulator is not executable or not present: %s\n' "${SIM}"
    exit 3
  fi
  if [[ -z "${ELF}" || ! -f "${ELF}" ]]; then
    printf 'ERROR: ELF is required and must exist\n'
    exit 3
  fi
  args=("+max-cycles=${MAX_CYCLES}")
  if [[ -n "${TRACE}" ]]; then args+=("+boom_equiv_trace=${TRACE}"); fi
  "${SIM}" "${args[@]}" "${ELF}"
} > "${LOG}" 2>&1
