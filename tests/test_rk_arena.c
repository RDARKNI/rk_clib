#ifndef RK__TESTDUMMY
# include "../rk_test/rk_test.h"
#else
# include "../rk_test/rk_test_dummy.h"
#endif

#define RK_IMPL
#include "../include/rklib_includeall.h"

RK_HEADER_BEGIN
RK__IGNWARN_CLANG_BEG("-Wunused-variable")
#define ARENA_SIZE 1024

static unsigned char arena_buf[ARENA_SIZE];
static Arena         a = arena_init_static(arena_buf);

static void          fill_ints(int* p, int n, int base) {
  for (int i = 0; i < n; ++i) { p[i] = base + i; }
}

static void expect_ints(const int* p, int n, int base) {
  for (int i = 0; i < n; ++i) { rk_expect_eq(p[i], base + i); }
}

// ---- init ----

RK_REGISTER_TEST("arena", test_arena_init_runtime) {
  unsigned char buf[73];
  Arena         x = arena_init(buf, sizeof(buf));

  rk_expect_eq(x.beg, buf);
  rk_expect_eq(x.cur, buf);
  rk_expect_eq(x.end, buf + sizeof(buf));

  rk_expect_eq(arena_cap(&x), sizeof(buf));
  rk_expect_eq(arena_used(&x), 0u);
  rk_expect_eq(arena_remaining(&x), sizeof(buf));
  rk_expect_true(arena_is_empty(&x));
}

RK_REGISTER_TEST("arena", test_arena_init_runtime_null) {
  Arena x = arena_init(rk_null, 123);

  rk_expect_eq(x.beg, rk_null);
  rk_expect_eq(x.cur, rk_null);
  rk_expect_eq(x.end, rk_null);
}

RK_REGISTER_TEST("arena", test_arena_init_static_basic) {
  unsigned char buf[64];
  Arena         x = arena_init_static(buf);

  rk_expect_eq(x.beg, buf);
  rk_expect_eq(x.cur, buf);
  rk_expect_eq(x.end, buf + sizeof(buf));

  rk_expect_eq(arena_cap(&x), sizeof(buf));
  rk_expect_eq(arena_used(&x), 0u);
  rk_expect_eq(arena_remaining(&x), sizeof(buf));
  rk_expect_true(arena_is_empty(&x));
}

// ---- cap / used / remaining ----

RK_REGISTER_TEST("arena", test_arena_cap_used_remaining_empty) {
  arena_clear(&a);

  rk_expect_eq(arena_cap(&a), ARENA_SIZE);
  rk_expect_eq(arena_used(&a), 0u);
  rk_expect_eq(arena_remaining(&a), ARENA_SIZE);
  rk_expect_true(arena_is_empty(&a));

  void* p1 = RK__arena_allocate(13, 1, &a);
  rk_expect_nonnull(p1);
  rk_expect_false(arena_is_empty(&a));
  rk_expect_eq(arena_used(&a) + arena_remaining(&a), arena_cap(&a));

  void* p2 = RK__arena_allocate(17, 8, &a);
  rk_expect_nonnull(p2);
  rk_expect_eq(arena_used(&a) + arena_remaining(&a), arena_cap(&a));

  arena_clear(&a);
  rk_expect_true(arena_is_empty(&a));
  rk_expect_eq(arena_used(&a), 0u);
  rk_expect_eq(arena_remaining(&a), ARENA_SIZE);
}

// ---- alloc ----

RK_REGISTER_TEST("arena", test_arena_alloc_basic) {
  arena_clear(&a);

  size_t ints    = 10;
  size_t longs   = 100;
  size_t alloced = 0;

  int*   i       = arena_new(int, ints, &a);
  rk_expect_nonnull(i);
  rk_expect_eq((uptr)i % alignof(int), 0u);
  rk_expect_true(arena_used(&a) >= (alloced = ints * sizeof(*i)));

  long* l = arena_new(long, longs, &a);
  rk_expect_nonnull(l);
  rk_expect_eq((uptr)l % alignof(long), 0u);
  rk_expect_true(arena_used(&a) >= (alloced += longs * sizeof(*l)));

  rk_expect_eq(arena_used(&a) + arena_remaining(&a), arena_cap(&a));
  arena_clear(&a);
}

RK_REGISTER_TEST("arena", test_RK__arena_allocate_zero_size) {
  arena_clear(&a);

  void* p1 = RK__arena_allocate(0, 1, &a);
  void* p2 = RK__arena_allocate(0, 8, &a);

  rk_expect_nonnull(p1);
  rk_expect_nonnull(p2);
  rk_expect_eq((uintptr_t)p1 % 1, 0u);
  rk_expect_eq((uintptr_t)p2 % 8, 0u);
  rk_expect_eq(arena_used(&a) + arena_remaining(&a), arena_cap(&a));

  arena_clear(&a);
}

RK_REGISTER_TEST("arena", test_RK__arena_allocate_alignment_range) {
  arena_clear(&a);

  for (size_t align = 1; align <= 64; align <<= 1) {
    void* p = RK__arena_allocate(1, align, &a);
    rk_expect_nonnull(p);
    rk_expect_eq((uintptr_t)p % align, 0u);
  }

  arena_clear(&a);
}

RK_REGISTER_TEST("arena", test_RK__arena_allocate_full_and_fail) {
  unsigned char buf[128];
  Arena         x = arena_init_static(buf);

  rk_expect_nonnull(RK__arena_allocate(sizeof(buf), 1, &x));
  rk_expect_eq(arena_remaining(&x), 0u);
  rk_assert_crash(RKT_FAULT_ANY, RK__arena_allocate(1, 1, &x););
}

RK_REGISTER_TEST("arena", test_arena_new_zero_count) {
  arena_clear(&a);

  int* p = arena_new(int, 0, &a);
  rk_expect_nonnull(p);
  rk_expect_eq((uintptr_t)p % alignof(int), 0u);
  rk_expect_true(arena_used(&a) <= arena_cap(&a));

  arena_clear(&a);
}

RK_REGISTER_TEST("arena", test_arena_new_aligned_overaligned) {
  arena_clear(&a);

  int* p = arena_new_aligned(int, 3, 32, &a);
  rk_expect_nonnull(p);
  rk_expect_eq((uintptr_t)p % 32, 0u);

  double* q = arena_new_aligned(double, 2, 64, &a);
  rk_expect_nonnull(q);
  rk_expect_eq((uintptr_t)q % 64, 0u);

  arena_clear(&a);
}

// ---- try_new ----

RK_REGISTER_TEST("arena", test_arena_try_new_returns_null_on_oom) {
  unsigned char buf[64];
  Arena         x = arena_init_static(buf);

  int*          p = arena_try_new(int, 4, &x);
  rk_expect_nonnull(p);
  rk_expect_eq((uintptr_t)p % alignof(int), 0u);

  int* q = arena_try_new(int, 10000, &x);
  rk_expect_null(q);

  arena_clear(&x);
}

RK_REGISTER_TEST("arena", test_arena_try_new_aligned_returns_null_on_oom) {
  unsigned char buf[128];
  Arena         x = arena_init_static(buf);

  int*          p = arena_try_new_aligned(int, 2, 32, &x);
  rk_expect_nonnull(p);
  rk_expect_eq((uintptr_t)p % 32, 0u);

  int* q = arena_try_new_aligned(int, 10000, 32, &x);
  rk_expect_null(q);

  arena_clear(&x);
}

// ---- clear ----

RK_REGISTER_TEST("arena", test_arena_clear_idempotent) {
  arena_clear(&a);
  rk_expect_eq(arena_clear(&a), &a);
  rk_expect_eq(a.cur, a.beg);

  rk_expect_nonnull(RK__arena_allocate(10, 1, &a));
  rk_expect_false(arena_is_empty(&a));

  rk_expect_eq(arena_clear(&a), &a);
  rk_expect_true(arena_is_empty(&a));
  rk_expect_eq(a.cur, a.beg);

  rk_expect_eq(arena_clear(&a), &a);
  rk_expect_eq(a.cur, a.beg);
}

// ---- mark / rewind_to ----

RK_REGISTER_TEST("arena", test_arena_mark_rewind_basic) {
  arena_clear(&a);

  ArenaMark m0 = arena_mark(&a);
  rk_expect_nonnull(RK__arena_allocate(10, 1, &a));
  ArenaMark m1 = arena_mark(&a);
  rk_expect_nonnull(RK__arena_allocate(20, 1, &a));

  rk_expect_eq(arena_rewind_to(&a, m1), &a);
  rk_expect_eq(a.cur, m1.pos);

  rk_expect_eq(arena_rewind_to(&a, m0), &a);
  rk_expect_eq(a.cur, m0.pos);
  rk_expect_true(arena_is_empty(&a));
}

RK_REGISTER_TEST("arena", test_arena_mark_rewind_roundtrip) {
  arena_clear(&a);

  arena_new(char, ARENA_SIZE / 4, &a);
  ArenaMark mark = arena_mark(&a);
  arena_new(char, ARENA_SIZE / 4, &a);
  unsigned char* prev = a.cur;

  arena_rewind_to(&a, mark);
  rk_expect_eq(a.cur, mark.pos);

  arena_new(char, ARENA_SIZE / 4, &a);
  rk_expect_eq(a.cur, prev);

  arena_rewind_to(&a, mark);
  rk_expect_eq(a.cur, mark.pos);

  arena_clear(&a);
  rk_expect_eq(a.cur, a.beg);
}

// ---- is_top_allocation ----

RK_REGISTER_TEST("arena", test_arena_is_top_allocation) {
  arena_clear(&a);

  int* p = arena_new(int, 4, &a);
  rk_expect_true(arena_is_top_allocation(&a, p, 4 * sizeof(int)));

  int* q = arena_new(int, 2, &a);
  rk_expect_false(arena_is_top_allocation(&a, p, 4 * sizeof(int)));
  rk_expect_true(arena_is_top_allocation(&a, q, 2 * sizeof(int)));

  arena_clear(&a);
}

// ---- extend / try_extend ----

RK_REGISTER_TEST("arena", test_arena_extend_in_place_grow_and_shrink) {
  arena_clear(&a);

  int* arr = arena_new(int, 10, &a);
  rk_expect_nonnull(arr);
  fill_ints(arr, 10, 0);

  int* grown = arena_extend(arr, 10, 20, &a);
  rk_expect_nonnull(grown);
  rk_expect_eq(grown, arr);
  expect_ints(grown, 10, 0);

  int* shrunk = arena_extend(grown, 20, 5, &a);
  rk_expect_nonnull(shrunk);
  rk_expect_eq(shrunk, arr);
  expect_ints(shrunk, 5, 0);

  arena_clear(&a);
}

RK_REGISTER_TEST("arena", test_arena_extend_not_top_asserts) {
  arena_clear(&a);
  int* arr = arena_new(int, 5, &a);
  fill_ints(arr, 5, 0);
  arena_new(int, 10, &a); // displace top
  // non-top pointer is a programming error — asserts
#ifdef RKLIB_DEBUG
  rk_assert_crash(RKT_FAULT_ANY, arena_extend(arr, 5, 10, &a););
#endif
  arena_clear(&a);
}

RK_REGISTER_TEST("arena", test_arena_try_extend_oom_returns_null) {
  unsigned char buf[sizeof(int) * 4];
  Arena         x = arena_init_static(buf);

  int*          p = arena_new(int, 4, &x);
  rk_expect_nonnull(p);
  rk_expect_eq(arena_remaining(&x), 0u);

  int* q = arena_try_extend(p, 4, 8, &x);
  rk_expect_null(q);

  arena_clear(&x);
}

// ---- allocator interface ----

RK_REGISTER_TEST("arena", test_arena_deallocate_top_only_via_allocator) {
  arena_clear(&a);

  Allocator alloc = arena_to_alloc(&a);

  int*      p1    = alloc_new(int, 4, alloc);
  int*      p2    = alloc_new(int, 4, alloc);
  rk_expect_nonnull(p1);
  rk_expect_nonnull(p2);

  unsigned char* top_after_p2 = a.cur;

  alloc_delete(p1, 4, alloc);
  rk_expect_eq(a.cur, top_after_p2); // non-top delete: no-op

  alloc_delete(p2, 4, alloc);
  rk_expect_eq(a.cur, (unsigned char*)p2); // top delete: cursor rewound

  arena_clear(&a);
}

RK_REGISTER_TEST("arena", test_arena_allocator_runtime_conversion) {
  arena_clear(&a);

  Allocator alloc = arena_to_alloc(&a);
  int*      top   = alloc_new(int, 10, alloc);
  rk_expect_nonnull(top);

  alloc_delete(top, 10, alloc);
  rk_expect_eq(a.cur, (unsigned char*)top);

  top        = alloc_new(int, 10, alloc);
  int* grown = alloc_renew(top, 10, 20, alloc);
  rk_expect_eq(grown, top); // top — in-place

  (void)alloc_new(int, 5, alloc); // displace top

  int* copied = alloc_renew(top, 20, 25, alloc);
  rk_expect_neq(copied, top); // not top — allocate and copy

  arena_clear(&a);
}

RK_REGISTER_TEST("arena", test_arena_to_alloc_static_macro) {
  arena_clear(&a);
  Allocator alloc = arena_to_alloc_static(&a);
  int*      p     = alloc_new(int, 3, alloc);

  rk_expect_nonnull(p);
  fill_ints(p, 3, 7);
  expect_ints(p, 3, 7);

  arena_clear(&a);
}

RK_REGISTER_TEST("arena", test_arr_allocator_type_and_init_macro) {
  arr_allocator(128) local = arr_allocator_init(&local);
  rk_expect_nonnull(local.vtab);
  rk_expect_eq(local.ctx, &local.arena);
  rk_expect_eq(local.arena.beg, local.arr);
  rk_expect_eq(local.arena.cur, local.arr);
  rk_expect_eq(local.arena.end, local.arr + sizeof(local.arr));

  int* p = alloc_new(int, 8, local);
  rk_expect_nonnull(p);
  fill_ints(p, 8, 20);
  expect_ints(p, 8, 20);

  local.arena.cur = local.arena.beg;
  rk_expect_true(arena_is_empty(&local.arena));
}

RK_REGISTER_TEST("arena", test_arr_allocator_create_macro) {
  arr_allocator_create(tmp, 256);
  rk_expect_nonnull(tmp.vtab);
  rk_expect_eq(tmp.ctx, &tmp.arena);
  rk_expect_eq(tmp.arena.beg, tmp.arr);
  rk_expect_eq(tmp.arena.cur, tmp.arr);
  rk_expect_eq(tmp.arena.end, tmp.arr + sizeof(tmp.arr));

  int* p1 = alloc_new(int, 16, tmp);
  rk_expect_nonnull(p1);
  fill_ints(p1, 16, 0);
  expect_ints(p1, 16, 0);

  alloc_delete(p1, 16, tmp);
  rk_expect_eq(tmp.arena.cur, (unsigned char*)p1);
}

RK_REGISTER_TEST("arena", test_arr_allocator_as_allocator_interface) {
  arr_allocator_create(tmp, 512);

  int*    p = alloc_new(int, 20, tmp);
  double* q = alloc_new(double, 8, tmp);

  rk_expect_nonnull(p);
  rk_expect_nonnull(q);

  fill_ints(p, 20, 100);
  expect_ints(p, 20, 100);

  for (int i = 0; i < 8; ++i) { q[i] = (double)i * 0.5; }
  for (int i = 0; i < 8; ++i) { rk_expect_eq(q[i], (double)i * 0.5); }
}

// ---- edge cases ----

RK_REGISTER_TEST("arena", test_arena_edgecases_full_allocation) {
  arena_clear(&a);

  rk_expect_nonnull(RK__arena_allocate(0, 1, &a));

  size_t remaining = arena_remaining(&a);
  rk_expect_nonnull(RK__arena_allocate(remaining, 1, &a));
  rk_expect_eq(arena_remaining(&a), 0u);

  arena_clear(&a);
}

RK_REGISTER_TEST("arena", test_arena_aliasing_patterns) {
  unsigned char storage[1024];
  Arena         x = arena_init_static(storage);

  for (int round = 0; round < 8; ++round) {
    for (int* p; (p = arena_try_new(int, 10, &x));) {
      for (int i = 0; i < 10; ++i) {
        p[i] = i;
        rk_expect_eq(p[i], i);
      }
    }
    arena_clear(&x);

    for (double* p; (p = arena_try_new(double, 10, &x));) {
      for (int i = 0; i < 10; ++i) {
        p[i] = (double)i;
        rk_expect_eq(p[i], (double)i);
      }
    }
    arena_clear(&x);

    for (short* p; (p = arena_try_new(short, 10, &x));) {
      for (short i = 0; i < 10; ++i) {
        p[i] = i;
        rk_expect_eq(p[i], i);
      }
    }
    arena_clear(&x);
  }
}

RK_REGISTER_TEST("arena", test_arena_multiple_clear_reuse_stability) {
  arena_clear(&a);

  for (int round = 0; round < 32; ++round) {
    int* p = arena_new(int, 16, &a);
    rk_expect_nonnull(p);
    fill_ints(p, 16, round * 100);
    expect_ints(p, 16, round * 100);
    arena_clear(&a);
    rk_expect_true(arena_is_empty(&a));
    rk_expect_eq(arena_used(&a), 0u);
    rk_expect_eq(arena_remaining(&a), arena_cap(&a));
  }
}

// ---- arena_try_allocate ----

RK_REGISTER_TEST("arena", test_arena_try_allocate_basic) {
  unsigned char buf[64];
  Arena         x = arena_init_static(buf);

  void*         p = arena_try_allocate(16, 1, &x);
  rk_expect_nonnull(p);
  rk_expect_eq(arena_used(&x), 16u);

  // Aligned allocation
  void* q = arena_try_allocate(8, 8, &x);
  rk_expect_nonnull(q);
  rk_expect_eq((uintptr_t)q % 8, 0u);

  // Exactly fills remaining
  size_t rem = arena_remaining(&x);
  void*  r   = arena_try_allocate(rem, 1, &x);
  rk_expect_nonnull(r);
  rk_expect_eq(arena_remaining(&x), 0u);

  // Now full — returns rk_null
  rk_expect_null(arena_try_allocate(1, 1, &x));

  arena_clear(&x);
}

RK_REGISTER_TEST("arena", test_arena_try_allocate_zero_size) {
  unsigned char buf[32];
  Arena         x = arena_init_static(buf);

  void*         p = arena_try_allocate(0, 1, &x);
  void*         q = arena_try_allocate(0, 8, &x);
  rk_expect_nonnull(p);
  rk_expect_nonnull(q);
  rk_expect_eq((uintptr_t)q % 8, 0u);
}

// ---- zero-initialised arena ----

RK_REGISTER_TEST("arena", arena_test_null_arena) {
  Arena a = {RK_ZINIT};

  // Accessors are all well-defined: pure pointer arithmetic on rk_null ptrs
  rk_expect_eq(arena_cap(&a), 0u);
  rk_expect_eq(arena_used(&a), 0u);
  rk_expect_eq(arena_remaining(&a), 0u);
  rk_expect_true(arena_is_empty(&a));

  // try_allocate returns rk_null — no backing memory
  rk_expect_null(arena_try_allocate(1, 1, &a));
  rk_expect_null(arena_try_allocate(0, 1, &a));

  // mark captures rk_null cur; rewind_to exits via early-out (mark.pos == cur)
  ArenaMark m = arena_mark(&a);
  rk_expect_null(m.pos);
  rk_expect_eq(arena_rewind_to(&a, m), &a);
  rk_expect_eq(a.cur, a.beg);

  // clear is a no-op
  rk_expect_eq(arena_clear(&a), &a);
  rk_expect_true(arena_is_empty(&a));
}

RK__IGNWARN_CLANG_END()
RK_HEADER_END
