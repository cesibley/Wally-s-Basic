#!/usr/bin/env sh
set -e

# Resolve script location so paths are stable
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
TEST_DIR="$SCRIPT_DIR/test cases"
WBASIC="$SCRIPT_DIR/../wbasic"

MODE="-r"  # default (current behavior)

usage() {
  echo "Usage: $(basename "$0") [-r|--run] [-c|--cli]"
  echo "  -r, --run   Run tests in normal mode (default)"
  echo "  -c, --cli   Run tests using CLI mode (../wbasic -c <file>)"
  exit 2
}

# Parse optional mode flag(s)
while [ $# -gt 0 ]; do
  case "$1" in
    -c|--cli) MODE="-c"; shift ;;
    -r|--run) MODE="-r"; shift ;;
    -h|--help) usage ;;
    *) echo "Unknown option: $1"; usage ;;
  esac
done

echo "=============================================="
echo "WBASIC Torture Test Suite Runner"
echo "Interpreter : $WBASIC"
echo "Test dir    : $TEST_DIR"
echo "Mode        : $MODE"
echo "=============================================="
echo

for test in "$TEST_DIR"/WBASIC_TT_*.bas; do
    echo "----------------------------------------------"
    echo "Running: $(basename "$test")"
    echo "----------------------------------------------"

    "$WBASIC" "$MODE" "$test"

    echo "Finished: $(basename "$test")"
    echo
done

echo "=============================================="
echo "All WBASIC torture tests have been launched."
echo "=============================================="

