# The IOP bench: every kernel with gcc and clang, -Os and -O2, and by hand. gcc is
# the IOP gcc, with ps2sdk's flags; needs envsetup.sh for its paths.
#
#   make              $(OUT)/iop_bench.irx
#   make sim          each variant in a MIPS I interpreter (load delays included)
#   make sizes        code size of each kernel and variant
#
# Run with maxsdk's probe/p25_iop.elf, see README.md.

PS2MAX ?= $(abspath $(CURDIR)/../../..)
OUT    ?= $(PS2MAX)/build/apps/llvm_bench/iop
LLVM   ?= $(PS2MAX)/build/toolchain/llvm/feature/bin
ifndef PS2MAX_IOP_BIN
$(error source envsetup.sh first: PS2MAX_IOP_BIN is not set)
endif

GCC     = $(PS2MAX_IOP_BIN)/mipsel-none-elf-gcc
CLANG   = $(LLVM)/clang --target=mipsel-none-elf -mcpu=r3000a
INCS    = -I$(PS2MAX_IOP_SDK)/iop/include -I$(PS2MAX_IOP_SDK)/common/include -I.
# ps2sdk's iop/Rules.make
GFLAGS  = -D_IOP -fno-builtin -G0 -Wall -msoft-float -mno-explicit-relocs $(INCS)
# tools/mxlink's IOP_CLANG
CFLAGS  = -D_IOP -fno-builtin -ffreestanding -G0 -Wall -msoft-float -mno-abicalls -fno-pic \
          -fno-unwind-tables -fno-asynchronous-unwind-tables $(INCS)

KOBJS   = $(OUT)/k_gos.o $(OUT)/k_go2.o $(OUT)/k_cos.o $(OUT)/k_co2.o $(OUT)/k_cor.o $(OUT)/kernels_asm.o $(OUT)/tables.o
OBJS    = $(OUT)/main.o $(KOBJS) $(OUT)/imports.o

all: $(OUT)/iop_bench.irx

$(OUT)/k_gos.o: kernels.c kernels.h
	@mkdir -p $(OUT)
	$(GCC) $(GFLAGS) -Os -DV=gos_ -c $< -o $@
$(OUT)/k_go2.o: kernels.c kernels.h
	@mkdir -p $(OUT)
	$(GCC) $(GFLAGS) -O2 -DV=go2_ -c $< -o $@
$(OUT)/k_cos.o: kernels.c kernels.h $(LLVM)/clang
	@mkdir -p $(OUT)
	$(CLANG) $(CFLAGS) -Os -DV=cos_ -c $< -o $@
$(OUT)/k_co2.o: kernels.c kernels.h $(LLVM)/clang
	@mkdir -p $(OUT)
	$(CLANG) $(CFLAGS) -O2 -DV=co2_ -c $< -o $@
$(OUT)/k_cor.o: kernels.c kernels.h $(LLVM)/clang
	@mkdir -p $(OUT)
	$(CLANG) $(CFLAGS) -O2 -DR=restrict -DV=cor_ -c $< -o $@
$(OUT)/kernels_asm.o: kernels_asm.S
	@mkdir -p $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@
$(OUT)/tables.o: tables.c kernels.h
	@mkdir -p $(OUT)
	$(CLANG) $(CFLAGS) -O2 -c $< -o $@
$(OUT)/main.o: main.c kernels.h irx_imports.h
	@mkdir -p $(OUT)
	$(CLANG) $(CFLAGS) -O2 -c $< -o $@
$(OUT)/imports.o: imports.lst irx_imports.h
	@mkdir -p $(OUT)
	{ echo '#include "irx_imports.h"'; cat $<; } > $(@:.o=.c)
	$(CLANG) $(CFLAGS) -O2 -c $(@:.o=.c) -o $@

$(OUT)/iop_bench.elf: $(OBJS)
	$(GCC) -T$(PS2MAX_IOP_SDK)/iop/startup/linkfile -o $@ $^ -nostdlib -dc -r

$(OUT)/iop_bench.irx: $(OUT)/iop_bench.elf
	$(PS2MAX_IOP_BIN)/mipsel-none-elf-strip --strip-unneeded --remove-section=.pdr \
	  --remove-section=.comment --remove-section=.mdebug.abi32 --remove-section=.gnu.attributes \
	  -o $(@:.irx=.stripped.elf) $<
	$(PS2MAX_IOP_SDK)/bin/iopfixup --irx1 -o $@ $(@:.irx=.stripped.elf)

# linked at a fixed address for the interpreter
$(OUT)/sim.elf: $(KOBJS)
	$(PS2MAX_IOP_BIN)/mipsel-none-elf-ld -e 0 -Ttext=0x10000 -o $@ $^

sim: $(OUT)/sim.elf
	python3 sim.py $< $(PS2MAX_IOP_BIN)/mipsel-none-elf-

sizes: $(OUT)/sim.elf
	$(PS2MAX_IOP_BIN)/mipsel-none-elf-nm -S $< > $(OUT)/sizes.txt

.PHONY: all sim sizes
