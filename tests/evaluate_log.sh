#!/usr/bin/env bash
set -e

if [ $# -ne 1 ]; then
    echo "Usage: $(basename "$0") <logfile>"
    exit 2
fi

LOGFILE="$1"

if [ ! -f "$LOGFILE" ]; then
    echo "Error: file not found: $LOGFILE"
    exit 2
fi

# Show lines where FAIL is non-zero
if grep -E 'FAIL[[:space:]]*=[[:space:]]*[1-9]' "$LOGFILE"; then
    echo
    echo "❌ Failures detected in $LOGFILE"
    exit 1
else
    echo "✅ All tests passed (FAIL=0 everywhere)"
fi

