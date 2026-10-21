#ifndef APP_TESTS_H
#define APP_TESTS_H

#include <tamtypes.h>

// Application kernels: the loops of 2D and 3D graphics, sound processing and
// emulators of older consoles, in plain C. Each kernel is timed in CPU cycles.
// Its result is a checksum of its output after one run on fresh input, so the
// output can be compared between compilers. The floating-point kernels use
// inputs for which every operation is exact, so the result does not depend on
// how the compiler orders or fuses the arithmetic.

typedef struct {
    const char *name;    // summary name
    const char *unit;    // per second, e.g. "Mpix/s"
    u32 items;           // items (pixels, vertices, ...) per run
    void (*init)(void);  // sets up the input
    u32 (*run)(void);    // one run; returns a checksum of the output
} app_test;

typedef struct {
    const char *section;  // summary section, capital letters only
    const char *title;
    const app_test *tests;
    unsigned count;
} app_suite;

extern const app_suite gfx_suite;    // 2D graphics
extern const app_suite geom_suite;   // 3D geometry
extern const app_suite sound_suite;  // sound processing
extern const app_suite emu_suite;    // emulation

void app_run_all(void);
void app_print_summary(void);

// FNV-1a over the 32-bit words of p
u32 app_hash(const void *p, unsigned bytes);
// Deterministic pseudo-random numbers; app_run_all seeds them for every test
u32 app_rand(void);

#endif // APP_TESTS_H
