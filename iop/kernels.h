/* The kernels: each has a C version in kernels.c, compiled once per variant, and
   a hand-written one in kernels_asm.S. All have this form:

     u32 kernel(void *d, const void *s, u32 n, u32 x)

   d, s: destination and source; n: bytes; x: what the kernel needs besides. The
   source of each pattern is in kernels.c. */

#ifndef KERNELS_H
#define KERNELS_H

#ifndef _IOP
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
#else
#include <tamtypes.h>
#endif

typedef u32 kernel(void *d, const void *s, u32 n, u32 x);

#define KERNELS(X)                                                                         \
  X(cpy_byte) X(cpy_word) X(cpy_unal) X(set_word) X(str_len) X(net_sum) X(net_swap)        \
  X(mc_edc) X(mc_ecc) X(bit_rev) X(up_24000) X(up_gen) X(mix_vol) X(gain_q16) X(fifo_rd) X(ring_put)   \
  X(list_walk) X(fat_scan) X(crc32) X(itoa10) X(mul_const) X(mul64)

extern const u8 mc_xortable[256];
extern const u32 crc_table[256];
extern const short up_lut[512];

#endif
