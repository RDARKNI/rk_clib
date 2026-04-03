#ifndef RK__TESTDUMMY
# include "../rk_test/rk_test.h"
#else
# include "../rk_test/rk_test_dummy.h"
#endif

#define RK_IMPL
#include "../include/rklib_includeall.h"

RK_HEADER_BEGIN
RK__IGNWARN_CLANG_BEG("-Wunused-variable")

#define ALIST_ARENA_SIZE 128

static void fill_ints_al(int* p, int n, int base) {
  for (int i = 0; i < n; ++i) { p[i] = base + i; }
}

static void expect_ints_al(const int* p, int n, int base) {
  for (int i = 0; i < n; ++i) { rk_expect_eq(p[i], base + i); }
}

// ---- init ----

RK_REGISTER_TEST("arenalist", test_arenalist_init_basic) {
  ArenaList al = arenalist_init(ALIST_ARENA_SIZE);

  rk_expect_eq(al.cur, 0u);
  rk_expect_true(al.arena_size >= ALIST_ARENA_SIZE);
  rk_expect_true(stdc_has_single_bit(al.arena_size));
  rk_expect_eq(vec_count(al.arenas), 1u);

  rk_expect_nonnull(al.arenas[0].beg);
  rk_expect_eq(al.arenas[0].cur, al.arenas[0].beg);
  rk_expect_eq(arena_cap(&al.arenas[0]), al.arena_size);
  rk_expect_eq(arena_used(&al.arenas[0]), 0u);
  rk_expect_eq(arena_remaining(&al.arenas[0]), al.arena_size);

  arenalist_release(&al);
  rk_expect_eq(al.arena_size, 0u);
  rk_expect_eq(al.cur, 0u);
}

RK_REGISTER_TEST("arenalist", test_arenalist_init_rounds_to_pow2) {
  ArenaList al = arenalist_init(100);

  rk_expect_true(stdc_has_single_bit(al.arena_size));
  rk_expect_true(al.arena_size >= 100u);
  rk_expect_eq(arena_cap(&al.arenas[0]), al.arena_size);

  arenalist_release(&al);
}

// ---- new / new_aligned ----

RK_REGISTER_TEST("arenalist", test_arenalist_new_basic) {
  ArenaList al = arenalist_init(ALIST_ARENA_SIZE);

  int*      p  = arenalist_new(int, 10, &al);
  rk_expect_nonnull(p);
  rk_expect_eq((uintptr_t)p % alignof(int), 0u);
  fill_ints_al(p, 10, 0);
  expect_ints_al(p, 10, 0);

  rk_expect_eq(al.cur, 0u);
  rk_expect_eq(vec_count(al.arenas), 1u);
  rk_expect_true(arena_used(&al.arenas[0]) >= sizeof(int) * 10u);

  arenalist_release(&al);
}

RK_REGISTER_TEST("arenalist", test_arenalist_new_zero_count) {
  ArenaList al = arenalist_init(ALIST_ARENA_SIZE);

  int*      p  = arenalist_new(int, 0, &al);
  rk_expect_nonnull(p);
  rk_expect_eq((uintptr_t)p % alignof(int), 0u);

  arenalist_release(&al);
}

RK_REGISTER_TEST("arenalist", test_arenalist_new_aligned) {
  ArenaList al = arenalist_init(ALIST_ARENA_SIZE);

  int*      p  = arenalist_new_aligned(int, 3, 32, &al);
  rk_expect_nonnull(p);
  rk_expect_eq((uintptr_t)p % 32u, 0u);

  double* q = arenalist_new_aligned(double, 2, 64, &al);
  rk_expect_nonnull(q);
  rk_expect_eq((uintptr_t)q % 64u, 0u);

  arenalist_release(&al);
}

RK_REGISTER_TEST("arenalist", test_arenalist_spills_into_multiple_arenas) {
  ArenaList al = arenalist_init(64);

  int*      p1 = arenalist_new(int, 8, &al);
  rk_expect_nonnull(p1);
  fill_ints_al(p1, 8, 0);

  int* p2 = arenalist_new(int, 16, &al);
  rk_expect_nonnull(p2);
  fill_ints_al(p2, 16, 100);

  rk_expect_true(vec_count(al.arenas) >= 2u);
  rk_expect_true(al.cur >= 1u);

  expect_ints_al(p1, 8, 0);
  expect_ints_al(p2, 16, 100);

  arenalist_release(&al);
}

RK_REGISTER_TEST("arenalist", test_arenalist_large_alloc_grows_arena_size) {
  ArenaList al = arenalist_init(64);

  rk_expect_eq(al.arena_size, 64u);

  char* p = arenalist_new(char, 200, &al);
  rk_expect_nonnull(p);

  rk_expect_true(vec_count(al.arenas) >= 2u);
  rk_expect_true(arena_cap(&al.arenas[al.cur]) >= 200u);

  arenalist_release(&al);
}

// ---- clear ----

RK_REGISTER_TEST("arenalist", test_arenalist_clear_single_arena) {
  ArenaList al = arenalist_init(ALIST_ARENA_SIZE);

  rk_expect_nonnull(arenalist_new(int, 10, &al));
  rk_expect_nonnull(arenalist_new(char, 7, &al));

  rk_expect_eq(arenalist_clear(&al), &al);
  rk_expect_eq(al.cur, 0u);
  rk_expect_eq(arena_used(&al.arenas[0]), 0u);
  rk_expect_true(arena_is_empty(&al.arenas[0]));

  arenalist_release(&al);
}

RK_REGISTER_TEST("arenalist", test_arenalist_clear_multiple_arenas) {
  ArenaList al = arenalist_init(64);

  rk_expect_nonnull(arenalist_new(char, 48, &al));
  rk_expect_nonnull(arenalist_new(char, 48, &al));
  rk_expect_nonnull(arenalist_new(char, 48, &al));

  rk_expect_true(vec_count(al.arenas) >= 3u);
  rk_expect_true(al.cur >= 2u);

  rk_expect_eq(arenalist_clear(&al), &al);
  rk_expect_eq(al.cur, 0u);

  for (size_t i = 0; i < vec_count(al.arenas); ++i) {
    rk_expect_eq(arena_used(&al.arenas[i]), 0u);
    rk_expect_true(arena_is_empty(&al.arenas[i]));
  }

  arenalist_release(&al);
}

// ---- mark / rewind_to ----

RK_REGISTER_TEST("arenalist", test_arenalist_mark_rewind_same_arena) {
  ArenaList al = arenalist_init(ALIST_ARENA_SIZE);

  char*     p1 = arenalist_new(char, 16, &al);
  ArenaMark m  = arenalist_mark(&al);
  char*     p2 = arenalist_new(char, 24, &al);
  char*     p3 = arenalist_new(char, 8, &al);
  rk_expect_nonnull(p1);
  rk_expect_nonnull(p2);
  rk_expect_nonnull(p3);

  rk_expect_eq(arenalist_rewind_to(&al, m), &al);
  rk_expect_eq(al.cur, 0u);
  rk_expect_eq(al.arenas[0].cur, m.pos);

  arenalist_release(&al);
}

RK_REGISTER_TEST("arenalist", test_arenalist_mark_rewind_previous_arena) {
  ArenaList al = arenalist_init(64);

  char*     p1 = arenalist_new(char, 48, &al);
  ArenaMark m  = arenalist_mark(&al); // mark after p1, in arena 0
  char*     p2 = arenalist_new(char, 48, &al);
  char*     p3 = arenalist_new(char, 48, &al);
  rk_expect_nonnull(p1);
  rk_expect_nonnull(p2);
  rk_expect_nonnull(p3);

  rk_expect_true(al.cur >= 2u);

  rk_expect_eq(arenalist_rewind_to(&al, m), &al);
  rk_expect_eq(al.cur, 0u);
  rk_expect_eq(al.arenas[0].cur, m.pos);

  for (size_t i = 1; i < vec_count(al.arenas); ++i) {
    rk_expect_true(arena_is_empty(&al.arenas[i]));
  }

  arenalist_release(&al);
}

RK_REGISTER_TEST("arenalist", test_arenalist_mark_rewind_roundtrip) {
  ArenaList al   = arenalist_init(64);

  char*     p1   = arenalist_new(char, 32, &al);
  ArenaMark mark = arenalist_mark(&al);
  char*     p2   = arenalist_new(char, 48, &al); // spills to next arena
  rk_expect_nonnull(p1);
  rk_expect_nonnull(p2);

  size_t arenas_after = vec_count(al.arenas);
  rk_expect_true(arenas_after >= 2u);

  arenalist_rewind_to(&al, mark);
  rk_expect_eq(al.cur, 0u);
  rk_expect_eq(al.arenas[0].cur, mark.pos);

  // re-allocate same amount — arenas reused, no new allocations
  char* p3 = arenalist_new(char, 48, &al);
  rk_expect_nonnull(p3);
  rk_expect_eq(vec_count(al.arenas), arenas_after);

  arenalist_release(&al);
}

RK_REGISTER_TEST("arenalist", test_arenalist_mark_null_rewind_noop) {
  ArenaList al        = arenalist_init(ALIST_ARENA_SIZE);
  ArenaMark null_mark = {rk_null};

  rk_expect_nonnull(arenalist_new(char, 10, &al));
  size_t used = arena_used(&al.arenas[0]);

  arenalist_rewind_to(&al, null_mark); // should be a no-op
  rk_expect_eq(arena_used(&al.arenas[0]), used);

  arenalist_release(&al);
}

// ---- allocator interface ----

RK_REGISTER_TEST("arenalist", test_arenalist_to_alloc_basic) {
  ArenaList al    = arenalist_init(128);
  Allocator alloc = arenalist_to_alloc(&al);

  int*      arr   = alloc_new(int, 10, alloc);
  rk_expect_nonnull(arr);
  fill_ints_al(arr, 10, 0);
  expect_ints_al(arr, 10, 0);

  int* arr2 = alloc_renew(arr, 10, 20, alloc);
  rk_expect_eq(arr2, arr); // top — in-place
  expect_ints_al(arr2, 10, 0);

  alloc_delete(arr2, 20, alloc);

  arenalist_release(&al);
}

RK_REGISTER_TEST("arenalist", test_arenalist_allocator_deallocate_top_only) {
  ArenaList al    = arenalist_init(128);
  Allocator alloc = arenalist_to_alloc(&al);

  int*      p1    = alloc_new(int, 4, alloc);
  int*      p2    = alloc_new(int, 4, alloc);
  rk_expect_nonnull(p1);
  rk_expect_nonnull(p2);

  unsigned char* top_after_p2 = al.arenas[al.cur].cur;
  alloc_delete(p1, 4, alloc);
  rk_expect_eq(al.arenas[al.cur].cur, top_after_p2); // non-top: no-op

  alloc_delete(p2, 4, alloc);
  rk_expect_eq(al.arenas[al.cur].cur, (unsigned char*)p2); // top: rewound

  arenalist_release(&al);
}

RK_REGISTER_TEST("arenalist", test_arenalist_allocator_realloc_copy_path) {
  ArenaList al    = arenalist_init(128);
  Allocator alloc = arenalist_to_alloc(&al);

  int*      p     = alloc_new(int, 5, alloc);
  rk_expect_nonnull(p);
  fill_ints_al(p, 5, 100);

  rk_expect_nonnull(alloc_new(int, 10, alloc)); // displace top

  int* q = alloc_renew(p, 5, 12, alloc);
  rk_expect_nonnull(q);
  rk_expect_neq(q, p); // not top — allocate and copy
  expect_ints_al(q, 5, 100);
  arenalist_release(&al);
}

// ---- reuse / stress ----

RK_REGISTER_TEST("arenalist", test_arenalist_reuse_after_clear) {
  ArenaList al = arenalist_init(64);

  for (int round = 0; round < 16; ++round) {
    int* p = arenalist_new(int, 8, &al);
    rk_expect_nonnull(p);
    fill_ints_al(p, 8, round * 10);
    expect_ints_al(p, 8, round * 10);

    rk_expect_eq(arenalist_clear(&al), &al);
    rk_expect_eq(al.cur, 0u);
    for (size_t i = 0; i < vec_count(al.arenas); ++i) {
      rk_expect_true(arena_is_empty(&al.arenas[i]));
    }
  }

  arenalist_release(&al);
}

RK_REGISTER_TEST("arenalist", test_arenalist_multiple_type_patterns) {
  ArenaList al = arenalist_init(128);

  for (int round = 0; round < 8; ++round) {
    for (int* p; (p = arena_try_new(int, 10, &al.arenas[al.cur]));) {
      rk_expect_nonnull(p);
      for (int i = 0; i < 10; ++i) {
        p[i] = i;
        rk_expect_eq(p[i], i);
      }
    }
    arenalist_clear(&al);

    for (double* p; (p = arena_try_new(double, 8, &al.arenas[al.cur]));) {
      rk_expect_nonnull(p);
      for (int i = 0; i < 8; ++i) {
        p[i] = (double)i;
        rk_expect_eq(p[i], (double)i);
      }
    }
    arenalist_clear(&al);

    for (short* p; (p = arena_try_new(short, 12, &al.arenas[al.cur]));) {
      rk_expect_nonnull(p);
      for (short i = 0; i < 12; ++i) {
        p[i] = i;
        rk_expect_eq(p[i], i);
      }
    }
    arenalist_clear(&al);
  }

  arenalist_release(&al);
}

RK_REGISTER_TEST("arenalist", test_arenalist_allocate_wrapper) {
  ArenaList al = arenalist_init(128);

  // arenalist_allocate is the typed-wrapper entry point used by the Allocator
  // interface — verify it behaves identically to arenalist_new
  void*     p = arenalist_allocate(sizeof(int) * 4, alignof(int), &al);
  rk_expect_nonnull(p);
  rk_expect_eq((uintptr_t)p % alignof(int), 0u);

  // Large alignment
  void* q = arenalist_allocate(sizeof(double), 64, &al);
  rk_expect_nonnull(q);
  rk_expect_eq((uintptr_t)q % 64u, 0u);

  arenalist_release(&al);
}

RK_REGISTER_TEST("arenalist", test_arenalist_lazy_init_zero_arena_size) {
  // An ArenaList with arena_size == 0 auto-initialises on first allocation
  ArenaList al = {RK_ZINIT};
  rk_expect_eq(al.arena_size, 0u);

  int* p = arenalist_new(int, 4, &al);
  rk_expect_nonnull(p);
  rk_expect_eq((uintptr_t)p % alignof(int), 0u);
  rk_expect_true(al.arena_size > 0u); // lazily initialised
  rk_expect_true(stdc_has_single_bit(al.arena_size));

  // Subsequent allocations work normally
  int* q = arenalist_new(int, 4, &al);
  rk_expect_nonnull(q);
  rk_expect_neq(p, q);

  arenalist_release(&al);
}

RK_REGISTER_TEST("arenalist", test_arenalist_release_after_growth) {
  ArenaList al = arenalist_init(64);

  rk_expect_nonnull(arenalist_new(char, 48, &al));
  rk_expect_nonnull(arenalist_new(char, 48, &al));
  rk_expect_nonnull(arenalist_new(char, 200, &al));
  rk_expect_true(vec_count(al.arenas) >= 3u);

  arenalist_release(&al);

  rk_expect_eq(al.arena_size, 0u);
  rk_expect_eq(al.cur, 0u);
}

RK__IGNWARN_CLANG_END()
RK_HEADER_END
