#!/usr/bin/env python3
"""Snapshot tests for the .blu programs in test/snippet.

Programs under error/ must exit 1, all others must exit 0.
Every program's output is snapshotted to <name>.blu.snap next to it.
Run with --update to re-record the snapshots.
"""

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BLU = ROOT / "out" / ("blu.exe" if sys.platform == "win32" else "blu")
SNIPPETS = ROOT / "test" / "snippet"
ANSI = re.compile(r"\x1b\[[0-9;]*m")


def run(path):
    p = subprocess.run(
        [BLU, path], capture_output=True, text=True, cwd=ROOT
    )
    return p.returncode, ANSI.sub("", p.stdout + p.stderr)


def main():
    update = "--update" in sys.argv
    if not BLU.exists():
        sys.exit(f"{BLU} not found, run python build.py first")

    failed = new = 0
    for path in sorted(SNIPPETS.rglob("*.blu")):
        rel = path.relative_to(ROOT)
        want_code = 1 if "error" in path.relative_to(SNIPPETS).parts else 0
        code, output = run(rel)

        if code != want_code:
            print(f"FAIL {rel}: exit {code}, expected {want_code}")
            print(output)
            failed += 1
            continue

        snap = path.with_suffix(".blu.snap")
        if update or not snap.exists():
            snap.write_text(output)
            print(f"{'UPDATED' if snap.exists() and update else 'NEW'} {rel}")
            new += 1
        elif snap.read_text() != output:
            print(f"FAIL {rel}: output differs from snapshot")
            print(diff(snap.read_text(), output))
            failed += 1
        else:
            print(f"ok   {rel}")

    print(f"\n{failed} failed, {new} recorded")
    return 1 if failed else 0


def diff(expected, actual):
    import difflib

    return "".join(
        difflib.unified_diff(
            expected.splitlines(True),
            actual.splitlines(True),
            "snapshot",
            "actual",
        )
    )


if __name__ == "__main__":
    sys.exit(main())
