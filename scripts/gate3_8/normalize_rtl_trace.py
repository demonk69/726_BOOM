#!/usr/bin/env python3
import argparse
import json
from pathlib import Path


ENTRY_PC = 0x80000000


def parse_hex(value):
    if value is None:
        return None
    return int(value, 16) if isinstance(value, str) else int(value)


def normalize_commit(record):
    rd_valid = bool(record.get("rd_valid"))
    memory_valid = bool(record.get("memory_valid"))
    exception = bool(record.get("exception"))
    return {
        "event": "commit",
        "pc": parse_hex(record.get("pc")),
        "instruction": parse_hex(record.get("instruction")),
        "rd_valid": rd_valid,
        "rd": int(record.get("rd")) if rd_valid else None,
        "rd_value": parse_hex(record.get("rd_value")) if rd_valid else None,
        "exception": exception,
        "exception_cause": parse_hex(record.get("exception_cause")) if exception else None,
        "memory_valid": memory_valid,
        "memory_address": parse_hex(record.get("memory_address")) if memory_valid else None,
        "memory_data": parse_hex(record.get("memory_data")) if memory_valid else None,
        "memory_mask": parse_hex(record.get("memory_mask")) if memory_valid else None,
        "is_store": bool(record.get("is_store")) if memory_valid else False,
    }


def load_records(path):
    records = []
    for line in path.read_text().splitlines():
        if line.strip():
            records.append(json.loads(line))
    return records


def normalize(path, post_last_reset=True):
    records = load_records(path)
    reset_releases = [record["cycle"] for record in records if record.get("event") == "reset" and record.get("reset") is False]
    cutoff = reset_releases[-1] if post_last_reset and reset_releases else -1
    normalized = []
    for record in records:
        if record.get("cycle", 0) < cutoff:
            continue
        if record.get("event") == "commit" and parse_hex(record.get("pc")) >= ENTRY_PC:
            normalized.append(normalize_commit(record))
        elif record.get("event") == "tohost":
            normalized.append({
                "event": "tohost",
                "address": parse_hex(record.get("address")),
                "value": parse_hex(record.get("value")),
                "mask": parse_hex(record.get("mask")),
                "committed": bool(record.get("committed")),
            })
    return normalized


def main():
    parser = argparse.ArgumentParser(description="Normalize Gate 3.8 RTL JSONL")
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--all-resets", action="store_true", help="Do not discard records before the last reset release")
    args = parser.parse_args()
    records = normalize(args.input, post_last_reset=not args.all_resets)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w") as handle:
        for record in records:
            handle.write(json.dumps(record, sort_keys=True, separators=(",", ":")) + "\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
