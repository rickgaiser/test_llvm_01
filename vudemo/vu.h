/* VU control from the EE, the way ps2gl and ps2stuff do it:
   - VU1 is fed by one VIF1 DMA packet per batch (UNPACK, MSCAL, FLUSH); its
     microcode is sent once with MPG.
   - VU0 data goes through COP2 (LQC2 + VSQI in, VLQI + SQC2 out, which VU0
     interlocks), and VU0 starts with VCALLMS; CFC2.I waits for its end.
   Only the reading back of VU1's results is not in those libraries: it uses
   the VU1 memory mapped at 0x1100c000, after VU1 has stopped.

   COP2 instructions the assembler does not know are emitted as words, with
   $8 (t0) for a GPR operand (encodings from the VU Users Manual):
     vcallms 0:              0x4a000038
     cfc2    $8, $vi29:      0x4848e800   (VPU-STAT)
     cfc2.i  $8, $vi29:      0x4848e801   (waits for the end of VU0's program)
     ctc2    $8, $vi28:      0x48c8e000   (FBRST)
     ctc2    $8, $vi2 / vi3: 0x48c81000 / 0x48c81800
     vsqi.xyzw vf2, (vi3++): 0x4be3137d
     vlqi.xyzw vf1, (vi2++): 0x4be1137c
     lqc2 $vf2, 0($9):       0xd9220000
     sqc2 $vf1, 0($9):       0xf9210000
   (demo.c is built without -mvu0, so the assembler rejects LQC2/SQC2.) */
#ifndef VU_H
#define VU_H

#include <dma.h>
#include <kernel.h>
#include <stdint.h>

#define VU1_DATA ((const volatile uint32_t *)0x1100c000)
#define VIF1_STAT (*(volatile uint32_t *)0x10003c00)
#define VIF1_FBRST (*(volatile uint32_t *)0x10003c10)

/* VPU-STAT bits */
#define VPU_STAT_VBS0 (1u << 0) /* VU0 busy */
#define VPU_STAT_VBS1 (1u << 8) /* VU1 busy */

static inline uint32_t vpu_stat(void) {
  uint32_t s;
  __asm__ volatile(".word 0x4848e800\n\tmove %0, $8" : "=r"(s) : : "$8");
  return s;
}

static inline uint32_t ee_cycles(void) {
  uint32_t c;
  __asm__ volatile("mfc0 %0, $9" : "=r"(c));
  return c;
}

static inline uint32_t cop0_status(void) {
  uint32_t s;
  __asm__ volatile("mfc0 %0, $12" : "=r"(s));
  return s;
}

/* The EE counter counts CPU cycles (294.912 MHz). */
static inline void delay_ms(unsigned ms) {
  for (unsigned i = 0; i < ms; i++) {
    uint32_t t = ee_cycles();
    while (ee_cycles() - t < 294912)
      ;
  }
}

/* Resets VU0, VU1 (FBRST: RS0 bit 1, RS1 bit 9) and VIF1, and sets up the
   VIF1 DMA channel. */
static inline void vu_init(void) {
  __asm__ volatile("li $8, 0x202\n\t.word 0x48c8e000" ::: "$8", "memory");
  VIF1_FBRST = 1;
  dma_channel_initialize(DMA_CHANNEL_VIF1, NULL, 0);
  dma_channel_fast_waits(DMA_CHANNEL_VIF1);
}

/* ---- VU0 ---- */

/* Copies quadwords to VU0 data memory at quadword address addr. */
static inline void vu0_write(int addr, const void *src, int quads) {
  __asm__ volatile("move $8, %0\n\t.word 0x48c81800" /* ctc2 $8, $vi3 */
                   : : "r"(addr) : "$8");
  for (int i = 0; i < quads; i++)
    __asm__ volatile("move $9, %0\n\t.word 0xd9220000\n\t" /* lqc2 $vf2, 0($9) */
                     ".word 0x4be3137d" /* vsqi */
                     : : "r"((const char *)src + 16 * i) : "$9", "memory");
}

/* Copies quadwords from VU0 data memory at quadword address addr. */
static inline void vu0_read(void *dst, int addr, int quads) {
  __asm__ volatile("move $8, %0\n\t.word 0x48c81000" /* ctc2 $8, $vi2 */
                   : : "r"(addr) : "$8");
  for (int i = 0; i < quads; i++)
    __asm__ volatile("move $9, %0\n\t.word 0x4be1137c\n\t" /* vlqi */
                     ".word 0xf9210000" /* sqc2 $vf1, 0($9) */
                     : : "r"((char *)dst + 16 * i) : "$9", "memory");
}

static inline void vu0_start(void) {
  __asm__ volatile(".word 0x4a000038" ::: "memory"); /* vcallms 0 */
}

/* Waits for the end of VU0's micro program (CFC2.I interlocks). */
static inline void vu0_wait(void) {
  __asm__ volatile(".word 0x4848e801" ::: "$8", "memory");
}

/* Uploads VU0 microcode: VU0 micro memory is written through its mapping
   at 0x11000000 while VU0 is stopped, with 128-bit stores, then SYNC. */
static inline void vu0_load_micro(const void *code, int quads) {
  for (int i = 0; i < quads; i++)
    __asm__ volatile("lq $8, 0(%1)\n\tsq $8, 0(%0)"
                     : : "r"((char *)0x11000000 + 16 * i),
                         "r"((const char *)code + 16 * i)
                     : "$8", "memory");
  __asm__ volatile("sync" ::: "memory");
}

/* ---- VU1, through VIF1 ---- */

/* VIF codes */
#define VIF_NOP 0x00000000u
#define VIF_STCYCL(wl, cl) (0x01000000u | (wl) << 8 | (cl))
#define VIF_MSCAL(addr8) (0x14000000u | (addr8))
/* FLUSHE: wait for the end of the micro program (FLUSH also waits for GIF
   PATH1/2 transfers, which ps2gl needs for XGKICK; here VIF1 then stalls) */
#define VIF_FLUSHE 0x10000000u
#define VIF_MPG(num64, addr8) (0x4a000000u | ((num64) & 0xff) << 16 | (addr8))
#define VIF_UNPACK_V4_32(num, addr) (0x6c000000u | ((num) & 0xff) << 16 | (addr))

/* A DMA packet for VIF1, built in 32-bit words (16-byte aligned buffer). */
struct vif_packet {
  uint32_t *w;
  int n;
};

static inline void vif_put(struct vif_packet *p, uint32_t word) { p->w[p->n++] = word; }

/* Pads with NOPs until the next word is at word index 4k + at. */
static inline void vif_align(struct vif_packet *p, int at) {
  while ((p->n & 3) != at)
    vif_put(p, VIF_NOP);
}

/* UNPACK V4-32 of quads quadwords to VU1 data memory at addr. */
static inline void vif_unpack(struct vif_packet *p, int addr, const void *src, int quads) {
  const uint32_t *s = (const uint32_t *)src;
  while (quads > 0) {
    int n = quads > 256 ? 256 : quads;
    vif_align(p, 3); /* the data then starts on a quadword */
    vif_put(p, VIF_UNPACK_V4_32(n, addr));
    for (int i = 0; i < 4 * n; i++)
      vif_put(p, *s++);
    addr += n;
    quads -= n;
  }
}

/* Starts the DMA of the packet to VIF1. */
static inline void vif_send(struct vif_packet *p) {
  vif_align(p, 0);
  FlushCache(0); /* write the packet back from the data cache */
  dma_channel_send_normal(DMA_CHANNEL_VIF1, p->w, p->n / 4, 0, 0);
}

/* Waits until VIF1 has done all of the packet (DMA done, VIF1 FIFO empty and
   idle) and VU1 has stopped. Returns 1 on a timeout. */
static inline int vif_wait(void) {
  if (dma_channel_wait(DMA_CHANNEL_VIF1, 1000000) < 0)
    return 1;
  uint32_t t = ee_cycles();
  /* FQC (bits 24-28) and VPS (bits 0-1) of VIF1_STAT */
  while ((VIF1_STAT & 0x1f000003u) || (vpu_stat() & VPU_STAT_VBS1))
    if (ee_cycles() - t > 294912000u)
      return 1;
  return 0;
}

/* Uploads VU1 microcode with MPG (at most 256 doublewords per MPG). */
static inline int vu1_load_micro(uint32_t *buf, const void *code, int bytes) {
  struct vif_packet p = {buf, 0};
  const uint32_t *s = (const uint32_t *)code;
  int dwords = (bytes + 7) / 8;
  for (int at = 0; at < dwords; at += 256) {
    int n = dwords - at > 256 ? 256 : dwords - at;
    vif_align(&p, 1); /* the code then starts on a doubleword */
    vif_put(&p, VIF_MPG(n, at));
    for (int i = 0; i < 2 * n; i++)
      vif_put(&p, *s++);
  }
  vif_send(&p);
  return vif_wait();
}

/* Copies quadwords from VU1 data memory (mapped; VU1 must be stopped). */
static inline void vu1_read(void *dst, int addr, int quads) {
  for (int i = 0; i < quads; i++)
    __asm__ volatile("lq $8, 0(%1)\n\tsq $8, 0(%0)"
                     : : "r"((char *)dst + 16 * i),
                         "r"((const volatile char *)VU1_DATA + 16 * (addr + i))
                     : "$8", "memory");
}

#endif
