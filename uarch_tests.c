#include <stdio.h>
#include <kernel.h>
#include "uarch_tests.h"

// R5900-only FPU accumulator instructions, as .word so that every assembler
// accepts them: madd.s fd = ACC + fs * ft, madda.s ACC += fs * ft,
// mula.s ACC = fs * ft.
#define MADD_S(fd, fs, ft) \
    ".word (0x4600001c | (" #ft " << 16) | (" #fs " << 11) | (" #fd " << 6))\n"
#define MADDA_S(fs, ft) \
    ".word (0x4600001e | (" #ft " << 16) | (" #fs " << 11))\n"
#define MULA_S(fs, ft) \
    ".word (0x4600001a | (" #ft " << 16) | (" #fs " << 11))\n"

// Setup: $f0-$f3, $f8, $f9 = 1.0, ACC = 1.0, $9-$15 = 1
#define SETUP                                                         \
    "lui $9, 0x3f80\n"                                                \
    "mtc1 $9, $f0\n" "mtc1 $9, $f1\n" "mtc1 $9, $f2\n" "mtc1 $9, $f3\n" \
    "mtc1 $9, $f8\n" "mtc1 $9, $f9\n"                                 \
    MULA_S(0, 0)                                                      \
    "li $9, 1\n" "li $10, 1\n" "li $11, 1\n" "li $12, 1\n"            \
    "li $13, 1\n" "li $14, 1\n" "li $15, 1\n"

// A kernel runs `body` reps times per loop iteration, for iters iterations,
// and returns the elapsed CPU cycles. %3 is the pointer argument.
#define KERNEL(name, reps, setup, body)                                    \
    static u32 name(u32 *p, int iters) {                                   \
        u32 t0, t1;                                                        \
        __asm__ __volatile__(                                              \
            ".set push\n.set noreorder\n.set noat\n"                       \
            setup                                                          \
            "mfc0 %0, $9\n"                                                \
            ".align 3\n"                                                   \
            "1:\n"                                                         \
            ".rept " #reps "\n" body ".endr\n"                             \
            "addiu %2, %2, -1\n"                                           \
            "bnez %2, 1b\n"                                                \
            "nop\n"                                                        \
            "mfc0 %1, $9\n"                                                \
            ".set pop\n"                                                   \
            : "=&r"(t0), "=&r"(t1), "+r"(iters)                            \
            : "r"(p)                                                       \
            : "memory", "hi", "lo", "$8", "$9", "$10", "$11", "$12",       \
              "$13", "$14", "$15", "$24", "$25", "$f0", "$f1", "$f2",      \
              "$f3", "$f4", "$f5", "$f6", "$f7", "$f8", "$f9");            \
        return t1 - t0;                                                    \
    }

// --- Latency: each instruction depends on the previous one ---
KERNEL(k_add_s, 64, SETUP, "add.s $f0, $f0, $f1\n")
KERNEL(k_mul_s, 64, SETUP, "mul.s $f0, $f0, $f1\n")
KERNEL(k_mov_s, 64, SETUP, "mov.s $f1, $f0\n" "mov.s $f0, $f1\n")
KERNEL(k_div_s, 64, SETUP, "div.s $f0, $f0, $f1\n")
KERNEL(k_mul_add_s, 64, SETUP, "mul.s $f1, $f1, $f2\n" "add.s $f1, $f1, $f3\n")
KERNEL(k_madda_s, 64, SETUP, MADDA_S(1, 2))
KERNEL(k_madd_s, 64, SETUP, MADD_S(1, 1, 2))
KERNEL(k_mula_madd_s, 64, SETUP, MULA_S(1, 2) MADD_S(1, 2, 3))
KERNEL(k_mtc1_mfc1, 64, SETUP, "mtc1 $9, $f1\n" "mfc1 $9, $f1\n")
KERNEL(k_mult_mflo, 64, SETUP, "mult $9, $10\n" "mflo $9\n")
// p[0] = p: lw reads its own address
KERNEL(k_lw, 64, SETUP "move $8, %3\n", "lw $8, 0($8)\n")
// p[1] = 0: the loaded zero is added to the address of the next lwc1
KERNEL(k_lwc1, 64, SETUP "addiu $8, %3, 4\n" "mtc1 $0, $f1\n",
       "mfc1 $9, $f1\n" "addu $10, $8, $9\n" "lwc1 $f1, 0($10)\n")

// Producer and consumer per repetition, independent between repetitions. In
// order, the next producer issues with or after the consumer: cycles/rep is
// the latency (+1 when both use the single C1 slot).
KERNEL(k_lw_use, 64, SETUP, "lw $9, 0(%3)\n" "addu $10, $9, $11\n")
KERNEL(k_lwc1_use, 64, SETUP, "lwc1 $f1, 0(%3)\n" "add.s $f2, $f1, $f3\n")
KERNEL(k_mtc1_use, 64, SETUP, "mtc1 $9, $f1\n" "add.s $f2, $f1, $f3\n")
KERNEL(k_add_mfc1, 64, SETUP, "add.s $f1, $f2, $f3\n" "mfc1 $9, $f1\n")
KERNEL(k_mfc1_use, 64, SETUP, "mfc1 $9, $f8\n" "addu $10, $9, $11\n")
KERNEL(k_add_swc1, 64, SETUP, "add.s $f1, $f2, $f3\n" "swc1 $f1, 8(%3)\n")
KERNEL(k_sqrt_s, 64, SETUP, "sqrt.s $f0, $f0\n")
KERNEL(k_cvt, 64, SETUP, "cvt.w.s $f1, $f1\n" "cvt.s.w $f1, $f1\n")

// --- Pairing: independent instructions ---
KERNEL(k_addu_addu, 64, SETUP, "addu $9, $10, $11\n" "addu $12, $10, $11\n")
KERNEL(k_lw_lw, 64, SETUP, "lw $9, 0(%3)\n" "lw $10, 4(%3)\n")
KERNEL(k_lw_addu, 64, SETUP, "lw $9, 0(%3)\n" "addu $10, $11, $12\n")
KERNEL(k_addu_lw, 64, SETUP, "addu $10, $11, $12\n" "lw $9, 0(%3)\n")
KERNEL(k_mul_mul, 64, SETUP, "mul.s $f0, $f8, $f9\n" "mul.s $f1, $f8, $f9\n")
KERNEL(k_mul_addu, 64, SETUP, "mul.s $f0, $f8, $f9\n" "addu $10, $11, $12\n")
KERNEL(k_mul_lwc1, 64, SETUP, "mul.s $f0, $f8, $f9\n" "lwc1 $f2, 0(%3)\n")
KERNEL(k_lwc1_mul, 64, SETUP, "lwc1 $f2, 0(%3)\n" "mul.s $f0, $f8, $f9\n")
KERNEL(k_madda_lwc1, 64, SETUP, MADDA_S(8, 9) "lwc1 $f2, 0(%3)\n")
KERNEL(k_lwc1_lwc1, 64, SETUP, "lwc1 $f2, 0(%3)\n" "lwc1 $f3, 4(%3)\n")
KERNEL(k_mul_mfc1, 64, SETUP, "mul.s $f0, $f8, $f9\n" "mfc1 $9, $f8\n")
KERNEL(k_lw_mul, 64, SETUP, "lw $9, 0(%3)\n" "mul.s $f0, $f8, $f9\n")
KERNEL(k_mul_swc1, 64, SETUP, "mul.s $f0, $f8, $f9\n" "swc1 $f8, 8(%3)\n")

// --- Pipe and unit conflicts ---
// mult and mul.s both issue to pipe 0: 3 cycles if they conflict, 2 (MULT
// throughput) if not
KERNEL(k_mult_mul_mul, 64, SETUP,
       "mult $10, $11\n" "mul.s $f0, $f8, $f9\n" "mul.s $f1, $f8, $f9\n")
// Does MULT occupy pipe 0 for its whole 2-cycle throughput? 2 if not, 3 if so
KERNEL(k_mult_addu3, 64, SETUP,
       "mult $10, $11\n" "addu $12, $10, $11\n" "addu $13, $10, $11\n"
       "addu $14, $10, $11\n")
// Does div.s block other FPU instructions? 7 if not, 13 if so
KERNEL(k_div_add6, 64, SETUP,
       "div.s $f0, $f8, $f9\n" "add.s $f1, $f8, $f9\n" "add.s $f2, $f8, $f9\n"
       "add.s $f3, $f8, $f9\n" "add.s $f4, $f8, $f9\n" "add.s $f5, $f8, $f9\n"
       "add.s $f6, $f8, $f9\n")

KERNEL(k_div_add5, 64, SETUP,
       "div.s $f0, $f8, $f9\n" "add.s $f1, $f8, $f9\n" "add.s $f2, $f8, $f9\n"
       "add.s $f3, $f8, $f9\n" "add.s $f4, $f8, $f9\n" "add.s $f5, $f8, $f9\n")

// --- Memory: one cache line (64 bytes) per loop iteration ---
// Pointer chase over lines that are not in the cache: miss latency
KERNEL(k_chase_miss, 64, "move $8, %3\n", "lw $8, 0($8)\n")

#define LWADD(off) "lw $9, " #off "($8)\n" "addu $25, $25, $9\n"
// Each load is used by the next instruction
KERNEL(k_stream_use0, 1, "move $8, %3\n" "move $25, $0\n",
       LWADD(0) LWADD(4) LWADD(8) LWADD(12) LWADD(16) LWADD(20) LWADD(24)
       LWADD(28) LWADD(32) LWADD(36) LWADD(40) LWADD(44) LWADD(48) LWADD(52)
       LWADD(56) LWADD(60) "addiu $8, $8, 64\n")

#define LW8(o)                                                      \
    "lw $9, " #o "+0($8)\n" "lw $10, " #o "+4($8)\n"                 \
    "lw $11, " #o "+8($8)\n" "lw $12, " #o "+12($8)\n"               \
    "lw $13, " #o "+16($8)\n" "lw $14, " #o "+20($8)\n"              \
    "lw $15, " #o "+24($8)\n" "lw $24, " #o "+28($8)\n"
#define ADD8                                                        \
    "addu $25, $25, $9\n" "addu $25, $25, $10\n" "addu $25, $25, $11\n" \
    "addu $25, $25, $12\n" "addu $25, $25, $13\n" "addu $25, $25, $14\n" \
    "addu $25, $25, $15\n" "addu $25, $25, $24\n"
// 8 loads, then the 8 adds that use them
KERNEL(k_stream_use8, 1, "move $8, %3\n" "move $25, $0\n",
       LW8(0) ADD8 LW8(32) ADD8 "addiu $8, $8, 64\n")

typedef u32 (*kernel_fn)(u32 *p, int iters);

typedef struct {
    const char *name;
    kernel_fn fn;
    int reps;          // body repetitions per loop iteration
    const char *unit;  // what one body repetition is
    const char *expect;
} uarch_test;

#define LAT_ITERS 1000

static float run(kernel_fn fn, u32 *p, int iters, int reps) {
    u32 best = 0xffffffff;
    for (int i = 0; i < 3; i++) {
        DI();
        u32 c = fn(p, iters);
        EI();
        if (c < best)
            best = c;
    }
    return (float)best / ((float)iters * reps);
}

static void print_line(const char *name, float cyc, const char *unit,
                       const char *expect) {
    printf("  %-22s %7.2f cyc/%-6s (%s)\n", name, cyc, unit, expect);
}

void uarch_run_all(u32 *big, int big_words) {
    static u32 buf[16] __attribute__((aligned(64)));
    buf[0] = (u32)&buf[0];
    buf[1] = 0;

    static const uarch_test lat[] = {
        {"add.s", k_add_s, 64, "instr", "manual: 4"},
        {"mul.s", k_mul_s, 64, "instr", "manual: 4"},
        {"mov.s", k_mov_s, 64, "2instr", "manual: 4+4"},
        {"div.s", k_div_s, 64, "instr", "manual: 8"},
        {"mul.s+add.s", k_mul_add_s, 64, "2instr", "manual: 4+4"},
        {"madda.s ACC->ACC", k_madda_s, 64, "instr", "?"},
        {"madd.s FPR->FPR", k_madd_s, 64, "instr", "manual: 4"},
        {"mula.s+madd.s", k_mula_madd_s, 64, "2instr", "ACC->FPR + FPR->ACC"},
        {"mtc1+mfc1", k_mtc1_mfc1, 64, "2instr", "manual: 2+2"},
        {"mult+mflo", k_mult_mflo, 64, "2instr", "manual: 4+1"},
        {"lw (hit)", k_lw, 64, "instr", "manual: 1"},
        {"mfc1+addu+lwc1", k_lwc1, 64, "3instr", "manual: 2+1+2"},
        {"sqrt.s", k_sqrt_s, 64, "instr", "manual: 8"},
        {"cvt.w.s+cvt.s.w", k_cvt, 64, "2instr", "manual: 4+4"},
    };
    static const uarch_test use[] = {
        {"lw->addu", k_lw_use, 64, "rep", "= latency"},
        {"lwc1->add.s", k_lwc1_use, 64, "rep", "= latency + 1"},
        {"mtc1->add.s", k_mtc1_use, 64, "rep", "= latency + 1"},
        {"add.s->mfc1", k_add_mfc1, 64, "rep", "= latency + 1"},
        {"mfc1->addu", k_mfc1_use, 64, "rep", "= latency"},
        {"add.s->swc1", k_add_swc1, 64, "rep", "= latency + 1"},
    };
    static const uarch_test pair[] = {
        {"addu|addu", k_addu_addu, 64, "pair", "1 = paired"},
        {"lw|lw", k_lw_lw, 64, "pair", "2 = one LS"},
        {"lw|addu", k_lw_addu, 64, "pair", "1 = paired"},
        {"addu|lw", k_addu_lw, 64, "pair", "1 = paired"},
        {"mul.s|mul.s", k_mul_mul, 64, "pair", "2 = one FPU"},
        {"mul.s|addu", k_mul_addu, 64, "pair", "1 = paired"},
        {"mul.s|lwc1", k_mul_lwc1, 64, "pair", "1 = paired"},
        {"lwc1|mul.s", k_lwc1_mul, 64, "pair", "1 = paired"},
        {"madda.s|lwc1", k_madda_lwc1, 64, "pair", "1 = paired"},
        {"lwc1|lwc1", k_lwc1_lwc1, 64, "pair", "2 = one LS"},
        {"mul.s|mfc1", k_mul_mfc1, 64, "pair", "1 = paired"},
        {"mul.s|swc1", k_mul_swc1, 64, "pair", "1 = paired"},
        {"lw|mul.s", k_lw_mul, 64, "pair", "1 = paired"},
        {"mult,mul.s,mul.s", k_mult_mul_mul, 64, "group", "2 = no conflict, 3 = pipe 0"},
        {"mult,addu x3", k_mult_addu3, 64, "group", "2 = no conflict, 3 = pipe 0 busy"},
        {"div.s,add.s x6", k_div_add6, 64, "group", "7 = no conflict, 13 = FPU busy"},
        {"div.s,add.s x5", k_div_add5, 64, "group", "7 = div throughput"},
    };

    printf("--- Latency (dependent chains) ---\n");
    for (unsigned i = 0; i < sizeof(lat) / sizeof(lat[0]); i++)
        print_line(lat[i].name, run(lat[i].fn, buf, LAT_ITERS, lat[i].reps),
                   lat[i].unit, lat[i].expect);

    printf("--- Latency to consumer (producer + consumer per rep) ---\n");
    for (unsigned i = 0; i < sizeof(use) / sizeof(use[0]); i++)
        print_line(use[i].name, run(use[i].fn, buf, LAT_ITERS, use[i].reps),
                   use[i].unit, use[i].expect);

    printf("--- Pairing (independent instructions) ---\n");
    for (unsigned i = 0; i < sizeof(pair) / sizeof(pair[0]); i++)
        print_line(pair[i].name, run(pair[i].fn, buf, LAT_ITERS, pair[i].reps),
                   pair[i].unit, pair[i].expect);

    // Memory: one line per 16 words, much larger than the 8KB data cache
    int lines = big_words / 16;
    for (int i = 0; i < lines; i++)
        big[i * 16] = (u32)&big[((i + 1) % lines) * 16];
    printf("--- Memory (%d KB, not cached) ---\n", big_words * 4 / 1024);
    print_line("lw chase (miss)", run(k_chase_miss, big, lines / 64, 64),
               "load", "miss latency");
    print_line("stream lw,use", run(k_stream_use0, big, lines, 1), "line",
               "use right after load");
    print_line("stream 8 lw,8 use", run(k_stream_use8, big, lines, 1), "line",
               "use 8 loads later");
    printf("\n");
}
