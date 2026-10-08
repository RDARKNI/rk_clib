#ifndef TESTARENA_H
#define TESTARENA_H
#include "conf.h"

RKI_HEADER_BEGIN
RKI_IGNWARN_CLANG_BEG("-Wunused-variable")
#define ARENA_SIZE 1024

static unsigned char arena_buf[ARENA_SIZE];
static Arena         glob_a = arena_init_static(arena_buf);
static void          fill_ints(int* p, int n, int base) {
  for (int i = 0; i < n; ++i) { p[i] = base + i; }
}

static void expect_ints(const int* p, int n, int base) {
  for (int i = 0; i < n; ++i) { triax_expect_eq(p[i], base + i); }
}

// ---- init ----

triax_test(arena, init_runtime) {
  unsigned char buf[73];
  Arena         x = arena_init(buf, sizeof(buf));

  triax_expect_eq(x.beg, buf);
  triax_expect_eq(x.cur, buf);
  triax_expect_eq(x.end, buf + sizeof(buf));

  triax_expect_eq(arena_cap(&x), sizeof(buf));
  triax_expect_eq(arena_used(&x), 0u);
  triax_expect_eq(arena_remaining(&x), sizeof(buf));
  triax_expect_true(arena_is_empty(&x));
}

triax_test(arena, init_runtime_null) {
  Arena x = arena_init(rk_null, 123);

  triax_expect_eq(x.beg, rk_null);
  triax_expect_eq(x.cur, rk_null);
  triax_expect_eq(x.end, rk_null);
}

triax_test(arena, init_static_basic) {
  unsigned char buf[64];
  Arena         x = arena_init_static(buf);

  triax_expect_eq(x.beg, buf);
  triax_expect_eq(x.cur, buf);
  triax_expect_eq(x.end, buf + sizeof(buf));

  triax_expect_eq(arena_cap(&x), sizeof(buf));
  triax_expect_eq(arena_used(&x), 0u);
  triax_expect_eq(arena_remaining(&x), sizeof(buf));
  triax_expect_true(arena_is_empty(&x));
}

// ---- cap / used / remaining ----

triax_test(arena, cap_used_remaining_empty) {
  arena_clear(&glob_a);

  triax_expect_eq(arena_cap(&glob_a), ARENA_SIZE);
  triax_expect_eq(arena_used(&glob_a), 0u);
  triax_expect_eq(arena_remaining(&glob_a), ARENA_SIZE);
  triax_expect_true(arena_is_empty(&glob_a));

  void* p1 = rki_arena_allocate(13, 1, &glob_a);
  triax_expect_nonnull(p1);
  triax_expect_false(arena_is_empty(&glob_a));
  triax_expect_eq(arena_used(&glob_a) + arena_remaining(&glob_a), arena_cap(&glob_a));

  void* p2 = rki_arena_allocate(17, 8, &glob_a);
  triax_expect_nonnull(p2);
  triax_expect_eq(arena_used(&glob_a) + arena_remaining(&glob_a), arena_cap(&glob_a));

  arena_clear(&glob_a);
  triax_expect_true(arena_is_empty(&glob_a));
  triax_expect_eq(arena_used(&glob_a), 0u);
  triax_expect_eq(arena_remaining(&glob_a), ARENA_SIZE);
}

// ---- alloc ----

triax_test(arena, alloc_basic) {
  arena_clear(&glob_a);

  size_t ints    = 10;
  size_t longs   = 100;
  size_t alloced = 0;

  int*   i       = arena_new(int, ints, &glob_a);
  triax_expect_nonnull(i);
  triax_expect_eq((uptr)i % alignof(int), 0u);
  triax_expect_true(arena_used(&glob_a) >= (alloced = ints * sizeof(*i)));

  long* l = arena_new(long, longs, &glob_a);
  triax_expect_nonnull(l);
  triax_expect_eq((uptr)l % alignof(long), 0u);
  triax_expect_true(arena_used(&glob_a) >= (alloced += longs * sizeof(*l)));

  triax_expect_eq(arena_used(&glob_a) + arena_remaining(&glob_a), arena_cap(&glob_a));
  arena_clear(&glob_a);
}

triax_test(arena, allocate_zero_size) {
  arena_clear(&glob_a);

  void* p1 = rki_arena_allocate(0, 1, &glob_a);
  void* p2 = rki_arena_allocate(0, 8, &glob_a);

  triax_expect_nonnull(p1);
  triax_expect_nonnull(p2);
  triax_expect_eq((uintptr_t)p1 % 1, 0u);
  triax_expect_eq((uintptr_t)p2 % 8, 0u);
  triax_expect_eq(arena_used(&glob_a) + arena_remaining(&glob_a), arena_cap(&glob_a));

  arena_clear(&glob_a);
}

triax_test(arena, allocate_alignment_range) {
  arena_clear(&glob_a);

  for (size_t align = 1; align <= 64; align <<= 1) {
    void* p = rki_arena_allocate(1, align, &glob_a);
    triax_expect_nonnull(p);
    triax_expect_eq((uintptr_t)p % align, 0u);
  }

  arena_clear(&glob_a);
}

triax_test(arena, allocate_full_and_fail, .isolation = TRIAX_ISOLATION_ON) {
  unsigned char buf[128];
  Arena         x = arena_init_static(buf);

  triax_expect_nonnull(rki_arena_allocate(sizeof(buf), 1, &x));
  triax_expect_eq(arena_remaining(&x), 0u);
  triax_assert_fault(TRIAX_FAULT_ANY, rki_arena_allocate(1, 1, &x););
}

triax_test(arena, new_zero_count) {
  arena_clear(&glob_a);

  int* p = arena_new(int, 0, &glob_a);
  triax_expect_nonnull(p);
  triax_expect_eq((uintptr_t)p % alignof(int), 0u);
  triax_expect_true(arena_used(&glob_a) <= arena_cap(&glob_a));

  arena_clear(&glob_a);
}

triax_test(arena, new_aligned_overaligned) {
  arena_clear(&glob_a);

  int* p = arena_new_aligned(int, 3, 32, &glob_a);
  triax_expect_nonnull(p);
  triax_expect_eq((uintptr_t)p % 32, 0u);

  double* q = arena_new_aligned(double, 2, 64, &glob_a);
  triax_expect_nonnull(q);
  triax_expect_eq((uintptr_t)q % 64, 0u);

  arena_clear(&glob_a);
}

// ---- try_new ----

triax_test(arena, try_new_returns_null_on_oom) {
  unsigned char buf[64];
  Arena         x = arena_init_static(buf);

  int*          p = arena_try_new(int, 4, &x);
  triax_expect_nonnull(p);
  triax_expect_eq((uintptr_t)p % alignof(int), 0u);

  int* q = arena_try_new(int, 10000, &x);
  triax_expect_null(q);

  arena_clear(&x);
}

triax_test(arena, try_new_aligned_returns_null_on_oom) {
  unsigned char buf[128];
  Arena         x = arena_init_static(buf);

  int*          p = arena_try_new_aligned(int, 2, 32, &x);
  triax_expect_nonnull(p);
  triax_expect_eq((uintptr_t)p % 32, 0u);

  int* q = arena_try_new_aligned(int, 10000, 32, &x);
  triax_expect_null(q);

  arena_clear(&x);
}

// ---- clear ----

triax_test(arena, clear_idempotent) {
  arena_clear(&glob_a);
  triax_expect_eq(arena_clear(&glob_a), &glob_a);
  triax_expect_eq(glob_a.cur, glob_a.beg);

  triax_expect_nonnull(rki_arena_allocate(10, 1, &glob_a));
  triax_expect_false(arena_is_empty(&glob_a));

  triax_expect_eq(arena_clear(&glob_a), &glob_a);
  triax_expect_true(arena_is_empty(&glob_a));
  triax_expect_eq(glob_a.cur, glob_a.beg);

  triax_expect_eq(arena_clear(&glob_a), &glob_a);
  triax_expect_eq(glob_a.cur, glob_a.beg);
}

// ---- mark / rewind_to ----

triax_test(arena, mark_rewind_basic) {
  arena_clear(&glob_a);

  ArenaMark m0 = arena_mark(&glob_a);
  triax_expect_nonnull(rki_arena_allocate(10, 1, &glob_a));
  ArenaMark m1 = arena_mark(&glob_a);
  triax_expect_nonnull(rki_arena_allocate(20, 1, &glob_a));

  triax_expect_eq(arena_rewind_to(&glob_a, m1), &glob_a);
  triax_expect_eq(glob_a.cur, m1.pos);

  triax_expect_eq(arena_rewind_to(&glob_a, m0), &glob_a);
  triax_expect_eq(glob_a.cur, m0.pos);
  triax_expect_true(arena_is_empty(&glob_a));
}

triax_test(arena, mark_rewind_roundtrip) {
  arena_clear(&glob_a);

  arena_new(char, ARENA_SIZE / 4, &glob_a);
  ArenaMark mark = arena_mark(&glob_a);
  arena_new(char, ARENA_SIZE / 4, &glob_a);
  unsigned char* prev = glob_a.cur;

  arena_rewind_to(&glob_a, mark);
  triax_expect_eq(glob_a.cur, mark.pos);

  arena_new(char, ARENA_SIZE / 4, &glob_a);
  triax_expect_eq(glob_a.cur, prev);

  arena_rewind_to(&glob_a, mark);
  triax_expect_eq(glob_a.cur, mark.pos);

  arena_clear(&glob_a);
  triax_expect_eq(glob_a.cur, glob_a.beg);
}

// ---- is_top_allocation ----

triax_test(arena, is_top_allocation) {
  arena_clear(&glob_a);

  int* p = arena_new(int, 4, &glob_a);
  triax_expect_true(arena_is_top_allocation(&glob_a, p, 4 * sizeof(int)));

  int* q = arena_new(int, 2, &glob_a);
  triax_expect_false(arena_is_top_allocation(&glob_a, p, 4 * sizeof(int)));
  triax_expect_true(arena_is_top_allocation(&glob_a, q, 2 * sizeof(int)));

  arena_clear(&glob_a);
}

// ---- extend / try_extend ----

triax_test(arena, extend_in_place_grow_and_shrink) {
  arena_clear(&glob_a);

  int* arr = arena_new(int, 10, &glob_a);
  triax_expect_nonnull(arr);
  fill_ints(arr, 10, 0);

  int* grown = arena_extend(arr, 10, 20, &glob_a);
  triax_expect_nonnull(grown);
  triax_expect_eq(grown, arr);
  expect_ints(grown, 10, 0);

  int* shrunk = arena_extend(grown, 20, 5, &glob_a);
  triax_expect_nonnull(shrunk);
  triax_expect_eq(shrunk, arr);
  expect_ints(shrunk, 5, 0);

  arena_clear(&glob_a);
}

// A non-top pointer is an ordinary extend failure, so arena_extend() reports it through
// RK_ARENA_FAIL (aborts by default, in release builds too).
triax_test(arena, extend_not_top_invokes_failure_handler, .isolation = TRIAX_ISOLATION_ON) {
  arena_clear(&glob_a);
  int* arr = arena_new(int, 5, &glob_a);
  fill_ints(arr, 5, 0);
  arena_new(int, 10, &glob_a); // displace top
  triax_assert_fault(TRIAX_FAULT_ABORT, (void)arena_extend(arr, 5, 10, &glob_a););
}

// arena_try_extend() on a non-top allocation fails without touching the arena or the data, for
// growth, shrinking, and resizing to zero.
triax_test(arena, try_extend_not_top_returns_null_and_leaves_arena) {
  arena_clear(&glob_a);
  int* arr = arena_new(int, 5, &glob_a);
  fill_ints(arr, 5, 0);
  int*           top  = arena_new(int, 10, &glob_a); // displace top
  unsigned char* cur0 = glob_a.cur;
  triax_expect_null(arena_try_extend(arr, 5, 10, &glob_a));
  triax_expect_null(arena_try_extend(arr, 5, 2, &glob_a));
  triax_expect_null(arena_try_extend(arr, 5, 0, &glob_a));
  triax_expect_eq(glob_a.cur, cur0);
  expect_ints(arr, 5, 0);
  triax_expect_eq(arena_try_extend(top, 10, 12, &glob_a), top); // the real top still resizes
  arena_clear(&glob_a);
}

// Byte-sized arena_try_resize(): resizes the given pointer if it is on top (growing or shrinking),
// otherwise fails without touching the arena.
triax_test(arena, try_resize_bytes_top_and_non_top) {
  unsigned char  buf[64];
  Arena          x    = arena_init(buf, sizeof buf);
  unsigned char* a    = (unsigned char*)arena_allocate(8, 1, &x);
  unsigned char* b    = (unsigned char*)arena_allocate(8, 1, &x);
  unsigned char* cur0 = x.cur;

  triax_expect_null(arena_try_resize(a, 8, 16, &x)); // not top
  triax_expect_null(arena_try_resize(a, 8, 0, &x));
  triax_expect_eq(x.cur, cur0);

  triax_expect_eq(arena_try_resize(b, 8, 20, &x), b); // grow
  triax_expect_eq(x.cur, b + 20);
  triax_expect_eq(arena_try_resize(b, 20, 4, &x), b); // shrink
  triax_expect_eq(x.cur, b + 4);
  triax_expect_null(arena_try_resize(b, 4, 1000, &x)); // does not fit
  triax_expect_eq(x.cur, b + 4);
  triax_expect_eq(arena_try_resize(b, 4, 0, &x), b); // to zero: a is top again
  triax_expect_eq(arena_try_resize(a, 8, 12, &x), a);
}

triax_test(arena, try_extend_oom_returns_null) {
  unsigned char buf[sizeof(int) * 4];
  Arena         x = arena_init_static(buf);

  int*          p = arena_new(int, 4, &x);
  triax_expect_nonnull(p);
  triax_expect_eq(arena_remaining(&x), 0u);

  int* q = arena_try_extend(p, 4, 8, &x);
  triax_expect_null(q);

  arena_clear(&x);
}

// ---- edge cases ----

triax_test(arena, edgecases_full_allocation) {
  arena_clear(&glob_a);

  triax_expect_nonnull(rki_arena_allocate(0, 1, &glob_a));

  size_t remaining = arena_remaining(&glob_a);
  triax_expect_nonnull(rki_arena_allocate(remaining, 1, &glob_a));
  triax_expect_eq(arena_remaining(&glob_a), 0u);

  arena_clear(&glob_a);
}

triax_test(arena, aliasing_patterns) {
  unsigned char arena_storage[1024];
  Arena         x = arena_init_static(arena_storage);

  for (int round = 0; round < 8; ++round) {
    for (int* p; (p = arena_try_new(int, 10, &x));) {
      for (int i = 0; i < 10; ++i) {
        p[i] = i;
        triax_expect_eq(p[i], i);
      }
    }
    arena_clear(&x);

    for (double* p; (p = arena_try_new(double, 10, &x));) {
      for (int i = 0; i < 10; ++i) {
        p[i] = (double)i;
        triax_expect_eq(p[i], (double)i);
      }
    }
    arena_clear(&x);

    for (short* p; (p = arena_try_new(short, 10, &x));) {
      for (short i = 0; i < 10; ++i) {
        p[i] = i;
        triax_expect_eq(p[i], i);
      }
    }
    arena_clear(&x);
  }
}

triax_test(arena, multiple_clear_reuse_stability) {
  arena_clear(&glob_a);

  for (int round = 0; round < 32; ++round) {
    int* p = arena_new(int, 16, &glob_a);
    triax_expect_nonnull(p);
    fill_ints(p, 16, round * 100);
    expect_ints(p, 16, round * 100);
    arena_clear(&glob_a);
    triax_expect_true(arena_is_empty(&glob_a));
    triax_expect_eq(arena_used(&glob_a), 0u);
    triax_expect_eq(arena_remaining(&glob_a), arena_cap(&glob_a));
  }
}

// ---- arena_try_allocate ----

triax_test(arena, try_allocate_basic) {
  unsigned char buf[64];
  Arena         x = arena_init_static(buf);

  void*         p = arena_try_allocate(16, 1, &x);
  triax_expect_nonnull(p);
  triax_expect_eq(arena_used(&x), 16u);

  // Aligned allocation
  void* q = arena_try_allocate(8, 8, &x);
  triax_expect_nonnull(q);
  triax_expect_eq((uintptr_t)q % 8, 0u);

  // Exactly fills remaining
  size_t rem = arena_remaining(&x);
  void*  r   = arena_try_allocate(rem, 1, &x);
  triax_expect_nonnull(r);
  triax_expect_eq(arena_remaining(&x), 0u);

  // Now full — returns rk_null
  triax_expect_null(arena_try_allocate(1, 1, &x));

  arena_clear(&x);
}

triax_test(arena, try_allocate_zero_size) {
  unsigned char buf[32];
  Arena         x = arena_init_static(buf);

  void*         p = arena_try_allocate(0, 1, &x);
  void*         q = arena_try_allocate(0, 8, &x);
  triax_expect_nonnull(p);
  triax_expect_nonnull(q);
  triax_expect_eq((uintptr_t)q % 8, 0u);
}

// ---- zero-initialised arena ----

triax_test(arena, null_arena) {
  Arena la = {RKI_ZINIT};

  // Accessors are all well-defined: pure pointer arithmetic on rk_null ptrs
  triax_expect_eq(arena_cap(&la), 0u);
  triax_expect_eq(arena_used(&la), 0u);
  triax_expect_eq(arena_remaining(&la), 0u);
  triax_expect_true(arena_is_empty(&la));

  // try_allocate returns rk_null — no backing memory
  triax_expect_null(arena_try_allocate(1, 1, &la));
  triax_expect_null(arena_try_allocate(0, 1, &la));

  // mark captures rk_null cur; rewind_to exits via early-out (mark.pos == cur)
  ArenaMark m = arena_mark(&la);
  triax_expect_null(m.pos);
  triax_expect_eq(arena_rewind_to(&la, m), &la);
  triax_expect_eq(la.cur, la.beg);

  // clear is a no-op
  triax_expect_eq(arena_clear(&la), &la);
  triax_expect_true(arena_is_empty(&la));
}

// Resizing on a null-backed arena fails without touching it, including a zero-to-zero resize.
triax_test(arena, null_arena_resize_and_extend_fail) {
  Arena la = {RKI_ZINIT};
  int   x  = 0;
  triax_expect_null(arena_try_resize_top(0, 0, &la));
  triax_expect_null(arena_try_resize_top(0, 8, &la));
  triax_expect_null(arena_try_extend(&x, 1, 2, &la));
  triax_expect_null(arena_try_extend(&x, 1, 0, &la));
  triax_expect_null(la.cur);
}

// ---- allocator interface ----
#if RK_CUSTOM_ALLOCATORS

triax_test(arena, deallocate_top_only_via_allocator) {
  arena_clear(&glob_a);
  SWAP_ALLOC(arena_to_alloc(&glob_a));
  int* p1 = alloc_new(int, 4);
  int* p2 = alloc_new(int, 4);
  triax_expect_nonnull(p1);
  triax_expect_nonnull(p2);

  unsigned char* top_after_p2 = glob_a.cur;

  alloc_delete(p1, 4);
  triax_expect_eq(glob_a.cur, top_after_p2); // non-top delete: no-op

  alloc_delete(p2, 4);
  triax_expect_eq(glob_a.cur, (unsigned char*)p2); // top delete: cursor rewound

  arena_clear(&glob_a);
  RK_IFNMALLOC(rk_SWAP(alloc_cpy, alloc_ctx);)
}

triax_test(arena, allocator_runtime_conversion) {
  arena_clear(&glob_a);
  SWAP_ALLOC(arena_to_alloc(&glob_a));

  int* top = alloc_new(int, 10);
  triax_expect_nonnull(top);

  alloc_delete(top, 10);
  triax_expect_eq(glob_a.cur, (unsigned char*)top);

  top        = alloc_new(int, 10);
  int* grown = alloc_renew(top, 10, 20);
  triax_expect_eq(grown, top); // top — in-place
  (void)alloc_new(int, 5);     // displace top

  int* copied = alloc_renew(top, 20, 25);
  triax_expect_neq(copied, top); // not top — allocate and copy

  arena_clear(&glob_a);
  RK_IFNMALLOC(rk_SWAP(alloc_cpy, alloc_ctx);)
}

triax_test(arena, arr_allocator_type_and_init_macro) {
  arr_allocator(128) tmp = arr_allocator_init(&tmp);
  SWAP_ALLOC(tmp.alloc);
  triax_expect_nonnull(alloc_ctx.vtab);
  triax_expect_eq(tmp.ctx, &tmp.arena);
  triax_expect_eq(tmp.arena.beg, tmp.arr);
  triax_expect_eq(tmp.arena.cur, tmp.arr);
  triax_expect_eq(tmp.arena.end, tmp.arr + sizeof(tmp.arr));

  int* p = alloc_new(int, 8);
  triax_expect_nonnull(p);
  fill_ints(p, 8, 20);
  expect_ints(p, 8, 20);

  tmp.arena.cur = tmp.arena.beg;
  triax_expect_true(arena_is_empty(&tmp.arena));
  RK_IFNMALLOC(rk_SWAP(alloc_cpy, alloc_ctx);)
}

triax_test(arena, arr_allocator_create_macro) {
  arr_allocator_create(tmp, 256);
  SWAP_ALLOC(tmp.alloc);

  triax_expect_nonnull(tmp.vtab);
  triax_expect_eq(tmp.ctx, &tmp.arena);
  triax_expect_eq(tmp.arena.beg, tmp.arr);
  triax_expect_eq(tmp.arena.cur, tmp.arr);
  triax_expect_eq(tmp.arena.end, tmp.arr + sizeof(tmp.arr));

  int* p1 = alloc_new(int, 16);
  triax_expect_nonnull(p1);
  fill_ints(p1, 16, 0);
  expect_ints(p1, 16, 0);

  alloc_delete(p1, 16);
  triax_expect_eq(tmp.arena.cur, (unsigned char*)p1);
  RK_IFNMALLOC(rk_SWAP(alloc_cpy, alloc_ctx);)
}

triax_test(arena, arr_allocator_as_allocator_interface) {
  arr_allocator_create(tmp, 512);
  SWAP_ALLOC(tmp.alloc);
  int*    p = alloc_new(int, 20);
  double* q = alloc_new(double, 8);

  triax_expect_nonnull(p);
  triax_expect_nonnull(q);

  fill_ints(p, 20, 100);
  expect_ints(p, 20, 100);

  for (int i = 0; i < 8; ++i) { q[i] = (double)i * 0.5; }
  for (int i = 0; i < 8; ++i) { triax_expect_eq(q[i], (double)i * 0.5); }
  rk_SWAP(alloc_cpy, alloc_ctx);
}

#endif

// A null-backed arena rewinds to its own null-position mark.
triax_test(arena, rewind_null_backed_to_null_mark) {
  Arena     a = {0};
  ArenaMark m = arena_mark(&a);
  arena_rewind_to(&a, m);
  triax_expect_null(a.cur);
  triax_expect_true(arena_is_empty(&a));
}

// A null-position mark used after the arena was given backing storage is stale.
triax_test(arena, rewind_to_null_mark_on_backed_arena_asserts, .isolation = TRIAX_ISOLATION_ON) {
  Arena         a         = {0};
  ArenaMark     null_mark = arena_mark(&a);
  unsigned char buf[64];
  a = arena_init(buf, sizeof buf);
  (void)arena_allocate(8, 1, &a);
#ifdef RKLIB_DEBUG
  triax_assert_fault(TRIAX_FAULT_ABORT, { (void)arena_rewind_to(&a, null_mark); });
#endif
}

// The raw realloc functions dispatch on a null pointer, for every alignment.
triax_test(alloc, malloc_reallocate_null_ptr_allocates) {
  for (size_t align = 1; align <= 256; align *= 2) {
    unsigned char* p = (unsigned char*)malloc_reallocate(rk_null, 0, 128, align);
    triax_assert_nonnull(p);
    triax_expect_eq((uptr)p % align, 0u);
    p[127] = 1;
    malloc_deallocate(p, align);
  }
}

triax_test(alloc, page_realloc_null_ptr_allocates) {
  unsigned char* p = (unsigned char*)page_realloc(rk_null, 0, 200);
  triax_assert_nonnull(p);
  p[199] = 1;
  page_free(p, 200);
}

triax_test(alloc, page_free_null_is_noop) {
  page_free(rk_null, 0);
  page_free(rk_null, 4096);
  triax_expect_null(page_realloc(rk_null, 0, 0));
}

// Pointer/old-size mismatches are contract violations at every layer, including when the new
// size is zero (which previously skipped the check in page_realloc and leaked the mapping).
// An expected fault ends the test process, so each case needs its own test.
#ifdef RKLIB_DEBUG
triax_test(alloc, page_realloc_ptr_zero_old_size_zero_new_asserts,
           .isolation = TRIAX_ISOLATION_ON) {
  void* p = page_alloc(100);
  triax_assert_fault(TRIAX_FAULT_ABORT, { (void)page_realloc(p, 0, 0); });
}

triax_test(alloc, page_realloc_null_nonzero_old_size_zero_new_asserts,
           .isolation = TRIAX_ISOLATION_ON) {
  triax_assert_fault(TRIAX_FAULT_ABORT, { (void)page_realloc(rk_null, 100, 0); });
}

triax_test(alloc, malloc_reallocate_ptr_zero_old_size_zero_new_asserts,
           .isolation = TRIAX_ISOLATION_ON) {
  void* p = malloc_allocate(64, 8);
  triax_assert_fault(TRIAX_FAULT_ABORT, { (void)malloc_reallocate(p, 0, 0, 8); });
}

triax_test(alloc, malloc_reallocate_overaligned_ptr_zero_old_size_zero_new_asserts,
           .isolation = TRIAX_ISOLATION_ON) {
  void* p = malloc_allocate(64, 64);
  triax_assert_fault(TRIAX_FAULT_ABORT, { (void)malloc_reallocate(p, 0, 0, 64); });
}

triax_test(alloc, malloc_reallocate_null_nonzero_old_size_zero_new_asserts,
           .isolation = TRIAX_ISOLATION_ON) {
  triax_assert_fault(TRIAX_FAULT_ABORT, { (void)malloc_reallocate(rk_null, 64, 0, 8); });
}

triax_test(alloc, malloc_reallocate_null_nonzero_old_size_asserts,
           .isolation = TRIAX_ISOLATION_ON) {
  triax_assert_fault(TRIAX_FAULT_ABORT, { (void)malloc_reallocate(rk_null, 64, 128, 64); });
}
#endif

RKI_IGNWARN_CLANG_END()
RKI_HEADER_END

#endif
