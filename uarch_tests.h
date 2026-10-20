#ifndef UARCH_TESTS_H
#define UARCH_TESTS_H

#include <tamtypes.h>

// Microarchitecture tests: hand-written assembly kernels that measure the
// latencies, issue pairing and pipe conflicts of the R5900, in CPU cycles
// (COP0 Count). The kernels are the same for every compiler, they provide the
// numbers for the scheduling model.
//
// big/big_words: a buffer much larger than the data cache, used for the
// cache-miss tests (its contents are overwritten).
void uarch_run_all(u32 *big, int big_words);

#endif // UARCH_TESTS_H
