# FPU + VU0 + VU1 demo: one polynomial, written in vuk for the VUs and in C
# for the EE. Built on maxsdk (generic/llvm/maxsdk), no ps2sdk.
#
#   make [D=16] [OUT=<dir>]   -> $(OUT)/vudemo_{macro,micro,all,bench,debug}.elf
#
# Run each ELF with the ps2-pcsx2-test skill (correctness only).

HERE   := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
ROOT   := $(abspath $(HERE)/../../../..)
LLVM   ?= $(ROOT)/build/llvm/bin
OUT    ?= $(ROOT)/build/vudemo
MAXSDK ?= $(abspath $(HERE)/../../maxsdk)
MX_OUT ?= $(ROOT)/build/maxsdk/clang
D      ?= 16
KERNEL ?= poly4   # poly4.vuk (4 degrees per step) or poly.vuk (plain Horner)

CLANG   = $(LLVM)/clang
LLC     = $(LLVM)/llc
OBJCOPY = $(LLVM)/llvm-objcopy
VUK     = python3 -I $(HERE)/../vuk/vuk.py

CFLAGS = -Wall -O3 -ffast-math -ffreestanding -I$(MAXSDK)/include -I$(OUT) -I$(HERE) \
         --target=mips64el -mcpu=r5900 -mabi=n32 -fno-pic -mno-abicalls -G 0 \
         -ffp-contract=fast -fomit-frame-pointer $(EXTRA_CFLAGS)
# The kernels keep the 4-degree grouping of the source: reassociation turns it
# into 2-degree groups, a chain of 8 instead of 4 dependent multiply-adds.
KERNEL_CFLAGS = $(CFLAGS) -fno-associative-math
# The FPU part must stay scalar FPU code.
FPU_CFLAGS = $(KERNEL_CFLAGS) -fno-vectorize -fno-slp-vectorize
LLCFLAGS = -mtriple=vu -O3
MX_LIB  = $(MX_OUT)/libmx.a
MX_CRT0 = $(MX_OUT)/obj/crt0.o
LINK    = $(LLVM)/ld.lld -m elf32ltsmipn32 -T$(MAXSDK)/ld/maxsdk.ld -z max-page-size=128 \
          --gc-sections

MODES = macro micro all bench debug
ELFS  = $(MODES:%=$(OUT)/vudemo_%.elf)

all: $(ELFS) $(OUT)/mulprobe.elf $(OUT)/vu0_poly.s $(OUT)/vu1_poly.s

$(OUT):
	mkdir -p $@

# maxsdk itself (its own Makefile knows when to rebuild)
$(MX_LIB) $(MX_CRT0): FORCE
	$(MAKE) -C $(MAXSDK) OUT=$(MX_OUT) $(MX_LIB) $(MX_CRT0)

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

$(OUT)/demo_%.o: $(HERE)/demo.c $(HERE)/demo.h $(OUT)/poly.h $(OUT)/expected.h
	$(CLANG) $(CFLAGS) -DMODE_$(shell echo $* | tr a-z A-Z) -c $< -o $@

# The bench run with every step printed, and 200 ms between steps.
$(OUT)/demo_debug.o: $(HERE)/demo.c $(HERE)/demo.h $(OUT)/poly.h $(OUT)/expected.h
	$(CLANG) $(CFLAGS) -DMODE_BENCH -DVUDEMO_DEBUG=200 -c $< -o $@

$(OUT)/vudemo_%.elf: $(OUT)/demo_%.o $(OUT)/fpu.o $(OUT)/macro.o $(OUT)/blobs.o $(MX_LIB) $(MX_CRT0)
	$(LINK) -Map $(@:.elf=.map) -o $@ $(MX_CRT0) $(filter-out $(MX_CRT0),$(filter %.o,$^)) $(MX_LIB)

# Hardware check: is the multiplier exact in both operand orders?
$(OUT)/mulprobe.o: $(HERE)/mulprobe.c | $(OUT)
	$(CLANG) $(CFLAGS) -O1 -fno-fast-math -ffinite-math-only -mvu0 -c $< -o $@

$(OUT)/mulprobe.elf: $(OUT)/mulprobe.o $(MX_LIB) $(MX_CRT0)
	$(LINK) -Map $(@:.elf=.map) -o $@ $(MX_CRT0) $< $(MX_LIB)

clean:
	rm -rf $(OUT)

.PHONY: all clean FORCE
.SECONDARY:
