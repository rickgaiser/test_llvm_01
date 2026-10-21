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

// Overlapping misses: $8 walks the first half of the buffer, $11 the second
#define HALVES_SETUP "move $8, %3\n" "lui $11, 8\n" "addu $11, $11, %3\n"
// One miss per iteration
KERNEL(k_miss1, 1, HALVES_SETUP,
       "lw $9, 0($8)\n" "addu $25, $25, $9\n" "addiu $8, $8, 64\n")
// Two misses to different lines, both used afterwards
KERNEL(k_miss2, 1, HALVES_SETUP,
       "lw $9, 0($8)\n" "lw $10, 0($11)\n" "addu $25, $25, $9\n"
       "addu $25, $25, $10\n" "addiu $8, $8, 64\n" "addiu $11, $11, 64\n")

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

// --- Divide, multiply-add, HI/LO moves, SA, unaligned loads ---
#define W(x) ".word " #x "\n"
#define DIV_9_10 W(0x012a001a)      // div $9, $10
#define DIV_12_13 W(0x018d001a)     // div $12, $13
#define DIV1_9_10 W(0x712a001a)     // div1 $9, $10
#define DIV1_12_13 W(0x718d001a)    // div1 $12, $13
#define MFLO_9 W(0x00004812)        // mflo $9
#define MFLO1_9 W(0x70004812)       // mflo1 $9
#define MTLO_9 W(0x01200013)        // mtlo $9
#define MADD_ACC W(0x714b0000)      // madd $0, $10, $11
#define MADD_9_9 W(0x712b4800)      // madd $9, $9, $11
#define MADD_9_10 W(0x714b4800)     // madd $9, $10, $11
#define MADD1_ACC W(0x714b0020)     // madd1 $0, $10, $11
#define MULT1_ACC_9 W(0x712a0018)   // mult1 $0, $9, $10
#define PMFHL_LW_9 W(0x70004830)    // pmfhl.lw $9
#define MTSAB_9 W(0x05380000)       // mtsab $9, 0
#define MTSAB_0 W(0x04180000)       // mtsab $0, 0
#define QFSRV(d, s, t) R3(0x700006e8, d, s, t)
#define PMADDW_ACC R3(0x70000009, 0, 10, 11)  // pmaddw $0, $10, $11

KERNEL(k_div, 16, MMI_SETUP, DIV_9_10 MFLO_9)
KERNEL(k_div_x1, 16, MMI_SETUP, DIV_12_13)
KERNEL(k_div_addu, 16, MMI_SETUP,
       DIV_12_13 "addu $9, $10, $11\n" "addu $14, $10, $11\n"
       "addu $9, $10, $11\n" "addu $14, $10, $11\n" "addu $9, $10, $11\n"
       "addu $14, $10, $11\n" "addu $9, $10, $11\n" "addu $14, $10, $11\n"
       "addu $9, $10, $11\n" "addu $14, $10, $11\n")
KERNEL(k_div1, 16, MMI_SETUP, DIV1_9_10 MFLO1_9)
KERNEL(k_div_div1, 16, MMI_SETUP, DIV_12_13 DIV1_12_13)
KERNEL(k_madd_acc, 64, MMI_SETUP, MADD_ACC)
KERNEL(k_madd_rd, 64, MMI_SETUP, MADD_9_9)
KERNEL(k_madd1_acc, 64, MMI_SETUP, MADD1_ACC)
KERNEL(k_mtlo_madd, 64, MMI_SETUP, MTLO_9 MADD_9_10)
KERNEL(k_mult1_mflo1, 64, MMI_SETUP, MULT1_ACC_9 MFLO1_9)
KERNEL(k_pmaddw_acc, 64, MMI_SETUP, PMADDW_ACC)
KERNEL(k_pmultw_pmfhl, 64, MMI_SETUP, PMULTW(0, 9, 10) PMFHL_LW_9)
KERNEL(k_qfsrv, 64, MMI_SETUP MTSAB_0, QFSRV(9, 9, 10))
KERNEL(k_mtsab_qfsrv, 64, MMI_SETUP, MTSAB_9 QFSRV(9, 9, 10))
KERNEL(k_qfsrv_addu, 64, MMI_SETUP MTSAB_0,
       QFSRV(9, 12, 13) "addu $10, $12, $13\n")
KERNEL(k_qfsrv_lw, 64, MMI_SETUP MTSAB_0, QFSRV(9, 12, 13) "lw $10, 0($24)\n")
// ldr merges into the register ldl wrote
KERNEL(k_ldl_ldr, 64, MMI_SETUP, "ldl $9, 7($24)\n" "ldr $9, 0($24)\n")
KERNEL(k_ldl_ldr_use, 64, MMI_SETUP,
       "ldl $9, 7($24)\n" "ldr $9, 0($24)\n" "addu $10, $9, $11\n")

// --- FPU: R5900 min/max/rsqrt, compare to branch ---
KERNEL(k_max_s, 64, SETUP, W(0x46010028))    // max.s $f0, $f0, $f1
KERNEL(k_min_s, 64, SETUP, W(0x46010029))    // min.s $f0, $f0, $f1
KERNEL(k_rsqrt_s, 64, SETUP, W(0x46004016))  // rsqrt.s $f0, $f8, $f0
// $f8 == $f9: bc1f is not taken, bc1t is taken (to the next instruction)
KERNEL(k_ceq_bc1f, 64, SETUP, "c.eq.s $f8, $f9\n" "bc1f 2f\n" "nop\n" "2:\n")
KERNEL(k_ceq_bc1t, 64, SETUP, "c.eq.s $f8, $f9\n" "bc1t 2f\n" "nop\n" "2:\n")
KERNEL(k_ceq_x2_bc1f, 64, SETUP,
       "c.eq.s $f8, $f9\n" "addu $12, $10, $11\n" "addu $13, $10, $11\n"
       "bc1f 2f\n" "nop\n" "2:\n")

// --- Branches ---
KERNEL(k_br_nt, 64, SETUP, "bne $9, $10, 2f\n" "nop\n" "2:\n")
KERNEL(k_br_t, 64, SETUP, "beq $9, $10, 2f\n" "nop\n" "2:\n")
KERNEL(k_br_nt_addu, 64, SETUP,
       "addu $12, $10, $11\n" "bne $9, $10, 2f\n" "addu $13, $10, $11\n" "2:\n")
// xorshift32 in $11; the branch depends on bit 0 (random) or on nothing
#define XORSHIFT                                                     \
    "sll $12, $11, 13\n" "xor $11, $11, $12\n" "srl $12, $11, 17\n"  \
    "xor $11, $11, $12\n" "sll $12, $11, 5\n" "xor $11, $11, $12\n"
#define RAND_SETUP SETUP "lui $11, 0x1234\n" "ori $11, $11, 0x5679\n"
KERNEL(k_br_rand, 16, RAND_SETUP,
       XORSHIFT "andi $13, $11, 1\n" "beq $13, $0, 2f\n" "nop\n" "2:\n")
KERNEL(k_br_rand_base, 16, RAND_SETUP,
       XORSHIFT "andi $13, $11, 0\n" "beq $13, $0, 2f\n" "nop\n" "2:\n")

// --- VU0 macro mode (COP2) ---
// Encodings from the VU Users Manual.
#define VADD_1 W(0x4be20868)        // vadd.xyzw $vf1, $vf1, $vf2
#define VMUL_1 W(0x4be2086a)        // vmul.xyzw $vf1, $vf1, $vf2
#define VMADD_1 W(0x4be20869)       // vmadd.xyzw $vf1, $vf1, $vf2
#define VMAX_1 W(0x4be2086b)        // vmax.xyzw $vf1, $vf1, $vf2
#define VMULA_1 W(0x4be20abe)       // vmula.xyzw $ACC, $vf1, $vf2
#define VMADDA_34 W(0x4be41abd)     // vmadda.xyzw $ACC, $vf3, $vf4
#define VADD_3 W(0x4be628e8)        // vadd.xyzw $vf3, $vf5, $vf6
#define VADD_4 W(0x4be62928)        // vadd.xyzw $vf4, $vf5, $vf6
#define VADD_2_1 W(0x4be308a8)      // vadd.xyzw $vf2, $vf1, $vf3
#define QMTC2_9_1 W(0x48a90800)     // qmtc2 $9, $vf1
#define QMFC2_10_3 W(0x482a1800)    // qmfc2 $10, $vf3
#define LQC2_1 W(0xdb010000)        // lqc2 $vf1, 0($24)
#define LQC2_7 W(0xdb070000)        // lqc2 $vf7, 0($24)
#define SQC2_3 W(0xfb030020)        // sqc2 $vf3, 32($24)
#define VDIV W(0x4a020bbc)          // vdiv $Q, $vf1x, $vf2x
#define VSQRT W(0x4a0203bd)         // vsqrt $Q, $vf2x
#define VRSQRT W(0x4a020bbe)        // vrsqrt $Q, $vf1x, $vf2x
#define VWAITQ W(0x4a0003bf)        // vwaitq
// $vf1-$vf6 = (1.0, 0, 0, 0), ACC = $vf1 * $vf2
#define VU0_SETUP                                                    \
    MMI_SETUP "lui $8, 0x3f80\n"                                     \
    W(0x48a80800) W(0x48a81000) W(0x48a81800) W(0x48a82000)          \
    W(0x48a82800) W(0x48a83000) VMULA_1

KERNEL(k_vadd, 64, VU0_SETUP, VADD_1)
KERNEL(k_vmul, 64, VU0_SETUP, VMUL_1)
KERNEL(k_vmadd, 64, VU0_SETUP, VMADD_1)
KERNEL(k_vmax, 64, VU0_SETUP, VMAX_1)
KERNEL(k_vmadda_acc, 64, VU0_SETUP, VMADDA_34)
KERNEL(k_vmula_vmadd, 64, VU0_SETUP, VMULA_1 VMADD_1)
KERNEL(k_qmtc2_vadd, 64, VU0_SETUP, QMTC2_9_1 VADD_2_1)
KERNEL(k_vadd_qmfc2, 64, VU0_SETUP, VADD_3 QMFC2_10_3)
KERNEL(k_lqc2_vadd, 64, VU0_SETUP, LQC2_1 VADD_2_1)
KERNEL(k_vadd_sqc2, 64, VU0_SETUP, VADD_3 SQC2_3)
KERNEL(k_vdiv_wait, 16, VU0_SETUP, VDIV VWAITQ)
KERNEL(k_vsqrt_wait, 16, VU0_SETUP, VSQRT VWAITQ)
KERNEL(k_vrsqrt_wait, 16, VU0_SETUP, VRSQRT VWAITQ)
// Does vdiv block other VU0 instructions? 7 if not
KERNEL(k_vdiv_vadd6, 16, VU0_SETUP,
       VDIV VADD_3 VADD_4 VADD_3 VADD_4 VADD_3 VADD_4 VWAITQ)
KERNEL(k_vadd_vadd, 64, VU0_SETUP, VADD_3 VADD_4)
KERNEL(k_vadd_addu, 64, VU0_SETUP, VADD_3 "addu $10, $12, $13\n")
KERNEL(k_vadd_lw, 64, VU0_SETUP, VADD_3 "lw $10, 0($24)\n")
KERNEL(k_vadd_lqc2, 64, VU0_SETUP, VADD_3 LQC2_7)
KERNEL(k_vadd_mul_s, 64, VU0_SETUP, VADD_3 "mul.s $f0, $f8, $f9\n")
KERNEL(k_vadd_paddw, 64, VU0_SETUP, VADD_3 PADDW(9, 12, 13))

// STREAM Sum loop bodies: each load is followed by the add of the load LAG
// loads before it (the compiler emits LAG 2 without and LAG 1 with partial
// unrolling), for 1 or 2 cache lines per iteration.
#define SUM_L1_LAG1 \
    "lw $13, 0($8)\n" "lw $14, 4($8)\n" "addu $9, $9, $13\n" \
    "lw $13, 8($8)\n" "addu $10, $10, $14\n" "lw $14, 12($8)\n" \
    "addu $11, $11, $13\n" "lw $13, 16($8)\n" "addu $12, $12, $14\n" \
    "lw $14, 20($8)\n" "addu $9, $9, $13\n" "lw $13, 24($8)\n" \
    "addu $10, $10, $14\n" "lw $14, 28($8)\n" "addu $11, $11, $13\n" \
    "lw $13, 32($8)\n" "addu $12, $12, $14\n" "lw $14, 36($8)\n" \
    "addu $9, $9, $13\n" "lw $13, 40($8)\n" "addu $10, $10, $14\n" \
    "lw $14, 44($8)\n" "addu $11, $11, $13\n" "lw $13, 48($8)\n" \
    "addu $12, $12, $14\n" "lw $14, 52($8)\n" "addu $9, $9, $13\n" \
    "lw $13, 56($8)\n" "addu $10, $10, $14\n" "lw $14, 60($8)\n" \
    "addu $11, $11, $13\n" "addu $12, $12, $14\n" "addiu $8, $8, 64\n"
#define SUM_L1_LAG2 \
    "lw $13, 0($8)\n" "lw $14, 4($8)\n" "lw $15, 8($8)\n" \
    "addu $9, $9, $13\n" "lw $13, 12($8)\n" "addu $10, $10, $14\n" \
    "lw $14, 16($8)\n" "addu $11, $11, $15\n" "lw $15, 20($8)\n" \
    "addu $12, $12, $13\n" "lw $13, 24($8)\n" "addu $9, $9, $14\n" \
    "lw $14, 28($8)\n" "addu $10, $10, $15\n" "lw $15, 32($8)\n" \
    "addu $11, $11, $13\n" "lw $13, 36($8)\n" "addu $12, $12, $14\n" \
    "lw $14, 40($8)\n" "addu $9, $9, $15\n" "lw $15, 44($8)\n" \
    "addu $10, $10, $13\n" "lw $13, 48($8)\n" "addu $11, $11, $14\n" \
    "lw $14, 52($8)\n" "addu $12, $12, $15\n" "lw $15, 56($8)\n" \
    "addu $9, $9, $13\n" "lw $13, 60($8)\n" "addu $10, $10, $14\n" \
    "addu $11, $11, $15\n" "addu $12, $12, $13\n" "addiu $8, $8, 64\n"
#define SUM_L1_LAG3 \
    "lw $13, 0($8)\n" "lw $14, 4($8)\n" "lw $15, 8($8)\n" \
    "lw $24, 12($8)\n" "addu $9, $9, $13\n" "lw $13, 16($8)\n" \
    "addu $10, $10, $14\n" "lw $14, 20($8)\n" "addu $11, $11, $15\n" \
    "lw $15, 24($8)\n" "addu $12, $12, $24\n" "lw $24, 28($8)\n" \
    "addu $9, $9, $13\n" "lw $13, 32($8)\n" "addu $10, $10, $14\n" \
    "lw $14, 36($8)\n" "addu $11, $11, $15\n" "lw $15, 40($8)\n" \
    "addu $12, $12, $24\n" "lw $24, 44($8)\n" "addu $9, $9, $13\n" \
    "lw $13, 48($8)\n" "addu $10, $10, $14\n" "lw $14, 52($8)\n" \
    "addu $11, $11, $15\n" "lw $15, 56($8)\n" "addu $12, $12, $24\n" \
    "lw $24, 60($8)\n" "addu $9, $9, $13\n" "addu $10, $10, $14\n" \
    "addu $11, $11, $15\n" "addu $12, $12, $24\n" "addiu $8, $8, 64\n"
#define SUM_L1_LAG4 \
    "lw $13, 0($8)\n" "lw $14, 4($8)\n" "lw $15, 8($8)\n" \
    "lw $24, 12($8)\n" "lw $25, 16($8)\n" "addu $9, $9, $13\n" \
    "lw $13, 20($8)\n" "addu $10, $10, $14\n" "lw $14, 24($8)\n" \
    "addu $11, $11, $15\n" "lw $15, 28($8)\n" "addu $12, $12, $24\n" \
    "lw $24, 32($8)\n" "addu $9, $9, $25\n" "lw $25, 36($8)\n" \
    "addu $10, $10, $13\n" "lw $13, 40($8)\n" "addu $11, $11, $14\n" \
    "lw $14, 44($8)\n" "addu $12, $12, $15\n" "lw $15, 48($8)\n" \
    "addu $9, $9, $24\n" "lw $24, 52($8)\n" "addu $10, $10, $25\n" \
    "lw $25, 56($8)\n" "addu $11, $11, $13\n" "lw $13, 60($8)\n" \
    "addu $12, $12, $14\n" "addu $9, $9, $15\n" "addu $10, $10, $24\n" \
    "addu $11, $11, $25\n" "addu $12, $12, $13\n" "addiu $8, $8, 64\n"
#define SUM_L2_LAG1 \
    "lw $13, 0($8)\n" "lw $14, 4($8)\n" "addu $9, $9, $13\n" \
    "lw $13, 8($8)\n" "addu $10, $10, $14\n" "lw $14, 12($8)\n" \
    "addu $11, $11, $13\n" "lw $13, 16($8)\n" "addu $12, $12, $14\n" \
    "lw $14, 20($8)\n" "addu $9, $9, $13\n" "lw $13, 24($8)\n" \
    "addu $10, $10, $14\n" "lw $14, 28($8)\n" "addu $11, $11, $13\n" \
    "lw $13, 32($8)\n" "addu $12, $12, $14\n" "lw $14, 36($8)\n" \
    "addu $9, $9, $13\n" "lw $13, 40($8)\n" "addu $10, $10, $14\n" \
    "lw $14, 44($8)\n" "addu $11, $11, $13\n" "lw $13, 48($8)\n" \
    "addu $12, $12, $14\n" "lw $14, 52($8)\n" "addu $9, $9, $13\n" \
    "lw $13, 56($8)\n" "addu $10, $10, $14\n" "lw $14, 60($8)\n" \
    "addu $11, $11, $13\n" "lw $13, 64($8)\n" "addu $12, $12, $14\n" \
    "lw $14, 68($8)\n" "addu $9, $9, $13\n" "lw $13, 72($8)\n" \
    "addu $10, $10, $14\n" "lw $14, 76($8)\n" "addu $11, $11, $13\n" \
    "lw $13, 80($8)\n" "addu $12, $12, $14\n" "lw $14, 84($8)\n" \
    "addu $9, $9, $13\n" "lw $13, 88($8)\n" "addu $10, $10, $14\n" \
    "lw $14, 92($8)\n" "addu $11, $11, $13\n" "lw $13, 96($8)\n" \
    "addu $12, $12, $14\n" "lw $14, 100($8)\n" "addu $9, $9, $13\n" \
    "lw $13, 104($8)\n" "addu $10, $10, $14\n" "lw $14, 108($8)\n" \
    "addu $11, $11, $13\n" "lw $13, 112($8)\n" "addu $12, $12, $14\n" \
    "lw $14, 116($8)\n" "addu $9, $9, $13\n" "lw $13, 120($8)\n" \
    "addu $10, $10, $14\n" "lw $14, 124($8)\n" "addu $11, $11, $13\n" \
    "addu $12, $12, $14\n" "addiu $8, $8, 128\n"
#define SUM_L2_LAG2 \
    "lw $13, 0($8)\n" "lw $14, 4($8)\n" "lw $15, 8($8)\n" \
    "addu $9, $9, $13\n" "lw $13, 12($8)\n" "addu $10, $10, $14\n" \
    "lw $14, 16($8)\n" "addu $11, $11, $15\n" "lw $15, 20($8)\n" \
    "addu $12, $12, $13\n" "lw $13, 24($8)\n" "addu $9, $9, $14\n" \
    "lw $14, 28($8)\n" "addu $10, $10, $15\n" "lw $15, 32($8)\n" \
    "addu $11, $11, $13\n" "lw $13, 36($8)\n" "addu $12, $12, $14\n" \
    "lw $14, 40($8)\n" "addu $9, $9, $15\n" "lw $15, 44($8)\n" \
    "addu $10, $10, $13\n" "lw $13, 48($8)\n" "addu $11, $11, $14\n" \
    "lw $14, 52($8)\n" "addu $12, $12, $15\n" "lw $15, 56($8)\n" \
    "addu $9, $9, $13\n" "lw $13, 60($8)\n" "addu $10, $10, $14\n" \
    "lw $14, 64($8)\n" "addu $11, $11, $15\n" "lw $15, 68($8)\n" \
    "addu $12, $12, $13\n" "lw $13, 72($8)\n" "addu $9, $9, $14\n" \
    "lw $14, 76($8)\n" "addu $10, $10, $15\n" "lw $15, 80($8)\n" \
    "addu $11, $11, $13\n" "lw $13, 84($8)\n" "addu $12, $12, $14\n" \
    "lw $14, 88($8)\n" "addu $9, $9, $15\n" "lw $15, 92($8)\n" \
    "addu $10, $10, $13\n" "lw $13, 96($8)\n" "addu $11, $11, $14\n" \
    "lw $14, 100($8)\n" "addu $12, $12, $15\n" "lw $15, 104($8)\n" \
    "addu $9, $9, $13\n" "lw $13, 108($8)\n" "addu $10, $10, $14\n" \
    "lw $14, 112($8)\n" "addu $11, $11, $15\n" "lw $15, 116($8)\n" \
    "addu $12, $12, $13\n" "lw $13, 120($8)\n" "addu $9, $9, $14\n" \
    "lw $14, 124($8)\n" "addu $10, $10, $15\n" "addu $11, $11, $13\n" \
    "addu $12, $12, $14\n" "addiu $8, $8, 128\n"
#define SUM_L2_LAG3 \
    "lw $13, 0($8)\n" "lw $14, 4($8)\n" "lw $15, 8($8)\n" \
    "lw $24, 12($8)\n" "addu $9, $9, $13\n" "lw $13, 16($8)\n" \
    "addu $10, $10, $14\n" "lw $14, 20($8)\n" "addu $11, $11, $15\n" \
    "lw $15, 24($8)\n" "addu $12, $12, $24\n" "lw $24, 28($8)\n" \
    "addu $9, $9, $13\n" "lw $13, 32($8)\n" "addu $10, $10, $14\n" \
    "lw $14, 36($8)\n" "addu $11, $11, $15\n" "lw $15, 40($8)\n" \
    "addu $12, $12, $24\n" "lw $24, 44($8)\n" "addu $9, $9, $13\n" \
    "lw $13, 48($8)\n" "addu $10, $10, $14\n" "lw $14, 52($8)\n" \
    "addu $11, $11, $15\n" "lw $15, 56($8)\n" "addu $12, $12, $24\n" \
    "lw $24, 60($8)\n" "addu $9, $9, $13\n" "lw $13, 64($8)\n" \
    "addu $10, $10, $14\n" "lw $14, 68($8)\n" "addu $11, $11, $15\n" \
    "lw $15, 72($8)\n" "addu $12, $12, $24\n" "lw $24, 76($8)\n" \
    "addu $9, $9, $13\n" "lw $13, 80($8)\n" "addu $10, $10, $14\n" \
    "lw $14, 84($8)\n" "addu $11, $11, $15\n" "lw $15, 88($8)\n" \
    "addu $12, $12, $24\n" "lw $24, 92($8)\n" "addu $9, $9, $13\n" \
    "lw $13, 96($8)\n" "addu $10, $10, $14\n" "lw $14, 100($8)\n" \
    "addu $11, $11, $15\n" "lw $15, 104($8)\n" "addu $12, $12, $24\n" \
    "lw $24, 108($8)\n" "addu $9, $9, $13\n" "lw $13, 112($8)\n" \
    "addu $10, $10, $14\n" "lw $14, 116($8)\n" "addu $11, $11, $15\n" \
    "lw $15, 120($8)\n" "addu $12, $12, $24\n" "lw $24, 124($8)\n" \
    "addu $9, $9, $13\n" "addu $10, $10, $14\n" "addu $11, $11, $15\n" \
    "addu $12, $12, $24\n" "addiu $8, $8, 128\n"
#define SUM_L2_LAG4 \
    "lw $13, 0($8)\n" "lw $14, 4($8)\n" "lw $15, 8($8)\n" \
    "lw $24, 12($8)\n" "lw $25, 16($8)\n" "addu $9, $9, $13\n" \
    "lw $13, 20($8)\n" "addu $10, $10, $14\n" "lw $14, 24($8)\n" \
    "addu $11, $11, $15\n" "lw $15, 28($8)\n" "addu $12, $12, $24\n" \
    "lw $24, 32($8)\n" "addu $9, $9, $25\n" "lw $25, 36($8)\n" \
    "addu $10, $10, $13\n" "lw $13, 40($8)\n" "addu $11, $11, $14\n" \
    "lw $14, 44($8)\n" "addu $12, $12, $15\n" "lw $15, 48($8)\n" \
    "addu $9, $9, $24\n" "lw $24, 52($8)\n" "addu $10, $10, $25\n" \
    "lw $25, 56($8)\n" "addu $11, $11, $13\n" "lw $13, 60($8)\n" \
    "addu $12, $12, $14\n" "lw $14, 64($8)\n" "addu $9, $9, $15\n" \
    "lw $15, 68($8)\n" "addu $10, $10, $24\n" "lw $24, 72($8)\n" \
    "addu $11, $11, $25\n" "lw $25, 76($8)\n" "addu $12, $12, $13\n" \
    "lw $13, 80($8)\n" "addu $9, $9, $14\n" "lw $14, 84($8)\n" \
    "addu $10, $10, $15\n" "lw $15, 88($8)\n" "addu $11, $11, $24\n" \
    "lw $24, 92($8)\n" "addu $12, $12, $25\n" "lw $25, 96($8)\n" \
    "addu $9, $9, $13\n" "lw $13, 100($8)\n" "addu $10, $10, $14\n" \
    "lw $14, 104($8)\n" "addu $11, $11, $15\n" "lw $15, 108($8)\n" \
    "addu $12, $12, $24\n" "lw $24, 112($8)\n" "addu $9, $9, $25\n" \
    "lw $25, 116($8)\n" "addu $10, $10, $13\n" "lw $13, 120($8)\n" \
    "addu $11, $11, $14\n" "lw $14, 124($8)\n" "addu $12, $12, $15\n" \
    "addu $9, $9, $24\n" "addu $10, $10, $25\n" "addu $11, $11, $13\n" \
    "addu $12, $12, $14\n" "addiu $8, $8, 128\n"

// As KERNEL, but the loop starts pad words after a 64-byte boundary.
#define KERNEL_PAD(name, setup, body, pad)                                 \
    static u32 name(u32 *p, int iters) {                                   \
        u32 t0, t1;                                                        \
        __asm__ __volatile__(                                              \
            ".set push\n.set noreorder\n.set noat\n"                       \
            setup                                                          \
            "mfc0 %0, $9\n"                                                \
            ".align 6\n"                                                   \
            ".rept " #pad "\nnop\n.endr\n"                                  \
            "1:\n" body                                                    \
            "addiu %2, %2, -1\n"                                           \
            "bnez %2, 1b\n"                                                \
            "nop\n"                                                        \
            "mfc0 %1, $9\n"                                                \
            ".set pop\n"                                                   \
            : "=&r"(t0), "=&r"(t1), "+r"(iters)                            \
            : "r"(p)                                                       \
            : "memory", "$8", "$9", "$10", "$11", "$12", "$13", "$14",     \
              "$15", "$24", "$25");                                                      \
        return t1 - t0;                                                    \
    }

#define SUM_SETUP                                                     \
    "move $8, %3\n" "move $9, $0\n" "move $10, $0\n" "move $11, $0\n" \
    "move $12, $0\n"
KERNEL_PAD(k_sum1_lag1, SUM_SETUP, SUM_L1_LAG1, 0)
KERNEL_PAD(k_sum1_lag2, SUM_SETUP, SUM_L1_LAG2, 0)
KERNEL_PAD(k_sum1_lag3, SUM_SETUP, SUM_L1_LAG3, 0)
KERNEL_PAD(k_sum1_lag4, SUM_SETUP, SUM_L1_LAG4, 0)
KERNEL_PAD(k_sum2_lag1, SUM_SETUP, SUM_L2_LAG1, 0)
KERNEL_PAD(k_sum2_lag2, SUM_SETUP, SUM_L2_LAG2, 0)
KERNEL_PAD(k_sum2_lag3, SUM_SETUP, SUM_L2_LAG3, 0)
KERNEL_PAD(k_sum2_lag4, SUM_SETUP, SUM_L2_LAG4, 0)

// The STREAM Sum loops exactly as compiled, without (SUMC_NOUNROLL) and with
// (SUMC_UNROLL) partial unrolling, in their original registers.
#define SUMC_NOUNROLL \
    "lw $1, -32($2)\n" "lw $8, -28($2)\n" "lw $9, -24($2)\n" \
    "addu $1, $1, $7\n" "lw $7, -20($2)\n" "addu $6, $8, $6\n" \
    "lw $8, -16($2)\n" "addu $5, $9, $5\n" "lw $9, -12($2)\n" \
    "addu $4, $7, $4\n" "lw $7, -8($2)\n" "addu $1, $8, $1\n" \
    "lw $8, -4($2)\n" "addu $6, $9, $6\n" "lw $9, 0($2)\n" \
    "addu $5, $7, $5\n" "lw $7, 4($2)\n" "addu $4, $8, $4\n" \
    "lw $8, 8($2)\n" "addu $1, $9, $1\n" "lw $9, 12($2)\n" \
    "addu $6, $7, $6\n" "lw $7, 16($2)\n" "addu $5, $8, $5\n" \
    "lw $8, 20($2)\n" "addu $4, $9, $4\n" "lw $9, 24($2)\n" \
    "addu $7, $7, $1\n" "lw $1, 28($2)\n" "addu $6, $8, $6\n" \
    "addiu $2, $2, 64\n" "addu $5, $9, $5\n" "bne $2, $3, 2b\n" \
    "addu $4, $1, $4\n"
#define SUMC_UNROLL \
    "lw $1, -64($4)\n" "addiu $3, $3, 32\n" "lw $11, -60($4)\n" \
    "addu $1, $1, $10\n" "lw $10, -56($4)\n" "addu $9, $11, $9\n" \
    "lw $11, -52($4)\n" "addu $8, $10, $8\n" "lw $10, -48($4)\n" \
    "addu $7, $11, $7\n" "lw $11, -44($4)\n" "addu $1, $10, $1\n" \
    "lw $10, -40($4)\n" "addu $9, $11, $9\n" "lw $11, -36($4)\n" \
    "addu $8, $10, $8\n" "lw $10, -32($4)\n" "addu $7, $11, $7\n" \
    "lw $11, -28($4)\n" "addu $1, $10, $1\n" "lw $10, -24($4)\n" \
    "addu $9, $11, $9\n" "lw $11, -20($4)\n" "addu $8, $10, $8\n" \
    "lw $10, -16($4)\n" "addu $7, $11, $7\n" "lw $11, -12($4)\n" \
    "addu $1, $10, $1\n" "lw $10, -8($4)\n" "addu $9, $11, $9\n" \
    "lw $11, -4($4)\n" "addu $8, $10, $8\n" "lw $10, 0($4)\n" \
    "addu $7, $11, $7\n" "lw $11, 4($4)\n" "addu $1, $10, $1\n" \
    "lw $10, 8($4)\n" "addu $9, $11, $9\n" "lw $11, 12($4)\n" \
    "addu $8, $10, $8\n" "lw $10, 16($4)\n" "addu $7, $11, $7\n" \
    "lw $11, 20($4)\n" "addu $1, $10, $1\n" "lw $10, 24($4)\n" \
    "addu $9, $11, $9\n" "lw $11, 28($4)\n" "addu $8, $10, $8\n" \
    "lw $10, 32($4)\n" "addu $7, $11, $7\n" "lw $11, 36($4)\n" \
    "addu $1, $10, $1\n" "lw $10, 40($4)\n" "addu $9, $11, $9\n" \
    "lw $11, 44($4)\n" "addu $8, $10, $8\n" "lw $10, 48($4)\n" \
    "addu $7, $11, $7\n" "lw $11, 52($4)\n" "addu $10, $10, $1\n" \
    "lw $1, 56($4)\n" "addu $9, $11, $9\n" "lw $11, 60($4)\n" \
    "addu $8, $1, $8\n" "addiu $4, $4, 128\n" "bne $4, $6, 2b\n" \
    "addu $7, $11, $7\n"
// The remainder counter update replaced by a nop, or moved to the end.
#define SUMC_UNROLL_NOPFILL \
    "lw $1, -64($4)\n" "nop\n" "lw $11, -60($4)\n" \
    "addu $1, $1, $10\n" "lw $10, -56($4)\n" "addu $9, $11, $9\n" \
    "lw $11, -52($4)\n" "addu $8, $10, $8\n" "lw $10, -48($4)\n" \
    "addu $7, $11, $7\n" "lw $11, -44($4)\n" "addu $1, $10, $1\n" \
    "lw $10, -40($4)\n" "addu $9, $11, $9\n" "lw $11, -36($4)\n" \
    "addu $8, $10, $8\n" "lw $10, -32($4)\n" "addu $7, $11, $7\n" \
    "lw $11, -28($4)\n" "addu $1, $10, $1\n" "lw $10, -24($4)\n" \
    "addu $9, $11, $9\n" "lw $11, -20($4)\n" "addu $8, $10, $8\n" \
    "lw $10, -16($4)\n" "addu $7, $11, $7\n" "lw $11, -12($4)\n" \
    "addu $1, $10, $1\n" "lw $10, -8($4)\n" "addu $9, $11, $9\n" \
    "lw $11, -4($4)\n" "addu $8, $10, $8\n" "lw $10, 0($4)\n" \
    "addu $7, $11, $7\n" "lw $11, 4($4)\n" "addu $1, $10, $1\n" \
    "lw $10, 8($4)\n" "addu $9, $11, $9\n" "lw $11, 12($4)\n" \
    "addu $8, $10, $8\n" "lw $10, 16($4)\n" "addu $7, $11, $7\n" \
    "lw $11, 20($4)\n" "addu $1, $10, $1\n" "lw $10, 24($4)\n" \
    "addu $9, $11, $9\n" "lw $11, 28($4)\n" "addu $8, $10, $8\n" \
    "lw $10, 32($4)\n" "addu $7, $11, $7\n" "lw $11, 36($4)\n" \
    "addu $1, $10, $1\n" "lw $10, 40($4)\n" "addu $9, $11, $9\n" \
    "lw $11, 44($4)\n" "addu $8, $10, $8\n" "lw $10, 48($4)\n" \
    "addu $7, $11, $7\n" "lw $11, 52($4)\n" "addu $10, $10, $1\n" \
    "lw $1, 56($4)\n" "addu $9, $11, $9\n" "lw $11, 60($4)\n" \
    "addu $8, $1, $8\n" "addiu $4, $4, 128\n" "bne $4, $6, 2b\n" \
    "addu $7, $11, $7\n"
#define SUMC_UNROLL_ADDIU_END \
    "lw $1, -64($4)\n" "lw $11, -60($4)\n" \
    "addu $1, $1, $10\n" "lw $10, -56($4)\n" "addu $9, $11, $9\n" \
    "lw $11, -52($4)\n" "addu $8, $10, $8\n" "lw $10, -48($4)\n" \
    "addu $7, $11, $7\n" "lw $11, -44($4)\n" "addu $1, $10, $1\n" \
    "lw $10, -40($4)\n" "addu $9, $11, $9\n" "lw $11, -36($4)\n" \
    "addu $8, $10, $8\n" "lw $10, -32($4)\n" "addu $7, $11, $7\n" \
    "lw $11, -28($4)\n" "addu $1, $10, $1\n" "lw $10, -24($4)\n" \
    "addu $9, $11, $9\n" "lw $11, -20($4)\n" "addu $8, $10, $8\n" \
    "lw $10, -16($4)\n" "addu $7, $11, $7\n" "lw $11, -12($4)\n" \
    "addu $1, $10, $1\n" "lw $10, -8($4)\n" "addu $9, $11, $9\n" \
    "lw $11, -4($4)\n" "addu $8, $10, $8\n" "lw $10, 0($4)\n" \
    "addu $7, $11, $7\n" "lw $11, 4($4)\n" "addu $1, $10, $1\n" \
    "lw $10, 8($4)\n" "addu $9, $11, $9\n" "lw $11, 12($4)\n" \
    "addu $8, $10, $8\n" "lw $10, 16($4)\n" "addu $7, $11, $7\n" \
    "lw $11, 20($4)\n" "addu $1, $10, $1\n" "lw $10, 24($4)\n" \
    "addu $9, $11, $9\n" "lw $11, 28($4)\n" "addu $8, $10, $8\n" \
    "lw $10, 32($4)\n" "addu $7, $11, $7\n" "lw $11, 36($4)\n" \
    "addu $1, $10, $1\n" "lw $10, 40($4)\n" "addu $9, $11, $9\n" \
    "lw $11, 44($4)\n" "addu $8, $10, $8\n" "lw $10, 48($4)\n" \
    "addu $7, $11, $7\n" "lw $11, 52($4)\n" "addu $10, $10, $1\n" \
    "lw $1, 56($4)\n" "addu $9, $11, $9\n" "lw $11, 60($4)\n" \
    "addu $8, $1, $8\n" "addiu $3, $3, 32\n" "addiu $4, $4, 128\n" "bne $4, $6, 2b\n" \
    "addu $7, $11, $7\n"
// Without the remainder counter update that starts the unrolled loop.
#define SUMC_UNROLL_NOADDIU \
    "lw $1, -64($4)\n" "lw $11, -60($4)\n" \
    "addu $1, $1, $10\n" "lw $10, -56($4)\n" "addu $9, $11, $9\n" \
    "lw $11, -52($4)\n" "addu $8, $10, $8\n" "lw $10, -48($4)\n" \
    "addu $7, $11, $7\n" "lw $11, -44($4)\n" "addu $1, $10, $1\n" \
    "lw $10, -40($4)\n" "addu $9, $11, $9\n" "lw $11, -36($4)\n" \
    "addu $8, $10, $8\n" "lw $10, -32($4)\n" "addu $7, $11, $7\n" \
    "lw $11, -28($4)\n" "addu $1, $10, $1\n" "lw $10, -24($4)\n" \
    "addu $9, $11, $9\n" "lw $11, -20($4)\n" "addu $8, $10, $8\n" \
    "lw $10, -16($4)\n" "addu $7, $11, $7\n" "lw $11, -12($4)\n" \
    "addu $1, $10, $1\n" "lw $10, -8($4)\n" "addu $9, $11, $9\n" \
    "lw $11, -4($4)\n" "addu $8, $10, $8\n" "lw $10, 0($4)\n" \
    "addu $7, $11, $7\n" "lw $11, 4($4)\n" "addu $1, $10, $1\n" \
    "lw $10, 8($4)\n" "addu $9, $11, $9\n" "lw $11, 12($4)\n" \
    "addu $8, $10, $8\n" "lw $10, 16($4)\n" "addu $7, $11, $7\n" \
    "lw $11, 20($4)\n" "addu $1, $10, $1\n" "lw $10, 24($4)\n" \
    "addu $9, $11, $9\n" "lw $11, 28($4)\n" "addu $8, $10, $8\n" \
    "lw $10, 32($4)\n" "addu $7, $11, $7\n" "lw $11, 36($4)\n" \
    "addu $1, $10, $1\n" "lw $10, 40($4)\n" "addu $9, $11, $9\n" \
    "lw $11, 44($4)\n" "addu $8, $10, $8\n" "lw $10, 48($4)\n" \
    "addu $7, $11, $7\n" "lw $11, 52($4)\n" "addu $10, $10, $1\n" \
    "lw $1, 56($4)\n" "addu $9, $11, $9\n" "lw $11, 60($4)\n" \
    "addu $8, $1, $8\n" "addiu $4, $4, 128\n" "bne $4, $6, 2b\n" \
    "addu $7, $11, $7\n"

// Runs loop once over n lines; setup sets up its pointer and end registers.
#define KERNEL_RAW(name, setup, loop)                                      \
    static u32 name(u32 *p, int n) {                                      \
        u32 t0, t1;                                                        \
        __asm__ __volatile__(                                              \
            ".set push\n.set noreorder\n.set noat\n"                       \
            setup                                                          \
            "mfc0 %0, $9\n"                                                \
            ".align 3\n"                                                   \
            "2:\n" loop                                                    \
            "mfc0 %1, $9\n"                                                \
            ".set pop\n"                                                   \
            : "=&r"(t0), "=&r"(t1)                                         \
            : "r"(p), "r"(n)                                               \
            : "memory", "$1", "$2", "$3", "$4", "$5", "$6", "$7", "$8",    \
              "$9", "$10", "$11");                                         \
        return t1 - t0;                                                    \
    }

// As KERNEL_RAW, but the loop starts at an odd word (the nop runs once).
#define KERNEL_RAW_ODD(name, setup, loop)                                      \
    static u32 name(u32 *p, int n) {                                      \
        u32 t0, t1;                                                        \
        __asm__ __volatile__(                                              \
            ".set push\n.set noreorder\n.set noat\n"                       \
            setup                                                          \
            "mfc0 %0, $9\n"                                                \
            ".align 3\n" "nop\n"                                        \
            "2:\n" loop                                                    \
            "mfc0 %1, $9\n"                                                \
            ".set pop\n"                                                   \
            : "=&r"(t0), "=&r"(t1)                                         \
            : "r"(p), "r"(n)                                               \
            : "memory", "$1", "$2", "$3", "$4", "$5", "$6", "$7", "$8",    \
              "$9", "$10", "$11");                                         \
        return t1 - t0;                                                    \
    }

// No unroll: $2 = p + 32 walks the lines, $3 = its end, $4-$7 are the sums.
// Unrolled: $4 = p + 64 walks pairs of lines, $6 = its end, $1 and $7-$10
// are the sums.
#define SUMC_UNROLL_SETUP                                               \
    "addiu $4, %2, 64\n" "sll $6, %3, 6\n" "addu $6, $6, $4\n"            \
    "move $3, $0\n" "move $7, $0\n" "move $8, $0\n" "move $9, $0\n"      \
    "move $10, $0\n"
#define SUMC_NOUNROLL_SETUP                                             \
    "addiu $2, %2, 32\n" "sll $3, %3, 6\n" "addu $3, $3, $2\n"            \
    "move $4, $0\n" "move $5, $0\n" "move $6, $0\n" "move $7, $0\n"
KERNEL_RAW(k_sumc_nounroll, SUMC_NOUNROLL_SETUP, SUMC_NOUNROLL)
KERNEL_RAW(k_sumc_unroll, SUMC_UNROLL_SETUP, SUMC_UNROLL)
KERNEL_RAW(k_sumc_unroll_noaddiu, SUMC_UNROLL_SETUP, SUMC_UNROLL_NOADDIU)
// A nop at the start of the loop shifts which instructions issue together.
KERNEL_RAW(k_sumc_unroll_nop, SUMC_UNROLL_SETUP, "nop\n" SUMC_UNROLL)
KERNEL_RAW(k_sumc_nounroll_nop, SUMC_NOUNROLL_SETUP, "nop\n" SUMC_NOUNROLL)
KERNEL_RAW_ODD(k_sumc_unroll_odd, SUMC_UNROLL_SETUP, SUMC_UNROLL)
KERNEL_RAW(k_sumc_unroll_nopfill, SUMC_UNROLL_SETUP, SUMC_UNROLL_NOPFILL)
KERNEL_RAW(k_sumc_unroll_addiu_end, SUMC_UNROLL_SETUP, SUMC_UNROLL_ADDIU_END)
KERNEL_RAW_ODD(k_sumc_nounroll_odd, SUMC_NOUNROLL_SETUP, SUMC_NOUNROLL)

// --- Issue pairing of aligned 8-byte blocks ---
// T3: a load and its use share a block, then a block of 2 independent ALU ops.
// 4 cycles if only the 2 instructions of an aligned block issue together, 3 if
// any 2 consecutive instructions can.
#define T3 "lw $9, 0($24)\n" "addu $10, $9, $11\n" "addu $12, $13, $14\n" \
           "addu $15, $13, $14\n"
// T7: 4 cycles if a block's first instruction issues alone when its second is
// not ready, 5 if the block waits as a whole.
#define T7 "lw $9, 0($24)\n" "nop\n" "lw $12, 4($24)\n" "addu $10, $9, $11\n" \
           "addu $13, $12, $11\n" "nop\n"
#define REP64(x) ".rept 64\n" x ".endr\n"
KERNEL_PAD(k_t3_even, MMI_SETUP, REP64(T3), 0)
KERNEL_PAD(k_t3_odd, MMI_SETUP, REP64(T3), 1)
KERNEL_PAD(k_t7_even, MMI_SETUP, REP64(T7), 0)
KERNEL_PAD(k_t7_odd, MMI_SETUP, REP64(T7), 1)

// Loops of N pairable blocks [addu | lw] plus addiu, bnez, nop: the branch
// and its delay slot are in different blocks, or (with a nop added) the same.
#define PAIRS8 ".rept 8\n" "addu $10, $12, $13\n" "lw $9, 0($24)\n" ".endr\n"
#define PAIRS32 ".rept 32\n" "addu $10, $12, $13\n" "lw $9, 0($24)\n" ".endr\n"
KERNEL(k_loop8, 1, MMI_SETUP, PAIRS8)
KERNEL(k_loop8_nop, 1, MMI_SETUP, PAIRS8 "nop\n")
KERNEL(k_loop32, 1, MMI_SETUP, PAIRS32)
KERNEL(k_loop32_nop, 1, MMI_SETUP, PAIRS32 "nop\n")

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

static void run_table(const char *title, const uarch_test *t, unsigned n,
                      u32 *p) {
    printf("--- %s ---\n", title);
    for (unsigned i = 0; i < n; i++)
        print_line(t[i].name, run(t[i].fn, p, LAT_ITERS, t[i].reps), t[i].unit,
                   t[i].expect);
}

// --- Hazard checks ---
// MIPS I-III need an instruction between c.cond.s and bc1t/bc1f, and between
// mfc1 and the use of its result. These run the pairs back to back and check
// that the new value is used; the condition and the value change every
// repetition.

// $f8 = 1.0, $f9 = 1.0 or 2.0 alternating; sums the repetition counts at
// which bc1t is taken. A stale condition would give the other half.
static u32 hazard_fcc(int reps) {
    u32 sum;
    __asm__ __volatile__(
        ".set push\n.set noreorder\n.set noat\n"
        "li $8, 0x3f800000\n" "mtc1 $8, $f8\n"
        "li $9, 0x40000000\n"
        "move %0, $0\n"
        "1:\n"
        "andi $10, %1, 1\n"
        "movn $11, $8, $10\n" "movz $11, $9, $10\n"
        "mtc1 $11, $f9\n"
        "nop\n" "nop\n" "nop\n" "nop\n"
        "c.eq.s $f8, $f9\n"
        "bc1t 2f\n"
        "nop\n"
        "b 3f\n"
        "nop\n"
        "2:\n"
        "addu %0, %0, %1\n"
        "3:\n"
        "addiu %1, %1, -1\n"
        "bnez %1, 1b\n"
        "nop\n"
        ".set pop\n"
        : "=&r"(sum), "+r"(reps)
        :
        : "$8", "$9", "$10", "$11", "$f8", "$f9", "memory");
    return sum;
}

// $f1 = the repetition count; sums mfc1's result through an addu right after
static u32 hazard_mfc1(int reps) {
    u32 sum;
    __asm__ __volatile__(
        ".set push\n.set noreorder\n.set noat\n"
        "move %0, $0\n"
        "1:\n"
        "mtc1 %1, $f1\n"
        "nop\n" "nop\n" "nop\n" "nop\n"
        "mfc1 $9, $f1\n"
        "addu %0, %0, $9\n"
        "move $9, $0\n"
        "addiu %1, %1, -1\n"
        "bnez %1, 1b\n"
        "nop\n"
        ".set pop\n"
        : "=&r"(sum), "+r"(reps)
        :
        : "$9", "$f1", "memory");
    return sum;
}

// mtc1 the repetition count, mov.s it right after; sums it through mfc1
static u32 hazard_mtc1(int reps) {
    u32 sum;
    __asm__ __volatile__(
        ".set push\n.set noreorder\n.set noat\n"
        "move %0, $0\n"
        "mtc1 $0, $f1\n"
        "1:\n"
        "mtc1 %1, $f1\n"
        "mov.s $f2, $f1\n"
        "nop\n" "nop\n" "nop\n" "nop\n"
        "mfc1 $9, $f2\n"
        "nop\n" "nop\n"
        "addu %0, %0, $9\n"
        "addiu %1, %1, -1\n"
        "bnez %1, 1b\n"
        "nop\n"
        ".set pop\n"
        : "=&r"(sum), "+r"(reps)
        :
        : "$9", "$f1", "$f2", "memory");
    return sum;
}

void uarch_run_all(u32 *big, int big_words) {
    // reps = 1000: bc1t is taken at the odd counts, 1 + 3 + ... + 999; the
    // mfc1 sum is 1 + 2 + ... + 1000
    printf("--- Hazard checks ---\n");
    printf("Result: c.eq.s,bc1t sum %u (expect 250000)\n",
           (unsigned)hazard_fcc(1000));
    printf("Result: mfc1,addu sum %u (expect 500500)\n",
           (unsigned)hazard_mfc1(1000));
    printf("Result: mtc1,mov.s sum %u (expect 500500)\n",
           (unsigned)hazard_mtc1(1000));
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

    static const uarch_test intx[] = {
        {"div+mflo", k_div, 16, "2instr", "?"},
        {"div", k_div_x1, 16, "instr", "throughput"},
        {"div,addu x10", k_div_addu, 16, "group", "= div: ALU not blocked"},
        {"div1+mflo1", k_div1, 16, "2instr", "?"},
        {"div|div1", k_div_div1, 16, "pair", "= div: in parallel"},
        {"madd ACC->ACC", k_madd_acc, 64, "instr", "?"},
        {"madd rd->rs", k_madd_rd, 64, "instr", "?"},
        {"madd1 ACC->ACC", k_madd1_acc, 64, "instr", "?"},
        {"mtlo+madd", k_mtlo_madd, 64, "2instr", "?"},
        {"mult1+mflo1", k_mult1_mflo1, 64, "2instr", "?"},
        {"pmaddw ACC->ACC", k_pmaddw_acc, 64, "instr", "?"},
        {"pmultw+pmfhl.lw", k_pmultw_pmfhl, 64, "2instr", "?"},
        {"qfsrv", k_qfsrv, 64, "instr", "?"},
        {"mtsab+qfsrv", k_mtsab_qfsrv, 64, "2instr", "?"},
        {"qfsrv|addu", k_qfsrv_addu, 64, "pair", "1 = paired"},
        {"qfsrv|lw", k_qfsrv_lw, 64, "pair", "1 = paired"},
        {"ldl+ldr", k_ldl_ldr, 64, "2instr", "?"},
        {"ldl+ldr->addu", k_ldl_ldr_use, 64, "rep", "?"},
    };
    static const uarch_test fpbr[] = {
        {"max.s", k_max_s, 64, "instr", "?"},
        {"min.s", k_min_s, 64, "instr", "?"},
        {"rsqrt.s", k_rsqrt_s, 64, "instr", "?"},
        {"c.eq.s,bc1f (not taken)", k_ceq_bc1f, 64, "rep", "?"},
        {"c.eq.s,bc1t (taken)", k_ceq_bc1t, 64, "rep", "?"},
        {"c.eq.s,2 addu,bc1f", k_ceq_x2_bc1f, 64, "rep", "?"},
        {"bne,nop (not taken)", k_br_nt, 64, "rep", "?"},
        {"beq,nop (taken)", k_br_t, 64, "rep", "?"},
        {"addu,bne,addu", k_br_nt_addu, 64, "rep", "?"},
        {"random branch", k_br_rand, 16, "rep", "50% mispredicted"},
        {"same, always taken", k_br_rand_base, 16, "rep",
         "difference x2 = penalty"},
    };
    static const uarch_test vu0[] = {
        {"vadd", k_vadd, 64, "instr", "?"},
        {"vmul", k_vmul, 64, "instr", "?"},
        {"vmadd", k_vmadd, 64, "instr", "?"},
        {"vmax", k_vmax, 64, "instr", "?"},
        {"vmadda ACC->ACC", k_vmadda_acc, 64, "instr", "?"},
        {"vmula+vmadd", k_vmula_vmadd, 64, "2instr", "?"},
        {"qmtc2->vadd", k_qmtc2_vadd, 64, "rep", "= latency"},
        {"vadd->qmfc2", k_vadd_qmfc2, 64, "rep", "= latency"},
        {"lqc2->vadd", k_lqc2_vadd, 64, "rep", "= latency"},
        {"vadd->sqc2", k_vadd_sqc2, 64, "rep", "= latency"},
        {"vdiv+vwaitq", k_vdiv_wait, 16, "rep", "?"},
        {"vsqrt+vwaitq", k_vsqrt_wait, 16, "rep", "?"},
        {"vrsqrt+vwaitq", k_vrsqrt_wait, 16, "rep", "?"},
        {"vdiv,vadd x6,vwaitq", k_vdiv_vadd6, 16, "group",
         "= vdiv: not blocked"},
        {"vadd|vadd", k_vadd_vadd, 64, "pair", "1 = paired"},
        {"vadd|addu", k_vadd_addu, 64, "pair", "1 = paired"},
        {"vadd|lw", k_vadd_lw, 64, "pair", "1 = paired"},
        {"vadd|lqc2", k_vadd_lqc2, 64, "pair", "1 = paired"},
        {"vadd|mul.s", k_vadd_mul_s, 64, "pair", "1 = paired"},
        {"vadd|paddw", k_vadd_paddw, 64, "pair", "1 = paired"},
    };

    run_table("Divide, multiply-add, HI/LO, SA, ldl/ldr", intx,
              sizeof(intx) / sizeof(intx[0]), buf);
    run_table("FPU min/max/rsqrt, compare to branch, branches", fpbr,
              sizeof(fpbr) / sizeof(fpbr[0]), buf);
    static const uarch_test issue[] = {
        {"T3, even", k_t3_even, 64, "rep", "4 = aligned blocks, 3 = sliding"},
        {"T3, odd", k_t3_odd, 64, "rep", "3"},
        {"T7, even", k_t7_even, 64, "rep", "4 = first issues alone, 5 = waits"},
        {"T7, odd", k_t7_odd, 64, "rep", "?"},
        {"loop 8 pairs", k_loop8, 1, "iter", "branch, delay slot in 2 blocks"},
        {"loop 8 pairs + nop", k_loop8_nop, 1, "iter", "in 1 block"},
        {"loop 32 pairs", k_loop32, 1, "iter", "branch, delay slot in 2 blocks"},
        {"loop 32 pairs + nop", k_loop32_nop, 1, "iter", "in 1 block"},
    };
    run_table("Issue pairing of aligned 8-byte blocks", issue,
              sizeof(issue) / sizeof(issue[0]), buf);
    u32 status;
    __asm__ __volatile__("mfc0 %0, $12" : "=r"(status));
    if (status & (1u << 30))  // Status.CU2
        run_table("VU0 macro mode", vu0, sizeof(vu0) / sizeof(vu0[0]), buf);
    else
        printf("--- VU0 macro mode: skipped, COP2 not usable ---\n");

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
    printf("--- STREAM Sum loop, load to add distance (cyc/line) ---\n");
    static const struct {
        const char *name;
        kernel_fn fn;
        int lines;
    } sums[] = {
        {"1 line, add lags 1 load", k_sum1_lag1, 1},
        {"1 line, add lags 2 loads", k_sum1_lag2, 1},
        {"1 line, add lags 3 loads", k_sum1_lag3, 1},
        {"1 line, add lags 4 loads", k_sum1_lag4, 1},
        {"2 lines, add lags 1 load", k_sum2_lag1, 2},
        {"2 lines, add lags 2 loads", k_sum2_lag2, 2},
        {"2 lines, add lags 3 loads", k_sum2_lag3, 2},
        {"2 lines, add lags 4 loads", k_sum2_lag4, 2},
    };
    for (unsigned i = 0; i < sizeof(sums) / sizeof(sums[0]); i++)
        print_line(sums[i].name,
                   run(sums[i].fn, big, lines / sums[i].lines, sums[i].lines),
                   "line", "16 lw+addu per line, not cached");
    print_line("compiled, no unroll", run(k_sumc_nounroll, big, lines, 1),
               "line", "stream_sum loop as compiled");
    print_line("compiled, unrolled", run(k_sumc_unroll, big, lines, 1),
               "line", "stream_sum loop, unrolled");
    print_line("unrolled, no addiu", run(k_sumc_unroll_noaddiu, big, lines, 1),
               "line", "without the remainder counter");
    print_line("unrolled, nop first", run(k_sumc_unroll_nop, big, lines, 1),
               "line", "pairing shifted by one");
    print_line("no unroll, nop first", run(k_sumc_nounroll_nop, big, lines, 1),
               "line", "pairing shifted by one");
    print_line("unrolled, odd word", run(k_sumc_unroll_odd, big, lines, 1),
               "line", "same code, loop at an odd word");
    print_line("unrolled, addiu->nop", run(k_sumc_unroll_nopfill, big, lines, 1),
               "line", "same slots, nop instead of addiu");
    print_line("unrolled, addiu at end", run(k_sumc_unroll_addiu_end, big, lines, 1),
               "line", "addiu moved to the end");
    print_line("no unroll, odd word", run(k_sumc_nounroll_odd, big, lines, 1),
               "line", "same code, loop at an odd word");
    // 64 lines (4 KB) that stay in the data cache
    print_line("unrolled, cached", run(k_sumc_unroll, big, 64, 1), "line",
               "in cache");
    print_line("unrolled+nop, cached", run(k_sumc_unroll_nop, big, 64, 1),
               "line", "in cache");
    printf("--- Overlapping misses (cyc/iteration) ---\n");
    print_line("1 miss", run(k_miss1, big, lines / 2, 1), "iter",
               "one load miss");
    print_line("2 misses", run(k_miss2, big, lines / 2, 1), "iter",
               "= 1 miss: overlap, 2x: serialized");
    printf("\n");
}
