#!/usr/bin/env python3
import subprocess
import sys
from pathlib import Path
import platform

is_macos = platform.system() == 'Darwin'
is_windows = platform.system() == 'Windows'

ROOT = Path(__file__).resolve().parent
BLU = ROOT / "out" / "blu.exe" if is_windows else "blu"
TEST_DIR = ROOT / "test" / "basic"


def run_test(blu_file: Path) -> tuple[bool, str]:
    expected_path = blu_file.with_suffix(blu_file.suffix + ".expected")
    expected_stdout = expected_path.read_text() if expected_path.exists() else ""

    result = subprocess.run(
        [str(BLU), str(blu_file)],
        capture_output=True,
        text=True,
    )

    if result.returncode != 0:
        return False, f"exit code {result.returncode}\nstderr:\n{result.stderr}"

    if result.stdout != expected_stdout:
        return False, (
            f"stdout mismatch\n"
            f"--- expected ---\n{expected_stdout!r}\n"
            f"--- actual ---\n{result.stdout!r}"
        )

    return True, ""


def main() -> int:
    if not BLU.exists():
        print(f"error: {BLU} not found — run python build.py first", file=sys.stderr)
        return 2

    blu_files = sorted(TEST_DIR.glob("*.blu"))
    if not blu_files:
        print(f"error: no .blu files in {TEST_DIR}", file=sys.stderr)
        return 2

    passed = 0
    failed = 0
    for f in blu_files:
        ok, message = run_test(f)
        rel = f.relative_to(ROOT)
        if ok:
            print(f"PASS  {rel}")
            passed += 1
        else:
            print(f"FAIL  {rel}")
            for line in message.splitlines():
                print(f"      {line}")
            failed += 1

    print()
    print(f"{passed} passed, {failed} failed")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
