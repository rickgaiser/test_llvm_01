// Kernels of ps2-llvm-benchmarks (git.techwritescode.dev/techwritescode/
// ps2-llvm-benchmarks, bench/src/main.c): a word copy loop, GIF packet
// building, an LZ77 decoder, particle physics and a triangle rasterizer.
// Changes: the random triangles are made in init instead of in the timed
// loop, the particle arrays are padded so that they do not all fall in the
// same D-cache set (they are 4 KB apart in the original), the physics result
// is a count of bounces and wall hits, which does not depend on how the
// compiler rounds, and the compressed stream has room for all of its 23 KB
// (the original writes it into 16 KB).
#include <math.h>
#include "app_tests.h"

typedef unsigned long long u64_t;

// The table at the end; init of LZ Decompress and Tri Raster sets their items
enum { T_COPY, T_PACKET, T_LZ, T_PARTICLES, T_RASTER, NUM_GAME_TESTS };
static app_test game_tests[NUM_GAME_TESTS];

// --- Word copy loop (64 KB) ---

#define COPY_BYTES (64 * 1024)
KERNEL_DATA u32 copy_src[COPY_BYTES / 4] __attribute__((aligned(64)));
KERNEL_DATA u32 copy_dst[COPY_BYTES / 4] __attribute__((aligned(64)));

static void init_copy(void) {
    u8 *b = (u8 *)copy_src;
    for (u32 i = 0; i < COPY_BYTES; i++)
        b[i] = (u8)(i * 7 + (i >> 8));
}

#ifndef BENCH_ASM
static __attribute__((noinline)) void copy_loop(u32 *dst, const u32 *src,
                                                u32 words) {
    for (u32 i = 0; i < words; i++)
        dst[i] = src[i];
}

static u32 game_copy(void) {
    copy_loop(copy_dst, copy_src, COPY_BYTES / 4);
    return app_hash(copy_dst, sizeof(copy_dst));
}
#else
u32 game_copy(void); // asm/game_tests.S
#endif

// --- GIF packet building: a tag, then per vertex RGBAQ, ST and XYZ2 ---

#define PACKET_VERTS 2000
KERNEL_DATA u64_t packets[2 + PACKET_VERTS * 4] __attribute__((aligned(64)));

static void init_packet(void) {}

#ifndef BENCH_ASM
static __attribute__((noinline)) u32 packet_build(u64_t *out, u32 vertices) {
    u64_t *p = out;

    *p++ = 0x1000000000008001ull | (u64_t)vertices;
    *p++ = 0x5551ull;
    for (u32 i = 0; i < vertices; i++) {
        u32 x = (i * 37) & 0x1ff, y = (i * 91) & 0x1ff, z = i * 3;
        u32 r = (i * 5) & 0xff, g = (i * 11) & 0xff, b = (i * 17) & 0xff;

        *p++ = (u64_t)r | ((u64_t)g << 8) | ((u64_t)b << 16) | (0x80ull << 24);
        *p++ = 0x3f800000ull;
        *p++ = (u64_t)(x << 4) | ((u64_t)(y << 4) << 16) | ((u64_t)z << 32);
        *p++ = 0;
    }
    return (u32)(p - out);
}

static u32 game_packet(void) {
    u32 n = packet_build(packets, PACKET_VERTS);
    return app_hash(packets, n * 8);
}
#else
u32 game_packet(void); // asm/game_tests.S
#endif

// --- LZ77 decoder: literal runs and (length, distance) matches ---

KERNEL_DATA u8 lz_in[24576] __attribute__((aligned(64)));
KERNEL_DATA u8 lz_out[65536] __attribute__((aligned(64)));
KERNEL_DATA u32 lz_in_len;

static void init_lz(void) {
    u8 *o = lz_in;
    u32 produced = 0;

    for (u32 i = 0; i < 2000; i++) {
        u32 n = 4 + ((app_rand() >> 8) & 7);

        *o++ = (u8)(n - 1);
        for (u32 k = 0; k < n; k++)
            *o++ = (u8)(app_rand() >> 8);
        produced += n;
        if (produced > 64) {
            u32 len = 3 + ((app_rand() >> 8) & 15);
            u32 dist = (app_rand() >> 8) % 48;

            *o++ = (u8)(0x80 | (len - 3));
            *o++ = (u8)(dist >> 8);
            *o++ = (u8)dist;
            produced += len;
        }
    }
    lz_in_len = (u32)(o - lz_in);
    game_tests[T_LZ].items = produced;
}

#ifndef BENCH_ASM
static __attribute__((noinline)) u32 lz_decode(u8 *out, const u8 *in,
                                               u32 in_len) {
    u8 *o = out;
    const u8 *end = in + in_len;

    while (in < end) {
        u32 tag = *in++;

        if (tag < 0x80) {
            u32 n = tag + 1;

            while (n--)
                *o++ = *in++;
        } else {
            u32 len = (tag & 0x7f) + 3;
            u32 dist = ((u32)in[0] << 8 | in[1]) + 1;
            const u8 *m = o - dist;

            in += 2;
            while (len--)
                *o++ = *m++;
        }
    }
    return (u32)(o - out);
}

static u32 game_lz(void) {
    u32 n = lz_decode(lz_out, lz_in, lz_in_len);
    return app_hash(lz_out, n & ~3u) ^ n;
}
#else
u32 game_lz(void); // asm/game_tests.S
#endif

// --- Particle physics: gravity, a floor bounce and a circular wall ---

#define PARTICLES 1024
// Each array 64 bytes longer than 4 KB, so they map to different cache sets
#define PSTRIDE (PARTICLES + 16)
KERNEL_DATA float part[6][PSTRIDE] __attribute__((aligned(64)));
#define px part[0]
#define py part[1]
#define pz part[2]
#define vx part[3]
#define vy part[4]
#define vz part[5]

static void init_particles(void) {
    for (u32 i = 0; i < PARTICLES; i++) {
        px[i] = (float)((app_rand() >> 8) & 15) - 8.0f;
        py[i] = (float)((app_rand() >> 8) & 15);
        pz[i] = (float)((app_rand() >> 8) & 15) - 8.0f;
        vx[i] = (float)((app_rand() >> 8) & 7) - 4.0f;
        vy[i] = 0.0f;
        vz[i] = (float)((app_rand() >> 8) & 7) - 4.0f;
    }
}

#ifndef BENCH_ASM
static __attribute__((noinline)) u32 physics_frame(void) {
    const float dt = 1.0f / 60.0f;
    u32 bounces = 0, walls = 0;

    for (u32 i = 0; i < PARTICLES; i++) {
        vy[i] -= 9.8f * dt;
        px[i] += vx[i] * dt;
        py[i] += vy[i] * dt;
        pz[i] += vz[i] * dt;
        if (py[i] < 0.0f) {
            py[i] = -py[i];
            vy[i] = -vy[i] * 0.8f;
            bounces++;
        }
        float d2 = px[i] * px[i] + pz[i] * pz[i];
        if (d2 > 100.0f) {
            float inv = 1.0f / sqrtf(d2);
            vx[i] = -px[i] * inv;
            vz[i] = -pz[i] * inv;
            walls++;
        }
    }
    return bounces << 16 | walls;
}

static u32 game_particles(void) { return physics_frame(); }
#else
u32 game_particles(void); // asm/game_tests.S
#endif

// --- Triangle rasterizer: edge functions over the bounding box ---

#define FB_W 128
#define FB_H 128
#define TRIANGLES 200
KERNEL_DATA u32 fb[FB_W * FB_H] __attribute__((aligned(64)));
KERNEL_DATA int tri[TRIANGLES][6];
KERNEL_DATA u32 tri_color[TRIANGLES];

static void init_raster(void) {
    u32 pixels = 0;
    for (u32 t = 0; t < TRIANGLES; t++) {
        int *v = tri[t];
        for (int k = 0; k < 6; k += 2) {
            v[k] = (int)((app_rand() >> 8) % FB_W);
            v[k + 1] = (int)((app_rand() >> 8) % FB_H);
        }
        tri_color[t] = app_rand() >> 8;
        int minx = v[0], maxx = v[0], miny = v[1], maxy = v[1];
        for (int k = 2; k < 6; k += 2) {
            minx = v[k] < minx ? v[k] : minx;
            maxx = v[k] > maxx ? v[k] : maxx;
            miny = v[k + 1] < miny ? v[k + 1] : miny;
            maxy = v[k + 1] > maxy ? v[k + 1] : maxy;
        }
        pixels += (u32)((maxx - minx + 1) * (maxy - miny + 1));
    }
    game_tests[T_RASTER].items = pixels;
}

#ifndef BENCH_ASM
static __attribute__((noinline)) void raster_tris(void) {
    for (u32 t = 0; t < TRIANGLES; t++) {
        int x0 = tri[t][0], y0 = tri[t][1];
        int x1 = tri[t][2], y1 = tri[t][3];
        int x2 = tri[t][4], y2 = tri[t][5];
        u32 color = tri_color[t];
        int minx = x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2);
        int maxx = x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2);
        int miny = y0 < y1 ? (y0 < y2 ? y0 : y2) : (y1 < y2 ? y1 : y2);
        int maxy = y0 > y1 ? (y0 > y2 ? y0 : y2) : (y1 > y2 ? y1 : y2);
        int area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);

        if (area == 0)
            continue;
        for (int y = miny; y <= maxy; y++) {
            for (int x = minx; x <= maxx; x++) {
                int w0 = (x1 - x0) * (y - y0) - (x - x0) * (y1 - y0);
                int w1 = (x2 - x1) * (y - y1) - (x - x1) * (y2 - y1);
                int w2 = (x0 - x2) * (y - y2) - (x - x2) * (y0 - y2);

                if (area > 0 ? (w0 >= 0 && w1 >= 0 && w2 >= 0)
                             : (w0 <= 0 && w1 <= 0 && w2 <= 0))
                    fb[y * FB_W + x] = color;
            }
        }
    }
}

static u32 game_raster(void) {
    raster_tris();
    return app_hash(fb, sizeof(fb));
}
#else
u32 game_raster(void); // asm/game_tests.S
#endif

static app_test game_tests[NUM_GAME_TESTS] = {
    {"Copy Loop", "MB/s", COPY_BYTES, init_copy, game_copy},
    {"Packet Build", "Mvert/s", PACKET_VERTS, init_packet, game_packet},
    {"LZ Decompress", "MB/s", 0, init_lz, game_lz},
    {"Particles", "Mpart/s", PARTICLES, init_particles, game_particles},
    {"Tri Raster", "Mpix/s", 0, init_raster, game_raster},
};
const app_suite game_suite = {"GAME", "Game loops", game_tests,
                              NUM_GAME_TESTS};
