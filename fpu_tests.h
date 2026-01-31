#ifndef FPU_TESTS_H
#define FPU_TESTS_H

#define MAT_SIZE 4
#define NUM_VECTORS 256

// 128-bit aligned types for VU0 operations
typedef float __attribute__((aligned(16))) vec4f[4];
typedef float __attribute__((aligned(16))) mat4f[4][4];

// Alignment hint macro - Clang supports on params, GCC doesn't
#ifdef __clang__
#define ALIGNED16 __attribute__((aligned(16)))
#else
#define ALIGNED16
#endif

// Utility functions
void print_matrix(const char *name, float mat[MAT_SIZE][MAT_SIZE]);
void print_vector(const char *name, float vec[MAT_SIZE]);

// Test functions - all pointers 128-bit aligned
void matrix_vector_multiply(float (* restrict mat)[MAT_SIZE] ALIGNED16,
                           float * restrict vec ALIGNED16,
                           float * restrict result ALIGNED16);

void matrix_matrix_multiply(float (* restrict a)[MAT_SIZE] ALIGNED16,
                           float (* restrict b)[MAT_SIZE] ALIGNED16,
                           float (* restrict result)[MAT_SIZE] ALIGNED16);

void transform_vector_list(float (* restrict mat)[MAT_SIZE] ALIGNED16,
                          float (* restrict vecs_in)[MAT_SIZE] ALIGNED16,
                          float (* restrict vecs_out)[MAT_SIZE] ALIGNED16,
                          int count);

// FPU compare test results structure
typedef struct {
    int eq;   // a == b
    int lt;   // a < b
    int le;   // a <= b
    int gt;   // a > b
    int ge;   // a >= b
    int ne;   // a != b
} fpu_cmp_result_t;

// FPU compare tests
void fpu_compare_test(float a, float b, fpu_cmp_result_t *result);
int fpu_compare_validate_all(void);

#endif // FPU_TESTS_H
