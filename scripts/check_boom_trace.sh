#!/usr/bin/env bash
set -euo pipefail

TRACE="${1:-}"
if [[ -z "${TRACE}" ]]; then
  echo "ERROR: trace path required" >&2
  exit 2
fi
if [[ ! -f "${TRACE}" ]]; then
  echo "ERROR: trace file not found: ${TRACE}" >&2
  exit 3
fi
if [[ ! -s "${TRACE}" ]]; then
  echo "ERROR: trace file is empty: ${TRACE}" >&2
  exit 4
fi

python3 - "$TRACE" <<'PY'
import json, sys
path = sys.argv[1]
last_cycle = -1
events = 0
commits = 0
seen_commits = set()
with open(path, 'r', encoding='utf-8') as f:
    for lineno, line in enumerate(f, 1):
        line=line.strip()
        if not line:
            continue
        try:
            rec=json.loads(line)
        except Exception as e:
            print(f"ERROR: line {lineno}: invalid JSON: {e}", file=sys.stderr)
            sys.exit(5)
        cycle=rec.get('cycle')
        if not isinstance(cycle, int):
            print(f"ERROR: line {lineno}: cycle missing/not integer", file=sys.stderr)
            sys.exit(6)
        if cycle < last_cycle:
            print(f"ERROR: line {lineno}: cycle decreased {cycle} < {last_cycle}", file=sys.stderr)
            sys.exit(7)
        last_cycle=cycle
        events += 1
        if rec.get('event') == 'commit':
            commits += 1
            key=(rec.get('cycle'), rec.get('slot'), rec.get('pc'), rec.get('instruction'), rec.get('rd'))
            if key in seen_commits:
                print(f"ERROR: duplicate commit-like record at line {lineno}: {key}", file=sys.stderr)
                sys.exit(8)
            seen_commits.add(key)
if events == 0:
    print("ERROR: no records", file=sys.stderr)
    sys.exit(9)
print(f"TRACE_CHECK PASS events={events} commits={commits} last_cycle={last_cycle}")
PY
