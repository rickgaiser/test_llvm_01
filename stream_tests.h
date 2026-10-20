#ifndef STREAM_TESTS_H
#define STREAM_TESTS_H

// Memory streaming tests: the arrays are much larger than the 8KB data cache,
// so every cache line comes from main memory. Each kernel exists with and
// without a software prefetch (PREF) a fixed number of bytes ahead.

#define STREAM_INTS (256 * 1024)  // 1MB per array
#define STREAM_LINE_INTS 16       // 64-byte cache line
#define STREAM_PASSES 16

// Prefetch distances in bytes; 0 = no prefetch
#define STREAM_NUM_DISTS 5
extern const int stream_dists[STREAM_NUM_DISTS];

typedef int (*stream_sum_fn)(const int *in, int count);
typedef void (*stream_scale_fn)(const int *in, int *out, int count);
typedef void (*stream_saxpy_fn)(const float *x, float *y, int count, float a);

// Indexed like stream_dists
extern const stream_sum_fn stream_sum[STREAM_NUM_DISTS];
extern const stream_scale_fn stream_scale[STREAM_NUM_DISTS];
extern const stream_saxpy_fn stream_saxpy[STREAM_NUM_DISTS];

#endif // STREAM_TESTS_H
