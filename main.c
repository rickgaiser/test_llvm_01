#include <stdio.h>
#include <string.h>
#include <timer.h>
#include "fpu_tests.h"
#include "int_tests.h"
#include "vpu_tests.h"

#define NUM_ITERATIONS 100000

// PS2 Scratchpad RAM: 16KB at 0x70000000, single-cycle latency
#define SPRAM_BASE 0x70000000
#define SPRAM_VEC_A   ((v4sf*)(SPRAM_BASE))                              // 0x70000000 - 4KB
#define SPRAM_VEC_OUT ((v4sf*)(SPRAM_BASE + VPU_NUM_VECTORS * sizeof(v4sf)))  // 0x70001000 - 4KB

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

// Larger arrays for main memory (cached) testing
#define MAIN_MEM_VECS 4080  // 4080 vectors = 65280 bytes, divisible by 12
static v4sf main_mem_vec_in[MAIN_MEM_VECS] __attribute__((aligned(64)));
static v4sf main_mem_vec_out[MAIN_MEM_VECS] __attribute__((aligned(64)));

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
    float mops1, mops2, mops3, mops4;
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

    // === FPU Compare Tests ===
    printf("=== FPU Compare Tests ===\n\n");
    int cmp_pass = fpu_compare_validate_all();
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
    memcpy(SPRAM_VEC_A, vpu_vec_a, VPU_NUM_VECTORS * sizeof(v4sf));

    u64 start11 = GetTimerSystemTime();
    for (int iter = 0; iter < vpu_batches; iter++) {
        vpu_matrix_vector_multiply(vpu_mat, SPRAM_VEC_A, SPRAM_VEC_OUT, VPU_NUM_VECTORS);
    }
    u64 end11 = GetTimerSystemTime();

    u64 ticks11 = end11 - start11;
    float usec11 = (float)ticks11 / 150.0f;
    float ops11 = (float)VPU_NUM_VECTORS * vpu_batches * 28;
    mflops_vpu4 = ops11 / usec11;
    float vpu_matvec_per_sec = (VPU_NUM_VECTORS * vpu_batches) / (usec11 / 1000000.0f);

    printf("First: [%.2f, %.2f, %.2f, %.2f]\n",
           SPRAM_VEC_OUT[0][0], SPRAM_VEC_OUT[0][1], SPRAM_VEC_OUT[0][2], SPRAM_VEC_OUT[0][3]);
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

#ifdef __mips_vu0
    // Test 13: VPU Matrix-Vector Multiply ASM variants - using Scratchpad RAM
    // Use 252 vectors (divisible by 1, 2, 3, 4), 248 for 8v (divisible by 8)
    int asm_vec_count = 252;
    int asm_vec_count_8v = 248;
    float mflops_asm_1v, mflops_asm_2v, mflops_asm_3v, mflops_asm_4v, mflops_asm_8v;

    // Re-copy input vectors to scratchpad
    memcpy(SPRAM_VEC_A, vpu_vec_a, VPU_NUM_VECTORS * sizeof(v4sf));

    // Test 13a: 1 vertex in flight
    printf("--- Test 13a: VPU ASM 1v SPRAM (%d vecs x %d batches) ---\n", asm_vec_count, vpu_batches);
    u64 start13a = GetTimerSystemTime();
    for (int iter = 0; iter < vpu_batches; iter++) {
        vpu_matrix_vector_multiply_asm_1v(vpu_mat, SPRAM_VEC_A, SPRAM_VEC_OUT, asm_vec_count);
    }
    u64 end13a = GetTimerSystemTime();
    u64 ticks13a = end13a - start13a;
    float usec13a = (float)ticks13a / 150.0f;
    mflops_asm_1v = ((float)asm_vec_count * vpu_batches * 28) / usec13a;
    printf("First: [%.2f, %.2f, %.2f, %.2f]\n",
           SPRAM_VEC_OUT[0][0], SPRAM_VEC_OUT[0][1], SPRAM_VEC_OUT[0][2], SPRAM_VEC_OUT[0][3]);
    printf("Time: %llu ticks, Performance: %.2f MFLOPS\n\n", ticks13a, mflops_asm_1v);

    // Test 13b: 2 vertices in flight
    printf("--- Test 13b: VPU ASM 2v SPRAM (%d vecs x %d batches) ---\n", asm_vec_count, vpu_batches);
    u64 start13b = GetTimerSystemTime();
    for (int iter = 0; iter < vpu_batches; iter++) {
        vpu_matrix_vector_multiply_asm_2v(vpu_mat, SPRAM_VEC_A, SPRAM_VEC_OUT, asm_vec_count);
    }
    u64 end13b = GetTimerSystemTime();
    u64 ticks13b = end13b - start13b;
    float usec13b = (float)ticks13b / 150.0f;
    mflops_asm_2v = ((float)asm_vec_count * vpu_batches * 28) / usec13b;
    printf("First: [%.2f, %.2f, %.2f, %.2f]\n",
           SPRAM_VEC_OUT[0][0], SPRAM_VEC_OUT[0][1], SPRAM_VEC_OUT[0][2], SPRAM_VEC_OUT[0][3]);
    printf("Time: %llu ticks, Performance: %.2f MFLOPS\n\n", ticks13b, mflops_asm_2v);

    // Test 13c: 3 vertices in flight
    printf("--- Test 13c: VPU ASM 3v SPRAM (%d vecs x %d batches) ---\n", asm_vec_count, vpu_batches);
    u64 start13c = GetTimerSystemTime();
    for (int iter = 0; iter < vpu_batches; iter++) {
        vpu_matrix_vector_multiply_asm_3v(vpu_mat, SPRAM_VEC_A, SPRAM_VEC_OUT, asm_vec_count);
    }
    u64 end13c = GetTimerSystemTime();
    u64 ticks13c = end13c - start13c;
    float usec13c = (float)ticks13c / 150.0f;
    mflops_asm_3v = ((float)asm_vec_count * vpu_batches * 28) / usec13c;
    printf("First: [%.2f, %.2f, %.2f, %.2f]\n",
           SPRAM_VEC_OUT[0][0], SPRAM_VEC_OUT[0][1], SPRAM_VEC_OUT[0][2], SPRAM_VEC_OUT[0][3]);
    printf("Time: %llu ticks, Performance: %.2f MFLOPS\n\n", ticks13c, mflops_asm_3v);

    // Test 13d: 4 vertices in flight
    printf("--- Test 13d: VPU ASM 4v SPRAM (%d vecs x %d batches) ---\n", asm_vec_count, vpu_batches);
    u64 start13d = GetTimerSystemTime();
    for (int iter = 0; iter < vpu_batches; iter++) {
        vpu_matrix_vector_multiply_asm_4v(vpu_mat, SPRAM_VEC_A, SPRAM_VEC_OUT, asm_vec_count);
    }
    u64 end13d = GetTimerSystemTime();
    u64 ticks13d = end13d - start13d;
    float usec13d = (float)ticks13d / 150.0f;
    mflops_asm_4v = ((float)asm_vec_count * vpu_batches * 28) / usec13d;
    printf("First: [%.2f, %.2f, %.2f, %.2f]\n",
           SPRAM_VEC_OUT[0][0], SPRAM_VEC_OUT[0][1], SPRAM_VEC_OUT[0][2], SPRAM_VEC_OUT[0][3]);
    printf("Time: %llu ticks, Performance: %.2f MFLOPS\n\n", ticks13d, mflops_asm_4v);

    // Test 13e: 8 vertices in flight (SPRAM)
    printf("--- Test 13e: VPU ASM 8v SPRAM (%d vecs x %d batches) ---\n", asm_vec_count_8v, vpu_batches);
    u64 start13e = GetTimerSystemTime();
    for (int iter = 0; iter < vpu_batches; iter++) {
        vpu_matrix_vector_multiply_asm_8v(vpu_mat, SPRAM_VEC_A, SPRAM_VEC_OUT, asm_vec_count_8v);
    }
    u64 end13e = GetTimerSystemTime();
    u64 ticks13e = end13e - start13e;
    float usec13e = (float)ticks13e / 150.0f;
    mflops_asm_8v = ((float)asm_vec_count_8v * vpu_batches * 28) / usec13e;
    printf("First: [%.2f, %.2f, %.2f, %.2f]\n",
           SPRAM_VEC_OUT[0][0], SPRAM_VEC_OUT[0][1], SPRAM_VEC_OUT[0][2], SPRAM_VEC_OUT[0][3]);
    printf("Time: %llu ticks, Performance: %.2f MFLOPS\n\n", ticks13e, mflops_asm_8v);

    // Test 13f: 12 vertices in flight (maximum registers)
    int asm_vec_count_12v = 240;  // divisible by 12
    float mflops_asm_12v;
    printf("--- Test 13f: VPU ASM 12v SPRAM (%d vecs x %d batches) ---\n", asm_vec_count_12v, vpu_batches);
    u64 start13f = GetTimerSystemTime();
    for (int iter = 0; iter < vpu_batches; iter++) {
        vpu_matrix_vector_multiply_asm_12v(vpu_mat, SPRAM_VEC_A, SPRAM_VEC_OUT, asm_vec_count_12v);
    }
    u64 end13f = GetTimerSystemTime();
    u64 ticks13f = end13f - start13f;
    float usec13f = (float)ticks13f / 150.0f;
    mflops_asm_12v = ((float)asm_vec_count_12v * vpu_batches * 28) / usec13f;
    printf("First: [%.2f, %.2f, %.2f, %.2f]\n",
           SPRAM_VEC_OUT[0][0], SPRAM_VEC_OUT[0][1], SPRAM_VEC_OUT[0][2], SPRAM_VEC_OUT[0][3]);
    printf("Time: %llu ticks, Performance: %.2f MFLOPS\n\n", ticks13f, mflops_asm_12v);

    // Test 13g: 12v with main memory (cached) - realistic scenario
    // 4080 vectors = ~64KB input + ~64KB output, exceeds L1 (16KB) but fits L2
    int main_mem_batches = 24;  // Process full array multiple times
    float mflops_main_mem;
    printf("--- Test 13g: VPU ASM 12v MAIN MEM (%d vecs x %d batches) ---\n", MAIN_MEM_VECS, main_mem_batches);
    u64 start13g = GetTimerSystemTime();
    for (int iter = 0; iter < main_mem_batches; iter++) {
        vpu_matrix_vector_multiply_asm_12v(vpu_mat, main_mem_vec_in, main_mem_vec_out, MAIN_MEM_VECS);
    }
    u64 end13g = GetTimerSystemTime();
    u64 ticks13g = end13g - start13g;
    float usec13g = (float)ticks13g / 150.0f;
    mflops_main_mem = ((float)MAIN_MEM_VECS * main_mem_batches * 28) / usec13g;
    float cycles_per_vec_main = (float)(ticks13g * 2) / (MAIN_MEM_VECS * main_mem_batches);
    printf("First: [%.2f, %.2f, %.2f, %.2f]\n",
           main_mem_vec_out[0][0], main_mem_vec_out[0][1], main_mem_vec_out[0][2], main_mem_vec_out[0][3]);
    printf("Time: %llu ticks, Performance: %.2f MFLOPS\n", ticks13g, mflops_main_mem);
    printf("Cycles/vec: %.2f (vs 6.23 SPRAM)\n\n", cycles_per_vec_main);

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

    // Summary
    printf("=== Summary ===\n");
    printf("INT:\n");
    printf("  Vec List FP:    %7.2f MOPS\n", mops1);
    printf("  Int Scalar:     %7.2f MOPS\n", mops2);
    printf("  Dual Pipeline:  %7.2f MOPS\n", mops3);
    printf("  MADD Pipeline:  %7.2f MOPS\n", mops4);
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
    printf("  ASM 1v:         %7.2f MFLOPS\n", mflops_asm_1v);
    printf("  ASM 2v:         %7.2f MFLOPS\n", mflops_asm_2v);
    printf("  ASM 3v:         %7.2f MFLOPS\n", mflops_asm_3v);
    printf("  ASM 4v:         %7.2f MFLOPS\n", mflops_asm_4v);
    printf("  ASM 8v:         %7.2f MFLOPS\n", mflops_asm_8v);
    printf("  ASM 12v:        %7.2f MFLOPS\n", mflops_asm_12v);
    printf("  Main Mem 12v:   %7.2f MFLOPS\n", mflops_main_mem);
    printf("  Compute Only:   %7.2f MFLOPS\n", mflops_vpu7);
#endif
    printf("  Dot Prod:       %7.2f MFLOPS\n", mflops_vpu5);
    printf("=== Test Complete ===\n");

    return 0;
}
