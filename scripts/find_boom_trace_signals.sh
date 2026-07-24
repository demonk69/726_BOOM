#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BOOM_GEN="${BOOM_GEN:-${ROOT}/../chipyard.TestHarness.SmallBoomConfig}"
OUT="${ROOT}/docs/verilator_trace_signal_inventory.generated.txt"

mkdir -p "$(dirname "${OUT}")"
{
  printf 'BOOM generated root: %s\n' "${BOOM_GEN}"
  printf 'Generated at: %s\n\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  if [[ ! -d "${BOOM_GEN}" ]]; then
    printf 'ERROR: generated root not found\n'
    exit 2
  fi
  if command -v rg >/dev/null 2>&1; then
    rg -n 'enableCommitLogPrintf|enableBranchPrintf|enableMemtracePrintf|"trace":false|rob_io_commit_arch_valids_0|rob_io_enq_valids_0|rob_io_flush_valid|brinfos_0_valid|ll_wbarb_io_out_valid|csr_io_pc|VM_TRACE' "${BOOM_GEN}" || true
  else
    grep -RInE 'enableCommitLogPrintf|enableBranchPrintf|enableMemtracePrintf|"trace":false|rob_io_commit_arch_valids_0|rob_io_enq_valids_0|rob_io_flush_valid|brinfos_0_valid|ll_wbarb_io_out_valid|csr_io_pc|VM_TRACE' "${BOOM_GEN}" || true
  fi
} > "${OUT}"

printf '%s\n' "${OUT}"
