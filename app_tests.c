#include <stdio.h>
#include "app_tests.h"

#define CPU_MHZ 294.912f
// Each kernel runs for at least this many cycles (about 0.1 s)
#define MIN_CYCLES 30000000u

static const app_suite *const suites[] = {&gfx_suite, &geom_suite,
                                          &sound_suite, &emu_suite};
#define NUM_SUITES (sizeof(suites) / sizeof(suites[0]))
#define MAX_TESTS 8

static float perf[NUM_SUITES][MAX_TESTS];
static u32 rand_state;

u32 app_rand(void) {
    rand_state = rand_state * 1664525u + 1013904223u;
    return rand_state;
}

u32 app_hash(const void *p, unsigned bytes) {
    const u32 *w = p;
    u32 h = 2166136261u;
    for (unsigned i = 0; i < bytes / 4; i++)
        h = (h ^ w[i]) * 16777619u;
    return h;
}

static inline u32 cycles(void) {
    u32 c;
    __asm__ __volatile__("mfc0 %0, $9" : "=r"(c));
    return c;
}

void app_run_all(void) {
    for (unsigned s = 0; s < NUM_SUITES; s++) {
        const app_suite *suite = suites[s];
        printf("=== %s ===\n\n", suite->title);
        for (unsigned i = 0; i < suite->count && i < MAX_TESTS; i++) {
            const app_test *t = &suite->tests[i];
            rand_state = 1;
            t->init();
            u32 result = t->run();

            u32 runs = 0, c0 = cycles(), c;
            do {
                t->run();
                runs++;
                c = cycles() - c0;
            } while (c < MIN_CYCLES);
            perf[s][i] = (float)t->items * runs * CPU_MHZ / c;

            printf("--- %s: %s ---\n", suite->section, t->name);
            printf("Result: 0x%08x\n", (unsigned)result);
            printf("Performance: %.2f %s\n\n", perf[s][i], t->unit);
        }
    }
}

void app_print_summary(void) {
    for (unsigned s = 0; s < NUM_SUITES; s++) {
        const app_suite *suite = suites[s];
        printf("%s:\n", suite->section);
        for (unsigned i = 0; i < suite->count && i < MAX_TESTS; i++) {
            char name[32];
            snprintf(name, sizeof(name), "%s:", suite->tests[i].name);
            printf("  %-15s %7.2f %s\n", name, perf[s][i],
                   suite->tests[i].unit);
        }
    }
}
