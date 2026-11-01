#!/usr/bin/env python3
"""Generates appv.S and appv.h for app.c: variants of the application
kernels (asm/gen/gfx.py, ...) to measure on the PS2. Usage:
  gen_app.py <output dir> <set>
with the sets of variants below. Build and run with build_app.sh."""
import sys, os
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'gen'))
import gfx
import sound
import emu

OUT, SET = sys.argv[1], sys.argv[2]
specs = []   # (test name, variant name, generator(symbol))


def add(test, vname, gen):
    specs.append((test, vname, gen))


if SET == 'gfx2d':
    for U in (1, 2):
        for D in (0, 4, 8):
            add('Alpha Blend', f'u{U}_d{D}', lambda n, U=U, D=D: gfx.alpha_blend(n, U, D))
    for mode in ('alu', 'mmi'):
        for D in (0, 4):
            add('CLUT8', f'{mode}_d{D}', lambda n, m=mode, D=D: gfx.clut8(n, m, D))
    for what, test in (('key', 'Color Key'), ('test', 'Alpha Test')):
        for U, D, w in ((8, 2, True), (8, 4, True), (4, 2, True), (8, 2, False), (8, 0, False)):
            add(test, f'u{U}_d{D}_w{int(w)}', lambda n, what=what, U=U, D=D, w=w: gfx.select2(n, what, U, D, w))
    for U in (1, 2, 4):
        for D in (0, 4):
            add('To CT16', f'u{U}_d{D}', lambda n, U=U, D=D: gfx.to_ct16(n, U, D))
    for D in (0, 4):
        add('Scale NN', f'd{D}', lambda n, D=D: gfx.scale_nn(n, D))
elif SET == 'geom':
    for D in (0, 4):
        add('Transform', f'vu0_d{D}', lambda n, D=D: gfx.transform_vu0(n, D))
    add('Transform', 'fpu', lambda n: gfx.transform_fpu(n))
    for v4, test in ((False, 'Lighting'), (True, 'Lighting v4sf')):
        for D in (0, 4):
            add(test, f'vu0_d{D}', lambda n, v4=v4, D=D: gfx.lighting_vu0(n, v4, D))
        add(test, 'fpu', lambda n, v4=v4: gfx.lighting_fpu(n, v4))
    for what, test in (('scale', 'Field Scale v4sf'), ('offset', 'Field Offset bc v4sf'),
                       ('project', 'Project v4sf')):
        for unit in ('vu0', 'fpu'):
            for D in (0, 4, 8):
                add(test, f'{unit}_u4_d{D}', lambda n, w=what, u=unit, D=D: gfx.stream_v(n, w, u, 4, D))
    for unit in ('vu0', 'fpu'):
        for D in (0, 4):
            add('Weighted Sum v4sf', f'{unit}_d{D}', lambda n, u=unit, D=D: gfx.weighted_sum(n, u, 4, D))
    for form, test in (('row4', 'MxV Row 4x4'), ('col4', 'MxV Col 4x4'), ('row3', 'MxV Row 3x4'),
                       ('col3', 'MxV Col 3x4')):
        for unit in ('vu0', 'fpu'):
            for D in (0, 4):
                add(test, f'{unit}_d{D}', lambda n, f=form, u=unit, D=D:
                    gfx.mxv(n + '_k', f, u, D) + gfx.mxv_wrapper(n, n + '_k', f))
    for D in (0, 4):
        add('Backface Cull', f'd{D}', lambda n, D=D: gfx.backface_cull(n, D))
    for vec, test in ((False, 'Face Normals'), (True, 'Face Normals v4sf')):
        for unit in ('vu0', 'fpu'):
            for D in (0, 4):
                add(test, f'{unit}_d{D}', lambda n, v=vec, u=unit, D=D: gfx.face_normal(n, u, v, D))
    for unit in ('vu0', 'fpu'):
        for D in (0, 4):
            add('Frustum Cull', f'{unit}_d{D}', lambda n, u=unit, D=D: gfx.frustum_cull(n, u, D))
elif SET == 'geom2':
    for D, W in ((0, 0), (3, 1), (4, 1), (4, 0), (6, 1)):
        add('Transform', f'vu0_d{D}_w{W}', lambda n, D=D, W=W: gfx.transform_vu0(n, D, W))
    for v4, test in ((False, 'Lighting'), (True, 'Lighting v4sf')):
        for D in (2, 3, 6):
            add(test, f'vu0_d{D}', lambda n, v4=v4, D=D: gfx.lighting_vu0(n, v4, D))
    for what, test in (('scale', 'Field Scale v4sf'), ('project', 'Project v4sf')):
        for unit in ('vu0', 'fpu'):
            for D, W in ((2, 0), (3, 0), (3, 1), (4, 1), (6, 0)):
                add(test, f'{unit}_d{D}_w{W}', lambda n, w=what, u=unit, D=D, W=W: gfx.stream_v(n, w, u, 4, D, W))
        for D, W in ((3, 0), (4, 0)):
            add(test, f'vu0_u8_d{D}_w{W}', lambda n, w=what, D=D, W=W: gfx.stream_v(n, w, 'vu0', 8, D, W))
    for unit in ('vu0', 'fpu'):
        for D in (2, 3, 6):
            add('Weighted Sum v4sf', f'{unit}_d{D}', lambda n, u=unit, D=D: gfx.weighted_sum(n, u, 4, D))
        add('Weighted Sum v4sf', f'{unit}_u8_d4', lambda n, u=unit: gfx.weighted_sum(n, u, 8, 4))
    for form, test in (('row4', 'MxV Row 4x4'), ('row3', 'MxV Row 3x4')):
        for unit in ('vu0', 'fpu'):
            for D, W in ((2, 0), (3, 0), (3, 1), (4, 1), (6, 0)):
                add(test, f'{unit}_d{D}_w{W}', lambda n, f=form, u=unit, D=D, W=W:
                    gfx.mxv(n + '_k', f, u, D, W) + gfx.mxv_wrapper(n, n + '_k', f))
    for D in (2, 3, 6):
        add('Backface Cull', f'd{D}', lambda n, D=D: gfx.backface_cull(n, D))
    for unit in ('vu0', 'fpu'):
        for D, W in ((2, 0), (3, 0), (3, 1), (4, 1), (6, 0)):
            add('Face Normals v4sf', f'{unit}_d{D}_w{W}', lambda n, u=unit, D=D, W=W: gfx.face_normal(n, u, True, D, W))
    for unit in ('vu0', 'fpu'):
        for D in (2, 3, 6, 8):
            add('Frustum Cull', f'{unit}_d{D}', lambda n, u=unit, D=D: gfx.frustum_cull(n, u, D))
elif SET == 'sound':
    add('Mix 8 Voices', 'mmi', lambda n: sound.mix(n))
    add('ADPCM Decode', 'scalar', lambda n: sound.adpcm(n))
    add('FIR 32', 'mmi', lambda n: sound.fir(n))
    for U in (2, 4):
        add('Resample', f'u{U}', lambda n, U=U: sound.resample(n, U))
    add('Correlate Q15', 'mmi', lambda n: sound.correlate(n))
    for unit, U in (('vu0', 4), ('vu0', 8), ('fpu', 16)):
        for D in (0, 3, 5):
            add('Energy f32', f'{unit}_u{U}_d{D}', lambda n, u=unit, U=U, D=D: sound.energy(n, u, U, D))
    for D, W in ((2, 0), (3, 0), (3, 1), (5, 0)):
        add('Transform', f'fpu_d{D}_w{W}', lambda n, D=D, W=W: gfx.transform_fpu(n, D, W))
    for D in (2, 4):
        add('Lighting', f'fpu_d{D}', lambda n, D=D: gfx.lighting_fpu(n, False, D))
    for D in (2, 4):
        add('Lighting', f'fpu_u2_d{D}', lambda n, D=D: gfx.lighting_fpu_u2(n, False, D))
    for U, D in ((1, 2), (1, 3), (1, 6)):
        add('Alpha Blend', f'u{U}_d{D}', lambda n, U=U, D=D: gfx.alpha_blend(n, U, D))
elif SET == 'emu':
    add('6502 Interp', 'threaded', lambda n: emu.cpu8(n))
    add('R3000 Interp', 'threaded', lambda n: emu.r3000(n))
    add('SNES Tiles', 'table', lambda n: emu.tile_decode(n))
    add('SNES Tiles', 'mmi', lambda n: emu.tile_decode_mmi(n))
    add('N64 To CT16', 'u4_a30', lambda n: emu.n64_to_ct16(n, 4, 30, 2))
    add('N64 To CT16', 'u2_a0', lambda n: emu.n64_to_ct16(n, 2, 0, 2))
    for mode in ('scalar', 'mmi'):
        add('Guest Loads', f'{mode}', lambda n, m=mode: emu.guest_loads(n, m, 4))
    for U in (4, 8):
        add('Resample', f'u{U}b', lambda n, U=U: sound.resample(n, U))
    for D in (1, 2):
        add('Energy f32', f'fpu_u16_d{D}', lambda n, D=D: sound.energy(n, 'fpu', 16, D))
    add('Energy f32', 'fpu_u32_d2', lambda n: sound.energy(n, 'fpu', 32, 2))
elif SET == 'emu2':
    for mode in ('scalar', 'mmi'):
        for D, K in ((1, 0), (2, 0), (1, 8), (1, 16), (2, 16), (1, 32)):
            add('Guest Loads', f'{mode}_d{D}_k{K}', lambda n, m=mode, D=D, K=K: emu.guest_loads(n, m, 4, D, K))
    for unit, U in (('vu0', 4), ('vu0', 8)):
        for D in (1, 2):
            add('Energy f32', f'{unit}_u{U}_d{D}', lambda n, u=unit, U=U, D=D: sound.energy(n, u, U, D))
    add('Energy f32', 'fpu_u32_d1', lambda n: sound.energy(n, 'fpu', 32, 1))
    for D in (1, 2):
        add('Weighted Sum v4sf', f'vu0_d{D}x', lambda n, D=D: gfx.weighted_sum(n, 'vu0', 4, D))
        add('Weighted Sum v4sf', f'fpu_d{D}x', lambda n, D=D: gfx.weighted_sum(n, 'fpu', 4, D))
        add('Field Scale v4sf', f'vu0_d{D}x', lambda n, D=D: gfx.stream_v(n, 'scale', 'vu0', 4, D))
        add('MxV Row 4x4', f'vu0_d{D}x', lambda n, D=D: gfx.mxv(n + '_k', 'row4', 'vu0', D) + gfx.mxv_wrapper(n, n + '_k', 'row4'))
        add('Face Normals v4sf', f'vu0_d{D}x', lambda n, D=D: gfx.face_normal(n, 'vu0', True, D))
        add('Project v4sf', f'fpu_d{D}x', lambda n, D=D: gfx.stream_v(n, 'project', 'fpu', 4, D))
elif SET == 'nn':
    add('Scale NN', 'd0', lambda n: gfx.scale_nn(n, 0))
else:
    sys.exit(f'unknown set {SET}')

out = ['\t.set noreorder', '\t.set noat', gfx.PRELUDE, '\t.text']
syms = []
for k, (test, vname, gen) in enumerate(specs):
    sym = f'v{k}_' + ''.join(c if c.isalnum() else '_' for c in vname)
    syms.append(sym)
    out.append('\t.p2align 6')
    out.append(gen(sym))
open(os.path.join(OUT, 'appv.S'), 'w').write('\n'.join(out) + '\n')
h = [f'u32 {s}(void);' for s in syms]
h.append('static const struct { const char *test, *name; u32 (*f)(void); } variants[] = {')
for (test, vname, gen), s in zip(specs, syms):
    h.append(f'    {{"{test}", "{vname}", {s}}},')
h.append('};')
open(os.path.join(OUT, 'appv.h'), 'w').write('\n'.join(h) + '\n')
print(len(specs), 'variants')
for k, v in gfx.EST.items():
    print(' ', k, v)
