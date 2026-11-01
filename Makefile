# Build the benchmark with one compiler into an out-of-tree directory.
#
#   make COMPILER=gcc                         OUT=<dir>
#   make COMPILER=clang CLANG=<path to clang> OUT=<dir>
#   make COMPILER=clang SDK=maxsdk OUT=<dir>   on maxsdk + picolibc, no ps2sdk
#   make COMPILER=clang BENCH_ASM=1 ...         the kernels in asm/*.S instead of C
#
# Normally driven by bench.sh.

COMPILER ?= clang
OUT      ?= out/$(COMPILER)
CLANG    ?= clang

SRCS = main.c fpu_tests.c int_tests.c vpu_tests.c stream_tests.c uarch_tests.c \
       app_tests.c gfx_tests.c sound_tests.c emu_tests.c shuffle_tests.c
OBJS = $(SRCS:%.c=$(OUT)/%.o)
ASMS = $(SRCS:%.c=$(OUT)/%.s)
BIN  = $(OUT)/test.elf

EE_GCC = $(PS2DEV)/ee/bin/mips64r5900el-ps2-elf-gcc

CFLAGS_COMMON = -D_EE -Wall -O3 -ffast-math -I$(PS2SDK)/ee/include -I$(PS2SDK)/common/include -I.
LDFLAGS = -T$(PS2SDK)/ee/startup/linkfile -L$(PS2SDK)/ee/lib -Wl,-zmax-page-size=128

ifeq ($(COMPILER),gcc)
CC     = $(EE_GCC)
CFLAGS = $(CFLAGS_COMMON) -G0 -fno-lto
else
CC     = $(CLANG)
CFLAGS = $(CFLAGS_COMMON) -I$(PS2DEV)/ee/mips64r5900el-ps2-elf/include \
         --target=mips64el -mcpu=r5900 -mabi=n32 -fno-pic -mno-abicalls -G 0 \
         -ffp-contract=fast -fomit-frame-pointer
endif
ifeq ($(SDK),maxsdk)
# maxsdk (libs/maxsdk) with picolibc: its compat headers give the few ps2sdk
# calls used here (tamtypes.h, timer.h, kernel.h)
MAXSDK   ?= $(abspath ../../libs/maxsdk)
PS2MAX   ?= $(abspath ../..)
MX_OUT   ?= $(PS2MAX)/build/libs/maxsdk
PICOLIBC ?= $(PS2MAX)/build/libs/picolibc/install
LLVM     ?= $(dir $(CLANG))
CFLAGS    = -Wall -O3 -ffast-math -I. -isystem $(PICOLIBC)/include -I$(MAXSDK)/compat/ps2sdk \
            -I$(MAXSDK)/include --target=mips64el -mcpu=r5900 -mabi=n32 -fno-pic -mno-abicalls \
            -G 0 -ffp-contract=fast -fomit-frame-pointer
MX_LIBS   = $(MX_OUT)/libmxc.a $(PICOLIBC)/lib/libc.a $(MX_OUT)/libmx.a $(MX_OUT)/librt.a
endif
CFLAGS += $(EXTRA_CFLAGS)

# The hand-written kernels (asm/<file>.S for <file>.c): with BENCH_ASM the C
# files declare them instead of defining their own. ASM_VU0=1 selects the
# kernels that use VU0 (asm-vu0) where a kernel has a version with and one
# without VU0.
ifeq ($(ASM_VU0),1)
CFLAGS += -DASM_VU0
endif
ifeq ($(BENCH_ASM),1)
CFLAGS += -DBENCH_ASM
OBJS   += $(patsubst asm/%.S,$(OUT)/asm_%.o,$(wildcard asm/*.S))
endif

all: $(BIN) $(ASMS)

ifeq ($(SDK),maxsdk)
$(BIN): $(OBJS)
	$(LLVM)ld.lld -m elf32ltsmipn32 -T$(MAXSDK)/ld/maxsdk.ld -z max-page-size=128 --gc-sections \
	  -o $@ $(MX_OUT)/obj/crt0.o $^ $(MX_LIBS)
else
$(BIN): $(OBJS)
	$(EE_GCC) -o $@ $^ $(LDFLAGS)
endif

$(OUT)/%.o: %.c *.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

# -mvu0 for the VU0 macro instructions of the v4sf kernels
$(OUT)/asm_%.o: asm/%.S | $(OUT)
	$(CC) $(CFLAGS) -mvu0 -c $< -o $@

$(OUT)/%.s: %.c *.h | $(OUT)
	$(CC) $(CFLAGS) -S $< -o $@

$(OUT):
	mkdir -p $@

clean:
	rm -rf $(OUT)

.PHONY: all clean
