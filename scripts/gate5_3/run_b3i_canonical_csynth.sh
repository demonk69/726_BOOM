#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
REPORT="$ROOT/reports/gate5_3_fetch_buffer/b3i"
BUILD="$ROOT/build/gate5_3_fetch_buffer/b3i/canonical_csynth"
VITIS_HLS=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
mkdir -p "$REPORT/logs/canonical_csynth" "$BUILD"

HLS_BOOM_ROOT="$ROOT" GATE5_3_B3I_PACKET_PROJECT="$BUILD/fetch_packet_hls" \
    "$VITIS_HLS" -f "$ROOT/scripts/gate5_3/b3i_packet_csynth.tcl" \
    >"$REPORT/logs/canonical_csynth/fetch_packet.log" 2>&1

GATE5_3_CANONICAL_REPORT="$REPORT" \
GATE5_3_CANONICAL_TAG=gate5_3_fetch_buffer_b3i \
GATE5_3_FETCH_BUFFER_CSYNTH_BUILD="$BUILD/fetch_buffer" \
GATE5_3_FETCH_BUFFER_CSYNTH_REPORT="$REPORT/logs/canonical_csynth/fetch_buffer" \
VITIS_HLS="$VITIS_HLS" "$ROOT/scripts/gate5_3/run_b2_canonical_csynth.sh"

printf '%s\n' 'GATE5_3_B3I_CANONICAL_CSYNTH_COMPLETE standalone=1 canonical=11'
