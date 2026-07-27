#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from pathlib import Path

PROGRAMS = [
    "branch_not_taken",
    "branch_taken",
    "independent_alu",
    "nested_branch",
    "raw_chain",
]
SOURCES = ["hls_cpp", "hls_csim"]


def files_identical(a: Path, b: Path) -> bool:
    return a.exists() and b.exists() and a.read_bytes() == b.read_bytes()


def read_csv(path: Path) -> list[dict[str, str]]:
    if not path.exists():
        return []
    with path.open(encoding="utf-8", newline="") as handle:
        return list(csv.DictReader(handle))


def write_trace_diff(root: Path, variant: str) -> bool:
    report_dir = root / "reports" / "gate3_5" / "variants" / variant
    baseline = root / "reports" / "gate3_5" / "baseline_artifacts" / "baseline_traces"
    trace_dir = report_dir / "hls_traces"
    rows = []
    all_pass = True
    for source in SOURCES:
        for program in PROGRAMS:
            name = f"{program}_{source}_full.jsonl"
            base_path = baseline / source / name
            var_path = trace_dir / name
            status = "PASS" if files_identical(base_path, var_path) else "FAIL"
            if status != "PASS":
                all_pass = False
            rows.append((program, source, status, base_path, var_path))

    lines = [
        f"# {variant} Trace Diff",
        "",
        f"Result: {sum(1 for r in rows if r[2] == 'PASS')}/{len(rows)} byte-identical",
        "",
        "| Program | Source | Status |",
        "|---|---|---|",
    ]
    for program, source, status, _base, _var in rows:
        lines.append(f"| {program} | {source} | {status} |")
    if not all_pass:
        lines.extend(["", "## First Non-Identical Files", ""])
        for program, source, status, base_path, var_path in rows:
            if status != "PASS":
                lines.append(f"- {program} {source}: baseline={base_path}, variant={var_path}")
                break
    (report_dir / "trace_diff.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    return all_pass


def write_resource_delta(root: Path, variant: str) -> None:
    report_dir = root / "reports" / "gate3_5" / "variants" / variant
    baseline_rows = {row["module"]: row for row in read_csv(root / "reports" / "gate3_4" / "module_baseline.csv")}
    variant_rows = read_csv(report_dir / "csynth_summary.csv")
    fields = ["module", "status", "baseline_lut", "variant_lut", "delta_lut", "baseline_ff", "variant_ff", "delta_ff", "baseline_bram_18k", "variant_bram_18k", "baseline_dsp", "variant_dsp", "baseline_period_ns", "variant_period_ns"]
    with (report_dir / "resource_delta.csv").open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        for row in variant_rows:
            base = baseline_rows.get(row["module"], {})
            def intval(value: str) -> int | None:
                try:
                    return int(value)
                except Exception:
                    return None
            bl = intval(base.get("lut", ""))
            vl = intval(row.get("lut", ""))
            bf = intval(base.get("ff", ""))
            vf = intval(row.get("ff", ""))
            writer.writerow({
                "module": row.get("module", ""),
                "status": row.get("status", ""),
                "baseline_lut": base.get("lut", ""),
                "variant_lut": row.get("lut", ""),
                "delta_lut": "" if bl is None or vl is None else vl - bl,
                "baseline_ff": base.get("ff", ""),
                "variant_ff": row.get("ff", ""),
                "delta_ff": "" if bf is None or vf is None else vf - bf,
                "baseline_bram_18k": base.get("bram_18k", ""),
                "variant_bram_18k": row.get("bram_18k", ""),
                "baseline_dsp": base.get("dsp", ""),
                "variant_dsp": row.get("dsp", ""),
                "baseline_period_ns": base.get("estimated_period_ns", ""),
                "variant_period_ns": row.get("estimated_period_ns", ""),
            })


def write_csynth_summary_md(root: Path, variant: str) -> None:
    report_dir = root / "reports" / "gate3_5" / "variants" / variant
    rows = read_csv(report_dir / "csynth_summary.csv")
    lines = [
        f"# {variant} Csynth Summary",
        "",
        "| Module | Status | Period ns | LUT | FF | BRAM_18K | DSP | Runtime s |",
        "|---|---|---:|---:|---:|---:|---:|---:|",
    ]
    for row in rows:
        lines.append(f"| {row.get('module','')} | {row.get('status','')} | {row.get('estimated_period_ns','')} | {row.get('lut','')} | {row.get('ff','')} | {row.get('bram_18k','')} | {row.get('dsp','')} | {row.get('runtime_seconds','')} |")
    (report_dir / "csynth_summary.md").write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--variant", required=True)
    parser.add_argument("--trace-only", action="store_true")
    args = parser.parse_args()
    root = args.root.resolve()
    trace_ok = write_trace_diff(root, args.variant)
    if not args.trace_only:
        write_resource_delta(root, args.variant)
        write_csynth_summary_md(root, args.variant)
    return 0 if trace_ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
