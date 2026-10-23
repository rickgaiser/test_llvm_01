#!/bin/bash
# Checks for vuk: the examples compile for VU0 and VU1, and bad programs give
# the expected errors.
#
#   ./test.sh [<llvm bin dir>]     (default: toolchain/build/llvm/bin)

HERE=$(cd "$(dirname "$0")" && pwd)
LLVM=${1:-$(cd "$HERE/../../../../build/llvm/bin" && pwd)}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
FAILS=0

fail() { echo "FAIL: $*"; FAILS=$((FAILS + 1)); }

# Examples: vuk -> llc for both units, with the machine verifier.
for f in "$HERE"/*.vuk; do
  for d in 1 4 16; do
    python3 -I "$HERE/vuk.py" "$f" -D D=$d -o "$TMP/k.ll" -H "$TMP/k.h" || { fail "$f D=$d: vuk"; continue; }
    for cpu in vu0 vu1; do
      "$LLVM/llc" -mtriple=vu -mcpu=$cpu -O3 -verify-machineinstrs "$TMP/k.ll" -o /dev/null 2> "$TMP/err" ||
        fail "$(basename "$f") D=$d $cpu: llc: $(head -1 "$TMP/err")"
    done
  done
done

# Errors: each case is "<program>|<expected message part>".
while IFS='|' read -r prog msg; do
  [ -z "$prog" ] && continue
  printf '%s\n' "$prog" > "$TMP/e.vuk"
  out=$(python3 -I "$HERE/vuk.py" "$TMP/e.vuk" -o /dev/null 2>&1) && { fail "accepted: $prog"; continue; }
  case "$out" in *"$msg"*) ;; *) fail "$prog: got '$out', want '$msg'" ;; esac
done <<'EOF'
kernel k(x: f32x4[], n: i16) { }|an unsized array must be the last parameter
kernel k(c: f32[4], n: i16) { for i in 0..n { var y = c[i]; } }|needs a constant index
kernel k(n: i16) { y = 1; }|cannot assign to y
kernel k(n: i16) { var y = z; }|unknown name z
kernel k(n: i16) { var y = n / 2; }|integer division is not supported
kernel k(n: i16 { }|expected ,
kernel k(n: u8) { }|unknown type u8
EOF

# The layout header matches the documented layout.
python3 -I "$HERE/vuk.py" "$HERE/poly.vuk" -D D=16 -o /dev/null -H "$TMP/p.h"
for want in "VUK_POLY_N 0" "VUK_POLY_C 1" "VUK_POLY_X 6" "VUK_D 16"; do
  grep -qE "#define $want( |$)" "$TMP/p.h" || fail "layout: missing $want"
done

if [ $FAILS -eq 0 ]; then echo "vuk: all checks passed"; else echo "vuk: $FAILS failed"; exit 1; fi
