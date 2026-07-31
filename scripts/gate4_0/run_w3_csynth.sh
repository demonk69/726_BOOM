#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
VITIS_HLS_BIN=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
BUILD_ROOT="$ROOT/build/gate4_0/w3_csynth"
PROJECT_ROOT="$BUILD_ROOT/projects"
REPORT_BASE="$ROOT/reports/gate4_0/w3"
REPORT_ROOT="$REPORT_BASE/csynth"
EVIDENCE_TOOL="$ROOT/scripts/gate4_0/w3_csynth_evidence.py"
SOLUTION=solution_w3_csynth
PART=xczu7ev-ffvc1156-2-e
CLOCK=10
TOPS=(synth_issue_top synth_execute_top synth_rob_top synth_lsu_top boom_core_top)

if [[ ! -x "$VITIS_HLS_BIN" ]]; then
  printf 'Vitis HLS executable not found: %s\n' "$VITIS_HLS_BIN" >&2
  exit 1
fi
if ! "$VITIS_HLS_BIN" -version 2>&1 | tee "$REPORT_BASE/vitis_hls_version.txt" | grep -q 'v2021\.2'; then
  printf 'Vitis HLS 2021.2 is required\n' >&2
  exit 1
fi

mkdir -p "$PROJECT_ROOT" "$REPORT_ROOT"
python3 "$EVIDENCE_TOOL" hash-tree "$ROOT/reports/gate4_0/w2" "$BUILD_ROOT/w2_hashes.before"
"$ROOT/scripts/generate_merged.sh"
python3 "$EVIDENCE_TOOL" audit-source "$ROOT" "$REPORT_BASE/guardrail_audit.md"
python3 "$EVIDENCE_TOOL" hash-tree "$ROOT/src" "$BUILD_ROOT/source_hashes.before" --source-only --exclude boom_all.cpp
python3 "$EVIDENCE_TOOL" hash-tree "$ROOT/include" "$BUILD_ROOT/include_hashes.before" --source-only
cp "$BUILD_ROOT/source_hashes.before" "$REPORT_BASE/source_hashes.sha256"
cat "$BUILD_ROOT/include_hashes.before" >> "$REPORT_BASE/source_hashes.sha256"

run_top() {
  local top=$1
  local project="$PROJECT_ROOT/$top"
  local report="$REPORT_ROOT/$top"
  local synth_report="$project/$SOLUTION/syn/report"
  local database="$project/$SOLUTION/.autopilot/db"
  mkdir -p "$report/rtl" "$report/verbose"

  (
    cd "$PROJECT_ROOT"
    BOOM_HLS_GATE=gate4_0_w3 BOOM_HLS_TOP="$top" BOOM_HLS_PROJECT="$top" \
      BOOM_HLS_SOLUTION="$SOLUTION" BOOM_HLS_CFLAGS_EXTRA= FPGA_PART="$PART" CLOCK_PERIOD="$CLOCK" \
      /usr/bin/time -f 'runtime_seconds=%e\npeak_memory_kb=%M' -o "$report/csynth.time" \
      "$VITIS_HLS_BIN" -f "$ROOT/scripts/run_top_csynth.tcl" > "$report/csynth.log" 2>&1
  )

  python3 "$EVIDENCE_TOOL" audit-project "$project" "$SOLUTION" "$top"
  cp "$synth_report/${top}_csynth.rpt" "$report/${top}_csynth.rpt"
  cp "$synth_report/${top}_csynth.xml" "$report/${top}_csynth.xml"
  cp "$synth_report/csynth.rpt" "$report/csynth.rpt"
  cp "$synth_report/csynth.xml" "$report/csynth.xml"
  cp "$project/$SOLUTION/${SOLUTION}.log" "$report/${SOLUTION}.log"
  cp "$project/$SOLUTION/${SOLUTION}.directive" "$report/${SOLUTION}.directive"
  cp "$project/$SOLUTION/syn/verilog"/* "$report/rtl/"
  cp "$database"/*.verbose.rpt "$report/verbose/"
  cp "$database"/*.verbose.sched.rpt "$report/verbose/"
  cp "$database"/*.verbose.bind.rpt "$report/verbose/"
  printf 'W3_CSYNTH_PASS top=%s report=%s\n' "$top" "$report/${top}_csynth.rpt"
}

for top in "${TOPS[@]}"; do
  run_top "$top"
done

python3 "$EVIDENCE_TOOL" hash-tree "$ROOT/src" "$BUILD_ROOT/source_hashes.after" --source-only --exclude boom_all.cpp
python3 "$EVIDENCE_TOOL" hash-tree "$ROOT/include" "$BUILD_ROOT/include_hashes.after" --source-only
if ! cmp -s "$BUILD_ROOT/source_hashes.before" "$BUILD_ROOT/source_hashes.after" || \
   ! cmp -s "$BUILD_ROOT/include_hashes.before" "$BUILD_ROOT/include_hashes.after"; then
  printf 'Production source changed during W3 synthesis\n' >&2
  exit 1
fi
cat >> "$REPORT_BASE/guardrail_audit.md" <<'EOF'
## Post-Synthesis Audit

- All five canonical XML reports identify Vitis HLS 2021.2, `xczu7ev-ffvc1156-2-e`, a 10 ns target, and `PipelineType=no`.
- `boom_core_top` contains `CORE_CYCLE` without a `PipelineII` element.
- Every generated preprocessed source and solution directive was checked for active pipeline, dataflow, array-partition, and dependence overrides; none was found.
- Production source hashes were unchanged across the five synthesis runs.

EOF
python3 "$EVIDENCE_TOOL" summarize "$ROOT" "$REPORT_ROOT" "$REPORT_BASE/w2_resource_baseline.csv"
python3 "$EVIDENCE_TOOL" hash-tree "$ROOT/reports/gate4_0/w2" "$BUILD_ROOT/w2_hashes.after"
if ! cmp -s "$BUILD_ROOT/w2_hashes.before" "$BUILD_ROOT/w2_hashes.after"; then
  printf 'W2 evidence changed during W3 synthesis\n' >&2
  exit 1
fi
cp "$BUILD_ROOT/w2_hashes.after" "$REPORT_BASE/w2_integrity.sha256"
python3 "$EVIDENCE_TOOL" hash-tree "$REPORT_ROOT" "$REPORT_BASE/report_hashes.sha256"
printf '%s\n' 'Gate 4.0 W3 synthesis and guardrail audit complete.'
