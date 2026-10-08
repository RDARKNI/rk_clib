#ifndef TESTARENASTACK_H
#define TESTARENASTACK_H
#include "conf.h"

RKI_HEADER_BEGIN
RKI_IGNWARN_CLANG_BEG("-Wunused-variable")

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

// A mark taken from a zero-initialized stack denotes its starting position: rewinding to it after
// lazy initialization discards everything allocated since.
triax_test(arenastack, mark_null_rewind_clears) {
  ArenaStack al        = {0};
  ArenaMark  null_mark = arenastack_mark(&al);
  triax_expect_null(null_mark.pos);

  triax_expect_nonnull(arenastack_new(char, 10, &al)); // lazily initializes
  triax_expect_nonnull(arenastack_new(char, 4096, &al)); // spills into a second arena
  triax_expect_true(vec_count(al.arenas) >= 2u);

  arenastack_rewind_to(&al, null_mark);
  triax_expect_eq(al.cur, 0u);
  triax_expect_true(arena_is_empty(&al.arenas[0]));

  arenastack_release(&al);
}

triax_test(arenastack, mark_null_rewind_on_zero_initialized_is_noop) {
  ArenaStack al = {0};
  arenastack_rewind_to(&al, arenastack_mark(&al));
  triax_expect_eq(al.arena_size, 0u);
  triax_expect_eq(vec_count(al.arenas), 0u);
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
  ArenaStack al = {RKI_ZINIT};
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

// ---- try_resize_top / try_extend ----

triax_test(arenastack, try_extend_top_grow_shrink_zero) {
  ArenaStack al = arenastack_init(64);
  int*       a  = arenastack_new(int, 4, &al);
  fill_ints_al(a, 4, 7);
  triax_expect_eq(arenastack_try_extend(a, 4, 8, &al), a);
  expect_ints_al(a, 4, 7);
  triax_expect_eq(arenastack_try_extend(a, 8, 2, &al), a);
  expect_ints_al(a, 2, 7);
  triax_expect_eq(arenastack_try_extend(a, 2, 0, &al), a);
  triax_expect_eq(al.arenas[al.cur].cur, (unsigned char*)a);
  arenastack_release(&al);
}

// A top allocation that cannot grow in the active arena fails without moving data, adding an
// arena, or touching the cursor.
triax_test(arenastack, try_extend_too_large_fails_and_leaves_cursor) {
  ArenaStack al = arenastack_init(64);
  int*       a  = arenastack_new(int, 4, &al);
  fill_ints_al(a, 4, 7);
  unsigned char* cur0   = al.arenas[al.cur].cur;
  size_t         arenas = vec_count(al.arenas);
  triax_expect_null(arenastack_try_extend(a, 4, 1000, &al));
  triax_expect_eq(al.arenas[al.cur].cur, cur0);
  triax_expect_eq(vec_count(al.arenas), arenas);
  expect_ints_al(a, 4, 7);
  triax_expect_null(arenastack_try_resize_top(16, 1000, &al));
  triax_expect_eq(al.arenas[al.cur].cur, cur0);
  arenastack_release(&al);
}

triax_test(arenastack, try_resize_top_resizes_active_top) {
  ArenaStack al = arenastack_init(64);
  char*      a  = arenastack_new(char, 8, &al);
  triax_expect_eq(arenastack_try_resize_top(8, 20, &al), a);
  triax_expect_eq(al.arenas[al.cur].cur, (unsigned char*)a + 20);
  triax_expect_eq(arenastack_try_resize_top(20, 0, &al), a);
  triax_expect_eq(al.arenas[al.cur].cur, (unsigned char*)a);
  arenastack_release(&al);
}

triax_test(arenastack, try_resize_top_on_zero_initialized_fails) {
  ArenaStack al = {0};
  triax_expect_null(arenastack_try_resize_top(0, 8, &al));
  triax_expect_null(arenastack_try_resize_top(0, 0, &al));
  triax_expect_eq(al.arena_size, 0u);
}

// A non-top allocation is an ordinary try_extend failure, as for arena_try_extend(): NULL, with
// the stack and data untouched, for growth, shrinking, and resizing to zero.
triax_test(arenastack, try_extend_non_top_returns_null_and_leaves_stack) {
  ArenaStack al = arenastack_init(64);
  int*       a  = arenastack_new(int, 4, &al);
  fill_ints_al(a, 4, 7);
  int*           b    = arenastack_new(int, 4, &al); // a is no longer top
  unsigned char* cur0 = al.arenas[al.cur].cur;
  triax_expect_null(arenastack_try_extend(a, 4, 8, &al));
  triax_expect_null(arenastack_try_extend(a, 4, 2, &al));
  triax_expect_null(arenastack_try_extend(a, 4, 0, &al));
  triax_expect_eq(al.arenas[al.cur].cur, cur0);
  expect_ints_al(a, 4, 7);
  triax_expect_eq(arenastack_try_extend(b, 4, 6, &al), b); // the real top still resizes
  arenastack_release(&al);
}

// The top of an earlier arena is not the stack's top once the stack has grown.
triax_test(arenastack, try_extend_top_of_earlier_arena_returns_null) {
  ArenaStack al    = arenastack_init(64);
  int*       a     = arenastack_new(int, 4, &al); // top of the first arena
  size_t     first = al.cur;
  (void)arenastack_new(int, 64, &al); // spills into a new active arena
  triax_assert_true(al.cur != first);
  unsigned char* cur_first  = al.arenas[first].cur;
  unsigned char* cur_active = al.arenas[al.cur].cur;
  triax_expect_null(arenastack_try_extend(a, 4, 0, &al));
  triax_expect_null(arenastack_try_extend(a, 4, 8, &al));
  triax_expect_eq(al.arenas[first].cur, cur_first);
  triax_expect_eq(al.arenas[al.cur].cur, cur_active);
  arenastack_release(&al);
}

// Byte-sized arenastack_try_resize(): same semantics as the typed arenastack_try_extend().
triax_test(arenastack, try_resize_bytes_top_and_non_top) {
  ArenaStack     al   = arenastack_init(64);
  unsigned char* a    = (unsigned char*)arenastack_allocate(8, 1, &al);
  unsigned char* b    = (unsigned char*)arenastack_allocate(8, 1, &al);
  unsigned char* cur0 = al.arenas[al.cur].cur;

  triax_expect_null(arenastack_try_resize(a, 8, 16, &al)); // not top
  triax_expect_null(arenastack_try_resize(a, 8, 0, &al));
  triax_expect_eq(al.arenas[al.cur].cur, cur0);

  triax_expect_eq(arenastack_try_resize(b, 8, 20, &al), b); // grow
  triax_expect_eq(arenastack_try_resize(b, 20, 4, &al), b); // shrink
  triax_expect_eq(al.arenas[al.cur].cur, b + 4);
  size_t arenas = vec_count(al.arenas);
  triax_expect_null(arenastack_try_resize(b, 4, 1000, &al)); // does not fit; no new arena
  triax_expect_eq(vec_count(al.arenas), arenas);
  triax_expect_eq(al.arenas[al.cur].cur, b + 4);

  ArenaStack zero = {0};
  int        x    = 0;
  triax_expect_null(arenastack_try_resize(&x, sizeof x, 0, &zero));
  arenastack_release(&al);
}

triax_test(arenastack, try_extend_on_zero_initialized_returns_null) {
  ArenaStack al = {0};
  int        x  = 0;
  triax_expect_null(arenastack_try_extend(&x, 1, 0, &al));
  triax_expect_eq(al.arena_size, 0u);
}

// ---- allocator interface ----

#if RK_CUSTOM_ALLOCATORS
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

// With a bump allocator as backing, consecutive arena buffers can be adjacent. When the active
// arena is empty, the previous arena's top allocation then also ends at the active cursor; it must
// not be treated as the active arena's top allocation.
static unsigned char arenastack_adjacent_backing[1 << 12];
triax_test(arenastack, adjacent_buffers_foreign_top_not_reclaimed) {
  Arena      back = arena_init(arenastack_adjacent_backing, sizeof arenastack_adjacent_backing);
  ArenaStack al   = arenastack_init(16, arena_to_alloc(&back));
  Allocator  a    = arenastack_to_alloc(&al);
  void*      p[6];
  // One full arena each; once the descriptor vector has spare capacity, buffers become adjacent.
  for (int i = 0; i < 6; ++i) { p[i] = alloc_allocate(16, 1, a); }
  void* q = alloc_allocate(8, 1, a);

  Arena* active = &al.arenas[al.cur];
  Arena* prev   = &al.arenas[al.cur - 1];
  triax_assert_true(prev->end == active->beg); // precondition for this scenario

  alloc_deallocate(q, 8, 1, a); // active arena is now empty
  triax_expect_true(arena_is_empty(active));
  alloc_deallocate(p[5], 16, 1, a); // foreign top: reclaims nothing
  triax_expect_true(arena_is_empty(active));
  triax_expect_eq(prev->cur, prev->end);

  // Growth must move rather than extend across the arena boundary. (May add an arena and move
  // the descriptor vector, so `active`/`prev` are not used past this point.)
  void* grown = alloc_reallocate(p[5], 16, 24, 1, a);
  triax_expect_neq(grown, p[5]);
  vec_foreach(al.arenas, ar) {
    triax_expect_true(ar->beg <= ar->cur && ar->cur <= ar->end);
  }
  arenastack_release(&al);
}

// Same adjacent-buffer setup: the previous arena's top is not the stack's top for
// arenastack_try_extend() either, even though it ends exactly at the empty active cursor.
triax_test(arenastack, adjacent_buffers_try_extend_foreign_top_returns_null) {
  Arena      back = arena_init(arenastack_adjacent_backing, sizeof arenastack_adjacent_backing);
  ArenaStack al   = arenastack_init(16, arena_to_alloc(&back));
  Allocator  a    = arenastack_to_alloc(&al);
  char*      p[6];
  for (int i = 0; i < 6; ++i) { p[i] = (char*)alloc_allocate(16, 1, a); }
  void* q = alloc_allocate(8, 1, a);
  Arena* active = &al.arenas[al.cur];
  Arena* prev   = &al.arenas[al.cur - 1];
  triax_assert_true(prev->end == active->beg);
  alloc_deallocate(q, 8, 1, a); // active arena is now empty
  triax_expect_null(arenastack_try_extend(p[5], 16, 0, &al));
  triax_expect_null(arenastack_try_extend(p[5], 16, 24, &al));
  triax_expect_true(arena_is_empty(active));
  triax_expect_eq(prev->cur, prev->end);
  arenastack_release(&al);
}
#endif

RKI_IGNWARN_CLANG_END()
RKI_HEADER_END

#endif
