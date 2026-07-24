#!/usr/bin/env python3
import argparse
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path


def load_trace(path):
    records = []
    commits = []
    metadata = []
    with open(path, "r", encoding="utf-8") as f:
        for lineno, line in enumerate(f, 1):
            line = line.strip()
            if not line:
                continue
            rec = json.loads(line)
            rec["_lineno"] = lineno
            records.append(rec)
            if rec.get("event") == "commit":
                commits.append(rec)
            elif rec.get("event") == "metadata":
                metadata.append(rec)
    return records, commits, metadata


def parse_loadmem(path, base):
    mapping = {}
    addr = base
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.split("#", 1)[0].strip()
            if not line:
                continue
            if len(line) % 2:
                raise ValueError(f"odd-length loadmem line: {line}")
            data = bytes.fromhex(line)
            # The standalone loader consumes byte pairs from the end of each line.
            data = data[::-1]
            for i in range(0, len(data), 4):
                word = int.from_bytes(data[i:i + 4], "little")
                mapping[addr + i] = f"0x{word:08x}"
            addr += len(data)
    return mapping


def require_tool(name):
    path = shutil.which(name)
    if not path:
        raise RuntimeError(f"required tool not found: {name}")
    return path


def run_tool(cmd):
    return subprocess.run(cmd, check=False, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout


def parse_objdump(output):
    mapping = {}
    inst_re = re.compile(r"^\s*([0-9a-fA-F]+):\s+([0-9a-fA-F]+)\s+(.*)$")
    for line in output.splitlines():
        m = inst_re.match(line)
        if not m:
            continue
        pc = int(m.group(1), 16)
        enc = int(m.group(2), 16)
        mapping[pc] = (f"0x{enc:08x}", m.group(3).strip())
    return mapping


def validate_mapping(commits, mapping, max_commits):
    checked = 0
    mismatches = []
    loaded_commits = []
    for rec in commits:
        pc_s = rec.get("pc")
        inst_s = rec.get("instruction")
        if not pc_s:
            continue
        pc = int(pc_s, 16)
        if pc not in mapping:
            continue
        expected = mapping[pc][0] if isinstance(mapping[pc], tuple) else mapping[pc]
        loaded_commits.append(rec)
        checked += 1
        if inst_s != expected:
            mismatches.append((rec.get("_lineno"), pc_s, inst_s, expected))
        if checked >= max_commits:
            break
    return loaded_commits, mismatches


def write_report(path, title, lines):
    with open(path, "w", encoding="utf-8") as f:
        f.write(f"# {title}\n\n")
        for line in lines:
            f.write(line.rstrip() + "\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--trace", required=True)
    ap.add_argument("--elf")
    ap.add_argument("--loadmem")
    ap.add_argument("--loadmem-addr", default="0x80000000")
    ap.add_argument("--report", required=True)
    ap.add_argument("--max-commits", type=int, default=64)
    args = ap.parse_args()

    report = []
    status = "FAIL"
    exit_code = 1

    try:
        records, commits, metadata = load_trace(args.trace)
        report.append(f"Trace: `{args.trace}`")
        report.append(f"Records: {len(records)}")
        report.append(f"Commits: {len(commits)}")
        end_meta = next((m for m in reversed(metadata) if m.get("phase") == "end"), None)
        if end_meta:
            report.append(f"Termination: `{end_meta.get('termination_reason')}`")
            report.append(f"Exit code: `{end_meta.get('exit_code')}`")
            report.append(f"Tohost value: `{end_meta.get('tohost_value')}`")
            report.append(f"Max cycles reached: `{end_meta.get('max_cycles_reached')}`")

        if args.loadmem:
            mapping = parse_loadmem(args.loadmem, int(args.loadmem_addr, 0))
            loaded_commits, mismatches = validate_mapping(commits, mapping, args.max_commits)
            report.append("")
            report.append("## Loadmem Check")
            report.append(f"Loadmem: `{args.loadmem}`")
            report.append(f"Mapped instructions: {len(mapping)}")
            report.append(f"Checked loaded commits: {len(loaded_commits)}")
            if mismatches:
                report.append("Mismatches:")
                for lineno, pc, got, exp in mismatches[:20]:
                    report.append(f"- line {lineno}: pc {pc} got {got}, expected {exp}")
            if not loaded_commits:
                report.append("No commits matched the loadmem address map.")
            elif not mismatches and end_meta and end_meta.get("termination_reason") != "max_cycles" and end_meta.get("exit_code") == 0:
                status = "PASS_LOADMEM"
                exit_code = 0

        if args.elf:
            report.append("")
            report.append("## ELF Check")
            report.append(f"ELF: `{args.elf}`")
            if not os.path.exists(args.elf):
                raise RuntimeError(f"ELF not found: {args.elf}")
            readelf = require_tool(os.environ.get("RISCV_READELF", "riscv64-unknown-elf-readelf"))
            objdump = require_tool(os.environ.get("RISCV_OBJDUMP", "riscv64-unknown-elf-objdump"))
            nm = require_tool(os.environ.get("RISCV_NM", "riscv64-unknown-elf-nm"))
            readelf_out = run_tool([readelf, "-h", "-S", args.elf])
            objdump_out = run_tool([objdump, "-d", args.elf])
            nm_out = run_tool([nm, args.elf])
            mapping = parse_objdump(objdump_out)
            loaded_commits, mismatches = validate_mapping(commits, mapping, args.max_commits)
            report.append(f"readelf bytes: {len(readelf_out)}")
            report.append(f"objdump instructions: {len(mapping)}")
            report.append(f"nm bytes: {len(nm_out)}")
            report.append(f"Checked ELF commits: {len(loaded_commits)}")
            if mismatches:
                for lineno, pc, got, exp in mismatches[:20]:
                    report.append(f"- line {lineno}: pc {pc} got {got}, expected {exp}")
            elif loaded_commits and status != "PASS_LOADMEM":
                status = "PASS_ELF"
                exit_code = 0

        if not args.elf and not args.loadmem:
            raise RuntimeError("either --elf or --loadmem is required")
    except Exception as exc:
        report.append("")
        report.append(f"Error: {exc}")
        exit_code = 2

    report.insert(0, f"Status: {status}")
    write_report(args.report, "Trace Against ELF/Loadmem Check", report)
    return exit_code


if __name__ == "__main__":
    sys.exit(main())
