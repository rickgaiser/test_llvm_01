/* VU micro-mode microbenchmarks (gen.py): times each probe program with the
   body unrolled K1 and K2 times and prints the cycles per body next to the
   VU target's model. Run on the PS2 only; PCSX2 timings mean nothing. */

#include <mx/mx.h>

struct probe {
  const char *name;
  int vu;
  const unsigned char *code, *end;
  u32 k2;    /* entry of the K2 program, 64-bit units (K1 is at 0) */
  int model; /* cycles per body x 100, -1: value check */
  int show;  /* quadwords from data address 1 to print */
  const char *note;
};

#include "probes.h"

#define RUNS 9

static u32 median(u32 *v, int n) {
  for (int i = 1; i < n; i++)
    for (int j = i; j > 0 && v[j] < v[j - 1]; j--) {
      u32 t = v[j];
      v[j] = v[j - 1];
      v[j - 1] = t;
    }
  return v[n / 2];
}

static struct mx_vuq q1;
static u32 hdr[4] MX_ALIGNED(16);
static u32 item[4] MX_ALIGNED(16);

/* EE cycles from the start of the program to the end of the wait */
static u32 run(const struct probe *p, u32 entry) {
  if (p->vu == 0) {
    u32 t0 = mx_count();
    mx_vu0_call(entry);
    mx_vu0_wait();
    return mx_count() - t0;
  }
  /* the K1 and the K2 program as two kernels: a kernel's code is uploaded
     at its entry */
  const unsigned char *code = p->code + 8 * entry;
  u32 bytes = entry ? (u32)(p->end - code) : 8 * p->k2;
  struct mx_kernel k = {.vu = 1,
                        .code = code,
                        .code_bytes = bytes,
                        .entry = entry,
                        .hdr_qw = 1,
                        .count_qw = 0,
                        .data_qw = 64,
                        .item_qw = 1};
  u32 t0 = mx_count();
  mx_event e = mx_vuq_batch(&q1, &k, hdr, item, 1);
  if (mx_vuq_wait(&q1, e) < 0)
    mx_printf("%s: VU1 did not end\n", p->name);
  return mx_count() - t0;
}

int main(void) {
  mx_vuq_init(&q1, 1, 1);
  int n = (int)(sizeof probes / sizeof probes[0]);
  mx_printf("VU micro probes: R %d, K %d/%d, median of %d (cycles per body)\n", R, K1, K2, RUNS);
  for (int i = 0; i < n; i++) {
    const struct probe *p = &probes[i];
    if (p->vu == 0 && mx_vu_load(0, p->code, (u32)(p->end - p->code), 0) < 0) {
      mx_printf("%s: upload failed\n", p->name);
      continue;
    }
    u32 a[RUNS], b[RUNS];
    for (int r = 0; r < RUNS; r++) {
      a[r] = run(p, 0);
      b[r] = run(p, p->k2);
    }
    u32 t1 = median(a, RUNS), t2 = median(b, RUNS);
    int per = (int)(((s32)(t2 - t1)) * 100 / (R * (K2 - K1)));
    char model[16];
    if (p->model < 0)
      mx_snprintf(model, sizeof model, "-");
    else
      mx_snprintf(model, sizeof model, "%d.%02d", p->model / 100, p->model % 100);
    mx_printf("vu%d %-12s %4d.%02d model %6s  %s\n", p->vu, p->name, per / 100,
              (per < 0 ? -per : per) % 100, model, p->note);
    for (int s = 0; s < p->show; s++) {
      u32 v[4] MX_ALIGNED(16);
      if (p->vu == 0)
        mx_vu0_read(v, 1 + s, 1);
      else
        mx_vu1_read(v, 1 + s, 1);
      mx_printf("Check: vu%d %s qw%d = %08x %08x %08x %08x\n", p->vu, p->name, 1 + s, v[0],
                v[1], v[2], v[3]);
    }
  }
  mx_vuq_release(&q1);
  mx_printf("@@PS2TEST_END@@ status=0\n");
  return 0;
}
