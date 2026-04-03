#ifndef RK__TESTDUMMY
# include "../rk_test/rk_test.h"
#else
# include "../rk_test/rk_test_dummy.h"
#endif
#define RK_IMPL
#include "../include/rklib_includeall.h"
RK_HEADER_BEGIN
RK__IGNWARN_CLANG_BEG("-Wunused-variable")

unsigned char storage[102400];
Arena         MYARENA = arena_init_static(storage);
Allocator     alloc   = arena_to_alloc_static(&MYARENA);
RK_REGISTER_TEST("vec", test_vec_init) {
  Vec(int) v = vec_init(int, 4);
  rk_expect_true(vec_is_empty(v));
  vec_push(v, 99);
  rk_expect_false(vec_is_empty(v));
  vec_pop(v);
  rk_expect_nonnull(v);
  rk_expect_eq(vec_count(v), 0);
  rk_expect_eq(vec_cap(v), 4);
  rk_expect_memeq((Allocator[]){vec_allocator(v)}, &alloc_ctx,
                  sizeof(alloc_ctx));

  rk_expect_eq(vec_allocation_size(v), sizeof(VecHeader) + sizeof(int) * 4);
  vec_release(v);
  rk_expect_null(v);
  v = vec_init(int, 4, alloc);
  rk_expect_nonnull(v);
  rk_expect_eq(vec_count(v), 0);
  rk_expect_eq(vec_cap(v), 4);

  rk_expect_memeq((Allocator[]){vec_allocator(v)}, &alloc, sizeof(alloc_ctx));
  rk_expect_eq(vec_allocation_size(v), sizeof(VecHeader) + sizeof(int) * 4);
  vec_release(v);
  rk_expect_null(v);

  v = vec_init_list(int, 0, 1, 2, 3, 4, 5);
  for (int i = 0; i < 6; ++i) { rk_expect_eq(v[i], i); }
  rk_expect_eq(vec_count(v), 6);
  vec_clear(v);
  rk_expect_eq(vec_count(v), 0);
  vec_release(v);
  v = vec_init_list(int, alloc, 0, 1, 2, 3, 4, 5);
  for (int i = 0; i < 6; ++i) { rk_expect_eq(v[i], i); }
  rk_expect_eq(vec_count(v), 6);

  // clone without custom allocator
  Vec(int) v2 = vec_copy(v);

  for (int i = 0; i < 6; ++i) { rk_expect_eq(v[i], i); }
  rk_expect_eq(vec_count(v2), vec_count(v));
  rk_expect_memeq((Allocator[]){vec_allocator(v)},
                  (Allocator[]){vec_allocator(v2)}, sizeof(alloc_ctx));
  vec_release(v2);
  // clone with custom allocator
  v2 = vec_copy(v, alloc_ctx);

  for (int i = 0; i < 6; ++i) { rk_expect_eq(v[i], i); }
  rk_expect_eq(vec_count(v2), vec_count(v));
  rk_expect_memeq(&alloc_ctx, (Allocator[]){vec_allocator(v2)},
                  sizeof(alloc_ctx));
  vec_release(v2);
  vec_release(v);
  arena_clear(&MYARENA);
}

RK_REGISTER_TEST("vec", test_vec_push_pop) {
  Vec(int) v = vec_init(int, 4);
  vec_push(v, 10);
  vec_push(v, 20);
  rk_expect_eq(vec_count(v), 2);
  rk_expect_eq(vec_pop(v), 20);
  rk_expect_eq(vec_pop(v), 10);
  for (int i = 0; i < 129; ++i) { vec_push(v, i); }
  rk_expect_eq(vec_count(v), 129);
  for (int i = 0; i < 129; ++i) {
    rk_expect_eq(v[i], i);
    rk_expect_eq(vec_pop(v), 129 - i - 1);
  }
  rk_expect_eq(vec_count(v), 0);
  vec_release(v);
}

RK_REGISTER_TEST("vec", test_vec_insert_erase) {
  Vec(int) v = vec_init(int, 4);
  vec_push(v, 1);
  vec_push(v, 2);
  vec_push(v, 3);
  vec_insert_at(v, 1, 42);
  rk_expect_eq(v[1], 42);
  vec_erase_at(v, 1);
  rk_expect_eq(v[1], 2);
  vec_release(v);
}

RK_REGISTER_TEST("vec", test_vec_foreach) {
  Vec(int) v = vec_init_list(int, 0, 1, 2, 3, 4);
  rk_expect_eq(vec_count(v), 5);
  for (int i = 0; i < 5; ++i) { rk_expect_eq(v[i], i); }
  int sum = 0;
  vec_foreach(v, it) { sum += *it; }
  rk_expect_eq(sum, 10);
  vec_release(v);

  Vec(int) v2 = vec_init_list(int, alloc, 0, 1, 2, 3, 4);
  rk_expect_eq(vec_count(v2), 5);
  for (int i = 0; i < 5; ++i) { rk_expect_eq(v2[i], i); }
  sum = 0;
  vec_foreach(v2, it) { sum += *it; }
  rk_expect_eq(sum, 10);
  vec_release(v2);
}

RK_REGISTER_TEST("vec", test_vec_reverse) {
  Vec(int) v = vec_init_list(int, 1, 2, 3, 4);
  vec_reverse(v);
  rk_expect_eq(v[0], 4);
  rk_expect_eq(v[1], 3);
  rk_expect_eq(v[2], 2);
  rk_expect_eq(v[3], 1);
  vec_release(v);
}

RK_REGISTER_TEST("vec", test_vec_index_in_range) {
  Vec(int) v = vec_init_list(int, 10, 20, 30);
  rk_expect_true(vec_index_in_range(v, 0));
  rk_expect_true(vec_index_in_range(v, 2));
  rk_expect_false(vec_index_in_range(v, 3));
  vec_release(v);
}

RK_REGISTER_TEST("vec", test_vec_push_n_pop_n) {
  Vec(int) v = vec_init(int, 5);
  vec_push_n(v, ((int[]){1, 2, 3}), 3);
  rk_expect_eq(vec_count(v), 3);
  int* popped = vec_pop_n(v, 2);
  rk_expect_eq(popped[0], 2);
  rk_expect_eq(popped[1], 3);
  rk_expect_eq(vec_count(v), 1);
  vec_release(v);
}

RK_REGISTER_TEST("vec", test_vec_front_back) {
  Vec(int) v = vec_init_list(int, 5, 10, 15);
  rk_expect_eq(vec_front(v), 5);
  rk_expect_eq(vec_back(v), 15);
  vec_release(v);
}

RK_REGISTER_TEST("vec", test_vec_insert_arr_erase_n) {
  Vec(int) v     = vec_init(int, 5);
  int      arr[] = {10, 20, 30};
  vec_insert_arr_at(v, 0, arr, 3);
  rk_expect_eq(vec_count(v), 3);
  vec_erase_at_n(v, 0, 2);
  rk_expect_eq(vec_count(v), 1);
  rk_expect_eq(v[0], 30);
  vec_release(v);
}

RK_REGISTER_TEST("vec", test_vec_clear_resize_reserve) {
  Vec(int) v = vec_init_list(int, 1, 2, 3);
  vec_clear(v);
  rk_expect_eq(vec_count(v), 0);
  vec_resize(v, 5);
  rk_expect_eq(vec_count(v), 5);
  vec_reserve(v, 10);
  rk_expect_true(vec_cap(v) >= 10);
  vec_release(v);
}

RK_REGISTER_TEST("vec", test_vec_reverse_foreach) {
  Vec(int) v = vec_init_list(int, 1, 2, 3, 4);
  vec_reverse(v);
  int    expected[] = {4, 3, 2, 1};
  size_t i          = 0;
  vec_foreach(v, it) { rk_expect_eq(*it, expected[i++]); }
  vec_release(v);
}

RK_REGISTER_TEST("vec", test_vec_remaining) {
  Vec(int) v   = vec_init(int, 4);
  size_t   cap = vec_cap(v);
  rk_expect_eq(cap, 4);
  rk_expect_eq(vec_remaining(v), 4);
  vec_push(v, 1);
  rk_expect_eq(vec_remaining(v), 3);
  vec_release(v);
}
typedef struct DummyStruct {
  int x, y;
} DummyStruct;

static inline DummyStruct dummystructret() { return (DummyStruct){}; }
RK_REGISTER_TEST("vec", test_vec_compound) {
  Vec(DummyStruct) v = vec_init(DummyStruct, 4);
  vec_push(v, ((DummyStruct){1, 2}));
  vec_push(v, ((DummyStruct){3, 4}));
  vec_push(v, dummystructret());
  vec_push(v, ((void)(DummyStruct){3, 4}, (DummyStruct){3, 4}));

  rk_expect_eq(vec_remaining(v), 0);
  vec_release(v);
}

RK_REGISTER_TEST("vec", test_veceraseif) {
  Vec(int) v = vec_init_list(int, alloc_ctx, 45u, 5);
  rk_assert_eq(v[0], 45);
  rk_assert_eq(v[1], 5);
  vec_erase_if(v, ot, *ot == 5);
  rk_assert_eq(vec_count(v), 1);
  Vec(int)   v2 = vec_init_list(int, 1u);
  char       ss;
  Vec(char*) v3 = vec_init_list(char*, &ss);
}

RK_REGISTER_TEST("vec", test_vec_pushetc) {
  Vec(int) v = 0;
  for (int i = 0; i < 2540000; ++i) {
    vec_push(v, i * 3);
    rk_assert_eq(v[i], i * 3);
  }
}

RK_REGISTER_TEST("vec", test_vec_insert_at_unordered) {
  // Insert into the middle — displaced element goes to back
  Vec(int) v = vec_init_list(int, 1, 2, 3, 4);
  vec_insert_at_unordered(v, 1, 99);
  rk_expect_eq(vec_count(v), 5u);
  rk_expect_eq(v[1], 99); // new element at idx
  rk_expect_eq(v[4], 2);  // displaced element at back
  rk_expect_eq(v[0], 1);
  rk_expect_eq(v[2], 3);
  rk_expect_eq(v[3], 4);

  // Insert at the end — behaves like push, no displacement
  vec_insert_at_unordered(v, vec_count(v), 77);
  rk_expect_eq(vec_count(v), 6u);
  rk_expect_eq(v[5], 77);

  // Insert at front
  vec_insert_at_unordered(v, 0, 55);
  rk_expect_eq(vec_count(v), 7u);
  rk_expect_eq(v[0], 55);

  vec_release(v);
}

RK_REGISTER_TEST("vec", test_vec_erase_at_unordered) {
  // Erase from middle — last element fills the gap
  Vec(int) v = vec_init_list(int, 10, 20, 30, 40, 50);
  vec_erase_at_unordered(v, 1);
  rk_expect_eq(vec_count(v), 4u);
  rk_expect_eq(v[1], 50); // last element moved into erased slot
  rk_expect_eq(v[0], 10);
  rk_expect_eq(v[2], 30);
  rk_expect_eq(v[3], 40);

  // Erase last element — simple shrink, no swap
  vec_erase_at_unordered(v, 3);
  rk_expect_eq(vec_count(v), 3u);
  rk_expect_eq(v[2], 30);

  // Erase first element
  vec_erase_at_unordered(v, 0);
  rk_expect_eq(vec_count(v), 2u);

  vec_release(v);
}

RK_REGISTER_TEST("vec", test_vec_foreach_reversed) {
  Vec(int) v          = vec_init_list(int, 1, 2, 3, 4, 5);
  int      expected[] = {5, 4, 3, 2, 1};
  size_t   i          = 0;
  vec_foreach_reversed(v, it) {
    rk_expect_eq(*it, expected[i]);
    ++i;
  }
  rk_expect_eq(i, 5u);
  vec_release(v);
}

RK_REGISTER_TEST("vec", test_vec_foreach_reversed_empty) {
  Vec(int) v     = vec_init(int, 4);
  int      count = 0;
  vec_foreach_reversed(v, it) {
    (void)it;
    ++count;
  }
  rk_expect_eq(count, 0);
  vec_release(v);
}

RK_REGISTER_TEST("vec", test_vec_push_unchecked) {
  Vec(int) v = vec_init(int, 4);
  vec_push(v, 1);
  vec_push(v, 2);
  // 2 remaining — unchecked push is safe
  vec_push_unchecked(v, 3);
  vec_push_unchecked(v, 4);
  rk_expect_eq(vec_count(v), 4u);
  rk_expect_eq(v[2], 3);
  rk_expect_eq(v[3], 4);
  rk_expect_eq(vec_remaining(v), 0u);
  vec_release(v);
}

RK_REGISTER_TEST("vec", test_vec_shrink_to_fit) {
  Vec(int) v = vec_init(int, 32);
  vec_push(v, 1);
  vec_push(v, 2);
  vec_push(v, 3);
  rk_expect_eq(vec_count(v), 3u);
  rk_expect_true(vec_cap(v) >= 32u);

  vec_shrink_to_fit(v);
  rk_expect_eq(vec_count(v), 3u);
  rk_expect_eq(vec_cap(v), 3u);
  rk_expect_eq(v[0], 1);
  rk_expect_eq(v[1], 2);
  rk_expect_eq(v[2], 3);
  vec_release(v);
}

RK_REGISTER_TEST("vec", test_vec_shrink_to_fit_empty) {
  Vec(int) v = vec_init(int, 16);
  rk_expect_eq(vec_count(v), 0u);
  vec_shrink_to_fit(v);
  rk_expect_null(v); // empty vec deallocated entirely
}

RK_REGISTER_TEST("vec", test_vec_assign) {
  Vec(int) v     = vec_init(int, 4);
  int      arr[] = {10, 20, 30, 40, 50};
  vec_assign(v, arr, 5);
  rk_expect_eq(vec_count(v), 5u);
  for (int i = 0; i < 5; ++i) { rk_expect_eq(v[i], arr[i]); }

  // Assign smaller — count shrinks, no realloc needed
  int arr2[] = {7, 8};
  vec_assign(v, arr2, 2);
  rk_expect_eq(vec_count(v), 2u);
  rk_expect_eq(v[0], 7);
  rk_expect_eq(v[1], 8);

  vec_release(v);
}

RK_REGISTER_TEST("vec", test_vec_end) {
  Vec(int) v = vec_init_list(int, 1, 2, 3);
  rk_expect_eq(vec_end(v), v + 3);

  vec_push(v, 4);
  rk_expect_eq(vec_end(v), v + 4);

  // rk_null vec: end is rk_null
  Vec(int) n = rk_null;
  rk_expect_null(vec_end(n));

  vec_release(v);
}

RK__IGNWARN_CLANG_END()
RK_HEADER_END
