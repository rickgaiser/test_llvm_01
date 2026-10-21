// Sound processing kernels on 16-bit PCM, as for the SPU2.
#include "app_tests.h"

static inline s16 sat16(s32 v) {
    return v > 32767 ? 32767 : v < -32768 ? -32768 : v;
}

// --- Software mixer: 8 voices with a Q15 left and right volume ---

#define NUM_VOICES 8
#define MIX_FRAMES 256
static s16 voice_pcm[NUM_VOICES][MIX_FRAMES] __attribute__((aligned(64)));
static s16 voice_vol[NUM_VOICES][2];
static s16 mix_out[MIX_FRAMES * 2] __attribute__((aligned(64)));

static void init_mix(void) {
    for (int v = 0; v < NUM_VOICES; v++) {
        for (int i = 0; i < MIX_FRAMES; i++)
            voice_pcm[v][i] = app_rand() >> 16;
        voice_vol[v][0] = app_rand() >> 17;
        voice_vol[v][1] = app_rand() >> 17;
    }
}

static u32 mix(void) {
    for (int i = 0; i < MIX_FRAMES; i++) {
        s32 l = 0, r = 0;
        for (int v = 0; v < NUM_VOICES; v++) {
            s32 s = voice_pcm[v][i];
            l += (s * voice_vol[v][0]) >> 15;
            r += (s * voice_vol[v][1]) >> 15;
        }
        mix_out[i * 2] = sat16(l);
        mix_out[i * 2 + 1] = sat16(r);
    }
    return app_hash(mix_out, sizeof(mix_out));
}

// --- PS-ADPCM (VAG) decoding ---
// A block is 16 bytes: shift and filter, flags, then 28 4-bit samples.

#define ADPCM_BLOCKS 64
#define ADPCM_SAMPLES (ADPCM_BLOCKS * 28)
static u8 adpcm[ADPCM_BLOCKS * 16] __attribute__((aligned(64)));
static s16 pcm[ADPCM_SAMPLES] __attribute__((aligned(64)));

static void init_adpcm(void) {
    for (int b = 0; b < ADPCM_BLOCKS; b++) {
        adpcm[b * 16] = ((app_rand() >> 16) % 13) | (((app_rand() >> 16) % 5) << 4);
        adpcm[b * 16 + 1] = 0;
        for (int i = 2; i < 16; i++)
            adpcm[b * 16 + i] = app_rand() >> 24;
    }
}

static u32 adpcm_decode(void) {
    static const s32 coef[5][2] = {
        {0, 0}, {60, 0}, {115, -52}, {98, -55}, {122, -60}};
    s32 old = 0, older = 0;
    s16 *out = pcm;
    for (int b = 0; b < ADPCM_BLOCKS; b++) {
        const u8 *blk = &adpcm[b * 16];
        int shift = blk[0] & 0xf, filter = blk[0] >> 4;
        s32 f0 = coef[filter][0], f1 = coef[filter][1];
        for (int i = 0; i < 28; i++) {
            u32 nibble = (blk[2 + i / 2] >> ((i & 1) * 4)) & 0xf;
            s32 s = (s32)((s16)(nibble << 12)) >> shift;
            s += (old * f0 + older * f1 + 32) >> 6;
            s = sat16(s);
            *out++ = s;
            older = old;
            old = s;
        }
    }
    return app_hash(pcm, sizeof(pcm));
}

// --- 32-tap FIR filter, Q15 ---

#define FIR_TAPS 32
#define FIR_SAMPLES 512
static s16 fir_in[FIR_SAMPLES + FIR_TAPS] __attribute__((aligned(64)));
static s16 fir_coef[FIR_TAPS] __attribute__((aligned(64)));
static s16 fir_out[FIR_SAMPLES] __attribute__((aligned(64)));

static void init_fir(void) {
    for (int i = 0; i < FIR_SAMPLES + FIR_TAPS; i++)
        fir_in[i] = app_rand() >> 16;
    // The sum of the coefficients stays below 1.0, so the sum fits in 32 bits
    for (int k = 0; k < FIR_TAPS; k++)
        fir_coef[k] = (s16)(app_rand() >> 16) >> 5;
}

static u32 fir(void) {
    for (int i = 0; i < FIR_SAMPLES; i++) {
        s32 acc = 1 << 14;
        for (int k = 0; k < FIR_TAPS; k++)
            acc += fir_in[i + k] * fir_coef[k];
        fir_out[i] = sat16(acc >> 15);
    }
    return app_hash(fir_out, sizeof(fir_out));
}

// --- Linear interpolation resampling, 44.1 kHz to 48 kHz ---

#define RESAMPLE_OUT 1024
#define RESAMPLE_IN 1024
static s16 rs_in[RESAMPLE_IN] __attribute__((aligned(64)));
static s16 rs_out[RESAMPLE_OUT] __attribute__((aligned(64)));

static void init_resample(void) {
    for (int i = 0; i < RESAMPLE_IN; i++)
        rs_in[i] = app_rand() >> 16;
}

static u32 resample(void) {
    u32 step = (44100u << 16) / 48000u, pos = 0;
    for (int i = 0; i < RESAMPLE_OUT; i++, pos += step) {
        u32 j = pos >> 16;
        s32 frac = (pos >> 1) & 0x7fff;
        s32 a = rs_in[j], b = rs_in[j + 1];
        rs_out[i] = a + (((b - a) * frac) >> 15);
    }
    return app_hash(rs_out, sizeof(rs_out));
}

static const app_test sound_tests[] = {
    {"Mix 8 Voices", "Msmp/s", NUM_VOICES * MIX_FRAMES, init_mix, mix},
    {"ADPCM Decode", "Msmp/s", ADPCM_SAMPLES, init_adpcm, adpcm_decode},
    {"FIR 32", "Msmp/s", FIR_SAMPLES, init_fir, fir},
    {"Resample", "Msmp/s", RESAMPLE_OUT, init_resample, resample},
};
const app_suite sound_suite = {"SOUND", "Sound", sound_tests,
                               sizeof(sound_tests) / sizeof(sound_tests[0])};
