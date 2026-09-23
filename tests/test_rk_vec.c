#ifndef TEST_VEC_H
#define TEST_VEC_H
#include "conf.h"

RK__IGNWARN_CLANG_BEG("-Wunused-variable")

static unsigned char vec_storage[102400];
static Arena         MYARENA          = arena_init_static(vec_storage);
static Allocator     glob_arena_alloc = arena_to_alloc_static(&MYARENA);

triax_test(vec, vec_init) {
  Allocator used_alloc = alloc_ctx;
  Vec(int)  v;
  {
    v = vec_init(int, 4);
    triax_expect_nonnull(v);
    RK_IFALLOC(triax_expect_memeq((Allocator[]){vec_allocator(v)}, &alloc_ctx, sizeof(alloc_ctx));)
    triax_expect_eq(vec_allocation_size(v), sizeof(VecHeader) + sizeof(int) * 4);
    triax_expect_eq(vec_count(v), 0), triax_expect_true(vec_is_empty(v));
    vec_push(v, 99);
    triax_expect_eq(vec_count(v), 1u), triax_expect_false(vec_is_empty(v));
    vec_pop(v);
    triax_expect_eq(vec_count(v), 0), triax_expect_true(vec_is_empty(v));
    triax_expect_eq(vec_cap(v), 4);
    triax_expect_eq(vec_allocation_size(v), sizeof(VecHeader) + sizeof(int) * 4);
    vec_release(v);
    triax_expect_null(v);
  }
  {
#if RK_ALLOCMODE == RK_ALLOCMODE_FULL
    Allocator used_alloc = glob_arena_alloc;
#endif
    v = vec_init(int, 4 RK_IFALLOC(, used_alloc));
    triax_expect_nonnull(v);
    RK_IFALLOC(
        triax_expect_memeq((Allocator[]){vec_allocator(v)}, &used_alloc, sizeof(used_alloc));)
    triax_expect_eq(vec_allocation_size(v), sizeof(VecHeader) + sizeof(int) * 4);
    triax_expect_eq(vec_count(v), 0), triax_expect_true(vec_is_empty(v));
    vec_push(v, 99);
    triax_expect_eq(vec_count(v), 1u), triax_expect_false(vec_is_empty(v));
    vec_pop(v);
    triax_expect_eq(vec_count(v), 0), triax_expect_true(vec_is_empty(v));
    triax_expect_eq(vec_cap(v), 4);
    triax_expect_eq(vec_allocation_size(v), sizeof(VecHeader) + sizeof(int) * 4);
    vec_release(v);
    triax_expect_null(v);
  }
}

triax_test(vec, init_list) {
  Allocator used_alloc = alloc_ctx;
  Vec(int)  v;
  {
    v = vec_init_list(int, 0, 1, 2, 3, 4, 5);
    triax_expect_nonnull(v);
    RK_IFALLOC(
        triax_expect_memeq((Allocator[]){vec_allocator(v)}, &used_alloc, sizeof(used_alloc));)
    triax_expect_eq(vec_count(v), 6);
    triax_expect_eq(vec_allocation_size(v), sizeof(VecHeader) + sizeof(int) * vec_count(v));
    triax_expect_true(vec_cap(v) >= vec_count(v));
    for (int i = 0; i < 6; ++i) { triax_expect_eq(v[i], i); }
    vec_release(v);
  }
  {
#if RK_ALLOCMODE == RK_ALLOCMODE_FULL
    Allocator used_alloc = glob_arena_alloc;
#endif
    v = vec_init_list(int, RK_IFALLOC(used_alloc, ) 0, 1, 2, 3, 4, 5);
    triax_expect_nonnull(v);
    RK_IFALLOC(
        triax_expect_memeq((Allocator[]){vec_allocator(v)}, &used_alloc, sizeof(used_alloc));)
    triax_expect_eq(vec_count(v), 6);
    triax_expect_eq(vec_allocation_size(v), sizeof(VecHeader) + sizeof(int) * vec_count(v));
    triax_expect_true(vec_cap(v) >= vec_count(v));
    for (int i = 0; i < 6; ++i) { triax_expect_eq(v[i], i); }
    vec_release(v);
  }
}

triax_test(vec, vec_copy) {
  Allocator used_alloc = alloc_ctx;
  Vec(int)  v;
  {
#if RK_ALLOCMODE == RK_ALLOCMODE_FULL
    Allocator used_alloc = glob_arena_alloc;
#endif
    v           = vec_init_list(int, RK_IFALLOC(used_alloc, ) 0, 1, 2, 3, 4, 5);
    Vec(int) v2 = vec_copy(v);
    triax_expect_eq(vec_count(v2), vec_count(v));
    triax_expect_memeq(v, v2, vec_count(v2) * sizeof(int));
    vec_release(v2);
    vec_release(v);
  }
  // todo copy semantics allocators
}

triax_test(vec, vec_push_pop) {
  enum { EL_COUNT = 128 };
  Vec(int) v;
  {
    v = vec_init(int, 4);
    vec_push(v, 10);
    vec_push(v, 20);
    triax_expect_eq(vec_count(v), 2);
    triax_expect_eq(vec_pop(v), 20);
    triax_expect_eq(vec_pop(v), 10);
    for (int i = 0; i < EL_COUNT; ++i) { vec_push(v, i); }
    triax_expect_eq(vec_count(v), EL_COUNT);
    for (int i = 0; i < EL_COUNT; ++i) {
      triax_expect_eq(v[i], i);
      triax_expect_eq(vec_pop(v), EL_COUNT - i - 1);
    }
    triax_expect_eq(vec_count(v), 0);
    vec_release(v);
  }
  {
    v = rk_null;
    vec_push(v, 10), vec_push(v, 20);
    triax_expect_eq(vec_count(v), 2);
    int v1 = vec_pop(v), v2 = vec_pop(v);
    triax_expect_eq(v1, 20), triax_expect_eq(v2, 10);
    for (int i = 0; i < EL_COUNT; ++i) { vec_push(v, i); }
    triax_expect_eq(vec_count(v), EL_COUNT);
    for (int i = 0; i < EL_COUNT; ++i) {
      triax_expect_eq(v[i], i);
      triax_expect_eq(vec_pop(v), EL_COUNT - i - 1);
    }
    triax_expect_eq(vec_count(v), 0);
    vec_release(v);
  }
}

triax_test(vec, vec_insert_erase) {
  Vec(int) v = vec_init_list(int, 1, 2, 3);
  vec_insert_at(v, 1, 42);
  triax_expect_eq(v[1], 42);
  vec_erase_at(v, 1);
  triax_expect_eq(v[1], 2);
  vec_release(v);
}

triax_test(vec, vec_foreach) {
  Vec(int) v = vec_init_list(int, 0, 1, 2, 3, 4);
  triax_expect_eq(vec_count(v), 5);
  for (int i = 0; i < 5; ++i) { triax_expect_eq(v[i], i); }
  int sum = 0;
  vec_foreach(v, it) { sum += *it; }
  triax_expect_eq(sum, 10);
  vec_release(v);
}

triax_test(vec, vec_reverse) {
  Vec(int) v;
  {
    v = rk_null;
    vec_reverse(v);
    vec_release(v);
  }

  {
    v = vec_init_list(int, 1, 2, 3, 4, 5);
    vec_reverse(v);
    triax_expect_eq(v[0], 5);
    triax_expect_eq(v[1], 4);
    triax_expect_eq(v[2], 3);
    triax_expect_eq(v[3], 2);
    triax_expect_eq(v[4], 1);
    vec_release(v);
  }
  {
    v = vec_init_list(int, 1, 2, 3, 4);
    vec_reverse(v);
    triax_expect_eq(v[0], 4);
    triax_expect_eq(v[1], 3);
    triax_expect_eq(v[2], 2);
    triax_expect_eq(v[3], 1);
    vec_release(v);
  }
}

triax_test(vec, vec_index_in_range) {
  Vec(int) v = vec_init_list(int, 10, 20, 30);
  triax_expect_true(vec_index_in_range(v, 0));
  triax_expect_true(vec_index_in_range(v, 2));
  triax_expect_false(vec_index_in_range(v, 3));
  vec_release(v);
}

triax_test(vec, vec_push_n_pop_n) {
  Vec(int) v;
  {
    v = vec_init(int, 5);
    vec_push_n(v, ((int[]){1, 2, 3}), 3);
    triax_expect_eq(vec_count(v), 3);
    int* p = vec_pop_n(v, 2);
    triax_expect_eq(p[0], 2), triax_expect_eq(p[1], 3);
    triax_expect_eq(vec_count(v), 1);
    p[0] = 1, p[1] = 1;
    vec_push_n(v, p, 2);
    triax_expect_eq(vec_count(v), 3);
    triax_expect_eq(v[0], 1), triax_expect_eq(v[1], 1), triax_expect_eq(v[2], 1);
    p = vec_pop_n(v, 3);
    triax_expect_eq(p[0], 1), triax_expect_eq(p[1], 1), triax_expect_eq(p[2], 1);
    triax_expect_eq(vec_count(v), 0);
    vec_release(v);
  }
  {
    v = rk_null;
    vec_push_n(v, ((int[]){1, 2, 3}), 3);
    triax_expect_eq(vec_count(v), 3);
    int* p = vec_pop_n(v, 2);
    triax_expect_eq(p[0], 2), triax_expect_eq(p[1], 3);
    triax_expect_eq(vec_count(v), 1);
    p[0] = 1, p[1] = 1;
    vec_push_n(v, p, 2);
    triax_expect_eq(vec_count(v), 3);
    triax_expect_eq(v[0], 1), triax_expect_eq(v[1], 1), triax_expect_eq(v[2], 1);
    p = vec_pop_n(v, 3);
    triax_expect_eq(p[0], 1), triax_expect_eq(p[1], 1), triax_expect_eq(p[2], 1);
    triax_expect_eq(vec_count(v), 0);
    vec_release(v);
  }
}

triax_test(vec, vec_front_back) {
  Vec(int) v = vec_init_list(int, 5, 10, 15);
  triax_expect_eq(vec_front(v), 5);
  triax_expect_eq(vec_back(v), 15);
  vec_release(v);
}

triax_test(vec, vec_insert_arr_erase_n) {
  Vec(int) v     = vec_init(int, 5);
  int      arr[] = {10, 20, 30};
  vec_insert_arr_at(v, 0, arr, 3);
  triax_expect_eq(vec_count(v), 3);
  vec_erase_at_n(v, 0, 2);
  triax_expect_eq(vec_count(v), 1);
  triax_expect_eq(v[0], 30);
  vec_release(v);
}

triax_test(vec, vec_clear_resize_reserve) {
  Vec(int) v = vec_init_list(int, 1, 2, 3);
  vec_clear(v);
  triax_expect_eq(vec_count(v), 0);
  vec_resize(v, 5);
  triax_expect_eq(vec_count(v), 5);
  vec_reserve(v, 10);
  triax_expect_true(vec_cap(v) >= 10);
  vec_release(v);
}

triax_test(vec, vec_reverse_foreach) {
  Vec(int) v = vec_init_list(int, 1, 2, 3, 4);
  vec_reverse(v);
  int    expected[] = {4, 3, 2, 1};
  size_t i          = 0;
  vec_foreach(v, it) { triax_expect_eq(*it, expected[i++]); }
  vec_release(v);
}

triax_test(vec, vec_remaining) {
  Vec(int) v   = vec_init(int, 4);
  size_t   cap = vec_cap(v);
  triax_expect_eq(cap, 4);
  triax_expect_eq(vec_remaining(v), 4);
  vec_push(v, 1);
  triax_expect_eq(vec_remaining(v), 3);
  vec_release(v);
}
typedef struct DummyStruct { int x, y; } DummyStruct;

static inline DummyStruct dummystructret(void) { return (DummyStruct){0, 0}; }
triax_test(vec, vec_compound) {
  Vec(DummyStruct) v = vec_init(DummyStruct, 4);
  vec_push(v, ((DummyStruct){1, 2}));
  vec_push(v, ((DummyStruct){3, 4}));
  vec_push(v, dummystructret());
  vec_push(v, ((void)(DummyStruct){3, 4}, (DummyStruct){3, 4}));

  triax_expect_eq(vec_remaining(v), 0);
  vec_release(v);
}

triax_test(vec, veceraseif) {
  Vec(int) v = vec_init_list(int, 45, 5);
  triax_assert_eq(v[0], 45);
  triax_assert_eq(v[1], 5);
  vec_erase_if(v, ot, *ot == 5);
  triax_assert_eq(vec_count(v), 1);
  vec_release(v);
}

triax_test(vec, vec_push, .skip = 1) {
  Vec(int) v = 0;
  for (int i = 0; i < 2540000; ++i) {
    vec_push(v, i * 3);
    triax_assert_eq(v[i], i * 3);
  }
}

triax_test(vec, vec_insert_at_unordered) {
  // Insert into the middle — displaced element goes to back
  Vec(int) v = vec_init_list(int, 1, 2, 3, 4);
  vec_insert_at_unordered(v, 1, 99);
  triax_expect_eq(vec_count(v), 5u);
  triax_expect_eq(v[1], 99); // new element at idx
  triax_expect_eq(v[4], 2);  // displaced element at back
  triax_expect_eq(v[0], 1);
  triax_expect_eq(v[2], 3);
  triax_expect_eq(v[3], 4);

  // Insert at the end — behaves like push, no displacement
  vec_insert_at_unordered(v, vec_count(v), 77);
  triax_expect_eq(vec_count(v), 6u);
  triax_expect_eq(v[5], 77);

  // Insert at front
  vec_insert_at_unordered(v, 0, 55);
  triax_expect_eq(vec_count(v), 7u);
  triax_expect_eq(v[0], 55);

  vec_release(v);
}

triax_test(vec, vec_erase_at_unordered) {
  // Erase from middle — last element fills the gap
  Vec(int) v = vec_init_list(int, 10, 20, 30, 40, 50);
  vec_erase_at_unordered(v, 1);
  triax_expect_eq(vec_count(v), 4u);
  triax_expect_eq(v[1], 50); // last element moved into erased slot
  triax_expect_eq(v[0], 10);
  triax_expect_eq(v[2], 30);
  triax_expect_eq(v[3], 40);
  // Erase last element — simple shrink, no swap
  vec_erase_at_unordered(v, 3);
  triax_expect_eq(vec_count(v), 3u);
  triax_expect_eq(v[2], 30);
  // Erase first element
  vec_erase_at_unordered(v, 0);
  triax_expect_eq(vec_count(v), 2u);

  vec_release(v);
}

triax_test(vec, vec_foreach_reversed) {
  Vec(int) v          = vec_init_list(int, 1, 2, 3, 4, 5);
  int      expected[] = {5, 4, 3, 2, 1};
  size_t   i          = 0;
  vec_foreach_reversed(v, it) {
    triax_expect_eq(*it, expected[i]);
    ++i;
  }
  triax_expect_eq(i, 5u);
  vec_release(v);
}

triax_test(vec, vec_foreach_reversed_empty) {
  Vec(int) v     = vec_init(int, 4);
  int      count = 0;
  vec_foreach_reversed(v, it) {
    (void)it;
    ++count;
  }
  triax_expect_eq(count, 0);
  vec_release(v);
}

triax_test(vec, vec_push_unchecked) {
  Vec(int) v = vec_init(int, 4);
  vec_push(v, 1);
  vec_push(v, 2);
  // 2 remaining — unchecked push is safe
  vec_push_unchecked(v, 3);
  vec_push_unchecked(v, 4);
  triax_expect_eq(vec_count(v), 4u);
  triax_expect_eq(v[2], 3);
  triax_expect_eq(v[3], 4);
  triax_expect_eq(vec_remaining(v), 0u);
  vec_release(v);
}

triax_test(vec, vec_shrink_to_fit) {
  Vec(int) v = vec_init(int, 32);
  vec_push(v, 1);
  vec_push(v, 2);
  vec_push(v, 3);
  triax_expect_eq(vec_count(v), 3u);
  triax_expect_true(vec_cap(v) >= 32u);

  vec_shrink_to_fit(v);
  triax_expect_eq(vec_count(v), 3u);
  triax_expect_eq(vec_cap(v), 3u);
  triax_expect_eq(v[0], 1);
  triax_expect_eq(v[1], 2);
  triax_expect_eq(v[2], 3);
  vec_release(v);
}

triax_test(vec, vec_shrink_to_fit_empty) {
  Vec(int) v = vec_init(int, 16);
  triax_expect_eq(vec_count(v), 0u);
  vec_shrink_to_fit(v);
  triax_expect_null(v); // empty vec deallocated entirely
}

triax_test(vec, vec_assign) {
  Vec(int) v     = vec_init(int, 4);
  int      arr[] = {10, 20, 30, 40, 50};
  vec_assign(v, arr, 5);
  triax_expect_eq(vec_count(v), 5u);
  for (int i = 0; i < 5; ++i) { triax_expect_eq(v[i], arr[i]); }

  // Assign smaller — count shrinks, no realloc needed
  int arr2[] = {7, 8};
  vec_assign(v, arr2, 2);
  triax_expect_eq(vec_count(v), 2u);
  triax_expect_eq(v[0], 7);
  triax_expect_eq(v[1], 8);

  vec_release(v);
}

triax_test(vec, vec_end) {
  Vec(int) v = vec_init_list(int, 1, 2, 3);
  triax_expect_eq(vec_end(v), v + 3);

  vec_push(v, 4);
  triax_expect_eq(vec_end(v), v + 4);

  // rk_null vec: end is rk_null
  Vec(int) n = rk_null;
  triax_expect_null(vec_end(n));

  vec_release(v);
}

RK__IGNWARN_CLANG_END()

#endif
