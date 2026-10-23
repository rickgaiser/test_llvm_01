#include <stdio.h>
#include <string.h>
#include <timer.h>
#include "fpu_tests.h"
#include "int_tests.h"
#include "vpu_tests.h"
#include "stream_tests.h"
#include "uarch_tests.h"
#include "app_tests.h"
#include "shuffle_tests.h"

#define NUM_ITERATIONS 100000

// PS2 Scratchpad RAM: 16KB at 0x70000000, single-cycle latency.
// It behaves as 4 banks of 4KB (measured, uarch_tests.c): a load after a
// store costs 2 cycles more if it is in another 4KB block than the byte
// after the store (store address + 16), also when they are far apart.
// Otherwise, or without a store before it, it costs nothing. So each bank
// holds the outputs of half of the vectors, then their inputs, and the SPRAM
// tests run once per bank: the loads of a call are in the bank of its
// stores, also after the last store of the bank.
#define SPRAM_BASE 0x70000000
#define SPRAM_HALF (VPU_NUM_VECTORS / 2)
#define SPRAM_OUT(b) ((v4sf*)(SPRAM_BASE + (b) * 4096))                  // 2KB
#define SPRAM_IN(b)  ((v4sf*)(SPRAM_BASE + (b) * 4096 + SPRAM_HALF * sizeof(v4sf)))
// n vectors, n / 2 in each bank
#define SPRAM_RUN(fn, n)                                                  \
    do {                                                                  \
        fn(vpu_mat, SPRAM_IN(0), SPRAM_OUT(0), (n) / 2);                  \
        fn(vpu_mat, SPRAM_IN(1), SPRAM_OUT(1), (n) / 2);                  \
    } while (0)
#define SPRAM_LOAD()                                                      \
    do {                                                                  \
        memcpy(SPRAM_IN(0), vpu_vec_a, SPRAM_HALF * sizeof(v4sf));        \
        memcpy(SPRAM_IN(1), vpu_vec_a + SPRAM_HALF,                       \
               SPRAM_HALF * sizeof(v4sf));                                \
    } while (0)

// FPU test data
static float mat_a[MAT_SIZE][MAT_SIZE] = {
    {1.0f, 2.0f, 3.0f, 4.0f},
    {5.0f, 6.0f, 7.0f, 8.0f},
    {9.0f, 10.0f, 11.0f, 12.0f},
    {13.0f, 14.0f, 15.0f, 16.0f}
};

static float mat_b[MAT_SIZE][MAT_SIZE] = {
    {0.5f, 0.1f, 0.0f, 0.0f},
    {0.1f, 0.5f, 0.1f, 0.0f},
    {0.0f, 0.1f, 0.5f, 0.1f},
    {0.0f, 0.0f, 0.1f, 0.5f}
};

static float vec_in[MAT_SIZE] = {1.0f, 2.0f, 3.0f, 4.0f};
static float vec_out[MAT_SIZE];
static float mat_result[MAT_SIZE][MAT_SIZE];
static float vec_list_in[NUM_VECTORS][MAT_SIZE];
static float vec_list_out[NUM_VECTORS][MAT_SIZE];

// Integer test data
static int int_list_in[NUM_INTS];
static int int_list_out[NUM_INTS];
static int fp_mat_a[4][4];
static int fp_vec_list_in[256][4];
static int fp_vec_list_out[256][4];

// Dual pipeline result arrays
static unsigned int dual_result[4];
static unsigned int dual_result2[2];

// VPU test data (aligned for vector operations)
static v4sf vpu_mat[4] __attribute__((aligned(16)));
static v4sf vpu_vec_a[VPU_NUM_VECTORS] __attribute__((aligned(16)));
static v4sf vpu_vec_b[VPU_NUM_VECTORS] __attribute__((aligned(16)));
static v4sf vpu_vec_out[VPU_NUM_VECTORS] __attribute__((aligned(16)));
static float vpu_dot_out[VPU_NUM_VECTORS] __attribute__((aligned(16)));
static v4sf vpu_hm_in[VPU_NUM_VECTORS] __attribute__((aligned(16)));
static float vpu_hmax_out[VPU_NUM_VECTORS] __attribute__((aligned(16)));
static float vpu_hmin_out[VPU_NUM_VECTORS] __attribute__((aligned(16)));

// Larger arrays for main memory (cached) testing
#define MAIN_MEM_VECS 4080  // 4080 vectors = 65280 bytes
static v4sf main_mem_vec_in[MAIN_MEM_VECS] __attribute__((aligned(64)));
static v4sf main_mem_vec_out[MAIN_MEM_VECS] __attribute__((aligned(64)));

// Streaming test data (1MB each, much larger than the 8KB data cache)
static int stream_in[STREAM_INTS] __attribute__((aligned(64)));
static int stream_out[STREAM_INTS] __attribute__((aligned(64)));
static float stream_x[STREAM_INTS] __attribute__((aligned(64)));
static float stream_y[STREAM_INTS] __attribute__((aligned(64)));

static float stream_mbs(u64 ticks, int bytes_per_elem) {
    float usec = (float)ticks / 150.0f;
    return ((float)bytes_per_elem * STREAM_INTS * STREAM_PASSES) / usec;
}

static void print_stream(const char *kernel, float mbs) {
    char name[32];
    snprintf(name, sizeof(name), "%s:", kernel);
    printf("  %-15s %7.2f MB/s\n", name, mbs);
}

int main() {
    printf("Hello Playstation 2!\n\n");

    // Initialize FPU vector list with test data
    for (int v = 0; v < NUM_VECTORS; v++) {
        for (int i = 0; i < MAT_SIZE; i++) {
            vec_list_in[v][i] = (float)(v * MAT_SIZE + i) * 0.1f;
        }
    }

    // Initialize integer list with test data
    for (int i = 0; i < NUM_INTS; i++) {
        int_list_in[i] = i + 1;
    }

    // Initialize fixed-point matrix (copy from mat_a)
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            fp_mat_a[i][j] = FLOAT_TO_FP(mat_a[i][j]);
        }
    }

    // Initialize fixed-point vector list
    for (int v = 0; v < 256; v++) {
        for (int i = 0; i < 4; i++) {
            fp_vec_list_in[v][i] = FLOAT_TO_FP((float)(v * 4 + i) * 0.1f);
        }
    }

    // Initialize VPU test data
    for (int i = 0; i < 4; i++) {
        vpu_mat[i] = (v4sf){mat_a[i][0], mat_a[i][1], mat_a[i][2], mat_a[i][3]};
    }
    for (int v = 0; v < VPU_NUM_VECTORS; v++) {
        float base = (float)v * 0.1f;
        vpu_vec_a[v] = (v4sf){base, base + 0.1f, base + 0.2f, base + 0.3f};
        vpu_vec_b[v] = (v4sf){1.0f, 2.0f, 3.0f, 4.0f};
    }
    // Horizontal max/min input: the largest and smallest lane vary
    unsigned int hm_seed = 12345;
    for (int v = 0; v < VPU_NUM_VECTORS; v++) {
        float lane[4];
        for (int i = 0; i < 4; i++) {
            hm_seed = hm_seed * 1103515245u + 12345u;
            lane[i] = (float)((int)(hm_seed >> 16 & 0x7fff) - 16384) * 0.25f;
        }
        vpu_hm_in[v] = (v4sf){lane[0], lane[1], lane[2], lane[3]};
    }

    // Initialize main memory test data
    for (int v = 0; v < MAIN_MEM_VECS; v++) {
        float base = (float)v * 0.1f;
        main_mem_vec_in[v] = (v4sf){base, base + 0.1f, base + 0.2f, base + 0.3f};
    }

    // Show input data
    print_matrix("Matrix A", mat_a);
    printf("\n");
    print_matrix("Matrix B", mat_b);
    printf("\n");
    print_vector("Vector", vec_in);
    printf("\n");

    int num_batches = NUM_ITERATIONS / NUM_VECTORS;
    int int_batches = NUM_ITERATIONS / NUM_INTS;
    int vpu_batches = NUM_ITERATIONS / VPU_NUM_VECTORS;
    int scalar = 7;

    // Result variables for summary
    float mops1, mops2, mops3, mops4, mops5;
    float mflops1, mflops2, mflops3;
    float mflops_vpu1, mflops_vpu2, mflops_vpu3, mflops_vpu4, mflops_vpu5;

    // === INT Tests ===
    printf("=== INT Tests ===\n\n");

    // Test 1: Fixed-point vector list transformation
    printf("--- Test 1: Vec List FIXED-POINT Q16.16 (%d batches) ---\n", num_batches);

    u64 start1 = GetTimerSystemTime();
    for (int iter = 0; iter < num_batches; iter++) {
        transform_vector_list_fp(fp_mat_a, fp_vec_list_in, fp_vec_list_out, NUM_VECTORS);
    }
    u64 end1 = GetTimerSystemTime();

    u64 ticks1 = end1 - start1;
    float usec1 = (float)ticks1 / 150.0f;
    float ops_per_iter1 = NUM_VECTORS * MAT_SIZE * MAT_SIZE * 2;
    mops1 = (ops_per_iter1 * num_batches) / usec1;
    float vecs_per_sec1 = (NUM_VECTORS * num_batches) / (usec1 / 1000000.0f);

    printf("First output: [%.2f, %.2f, %.2f, %.2f]\n",
           FP_TO_FLOAT(fp_vec_list_out[0][0]), FP_TO_FLOAT(fp_vec_list_out[0][1]),
           FP_TO_FLOAT(fp_vec_list_out[0][2]), FP_TO_FLOAT(fp_vec_list_out[0][3]));
    printf("Time: %llu ticks (%.2f us)\n", ticks1, usec1);
    printf("Performance: %.2f MOPS (%.0f vecs/sec)\n\n", mops1, vecs_per_sec1);

    // Test 2: Integer scalar multiplication
    printf("--- Test 2: Int Scalar Multiply (%d ints x %d batches) ---\n", NUM_INTS, int_batches);

    u64 start2 = GetTimerSystemTime();
    for (int iter = 0; iter < int_batches; iter++) {
        int_scalar_multiply(int_list_in, int_list_out, NUM_INTS, scalar);
    }
    u64 end2 = GetTimerSystemTime();

    u64 ticks2 = end2 - start2;
    float usec2 = (float)ticks2 / 150.0f;
    float ops2 = (float)NUM_INTS * int_batches;
    mops2 = ops2 / usec2;
    float ints_per_sec = ops2 / (usec2 / 1000000.0f);

    printf("First: %d * %d = %d\n", int_list_in[0], scalar, int_list_out[0]);
    printf("Last:  %d * %d = %d\n", int_list_in[NUM_INTS-1], scalar, int_list_out[NUM_INTS-1]);
    printf("Time: %llu ticks (%.2f us)\n", ticks2, usec2);
    printf("Performance: %.2f MOPS (%.0f ints/sec)\n\n", mops2, ints_per_sec);

    // Test 3: Dual pipeline integer test
    printf("--- Test 3: Dual Pipeline Int (%d ints x %d batches) ---\n", NUM_INTS, int_batches);

    u64 start3 = GetTimerSystemTime();
    for (int iter = 0; iter < int_batches; iter++) {
        test_dual_pipeline_int((unsigned int*)int_list_in, NUM_INTS, scalar, dual_result);
    }
    u64 end3 = GetTimerSystemTime();

    u64 ticks3 = end3 - start3;
    float usec3 = (float)ticks3 / 150.0f;
    float ops3 = (float)NUM_INTS * int_batches * 2;
    mops3 = ops3 / usec3;
    float cycles_per_8 = (float)ticks3 / ((float)NUM_INTS * int_batches / 8);

    printf("Time: %llu ticks (%.2f us)\n", ticks3, usec3);
    printf("Performance: %.2f MOPS\n", mops3);
    printf("Cycles/8 elements: %.2f (ideal dual-issue = 8)\n\n", cycles_per_8);

    // Test 4: MADD version (2 accumulators)
    printf("--- Test 4: MADD Pipeline (2 accum, %d ints x %d batches) ---\n", NUM_INTS, int_batches);

    u64 start4 = GetTimerSystemTime();
    for (int iter = 0; iter < int_batches; iter++) {
        test_dual_pipeline_madd((unsigned int*)int_list_in, NUM_INTS, scalar, dual_result2);
    }
    u64 end4 = GetTimerSystemTime();

    u64 ticks4 = end4 - start4;
    float usec4 = (float)ticks4 / 150.0f;
    float ops4 = (float)NUM_INTS * int_batches * 2;
    mops4 = ops4 / usec4;
    float cycles_per_4 = (float)ticks4 / ((float)NUM_INTS * int_batches / 4);

    printf("Time: %llu ticks (%.2f us)\n", ticks4, usec4);
    printf("Performance: %.2f MOPS\n", mops4);
    printf("Cycles/4 elements: %.2f (ideal dual-issue = 4)\n\n", cycles_per_4);

    // Test 5: independent divides
    printf("--- Test 5: Int Divide (%d ints x %d batches) ---\n", NUM_INTS, int_batches);

    u64 start_div = GetTimerSystemTime();
    for (int iter = 0; iter < int_batches; iter++) {
        int_divide(int_list_in, int_list_out, NUM_INTS);
    }
    u64 end_div = GetTimerSystemTime();

    u64 ticks_div = end_div - start_div;
    float usec_div = (float)ticks_div / 150.0f;
    mops5 = (float)NUM_INTS * int_batches / usec_div;
    float cycles_per_div = (float)ticks_div / ((float)NUM_INTS * int_batches);

    printf("First: %d -> %d, Last: %d -> %d\n", int_list_in[0], int_list_out[0],
           int_list_in[NUM_INTS-1], int_list_out[NUM_INTS-1]);
    printf("Time: %llu ticks (%.2f us)\n", ticks_div, usec_div);
    printf("Performance: %.2f MOPS\n", mops5);
    printf("Ticks/divide: %.2f\n\n", cycles_per_div);

    // === FPU Compare Tests ===
    printf("=== FPU Compare Tests ===\n\n");
    int cmp_pass = fpu_compare_validate_all();
    printf("\n");

    // === FPU Special Arithmetic Tests (NaN / +-INF inputs) ===
    printf("=== FPU Special Arith Tests (NaN/INF) ===\n\n");
    int special_pass = fpu_special_arith_validate_all();
    printf("\n");

    // === FPU Tests ===
    printf("=== FPU Tests ===\n\n");

    // Test 5: Matrix x Vector
    printf("--- Test 5: Matrix x Vector (%d iterations) ---\n", NUM_ITERATIONS);

    u64 start5 = GetTimerSystemTime();
    for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
        matrix_vector_multiply(mat_a, vec_in, vec_out);
    }
    u64 end5 = GetTimerSystemTime();

    u64 ticks5 = end5 - start5;
    float usec5 = (float)ticks5 / 150.0f;
    float ops_per_iter5 = MAT_SIZE * MAT_SIZE * 2;
    mflops1 = (ops_per_iter5 * NUM_ITERATIONS) / usec5;

    print_vector("Result", vec_out);
    printf("Time: %llu ticks (%.2f us)\n", ticks5, usec5);
    printf("Performance: %.2f MFLOPS\n\n", mflops1);

    // Test 6: Matrix x Matrix
    printf("--- Test 6: Matrix x Matrix (%d iterations) ---\n", NUM_ITERATIONS);

    u64 start6 = GetTimerSystemTime();
    for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
        matrix_matrix_multiply(mat_a, mat_b, mat_result);
    }
    u64 end6 = GetTimerSystemTime();

    u64 ticks6 = end6 - start6;
    float usec6 = (float)ticks6 / 150.0f;
    float ops_per_iter6 = MAT_SIZE * MAT_SIZE * MAT_SIZE * 2;
    mflops2 = (ops_per_iter6 * NUM_ITERATIONS) / usec6;

    print_matrix("Result", mat_result);
    printf("Time: %llu ticks (%.2f us)\n", ticks6, usec6);
    printf("Performance: %.2f MFLOPS\n\n", mflops2);

    // Test 7: Transform Vector List
    printf("--- Test 7: Transform %d vectors (%d batches) ---\n", NUM_VECTORS, num_batches);

    u64 start7 = GetTimerSystemTime();
    for (int iter = 0; iter < num_batches; iter++) {
        transform_vector_list(mat_a, vec_list_in, vec_list_out, NUM_VECTORS);
    }
    u64 end7 = GetTimerSystemTime();

    u64 ticks7 = end7 - start7;
    float usec7 = (float)ticks7 / 150.0f;
    float ops_per_iter7 = NUM_VECTORS * MAT_SIZE * MAT_SIZE * 2;
    mflops3 = (ops_per_iter7 * num_batches) / usec7;
    float vecs_per_sec7 = (NUM_VECTORS * num_batches) / (usec7 / 1000000.0f);

    printf("First output: [%.2f, %.2f, %.2f, %.2f]\n",
           vec_list_out[0][0], vec_list_out[0][1], vec_list_out[0][2], vec_list_out[0][3]);
    printf("Last output:  [%.2f, %.2f, %.2f, %.2f]\n",
           vec_list_out[NUM_VECTORS-1][0], vec_list_out[NUM_VECTORS-1][1],
           vec_list_out[NUM_VECTORS-1][2], vec_list_out[NUM_VECTORS-1][3]);
    printf("Time: %llu ticks (%.2f us)\n", ticks7, usec7);
    printf("Performance: %.2f MFLOPS (%.0f vecs/sec)\n\n", mflops3, vecs_per_sec7);

    // === VPU Tests ===
    printf("=== VPU Tests ===\n\n");

    // Test 8: VPU Vector Multiply (v4sf)
    printf("--- Test 8: VPU Vec Multiply v4sf (%d vecs x %d batches) ---\n", VPU_NUM_VECTORS, vpu_batches);

    u64 start8 = GetTimerSystemTime();
    for (int iter = 0; iter < vpu_batches; iter++) {
        vpu_vector_mul(vpu_vec_a, vpu_vec_b, vpu_vec_out, VPU_NUM_VECTORS);
    }
    u64 end8 = GetTimerSystemTime();

    u64 ticks8 = end8 - start8;
    float usec8 = (float)ticks8 / 150.0f;
    float ops8 = (float)VPU_NUM_VECTORS * vpu_batches * 4;
    mflops_vpu1 = ops8 / usec8;
    float vpu_vecs_per_sec = (VPU_NUM_VECTORS * vpu_batches) / (usec8 / 1000000.0f);

    printf("First: [%.2f, %.2f, %.2f, %.2f]\n",
           vpu_vec_out[0][0], vpu_vec_out[0][1], vpu_vec_out[0][2], vpu_vec_out[0][3]);
    printf("Time: %llu ticks (%.2f us)\n", ticks8, usec8);
    printf("Performance: %.2f MFLOPS (%.0f vecs/sec)\n\n", mflops_vpu1, vpu_vecs_per_sec);

    // Test 9: VPU Vector Add (v4sf)
    printf("--- Test 9: VPU Vec Add v4sf (%d vecs x %d batches) ---\n", VPU_NUM_VECTORS, vpu_batches);

    u64 start9 = GetTimerSystemTime();
    for (int iter = 0; iter < vpu_batches; iter++) {
        vpu_vector_add(vpu_vec_a, vpu_vec_b, vpu_vec_out, VPU_NUM_VECTORS);
    }
    u64 end9 = GetTimerSystemTime();

    u64 ticks9 = end9 - start9;
    float usec9 = (float)ticks9 / 150.0f;
    float ops9 = (float)VPU_NUM_VECTORS * vpu_batches * 4;
    mflops_vpu2 = ops9 / usec9;

    printf("First: [%.2f, %.2f, %.2f, %.2f]\n",
           vpu_vec_out[0][0], vpu_vec_out[0][1], vpu_vec_out[0][2], vpu_vec_out[0][3]);
    printf("Time: %llu ticks (%.2f us)\n", ticks9, usec9);
    printf("Performance: %.2f MFLOPS\n\n", mflops_vpu2);

    // Test 10: VPU Vector MADD (v4sf)
    printf("--- Test 10: VPU Vec MADD v4sf (%d vecs x %d batches) ---\n", VPU_NUM_VECTORS, vpu_batches);

    u64 start10 = GetTimerSystemTime();
    for (int iter = 0; iter < vpu_batches; iter++) {
        vpu_vector_madd(vpu_vec_a, vpu_vec_b, vpu_vec_a, vpu_vec_out, VPU_NUM_VECTORS);
    }
    u64 end10 = GetTimerSystemTime();

    u64 ticks10 = end10 - start10;
    float usec10 = (float)ticks10 / 150.0f;
    float ops10 = (float)VPU_NUM_VECTORS * vpu_batches * 8;
    mflops_vpu3 = ops10 / usec10;

    printf("First: [%.2f, %.2f, %.2f, %.2f]\n",
           vpu_vec_out[0][0], vpu_vec_out[0][1], vpu_vec_out[0][2], vpu_vec_out[0][3]);
    printf("Time: %llu ticks (%.2f us)\n", ticks10, usec10);
    printf("Performance: %.2f MFLOPS\n\n", mflops_vpu3);

    // Test 11: VPU Matrix-Vector Multiply (v4sf) - using Scratchpad RAM
    printf("--- Test 11: VPU Mat x Vec v4sf SPRAM (%d vecs x %d batches) ---\n", VPU_NUM_VECTORS, vpu_batches);

    // Copy input vectors to scratchpad RAM (single-cycle latency)
    SPRAM_LOAD();

    u64 start11 = GetTimerSystemTime();
    for (int iter = 0; iter < vpu_batches; iter++) {
        SPRAM_RUN(vpu_matrix_vector_multiply, VPU_NUM_VECTORS);
    }
    u64 end11 = GetTimerSystemTime();

    u64 ticks11 = end11 - start11;
    float usec11 = (float)ticks11 / 150.0f;
    float ops11 = (float)VPU_NUM_VECTORS * vpu_batches * 28;
    mflops_vpu4 = ops11 / usec11;
    float vpu_matvec_per_sec = (VPU_NUM_VECTORS * vpu_batches) / (usec11 / 1000000.0f);

    printf("First: [%.2f, %.2f, %.2f, %.2f]\n",
           SPRAM_OUT(0)[0][0], SPRAM_OUT(0)[0][1], SPRAM_OUT(0)[0][2], SPRAM_OUT(0)[0][3]);
    printf("Time: %llu ticks (%.2f us)\n", ticks11, usec11);
    printf("Performance: %.2f MFLOPS (%.0f vecs/sec)\n\n", mflops_vpu4, vpu_matvec_per_sec);

    // Test 12: VPU Dot Product (v4sf)
    printf("--- Test 12: VPU Dot Product v4sf (%d vecs x %d batches) ---\n", VPU_NUM_VECTORS, vpu_batches);

    u64 start12 = GetTimerSystemTime();
    for (int iter = 0; iter < vpu_batches; iter++) {
        vpu_dot_product(vpu_vec_a, vpu_vec_b, vpu_dot_out, VPU_NUM_VECTORS);
    }
    u64 end12 = GetTimerSystemTime();

    u64 ticks12 = end12 - start12;
    float usec12 = (float)ticks12 / 150.0f;
    float ops12 = (float)VPU_NUM_VECTORS * vpu_batches * 7;
    mflops_vpu5 = ops12 / usec12;
    float vpu_dot_per_sec = (VPU_NUM_VECTORS * vpu_batches) / (usec12 / 1000000.0f);

    printf("First dot: %.2f\n", vpu_dot_out[0]);
    printf("Time: %llu ticks (%.2f us)\n", ticks12, usec12);
    printf("Performance: %.2f MFLOPS (%.0f dots/sec)\n\n", mflops_vpu5, vpu_dot_per_sec);

    // Test 12b: VPU Horizontal Max/Min of a + b (v4sf)
    printf("--- Test 12b: VPU Horizontal Max/Min v4sf (%d vecs x %d batches) ---\n", VPU_NUM_VECTORS, vpu_batches);

    u64 start12b = GetTimerSystemTime();
    for (int iter = 0; iter < vpu_batches; iter++) {
        vpu_hmax_hmin(vpu_hm_in, vpu_vec_b, vpu_hmax_out, vpu_hmin_out, VPU_NUM_VECTORS);
    }
    u64 end12b = GetTimerSystemTime();

    u64 ticks12b = end12b - start12b;
    float usec12b = (float)ticks12b / 150.0f;
    float mvecs_hm = (float)VPU_NUM_VECTORS * vpu_batches / usec12b;
    unsigned int hmax_bits = 0, hmin_bits = 0;
    for (int v = 0; v < VPU_NUM_VECTORS; v++) {
        union { float f; unsigned int u; } mx = {vpu_hmax_out[v]}, mn = {vpu_hmin_out[v]};
        hmax_bits = (hmax_bits << 1 | hmax_bits >> 31) ^ mx.u;
        hmin_bits = (hmin_bits << 1 | hmin_bits >> 31) ^ mn.u;
    }

    printf("First max/min: %.2f %.2f\n", vpu_hmax_out[0], vpu_hmin_out[0]);
    printf("Result: hmax 0x%08x hmin 0x%08x\n", hmax_bits, hmin_bits);
    printf("Time: %llu ticks (%.2f us)\n", ticks12b, usec12b);
    printf("Performance: %.2f Mvec/s\n\n", mvecs_hm);

#ifdef __mips_vu0
    // Test 13: VPU Matrix-Vector Multiply in assembly - using Scratchpad RAM
    float mflops_asm;
    SPRAM_LOAD();
    printf("--- Test 13: VPU ASM Mat x Vec SPRAM (%d vecs x %d batches) ---\n", VPU_NUM_VECTORS, vpu_batches);
    u64 start13 = GetTimerSystemTime();
    for (int iter = 0; iter < vpu_batches; iter++) {
        SPRAM_RUN(vpu_matrix_vector_multiply_asm, VPU_NUM_VECTORS);
    }
    u64 end13 = GetTimerSystemTime();
    u64 ticks13 = end13 - start13;
    float usec13 = (float)ticks13 / 150.0f;
    mflops_asm = ((float)VPU_NUM_VECTORS * vpu_batches * 28) / usec13;
    printf("First: [%.2f, %.2f, %.2f, %.2f]\n",
           SPRAM_OUT(0)[0][0], SPRAM_OUT(0)[0][1], SPRAM_OUT(0)[0][2], SPRAM_OUT(0)[0][3]);
    // Against the matrix product in C. VU0 does not round as IEEE does, so
    // allow a relative error of 1e-5.
    int asm_errors = 0;
    for (int v = 0; v < VPU_NUM_VECTORS; v++) {
        v4sf in = vpu_vec_a[v];
        v4sf out = SPRAM_OUT(v / SPRAM_HALF)[v % SPRAM_HALF];
        for (int c = 0; c < 4; c++) {
            float ref = vpu_mat[0][c] * in[0] + vpu_mat[1][c] * in[1] +
                        vpu_mat[2][c] * in[2] + vpu_mat[3][c] * in[3];
            float err = out[c] - ref, mag = ref < 0 ? -ref : ref;
            if (err < 0)
                err = -err;
            if (err > 1e-5f * (mag > 1.0f ? mag : 1.0f))
                asm_errors++;
        }
    }
    printf("Result: ASM = C: %s\n", asm_errors ? "NO" : "yes");
    printf("Time: %llu ticks, Performance: %.2f MFLOPS\n\n", ticks13, mflops_asm);

    // Test 13g: the same with main memory (cached) - realistic scenario
    // 4080 vectors = ~64KB input + ~64KB output, more than the 8KB data cache
    int main_mem_batches = 24;  // Process full array multiple times
    float mflops_main_mem;
    printf("--- Test 13g: VPU ASM Mat x Vec MAIN MEM (%d vecs x %d batches) ---\n", MAIN_MEM_VECS, main_mem_batches);
    u64 start13g = GetTimerSystemTime();
    for (int iter = 0; iter < main_mem_batches; iter++) {
        vpu_matrix_vector_multiply_asm(vpu_mat, main_mem_vec_in, main_mem_vec_out, MAIN_MEM_VECS);
    }
    u64 end13g = GetTimerSystemTime();
    u64 ticks13g = end13g - start13g;
    float usec13g = (float)ticks13g / 150.0f;
    mflops_main_mem = ((float)MAIN_MEM_VECS * main_mem_batches * 28) / usec13g;
    float cycles_per_vec_main = (float)(ticks13g * 2) / (MAIN_MEM_VECS * main_mem_batches);
    printf("First: [%.2f, %.2f, %.2f, %.2f]\n",
           main_mem_vec_out[0][0], main_mem_vec_out[0][1], main_mem_vec_out[0][2], main_mem_vec_out[0][3]);
    printf("Time: %llu ticks, Performance: %.2f MFLOPS\n", ticks13g, mflops_main_mem);
    printf("Cycles/vec: %.2f (6 on SPRAM)\n\n", cycles_per_vec_main);

    // Test 14: VPU Matrix-Vector Multiply - Compute Only (no memory in loop)
    int compute_iterations = NUM_ITERATIONS;
    printf("--- Test 14: VPU Mat x Vec COMPUTE ONLY (%d iterations) ---\n", compute_iterations);

    u64 start14 = GetTimerSystemTime();
    vpu_matrix_vector_multiply_compute_only(vpu_mat, vpu_vec_a, vpu_vec_out, compute_iterations);
    u64 end14 = GetTimerSystemTime();

    u64 ticks14 = end14 - start14;
    float usec14 = (float)ticks14 / 150.0f;
    float ops14 = (float)compute_iterations * 28;  // 28 FLOPs per vector
    float mflops_vpu7 = ops14 / usec14;

    printf("Result: [%.2f, %.2f, %.2f, %.2f]\n",
           vpu_vec_out[0][0], vpu_vec_out[0][1], vpu_vec_out[0][2], vpu_vec_out[0][3]);
    printf("Time: %llu ticks (%.2f us)\n", ticks14, usec14);
    printf("Performance: %.2f MFLOPS (pure compute, no memory)\n\n", mflops_vpu7);
#endif // __mips_vu0

    // === Stream Tests ===
    printf("=== Stream Tests (%d KB arrays, %d passes) ===\n\n",
           STREAM_INTS * 4 / 1024, STREAM_PASSES);

    for (int i = 0; i < STREAM_INTS; i++) {
        stream_in[i] = i & 255;
        stream_x[i] = (float)(i & 255);
        stream_y[i] = 1.0f;
    }

    // Read only (4 bytes per element)
    int sum = 0;
    u64 t0 = GetTimerSystemTime();
    for (int p = 0; p < STREAM_PASSES; p++)
        sum = stream_sum(stream_in, STREAM_INTS);
    u64 t1 = GetTimerSystemTime();
    float mbs_sum = stream_mbs(t1 - t0, 4);
    printf("--- Stream Sum ---\n");
    printf("Result: %d\n", sum);
    printf("Performance: %.2f MB/s\n\n", mbs_sum);

    // Read + write (8 bytes per element)
    t0 = GetTimerSystemTime();
    for (int p = 0; p < STREAM_PASSES; p++)
        stream_scale(stream_in, stream_out, STREAM_INTS);
    t1 = GetTimerSystemTime();
    float mbs_scale = stream_mbs(t1 - t0, 8);
    unsigned int check = 0;
    for (int i = 0; i < STREAM_INTS; i++)
        check += stream_out[i];
    printf("--- Stream Scale ---\n");
    printf("Result: %u\n", check);
    printf("Performance: %.2f MB/s\n\n", mbs_scale);

    // Read 2 + write 1 (12 bytes per element). y accumulates over all
    // passes; all values stay exact in single precision.
    t0 = GetTimerSystemTime();
    for (int p = 0; p < STREAM_PASSES; p++)
        stream_saxpy(stream_x, stream_y, STREAM_INTS, 0.5f);
    t1 = GetTimerSystemTime();
    float mbs_saxpy = stream_mbs(t1 - t0, 12);
    check = 0;
    for (int i = 0; i < STREAM_INTS; i++)
        check += (unsigned int)stream_y[i];
    printf("--- Stream SAXPY ---\n");
    printf("Result: %u\n", check);
    printf("Performance: %.2f MB/s\n\n", mbs_saxpy);

    shuffle_checks();

    // === Application kernels ===
    app_run_all();

    // === Microarchitecture tests ===
    printf("=== Microarchitecture (CPU cycles) ===\n\n");
    uarch_run_all((u32 *)stream_in, STREAM_INTS);

    // Summary
    printf("=== Summary ===\n");
    printf("INT:\n");
    printf("  Vec List FP:    %7.2f MOPS\n", mops1);
    printf("  Int Scalar:     %7.2f MOPS\n", mops2);
    printf("  Dual Pipeline:  %7.2f MOPS\n", mops3);
    printf("  MADD Pipeline:  %7.2f MOPS\n", mops4);
    printf("  Int Divide:     %7.2f MOPS\n", mops5);
    printf("FPU:\n");
    printf("  Mat x Vec:      %7.2f MFLOPS\n", mflops1);
    printf("  Mat x Mat:      %7.2f MFLOPS\n", mflops2);
    printf("  Vec List:       %7.2f MFLOPS\n", mflops3);
    printf("VPU:\n");
    printf("  Vec Mul:        %7.2f MFLOPS\n", mflops_vpu1);
    printf("  Vec Add:        %7.2f MFLOPS\n", mflops_vpu2);
    printf("  Vec MADD:       %7.2f MFLOPS\n", mflops_vpu3);
    printf("  Mat x Vec:      %7.2f MFLOPS\n", mflops_vpu4);
#ifdef __mips_vu0
    printf("  ASM Mat x Vec:  %7.2f MFLOPS\n", mflops_asm);
    printf("  ASM Main Mem:   %7.2f MFLOPS\n", mflops_main_mem);
    printf("  Compute Only:   %7.2f MFLOPS\n", mflops_vpu7);
#endif
    printf("  Dot Prod:       %7.2f MFLOPS\n", mflops_vpu5);
    printf("  HMax/HMin:      %7.2f Mvec/s\n", mvecs_hm);
    printf("STREAM:\n");
    print_stream("Sum", mbs_sum);
    print_stream("Scale", mbs_scale);
    print_stream("SAXPY", mbs_saxpy);
    app_print_summary();
    printf("=== Test Complete ===\n");

    return 0;
}
