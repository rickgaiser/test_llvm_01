#define MAT_SIZE 4

// The following code is very slow on LLVM when using the machine scheduler
// cause: spilling of HI/LO accumulator registers
void int_scalar_multiply(int * restrict in,
                        int * restrict out,
                        int count, int scalar) {
    #pragma GCC unroll 4
    for (int i = 0; i < count; i++) {
        out[i] = in[i] * scalar;
    }
}

// The following code is very slow on LLVM when NOT using the machine scheduler
void matrix_vector_multiply(float (* restrict mat)[MAT_SIZE], float * restrict vec, float * restrict result) {
    for (int i = 0; i < MAT_SIZE; i++) {
        float sum = 0.0f;
        for (int j = 0; j < MAT_SIZE; j++) {
            sum += mat[i][j] * vec[j];
        }
        result[i] = sum;
    }
}
