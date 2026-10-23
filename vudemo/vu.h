/* VU control from the EE, as in ps2gl and ps2stuff:
   - VU1: the input of a batch goes by DMA through VIF1, from where it is in
     memory (REF tags), with UNPACK; MSCAL starts the micro program and
     FLUSHE waits for its end. The microcode is sent once with MPG. The
     results come back through the VU1 memory mapped at 0x1100c000, after
     VU1 has stopped (nothing else moves VU1 memory to main memory).
   - VU0: data through COP2 (LQC2 + VSQI in, VLQI + SQC2 out, which VU0
     interlocks), VCALLMS starts it and CFC2.I waits for its end.
     (VIF0 DMA, like VIF1, did not start on the PS2: D0_CHCR.STR stayed 0
     and TADR did not move, also when written directly; PCSX2 runs it.)

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
#define VIF0_STAT (*(volatile uint32_t *)0x10003800)
#define VIF0_FBRST (*(volatile uint32_t *)0x10003810)
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

/* Resets VU0, VU1 (FBRST: RS0 bit 1, RS1 bit 9), VIF0 and VIF1, and sets up
   their DMA channels. */
static inline void vu_init(void) {
  __asm__ volatile("li $8, 0x202\n\t.word 0x48c8e000" ::: "$8", "memory");
  VIF0_FBRST = 1;
  VIF1_FBRST = 1;
  dma_channel_initialize(DMA_CHANNEL_VIF0, NULL, 0);
  dma_channel_initialize(DMA_CHANNEL_VIF1, NULL, 0);
  dma_channel_fast_waits(DMA_CHANNEL_VIF0);
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

static inline void vu0_start(void) {
  __asm__ volatile(".word 0x4a000038" ::: "memory"); /* vcallms 0 */
}

/* Waits for the end of VU0's micro program (CFC2.I interlocks). */
static inline void vu0_wait(void) {
  __asm__ volatile(".word 0x4848e801" ::: "$8", "memory");
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

/* A DMA chain for VIF1: the data is sent from where it is (REF tags), and
   the VIF codes travel in the upper half of each tag (tag transfer). */
struct vif_chain {
  uint64_t (*q)[2]; /* 16-byte aligned */
  int n;
};

#define DMA_TAG_REF 3
#define DMA_TAG_END 7

static inline void dma_tag(struct vif_chain *c, int id, int qwc, const void *addr,
                           uint32_t vif0, uint32_t vif1) {
  c->q[c->n][0] = (uint64_t)qwc | (uint64_t)id << 28 |
                  (uint64_t)((uintptr_t)addr & 0x0fffffff) << 32;
  c->q[c->n][1] = vif0 | (uint64_t)vif1 << 32;
  c->n++;
}

/* Aliases of main memory: uncached, and uncached accelerated (stores are
   combined in a buffer; for writing results nobody reads back soon). */
#define UNCACHED(p) ((__typeof__(p))((uintptr_t)(p) | 0x20000000))
#define UCAB(p) ((__typeof__(p))((uintptr_t)(p) | 0x30000000))

/* UNPACK V4-32 of quads quadwords at src to VU data memory at addr. The
   data must be in memory, not only in the data cache. */
static inline void chain_unpack(struct vif_chain *c, int addr, const void *src,
                                int quads, uint32_t first_vif) {
  while (quads > 0) {
    int n = quads > 256 ? 256 : quads;
    dma_tag(c, DMA_TAG_REF, n, src, first_vif, VIF_UNPACK_V4_32(n, addr));
    first_vif = VIF_NOP;
    src = (const char *)src + 16 * n;
    addr += n;
    quads -= n;
  }
}

/* Ends the chain with the VIF codes vif0, vif1 and starts its DMA on the
   VIF channel (DMA_CHANNEL_VIF0 or DMA_CHANNEL_VIF1). The tags are written
   uncached (c->q is an UNCACHED address); SYNC makes sure they are in
   memory before the DMA reads them. */
static inline void chain_send(int channel, struct vif_chain *c, uint32_t vif0,
                              uint32_t vif1) {
  dma_tag(c, DMA_TAG_END, 0, 0, vif0, vif1);
  __asm__ volatile("sync" ::: "memory");
  /* the DMA controller takes a physical address (the uncached alias crashed
     the PS2, PCSX2 masks it) */
  dma_channel_send_chain(channel, (void *)((uintptr_t)c->q & 0x0fffffff), 0,
                         DMA_FLAG_TRANSFERTAG, 0);
}

/* Starts the DMA of the packet to VIF1. */
static inline void vif_send(struct vif_packet *p) {
  vif_align(p, 0);
  FlushCache(0); /* write the packet back from the data cache */
  dma_channel_send_normal(DMA_CHANNEL_VIF1, p->w, p->n / 4, 0, 0);
}

/* Waits until VIFn has done all it was sent (DMA done, FIFO empty, idle)
   and VUn has stopped. Returns 1 on a timeout. */
static inline int vif_wait(int vu) {
  if (dma_channel_wait(vu ? DMA_CHANNEL_VIF1 : DMA_CHANNEL_VIF0, 1000000) < 0)
    return 1;
  volatile uint32_t *stat = vu ? &VIF1_STAT : &VIF0_STAT;
  uint32_t busy = vu ? VPU_STAT_VBS1 : VPU_STAT_VBS0;
  uint32_t t = ee_cycles();
  /* FQC (bits 24-28) and VPS (bits 0-1) of VIFn_STAT */
  while ((*stat & 0x1f000003u) || (vpu_stat() & busy))
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
  return vif_wait(1);
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
