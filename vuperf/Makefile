# VU micro-mode code generation benchmarks: small compute kernels (kernels/
# *.ll, VUEntryABI.md) compiled by llc for VU0, timed on the PS2 in VU
# cycles per item (vuperf.c).
#
#   make [OUT=<dir>] [LLCFLAGS=...]   -> $(OUT)/vuperf.elf, $(OUT)/<kernel>.s
#
# Timings only from the real PS2 (ps2-hw-test skill), never from PCSX2.

HERE   := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
ROOT   := $(abspath $(HERE)/../../../..)
LLVM   ?= $(ROOT)/build/llvm/bin
OUT    ?= $(ROOT)/build/vuperf
MAXSDK ?= $(abspath $(HERE)/../../../../../libs/maxsdk)
MX_OUT ?= $(ROOT)/build/maxsdk/clang

CLANG    = $(LLVM)/clang
LLC      = $(LLVM)/llc
OBJCOPY  = $(LLVM)/llvm-objcopy
LLCFLAGS ?= -O3
CFLAGS   = -Wall -O2 -ffreestanding -I$(MAXSDK)/include -I$(OUT) \
           --target=mips64el -mcpu=r5900 -mabi=n32 -fno-pic -mno-abicalls -G 0
MX_LIB   = $(MX_OUT)/libmx.a
MX_CRT0  = $(MX_OUT)/obj/crt0.o
LINK     = $(LLVM)/ld.lld -m elf32ltsmipn32 -T$(MAXSDK)/ld/maxsdk.ld -z max-page-size=128 \
           --gc-sections

KERNELS := $(basename $(notdir $(wildcard $(HERE)/kernels/*.ll)))

all: $(OUT)/vuperf.elf

$(MX_LIB) $(MX_CRT0): FORCE
	$(MAKE) -C $(MAXSDK) OUT=$(MX_OUT) $(MX_LIB) $(MX_CRT0)

$(OUT)/%.vu.elf: $(HERE)/kernels/%.ll $(LLC) $(HERE)/Makefile
	@mkdir -p $(OUT)
	$(LLC) -mtriple=vu -mcpu=vu0 $(LLCFLAGS) -filetype=obj $< -o $@
	$(LLC) -mtriple=vu -mcpu=vu0 $(LLCFLAGS) $< -o $(OUT)/$*.s

$(OUT)/%.h: $(OUT)/%.vu.elf
	$(OBJCOPY) --dump-section .vu.header=$@ $< /dev/null

$(OUT)/%.bin: $(OUT)/%.vu.elf
	$(OBJCOPY) -O binary --only-section=.vutext $< $@

$(OUT)/kernels.S $(OUT)/kernels.h: $(KERNELS:%=$(OUT)/%.bin) $(KERNELS:%=$(OUT)/%.h) $(HERE)/table.py
	python3 -I $(HERE)/table.py $(OUT) $(KERNELS)

$(OUT)/kernels.o: $(OUT)/kernels.S
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/vuperf.o: $(HERE)/vuperf.c $(OUT)/kernels.h $(HERE)/Makefile
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/vuperf.elf: $(OUT)/vuperf.o $(OUT)/kernels.o $(MX_LIB) $(MX_CRT0)
	$(LINK) -Map $(@:.elf=.map) -o $@ $(MX_CRT0) $(OUT)/vuperf.o $(OUT)/kernels.o $(MX_LIB)

clean:
	rm -rf $(OUT)

.PHONY: all clean FORCE
.SECONDARY:
