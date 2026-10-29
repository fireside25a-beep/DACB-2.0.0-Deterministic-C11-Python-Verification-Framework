#!/usr/bin/env bash
set -euo pipefail
export PYTHONDONTWRITEBYTECODE=1
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

CFLAGS=(-std=c11 -O2 -Wall -Wextra -Werror -pedantic -I"$ROOT/canonical_c11/include")

echo "[1/14] build gcc"
gcc "${CFLAGS[@]}" "$ROOT/canonical_c11/src/dacb.c" "$ROOT/canonical_c11/src/main.c" -o "$TMP/dacb-gcc"
gcc "${CFLAGS[@]}" "$ROOT/canonical_c11/src/dacb.c" "$ROOT/tests/test_c.c" -o "$TMP/test-gcc"

echo "[2/14] build clang"
clang "${CFLAGS[@]}" "$ROOT/canonical_c11/src/dacb.c" "$ROOT/canonical_c11/src/main.c" -o "$TMP/dacb-clang"
clang "${CFLAGS[@]}" "$ROOT/canonical_c11/src/dacb.c" "$ROOT/tests/test_c.c" -o "$TMP/test-clang"

echo "[3/14] C tests gcc/clang"
"$TMP/test-gcc"
"$TMP/test-clang"

echo "[4/14] independent Python twin tests"
python3 "$ROOT/tests/test_python_twin.py"

echo "[5/14] canonical report byte identity"
"$TMP/dacb-gcc" report > "$TMP/gcc.report"
"$TMP/dacb-clang" report > "$TMP/clang.report"
python3 "$ROOT/python_twin/dacb_twin.py" report > "$TMP/python.report"
cmp "$TMP/gcc.report" "$TMP/clang.report"
cmp "$TMP/gcc.report" "$TMP/python.report"
grep -qx 'status=PASS' "$TMP/gcc.report"
sha256sum "$TMP/gcc.report"

echo "[6/14] 5000-step CLI byte identity"
"$TMP/dacb-gcc" run 5000 > "$TMP/gcc.5000"
"$TMP/dacb-clang" run 5000 > "$TMP/clang.5000"
python3 "$ROOT/python_twin/dacb_twin.py" run 5000 > "$TMP/python.5000"
cmp "$TMP/gcc.5000" "$TMP/clang.5000"
cmp "$TMP/gcc.5000" "$TMP/python.5000"
cat "$TMP/gcc.5000"

echo "[7/14] bounded exhaustive cross-language identity bits 2..11"
"$TMP/dacb-gcc" exhaustive 2 11 > "$TMP/c.ex11"
python3 "$ROOT/python_twin/dacb_twin.py" exhaustive 2 11 > "$TMP/py.ex11"
cmp "$TMP/c.ex11" "$TMP/py.ex11"
cat "$TMP/c.ex11"

echo "[8/14] extended C exhaustive bits 2..12"
"$TMP/dacb-gcc" exhaustive 2 12

echo "[9/14] long-chain stress"
"$TMP/dacb-gcc" run 1000000 > "$TMP/gcc.1m"
"$TMP/dacb-clang" run 1000000 > "$TMP/clang.1m"
python3 "$ROOT/python_twin/dacb_twin.py" run 1000000 > "$TMP/python.1m"
cmp "$TMP/gcc.1m" "$TMP/clang.1m"
cmp "$TMP/gcc.1m" "$TMP/python.1m"
cat "$TMP/gcc.1m"

echo "[10/14] sanitizer build"
clang -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic -I"$ROOT/canonical_c11/include" \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  "$ROOT/canonical_c11/src/dacb.c" "$ROOT/tests/test_safety.c" -o "$TMP/test-san"
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 "$TMP/test-san"

echo "[11/14] gcc analyzer"
gcc -std=c11 -O0 -Wall -Wextra -Werror -pedantic -fanalyzer -I"$ROOT/canonical_c11/include" \
  "$ROOT/canonical_c11/src/dacb.c" "$ROOT/canonical_c11/src/main.c" -o "$TMP/dacb-analyzer"

echo "[12/14] invalid-input rejection"
python3 "$ROOT/tests/test_cli_contract.py" "$TMP/dacb-gcc"
if "$TMP/dacb-gcc" run nope >/dev/null 2>&1; then exit 1; fi
if "$TMP/dacb-gcc" exhaustive 1 10 >/dev/null 2>&1; then exit 1; fi
if "$TMP/dacb-gcc" lattice bad >/dev/null 2>&1; then exit 1; fi

echo "[13/14] repeatability"
"$TMP/dacb-gcc" report > "$TMP/baseline.report"
for i in $(seq 1 20); do
  "$TMP/dacb-gcc" report > "$TMP/repeat.report"
  cmp "$TMP/baseline.report" "$TMP/repeat.report"
done

echo "[14/14] source-tree hygiene"
if find "$ROOT" -type f \( -name '*.o' -o -name '*.pyc' -o -name '*.exe' -o -name 'core' \) | grep -q .; then
  echo "unexpected build artifact in source tree" >&2
  exit 1
fi
if find "$ROOT" -type d -name '__pycache__' | grep -q .; then
  echo "unexpected __pycache__ in source tree" >&2
  exit 1
fi

echo "DACB_RELEASE_VERIFICATION_PASS"
