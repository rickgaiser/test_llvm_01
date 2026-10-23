# FPU + VU0 + VU1 demo: one polynomial, written in vuk for the VUs and in C
# for the EE.
#
#   make [D=16] [OUT=<dir>]   -> $(OUT)/vudemo_{macro,micro,all}.elf
#
# Run each ELF with the ps2-pcsx2-test skill (correctness only).

HERE  := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
ROOT  := $(abspath $(HERE)/../../../..)
LLVM  ?= $(ROOT)/build/llvm/bin
OUT   ?= $(ROOT)/build/vudemo
D     ?= 16
KERNEL ?= poly4   # poly4.vuk (4 degrees per step) or poly.vuk (plain Horner)

PS2DEV ?= /home/rgaiser/dev/ps2max/install
PS2SDK ?= $(PS2DEV)/ps2sdk
EE_GCC  = $(PS2DEV)/ee/bin/mips64r5900el-ps2-elf-gcc
CLANG   = $(LLVM)/clang
LLC     = $(LLVM)/llc
OBJCOPY = $(LLVM)/llvm-objcopy
VUK     = python3 -I $(HERE)/../vuk/vuk.py

CFLAGS = -D_EE -Wall -O3 -ffast-math -I$(PS2SDK)/ee/include -I$(PS2SDK)/common/include \
         -I$(PS2DEV)/ee/mips64r5900el-ps2-elf/include -I$(OUT) -I$(HERE) \
         --target=mips64el -mcpu=r5900 -mabi=n32 -fno-pic -mno-abicalls -G 0 \
         -ffp-contract=fast -fomit-frame-pointer
# The kernels keep the 4-degree grouping of the source: reassociation turns it
# into 2-degree groups, a chain of 8 instead of 4 dependent multiply-adds.
KERNEL_CFLAGS = $(CFLAGS) -fno-associative-math
# The FPU part must stay scalar FPU code.
FPU_CFLAGS = $(KERNEL_CFLAGS) -fno-vectorize -fno-slp-vectorize
LDFLAGS = -T$(PS2SDK)/ee/startup/linkfile -L$(PS2SDK)/ee/lib -Wl,-zmax-page-size=128
LLCFLAGS = -mtriple=vu -O3

MODES = macro micro all bench debug
ELFS  = $(MODES:%=$(OUT)/vudemo_%.elf)

all: $(ELFS) $(OUT)/mulprobe.elf $(OUT)/vu0_poly.s $(OUT)/vu1_poly.s

$(OUT):
	mkdir -p $@

$(OUT)/poly.ll $(OUT)/poly.h: $(HERE)/../vuk/$(strip $(KERNEL)).vuk $(HERE)/../vuk/vuk.py | $(OUT)
	$(VUK) $< -D D=$(D) -o $(OUT)/poly.ll -H $(OUT)/poly.h

$(OUT)/vu%_poly.o: $(OUT)/poly.ll
	$(LLC) $(LLCFLAGS) -mcpu=vu$* -filetype=obj $< -o $@

$(OUT)/vu%_poly.s: $(OUT)/poly.ll
	$(LLC) $(LLCFLAGS) -mcpu=vu$* $< -o $@

$(OUT)/vu%_poly.bin: $(OUT)/vu%_poly.o
	$(OBJCOPY) -O binary --only-section=.vutext $< $@

$(OUT)/blobs.S: $(OUT)/vu0_poly.bin $(OUT)/vu1_poly.bin
	printf '%s\n' '.section .rodata' \
	  '.balign 16' '.globl vu0_poly' 'vu0_poly:' '.incbin "$(OUT)/vu0_poly.bin"' \
	  '.globl vu0_poly_end' 'vu0_poly_end:' \
	  '.balign 16' '.globl vu1_poly' 'vu1_poly:' '.incbin "$(OUT)/vu1_poly.bin"' \
	  '.globl vu1_poly_end' 'vu1_poly_end:' > $@

$(OUT)/expected.h: $(HERE)/ref.c $(HERE)/demo.h $(OUT)/poly.h
	cc -O2 -I$(OUT) -I$(HERE) $< -o $(OUT)/ref
	$(OUT)/ref > $@
	$(OUT)/ref -v > $(OUT)/expected_vals.h

$(OUT)/blobs.o: $(OUT)/blobs.S
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/fpu.o: $(HERE)/fpu.c $(HERE)/demo.h $(OUT)/poly.h
	$(CLANG) $(FPU_CFLAGS) -c $< -o $@
	$(CLANG) $(FPU_CFLAGS) -S $< -o $(OUT)/fpu.s

$(OUT)/macro.o: $(HERE)/macro.c $(HERE)/demo.h $(OUT)/poly.h
	$(CLANG) $(KERNEL_CFLAGS) -mvu0 -c $< -o $@
	$(CLANG) $(KERNEL_CFLAGS) -mvu0 -S $< -o $(OUT)/macro.s

$(OUT)/demo_%.o: $(HERE)/demo.c $(HERE)/demo.h $(HERE)/vu.h $(OUT)/poly.h $(OUT)/expected.h
	$(CLANG) $(CFLAGS) -DMODE_$(shell echo $* | tr a-z A-Z) -c $< -o $@

# The bench run with every step printed, and 200 ms between steps.
$(OUT)/demo_debug.o: $(HERE)/demo.c $(HERE)/demo.h $(HERE)/vu.h $(OUT)/poly.h $(OUT)/expected.h
	$(CLANG) $(CFLAGS) -DMODE_BENCH -DVUDEMO_DEBUG=200 -c $< -o $@

$(OUT)/vudemo_%.elf: $(OUT)/demo_%.o $(OUT)/fpu.o $(OUT)/macro.o $(OUT)/blobs.o
	$(EE_GCC) -o $@ $^ $(LDFLAGS) -ldma

# Hardware check: is the multiplier exact in both operand orders?
$(OUT)/mulprobe.o: $(HERE)/mulprobe.c | $(OUT)
	$(CLANG) $(CFLAGS) -O1 -fno-fast-math -mvu0 -c $< -o $@

$(OUT)/mulprobe.elf: $(OUT)/mulprobe.o
	$(EE_GCC) -o $@ $^ $(LDFLAGS) -ldma

clean:
	rm -rf $(OUT)

.PHONY: all clean
.SECONDARY:
