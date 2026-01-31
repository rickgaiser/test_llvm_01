#include <stdio.h>
#include "fpu_tests.h"

void print_matrix(const char *name, float mat[MAT_SIZE][MAT_SIZE]) {
    printf("%s:\n", name);
    for (int i = 0; i < MAT_SIZE; i++) {
        printf("  [");
        for (int j = 0; j < MAT_SIZE; j++) {
            printf("%7.2f", mat[i][j]);
            if (j < MAT_SIZE - 1) printf(", ");
        }
        printf("]\n");
    }
}

void print_vector(const char *name, float vec[MAT_SIZE]) {
    printf("%s: [", name);
    for (int i = 0; i < MAT_SIZE; i++) {
        printf("%7.2f", vec[i]);
        if (i < MAT_SIZE - 1) printf(", ");
    }
    printf("]\n");
}

void matrix_vector_multiply(float (* restrict mat)[MAT_SIZE] ALIGNED16,
                           float * restrict vec ALIGNED16,
                           float * restrict result ALIGNED16) {
    for (int i = 0; i < MAT_SIZE; i++) {
        float sum = 0.0f;
        for (int j = 0; j < MAT_SIZE; j++) {
            sum += mat[i][j] * vec[j]; // ROW major
            //sum += mat[j][i] * vec[j]; // COLUMN major
        }
        result[i] = sum;
    }
}

void matrix_matrix_multiply(float (* restrict a)[MAT_SIZE] ALIGNED16,
                           float (* restrict b)[MAT_SIZE] ALIGNED16,
                           float (* restrict result)[MAT_SIZE] ALIGNED16) {
    for (int i = 0; i < MAT_SIZE; i++) {
        for (int j = 0; j < MAT_SIZE; j++) {
            float sum = 0.0f;
            for (int k = 0; k < MAT_SIZE; k++) {
                sum += a[i][k] * b[k][j];
            }
            result[i][j] = sum;
        }
    }
}

// Row-major: mat[row][col], result[i] = sum_j(mat[i][j] * vec[j])
void transform_vector_list(float (* restrict mat)[MAT_SIZE] ALIGNED16,
                          float (* restrict vecs_in)[MAT_SIZE] ALIGNED16,
                          float (* restrict vecs_out)[MAT_SIZE] ALIGNED16,
                          int count) {
    for (int v = 0; v < count; v++) {
        for (int i = 0; i < MAT_SIZE; i++) {
            float sum = 0.0f;
            for (int j = 0; j < MAT_SIZE; j++) {
                sum += mat[i][j] * vecs_in[v][j];
            }
            vecs_out[v][i] = sum;
        }
    }
}

// FPU compare test - tests all comparison operations
void fpu_compare_test(float a, float b, fpu_cmp_result_t *result) {
    result->eq = (a == b) ? 1 : 0;
    result->lt = (a < b) ? 1 : 0;
    result->le = (a <= b) ? 1 : 0;
    result->gt = (a > b) ? 1 : 0;
    result->ge = (a >= b) ? 1 : 0;
    result->ne = (a != b) ? 1 : 0;
}

// Helper to create positive/negative infinity and NaN
static float make_inf(int negative) {
    union { float f; unsigned int u; } u;
    u.u = negative ? 0xFF800000 : 0x7F800000;
    return u.f;
}

static float make_nan(void) {
    union { float f; unsigned int u; } u;
    u.u = 0x7FC00000;  // quiet NaN
    return u.f;
}

// Validate a single comparison result
static int validate_cmp(const char *name, float a, float b,
                       fpu_cmp_result_t *result, fpu_cmp_result_t *expected) {
    int pass = 1;
    if (result->eq != expected->eq) {
        printf("  FAIL %s: %.2f == %.2f: got %d, expected %d\n", name, a, b, result->eq, expected->eq);
        pass = 0;
    }
    if (result->lt != expected->lt) {
        printf("  FAIL %s: %.2f < %.2f: got %d, expected %d\n", name, a, b, result->lt, expected->lt);
        pass = 0;
    }
    if (result->le != expected->le) {
        printf("  FAIL %s: %.2f <= %.2f: got %d, expected %d\n", name, a, b, result->le, expected->le);
        pass = 0;
    }
    if (result->gt != expected->gt) {
        printf("  FAIL %s: %.2f > %.2f: got %d, expected %d\n", name, a, b, result->gt, expected->gt);
        pass = 0;
    }
    if (result->ge != expected->ge) {
        printf("  FAIL %s: %.2f >= %.2f: got %d, expected %d\n", name, a, b, result->ge, expected->ge);
        pass = 0;
    }
    if (result->ne != expected->ne) {
        printf("  FAIL %s: %.2f != %.2f: got %d, expected %d\n", name, a, b, result->ne, expected->ne);
        pass = 0;
    }
    return pass;
}

// Comprehensive FPU compare validation
int fpu_compare_validate_all(void) {
    fpu_cmp_result_t result, expected;
    int total_pass = 1;
    int tests_run = 0;
    int tests_passed = 0;

    // Test cases: {a, b, eq, lt, le, gt, ge, ne}
    struct {
        float a, b;
        int eq, lt, le, gt, ge, ne;
        const char *name;
    } tests[] = {
        // Basic comparisons
        {1.0f, 2.0f,   0, 1, 1, 0, 0, 1, "1 vs 2"},
        {2.0f, 1.0f,   0, 0, 0, 1, 1, 1, "2 vs 1"},
        {1.0f, 1.0f,   1, 0, 1, 0, 1, 0, "1 vs 1"},

        // Zero comparisons
        {0.0f, 0.0f,   1, 0, 1, 0, 1, 0, "0 vs 0"},
        {-0.0f, 0.0f,  1, 0, 1, 0, 1, 0, "-0 vs +0"},
        {0.0f, -0.0f,  1, 0, 1, 0, 1, 0, "+0 vs -0"},

        // Negative numbers
        {-1.0f, -2.0f, 0, 0, 0, 1, 1, 1, "-1 vs -2"},
        {-2.0f, -1.0f, 0, 1, 1, 0, 0, 1, "-2 vs -1"},
        {-1.0f, 1.0f,  0, 1, 1, 0, 0, 1, "-1 vs 1"},

        // Small differences
        {1.0f, 1.0000001f, 0, 1, 1, 0, 0, 1, "1 vs 1+eps"},
        {1.0000001f, 1.0f, 0, 0, 0, 1, 1, 1, "1+eps vs 1"},

        // Large numbers
        {1e30f, 1e30f,    1, 0, 1, 0, 1, 0, "1e30 vs 1e30"},
        {1e30f, -1e30f,   0, 0, 0, 1, 1, 1, "1e30 vs -1e30"},

        // Small numbers (denormals may behave differently)
        {1e-30f, 1e-30f,  1, 0, 1, 0, 1, 0, "1e-30 vs 1e-30"},
        {1e-30f, 2e-30f,  0, 1, 1, 0, 0, 1, "1e-30 vs 2e-30"},
    };

    int num_tests = sizeof(tests) / sizeof(tests[0]);

    printf("--- FPU Compare Tests ---\n");

    for (int i = 0; i < num_tests; i++) {
        fpu_compare_test(tests[i].a, tests[i].b, &result);
        expected.eq = tests[i].eq;
        expected.lt = tests[i].lt;
        expected.le = tests[i].le;
        expected.gt = tests[i].gt;
        expected.ge = tests[i].ge;
        expected.ne = tests[i].ne;

        tests_run++;
        if (validate_cmp(tests[i].name, tests[i].a, tests[i].b, &result, &expected)) {
            tests_passed++;
        } else {
            total_pass = 0;
        }
    }

    // Test infinity
    float pos_inf = make_inf(0);
    float neg_inf = make_inf(1);

    printf("--- Infinity Tests ---\n");

    // +inf vs +inf
    fpu_compare_test(pos_inf, pos_inf, &result);
    expected = (fpu_cmp_result_t){1, 0, 1, 0, 1, 0};
    tests_run++;
    if (validate_cmp("+inf vs +inf", pos_inf, pos_inf, &result, &expected)) {
        tests_passed++;
    } else {
        total_pass = 0;
    }

    // +inf vs 1.0
    fpu_compare_test(pos_inf, 1.0f, &result);
    expected = (fpu_cmp_result_t){0, 0, 0, 1, 1, 1};
    tests_run++;
    if (validate_cmp("+inf vs 1", pos_inf, 1.0f, &result, &expected)) {
        tests_passed++;
    } else {
        total_pass = 0;
    }

    // -inf vs 1.0
    fpu_compare_test(neg_inf, 1.0f, &result);
    expected = (fpu_cmp_result_t){0, 1, 1, 0, 0, 1};
    tests_run++;
    if (validate_cmp("-inf vs 1", neg_inf, 1.0f, &result, &expected)) {
        tests_passed++;
    } else {
        total_pass = 0;
    }

    // +inf vs -inf
    fpu_compare_test(pos_inf, neg_inf, &result);
    expected = (fpu_cmp_result_t){0, 0, 0, 1, 1, 1};
    tests_run++;
    if (validate_cmp("+inf vs -inf", pos_inf, neg_inf, &result, &expected)) {
        tests_passed++;
    } else {
        total_pass = 0;
    }

    // NaN tests - NaN comparisons should all be false except !=
    printf("--- NaN Tests ---\n");
    float nan_val = make_nan();

    // NaN vs 1.0
    fpu_compare_test(nan_val, 1.0f, &result);
    expected = (fpu_cmp_result_t){0, 0, 0, 0, 0, 1};
    tests_run++;
    if (validate_cmp("NaN vs 1", nan_val, 1.0f, &result, &expected)) {
        tests_passed++;
    } else {
        total_pass = 0;
    }

    // 1.0 vs NaN
    fpu_compare_test(1.0f, nan_val, &result);
    expected = (fpu_cmp_result_t){0, 0, 0, 0, 0, 1};
    tests_run++;
    if (validate_cmp("1 vs NaN", 1.0f, nan_val, &result, &expected)) {
        tests_passed++;
    } else {
        total_pass = 0;
    }

    // NaN vs NaN
    fpu_compare_test(nan_val, nan_val, &result);
    expected = (fpu_cmp_result_t){0, 0, 0, 0, 0, 1};
    tests_run++;
    if (validate_cmp("NaN vs NaN", nan_val, nan_val, &result, &expected)) {
        tests_passed++;
    } else {
        total_pass = 0;
    }

    printf("FPU Compare: %d/%d tests passed\n", tests_passed, tests_run);
    return total_pass;
}

