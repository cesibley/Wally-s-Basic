#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="$SCRIPT_DIR/test cases"
WBASIC="$SCRIPT_DIR/../wbasic"

MODE="-r"
QUIET=0
FAILFAST=0

if [[ -t 1 ]]; then
  INV_GREEN=$'\033[7;32m'
  INV_RED=$'\033[7;31m'
  RESET=$'\033[0m'
else
  INV_GREEN=""
  INV_RED=""
  RESET=""
fi

usage() {
  echo "Usage: $(basename "$0") [-r|--run] [-c|--cli] [-q|--quiet] [--fail-fast]"
  echo "  -r, --run       Run tests in normal mode (default)"
  echo "  -c, --cli       Run tests using CLI mode"
  echo "  -q, --quiet     Suppress per-test banners"
  echo "  --fail-fast     Stop on first failing test"
  exit 2
}

cleanup_test_artifacts() {
  # Cleanup must never abort the script
  set +e

  local removed=0

  echo
  echo "Cleaning up temporary files..."

  while IFS= read -r -d '' f; do
    rm -f -- "$f"
    removed=$((removed + 1))
  done < <(
    find "$SCRIPT_DIR" -maxdepth 1 -type f \
      \( -iname '*.tmp' -o -iname '*.txt' \) -print0
  )

  echo "Removed $removed temporary file(s)."
}

# Always cleanup (success, failure, fail-fast, Ctrl+C)
trap cleanup_test_artifacts EXIT

while [[ $# -gt 0 ]]; do
  case "$1" in
    -c|--cli) MODE="-c"; shift ;;
    -r|--run) MODE="-r"; shift ;;
    -q|--quiet) QUIET=1; shift ;;
    --fail-fast) FAILFAST=1; shift ;;
    -h|--help) usage ;;
    *) echo "Unknown option: $1"; usage ;;
  esac
done

RUNTIME_ERR_RE='(^|[[:space:]])ERROR at[[:space:]]+[0-9]+:|^\?\?[[:space:]]*$'
SELF_FAILCOUNT_RE='FAIL[[:space:]]*=[[:space:]]*[1-9][0-9]*'
SELF_FAILTEXT_RE='(^|[[:space:]])TEST[[:space:]]+FAIL|(^|[[:space:]])FAILURES?:[[:space:]]*[1-9]'

extract_reason() {
  local log="$1" status="$2" r=""

  r="$(grep -m 1 -E 'ERROR at[[:space:]]+[0-9]+:' "$log" || true)"
  [[ -n "$r" ]] && { printf '%s\n' "$r"; return; }

  r="$(grep -m 1 -E "$SELF_FAILCOUNT_RE" "$log" || true)"
  [[ -n "$r" ]] && { printf '%s\n' "$r"; return; }

  r="$(grep -m 1 -i -E "$SELF_FAILTEXT_RE" "$log" || true)"
  [[ -n "$r" ]] && { printf '%s\n' "$r"; return; }

  r="$(grep -m 1 -E '^\?\?[[:space:]]*$' "$log" || true)"
  [[ -n "$r" ]] && { printf '%s\n' "$r"; return; }

  if [[ "$status" -ne 0 ]]; then
    printf 'Non-zero exit status: %s\n' "$status"
  else
    printf 'Detected failure (pattern match)\n'
  fi
}

mapfile -d '' tests < <(
  find "$TEST_DIR" -maxdepth 1 -type f -name 'WBASIC_TT_*.bas' -print0 | sort -z
)

if (( ${#tests[@]} == 0 )); then
  echo "No tests found in: $TEST_DIR"
  exit 2
fi

echo "=============================================="
echo "WBASIC Torture Test Suite Runner"
echo "Interpreter : $WBASIC"
echo "Test dir    : $TEST_DIR"
echo "Mode        : $MODE"
[[ "$QUIET" -eq 1 ]] && echo "Quiet mode  : ON"
[[ "$FAILFAST" -eq 1 ]] && echo "Fail-fast   : ON"
echo "Tests found : ${#tests[@]}"
echo "=============================================="
echo

PASS=0
FAIL=0
fail_names=()
fail_reasons=()

for test in "${tests[@]}"; do
  name="$(basename "$test")"

  if [[ "$QUIET" -eq 0 ]]; then
    echo "----------------------------------------------"
    echo "Running: $name"
    echo "----------------------------------------------"
  else
    echo "Running: $name"
  fi

  tmp_out="$(mktemp)"
  status=0

  set +e
  "$WBASIC" "$MODE" "$test" >"$tmp_out" 2>&1
  status=$?
  set -e

  cat "$tmp_out"

  failed=0
  if [[ "$status" -ne 0 ]]; then
    failed=1
  elif grep -qE "$RUNTIME_ERR_RE" "$tmp_out"; then
    failed=1
  elif grep -qE "$SELF_FAILCOUNT_RE" "$tmp_out"; then
    failed=1
  elif grep -qiE "$SELF_FAILTEXT_RE" "$tmp_out"; then
    failed=1
  fi

  if [[ "$failed" -eq 1 ]]; then
    FAIL=$((FAIL + 1))
    reason="$(extract_reason "$tmp_out" "$status")"
    fail_names+=( "$name" )
    fail_reasons+=( "$reason" )
    echo "FAILED: $name"
    rm -f "$tmp_out"

    if [[ "$FAILFAST" -eq 1 ]]; then
      echo
      echo "Stopping early due to failure (--fail-fast)."
      break
    fi
  else
    PASS=$((PASS + 1))
    [[ "$QUIET" -eq 0 ]] && echo "Finished: $name"
    rm -f "$tmp_out"
  fi

  [[ "$QUIET" -eq 0 ]] && echo
done

echo "=============================================="
echo "WBASIC torture tests completed."

if [[ "$FAIL" -eq 0 ]]; then
  echo "${INV_GREEN}PASS=$PASS${RESET}  FAIL=$FAIL"
else
  echo "PASS=$PASS  ${INV_RED}FAIL=$FAIL${RESET}"
fi

if [[ "$FAIL" -ne 0 ]]; then
  echo
  echo "Failure Summary"
  echo "----------------------------------------------"
  for i in "${!fail_names[@]}"; do
    echo "${fail_names[$i]}: ${fail_reasons[$i]}"
  done
fi

echo "=============================================="

(( FAIL != 0 )) && exit 1
exit 0

