// Kernels of emulators of older consoles: CPU interpreters, graphics format
// conversion and guest memory access.
#include "app_tests.h"

// --- 8-bit CPU interpreter (a subset of the 6502, with its flags) ---

static u8 mem8[4096] __attribute__((aligned(64)));

// Sums and scrambles 192 bytes of the zero page 16 times
static const u8 prog8[] = {
    0xa0, 16,          // LDY #16
    0xa2, 0x00,        // start: LDX #0
    0xb5, 0x10,        // loop: LDA $10,X
    0x69, 0x37,        // ADC #$37
    0x49, 0x5a,        // EOR #$5A
    0x0a,              // ASL A
    0x95, 0x10,        // STA $10,X
    0x4a,              // LSR A
    0x65, 0x08,        // ADC $08
    0x85, 0x08,        // STA $08
    0xe8,              // INX
    0xe0, 0xc0,        // CPX #$C0
    0xd0, 0xed,        // BNE loop
    0x88,              // DEY
    0xd0, 0xe8,        // BNE start
    0x00,              // BRK
};
// LDY, 16 x (LDX, 192 x 11, DEY, BNE), BRK
#define CPU8_INSTRS (1 + 16 * (1 + 192 * 11 + 2) + 1)

static void init_cpu8(void) {
    for (int i = 0; i < 4096; i++)
        mem8[i] = app_rand() >> 24;
    for (unsigned i = 0; i < sizeof(prog8); i++)
        mem8[0x200 + i] = prog8[i];
    mem8[0x08] = 0;
}

static u32 cpu8(void) {
    u32 a = 0, x = 0, y = 0, c = 0, v = 0, nz = 1, pc = 0x200;
    for (;;) {
        u32 op = mem8[pc], m = mem8[(pc + 1) & 0xfff], t;
        pc += 1;
        switch (op) {
        case 0xa9: a = nz = m; pc++; break;                          // LDA #
        case 0xb5: a = nz = mem8[(m + x) & 0xff]; pc++; break;       // LDA zp,X
        case 0x85: mem8[m] = a; pc++; break;                         // STA zp
        case 0x95: mem8[(m + x) & 0xff] = a; pc++; break;            // STA zp,X
        case 0xa2: x = nz = m; pc++; break;                          // LDX #
        case 0xa0: y = nz = m; pc++; break;                          // LDY #
        case 0x65: m = mem8[m]; /* fall through */
        case 0x69:                                                   // ADC
            t = a + m + c;
            v = (~(a ^ m) & (a ^ t)) & 0x80;
            c = t >> 8;
            a = nz = t & 0xff;
            pc++;
            break;
        case 0xe9:                                                   // SBC #
            m ^= 0xff;
            t = a + m + c;
            v = (~(a ^ m) & (a ^ t)) & 0x80;
            c = t >> 8;
            a = nz = t & 0xff;
            pc++;
            break;
        case 0x29: a = nz = a & m; pc++; break;                      // AND #
        case 0x49: a = nz = a ^ m; pc++; break;                      // EOR #
        case 0x0a: c = a >> 7; a = nz = (a << 1) & 0xff; break;      // ASL A
        case 0x4a: c = a & 1; a = nz = a >> 1; break;                // LSR A
        case 0xc9: t = a - m; c = a >= m; nz = t & 0xff; pc++; break; // CMP #
        case 0xe0: t = x - m; c = x >= m; nz = t & 0xff; pc++; break; // CPX #
        case 0xe8: x = nz = (x + 1) & 0xff; break;                   // INX
        case 0xca: x = nz = (x - 1) & 0xff; break;                   // DEX
        case 0xc8: y = nz = (y + 1) & 0xff; break;                   // INY
        case 0x88: y = nz = (y - 1) & 0xff; break;                   // DEY
        case 0xaa: x = nz = a; break;                                // TAX
        case 0x8a: a = nz = x; break;                                // TXA
        case 0x18: c = 0; break;                                     // CLC
        case 0x38: c = 1; break;                                     // SEC
        case 0xd0: pc++; if (nz) pc += (s8)m; break;                 // BNE
        case 0x90: pc++; if (!c) pc += (s8)m; break;                 // BCC
        case 0xb0: pc++; if (c) pc += (s8)m; break;                  // BCS
        default:                                                     // BRK
            return app_hash(mem8, 256) ^ a ^ (x << 8) ^ (y << 16) ^
                   (c << 24) ^ (v << 25);
        }
        pc &= 0xfff;
    }
}

// --- MIPS R3000 (PS1) interpreter, with branch delay slots ---

#define GUEST_WORDS 4096  // 16 KB of guest RAM
static u32 ram[GUEST_WORDS] __attribute__((aligned(64)));

#define R(op, rs, rt, rd, sa, fn) \
    ((op) << 26 | (rs) << 21 | (rt) << 16 | (rd) << 11 | (sa) << 6 | (fn))
#define I(op, rs, rt, imm) ((op) << 26 | (rs) << 21 | (rt) << 16 | ((imm) & 0xffff))

// Checksums and rewrites 512 words (2 KB) at 0x1800. The data cache has 4 KB
// per way: with 4 KB of guest data, or data a multiple of 4 KB from the guest
// code, the guest code and the jump tables of the interpreter competed for one
// way, so the speed depended on where the linker put the tables.
static const u32 prog_r3000[] = {
    I(0x09, 0, 8, 0x1800),     // addiu $t0, $zero, 0x1800
    I(0x09, 0, 9, 512),        // addiu $t1, $zero, 512
    I(0x09, 0, 2, 0),          // addiu $v0, $zero, 0
    I(0x0f, 0, 3, 0x1234),     // lui   $v1, 0x1234
    I(0x23, 8, 10, 0),         // loop: lw $t2, 0($t0)
    I(0x09, 9, 9, -1),         // addiu $t1, $t1, -1
    R(0, 0, 10, 11, 3, 0x00),  // sll   $t3, $t2, 3
    R(0, 2, 11, 2, 0, 0x26),   // xor   $v0, $v0, $t3
    R(0, 0, 2, 12, 5, 0x02),   // srl   $t4, $v0, 5
    R(0, 2, 12, 2, 0, 0x21),   // addu  $v0, $v0, $t4
    I(0x0c, 10, 13, 0xff),     // andi  $t5, $t2, 0xff
    R(0, 2, 13, 2, 0, 0x21),   // addu  $v0, $v0, $t5
    R(0, 2, 3, 14, 0, 0x2a),   // slt   $t6, $v0, $v1
    R(0, 2, 14, 2, 0, 0x23),   // subu  $v0, $v0, $t6
    I(0x2b, 8, 2, 0),          // sw    $v0, 0($t0)
    I(0x05, 9, 0, -12),        // bne   $t1, $zero, loop
    I(0x09, 8, 8, 4),          // addiu $t0, $t0, 4 (delay slot)
    R(0, 0, 0, 0, 0, 0x0c),    // syscall: stop
};
#define R3000_INSTRS (4 + 512 * 13 + 1)

static void init_r3000(void) {
    for (int i = 0; i < GUEST_WORDS; i++)
        ram[i] = app_rand();
    for (unsigned i = 0; i < sizeof(prog_r3000) / 4; i++)
        ram[i] = prog_r3000[i];
}

static u32 r3000(void) {
    u32 r[32] = {0};
    u32 pc = 0, npc = 4;
    for (;;) {
        u32 in = ram[(pc >> 2) & (GUEST_WORDS - 1)];
        u32 rs = (in >> 21) & 31, rt = (in >> 16) & 31, rd = (in >> 11) & 31;
        u32 imm = (u32)(s32)(s16)in;
        pc = npc;
        npc += 4;
        switch (in >> 26) {
        case 0x00:
            switch (in & 63) {
            case 0x00: r[rd] = r[rt] << ((in >> 6) & 31); break;
            case 0x02: r[rd] = r[rt] >> ((in >> 6) & 31); break;
            case 0x0c: return app_hash(&ram[0x600], 2048) ^ r[2];
            case 0x21: r[rd] = r[rs] + r[rt]; break;
            case 0x23: r[rd] = r[rs] - r[rt]; break;
            case 0x24: r[rd] = r[rs] & r[rt]; break;
            case 0x25: r[rd] = r[rs] | r[rt]; break;
            case 0x26: r[rd] = r[rs] ^ r[rt]; break;
            case 0x2a: r[rd] = (s32)r[rs] < (s32)r[rt]; break;
            }
            break;
        case 0x04: if (r[rs] == r[rt]) npc = pc + (imm << 2); break;  // beq
        case 0x05: if (r[rs] != r[rt]) npc = pc + (imm << 2); break;  // bne
        case 0x09: r[rt] = r[rs] + imm; break;                        // addiu
        case 0x0a: r[rt] = (s32)r[rs] < (s32)imm; break;              // slti
        case 0x0c: r[rt] = r[rs] & (in & 0xffff); break;              // andi
        case 0x0d: r[rt] = r[rs] | (in & 0xffff); break;              // ori
        case 0x0f: r[rt] = in << 16; break;                           // lui
        case 0x23: r[rt] = ram[((r[rs] + imm) >> 2) & (GUEST_WORDS - 1)]; break;
        case 0x2b: ram[((r[rs] + imm) >> 2) & (GUEST_WORDS - 1)] = r[rt]; break;
        }
        r[0] = 0;
    }
}

// --- SNES 4bpp planar tiles to 8bpp ---
// A tile is 8x8 pixels in 32 bytes: rows of planes 0/1, then of planes 2/3.

#define NUM_TILES 128
static u8 tiles[NUM_TILES * 32] __attribute__((aligned(64)));
static u8 chunky[NUM_TILES * 64] __attribute__((aligned(64)));

static void init_tiles(void) {
    for (int i = 0; i < NUM_TILES * 32; i++)
        tiles[i] = app_rand() >> 24;
}

static u32 tile_decode(void) {
    for (int t = 0; t < NUM_TILES; t++) {
        const u8 *in = &tiles[t * 32];
        u8 *out = &chunky[t * 64];
        for (int y = 0; y < 8; y++) {
            u32 p0 = in[y * 2], p1 = in[y * 2 + 1];
            u32 p2 = in[16 + y * 2], p3 = in[16 + y * 2 + 1];
            for (int x = 0; x < 8; x++) {
                int b = 7 - x;
                out[y * 8 + x] = ((p0 >> b) & 1) | (((p1 >> b) & 1) << 1) |
                                 (((p2 >> b) & 1) << 2) |
                                 (((p3 >> b) & 1) << 3);
            }
        }
    }
    return app_hash(chunky, sizeof(chunky));
}

// --- N64 16-bit big-endian RGBA5551 (red high) to GS PSMCT16 (red low) ---

#define N64_PIXELS 4096
static u16 n64_tex[N64_PIXELS] __attribute__((aligned(64)));
static u16 ct16[N64_PIXELS] __attribute__((aligned(64)));

static void init_n64(void) {
    for (int i = 0; i < N64_PIXELS; i++)
        n64_tex[i] = app_rand() >> 16;
}

static u32 n64_to_ct16(void) {
    for (int i = 0; i < N64_PIXELS; i++) {
        u32 p = n64_tex[i];
        p = ((p & 0xff) << 8) | (p >> 8);
        ct16[i] = ((p >> 11) & 0x1f) | ((p >> 6) & 0x1f) << 5 |
                  ((p >> 1) & 0x1f) << 10 | (p & 1) << 15;
    }
    return app_hash(ct16, sizeof(ct16));
}

// --- Big-endian guest loads through a page table (4 KB pages) ---

#define GUEST_PAGES 8
#define GUEST_LOADS 4096
static u32 host_pages[GUEST_PAGES][1024] __attribute__((aligned(64)));
static u32 *page_table[GUEST_PAGES];
static u32 guest_addr[GUEST_LOADS];

static void init_guest_mem(void) {
    for (int p = 0; p < GUEST_PAGES; p++) {
        page_table[p] = host_pages[(p * 5) % GUEST_PAGES];
        for (int i = 0; i < 1024; i++)
            host_pages[p][i] = app_rand();
    }
    // Mostly sequential, with a jump every 16 loads
    u32 addr = 0;
    for (int i = 0; i < GUEST_LOADS; i++) {
        if ((i & 15) == 0)
            addr = app_rand() & (GUEST_PAGES * 4096 - 1) & ~3u;
        guest_addr[i] = addr;
        addr = (addr + 4) & (GUEST_PAGES * 4096 - 1);
    }
}

static u32 guest_loads(void) {
    u32 sum = 0;
    for (int i = 0; i < GUEST_LOADS; i++) {
        u32 addr = guest_addr[i];
        u32 w = page_table[addr >> 12][(addr & 4095) >> 2];
        w = (w >> 24) | ((w >> 8) & 0xff00) | ((w << 8) & 0xff0000) | (w << 24);
        sum = (sum << 1 | sum >> 31) + w;
    }
    return sum;
}

static const app_test emu_tests[] = {
    {"6502 Interp", "MIPS", CPU8_INSTRS, init_cpu8, cpu8},
    {"R3000 Interp", "MIPS", R3000_INSTRS, init_r3000, r3000},
    {"SNES Tiles", "Mpix/s", NUM_TILES * 64, init_tiles, tile_decode},
    {"N64 To CT16", "Mpix/s", N64_PIXELS, init_n64, n64_to_ct16},
    {"Guest Loads", "Mload/s", GUEST_LOADS, init_guest_mem, guest_loads},
};
const app_suite emu_suite = {"EMU", "Emulation", emu_tests,
                             sizeof(emu_tests) / sizeof(emu_tests[0])};
