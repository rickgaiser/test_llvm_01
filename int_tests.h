#ifndef INT_TESTS_H
#define INT_TESTS_H

#define NUM_INTS 1024

// Fixed-point Q16.16 format (16 bits integer, 16 bits fraction)
#define FP_SHIFT 16
#define FP_ONE (1 << FP_SHIFT)
#define FLOAT_TO_FP(f) ((int)((f) * FP_ONE))
#define FP_TO_FLOAT(fp) ((float)(fp) / FP_ONE)
#define FP_MUL(a, b) (((long long)(a) * (b)) >> FP_SHIFT)

// Alignment hint macro - Clang supports on params, GCC doesn't
#ifdef __clang__
#define ALIGNED16 __attribute__((aligned(16)))
#else
#define ALIGNED16
#endif

// Test functions - all pointers 128-bit aligned
void int_scalar_multiply(int * restrict in ALIGNED16,
                        int * restrict out ALIGNED16,
                        int count, int scalar);

void transform_vector_list_fp(int mat[4][4] ALIGNED16,
                             int (* restrict vecs_in)[4] ALIGNED16,
                             int (* restrict vecs_out)[4] ALIGNED16,
                             int count);

void test_dual_pipeline_int(unsigned int * restrict data ALIGNED16,
                           int count, int scalar,
                           unsigned int result[4] ALIGNED16);

void test_dual_pipeline_madd(unsigned int * restrict data ALIGNED16,
                            int count, int scalar,
                            unsigned int result[2] ALIGNED16);

#endif // INT_TESTS_H
