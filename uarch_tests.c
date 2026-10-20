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

// Prefetch placement: 4 lines (256 bytes) ahead, at the top of the line, after
// its first load, or at the end
#define PREF "pref 0, 256($8)\n"
#define LWADD15                                                         \
    LWADD(4) LWADD(8) LWADD(12) LWADD(16) LWADD(20) LWADD(24) LWADD(28)     \
    LWADD(32) LWADD(36) LWADD(40) LWADD(44) LWADD(48) LWADD(52) LWADD(56)   \
    LWADD(60)
#define STREAM_SETUP "move $8, %3\n" "move $25, $0\n"
KERNEL(k_pf_top_use0, 1, STREAM_SETUP,
       PREF LWADD(0) LWADD15 "addiu $8, $8, 64\n")
KERNEL(k_pf_first_use0, 1, STREAM_SETUP,
       LWADD(0) PREF LWADD15 "addiu $8, $8, 64\n")
KERNEL(k_pf_end_use0, 1, STREAM_SETUP,
       LWADD(0) LWADD15 PREF "addiu $8, $8, 64\n")

#define LW7                                                         \
    "lw $10, 4($8)\n" "lw $11, 8($8)\n" "lw $12, 12($8)\n"           \
    "lw $13, 16($8)\n" "lw $14, 20($8)\n" "lw $15, 24($8)\n"         \
    "lw $24, 28($8)\n"
KERNEL(k_pf_top_use8, 1, STREAM_SETUP,
       PREF LW8(0) ADD8 LW8(32) ADD8 "addiu $8, $8, 64\n")
KERNEL(k_pf_first_use8, 1, STREAM_SETUP,
       "lw $9, 0($8)\n" PREF LW7 ADD8 LW8(32) ADD8 "addiu $8, $8, 64\n")
KERNEL(k_pf_end_use8, 1, STREAM_SETUP,
       LW8(0) ADD8 LW8(32) ADD8 PREF "addiu $8, $8, 64\n")

// Overlapping misses: $8 walks the first half of the buffer, $11 the second
#define HALVES_SETUP "move $8, %3\n" "lui $11, 8\n" "addu $11, $11, %3\n"
// One miss per iteration
KERNEL(k_miss1, 1, HALVES_SETUP,
       "lw $9, 0($8)\n" "addu $25, $25, $9\n" "addiu $8, $8, 64\n")
// Two misses to different lines, both used afterwards
KERNEL(k_miss2, 1, HALVES_SETUP,
       "lw $9, 0($8)\n" "lw $10, 0($11)\n" "addu $25, $25, $9\n"
       "addu $25, $25, $10\n" "addiu $8, $8, 64\n" "addiu $11, $11, 64\n")
// A prefetch miss, then a load miss to a different line
KERNEL(k_pf_miss, 1, HALVES_SETUP,
       "pref 0, 0($11)\n" "lw $9, 0($8)\n" "addu $25, $25, $9\n"
       "addiu $8, $8, 64\n" "addiu $11, $11, 64\n")
// A load miss, then a prefetch miss to a different line
KERNEL(k_miss_pf, 1, HALVES_SETUP,
       "lw $9, 0($8)\n" "pref 0, 0($11)\n" "addu $25, $25, $9\n"
       "addiu $8, $8, 64\n" "addiu $11, $11, 64\n")

// --- 128-bit multimedia (MMI) ---
// Not every assembler accepts the R5900 instructions, so they are .word. The
// registers are numbers; $24 holds the pointer argument (MMI_SETUP).
#define R3(op, d, s, t) \
    ".word (" #op " | (" #s " << 21) | (" #t " << 16) | (" #d " << 11))\n"
#define R2(op, d, t) ".word (" #op " | (" #t " << 16) | (" #d " << 11))\n"
#define RSA(op, d, t, sa) \
    ".word (" #op " | (" #t " << 16) | (" #d " << 11) | (" #sa " << 6))\n"
#define MEM(op, t, off, b) \
    ".word (" #op " | (" #b " << 21) | (" #t " << 16) | (" #off "))\n"
#define LQ(t, off, b) MEM(0x78000000, t, off, b)
#define SQ(t, off, b) MEM(0x7c000000, t, off, b)
#define PADDW(d, s, t) R3(0x70000008, d, s, t)
#define PCGTW(d, s, t) R3(0x70000088, d, s, t)
#define PMAXW(d, s, t) R3(0x700000c8, d, s, t)
#define PAND(d, s, t) R3(0x70000489, d, s, t)
#define PEXTLW(d, s, t) R3(0x70000488, d, s, t)
#define PPACW(d, s, t) R3(0x700004c8, d, s, t)
#define PCPYLD(d, s, t) R3(0x70000389, d, s, t)
#define PCPYUD(d, s, t) R3(0x700003a9, d, s, t)
#define PMULTW(d, s, t) R3(0x70000309, d, s, t)
#define PMULTH(d, s, t) R3(0x70000709, d, s, t)
#define MULT3(d, s, t) R3(0x00000018, d, s, t)
#define MULT1(d, s, t) R3(0x70000018, d, s, t)
#define PLZCW(d, s) R3(0x70000004, d, s, 0)
#define PEXEW(d, t) R2(0x70000789, d, t)
#define PCPYH(d, t) R2(0x700006e9, d, t)
#define PROT3W(d, t) R2(0x700007c9, d, t)
#define PMFLO(d) R2(0x70000249, d, 0)
#define PSLLW(d, t, sa) RSA(0x7000003c, d, t, sa)
#define PSUBW(d, s, t) R3(0x70000048, d, s, t)
#define POR(d, s, t) R3(0x700004a9, d, s, t)
#define PXOR(d, s, t) R3(0x700004c9, d, s, t)
#define PNOR(d, s, t) R3(0x700004e9, d, s, t)
#define PADDB(d, s, t) R3(0x70000208, d, s, t)
#define PADDH(d, s, t) R3(0x70000108, d, s, t)
#define PCEQW(d, s, t) R3(0x700000a8, d, s, t)
#define PMINW(d, s, t) R3(0x700000e8, d, s, t)
#define PMINH(d, s, t) R3(0x700001e8, d, s, t)
#define PADDSW(d, s, t) R3(0x70000408, d, s, t)
#define PADDUB(d, s, t) R3(0x70000628, d, s, t)
#define PSUBUH(d, s, t) R3(0x70000568, d, s, t)
#define PSUBUW(d, s, t) R3(0x70000468, d, s, t)
#define PABSW(d, t) R2(0x70000068, d, t)
#define PABSH(d, t) R2(0x70000168, d, t)
#define PMFHI(d) R2(0x70000209, d, 0)
#define PSRLW(d, t, sa) RSA(0x7000003e, d, t, sa)
#define PSRAW(d, t, sa) RSA(0x7000003f, d, t, sa)
#define PSRAH(d, t, sa) RSA(0x70000037, d, t, sa)
#define PSLLH(d, t, sa) RSA(0x70000034, d, t, sa)

#define MMI_SETUP SETUP "move $24, %3\n"

// Latency: each instruction depends on the previous one
KERNEL(k_paddw, 64, MMI_SETUP, PADDW(9, 9, 10))
KERNEL(k_pcgtw, 64, MMI_SETUP, PCGTW(9, 9, 10))
KERNEL(k_pmaxw, 64, MMI_SETUP, PMAXW(9, 9, 10))
KERNEL(k_pand, 64, MMI_SETUP, PAND(9, 9, 10))
KERNEL(k_psllw, 64, MMI_SETUP, PSLLW(9, 9, 1))
KERNEL(k_pextlw, 64, MMI_SETUP, PEXTLW(9, 9, 10))
KERNEL(k_ppacw, 64, MMI_SETUP, PPACW(9, 9, 10))
KERNEL(k_pcpyld, 64, MMI_SETUP, PCPYLD(9, 9, 10))
KERNEL(k_pcpyud, 64, MMI_SETUP, PCPYUD(9, 9, 10))
KERNEL(k_pexew, 64, MMI_SETUP, PEXEW(9, 9))
KERNEL(k_pcpyh, 64, MMI_SETUP, PCPYH(9, 9))
KERNEL(k_prot3w, 64, MMI_SETUP, PROT3W(9, 9))
KERNEL(k_plzcw, 64, MMI_SETUP, PLZCW(9, 9))
KERNEL(k_pmultw, 64, MMI_SETUP, PMULTW(9, 9, 10))
KERNEL(k_pmulth, 64, MMI_SETUP, PMULTH(9, 9, 10))
KERNEL(k_pmultw_pmflo, 64, MMI_SETUP, PMULTW(0, 9, 10) PMFLO(9))
KERNEL(k_mult3, 64, MMI_SETUP, MULT3(9, 9, 10))
KERNEL(k_mult1_3, 64, MMI_SETUP, MULT1(9, 9, 10))
// p[0] = p: lq reads its own address
KERNEL(k_lq, 64, MMI_SETUP "move $9, %3\n", LQ(9, 0, 9))
KERNEL(k_psubw, 64, MMI_SETUP, PSUBW(9, 9, 10))
KERNEL(k_por, 64, MMI_SETUP, POR(9, 9, 10))
KERNEL(k_pxor, 64, MMI_SETUP, PXOR(9, 9, 10))
KERNEL(k_pnor, 64, MMI_SETUP, PNOR(9, 9, 10))
KERNEL(k_paddb, 64, MMI_SETUP, PADDB(9, 9, 10))
KERNEL(k_paddh, 64, MMI_SETUP, PADDH(9, 9, 10))
KERNEL(k_pceqw, 64, MMI_SETUP, PCEQW(9, 9, 10))
KERNEL(k_pminw, 64, MMI_SETUP, PMINW(9, 9, 10))
KERNEL(k_pminh, 64, MMI_SETUP, PMINH(9, 9, 10))
KERNEL(k_paddsw, 64, MMI_SETUP, PADDSW(9, 9, 10))
KERNEL(k_paddub, 64, MMI_SETUP, PADDUB(9, 9, 10))
KERNEL(k_psubuh, 64, MMI_SETUP, PSUBUH(9, 9, 10))
KERNEL(k_psubuw, 64, MMI_SETUP, PSUBUW(9, 9, 10))
KERNEL(k_pabsw, 64, MMI_SETUP, PABSW(9, 9))
KERNEL(k_pabsh, 64, MMI_SETUP, PABSH(9, 9))
KERNEL(k_psrlw, 64, MMI_SETUP, PSRLW(9, 9, 1))
KERNEL(k_psraw, 64, MMI_SETUP, PSRAW(9, 9, 1))
KERNEL(k_psrah, 64, MMI_SETUP, PSRAH(9, 9, 1))
KERNEL(k_psllh, 64, MMI_SETUP, PSLLH(9, 9, 1))

// Producer and consumer per repetition, independent between repetitions
KERNEL(k_lq_paddw, 64, MMI_SETUP, LQ(9, 0, 24) PADDW(10, 9, 11))
KERNEL(k_lq_addu, 64, MMI_SETUP, LQ(9, 0, 24) "addu $10, $9, $11\n")
KERNEL(k_paddw_addu, 64, MMI_SETUP, PADDW(9, 11, 12) "addu $10, $9, $11\n")
KERNEL(k_addu_paddw, 64, MMI_SETUP, "addu $9, $11, $12\n" PADDW(10, 9, 11))
KERNEL(k_paddw_sq, 64, MMI_SETUP, PADDW(9, 11, 12) SQ(9, 32, 24))
KERNEL(k_psllw_paddw, 64, MMI_SETUP, PSLLW(9, 11, 1) PADDW(10, 9, 11))
KERNEL(k_pcpyud_addu, 64, MMI_SETUP, PCPYUD(9, 11, 11) "addu $10, $9, $11\n")
KERNEL(k_pmultw_paddw, 64, MMI_SETUP, PMULTW(9, 11, 12) PADDW(10, 9, 11))
KERNEL(k_mult3_addu, 64, MMI_SETUP, MULT3(9, 11, 12) "addu $10, $9, $11\n")

// Round trips through memory, as in a stack temporary: each store is read back
// by the next load, which feeds the next store
KERNEL(k_sw_lw, 64, MMI_SETUP, "sw $9, 32($24)\n" "lw $9, 32($24)\n")
KERNEL(k_sq_lq, 64, MMI_SETUP, SQ(9, 32, 24) LQ(9, 32, 24))
KERNEL(k_sq_lw, 64, MMI_SETUP, SQ(9, 32, 24) "lw $9, 36($24)\n")
KERNEL(k_sq_ld, 64, MMI_SETUP, SQ(9, 32, 24) "ld $9, 40($24)\n")
KERNEL(k_sw_lq, 64, MMI_SETUP, "sw $9, 44($24)\n" LQ(9, 32, 24))
KERNEL(k_sd2_lq, 64, MMI_SETUP,
       "sd $9, 32($24)\n" "sd $9, 40($24)\n" LQ(9, 32, 24))
KERNEL(k_sw4_lq, 64, MMI_SETUP,
       "sw $9, 32($24)\n" "sw $9, 36($24)\n" "sw $9, 40($24)\n"
       "sw $9, 44($24)\n" LQ(9, 32, 24))

// Unaligned 128-bit load and store sequences, independent between repetitions.
// Through a stack temporary (as currently lowered):
KERNEL(k_uload_stack, 16, MMI_SETUP,
       "ldl $9, 31($24)\n" "ldr $9, 24($24)\n" "sd $9, 56($24)\n"
       "ldl $10, 23($24)\n" "ldr $10, 16($24)\n" "sd $10, 48($24)\n"
       LQ(11, 48, 24) PADDW(12, 11, 13))
// Combining the halves with pcpyld:
KERNEL(k_uload_pcpyld, 16, MMI_SETUP,
       "ldl $9, 31($24)\n" "ldr $9, 24($24)\n" "ldl $10, 23($24)\n"
       "ldr $10, 16($24)\n" PCPYLD(11, 9, 10) PADDW(12, 11, 13))
KERNEL(k_ustore_stack, 16, MMI_SETUP,
       PADDW(11, 12, 13) SQ(11, 48, 24) "ld $9, 56($24)\n" "ld $10, 48($24)\n"
       "sdl $9, 31($24)\n" "sdr $9, 24($24)\n" "sdl $10, 23($24)\n"
       "sdr $10, 16($24)\n")
KERNEL(k_ustore_pcpyud, 16, MMI_SETUP,
       PADDW(11, 12, 13) PCPYUD(9, 11, 11) "sdl $9, 31($24)\n"
       "sdr $9, 24($24)\n" "sdl $11, 23($24)\n" "sdr $11, 16($24)\n")

// Pairing: independent instructions
KERNEL(k_paddw_paddw, 64, MMI_SETUP, PADDW(9, 12, 13) PADDW(10, 12, 13))
KERNEL(k_paddw_addu2, 64, MMI_SETUP, PADDW(9, 12, 13) "addu $10, $12, $13\n")
KERNEL(k_addu_paddw2, 64, MMI_SETUP, "addu $10, $12, $13\n" PADDW(9, 12, 13))
KERNEL(k_paddw_lw, 64, MMI_SETUP, PADDW(9, 12, 13) "lw $10, 0($24)\n")
KERNEL(k_paddw_lq, 64, MMI_SETUP, PADDW(9, 12, 13) LQ(10, 0, 24))
KERNEL(k_paddw_sq2, 64, MMI_SETUP, PADDW(9, 12, 13) SQ(12, 48, 24))
KERNEL(k_lq_lq, 64, MMI_SETUP, LQ(9, 0, 24) LQ(10, 16, 24))
KERNEL(k_sq_sq, 64, MMI_SETUP, SQ(12, 32, 24) SQ(13, 48, 24))
KERNEL(k_psllw_paddw2, 64, MMI_SETUP, PSLLW(9, 12, 1) PADDW(10, 12, 13))
KERNEL(k_psllw_addu, 64, MMI_SETUP, PSLLW(9, 12, 1) "addu $10, $12, $13\n")
KERNEL(k_pcpyld_paddw, 64, MMI_SETUP, PCPYLD(9, 12, 13) PADDW(10, 12, 13))
KERNEL(k_paddw_plzcw, 64, MMI_SETUP, PADDW(9, 12, 13) PLZCW(10, 12))
KERNEL(k_paddw_mult1, 64, MMI_SETUP, PADDW(9, 12, 13) MULT1(10, 12, 13))
KERNEL(k_pmultw_addu, 64, MMI_SETUP, PMULTW(9, 12, 13) "addu $10, $12, $13\n")
KERNEL(k_pmultw_x1, 64, MMI_SETUP, PMULTW(9, 12, 13))
KERNEL(k_mult3_x1, 64, MMI_SETUP, MULT3(9, 12, 13))
KERNEL(k_mult3_mult1, 64, MMI_SETUP, MULT3(9, 12, 13) MULT1(10, 12, 13))
KERNEL(k_mult3_addu2, 64, MMI_SETUP, MULT3(9, 12, 13) "addu $10, $12, $13\n")
KERNEL(k_pmflo_addu, 64, MMI_SETUP, PMFLO(9) "addu $10, $12, $13\n")
KERNEL(k_pmfhi_lw, 64, MMI_SETUP, PMFHI(9) "lw $10, 0($24)\n")
KERNEL(k_pabsw_addu, 64, MMI_SETUP, PABSW(9, 12) "addu $10, $12, $13\n")
KERNEL(k_psrlw_lw, 64, MMI_SETUP, PSRLW(9, 12, 1) "lw $10, 0($24)\n")

// out[j] = in[j] * 3 + 1 for one 64-byte line (in at 0, out at 64), in cache.
// Scalar, 4 elements interleaved:
#define SC4(o)                                                          \
    "lw $8, " #o "+0($24)\n" "lw $9, " #o "+4($24)\n"                   \
    "lw $10, " #o "+8($24)\n" "lw $11, " #o "+12($24)\n"                \
    "sll $12, $8, 1\n" "sll $13, $9, 1\n" "sll $14, $10, 1\n"           \
    "sll $25, $11, 1\n" "addu $8, $12, $8\n" "addu $9, $13, $9\n"       \
    "addu $10, $14, $10\n" "addu $11, $25, $11\n" "addiu $8, $8, 1\n"   \
    "addiu $9, $9, 1\n" "addiu $10, $10, 1\n" "addiu $11, $11, 1\n"     \
    "sw $8, 64+" #o "+0($24)\n" "sw $9, 64+" #o "+4($24)\n"             \
    "sw $10, 64+" #o "+8($24)\n" "sw $11, 64+" #o "+12($24)\n"
KERNEL(k_scale_scalar, 1, MMI_SETUP, SC4(0) SC4(16) SC4(32) SC4(48))
// MMI, aligned: lq, psllw, paddw, paddw (the splat of 1 is in $15), sq
#define VSC(op4) op4(8, 12, 0) op4(9, 13, 16) op4(10, 14, 32) op4(11, 25, 48)
#define V_LQ(r, t, o) LQ(r, o, 24)
#define V_SLL(r, t, o) PSLLW(t, r, 1)
#define V_ADD(r, t, o) PADDW(t, t, r)
#define V_ADD1(r, t, o) PADDW(t, t, 15)
#define V_SQ(r, t, o) SQ(t, 64 + o, 24)
KERNEL(k_scale_mmi, 1, MMI_SETUP,
       VSC(V_LQ) VSC(V_SLL) VSC(V_ADD) VSC(V_ADD1) VSC(V_SQ))
// MMI, unaligned: ldl/ldr + pcpyld, ..., pcpyud + sdl/sdr
#define V_ULD(r, t, o)                                                  \
    "ldl $" #r ", " #o "+15($24)\n" "ldr $" #r ", " #o "+8($24)\n"      \
    "ldl $" #t ", " #o "+7($24)\n" "ldr $" #t ", " #o "+0($24)\n"       \
    PCPYLD(r, r, t)
#define V_UST(r, t, o)                                                  \
    PCPYUD(r, t, t) "sdl $" #r ", 64+" #o "+15($24)\n"                  \
    "sdr $" #r ", 64+" #o "+8($24)\n" "sdl $" #t ", 64+" #o "+7($24)\n" \
    "sdr $" #t ", 64+" #o "+0($24)\n"
KERNEL(k_scale_mmi_u, 1, MMI_SETUP,
       VSC(V_ULD) VSC(V_SLL) VSC(V_ADD) VSC(V_ADD1) VSC(V_UST))

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

    static const uarch_test mmi_lat[] = {
        {"paddw", k_paddw, 64, "instr", "?"},
        {"pcgtw", k_pcgtw, 64, "instr", "?"},
        {"pmaxw", k_pmaxw, 64, "instr", "?"},
        {"pand", k_pand, 64, "instr", "?"},
        {"psllw", k_psllw, 64, "instr", "?"},
        {"pextlw", k_pextlw, 64, "instr", "?"},
        {"ppacw", k_ppacw, 64, "instr", "?"},
        {"pcpyld", k_pcpyld, 64, "instr", "?"},
        {"pcpyud", k_pcpyud, 64, "instr", "?"},
        {"pexew", k_pexew, 64, "instr", "?"},
        {"pcpyh", k_pcpyh, 64, "instr", "?"},
        {"prot3w", k_prot3w, 64, "instr", "?"},
        {"plzcw", k_plzcw, 64, "instr", "?"},
        {"pmultw rd", k_pmultw, 64, "instr", "?"},
        {"pmulth rd", k_pmulth, 64, "instr", "?"},
        {"pmultw+pmflo", k_pmultw_pmflo, 64, "2instr", "?"},
        {"mult rd (3-op)", k_mult3, 64, "instr", "?"},
        {"mult1 rd (3-op)", k_mult1_3, 64, "instr", "?"},
        {"lq (hit)", k_lq, 64, "instr", "?"},
        {"psubw", k_psubw, 64, "instr", "?"},
        {"por", k_por, 64, "instr", "?"},
        {"pxor", k_pxor, 64, "instr", "?"},
        {"pnor", k_pnor, 64, "instr", "?"},
        {"paddb", k_paddb, 64, "instr", "?"},
        {"paddh", k_paddh, 64, "instr", "?"},
        {"pceqw", k_pceqw, 64, "instr", "?"},
        {"pminw", k_pminw, 64, "instr", "?"},
        {"pminh", k_pminh, 64, "instr", "?"},
        {"paddsw", k_paddsw, 64, "instr", "?"},
        {"paddub", k_paddub, 64, "instr", "?"},
        {"psubuh", k_psubuh, 64, "instr", "?"},
        {"psubuw", k_psubuw, 64, "instr", "?"},
        {"pabsw", k_pabsw, 64, "instr", "?"},
        {"pabsh", k_pabsh, 64, "instr", "?"},
        {"psrlw", k_psrlw, 64, "instr", "?"},
        {"psraw", k_psraw, 64, "instr", "?"},
        {"psrah", k_psrah, 64, "instr", "?"},
        {"psllh", k_psllh, 64, "instr", "?"},
    };
    static const uarch_test mmi_use[] = {
        {"lq->paddw", k_lq_paddw, 64, "rep", "= latency"},
        {"lq->addu", k_lq_addu, 64, "rep", "= latency"},
        {"paddw->addu", k_paddw_addu, 64, "rep", "= latency"},
        {"addu->paddw", k_addu_paddw, 64, "rep", "= latency"},
        {"paddw->sq", k_paddw_sq, 64, "rep", "= latency"},
        {"psllw->paddw", k_psllw_paddw, 64, "rep", "= latency"},
        {"pcpyud->addu", k_pcpyud_addu, 64, "rep", "= latency"},
        {"pmultw->paddw", k_pmultw_paddw, 64, "rep", "= latency"},
        {"mult rd->addu", k_mult3_addu, 64, "rep", "= latency"},
    };
    static const uarch_test mmi_mem[] = {
        {"sw->lw", k_sw_lw, 64, "rep", "store + load"},
        {"sq->lq", k_sq_lq, 64, "rep", "store + load"},
        {"sq->lw", k_sq_lw, 64, "rep", "store + load"},
        {"sq->ld", k_sq_ld, 64, "rep", "store + load"},
        {"sw->lq", k_sw_lq, 64, "rep", "store + load"},
        {"2 sd->lq", k_sd2_lq, 64, "rep", "stores + load"},
        {"4 sw->lq", k_sw4_lq, 64, "rep", "stores + load (splat)"},
        {"uload via stack", k_uload_stack, 16, "seq", "ldl/ldr x2, sd x2, lq, use"},
        {"uload via pcpyld", k_uload_pcpyld, 16, "seq", "ldl/ldr x2, pcpyld, use"},
        {"ustore via stack", k_ustore_stack, 16, "seq", "def, sq, ld x2, sdl/sdr x2"},
        {"ustore via pcpyud", k_ustore_pcpyud, 16, "seq", "def, pcpyud, sdl/sdr x2"},
    };
    static const uarch_test mmi_pair[] = {
        {"paddw|paddw", k_paddw_paddw, 64, "pair", "1 = paired"},
        {"paddw|addu", k_paddw_addu2, 64, "pair", "1 = paired"},
        {"addu|paddw", k_addu_paddw2, 64, "pair", "1 = paired"},
        {"paddw|lw", k_paddw_lw, 64, "pair", "1 = paired"},
        {"paddw|lq", k_paddw_lq, 64, "pair", "1 = paired"},
        {"paddw|sq", k_paddw_sq2, 64, "pair", "1 = paired"},
        {"lq|lq", k_lq_lq, 64, "pair", "1 = paired"},
        {"sq|sq", k_sq_sq, 64, "pair", "1 = paired"},
        {"psllw|paddw", k_psllw_paddw2, 64, "pair", "1 = paired"},
        {"psllw|addu", k_psllw_addu, 64, "pair", "1 = paired"},
        {"pcpyld|paddw", k_pcpyld_paddw, 64, "pair", "1 = paired"},
        {"paddw|plzcw", k_paddw_plzcw, 64, "pair", "1 = paired"},
        {"paddw|mult1", k_paddw_mult1, 64, "pair", "1 = paired"},
        {"pmultw|addu", k_pmultw_addu, 64, "pair", "1 = paired"},
        {"pmultw", k_pmultw_x1, 64, "instr", "throughput"},
        {"mult (3-op)", k_mult3_x1, 64, "instr", "throughput"},
        {"mult|mult1", k_mult3_mult1, 64, "pair", "throughput of both"},
        {"mult|addu", k_mult3_addu2, 64, "pair", "1 = paired"},
        {"pmflo|addu", k_pmflo_addu, 64, "pair", "1 = paired"},
        {"pmfhi|lw", k_pmfhi_lw, 64, "pair", "1 = paired"},
        {"pabsw|addu", k_pabsw_addu, 64, "pair", "1 = paired"},
        {"psrlw|lw", k_psrlw_lw, 64, "pair", "1 = paired"},
    };
    static const uarch_test scale[] = {
        {"scalar", k_scale_scalar, 1, "line", "lw, sll, addu, addiu, sw"},
        {"mmi aligned", k_scale_mmi, 1, "line", "lq, psllw, paddw x2, sq"},
        {"mmi unaligned", k_scale_mmi_u, 1, "line", "via pcpyld/pcpyud"},
    };
    static u32 sbuf[32] __attribute__((aligned(64)));

    printf("--- MMI latency (dependent chains) ---\n");
    for (unsigned i = 0; i < sizeof(mmi_lat) / sizeof(mmi_lat[0]); i++)
        print_line(mmi_lat[i].name,
                   run(mmi_lat[i].fn, buf, LAT_ITERS, mmi_lat[i].reps),
                   mmi_lat[i].unit, mmi_lat[i].expect);

    printf("--- MMI latency to consumer ---\n");
    for (unsigned i = 0; i < sizeof(mmi_use) / sizeof(mmi_use[0]); i++)
        print_line(mmi_use[i].name,
                   run(mmi_use[i].fn, buf, LAT_ITERS, mmi_use[i].reps),
                   mmi_use[i].unit, mmi_use[i].expect);

    printf("--- Memory round trips, unaligned 128-bit access ---\n");
    for (unsigned i = 0; i < sizeof(mmi_mem) / sizeof(mmi_mem[0]); i++)
        print_line(mmi_mem[i].name,
                   run(mmi_mem[i].fn, buf, LAT_ITERS, mmi_mem[i].reps),
                   mmi_mem[i].unit, mmi_mem[i].expect);

    printf("--- MMI pairing and throughput ---\n");
    for (unsigned i = 0; i < sizeof(mmi_pair) / sizeof(mmi_pair[0]); i++)
        print_line(mmi_pair[i].name,
                   run(mmi_pair[i].fn, buf, LAT_ITERS, mmi_pair[i].reps),
                   mmi_pair[i].unit, mmi_pair[i].expect);

    printf("--- out = in * 3 + 1, one line in cache ---\n");
    for (unsigned i = 0; i < sizeof(scale) / sizeof(scale[0]); i++)
        print_line(scale[i].name,
                   run(scale[i].fn, sbuf, LAT_ITERS, scale[i].reps),
                   scale[i].unit, scale[i].expect);

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
    printf("--- Prefetch 256 bytes ahead (cyc/line) ---\n");
    print_line("pf top, lw,use", run(k_pf_top_use0, big, lines, 1), "line",
               "pref before the line's loads");
    print_line("pf first, lw,use", run(k_pf_first_use0, big, lines, 1), "line",
               "pref after the first load");
    print_line("pf end, lw,use", run(k_pf_end_use0, big, lines, 1), "line",
               "pref after the line's loads");
    print_line("pf top, 8 lw,8 use", run(k_pf_top_use8, big, lines, 1), "line",
               "pref before the line's loads");
    print_line("pf first, 8 lw,8 use", run(k_pf_first_use8, big, lines, 1),
               "line", "pref after the first load");
    print_line("pf end, 8 lw,8 use", run(k_pf_end_use8, big, lines, 1), "line",
               "pref after the line's loads");
    printf("--- Overlapping misses (cyc/iteration) ---\n");
    print_line("1 miss", run(k_miss1, big, lines / 2, 1), "iter",
               "one load miss");
    print_line("2 misses", run(k_miss2, big, lines / 2, 1), "iter",
               "= 1 miss: overlap, 2x: serialized");
    print_line("pref miss, lw miss", run(k_pf_miss, big, lines / 2, 1), "iter",
               "= 1 miss: overlap");
    print_line("lw miss, pref miss", run(k_miss_pf, big, lines / 2, 1), "iter",
               "= 1 miss: overlap");
    printf("\n");
}
