# VU micro-mode microbenchmarks: latencies and throughputs of VU0/VU1 micro
# instructions, measured on the PS2 (gen.py, vuuarch.c). Built on maxsdk.
#
#   make [OUT=<dir>]   -> $(OUT)/vuuarch.elf
#
# Timings only from the real PS2 (ps2-hw-test skill), never from PCSX2.

HERE   := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
ROOT   := $(abspath $(HERE)/../../../..)
LLVM   ?= $(ROOT)/build/llvm/bin
OUT    ?= $(ROOT)/build/vuuarch
MAXSDK ?= $(abspath $(HERE)/../../maxsdk)
MX_OUT ?= $(ROOT)/build/maxsdk/clang

CLANG  = $(LLVM)/clang
CFLAGS = -Wall -O2 -ffreestanding -I$(MAXSDK)/include -I$(OUT) \
         --target=mips64el -mcpu=r5900 -mabi=n32 -fno-pic -mno-abicalls -G 0
MX_LIB  = $(MX_OUT)/libmx.a
MX_CRT0 = $(MX_OUT)/obj/crt0.o
LINK    = $(LLVM)/ld.lld -m elf32ltsmipn32 -T$(MAXSDK)/ld/maxsdk.ld -z max-page-size=128 \
          --gc-sections

all: $(OUT)/vuuarch.elf

$(MX_LIB) $(MX_CRT0): FORCE
	$(MAKE) -C $(MAXSDK) OUT=$(MX_OUT) $(MX_LIB) $(MX_CRT0)

$(OUT)/probes.h $(OUT)/probes.S: $(HERE)/gen.py $(LLVM)/llvm-mc
	mkdir -p $(OUT)
	python3 -I $(HERE)/gen.py $(OUT) $(LLVM)

$(OUT)/probes.o: $(OUT)/probes.S
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/vuuarch.o: $(HERE)/vuuarch.c $(OUT)/probes.h $(HERE)/Makefile
	$(CLANG) $(CFLAGS) -c $< -o $@

$(OUT)/vuuarch.elf: $(OUT)/vuuarch.o $(OUT)/probes.o $(MX_LIB) $(MX_CRT0)
	$(LINK) -Map $(@:.elf=.map) -o $@ $(MX_CRT0) $(OUT)/vuuarch.o $(OUT)/probes.o $(MX_LIB)

clean:
	rm -rf $(OUT)

.PHONY: all clean FORCE
