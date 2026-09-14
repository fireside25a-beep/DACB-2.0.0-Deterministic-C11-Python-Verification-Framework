#!/usr/bin/env bash
set -euo pipefail
export PYTHONDONTWRITEBYTECODE=1
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
cc -std=c11 -O3 -Wall -Wextra -Werror -pedantic -I"$ROOT/canonical_c11/include" "$ROOT/canonical_c11/src/dacb.c" "$ROOT/canonical_c11/src/main.c" -o "$TMP/dacb"
echo "Runtime timings are environment-dependent and are not correctness evidence."
for n in 5000 100000 1000000; do
  /usr/bin/time -f "C steps=$n elapsed=%e s" "$TMP/dacb" run "$n" >/dev/null
  /usr/bin/time -f "Python twin steps=$n elapsed=%e s" python3 "$ROOT/python_twin/dacb_twin.py" run "$n" >/dev/null
done
