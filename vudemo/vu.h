/* Minimal VU control from the EE, without DMA or VIF: the VU memories are
   mapped into the EE address space, VU0 starts with VCALLMS and VU1 with a
   write to CMSAR1, and VPU-STAT tells when they stop (VU Users Manual). */
#ifndef VU_H
#define VU_H

#include <stdint.h>

#define VU0_MICRO ((volatile uint32_t *)0x11000000)
#define VU0_DATA  ((volatile uint32_t *)0x11004000)
#define VU1_MICRO ((volatile uint32_t *)0x11008000)
#define VU1_DATA  ((volatile uint32_t *)0x1100C000)

/* VPU-STAT bits */
#define VPU_STAT_VBS0 (1u << 0) /* VU0 busy */
#define VPU_STAT_VBS1 (1u << 8) /* VU1 busy */

/* Encodings from the VU Users Manual, emitted as words so that they do not
   depend on assembler support. $8 (t0) carries the GPR operand.
     vcallms imm15:  0x4a000038 | imm15 << 6        (imm15 = address / 8)
     ctc2 $8, $31:   0x48c8f800  (CMSAR1 = vi31)
     cfc2 $8, $29:   0x4848e800  (VPU-STAT = vi29) */

static inline void vu0_start(void) {
  __asm__ volatile(".word 0x4a000038" ::: "memory"); /* vcallms 0 */
}

static inline void vu1_start(void) {
  __asm__ volatile("move $8, $0\n\t.word 0x48c8f800" ::: "$8", "memory");
}

static inline uint32_t vpu_stat(void) {
  uint32_t s;
  __asm__ volatile(".word 0x4848e800\n\tmove %0, $8" : "=r"(s) : : "$8");
  return s;
}

static inline void vu_wait(uint32_t busy_bits) {
  while (vpu_stat() & busy_bits)
    ;
}

/* Copies whole quadwords; VU memory is written while the VU is stopped. */
static inline void vu_copy_to(volatile uint32_t *dst, const void *src, int quads) {
  const uint32_t *s = (const uint32_t *)src;
  for (int i = 0; i < 4 * quads; i++)
    dst[i] = s[i];
}

static inline void vu_copy_from(void *dst, const volatile uint32_t *src, int quads) {
  uint32_t *d = (uint32_t *)dst;
  for (int i = 0; i < 4 * quads; i++)
    d[i] = src[i];
}

static inline uint32_t ee_cycles(void) {
  uint32_t c;
  __asm__ volatile("mfc0 %0, $9" : "=r"(c));
  return c;
}

#endif
