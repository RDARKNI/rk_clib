#ifndef TESTARENASTACK_H
#define TESTARENASTACK_H
#include "conf.h"

RK_HEADER_BEGIN
RK__IGNWARN_CLANG_BEG("-Wunused-variable")

#define ALIST_ARENA_SIZE 128

static void fill_ints_al(int* p, int n, int base) {
  for (int i = 0; i < n; ++i) { p[i] = base + i; }
}

static void expect_ints_al(const int* p, int n, int base) {
  for (int i = 0; i < n; ++i) { triax_expect_eq(p[i], base + i); }
}

// ---- init ----
triax_test(arenastack, init_basic) {
  ArenaStack al = arenastack_init(ALIST_ARENA_SIZE);

  triax_expect_eq(al.cur, 0u);
  triax_expect_true(al.arena_size >= ALIST_ARENA_SIZE);
  triax_expect_true(stdc_has_single_bit(al.arena_size));
  triax_expect_eq(vec_count(al.arenas), 1u);

  triax_expect_nonnull(al.arenas[0].beg);
  triax_expect_eq(al.arenas[0].cur, al.arenas[0].beg);
  triax_expect_eq(arena_cap(&al.arenas[0]), al.arena_size);
  triax_expect_eq(arena_used(&al.arenas[0]), 0u);
  triax_expect_eq(arena_remaining(&al.arenas[0]), al.arena_size);

  arenastack_release(&al);
  triax_expect_eq(al.arena_size, 0u);
  triax_expect_eq(al.cur, 0u);
}

triax_test(arenastack, init_rounds_to_pow2) {
  ArenaStack al = arenastack_init(100);

  triax_expect_true(stdc_has_single_bit(al.arena_size));
  triax_expect_true(al.arena_size >= 100u);
  triax_expect_eq(arena_cap(&al.arenas[0]), al.arena_size);

  arenastack_release(&al);
}

// ---- new / new_aligned ----

triax_test(arenastack, new_basic) {
  ArenaStack al = arenastack_init(ALIST_ARENA_SIZE);

  int*       p  = arenastack_new(int, 10, &al);
  triax_expect_nonnull(p);
  triax_expect_eq((uintptr_t)p % alignof(int), 0u);
  fill_ints_al(p, 10, 0);
  expect_ints_al(p, 10, 0);

  triax_expect_eq(al.cur, 0u);
  triax_expect_eq(vec_count(al.arenas), 1u);
  triax_expect_true(arena_used(&al.arenas[0]) >= sizeof(int) * 10u);

  arenastack_release(&al);
}

triax_test(arenastack, new_zero_count) {
  ArenaStack al = arenastack_init(ALIST_ARENA_SIZE);

  int*       p  = arenastack_new(int, 0, &al);
  triax_expect_nonnull(p);
  triax_expect_eq((uintptr_t)p % alignof(int), 0u);

  arenastack_release(&al);
}

triax_test(arenastack, new_aligned) {
  ArenaStack al = arenastack_init(ALIST_ARENA_SIZE);

  int*       p  = arenastack_new_aligned(int, 3, 32, &al);
  triax_expect_nonnull(p);
  triax_expect_eq((uintptr_t)p % 32u, 0u);

  double* q = arenastack_new_aligned(double, 2, 64, &al);
  triax_expect_nonnull(q);
  triax_expect_eq((uintptr_t)q % 64u, 0u);

  arenastack_release(&al);
}

triax_test(arenastack, spills_into_multiple_arenas) {
  ArenaStack al = arenastack_init(64);

  int*       p1 = arenastack_new(int, 8, &al);
  triax_expect_nonnull(p1);
  fill_ints_al(p1, 8, 0);

  int* p2 = arenastack_new(int, 16, &al);
  triax_expect_nonnull(p2);
  fill_ints_al(p2, 16, 100);

  triax_expect_true(vec_count(al.arenas) >= 2u);
  triax_expect_true(al.cur >= 1u);

  expect_ints_al(p1, 8, 0);
  expect_ints_al(p2, 16, 100);

  arenastack_release(&al);
}

triax_test(arenastack, large_alloc_grows_arena_size) {
  ArenaStack al = arenastack_init(64);

  triax_expect_eq(al.arena_size, 64u);

  char* p = arenastack_new(char, 200, &al);
  triax_expect_nonnull(p);

  triax_expect_true(vec_count(al.arenas) >= 2u);
  triax_expect_true(arena_cap(&al.arenas[al.cur]) >= 200u);

  arenastack_release(&al);
}

// ---- clear ----

triax_test(arenastack, clear_single_arena) {
  ArenaStack al = arenastack_init(ALIST_ARENA_SIZE);

  triax_expect_nonnull(arenastack_new(int, 10, &al));
  triax_expect_nonnull(arenastack_new(char, 7, &al));

  triax_expect_eq(arenastack_clear(&al), &al);
  triax_expect_eq(al.cur, 0u);
  triax_expect_eq(arena_used(&al.arenas[0]), 0u);
  triax_expect_true(arena_is_empty(&al.arenas[0]));

  arenastack_release(&al);
}

triax_test(arenastack, clear_multiple_arenas) {
  ArenaStack al = arenastack_init(64);

  triax_expect_nonnull(arenastack_new(char, 48, &al));
  triax_expect_nonnull(arenastack_new(char, 48, &al));
  triax_expect_nonnull(arenastack_new(char, 48, &al));

  triax_expect_true(vec_count(al.arenas) >= 3u);
  triax_expect_true(al.cur >= 2u);

  triax_expect_eq(arenastack_clear(&al), &al);
  triax_expect_eq(al.cur, 0u);

  for (size_t i = 0; i < vec_count(al.arenas); ++i) {
    triax_expect_eq(arena_used(&al.arenas[i]), 0u);
    triax_expect_true(arena_is_empty(&al.arenas[i]));
  }

  arenastack_release(&al);
}

// ---- mark / rewind_to ----

triax_test(arenastack, mark_rewind_same_arena) {
  ArenaStack al = arenastack_init(ALIST_ARENA_SIZE);

  char*      p1 = arenastack_new(char, 16, &al);
  ArenaMark  m  = arenastack_mark(&al);
  char*      p2 = arenastack_new(char, 24, &al);
  char*      p3 = arenastack_new(char, 8, &al);
  triax_expect_nonnull(p1);
  triax_expect_nonnull(p2);
  triax_expect_nonnull(p3);

  triax_expect_eq(arenastack_rewind_to(&al, m), &al);
  triax_expect_eq(al.cur, 0u);
  triax_expect_eq(al.arenas[0].cur, m.pos);

  arenastack_release(&al);
}

triax_test(arenastack, mark_rewind_previous_arena) {
  ArenaStack al = arenastack_init(64);

  char*      p1 = arenastack_new(char, 48, &al);
  ArenaMark  m  = arenastack_mark(&al); // mark after p1, in arena 0
  char*      p2 = arenastack_new(char, 48, &al);
  char*      p3 = arenastack_new(char, 48, &al);
  triax_expect_nonnull(p1);
  triax_expect_nonnull(p2);
  triax_expect_nonnull(p3);

  triax_expect_true(al.cur >= 2u);

  triax_expect_eq(arenastack_rewind_to(&al, m), &al);
  triax_expect_eq(al.cur, 0u);
  triax_expect_eq(al.arenas[0].cur, m.pos);

  for (size_t i = 1; i < vec_count(al.arenas); ++i) {
    triax_expect_true(arena_is_empty(&al.arenas[i]));
  }

  arenastack_release(&al);
}

triax_test(arenastack, mark_rewind_roundtrip) {
  ArenaStack al   = arenastack_init(64);

  char*      p1   = arenastack_new(char, 32, &al);
  ArenaMark  mark = arenastack_mark(&al);
  char*      p2   = arenastack_new(char, 48, &al); // spills to next arena
  triax_expect_nonnull(p1);
  triax_expect_nonnull(p2);

  size_t arenas_after = vec_count(al.arenas);
  triax_expect_true(arenas_after >= 2u);

  arenastack_rewind_to(&al, mark);
  triax_expect_eq(al.cur, 0u);
  triax_expect_eq(al.arenas[0].cur, mark.pos);

  // re-allocate same amount — arenas reused, no new allocations
  char* p3 = arenastack_new(char, 48, &al);
  triax_expect_nonnull(p3);
  triax_expect_eq(vec_count(al.arenas), arenas_after);

  arenastack_release(&al);
}

triax_test(arenastack, mark_null_rewind_noop) {
  ArenaStack al        = arenastack_init(ALIST_ARENA_SIZE);
  ArenaMark  null_mark = {rk_null};

  triax_expect_nonnull(arenastack_new(char, 10, &al));
  size_t used = arena_used(&al.arenas[0]);

  arenastack_rewind_to(&al, null_mark); // should be a no-op
  triax_expect_eq(arena_used(&al.arenas[0]), used);

  arenastack_release(&al);
}

// ---- reuse / stress ----

triax_test(arenastack, reuse_after_clear) {
  ArenaStack al = arenastack_init(64);

  for (int round = 0; round < 16; ++round) {
    int* p = arenastack_new(int, 8, &al);
    triax_expect_nonnull(p);
    fill_ints_al(p, 8, round * 10);
    expect_ints_al(p, 8, round * 10);

    triax_expect_eq(arenastack_clear(&al), &al);
    triax_expect_eq(al.cur, 0u);
    for (size_t i = 0; i < vec_count(al.arenas); ++i) {
      triax_expect_true(arena_is_empty(&al.arenas[i]));
    }
  }

  arenastack_release(&al);
}

triax_test(arenastack, multiple_type_patterns) {
  ArenaStack al = arenastack_init(128);

  for (int round = 0; round < 8; ++round) {
    for (int* p; (p = arena_try_new(int, 10, &al.arenas[al.cur]));) {
      triax_expect_nonnull(p);
      for (int i = 0; i < 10; ++i) {
        p[i] = i;
        triax_expect_eq(p[i], i);
      }
    }
    arenastack_clear(&al);

    for (double* p; (p = arena_try_new(double, 8, &al.arenas[al.cur]));) {
      triax_expect_nonnull(p);
      for (int i = 0; i < 8; ++i) {
        p[i] = (double)i;
        triax_expect_eq(p[i], (double)i);
      }
    }
    arenastack_clear(&al);

    for (short* p; (p = arena_try_new(short, 12, &al.arenas[al.cur]));) {
      triax_expect_nonnull(p);
      for (short i = 0; i < 12; ++i) {
        p[i] = i;
        triax_expect_eq(p[i], i);
      }
    }
    arenastack_clear(&al);
  }

  arenastack_release(&al);
}

triax_test(arenastack, allocate_wrapper) {
  ArenaStack al = arenastack_init(128);

  // arenastack_allocate is the typed-wrapper entry point used by the Allocator
  // interface — verify it behaves identically to arenastack_new
  void*      p = arenastack_allocate(sizeof(int) * 4, alignof(int), &al);
  triax_expect_nonnull(p);
  triax_expect_eq((uintptr_t)p % alignof(int), 0u);

  // Large alignment
  void* q = arenastack_allocate(sizeof(double), 64, &al);
  triax_expect_nonnull(q);
  triax_expect_eq((uintptr_t)q % 64u, 0u);

  arenastack_release(&al);
}

triax_test(arenastack, lazy_init_zero_arena_size) {
  // An ArenaStack with arena_size == 0 auto-initialises on first allocation
  ArenaStack al = {RK_ZINIT};
  triax_expect_eq(al.arena_size, 0u);

  int* p = arenastack_new(int, 4, &al);
  triax_expect_nonnull(p);
  triax_expect_eq((uintptr_t)p % alignof(int), 0u);
  triax_expect_true(al.arena_size > 0u); // lazily initialised
  triax_expect_true(stdc_has_single_bit(al.arena_size));

  // Subsequent allocations work normally
  int* q = arenastack_new(int, 4, &al);
  triax_expect_nonnull(q);
  triax_expect_neq(p, q);

  arenastack_release(&al);
}

triax_test(arenastack, release_after_growth) {
  ArenaStack al = arenastack_init(64);

  triax_expect_nonnull(arenastack_new(char, 48, &al));
  triax_expect_nonnull(arenastack_new(char, 48, &al));
  triax_expect_nonnull(arenastack_new(char, 200, &al));
  triax_expect_true(vec_count(al.arenas) >= 3u);

  arenastack_release(&al);

  triax_expect_eq(al.arena_size, 0u);
  triax_expect_eq(al.cur, 0u);
}

// ---- allocator interface ----

#if RK_ALLOCMODE != RK_ALLOCMODE_MALLOC_ONLY
triax_test(arenastack, to_alloc_basic) {
  ArenaStack al = arenastack_init(128);
  SWAP_ALLOC(arenastack_to_alloc(&al));
  int* arr = alloc_new(int, 10);
  triax_expect_nonnull(arr);
  fill_ints_al(arr, 10, 0);
  expect_ints_al(arr, 10, 0);

  int* arr2 = alloc_renew(arr, 10, 20);
  triax_expect_eq(arr2, arr); // top — in-place
  expect_ints_al(arr2, 10, 0);
  alloc_delete(arr2, 20);
  arenastack_release(&al);
  rk_SWAP(alloc_cpy, alloc_ctx);
}

triax_test(arenastack, allocator_deallocate_top_only) {
  ArenaStack al = arenastack_init(128);
  SWAP_ALLOC(arenastack_to_alloc(&al));
  int* p1 = alloc_new(int, 4);
  int* p2 = alloc_new(int, 4);
  triax_expect_nonnull(p1);
  triax_expect_nonnull(p2);

  unsigned char* top_after_p2 = al.arenas[al.cur].cur;
  alloc_delete(p1, 4);
  triax_expect_eq(al.arenas[al.cur].cur, top_after_p2); // non-top: no-op

  alloc_delete(p2, 4);
  triax_expect_eq(al.arenas[al.cur].cur, (unsigned char*)p2); // top: rewound

  arenastack_release(&al);
  rk_SWAP(alloc_cpy, alloc_ctx);
}

triax_test(arenastack, allocator_realloc_copy_path) {
  ArenaStack al = arenastack_init(128);
  SWAP_ALLOC(arenastack_to_alloc(&al));
  int* p = alloc_new(int, 5);
  triax_expect_nonnull(p);
  fill_ints_al(p, 5, 100);

  triax_expect_nonnull(alloc_new(int, 10)); // displace top

  int* q = alloc_renew(p, 5, 12);
  triax_expect_nonnull(q);
  triax_expect_neq(q, p); // not top — allocate and copy
  expect_ints_al(q, 5, 100);
  arenastack_release(&al);
  rk_SWAP(alloc_cpy, alloc_ctx);
}
#endif

RK__IGNWARN_CLANG_END()
RK_HEADER_END

#endif
