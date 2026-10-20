#include <stdio.h>
#include "fpu_tests.h"
#include <math.h>

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

// Get raw IEEE 754 bits of a float
static unsigned int float_bits(float f) {
    union { float f; unsigned int u; } u;
    u.f = f;
    return u.u;
}

// Check if a float has IEEE NaN bit pattern (exp=0xFF, mantissa!=0)
static int is_nan_bits(float f) {
    unsigned int b = float_bits(f);
    return (b & 0x7F800000) == 0x7F800000 && (b & 0x007FFFFF) != 0;
}

// Check if a float has IEEE INF bit pattern (exp=0xFF, mantissa==0)
static int is_inf_bits(float f) {
    unsigned int b = float_bits(f);
    return (b & 0x7FFFFFFF) == 0x7F800000;
}

// Create max/min representable finite float (PS2 overflow clamp target)
//   positive: 0x7F7FFFFF =  3.40282347e+38f
//   negative: 0xFF7FFFFF = -3.40282347e+38f
static float make_max_float(int negative) {
    union { float f; unsigned int u; } u;
    u.u = negative ? 0xFF7FFFFF : 0x7F7FFFFF;
    return u.f;
}

// Compare one PS2 FPU result against the correct IEEE 754 expected bit pattern.
// ieee_bits = 0x7FC00000 signals "IEEE mandates NaN"; since PS2 never produces
// IEEE NaN these cases always show DIFF, documenting the deviation.
// Returns 1 (MATCH) or 0 (DIFF).
static int print_ieee_cmp(const char *desc, float result, unsigned int ieee_bits) {
    unsigned int got = float_bits(result);
    int ieee_is_nan = (ieee_bits & 0x7F800000) == 0x7F800000 && (ieee_bits & 0x007FFFFF) != 0;
    int match = ieee_is_nan ? is_nan_bits(result) : (got == ieee_bits);
    if (is_nan_bits(result))
        printf("  %-22s PS2=0x%08X[NaN]", desc, got);
    else if (is_inf_bits(result))
        printf("  %-22s PS2=0x%08X[INF]", desc, got);
    else
        printf("  %-22s PS2=0x%08X(%g)", desc, got, (double)result);
    if (ieee_is_nan)
        printf("  IEEE=NaN  %s\n", match ? "MATCH" : "DIFF");
    else if ((ieee_bits & 0x7FFFFFFF) == 0x7F800000)
        printf("  IEEE=%cINF  %s\n", (ieee_bits >> 31) ? '-' : '+', match ? "MATCH" : "DIFF");
    else
        printf("  IEEE=0x%08X  %s\n", ieee_bits, match ? "MATCH" : "DIFF");
    return match;
}

// Check that result matches expected PS2 overflow clamp value (PASS/FAIL)
static int check_clamp_result(const char *desc, float result, unsigned int expected_bits) {
    unsigned int got = float_bits(result);
    int pass = (got == expected_bits);
    const char *verdict = pass ? "PASS" : "FAIL";
    if (is_nan_bits(result))
        printf("  %-28s -> 0x%08X [NaN]  %s\n", desc, got, verdict);
    else if (is_inf_bits(result))
        printf("  %-28s -> 0x%08X [INF]  %s\n", desc, got, verdict);
    else
        printf("  %-28s -> 0x%08X (%g)  %s\n", desc, got, (double)result, verdict);
    if (!pass)
        printf("    expected 0x%08X\n", expected_bits);
    return pass;
}

// Compare fadd/fsub/fmul/fdiv/fsqrt results (NaN/INF inputs) against IEEE 754.
// Returns number of DIFF cases (0 = all match IEEE 754).
static int fpu_nan_inf_diagnostic(void) {
    volatile float pos_inf  = make_inf(0);  // 0x7F800000
    volatile float neg_inf  = make_inf(1);  // 0xFF800000
    volatile float nan_val  = make_nan();   // 0x7FC00000
    volatile float zero     = 0.0f;
    volatile float large    = 1000000.0f;
    volatile float neg_large= -1000000.0f;
    int matches = 0, total = 0;
    float r;

    printf("Note: MATCH=PS2 agrees with IEEE 754, DIFF=PS2 deviates\n");
    printf("      [NaN]/[INF]=IEEE bit patterns visible in PS2 output\n");

    // IEEE 754 fadd: finite+INF=INF, INF+INF=INF, INF+(-INF)=NaN, NaN+x=NaN
    printf("--- IEEE 754 vs PS2: fadd ---\n");
    r = pos_inf + large;    matches += print_ieee_cmp("+INF + 1e6",  r, 0x7F800000); total++;
    r = neg_inf + large;    matches += print_ieee_cmp("-INF + 1e6",  r, 0xFF800000); total++;
    r = pos_inf + pos_inf;  matches += print_ieee_cmp("+INF + +INF", r, 0x7F800000); total++;
    r = pos_inf + neg_inf;  matches += print_ieee_cmp("+INF + -INF", r, 0x7FC00000); total++;
    r = nan_val + large;    matches += print_ieee_cmp("NaN + 1e6",   r, 0x7FC00000); total++;
    r = nan_val + nan_val;  matches += print_ieee_cmp("NaN + NaN",   r, 0x7FC00000); total++;

    // IEEE 754 fsub: INF-finite=INF, INF-INF=NaN, INF-(-INF)=INF, NaN-x=NaN
    printf("--- IEEE 754 vs PS2: fsub ---\n");
    r = pos_inf - large;    matches += print_ieee_cmp("+INF - 1e6",  r, 0x7F800000); total++;
    r = neg_inf - large;    matches += print_ieee_cmp("-INF - 1e6",  r, 0xFF800000); total++;
    r = pos_inf - pos_inf;  matches += print_ieee_cmp("+INF - +INF", r, 0x7FC00000); total++;
    r = pos_inf - neg_inf;  matches += print_ieee_cmp("+INF - -INF", r, 0x7F800000); total++;
    r = nan_val - large;    matches += print_ieee_cmp("NaN - 1e6",   r, 0x7FC00000); total++;

    // IEEE 754 fmul: INF*nonzero=INF, INF*0=NaN, INF*INF=INF, NaN*x=NaN
    printf("--- IEEE 754 vs PS2: fmul ---\n");
    r = pos_inf * large;    matches += print_ieee_cmp("+INF * 1e6",  r, 0x7F800000); total++;
    r = neg_inf * large;    matches += print_ieee_cmp("-INF * 1e6",  r, 0xFF800000); total++;
    r = pos_inf * neg_large;matches += print_ieee_cmp("+INF * -1e6", r, 0xFF800000); total++;
    r = pos_inf * zero;     matches += print_ieee_cmp("+INF * 0",    r, 0x7FC00000); total++;
    r = neg_inf * zero;     matches += print_ieee_cmp("-INF * 0",    r, 0x7FC00000); total++;
    r = pos_inf * pos_inf;  matches += print_ieee_cmp("+INF * +INF", r, 0x7F800000); total++;
    r = nan_val * large;    matches += print_ieee_cmp("NaN * 1e6",   r, 0x7FC00000); total++;

    // IEEE 754 fdiv: INF/finite=INF, finite/0=INF, INF/0=INF
    //               INF/INF=NaN, 0/0=NaN, NaN/x=NaN
    printf("--- IEEE 754 vs PS2: fdiv ---\n");
    r = pos_inf / large;    matches += print_ieee_cmp("+INF / 1e6",  r, 0x7F800000); total++;
    r = neg_inf / large;    matches += print_ieee_cmp("-INF / 1e6",  r, 0xFF800000); total++;
    r = pos_inf / pos_inf;  matches += print_ieee_cmp("+INF / +INF", r, 0x7FC00000); total++;
    r = large   / zero;     matches += print_ieee_cmp("1e6 / 0",     r, 0x7F800000); total++;
    r = zero    / zero;     matches += print_ieee_cmp("0 / 0",       r, 0x7FC00000); total++;
    r = pos_inf / zero;     matches += print_ieee_cmp("+INF / 0",    r, 0x7F800000); total++;
    r = nan_val / large;    matches += print_ieee_cmp("NaN / 1e6",   r, 0x7FC00000); total++;

    // IEEE 754 fsqrt: sqrt(+INF)=+INF, sqrt(NaN)=NaN, sqrt(-x)=NaN, sqrt(0)=0
    // sqrtf() compiles to the sqrt.s FPU instruction on MIPS
    printf("--- IEEE 754 vs PS2: fsqrt ---\n");
    r = sqrtf(pos_inf);     matches += print_ieee_cmp("sqrt(+INF)",  r, 0x7F800000); total++;
    r = sqrtf(nan_val);     matches += print_ieee_cmp("sqrt(NaN)",   r, 0x7FC00000); total++;
    r = sqrtf(neg_large);   matches += print_ieee_cmp("sqrt(-1e6)",  r, 0x7FC00000); total++;
    r = sqrtf(neg_inf);     matches += print_ieee_cmp("sqrt(-INF)",  r, 0x7FC00000); total++;
    r = sqrtf(zero);        matches += print_ieee_cmp("sqrt(0)",     r, 0x00000000); total++;

    printf("IEEE 754 vs PS2: %d/%d MATCH, %d DIFF\n", matches, total, total - matches);
    return total - matches;
}

// PASS/FAIL: validate PS2 FPU overflow behaviour.
// Split into its own function for the same GP-relative addressing reason.
//
// Two distinct overflow modes observed on R5900:
//   A) Exponent overflow (result needs exponent > 127):
//        positive → 0x7FFFFFFF  negative → 0xFFFFFFFF
//   B) Mantissa saturation (exponent stays at 127, mantissa rounds to max):
//        positive → 0x7F7FFFFF  negative → 0xFF7FFFFF  (true clamp)
static int fpu_overflow_clamp_validate(void) {
    volatile float max_f    = make_max_float(0); // 0x7F7FFFFF
    volatile float neg_max_f= make_max_float(1); // 0xFF7FFFFF
    volatile float large    = 1000000.0f;        // 1e6
    volatile float neg_large= -1000000.0f;       // -1e6
    volatile float two      = 2.0f;
    volatile float half     = 0.5f;
    volatile float quarter  = 0.25f;
    int pass = 1;
    float r;

    // --- Mode A: exponent overflow → 0x7FFFFFFF / 0xFFFFFFFF ---
    printf("--- Exponent overflow (exp>127): fadd ---\n");
    r = max_f     + max_f;     pass &= check_clamp_result("max_f + max_f",    r, 0x7FFFFFFF);
    r = neg_max_f + neg_max_f; pass &= check_clamp_result("-max_f + -max_f",  r, 0xFFFFFFFF);

    printf("--- Exponent overflow (exp>127): fsub ---\n");
    r = neg_max_f - max_f;     pass &= check_clamp_result("-max_f - max_f",   r, 0xFFFFFFFF);
    r = max_f     - neg_max_f; pass &= check_clamp_result("max_f - -max_f",   r, 0x7FFFFFFF);

    printf("--- Exponent overflow (exp>127): fmul ---\n");
    r = max_f     * two;       pass &= check_clamp_result("max_f * 2",        r, 0x7FFFFFFF);
    r = neg_max_f * two;       pass &= check_clamp_result("-max_f * 2",       r, 0xFFFFFFFF);
    r = max_f     * max_f;     pass &= check_clamp_result("max_f * max_f",    r, 0x7FFFFFFF);
    r = large     * max_f;     pass &= check_clamp_result("1e6 * max_f",      r, 0x7FFFFFFF);

    printf("--- Exponent overflow (exp>127): fdiv ---\n");
    r = max_f     / half;      pass &= check_clamp_result("max_f / 0.5",      r, 0x7FFFFFFF);
    r = neg_max_f / half;      pass &= check_clamp_result("-max_f / 0.5",     r, 0xFFFFFFFF);
    r = max_f     / quarter;   pass &= check_clamp_result("max_f / 0.25",     r, 0x7FFFFFFF);

    // --- Mode B: mantissa saturation (exp stays 127) → true clamp ---
    // Adding a value whose exponent is far below 127 doesn't increase the
    // exponent; the mantissa rounds up and saturates at max_float.
    printf("--- Mantissa saturation (exp=127): fadd ---\n");
    r = max_f     + large;     pass &= check_clamp_result("max_f + 1e6",      r, 0x7F7FFFFF);
    r = neg_max_f + neg_large; pass &= check_clamp_result("-max_f + -1e6",    r, 0xFF7FFFFF);

    printf("--- Mantissa saturation (exp=127): fsub ---\n");
    r = max_f     - neg_large; pass &= check_clamp_result("max_f - -1e6",     r, 0x7F7FFFFF);

    return pass;
}

// Test fadd/fsub/fmul/fdiv/fsqrt with NaN and +-INF inputs (diagnostic),
// then validate that normal-float overflow clamps to +-max_float on PS2.
// Returns 1 if every clamping test passes, 0 otherwise.
int fpu_special_arith_validate_all(void) {
    int ieee_diffs = fpu_nan_inf_diagnostic();
    int clamp_pass = fpu_overflow_clamp_validate();
    printf("FPU NaN/INF vs IEEE 754: %d case(s) differ\n", ieee_diffs);
    printf("FPU Special Arith: clamping %s\n", clamp_pass ? "PASS" : "FAIL");
    return clamp_pass;
}
