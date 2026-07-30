#!/usr/bin/env python3
import argparse
import re
import xml.etree.ElementTree as ET
from pathlib import Path


def value(root, name):
    node = root.find(f".//{name}")
    return node.text if node is not None else ""


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--configuration", required=True)
    parser.add_argument("--variant", required=True)
    parser.add_argument("--requested", required=True, type=float)
    parser.add_argument("--report", required=True, type=Path)
    parser.add_argument("--time", required=True, type=Path)
    args = parser.parse_args()
    root = ET.parse(args.report).getroot()
    estimated = float(value(root, "EstimatedClockPeriod"))
    runtime_match = re.search(r"runtime_seconds=([0-9.]+)", args.time.read_text())
    fields = [args.configuration, args.variant, f"{args.requested:.1f}", f"{estimated:.3f}",
              "YES" if estimated <= args.requested else "NO", "lsu_module/load_value",
              value(root, "LUT"), value(root, "FF"), value(root, "BRAM_18K"), value(root, "DSP"),
              runtime_match.group(1) if runtime_match else ""]
    print(",".join(fields))


if __name__ == "__main__":
    main()
