#!/usr/bin/env python3
import argparse
import csv
import re
from pathlib import Path


DECLARATION = re.compile(r"^(input|output)\s+(?:reg\s+)?(?:\[(\d+):(\d+)\]\s+)?(\w+)\s*;")


def main() -> int:
    parser = argparse.ArgumentParser(description="Extract generated boom_core_top RTL ports")
    parser.add_argument("rtl", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    rows = []
    for line_no, raw in enumerate(args.rtl.read_text().splitlines(), 1):
        match = DECLARATION.match(raw.strip())
        if not match:
            continue
        direction, msb, lsb, name = match.groups()
        width = abs(int(msb) - int(lsb)) + 1 if msb is not None else 1
        interface = "scalar"
        signal = name
        for bundle in ("imem_req_out", "imem_resp_in", "dmem_req_out", "dmem_resp_in", "commit_trace_out"):
            if name.startswith(bundle + "_"):
                interface = bundle
                signal = name[len(bundle) + 1 :]
                break
        rows.append((name, direction, width, interface, signal, "absent" if signal != "TLAST" else "present", line_no))

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(("port", "direction", "width", "interface", "axis_signal", "tlast", "rtl_line"))
        writer.writerows(rows)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
