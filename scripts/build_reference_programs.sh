#!/usr/bin/env bash
set -euo pipefail

HLS_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PROG_DIR="${HLS_ROOT}/tb/programs/boom_reference"
BUILD_DIR="${PROG_DIR}/build"
REPORT_DIR="${HLS_ROOT}/reports/equivalence/gate2"
LOG="${REPORT_DIR}/build_reference_programs.log"
CC="${RISCV_GCC:-riscv64-unknown-elf-gcc}"
OBJDUMP="${RISCV_OBJDUMP:-riscv64-unknown-elf-objdump}"

copy_manual_hex() {
  echo "Using hand-encoded RV64 loadmem images because the RISC-V ELF toolchain is unavailable."
  for src in "${PROG_DIR}"/*.hex; do
    name="$(basename "${src}" .hex)"
    cp "${src}" "${BUILD_DIR}/${name}.hex"
    {
      echo "${name}: hand-encoded RV64 loadmem image"
      sed 's/^# //g' "${src}"
    } > "${BUILD_DIR}/${name}.dump"
    echo "built ${name}.hex"
  done
}

mkdir -p "${BUILD_DIR}" "${REPORT_DIR}"
{
  echo "Reference program build started: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "Compiler: ${CC}"
  if ! command -v "${CC}" >/dev/null 2>&1; then
    echo "WARNING: RISC-V compiler not found: ${CC}"
    copy_manual_hex
    exit 0
  fi
  if ! command -v "${OBJDUMP}" >/dev/null 2>&1; then
    echo "WARNING: RISC-V objdump not found: ${OBJDUMP}"
    copy_manual_hex
    exit 0
  fi
  for src in "${PROG_DIR}"/*.S; do
    name="$(basename "${src}" .S)"
    "${CC}" -march=rv64imac -mabi=lp64 -nostdlib -nostartfiles -I "${PROG_DIR}" -T "${PROG_DIR}/linker.ld" "${src}" -o "${BUILD_DIR}/${name}.elf"
    "${OBJDUMP}" -d "${BUILD_DIR}/${name}.elf" > "${BUILD_DIR}/${name}.dump"
    echo "built ${name}"
  done
  copy_manual_hex
} > "${LOG}" 2>&1
