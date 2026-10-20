#!/bin/bash
# Compare gcc against the upstream/pr/feature clang builds.
#
# Usage:
#   ./bench.sh build   [variant...]   build the benchmark (default: all variants)
#   ./bench.sh codegen [variant...]   code size per function, compared to gcc (no hardware)
#   ./bench.sh run     <variant>      run on the PS2 and save the log (PS2 must be freshly booted)
#   ./bench.sh report                 results table from the saved logs
#
# Variants: gcc upstream pr feature
# Output:   toolchain/build/bench/<variant>/{*.o,*.s,test.elf,run.log}

set -e

HERE=$(cd "$(dirname "$0")" && pwd)
BUILD_ROOT=$(cd "$HERE/../../../build" && pwd)
OUT_ROOT=$BUILD_ROOT/bench
PS2_IP=${PS2_IP:-192.168.1.10}
ALL_VARIANTS="gcc upstream pr feature"

export PS2DEV=${PS2DEV:-/home/rgaiser/dev/ps2max/install}
export PS2SDK=${PS2SDK:-$PS2DEV/ps2sdk}

# Build directory of the clang used for a variant
clang_dir() {
  case "$1" in
    feature) echo "$BUILD_ROOT/llvm" ;;
    pr|upstream) echo "$BUILD_ROOT/llvm-$1" ;;
  esac
}

# Tool used to inspect objects (any recent llvm-size/objdump will do)
llvm_tool() {
  local v
  for v in feature upstream pr; do
    if [ -x "$(clang_dir $v)/bin/$1" ]; then
      echo "$(clang_dir $v)/bin/$1"
      return
    fi
  done
  echo "$1"
}

cmd_build() {
  local v
  for v in ${@:-$ALL_VARIANTS}; do
    local out=$OUT_ROOT/$v
    rm -rf "$out"
    if [ "$v" = gcc ]; then
      make -s -C "$HERE" COMPILER=gcc OUT="$out" > "$OUT_ROOT/$v.build.log" 2>&1 || true
    else
      local clang
      clang=$(clang_dir "$v")/bin/clang
      if [ ! -x "$clang" ]; then
        echo "$v: SKIP (no $clang)"
        continue
      fi
      mkdir -p "$OUT_ROOT"
      make -s -C "$HERE" COMPILER=clang CLANG="$clang" OUT="$out" > "$OUT_ROOT/$v.build.log" 2>&1 || true
    fi
    if [ -f "$out/test.elf" ]; then
      echo "$v: OK ($out/test.elf)"
    else
      echo "$v: FAILED (see $OUT_ROOT/$v.build.log)"
    fi
  done
}

# Print "<symbol> <size>" for all functions in a variant's objects
func_sizes() {
  "$(llvm_tool llvm-nm)" --print-size --size-sort --defined-only "$OUT_ROOT/$1"/*.o 2>/dev/null |
    awk '$3 ~ /^[Tt]$/ { printf "%s %d\n", $4, strtonum("0x" $2) }' | sort
}

cmd_codegen() {
  local variants=${@:-$ALL_VARIANTS}
  local tmp v present=""
  tmp=$(mktemp -d)
  for v in $variants; do
    if ls "$OUT_ROOT/$v"/*.o >/dev/null 2>&1; then
      func_sizes "$v" > "$tmp/$v"
      present="$present $v"
    fi
  done
  [ -n "$present" ] || { echo "Nothing built, run: $0 build"; rm -rf "$tmp"; exit 1; }

  # Table: function, size per variant (bytes)
  printf "%-36s" "function (bytes)"
  for v in $present; do printf "%10s" "$v"; done
  echo
  cat "$tmp"/* | awk '{ print $1 }' | sort -u | while read -r f; do
    printf "%-36s" "$f"
    for v in $present; do
      printf "%10s" "$(awk -v f="$f" '$1 == f { print $2 }' "$tmp/$v")"
    done
    echo
  done
  printf "%-36s" "TOTAL .text"
  for v in $present; do
    printf "%10s" "$(awk '{ s += $2 } END { print s }' "$tmp/$v")"
  done
  echo
  rm -rf "$tmp"
  echo
  echo "Assembly: $OUT_ROOT/<variant>/*.s  (e.g. diff -u $OUT_ROOT/upstream/fpu_tests.s $OUT_ROOT/feature/fpu_tests.s)"
}

cmd_run() {
  local v=$1
  [ -n "$v" ] || { echo "Usage: $0 run <variant>"; exit 1; }
  local out=$OUT_ROOT/$v
  [ -f "$out/test.elf" ] || { echo "$out/test.elf missing, run: $0 build $v"; exit 1; }
  # ps2client serves host: from its working directory
  cd "$out"
  timeout 180 ps2client -h "$PS2_IP" -t 15 execee host:test.elf 2>&1 | tr -d '\r' | tee run.log
  if grep -q "=== Test Complete ===" run.log; then
    echo "$v: run complete ($out/run.log)"
  else
    echo "$v: run INCOMPLETE (no '=== Test Complete ===' in $out/run.log)"
    exit 1
  fi
}

# Print "<section>/<name>|<value>|<unit>" from the summary block of a log
summary() {
  awk '
    /^=== Summary ===/ { on = 1; next }
    /^=== Test Complete ===/ { on = 0 }
    on && /^[A-Z]+:$/ { sect = substr($0, 1, length($0) - 1); next }
    on && /:/ {
      split($0, a, ":"); name = a[1]; gsub(/^ +| +$/, "", name)
      n = split(a[2], b, " "); printf "%s/%s|%s|%s\n", sect, name, b[1], b[2]
    }' "$1"
}

# Print the "First..."/"Result..." lines, used as a correctness check
results() {
  grep -E '^(First|Last|Result)' "$1" || true
}

# Pad the columns of a markdown table (first column left, others right aligned)
align_table() {
  awk -F'|' '
    { for (i = 2; i < NF; i++) { c = $i; gsub(/^ +| +$/, "", c); cell[NR, i] = c
        if (c !~ /^-+:?$/ && length(c) > w[i]) w[i] = length(c) } ; nf[NR] = NF }
    END {
      for (r = 1; r <= NR; r++) {
        line = "|"
        for (i = 2; i < nf[r]; i++) {
          c = cell[r, i]
          if (c ~ /^-+:?$/) { d = sprintf("%" w[i] "s", ""); gsub(/ /, "-", d); line = line (i == 2 ? "-" d "-" : "-" substr(d, 2) "-:") "|" }
          else line = line " " sprintf(i == 2 ? "%-" w[i] "s" : "%" w[i] "s", c) " |"
        }
        print line
      }
    }'
}

# Markdown table of the summary lines of the given variants' run logs
report_table() {
  local present="$*" v
  printf "| Test |"
  for v in $present; do printf " %s |" "$v"; done
  [ -f "$OUT_ROOT/gcc/run.log" ] && for v in $present; do [ "$v" != gcc ] && printf " %s vs gcc |" "$v"; done
  echo
  printf "|---|"
  for v in $present; do printf -- "---:|"; done
  [ -f "$OUT_ROOT/gcc/run.log" ] && for v in $present; do [ "$v" != gcc ] && printf -- "---:|"; done
  echo

  local first
  first=$(echo $present | awk '{ print $1 }')
  summary "$OUT_ROOT/$first/run.log" | while IFS='|' read -r key _ unit; do
    printf "| %s (%s) |" "$key" "$unit"
    local gccval="" val
    [ -f "$OUT_ROOT/gcc/run.log" ] && gccval=$(summary "$OUT_ROOT/gcc/run.log" | awk -F'|' -v k="$key" '$1 == k { print $2 }')
    for v in $present; do
      printf " %s |" "$(summary "$OUT_ROOT/$v/run.log" | awk -F'|' -v k="$key" '$1 == k { print $2 }')"
    done
    if [ -n "$gccval" ]; then
      for v in $present; do
        [ "$v" = gcc ] && continue
        val=$(summary "$OUT_ROOT/$v/run.log" | awk -F'|' -v k="$key" '$1 == k { print $2 }')
        awk -v a="$val" -v g="$gccval" 'BEGIN { if (a == "" || g == 0) printf " - |"; else printf " %+.0f%% |", (a / g - 1) * 100 }'
      done
    fi
    echo
  done
}

cmd_report() {
  local v present=""
  for v in $ALL_VARIANTS; do
    [ -f "$OUT_ROOT/$v/run.log" ] && present="$present $v"
  done
  [ -n "$present" ] || { echo "No run logs in $OUT_ROOT/*/run.log"; exit 1; }

  report_table $present | align_table

  if [ -f "$OUT_ROOT/gcc/run.log" ]; then
    echo
    for v in $present; do
      [ "$v" = gcc ] && continue
      if diff -q <(results "$OUT_ROOT/gcc/run.log") <(results "$OUT_ROOT/$v/run.log") >/dev/null; then
        echo "$v: results match gcc"
      else
        echo "$v: results DIFFER from gcc:"
        diff <(results "$OUT_ROOT/gcc/run.log") <(results "$OUT_ROOT/$v/run.log") | sed 's/^/    /'
      fi
    done
  fi
}

mkdir -p "$OUT_ROOT"
cmd=$1
shift || true
case "$cmd" in
  build) cmd_build "$@" ;;
  codegen) cmd_codegen "$@" ;;
  run) cmd_run "$@" ;;
  report) cmd_report ;;
  *) sed -n '2,11p' "$0" | sed 's/^# \{0,1\}//'; exit 1 ;;
esac
