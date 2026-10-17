#!/usr/bin/env python3
"""Create comment-free $readmemh views without changing program image bytes."""
import hashlib
import json
import sys
from pathlib import Path


PROGRAMS = ["independent_alu", "raw_chain", "branch_taken", "branch_not_taken", "nested_branch"]


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: ref1_prepare_programs.py <reference_hex_dir> <output_dir>")
    source_dir, output_dir = map(Path, sys.argv[1:])
    output_dir.mkdir(parents=True, exist_ok=True)
    manifest = []
    for name in PROGRAMS:
        source = source_dir / f"{name}.hex"
        target = output_dir / f"{name}.readmemh"
        words = []
        for lineno, line in enumerate(source.read_text(encoding="ascii").splitlines(), 1):
            token = line.split("#", 1)[0].strip()
            if not token:
                continue
            if len(token) != 16 or any(c not in "0123456789abcdefABCDEF" for c in token):
                raise SystemExit(f"{source}:{lineno}: expected one 64-bit hex word, got {token!r}")
            words.append(token.lower())
        if not words:
            raise SystemExit(f"{source}: no words")
        data = ("\n".join(words) + "\n").encode("ascii")
        target.write_bytes(data)
        # The input reference runner reverses each line's bytes into little-endian
        # memory. $readmemh loads each same 64-bit token; lower 32 bits therefore
        # present the identical first instruction to the HLS instruction model.
        byte_image = b"".join(bytes.fromhex(word)[::-1] for word in words)
        manifest.append({
            "program": name,
            "source_path": str(source),
            "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
            "readmemh_path": str(target),
            "readmemh_sha256": hashlib.sha256(data).hexdigest(),
            "instruction_image_bytes_sha256": hashlib.sha256(byte_image).hexdigest(),
            "word_count_64": len(words),
            "instruction_image_bytes": len(byte_image),
        })
    (output_dir / "program_image_manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )


if __name__ == "__main__":
    main()
