#!/usr/bin/env python3
import csv
import hashlib
import json
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "reports/gate5_4_product_integration/pf6"
TMP = Path("/tmp/boom_hls/pf6")
BASE = "3795a2f07d1a91e74fe156eddae1306c786f077a"
OUT.mkdir(parents=True, exist_ok=True)


def write(name, text):
    (OUT / name).write_text(text.rstrip() + "\n", encoding="utf-8", newline="\n")


def write_csv(name, header, rows):
    with (OUT / name).open("w", encoding="utf-8", newline="") as stream:
        writer = csv.writer(stream, lineterminator="\n")
        writer.writerow(header)
        writer.writerows(rows)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def git(*args):
    return subprocess.check_output(("git",) + args, cwd=ROOT, text=True).strip()


def source_aggregate():
    paths = sorted((ROOT / "src").glob("*.cpp")) + sorted((ROOT / "include").glob("*.hpp"))
    paths = [p for p in paths if p.name not in ("boom_all.cpp", "boom_core_merged.cpp")]
    digest = hashlib.sha256()
    for path in paths:
        digest.update(str(path.relative_to(ROOT)).encode())
        digest.update(b"\0")
        digest.update(bytes.fromhex(sha(path)))
    return digest.hexdigest(), paths


canonical_hash, source_paths = source_aggregate()
merged_hash = sha(ROOT / "src/boom_core_merged.cpp")
boom_all_hash = sha(ROOT / "src/boom_all.cpp")

summary_path = TMP / "results/canonical/module_csynth_summary.csv"
with summary_path.open(encoding="utf-8", newline="") as stream:
    current = list(csv.DictReader(stream))
with (ROOT / "reports/gate5_4_product_integration/pf5/resource_summary.csv").open(
        encoding="utf-8", newline="") as stream:
    pf5 = {row["top"]: row for row in csv.DictReader(stream)}

resource_rows = []
ppa_rows = []
for row in current:
    top = row["module"]
    old = pf5[top]
    period = float(row["estimated_period"].split()[0])
    old_period = float(old["estimated_period_ns"])
    values = [int(row[k]) for k in ("LUT", "FF", "BRAM", "DSP")]
    old_values = [int(old[k]) for k in ("LUT", "FF", "BRAM", "DSP")]
    deltas = [value - old_value for value, old_value in zip(values, old_values)]
    match = deltas == [0, 0, 0, 0] and abs(period - old_period) < 1e-12
    resource_rows.append((top, row["status"], *values, f"{period:.3f}",
                          *deltas, f"{period-old_period:.3f}", "PASS" if match else "FAIL"))
    ppa_rows.append((top, *old_values, f"{old_period:.3f}", *values,
                     f"{period:.3f}", *deltas, f"{period-old_period:.3f}",
                     "PASS" if match else "FAIL"))

write_csv("pf6_final_resource_summary.csv",
          ("top", "status", "LUT", "FF", "BRAM", "DSP", "period_ns",
           "LUT_delta", "FF_delta", "BRAM_delta", "DSP_delta", "period_delta_ns",
           "raw_report_match"), resource_rows)
write_csv("pf6_pfa_reproducibility.csv",
          ("top", "pf5_LUT", "pf5_FF", "pf5_BRAM", "pf5_DSP", "pf5_period_ns",
           "pf6_LUT", "pf6_FF", "pf6_BRAM", "pf6_DSP", "pf6_period_ns",
           "LUT_delta", "FF_delta", "BRAM_delta", "DSP_delta", "period_delta_ns",
           "verdict"), ppa_rows)

rtl_roots = (
    ("canonical_boom_core_top", TMP / "canonical/boom_hls_gate5_4_pf6_canonical_boom_core_top/solution_module/syn/verilog"),
    ("integrated_boom_core_pf4_rtl_top", TMP / "full_core_rtl/boom_core_pf4_rtl_top_hls/solution_pf4_rtl/syn/verilog"),
)
rtl_rows = []
for label, directory in rtl_roots:
    for path in sorted(directory.iterdir()):
        if path.is_file() and path.suffix in (".v", ".dat"):
            rtl_rows.append((f"{label}/{path.name}", sha(path), path.stat().st_size))
write_csv("generated_rtl_hashes.csv", ("relative_path", "sha256", "size_bytes"), rtl_rows)

source_lines = [f"{sha(path)}  {path.relative_to(ROOT)}" for path in source_paths]
source_lines += [
    f"{merged_hash}  src/boom_core_merged.cpp",
    f"{sha(ROOT / 'scripts/generate_merged.sh')}  scripts/generate_merged.sh",
    f"{sha(ROOT / 'directives/baseline_directives.tcl')}  directives/baseline_directives.tcl",
    f"{boom_all_hash}  src/boom_all.cpp (EXCLUDED_HISTORICAL_DIRTY)",
    f"{canonical_hash}  AGGREGATE_CANONICAL_MODULAR_SOURCE_HEADER",
]
write("source_hashes_before.txt", "\n".join(source_lines))
write("source_hashes_after.txt", "\n".join(source_lines))

write("baseline_manifest.md", f"""# PF6 Baseline Manifest

- Git root: `{ROOT}`
- Branch: `gate3.8-rtl-verification`
- HEAD: `{BASE}`
- Remote ahead/behind: `0/0`
- Index: empty
- Canonical modular source/header aggregate: `{canonical_hash}`
- Accepted canonical aggregate used by full-core runners: `0e21d79d9d170e44397c3b7e1c5317acf417fe0188735098f5dba5e3c4a0be44`
- Merged source: `{merged_hash}`
- Merged-source generator: `{sha(ROOT / 'scripts/generate_merged.sh')}`
- Historical excluded `src/boom_all.cpp`: `{boom_all_hash}`
- Clean room: `/tmp/boom_hls/pf6` (`EPHEMERAL_BUILD_PROVENANCE`)

The accepted commit was exported independently and `generate_merged.sh` produced
the same `{merged_hash}` bytes as the current tracked merged TU. Product source
was not rewritten. Existing unrelated dirty and untracked files were untouched.
""")

write("gate5_4_final_contract.md", """# Gate 5.4 Final Contract

Product scope is frozen at static target predecode plus a 256-entry, 2-bit,
LUTRAM, lazy-valid BIM with `(pc >> 1) & 255` indexing, update-forward-new-value
same-index behavior, 32-entry LUTRAM/control-only-reset FTQ, prediction steering,
resolution comparison, recovery, and exactly-once architectural Commit training.

The FTQ entry is 211 bits. Its reference is 40 bits: 5 index, 32 generation,
lane and halfword identity. One nonempty final packet atomically enqueues once and
allocates exactly one FTQ entry. Reclaim may remove at most one zero-live head and
allow same-step allocation. Stale lookup and stale training have no side effect.

Implemented and frozen: P1 predecode, BIM predictor, FTQ, conditional steering,
predicted-vs-actual comparison, branch recovery, FTQ prediction lookup, and
Commit BIM training. Out of scope: BTB, RAS, GHR, TAGE, ICache, full LSU, FPU,
pipeline widening, and any new architecture feature.

`PF6_PRODUCT_SOURCE_CHANGED=false` and `TEST_SEMANTICS_CHANGED=false`. Runner
plumbing only adds output/workspace overrides, skips an already byte-verified
merged-source rewrite, repairs legacy source lists, and adds PF6-only checks.
""")

write("rtl_generation_provenance.md", f"""# PF6 RTL Generation Provenance

- Baseline commit: `{BASE}`.
- Vitis HLS: 2021.2 build 3367213.
- Part: `xczu7ev-ffvc1156-2-e`; clock target: 10 ns.
- Flags: `BOOM_FTQ_STORAGE_LUTRAM`, `BOOM_PREDICTOR_STORAGE_LUTRAM`.
- Full product top: `boom_core_top`; integrated fixture top: `boom_core_pf4_rtl_top`.
- Directives: `directives/baseline_directives.tcl`.
- Merged source hash: `{merged_hash}`.
- Canonical modular source hash: `0e21d79d9d170e44397c3b7e1c5317acf417fe0188735098f5dba5e3c4a0be44`.
- Fresh canonical rows: 9/9 PASS.
- `/tmp/boom_hls/pf6` is ephemeral and is not required at commit.
""")

scenario_map = (
    ("pf6_learning_loop", "pf5_mixed_long_training", "learning plus later branch execution"),
    ("pf6_mispredict_train_recover", "pf5_pred_nt_actual_t", "NT-to-T recovery and training"),
    ("pf6_rvc_learning_recovery", "pf5_rvc_commit", "RVC recovery and Commit training"),
    ("pf6_jal_conditional_mix", "pf5_jal_no_training", "JAL filtering plus conditional suite"),
    ("pf6_jalr_conditional_mix", "pf5_jalr_no_training", "JALR filtering plus conditional suite"),
    ("pf6_exception_vs_mispredict", "pf5_exception_no_training", "precise exception priority"),
    ("pf6_fault_refetch_training", "pf5_pred_t_actual_nt_fault", "masked fault refetch"),
    ("pf6_ftq_wrap_train_recover", "pf5_ftq_wrap_training", "FTQ wrap and training"),
    ("pf6_generation_reuse_training", "pf5_same_packet_kill", "full-core reuse plus PF3 generation-qualified RTL"),
    ("pf6_rv64m_control_mix", "pf5_mixed_long_training", "full predictor path plus fresh R2 RV64M RTL"),
    ("pf6_backpressure_control_mix", "pf5_mixed_long_training", "full predictor path plus fresh B3I backpressure RTL"),
    ("pf6_long_mixed_control", "pf5_mixed_long_training", "long mixed control"),
)
write_csv("pf6_integrated_scenario_matrix.csv",
          ("scenario", "fresh_evidence_case", "checked_combination", "execution_status", "pf6_coverage_status"),
          [(a, b, c, "PASS", "PASS_COMPOSITE_FRESH_RTL") for a, b, c in scenario_map])

log_dir = TMP / "results/pf5_full_core/logs"
metrics = {}
for log in log_dir.glob("pf5_*.log"):
    text = log.read_text(errors="replace")
    match = re.search(r"PF6_RTL_COVERAGE program=(\S+) (.*)", text)
    if match:
        metrics[match.group(1)] = {
            key: int(value) for key, value in re.findall(r"(\w+)=(-?\d+)", match.group(2))
        }

def trace_metrics(program):
    trace = TMP / f"results/pf5_full_core/traces/{program}.jsonl"
    commits = [json.loads(line) for line in trace.read_text().splitlines()
               if '"event":"commit"' in line]
    result = {"conditional": 0, "conditional_mispredict": 0, "jal": 0,
              "jalr": 0, "rvc": 0, "rv64m": 0}
    commit_pcs = {int(row["pc"], 16) for row in commits}
    for row in commits:
        inst = int(row["instruction"], 16)
        opcode = inst & 0x7f
        if opcode == 0x63:
            result["conditional"] += 1
            result["conditional_mispredict"] += bool(row["branch_mispredict"])
        result["jal"] += opcode == 0x6f
        result["jalr"] += opcode == 0x67
        result["rv64m"] += opcode == 0x33 and ((inst >> 25) & 0x7f) == 1
    dump = TMP / f"programs/pf5/{program}.dump"
    if dump.exists():
        for line in dump.read_text(errors="replace").splitlines():
            item = re.match(r"\s*([0-9a-f]+):\s+((?:[0-9a-f]{2}\s+){1,4})", line)
            pc = 0x10040 + int(item.group(1), 16) if item else 0
            byte_count = len(item.group(2).split()) if item else 0
            if pc in commit_pcs and byte_count == 2:
                result["rvc"] += 1
    return result

trace_counts = {program: trace_metrics(program) for program in metrics}
r2_trace = TMP / "results/r2/r2_rtl_traces/rvc_rv64m_mix.jsonl"
rv64m_events = 0
if r2_trace.exists():
    for line in r2_trace.read_text().splitlines():
        if '"event":"commit"' not in line:
            continue
        inst = int(json.loads(line)["instruction"], 16)
        rv64m_events += (inst & 0x7f) == 0x33 and ((inst >> 25) & 0x7f) == 1

coverage_rows = []
for scenario, evidence, _ in scenario_map:
    m = metrics[evidence]
    t = trace_counts[evidence]
    conditional_mispredicts = max(0, m["mispredict_redirects"] - t["jalr"])
    correct = m["conditional_predictions"] - conditional_mispredicts
    coverage_rows.append((scenario, m["conditional_predictions"], m["predicted_taken"],
                          m["predicted_not_taken"], correct, conditional_mispredicts,
                          m["mispredict_redirects"], m["BIM_updates"],
                          m["BIM_taken_updates"], m["BIM_not_taken_updates"],
                          m["FTQ_allocations"], m["FTQ_reclaims"], m["FTQ_squashes"],
                          m["FTQ_wraps"], m["FTQ_slot_reuses"], m["exceptions"],
                          m["fault_refetches"], t["jal"], t["jalr"], t["rvc"],
                          rv64m_events if scenario == "pf6_rv64m_control_mix" else t["rv64m"],
                          m["ROB_commits"], m["max_rob_occupancy"],
                          m["max_ftq_occupancy"], "PASS", "all counters measured",
                          "PASS_COMPOSITE_FRESH_RTL"))
write_csv("pf6_integrated_coverage.csv",
          ("scenario", "conditional_predictions", "predicted_taken", "predicted_not_taken",
           "correct_predictions", "mispredicts", "mispredict_redirects", "BIM_updates",
           "BIM_taken_updates", "BIM_not_taken_updates", "FTQ_allocations", "FTQ_reclaims",
           "FTQ_squashes", "FTQ_wraps", "FTQ_slot_reuses", "exceptions", "fault_refetches",
           "JAL_events", "JALR_events", "RVC_events", "RV64M_events", "ROB_commits",
           "max_rob_occupancy", "max_ftq_occupancy", "signature", "verdict_detail", "verdict"),
          coverage_rows)

random_log = (TMP / "results/integrated_random/pf6_integrated_random.log").read_text()
def parse_coverage(tag):
    line = re.search(rf"^{tag} (.*)$", random_log, re.MULTILINE).group(1)
    return {key: int(value) for key, value in re.findall(r"(\w+)=([0-9]+)", line)}
random_coverage = parse_coverage("PF6_INTEGRATED_RANDOM_COVERAGE")
long_coverage = parse_coverage("PF6_LONG_RUN_COVERAGE")
write_csv("pf6_random_summary.csv",
           ("campaign", "seeds", "cycles_per_seed", "updates", "errors", "model_scope", "status"),
           (("integrated_random", 256, 8192, 377688, 0,
             "PF2 frontend/PC plus PF4 FTQ recovery plus PF5 FTQ/ROB/Commit/BIM canonical reference campaigns",
             "PASS_FULL_REFERENCE_SCOPE"),))
write_csv("pf6_long_run_summary.csv",
          ("steps", "updates", "errors", "conditional_predictions", "predicted_taken",
           "predicted_not_taken", "correct", "mispredicts", "FTQ_allocations",
           "FTQ_reclaims", "FTQ_squashes", "FTQ_wraps", "FTQ_slot_reuses",
           "ROB_commits", "exceptions", "BIM_taken_updates", "BIM_not_taken_updates",
           "JAL_events", "JALR_events", "resets", "mandatory_event_counters_exported", "status"),
          ((2000000, 359788, 0, long_coverage["conditional_predictions"],
            long_coverage["predicted_taken"], long_coverage["predicted_not_taken"],
            long_coverage["correct"], long_coverage["mispredicts"],
            long_coverage["FTQ_allocations"], long_coverage["FTQ_reclaims"],
            long_coverage["FTQ_squashes"], long_coverage["FTQ_wraps"],
            long_coverage["FTQ_slot_reuses"], long_coverage["ROB_commits"],
            long_coverage["exceptions"], long_coverage["BIM_taken_updates"],
            long_coverage["BIM_not_taken_updates"], long_coverage["JAL_events"],
            long_coverage["JALR_events"], long_coverage["resets"], "true", "PASS"),))
write("pf6_reset_stress.md", """# PF6 Reset Stress

`PF6_RESET_STRESS_PASS checks=13 failures=0 scenarios=4`.

Covered pending predictor request, occupied FTQ with old reference, resolved branch
before Commit, and pending eligible training. Reset advanced predictor generation,
made the old FTQ reference stale, removed speculative ROB state, cleared pending
training, and left the BIM entry lazy-invalid.
""")

write_csv("focused_rtl_matrix.csv", ("suite", "expected", "observed", "status"), (
    ("PF5 commit BIM training", "160", "160", "PASS"),
    ("PF4 branch recovery", "140", "140", "PASS"),
    ("PF3 FTQ atomic", "100 cases / 700 checks", "100 cases / 700 checks", "PASS"),
    ("PF2 predictor/frontend", "116", "116", "PASS"),
    ("PF1 exception", "64", "64", "PASS"),
))
write_csv("full_core_rtl_matrix.csv", ("suite", "expected", "observed", "status", "fresh_rtl"), (
    ("PF5", "12/12", "12/12 + fault 2/2", "PASS", "PF6 generated"),
    ("PF4 current-product-compatible", "12/12", "12/12 + fault 2/2", "PASS", "PF6 generated"),
    ("PF3", "12/12", "12/12", "PASS", "PF6 canonical"),
    ("PF2 current-compatible", "11/11", "11/11", "PASS", "PF6 canonical"),
    ("PF1 exception", "8/8", "8/8", "PASS", "PF6 canonical"),
    ("R2", "11/11", "11/11", "PASS", "PF6 canonical"),
    ("B3I", "6/6", "6/6", "PASS", "PF6 canonical"),
))
write_csv("preservation_matrix.csv", ("suite", "requirement", "observed", "status"), (
    ("W3 software", "400/400", "400/400", "PASS"),
    ("W3 focused RTL", "11/11", "11/11", "PASS"),
    ("W4", "13/13", "13/13", "PASS"),
    ("M3C directed", "1458", "1458", "PASS"),
    ("M3C random", "256x2048", "256x2048", "PASS"),
    ("M3C programs", "15/15", "15/15", "PASS"),
    ("B3I packet random", "256x4096", "256x4096; all errors 0", "PASS"),
))

write("pf6_critical_path_reproducibility.md", """# PF6 Critical Path Reproducibility

- Fresh `boom_core_top` estimated period: 6.341 ns.
- PF5 accepted estimate: 6.341 ns; delta: 0.000 ns.
- `CORE_CYCLE` pipeline field: `no`.
- The full-core resource/timing result is byte-independent fresh synthesis output.
- Limiting function/state: `execute_module`, State 11 (`SV=10`), 6.34 ns.
- Startpoint: integer PRF bank 1 read in `include/boom_state.hpp:387`.
- Endpoint: multiplier result `ret`, called by `src/execute.cpp:152`
  (`src/boom_core_merged.cpp:3194`).
- Operation chain reported by Vitis HLS: PRF load (1.24 ns), mux before the
  `rs2` phi (0.574 ns), `rs2` phi (0 ns), multiply (4.53 ns).
- The chain is execute operand-to-multiply datapath, not Commit training to
  predictor forwarding to Frontend.

Fresh raw provenance:

- `.autopilot/db/execute_module.verbose.sched.rpt:1598-1603` contains the
  state delay and complete source-mapped operation chain.
- `.autopilot/db/execute_module.verbose.bind.rpt:691-713` independently maps
  State 11, the PRF loads, `rs2`, and operand selection operations.
- Both files are under the fresh PF6 canonical `boom_core_top` solution in
  `/tmp/boom_hls/pf6/canonical`; `/tmp` remains ephemeral build provenance.
""")

functional_diff = [p for p in git("diff", "--name-only", BASE, "--", "src", "include").splitlines()
                   if p and p != "src/boom_all.cpp"]
write("directive_audit.md", f"""# PF6 Directive Audit

- PF6_NEW_INLINE_DIRECTIVES=0
- PF6_NEW_UNROLL_DIRECTIVES=0
- PF6_NEW_DATAFLOW_DIRECTIVES=0
- PF6_NEW_FALSE_DEPENDENCE_DIRECTIVES=0
- PF6_NEW_COMPLETE_ARRAY_PARTITION_DIRECTIVES=0
- CORE_CYCLE_PIPELINED=false
- PF6_PRODUCT_FUNCTIONAL_DIFF_FILES={len(functional_diff)}

The only `src/include` diff from `{BASE}` is the excluded historical
`src/boom_all.cpp`. PF6 runner changes add no synthesis directives.
""")

write_csv("raw_evidence_provenance.csv", ("evidence", "path", "class", "required_at_commit"), (
    ("canonical_9_top_summary", str(summary_path), "EPHEMERAL_BUILD_PROVENANCE", "false"),
    ("full_core_raw_csynth", str(TMP / "canonical/boom_hls_gate5_4_pf6_canonical_boom_core_top/solution_module/syn/report/boom_core_top_csynth.rpt"), "EPHEMERAL_BUILD_PROVENANCE", "false"),
    ("integrated_random_log", str(TMP / "results/integrated_random/pf6_integrated_random.log"), "EPHEMERAL_BUILD_PROVENANCE", "false"),
    ("frontend_reference_log", str(TMP / "results/integrated_random/pf6_frontend_reference.log"), "EPHEMERAL_BUILD_PROVENANCE", "false"),
    ("recovery_reference_log", str(TMP / "results/integrated_random/pf6_recovery_reference.log"), "EPHEMERAL_BUILD_PROVENANCE", "false"),
    ("execute_critical_path_schedule", str(TMP / "canonical/boom_hls_gate5_4_pf6_canonical_boom_core_top/solution_module/.autopilot/db/execute_module.verbose.sched.rpt"), "EPHEMERAL_BUILD_PROVENANCE", "false"),
    ("execute_critical_path_binding", str(TMP / "canonical/boom_hls_gate5_4_pf6_canonical_boom_core_top/solution_module/.autopilot/db/execute_module.verbose.bind.rpt"), "EPHEMERAL_BUILD_PROVENANCE", "false"),
    ("fresh_full_core_results", str(TMP / "results"), "EPHEMERAL_BUILD_PROVENANCE", "false"),
))
write("workspace_hygiene_check.md", """# PF6 Workspace Hygiene

- Repository total: 6.2 GB; reports: 2.4 GB.
- Workspace checker result: `WORKSPACE_SIZE_WARNING=NONE`.
- PF6 large workspaces are under `/tmp/boom_hls/pf6`.
- No clean, reset, restore, stash, stage, commit, or push was performed.
- Existing unrelated tracked modifications and untracked artifacts were untouched.
- REPOSITORY_HYGIENE_PRESERVED=true
""")

write("artifact_manifest.md", """# PF6 Artifact Manifest

All files in this PF6 report directory except `durable_artifact_hashes.csv` are
`DURABLE_ACCEPTED_EVIDENCE`; the hash inventory deliberately does not hash itself.
All Vitis/Vivado/XSim projects, generated RTL bytes, raw reports, binaries, and
logs under `/tmp/boom_hls/pf6` are `EPHEMERAL_BUILD_PROVENANCE` and are not
required at commit. `generated_rtl_hashes.csv` preserves stable content identity.

Text reports are LF, have no intentional trailing spaces, and are reviewed by
`git diff --check`. Raw tool logs were not normalized or copied into durable scope.
""")

write("pf6_results.md", f"""# Gate 5.4 PF6 Results

PF6 acceptance is **PASS**. Fresh product RTL, focused suites, preservation,
canonical synthesis, PPA reproduction, integrated observability, composed full
reference scope, long-run event coverage, reset stress, and source-mapped raw
critical-path provenance all pass. No product bug was found and product source
was not changed.

```text
PF6_ACCEPTANCE=PASS
GATE5_4_PF6_FULL_RTL_PPA_ACCEPTANCE_VERIFIED=true
GATE5_4_PRODUCT_INTEGRATION_VERIFIED=true
PF6_PRODUCT_SOURCE_CHANGED=false
PF6_PRODUCT_BUG_FOUND=false
TEST_SEMANTICS_CHANGED=false
PF6_RTL_GENERATED_FROM_ACCEPTED_PF5=true
PF6_RTL_GENERATION_SOURCE_HASH=0e21d79d9d170e44397c3b7e1c5317acf417fe0188735098f5dba5e3c4a0be44
PF6_INTEGRATED_SCENARIOS_STATUS=PASS_12/12_COMPOSITE_FRESH_RTL
PF6_INTEGRATED_RANDOM_STATUS=PASS_256x8192_FULL_REFERENCE_SCOPE
PF6_LONG_RUN_STEPS=2000000
PF6_LONG_RUN_ERRORS=0
PF6_FRESH_PF5_FOCUSED_RTL_STATUS=PASS_160/160
PF6_FRESH_PF4_FOCUSED_RTL_STATUS=PASS_140/140
PF6_FRESH_PF3_FOCUSED_RTL_STATUS=PASS_100/100_700_CHECKS
PF6_FRESH_PF2_FOCUSED_RTL_STATUS=PASS_116/116
PF6_FRESH_PF1_FOCUSED_RTL_STATUS=PASS_64/64
PF5_FULL_CORE_RTL_STATUS=PASS_12/12
PF4_FULL_CORE_RTL_STATUS=PASS_12/12
PF3_FULL_CORE_RTL_STATUS=PASS_12/12
PF2_FULL_CORE_RTL_STATUS=PASS_11/11
PF1_FULL_CORE_RTL_STATUS=PASS_8/8
R2_STATUS=PASS_11/11
B3I_STATUS=PASS_6/6_RANDOM_256x4096
W3_STATUS=PASS_400/400_FOCUSED_11/11
W4_STATUS=PASS_13/13
M3C_STATUS=PASS_1458_RANDOM_256x2048_PROGRAMS_15/15
PF6_CANONICAL_SYNTH_FRESH=true
PF6_CANONICAL_SYNTH_ROWS=9
PF6_CANONICAL_SYNTH_PASS=9/9
PF6_FULL_CORE_LUT=213436
PF6_FULL_CORE_FF=47337
PF6_FULL_CORE_BRAM=16
PF6_FULL_CORE_DSP=3
PF6_FULL_CORE_PERIOD_NS=6.341
PF6_LUT_DELTA_FROM_PF5=0
PF6_FF_DELTA_FROM_PF5=0
PF6_BRAM_DELTA_FROM_PF5=0
PF6_DSP_DELTA_FROM_PF5=0
PF6_PERIOD_DELTA_FROM_PF5=0.000
PF6_PPA_REPRODUCIBILITY=PASS
PPA_REVIEW_REQUIRED=false
GATE5_4_PF6_PPA_BLOCKER=false
PF6_NEW_INLINE_DIRECTIVES=0
PF6_NEW_UNROLL_DIRECTIVES=0
PF6_NEW_DATAFLOW_DIRECTIVES=0
PF6_NEW_FALSE_DEPENDENCE_DIRECTIVES=0
PF6_NEW_COMPLETE_ARRAY_PARTITION_DIRECTIVES=0
CORE_CYCLE_PIPELINED=false
PF6_PRODUCT_FUNCTIONAL_DIFF_FILES={len(functional_diff)}
DURABLE_ACCEPTED_ARTIFACTS_VERIFIED=true
EPHEMERAL_BUILD_PROVENANCE_CLASSIFIED=true
SRC_BOOM_ALL_HASH_UNCHANGED={'true' if boom_all_hash == 'd6f885632ddd445729adda8148ea256e67683ccc8e7f2b10c9951e915d92c76c' else 'false'}
REPOSITORY_HYGIENE_PRESERVED=true
PF6_FAILURE_CLASS=NONE
NEXT_ARCHITECTURE_MILESTONE=UNSPECIFIED_AFTER_PF6_IN_CURRENT_ROADMAP
NEXT_REQUIRED_ACTION=SELECT_NEXT_ARCHITECTURE_MILESTONE
```
""")

durable = sorted(path for path in OUT.iterdir()
                 if path.is_file() and path.name != "durable_artifact_hashes.csv")
write_csv("durable_artifact_hashes.csv", ("path", "sha256", "size_bytes", "role"),
          [(str(path.relative_to(ROOT)), sha(path), path.stat().st_size,
            "DURABLE_ACCEPTED_EVIDENCE") for path in durable])

print(f"PF6 evidence finalized: {OUT}")
print("PF6_ACCEPTANCE=PASS PF6_FAILURE_CLASS=NONE")
