/* Why does VIF0 DMA not start on the PS2? Each test sends UNPACK of one
   marker quadword to VU0 data memory through DMA channel 0 (VIF0) and
   checks VU0 memory through its mapping at 0x11004000. */
#include <dma.h>
#include <kernel.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define D0 ((volatile uint32_t *)0x10008000) /* CHCR, MADR [4], QWC [8], TADR [12] */
#define VIF0_STAT (*(volatile uint32_t *)0x10003800)
#define VIF0_FBRST (*(volatile uint32_t *)0x10003810)
#define D_STAT (*(volatile uint32_t *)0x1000e010)
#define VU0_DATA ((volatile uint32_t *)0x11004000)

static uint32_t pkt[16] __attribute__((aligned(64)));
static uint64_t tags[4][2] __attribute__((aligned(64)));
static uint32_t data[4] __attribute__((aligned(64)));

static void wait_d0(void) {
  for (int i = 0; i < 10000000 && (D0[0] & 0x100); i++)
    ;
}

static int report(const char *name, int addr, uint32_t marker) {
  wait_d0();
  int ok = VU0_DATA[4 * addr] == marker;
  printf("%-34s %s: VU0[%d]=%08x  CHCR %08x MADR %08x QWC %08x TADR %08x VIF0_STAT %08x "
         "D_STAT %08x\n",
         name, ok ? "OK  " : "FAIL", addr, VU0_DATA[4 * addr], D0[0], D0[4], D0[8], D0[12],
         VIF0_STAT, D_STAT);
  return ok;
}

/* NOP, STCYCL 1 1, NOP, UNPACK V4-32 1 quadword to addr, then the data */
static int normal_packet(int addr, uint32_t marker) {
  pkt[0] = 0;
  pkt[1] = 0x01000101;
  pkt[2] = 0;
  pkt[3] = 0x6c010000 | addr;
  for (int i = 0; i < 4; i++)
    pkt[4 + i] = marker;
  FlushCache(0);
  return 2; /* quadwords */
}

int main(void) {
  int ok = 0;
  printf("vif0probe\n");

  /* 1. ps2sdk normal mode, before dma_channel_initialize */
  int qwc = normal_packet(1, 0x11111111);
  dma_channel_send_normal(DMA_CHANNEL_VIF0, pkt, qwc, 0, 0);
  ok += report("1 normal, sdk, no init", 1, 0x11111111);

  /* 2. after dma_channel_initialize */
  dma_channel_initialize(DMA_CHANNEL_VIF0, NULL, 0);
  qwc = normal_packet(2, 0x22222222);
  dma_channel_send_normal(DMA_CHANNEL_VIF0, pkt, qwc, 0, 0);
  ok += report("2 normal, sdk, after init", 2, 0x22222222);

  /* 3. chain mode with tag transfer (as in vudemo): REF to the data with
        STCYCL, UNPACK in the tag; END */
  for (int i = 0; i < 4; i++)
    data[i] = 0x33333333;
  tags[0][0] = 1 | 3ull << 28 | (uint64_t)((uintptr_t)data & 0x0fffffff) << 32;
  tags[0][1] = 0x01000101 | (uint64_t)(0x6c010000 | 3) << 32;
  tags[1][0] = 7ull << 28;
  tags[1][1] = 0;
  FlushCache(0);
  dma_channel_send_chain(DMA_CHANNEL_VIF0, tags, 0, DMA_FLAG_TRANSFERTAG, 0);
  ok += report("3 chain, TTE, sdk", 3, 0x33333333);

  /* 4. chain mode without tag transfer: CNT with the VIF codes and the data
        in the following quadwords; END */
  tags[0][0] = 2 | 1ull << 28;
  tags[0][1] = 0;
  ((uint32_t *)tags[1])[0] = 0;
  ((uint32_t *)tags[1])[1] = 0x01000101;
  ((uint32_t *)tags[1])[2] = 0;
  ((uint32_t *)tags[1])[3] = 0x6c010000 | 4;
  for (int i = 0; i < 4; i++)
    ((uint32_t *)tags[2])[i] = 0x44444444;
  tags[3][0] = 7ull << 28;
  tags[3][1] = 0;
  FlushCache(0);
  dma_channel_send_chain(DMA_CHANNEL_VIF0, tags, 0, 0, 0);
  ok += report("4 chain, no TTE, sdk", 4, 0x44444444);

  /* 5. normal mode, registers written directly */
  qwc = normal_packet(5, 0x55555555);
  D0[4] = (uintptr_t)pkt & 0x0fffffff;
  D0[8] = qwc;
  D0[0] = 0x101; /* STR, from memory, normal */
  ok += report("5 normal, direct", 5, 0x55555555);

  /* 6. as 1, after a VIF0 reset (vudemo reset VIF0 first) */
  VIF0_FBRST = 1;
  qwc = normal_packet(6, 0x66666666);
  dma_channel_send_normal(DMA_CHANNEL_VIF0, pkt, qwc, 0, 0);
  ok += report("6 normal, sdk, after VIF0 reset", 6, 0x66666666);

  /* The demo's setup, step by step (cumulative), each followed by a normal
     send to VIF0. The first failure is the step that stops VIF0 DMA. */
  int addr = 7;
  uint32_t m = 0x77777777;
#define SEND_AFTER(what)                                                       \
  do {                                                                         \
    qwc = normal_packet(addr, m);                                              \
    dma_channel_send_normal(DMA_CHANNEL_VIF0, pkt, qwc, 0, 0);                 \
    ok += report(what, addr, m);                                               \
    addr++;                                                                    \
    m += 0x01010101;                                                           \
  } while (0)

  dma_channel_fast_waits(DMA_CHANNEL_VIF0);
  SEND_AFTER("7 after fast_waits(VIF0)");
  dma_channel_initialize(DMA_CHANNEL_VIF1, NULL, 0);
  dma_channel_fast_waits(DMA_CHANNEL_VIF1);
  SEND_AFTER("8 after VIF1 init + fast_waits");
  __asm__ volatile("li $8, 0x202\n\t.word 0x48c8e000" ::: "$8", "memory"); /* FBRST */
  SEND_AFTER("9 after VU0/VU1 reset (FBRST)");
  for (int i = 0; i < 48; i++)
    __asm__ volatile("sq $0, 0(%0)" : : "r"((char *)0x11000000 + 16 * i) : "memory");
  __asm__ volatile("sync" ::: "memory");
  SEND_AFTER("10 after VU0 micro memory writes");
  *(volatile uint32_t *)0x10003c10 = 1; /* VIF1 reset */
  SEND_AFTER("11 after VIF1 reset");

  printf("vif0probe: %d of %d delivered\n", ok, addr - 1);
  printf("@@PS2TEST_END@@ status=0\n");
  return 0;
}
