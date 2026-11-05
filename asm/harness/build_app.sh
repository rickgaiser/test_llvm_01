#!/bin/sh
# Builds app.elf: app.c and the variants of gen_app.py <set>, linked with the
# kernel objects of the bench's asm build (bench.sh build asm). Usage:
#   build_app.sh <set> [out dir]   (default ps2max/build/ps2-hw-test/src/app)
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
BENCH=$(cd "$HERE/../.." && pwd)
PS2MAX=$(cd "$BENCH/../.." && pwd)
OUT=${2:-$PS2MAX/build/ps2-hw-test/src/app}
ASMB=$PS2MAX/build/apps/llvm_bench/asm
CLANG=$PS2MAX/build/toolchain/llvm/feature/bin/clang
GCC=$PS2DEV/ee/bin/mips64r5900el-ps2-elf-gcc
mkdir -p "$OUT"
python3 "$HERE/gen_app.py" "$OUT" "$1"
FLAGS="-D_EE -O2 -I$PS2SDK/ee/include -I$PS2SDK/common/include -I$PS2DEV/ee/mips64r5900el-ps2-elf/include -I$BENCH --target=mips64el -mcpu=r5900 -mabi=n32 -fno-pic -mno-abicalls -G 0 -DBENCH_ASM"
$CLANG $FLAGS -I"$OUT" -c "$HERE/app.c" -o "$OUT/app.o"
$CLANG $FLAGS -mvu0 -c "$OUT/appv.S" -o "$OUT/appv.o"
$GCC -o "$OUT/app.elf" "$OUT/app.o" "$OUT/appv.o" \
  "$ASMB/gfx_tests.o" "$ASMB/sound_tests.o" "$ASMB/emu_tests.o" "$ASMB/game_tests.o" \
  "$ASMB/asm_gfx_tests.o" "$ASMB/asm_sound_tests.o" "$ASMB/asm_emu_tests.o" \
  "$ASMB/asm_game_tests.o" \
  -T$PS2SDK/ee/startup/linkfile -L$PS2SDK/ee/lib -Wl,-zmax-page-size=128
echo "$OUT/app.elf"
