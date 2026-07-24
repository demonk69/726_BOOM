#!/usr/bin/env bash
set -euo pipefail

HLS_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ROOT="$(cd "${HLS_ROOT}/.." && pwd)"
REPORT_DIR="${HLS_ROOT}/reports/equivalence/gate2_5"
ENV_OUT="${REPORT_DIR}/chipyard_environment.txt"
MATRIX_OUT="${REPORT_DIR}/chipyard_dependency_matrix.csv"
BUILD_LOG="${REPORT_DIR}/chipyard_build.log"

mkdir -p "${REPORT_DIR}"

detect_cmd() {
  command -v "$1" 2>/dev/null || true
}

version_of() {
  local cmd="$1"
  if [[ -n "${cmd}" && -x "${cmd}" ]]; then
    "${cmd}" --version 2>&1 | sed -n '1p' || true
  fi
}

find_under_roots() {
  local pattern="$1"
  local result=""
  for root in "${ROOT}" "${HOME:-}"; do
    if [[ -n "${root}" && -d "${root}" ]]; then
      result="$(find "${root}" -maxdepth 6 -name "${pattern}" -print -quit 2>/dev/null || true)"
      if [[ -n "${result}" ]]; then
        printf '%s\n' "${result}"
        return 0
      fi
    fi
  done
  return 0
}

status_for_path() {
  if [[ -n "$1" && -e "$1" ]]; then printf 'FOUND'; else printf 'MISSING'; fi
}

java_path="$(detect_cmd java)"
sbt_path="$(detect_cmd sbt)"
mill_path="$(detect_cmd mill)"
verilator_path="$(detect_cmd verilator)"
spike_path="$(detect_cmd spike)"
conda_path="$(detect_cmd conda)"
gcc_path="$(detect_cmd riscv64-unknown-elf-gcc)"
objdump_path="$(detect_cmd riscv64-unknown-elf-objdump)"
readelf_path="$(detect_cmd riscv64-unknown-elf-readelf)"
nm_path="$(detect_cmd riscv64-unknown-elf-nm)"
libfesvr_path="${RISCV:-}/lib/libfesvr.a"
if [[ ! -f "${libfesvr_path}" ]]; then libfesvr_path="$(find_under_roots libfesvr.a)"; fi
libdramsim_path="${DRAMSIM_HOME:-}/libdramsim.a"
if [[ ! -f "${libdramsim_path}" ]]; then libdramsim_path="$(find_under_roots libdramsim.a)"; fi
chipyard_root="${CHIPYARD_ROOT:-}"
if [[ -z "${chipyard_root}" || ! -d "${chipyard_root}" ]]; then chipyard_root="$(find_under_roots chipyard)"; fi
env_sh="$(find_under_roots env.sh)"
build_setup="$(find_under_roots build-setup.sh)"
setup_paths="$(find_under_roots setup-paths.sh)"
generated_src="${ROOT}/chipyard.TestHarness.SmallBoomConfig"
sim_makefile="${generated_src}/chipyard.TestHarness.SmallBoomConfig/VTestHarness.mk"

{
  printf 'Gate 2.5 Chipyard environment diagnostic\n'
  date -u +%Y-%m-%dT%H:%M:%SZ
  printf 'ROOT=%s\n' "${ROOT}"
  printf 'HLS_ROOT=%s\n' "${HLS_ROOT}"
  printf 'CHIPYARD_ROOT=%s\n' "${CHIPYARD_ROOT:-}"
  printf 'RISCV=%s\n' "${RISCV:-}"
  printf 'JAVA_HOME=%s\n' "${JAVA_HOME:-}"
  printf 'DRAMSIM_HOME=%s\n' "${DRAMSIM_HOME:-}"
  printf 'PATH=%s\n' "${PATH}"
  printf 'java=%s %s\n' "${java_path}" "$(version_of "${java_path}")"
  printf 'sbt=%s %s\n' "${sbt_path}" "$(version_of "${sbt_path}")"
  printf 'mill=%s %s\n' "${mill_path}" "$(version_of "${mill_path}")"
  printf 'verilator=%s %s\n' "${verilator_path}" "$(version_of "${verilator_path}")"
  printf 'spike=%s %s\n' "${spike_path}" "$(version_of "${spike_path}")"
  printf 'conda=%s %s\n' "${conda_path}" "$(version_of "${conda_path}")"
} > "${ENV_OUT}"

{
  printf 'dependency,required_version,detected_version,detected_path,status,required_by,recovery_command\n'
  printf 'CHIPYARD_ROOT,matching generated SmallBoomConfig source tree,,%s,%s,official simulator rebuild,restore original Chipyard checkout and set CHIPYARD_ROOT\n' "${chipyard_root}" "$(status_for_path "${chipyard_root}")"
  printf 'RISCV toolchain,riscv64-unknown-elf-*,,%s,%s,reference ELF build,install or expose matching riscv64-unknown-elf toolchain\n' "${gcc_path}" "$(status_for_path "${gcc_path}")"
  printf 'riscv64-unknown-elf-objdump,matching toolchain,,%s,%s,ELF disassembly,install or expose matching riscv64-unknown-elf-objdump\n' "${objdump_path}" "$(status_for_path "${objdump_path}")"
  printf 'riscv64-unknown-elf-readelf,matching toolchain,,%s,%s,ELF manifest,install or expose matching riscv64-unknown-elf-readelf\n' "${readelf_path}" "$(status_for_path "${readelf_path}")"
  printf 'riscv64-unknown-elf-nm,matching toolchain,,%s,%s,tohost/fromhost symbol check,install or expose matching riscv64-unknown-elf-nm\n' "${nm_path}" "$(status_for_path "${nm_path}")"
  printf 'libfesvr,matching Chipyard riscv-tools,,%s,%s,HTIF/FESVR link,set RISCV to prefix containing lib/libfesvr.a\n' "${libfesvr_path}" "$(status_for_path "${libfesvr_path}")"
  printf 'DRAMSim2 libdramsim,matching Chipyard DRAMSim2,,%s,%s,SimDRAM link,set DRAMSIM_HOME to directory containing libdramsim.a\n' "${libdramsim_path}" "$(status_for_path "${libdramsim_path}")"
  printf 'Java,project-compatible,,%s,%s,Chipyard generators/build,%s\n' "${java_path}" "$(status_for_path "${java_path}")" 'install Java required by the original Chipyard checkout'
  printf 'sbt,project-compatible,,%s,%s,Chipyard generators/build,%s\n' "${sbt_path}" "$(status_for_path "${sbt_path}")" 'install sbt only if required by original checkout'
  printf 'mill,project-compatible,,%s,%s,Chipyard generators/build,%s\n' "${mill_path}" "$(status_for_path "${mill_path}")" 'install mill only if required by original checkout'
  printf 'Verilator,compatible with generated model,,%s,%s,Verilator model build,install compatible Verilator or set VERILATOR_ROOT\n' "${verilator_path}" "$(status_for_path "${verilator_path}")"
  printf 'Spike,optional matching ISA simulator,,%s,%s,cross-check,install only if required for reference comparison\n' "${spike_path}" "$(status_for_path "${spike_path}")"
  printf 'conda,optional project environment,,%s,%s,environment restoration,activate original Chipyard conda environment if available\n' "${conda_path}" "$(status_for_path "${conda_path}")"
  printf 'env.sh,project setup script,,%s,%s,environment restoration,source original env.sh if available\n' "${env_sh}" "$(status_for_path "${env_sh}")"
  printf 'build-setup.sh,project setup script,,%s,%s,environment restoration,run only from original Chipyard checkout if needed\n' "${build_setup}" "$(status_for_path "${build_setup}")"
  printf 'setup-paths.sh,project setup script,,%s,%s,environment restoration,source original setup-paths.sh if available\n' "${setup_paths}" "$(status_for_path "${setup_paths}")"
  printf 'generated-src,SmallBoomConfig generated files,,%s,%s,standalone/generated model,%s\n' "${generated_src}" "$(status_for_path "${generated_src}")" 'already present in this workspace'
  printf 'simulator Makefile,VTestHarness.mk,,%s,%s,official simulator rebuild,%s\n' "${sim_makefile}" "$(status_for_path "${sim_makefile}")" 'already present but hardcodes original paths'
} > "${MATRIX_OUT}"

{
  printf 'Diagnostic completed. Run scripts/build_official_chipyard_sim.sh for the official simulator build attempt.\n'
} > "${BUILD_LOG}"

printf '%s\n' "${ENV_OUT}"
printf '%s\n' "${MATRIX_OUT}"
printf '%s\n' "${BUILD_LOG}"
