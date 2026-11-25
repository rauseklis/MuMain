#!/usr/bin/env bash
# This project only ships Windows builds. Fail if a Linux or macOS build job in a
# workflow is not switched off with `if: false`.
set -euo pipefail

status=0
for file in .github/workflows/ci.yml .github/workflows/fork-ci.yml; do
  if ! python3 - "$file" <<'PY'
import re
import sys

path = sys.argv[1]
text = open(path, encoding="utf-8").read()
jobs = re.split(r"(?m)^  (?=[A-Za-z0-9_-]+:\s*$)", text.split("\njobs:\n", 1)[1])
bad = []
for job in jobs:
    head = job.split("\n", 1)[0].strip()
    runs_on = re.search(r"(?m)^    runs-on:\s*(\S+)", job)
    if not runs_on:
        continue
    runner = runs_on.group(1)
    if ("ubuntu" in runner or "macos" in runner) and head not in ("quality:", "release:", "claude:", "release-data:", "notify:"):
        if not re.search(r"(?m)^    if:\s*false\s*$", job):
            bad.append(head.rstrip(":"))
if bad:
    print(f"{path}: Linux/macOS build job(s) not switched off: {', '.join(bad)}")
    sys.exit(1)
PY
  then
    status=1
  fi
done
exit $status
