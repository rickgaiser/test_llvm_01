# Accumulator value checks: whether a sum kept in the R5900 FPU or VU0 ACC
# gives the same bits as separate multiplies and adds (accprobe.c), and how
# a value gets into and out of ACC exactly. Built on maxsdk.
#
#   make [OUT=<dir>]   -> $(OUT)/accprobe.elf
#
# Run it on the real PS2 (ps2-hw-test skill): PCSX2 does not round like it.

HERE   := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
ROOT   := $(abspath $(HERE)/../../../..)
LLVM   ?= $(ROOT)/build/llvm/bin
OUT    ?= $(ROOT)/build/accprobe
MAXSDK ?= $(abspath $(HERE)/../../../../../libs/maxsdk)
MX_OUT ?= $(ROOT)/build/maxsdk/clang

CLANG  = $(LLVM)/clang
CFLAGS = -Wall -O2 -ffreestanding -I$(MAXSDK)/include \
         --target=mips64el -mcpu=r5900 -mabi=n32 -fno-pic -mno-abicalls -G 0 -mvu0
MX_LIB  = $(MX_OUT)/libmx.a
MX_CRT0 = $(MX_OUT)/obj/crt0.o
LINK    = $(LLVM)/ld.lld -m elf32ltsmipn32 -T$(MAXSDK)/ld/maxsdk.ld -z max-page-size=128 \
          --gc-sections

all: $(OUT)/accprobe.elf

$(MX_LIB) $(MX_CRT0): FORCE
	$(MAKE) -C $(MAXSDK) OUT=$(MX_OUT) $(MX_LIB) $(MX_CRT0)

$(OUT)/accprobe.o: $(HERE)/accprobe.c $(HERE)/Makefile
	mkdir -p $(OUT)
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/accprobe.elf: $(OUT)/accprobe.o $(MX_LIB) $(MX_CRT0)
	$(LINK) -Map $(@:.elf=.map) -o $@ $(MX_CRT0) $(OUT)/accprobe.o $(MX_LIB)

clean:
	rm -rf $(OUT)

.PHONY: all clean FORCE
