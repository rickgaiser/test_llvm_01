#include "int_tests.h"

void int_scalar_multiply(int * restrict in ALIGNED16,
                        int * restrict out ALIGNED16,
                        int count, int scalar) {
    //#pragma GCC unroll 4
    #pragma clang loop interleave(enable)
    #pragma clang loop interleave_count(4)
    for (int i = 0; i < count; i++) {
        out[i] = in[i] * scalar;
    }
}

void transform_vector_list_fp(int mat[4][4] ALIGNED16,
                             int (* restrict vecs_in)[4] ALIGNED16,
                             int (* restrict vecs_out)[4] ALIGNED16,
                             int count) {
    // Load entire matrix into local variables
    const int m00 = mat[0][0], m01 = mat[0][1], m02 = mat[0][2], m03 = mat[0][3];
    const int m10 = mat[1][0], m11 = mat[1][1], m12 = mat[1][2], m13 = mat[1][3];
    const int m20 = mat[2][0], m21 = mat[2][1], m22 = mat[2][2], m23 = mat[2][3];
    const int m30 = mat[3][0], m31 = mat[3][1], m32 = mat[3][2], m33 = mat[3][3];

    #pragma GCC unroll 4
    for (int v = 0; v < count; v++) {
        // Load input vector
        int v0 = vecs_in[v][0];
        int v1 = vecs_in[v][1];
        int v2 = vecs_in[v][2];
        int v3 = vecs_in[v][3];

        // Compute output vector using fixed-point multiply
        // Each multiply produces 64-bit result, shift right by 16 to get Q16.16
        vecs_out[v][0] = (int)(((long long)m00*v0 + (long long)m01*v1 + (long long)m02*v2 + (long long)m03*v3) >> FP_SHIFT);
        vecs_out[v][1] = (int)(((long long)m10*v0 + (long long)m11*v1 + (long long)m12*v2 + (long long)m13*v3) >> FP_SHIFT);
        vecs_out[v][2] = (int)(((long long)m20*v0 + (long long)m21*v1 + (long long)m22*v2 + (long long)m23*v3) >> FP_SHIFT);
        vecs_out[v][3] = (int)(((long long)m30*v0 + (long long)m31*v1 + (long long)m32*v2 + (long long)m33*v3) >> FP_SHIFT);
    }
}

void test_dual_pipeline_int(unsigned int * restrict data ALIGNED16,
                           int count, int scalar,
                           unsigned int result[4] ALIGNED16) {
    // 4 independent accumulators for multiply-add
    unsigned int sum0 = 0, sum1 = 0, sum2 = 0, sum3 = 0;

    // Process 4 elements at a time with MADD operations
    for (int i = 0; i < count; i += 4) {
        unsigned int v0 = data[i];
        unsigned int v1 = data[i+1];
        unsigned int v2 = data[i+2];
        unsigned int v3 = data[i+3];

        // 4 independent multiply-adds - compiler should use MADD
        sum0 += v0 * scalar;
        sum1 += v1 * scalar;
        sum2 += v2 * scalar;
        sum3 += v3 * scalar;
    }

    // Store results
    result[0] = sum0;
    result[1] = sum1;
    result[2] = sum2;
    result[3] = sum3;
}

void test_dual_pipeline_madd(unsigned int * restrict data ALIGNED16,
                            int count, int scalar,
                            unsigned int result[2] ALIGNED16) {
    // 2 accumulators - can use LO and LO1 registers
    unsigned int sum0 = 0, sum1 = 0;

    // Process 2 elements at a time - allows madd + madd1 dual-issue
    #pragma GCC unroll 4
    for (int i = 0; i < count; i += 2) {
        unsigned int v0 = data[i];
        unsigned int v1 = data[i+1];

        sum0 += v0 * scalar;
        sum1 += v1 * scalar;
    }

    result[0] = sum0;
    result[1] = sum1;
}
