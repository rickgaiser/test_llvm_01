# Build the benchmark with one compiler into an out-of-tree directory.
#
#   make COMPILER=gcc                         OUT=<dir>
#   make COMPILER=clang CLANG=<path to clang> OUT=<dir>
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
CFLAGS += $(EXTRA_CFLAGS)

all: $(BIN) $(ASMS)

$(BIN): $(OBJS)
	$(EE_GCC) -o $@ $^ $(LDFLAGS)

$(OUT)/%.o: %.c *.h | $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/%.s: %.c *.h | $(OUT)
	$(CC) $(CFLAGS) -S $< -o $@

$(OUT):
	mkdir -p $@

clean:
	rm -rf $(OUT)

.PHONY: all clean
