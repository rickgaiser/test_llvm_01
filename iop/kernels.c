/* The patterns an IOP spends its time in, as ps2sdk's iop/ writes them. Compiled
   once per compiler and option set: V is the prefix of the names. */

#include "kernels.h"

#define CAT2(a, b) a##b
#define CAT(a, b) CAT2(a, b)
#define K(n) CAT(V, n)

/* ---- data ---- */

/* sysclib size_optimized/memcpy.c */
u32 K(cpy_byte)(void *d, const void *s, u32 n, u32 x) {
  u8 *p = d;
  const u8 *q = s;
  while (n--)
    *p++ = *q++;
  return 0;
}

/* sysclib _wmemcopy: aligned words */
u32 K(cpy_word)(void *d, const void *s, u32 n, u32 x) {
  u32 *p = d;
  const u32 *q = s;
  for (n /= 4; n; n--)
    *p++ = *q++;
  return 0;
}

/* a payload behind a 14-byte Ethernet header: source aligned to 2 */
typedef u32 __attribute__((aligned(1))) u32_unaligned;
u32 K(cpy_unal)(void *d, const void *s, u32 n, u32 x) {
  u32 *p = d;
  const u32_unaligned *q = s;
  for (n /= 4; n; n--)
    *p++ = *q++;
  return 0;
}

/* sysclib _wmemset */
u32 K(set_word)(void *d, const void *s, u32 n, u32 x) {
  u32 *p = d;
  for (n /= 4; n; n--)
    *p++ = x;
  return 0;
}

/* sysclib strlen */
u32 K(str_len)(void *d, const void *s, u32 n, u32 x) {
  const char *p = s;
  while (*p)
    p++;
  return p - (const char *)s;
}

/* ---- network ---- */

/* lwIP inet_chksum.c, lwip_standard_chksum, algorithm 2, even length */
u32 K(net_sum)(void *d, const void *s, u32 n, u32 x) {
  const u16 *ps = s;
  u32 sum = 0;
  while (n > 1) {
    sum += *ps++;
    n -= 2;
  }
  sum = (sum >> 16) + (sum & 0xffff);
  sum = (sum >> 16) + (sum & 0xffff);
  return sum;
}

/* htons() over the fields of a header, as lwIP's PP_HTONS */
u32 K(net_swap)(void *d, const void *s, u32 n, u32 x) {
  u16 *p = d;
  const u16 *q = s;
  for (n /= 2; n; n--) {
    u32 v = *q++;
    *p++ = (u16)((v << 8) | (v >> 8));
  }
  return 0;
}

/* smap xfer.c, CopyFromFIFO: the words a register gives, x is its address */
u32 K(fifo_rd)(void *d, const void *s, u32 n, u32 x) {
  volatile u32 *reg = (volatile u32 *)x;
  u32 *p = d;
  for (u32 i = 0; i < n; i += 4)
    p[i / 4] = *reg;
  return 0;
}

/* ---- storage ---- */

/* mcman main.c, mcman_calcEDC */
u32 K(mc_edc)(void *d, const void *s, u32 n, u32 x) {
  const u8 *p = s;
  u32 sum = 0;
  for (u32 i = 0; i < n; i++)
    sum ^= p[i];
  return sum & 0xff;
}

/* mcman main.c, McDataChecksum: for each 128-byte block */
u32 K(mc_ecc)(void *d, const void *s, u32 n, u32 x) {
  const u8 *p = s;
  u8 *e = d;
  for (; n; n -= 128, e += 3) {
    int i = 0, a2 = 0, a3 = 0, t0 = 0;
    do {
      int v = mc_xortable[*p++];
      a2 ^= v;
      if (v & 0x80) {
        a3 ^= ~i;
        t0 ^= i;
      }
    } while (++i < 0x80);
    e[0] = ~a2 & 0x77;
    e[1] = ~a3 & 0x7f;
    e[2] = ~t0 & 0x7f;
  }
  return 0;
}

/* mx4sio_bd mx4sio.c, reverse_buffer: bits of each byte */
u32 K(bit_rev)(void *d, const void *s, u32 n, u32 x) {
  u32 *b = d;
  for (u32 i = 0; i < n / 4; i++) {
    u32 v = b[i];
    v = ((v & 0xF0F0F0F0) >> 4) | ((v & 0x0F0F0F0F) << 4);
    v = ((v & 0xCCCCCCCC) >> 2) | ((v & 0x33333333) << 2);
    v = ((v & 0xAAAAAAAA) >> 1) | ((v & 0x55555555) << 1);
    b[i] = v;
  }
  return 0;
}

/* the usual table CRC-32 (GPT partition tables in bdm) */
u32 K(crc32)(void *d, const void *s, u32 n, u32 x) {
  const u8 *p = s;
  u32 c = ~0u;
  while (n--)
    c = crc_table[(c ^ *p++) & 0xff] ^ (c >> 8);
  return ~c;
}

/* vfat: the first free entry of a FAT32 table */
u32 K(fat_scan)(void *d, const void *s, u32 n, u32 x) {
  const u32 *f = s;
  u32 i = 0;
  while (f[i] != 0)
    i++;
  return i;
}

/* ---- sound ---- */

/* audsrv upsamplers.c, up_24000_16_stereo: n bytes in. Left at d, right at d + 2n / 2 */
u32 K(up_24000)(void *d, const void *s, u32 n, u32 x) {
  short *left = d;
  short *right = (short *)((u8 *)d + n);
  const short *src = s;
  for (u32 p = 0; p < n / 4; p++) {
    *left++ = src[0];
    *right++ = src[1];
    *left++ = *src++;
    *right++ = *src++;
  }
  return 0;
}

/* audsrv upsamplers.c, up_generic_16_stereo: 512 samples out for each block */
u32 K(up_gen)(void *d, const void *s, u32 n, u32 x) {
  short *left = d;
  short *right = (short *)((u8 *)d + 1024);
  const short *src = s;
  for (int p = 0; p < 512; p++) {
    int q = up_lut[p];
    *left++ = src[q * 2];
    *right++ = src[q * 2 + 1];
  }
  return 0;
}

/* a software mixer: d += s * volume, saturated; x is the volume */
u32 K(mix_vol)(void *d, const void *s, u32 n, u32 x) {
  short *p = d;
  const short *q = s;
  int vol = x;
  for (n /= 2; n; n--) {
    int v = *p + ((*q++ * vol) >> 15);
    if (v > 32767)
      v = 32767;
    else if (v < -32768)
      v = -32768;
    *p++ = v;
  }
  return 0;
}

/* fixed point gain, 16.16: x is any size, the samples are 16 bits */
u32 K(gain_q16)(void *d, const void *s, u32 n, u32 x) {
  int *p = d;
  const short *q = s;
  for (n /= 2; n; n--)
    *p++ = (int)(x * (u32)*q++) >> 16;
  return 0;
}

/* audsrv audsrv.c: into a ring, x is where the write position starts */
u32 K(ring_put)(void *d, const void *s, u32 n, u32 x) {
  u8 *ring = d;
  const u8 *p = s;
  u32 pos = x;
  while (n--) {
    ring[pos] = *p++;
    if (++pos == 2048)
      pos = 0;
  }
  return pos;
}

/* ---- system ---- */

/* threadman, sysmem: down a list */
struct node {
  struct node *next;
  u32 key;
};
u32 K(list_walk)(void *d, const void *s, u32 n, u32 x) {
  const struct node *p = s;
  u32 sum = 0;
  while (p) {
    sum += p->key;
    p = p->next;
  }
  return sum;
}

/* printf's %u (sysclib): the digits of each word */
u32 K(itoa10)(void *d, const void *s, u32 n, u32 x) {
  const u32 *p = s;
  char *o = d;
  for (n /= 4; n; n--) {
    u32 v = *p++;
    do {
      *o++ = '0' + v % 10;
      v /= 10;
    } while (v);
  }
  return o - (char *)d;
}

/* unit conversion, as timers do: a constant the compiler knows */
u32 K(mul_const)(void *d, const void *s, u32 n, u32 x) {
  u32 *p = d;
  const u32 *q = s;
  for (n /= 4; n; n--)
    *p++ = *q++ * 1000;
  return 0;
}

/* 64-bit products, as for timer values */
u32 K(mul64)(void *d, const void *s, u32 n, u32 x) {
  u64 *p = d;
  const u64 *q = s;
  for (n /= 16; n; n--, q += 2)
    *p++ = q[0] * q[1];
  return 0;
}
