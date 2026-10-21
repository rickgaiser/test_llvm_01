// 2D graphics and 3D geometry kernels. Pixels are 32-bit RGBA with red in the
// low byte, as in the GS PSMCT32 format; 0x80 is full intensity and opacity.
#include "app_tests.h"

#define SPR_W 64
#define SPR_H 64
#define SPR_PIXELS (SPR_W * SPR_H)

static u32 sprite[SPR_PIXELS] __attribute__((aligned(64)));
static u32 frame[SPR_PIXELS] __attribute__((aligned(64)));
static u8 indexed[SPR_PIXELS] __attribute__((aligned(64)));
static u32 palette[256] __attribute__((aligned(64)));
static u16 frame16[SPR_PIXELS] __attribute__((aligned(64)));

// --- 2D ---

static void init_sprite(void) {
    for (int i = 0; i < SPR_PIXELS; i++) {
        sprite[i] = app_rand();
        frame[i] = app_rand();
    }
}

// Blend a sprite over the frame with its alpha (0..255 per channel)
static u32 alpha_blend(void) {
    for (int i = 0; i < SPR_PIXELS; i++) {
        u32 s = sprite[i], d = frame[i];
        u32 a = s >> 24, na = 256 - a;
        u32 r = ((s & 0xff) * a + (d & 0xff) * na) >> 8;
        u32 g = (((s >> 8) & 0xff) * a + ((d >> 8) & 0xff) * na) >> 8;
        u32 b = (((s >> 16) & 0xff) * a + ((d >> 16) & 0xff) * na) >> 8;
        frame[i] = r | (g << 8) | (b << 16) | (d & 0xff000000);
    }
    return app_hash(frame, sizeof(frame));
}

static void init_indexed(void) {
    for (int i = 0; i < 256; i++)
        palette[i] = app_rand();
    for (int i = 0; i < SPR_PIXELS; i++)
        indexed[i] = app_rand() >> 24;
}

// Expand an 8-bit indexed (CLUT) image to 32 bits
static u32 clut8(void) {
    for (int i = 0; i < SPR_PIXELS; i++)
        frame[i] = palette[indexed[i]];
    return app_hash(frame, sizeof(frame));
}

// Convert 32-bit pixels to the 16-bit GS PSMCT16 format (RGBA5551)
static u32 to_ct16(void) {
    for (int i = 0; i < SPR_PIXELS; i++) {
        u32 p = sprite[i];
        frame16[i] = ((p >> 3) & 0x1f) | ((p >> 6) & 0x3e0) |
                     ((p >> 9) & 0x7c00) | ((p >> 16) & 0x8000);
    }
    return app_hash(frame16, sizeof(frame16));
}

#define SCALED_W 96
#define SCALED_H 96
static u32 scaled[SCALED_W * SCALED_H] __attribute__((aligned(64)));

// Scale the sprite to 96x96 with nearest neighbor, 16.16 fixed-point steps
static u32 scale_nn(void) {
    u32 step = (SPR_W << 16) / SCALED_W;
    u32 sy = 0;
    for (int y = 0; y < SCALED_H; y++, sy += step) {
        const u32 *row = &sprite[(sy >> 16) * SPR_W];
        u32 *out = &scaled[y * SCALED_W];
        u32 sx = 0;
        for (int x = 0; x < SCALED_W; x++, sx += step)
            out[x] = row[sx >> 16];
    }
    return app_hash(scaled, sizeof(scaled));
}

static const app_test gfx_tests[] = {
    {"Alpha Blend", "Mpix/s", SPR_PIXELS, init_sprite, alpha_blend},
    {"CLUT8", "Mpix/s", SPR_PIXELS, init_indexed, clut8},
    {"To CT16", "Mpix/s", SPR_PIXELS, init_sprite, to_ct16},
    {"Scale NN", "Mpix/s", SCALED_W * SCALED_H, init_sprite, scale_nn},
};
const app_suite gfx_suite = {"GFX", "2D Graphics", gfx_tests,
                             sizeof(gfx_tests) / sizeof(gfx_tests[0])};

// --- 3D ---

typedef struct {
    float x, y, z, w;
} vec4 __attribute__((aligned(16)));

#define NUM_VERTS 512
#define NUM_TRIS (NUM_VERTS - 2)
static vec4 verts[NUM_VERTS];
static vec4 normals[NUM_VERTS];
static vec4 matrix[3];
// Screen position in 12.4 fixed point and depth, as for the GS XYZ2 register
static struct {
    int x, y, z, pad;
} screen[NUM_VERTS] __attribute__((aligned(16)));
static u32 colors[NUM_VERTS];
static u16 visible[NUM_TRIS * 3];

// A small signed multiple of 1/8, so products and sums stay exact
static float eighths(int range) {
    return (float)((int)(app_rand() >> 16) % (range * 8 + 1) - range * 4) /
           8.0f;
}

static void init_geom(void) {
    // Rotation-like rows with exact entries. The w of the projection is the
    // z of the vertex, a power of two.
    matrix[0] = (vec4){0.75f, 0.0f, -0.5f, 0.0f};
    matrix[1] = (vec4){0.125f, 0.875f, 0.25f, 0.0f};
    matrix[2] = (vec4){0.5f, -0.25f, 0.75f, 0.0f};
    static const float depths[] = {1.0f, 2.0f, 4.0f, 8.0f, 16.0f};
    for (int i = 0; i < NUM_VERTS; i++) {
        verts[i].x = (float)((int)(app_rand() >> 26) - 32);
        verts[i].y = (float)((int)(app_rand() >> 26) - 32);
        verts[i].z = depths[(app_rand() >> 16) % 5];
        verts[i].w = 1.0f;
        normals[i] = (vec4){eighths(1), eighths(1), eighths(1), 0.0f};
    }
}

// Transform to clip space, divide by w and map to the 4096x4096 GS window
// centered at 2048, in 12.4 fixed point
static u32 transform(void) {
    for (int i = 0; i < NUM_VERTS; i++) {
        vec4 v = verts[i];
        float x = matrix[0].x * v.x + matrix[0].y * v.y + matrix[0].z * v.z +
                  matrix[0].w * v.w;
        float y = matrix[1].x * v.x + matrix[1].y * v.y + matrix[1].z * v.z +
                  matrix[1].w * v.w;
        float z = matrix[2].x * v.x + matrix[2].y * v.y + matrix[2].z * v.z +
                  matrix[2].w * v.w;
        // The positions are whole numbers; rounding them gives the same result
        // with an approximate reciprocal (-ffast-math)
        float iw = 1.0f / v.z;
        screen[i].x = (int)((x * iw * 64.0f + 2048.0f) * 16.0f + 0.5f);
        screen[i].y = (int)((y * iw * 64.0f + 2048.0f) * 16.0f + 0.5f);
        screen[i].z = (int)(iw * 65536.0f + 0.5f);
        screen[i].pad = (int)(z * 16.0f);
    }
    return app_hash(screen, sizeof(screen));
}

// Diffuse lighting with an ambient term and 3 directional lights, to a GS
// vertex color (0x80 = 1.0)
static u32 lighting(void) {
    static const vec4 dir[3] = {{0.5f, 0.5f, -0.5f, 0.0f},
                                {-0.75f, 0.25f, 0.0f, 0.0f},
                                {0.0f, -0.5f, 0.75f, 0.0f}};
    static const vec4 col[3] = {{0.5f, 0.375f, 0.25f, 0.0f},
                                {0.125f, 0.25f, 0.5f, 0.0f},
                                {0.25f, 0.25f, 0.125f, 0.0f}};
    for (int i = 0; i < NUM_VERTS; i++) {
        vec4 n = normals[i];
        float r = 0.125f, g = 0.125f, b = 0.125f;
        for (int l = 0; l < 3; l++) {
            float d = n.x * dir[l].x + n.y * dir[l].y + n.z * dir[l].z;
            if (d > 0.0f) {
                r += d * col[l].x;
                g += d * col[l].y;
                b += d * col[l].z;
            }
        }
        if (r > 1.0f)
            r = 1.0f;
        if (g > 1.0f)
            g = 1.0f;
        if (b > 1.0f)
            b = 1.0f;
        colors[i] = (u32)(r * 128.0f) | ((u32)(g * 128.0f) << 8) |
                    ((u32)(b * 128.0f) << 16) | 0x80000000u;
    }
    return app_hash(colors, sizeof(colors));
}

static void init_cull(void) {
    init_geom();
    transform();
}

// Keep the counter-clockwise triangles of a strip, by the sign of the 2D
// cross product of their 12.4 fixed-point screen positions
static u32 backface_cull(void) {
    int n = 0;
    for (int i = 0; i < NUM_TRIS; i++) {
        int a = i, b = i + 1 + (i & 1), c = i + 2 - (i & 1);
        s32 ex = screen[b].x - screen[a].x, ey = screen[b].y - screen[a].y;
        s32 fx = screen[c].x - screen[a].x, fy = screen[c].y - screen[a].y;
        if ((s64)ex * fy - (s64)ey * fx > 0) {
            visible[n++] = a;
            visible[n++] = b;
            visible[n++] = c;
        }
    }
    return app_hash(visible, n * 2) ^ n;
}

// Bounding spheres against the 6 planes of a view frustum
#define NUM_SPHERES 1024
static vec4 spheres[NUM_SPHERES];  // center, radius
static u8 sphere_visible[NUM_SPHERES];

static void init_spheres(void) {
    for (int i = 0; i < NUM_SPHERES; i++)
        spheres[i] = (vec4){(float)((int)(app_rand() >> 25) - 64),
                            (float)((int)(app_rand() >> 25) - 64),
                            (float)(app_rand() >> 25),
                            (float)(app_rand() >> 28)};
}

static u32 frustum_cull(void) {
    static const vec4 planes[6] = {
        {0.75f, 0.0f, 0.5f, 0.0f},   {-0.75f, 0.0f, 0.5f, 0.0f},
        {0.0f, 0.75f, 0.5f, 0.0f},   {0.0f, -0.75f, 0.5f, 0.0f},
        {0.0f, 0.0f, 1.0f, -1.0f},   {0.0f, 0.0f, -1.0f, 100.0f}};
    u32 count = 0;
    for (int i = 0; i < NUM_SPHERES; i++) {
        vec4 s = spheres[i];
        int in = 1;
        for (int p = 0; p < 6; p++) {
            float d = planes[p].x * s.x + planes[p].y * s.y +
                      planes[p].z * s.z + planes[p].w;
            if (d < -s.w) {
                in = 0;
                break;
            }
        }
        sphere_visible[i] = in;
        count += in;
    }
    return app_hash(sphere_visible, sizeof(sphere_visible)) ^ count;
}

static const app_test geom_tests[] = {
    {"Transform", "Mvert/s", NUM_VERTS, init_geom, transform},
    {"Lighting", "Mvert/s", NUM_VERTS, init_geom, lighting},
    {"Backface Cull", "Mtri/s", NUM_TRIS, init_cull, backface_cull},
    {"Frustum Cull", "Mobj/s", NUM_SPHERES, init_spheres, frustum_cull},
};
const app_suite geom_suite = {"GEOM", "3D Geometry", geom_tests,
                              sizeof(geom_tests) / sizeof(geom_tests[0])};
