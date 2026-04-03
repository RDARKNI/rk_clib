// Benchmarks for rk_dict.
//
// Compile with optimisations, e.g.:
//   cc -O2 -std=c23 -o bench_dict bench_rk_dict.c && ./bench_dict
//
// Each benchmark reports:
//   - throughput (Mops/s)
//   - time per operation (ns)
//
// Methodology:
//   - Operations performed on a pre-filled table to measure steady-state cost.
//   - Keys are drawn from a fixed pseudo-random sequence (xorshift32) so
//     results are reproducible.
//   - A volatile sink prevents the compiler from eliminating dead reads.
//   - Each benchmark is run BENCH_REPS times; the minimum time is reported
//     (min reflects best-case CPU state, less sensitive to OS jitter).

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define RK_IMPL
#define RK_ALLOCMODE RK_ALLOCMODE_MALLOC_ONLY
#include "../include/rklib_includeall.h"
RK_HEADER_BEGIN

// ---- dict instantiation -------------------------------------------------- //

typedef unsigned int uint;
static unsigned int  u32_hash(unsigned int k) { return k * 2654435761u; }
static int           u32_cmp(unsigned int a, unsigned int b) { return a != b; }
DICT_DEFINE(uint, uint, u32_hash, u32_cmp)

// ---- timing -------------------------------------------------------------- //

static long long bench_now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

// ---- rng ----------------------------------------------------------------- //

static unsigned int xorshift32(unsigned int* state) {
  unsigned int x  = *state;
  x              ^= x << 13;
  x              ^= x >> 17;
  x              ^= x << 5;
  return *state   = x;
}

// ---- infrastructure ------------------------------------------------------ //

#define BENCH_REPS   5         // repetitions — take the minimum
#define BENCH_WARMUP 1         // warmup reps (not measured)
#define N_SMALL      (1 << 10) //  1 K entries  — fits in L1
#define N_MEDIUM     (1 << 16) // 64 K entries  — fits in L2/L3
#define N_LARGE      (1 << 20) //  1 M entries  — exceeds cache

static void bench_print(const char* name, long long ns, long long n) {
  double ns_per_op = (double)ns / (double)n;
  double mops      = (double)n / ((double)ns / 1e3);
  printf("  %-42s  %7.2f ns/op   %8.2f Mops/s\n", name, ns_per_op, mops);
}

// Run fn(dict, keys, n) BENCH_REPS+BENCH_WARMUP times, report minimum.
#define RUN_BENCH(label, setup, body, teardown, n_ops)                         \
  do {                                                                         \
    long long rk__best = LLONG_MAX;                                            \
    for (int rk__r = 0; rk__r < (BENCH_REPS) + (BENCH_WARMUP); ++rk__r) {      \
      setup;                                                                   \
      long long rk__t0 = bench_now_ns();                                       \
      body;                                                                    \
      long long rk__t1 = bench_now_ns();                                       \
      teardown;                                                                \
      if (rk__r >= (BENCH_WARMUP)) {                                           \
        long long rk__dt = rk__t1 - rk__t0;                                    \
        if (rk__dt < rk__best) rk__best = rk__dt;                              \
      }                                                                        \
    }                                                                          \
    bench_print((label), rk__best, (n_ops));                                   \
  } while (0)

// ---- individual benchmarks ----------------------------------------------- //

// Fills a dict with n sequential keys starting from 1.
static void fill_seq(Dict(uint, uint)* d, size_t n) {
  for (unsigned int i = 1; i <= (unsigned int)n; ++i) {
    dict_set(uint, uint, d, i, i);
  }
}

// Fills a dict with n pseudo-random keys.
static void fill_rng(Dict(uint, uint)* d, size_t n, unsigned int seed) {
  unsigned int state = seed;
  for (size_t i = 0; i < n; ++i) {
    unsigned int k = xorshift32(&state);
    dict_set(uint, uint, d, k, k);
  }
}

static void bench_insert_fresh(size_t n, const char* tag) {
  char label[64];
  snprintf(label, sizeof(label), "insert (fresh, %s)", tag);
  RUN_BENCH(
      label,
      /* setup */
      Dict(uint, uint) d = dict_init(uint, uint, n);
      ,
      /* body */
      do {
        unsigned int state = 0xdeadbeef;
        for (size_t i = 0; i < n; ++i) {
          unsigned int k = xorshift32(&state);
          dict_set(uint, uint, &d, k, k);
        }
      } while (0),
      /* teardown */
      dict_release(uint, uint, &d), (long long)n);
}

static void bench_lookup_hit(size_t n, const char* tag) {
  char label[64];
  snprintf(label, sizeof(label), "get (hit, %s)", tag);

  // Pre-generate keys so the lookup sequence matches what was inserted.
  unsigned int* keys  = malloc(n * sizeof(unsigned int));
  unsigned int  state = 0xdeadbeef;
  for (size_t i = 0; i < n; ++i) { keys[i] = xorshift32(&state); }

  Dict(uint, uint) d = dict_init(uint, uint, n);
  for (size_t i = 0; i < n; ++i) { dict_set(uint, uint, &d, keys[i], keys[i]); }

  volatile unsigned int sink = 0;
  RUN_BENCH(
      label,
      /* setup */ (void)0,
      /* body */
      do {
        for (size_t i = 0; i < n; ++i) {
          unsigned int* v = dict_get(uint, uint, &d, keys[i]);
          if (v) { sink = *v; }
        }
      } while (0),
      /* teardown */ (void)0, (long long)n);

  (void)sink;
  dict_release(uint, uint, &d);
  free(keys);
}

static void bench_lookup_miss(size_t n, const char* tag) {
  char label[64];
  snprintf(label, sizeof(label), "get (miss, %s)", tag);

  // Insert even keys, look up odd keys — guaranteed misses.
  Dict(uint, uint) d = dict_init(uint, uint, n);
  for (unsigned int i = 0; i < (unsigned int)n * 2; i += 2) {
    dict_set(uint, uint, &d, i, i);
  }

  volatile unsigned int sink = 0;
  RUN_BENCH(
      label,
      /* setup */ (void)0,
      /* body */
      do {
        for (unsigned int i = 1; i < (unsigned int)n * 2; i += 2) {
          unsigned int* v  = dict_get(uint, uint, &d, i);
          sink            += (v != NULL);
        }
      } while (0),
      /* teardown */ (void)0, (long long)n);

  (void)sink;
  dict_release(uint, uint, &d);
}

static void bench_remove_reinsert(size_t n, const char* tag) {
  // Measures tombstone pressure: repeatedly remove and re-insert the same
  // n/2 keys while keeping the other n/2 live.
  char label[64];
  snprintf(label, sizeof(label), "remove+reinsert (tombstone stress, %s)", tag);

  unsigned int* keys  = malloc(n * sizeof(unsigned int));
  unsigned int  state = 0xdeadbeef;
  for (size_t i = 0; i < n; ++i) { keys[i] = xorshift32(&state); }

  size_t half = n / 2;

  RUN_BENCH(
      label,
      /* setup */
      do {
        // Rebuild the dict each rep so tombstone count resets.
      } while (0),
      /* body */
      do {
        Dict(uint, uint) d = dict_init(uint, uint, n);
        for (size_t i = 0; i < n; ++i) {
          dict_set(uint, uint, &d, keys[i], keys[i]);
        }
        for (size_t i = 0; i < half; ++i) {
          dict_remove(uint, uint, &d, keys[i]);
          dict_set(uint, uint, &d, keys[i], keys[i]);
        }
        dict_release(uint, uint, &d);
      } while (0),
      /* teardown */ (void)0, (long long)n);

  free(keys);
}

static void bench_foreach(size_t n, const char* tag) {
  char label[64];
  snprintf(label, sizeof(label), "foreach (full scan, %s)", tag);

  Dict(uint, uint) d = dict_init(uint, uint, n);
  fill_rng(&d, n, 0xdeadbeef);

  volatile unsigned int sink = 0;
  RUN_BENCH(
      label,
      /* setup */ (void)0,
      /* body */
      do {
        dict_foreach(&d, k, v) { sink += *k + *v; }
      } while (0),
      /* teardown */ (void)0, (long long)dict_count(&d));

  (void)sink;
  dict_release(uint, uint, &d);
}

// ---- main ---------------------------------------------------------------- //

RK_HEADER_END

int main(void) {
  printf("rk_dict benchmarks\n");
  printf("  BENCH_REPS=%d  WARMUP=%d\n\n", BENCH_REPS, BENCH_WARMUP);

  struct {
    size_t      n;
    const char* tag;
  } sizes[] = {
      {N_SMALL, "1K,  L1"},
      {N_MEDIUM, "64K, L2/3"},
      {N_LARGE, "1M,  RAM"},
  };

  for (size_t s = 0; s < sizeof(sizes) / sizeof(sizes[0]); ++s) {
    size_t      n   = sizes[s].n;
    const char* tag = sizes[s].tag;
    printf("--- n=%zu (%s) ---\n", n, tag);
    bench_insert_fresh(n, tag);
    bench_lookup_hit(n, tag);
    bench_lookup_miss(n, tag);
    bench_remove_reinsert(n, tag);
    bench_foreach(n, tag);
    printf("\n");
  }
}
