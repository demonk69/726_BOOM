#!/usr/bin/env python3
"""Derive REF1's HLS-only adapter from the prior stream TB with zero DMEM init."""
import hashlib
import json
import sys
from pathlib import Path


EXPECTED_INITIALIZERS = [
    "        write_qword(DATA_BASE + 64'd0, 64'd3, 8'hff);",
    "        write_qword(DATA_BASE + 64'd8, 64'd5, 8'hff);",
    "        write_qword(DATA_BASE + 64'd16, 64'd7, 8'hff);",
    "        write_qword(DATA_BASE + 64'd24, 64'd11, 8'hff);",
    "        write_qword(DATA_BASE + 64'd40, 64'hffff_ffff_ffff_ff00, 8'hff);",
]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    if len(sys.argv) != 4:
        raise SystemExit("usage: ref1_prepare_hls_tb.py <source_tb.sv> <output_tb.sv> <manifest.json>")
    source, output, manifest_path = map(Path, sys.argv[1:])
    text = source.read_text(encoding="utf-8")
    lines = text.splitlines()
    remaining = list(EXPECTED_INITIALIZERS)
    filtered = []
    removed = []
    for line in lines:
        if line in remaining:
            remaining.remove(line)
            removed.append(line)
        else:
            filtered.append(line)
    if remaining or len(removed) != len(EXPECTED_INITIALIZERS):
        raise SystemExit(f"expected exactly five PPA6 data sentinel initializers; missing={remaining}")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text("\n".join(filtered) + "\n", encoding="utf-8")
    manifest = {
        "purpose": "REF1 HLS adapter with zero-initialized DMEM to match reference sparse-memory default",
        "source_tb": str(source),
        "source_tb_sha256": sha(source),
        "derived_tb": str(output),
        "derived_tb_sha256": sha(output),
        "removed_initializer_lines": removed,
        "removed_initializer_count": len(removed),
        "product_source_modified": False,
    }
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
