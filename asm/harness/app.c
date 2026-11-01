// The application kernels of the bench (gfx_tests.c, sound_tests.c,
// emu_tests.c): hand-written variants (appv.S and appv.h, made by
// gen_app.py) against the kernels of the asm build, which give the expected
// result. Linked with the objects of the asm build except main.o and
// app_tests.o: this file has its own app_rand and app_hash. Prints the rate
// per variant in the unit of the test, as the bench does.
#include <stdio.h>
#include <string.h>
#include "app_tests.h"

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
#include "appv.h"

static const app_suite *const suites[] = {&gfx_suite, &geom_suite,
                                          &sound_suite, &emu_suite};
#define MIN_CYCLES 10000000u

int main(void) {
    int nv = sizeof(variants) / sizeof(variants[0]), bad = 0;
    for (int rep = 0; rep < 2; rep++)
        for (int k = 0; k < nv; k++) {
            const app_test *t = 0;
            for (unsigned s = 0; s < 4 && !t; s++)
                for (unsigned i = 0; i < suites[s]->count; i++)
                    if (!strcmp(suites[s]->tests[i].name, variants[k].test))
                        t = &suites[s]->tests[i];
            if (!t) {
                printf("V %d %s %s NOTEST\n", rep, variants[k].test, variants[k].name);
                bad++;
                continue;
            }
            rand_state = 1;
            t->init();
            u32 ref = t->run();
            rand_state = 1;
            t->init();
            u32 got = variants[k].f();
            u32 runs = 0, c0 = cycles(), c;
            do {
                variants[k].f();
                runs++;
                c = cycles() - c0;
            } while (c < MIN_CYCLES);
            unsigned rate = (unsigned)((unsigned long long)t->items * runs * 294912 / c);
            bad += got != ref;
            printf("V %d %s|%s %u.%03u%s\n", rep, t->name, variants[k].name,
                   rate / 1000, rate % 1000, got == ref ? "" : " WRONG");
        }
    printf("@@PS2TEST_END@@ status=%d\n", bad ? 1 : 0);
    return 0;
}
