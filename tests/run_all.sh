#!/usr/bin/env sh
set -e

# Resolve script location so paths are stable
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
TEST_DIR="$SCRIPT_DIR/test cases"
WBASIC="$SCRIPT_DIR/../wbasic"

echo "=============================================="
echo "WBASIC Torture Test Suite Runner"
echo "Interpreter : $WBASIC"
echo "Test dir    : $TEST_DIR"
echo "=============================================="
echo

for test in "$TEST_DIR"/WBASIC_TT_*.bas; do
    echo "----------------------------------------------"
    echo "Running: $(basename "$test")"
    echo "----------------------------------------------"

    "$WBASIC" -r "$test"

    echo "Finished: $(basename "$test")"
    echo
done

echo "=============================================="
echo "All WBASIC torture tests have been launched."
echo "=============================================="

