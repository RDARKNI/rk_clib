#ifndef TEST_VEC_H
#define TEST_VEC_H
#include "conf.h"

RKI_IGNWARN_CLANG_BEG("-Wunused-variable")

static unsigned char   vec_storage[102400];
rk_unused static Arena MYARENA = arena_init_static(vec_storage);

triax_test(vec, vec_init) {
  rk_unused Allocator used_alloc = alloc_ctx;
  Vec(int)            v;
  {
    v = vec_init(int, 4);
    triax_expect_nonnull(v);
    RK_IFALLOC(triax_expect_memeq((Allocator[]){vec_allocator(v)}, &alloc_ctx, sizeof(alloc_ctx));)
    triax_expect_eq(vec_allocation_size(v), sizeof(RKI_VecHdr) + sizeof(int) * 4);
    triax_expect_eq(vec_count(v), 0), triax_expect_true(vec_is_empty(v));
    vec_push(v, 99);
    triax_expect_eq(vec_count(v), 1u), triax_expect_false(vec_is_empty(v));
    (void)vec_pop(v);
    triax_expect_eq(vec_count(v), 0), triax_expect_true(vec_is_empty(v));
    triax_expect_eq(vec_cap(v), 4);
    triax_expect_eq(vec_allocation_size(v), sizeof(RKI_VecHdr) + sizeof(int) * 4);
    vec_release(v);
    triax_expect_null(v);
  }
  {
#if RK_CUSTOM_ALLOCATORS
    used_alloc = arena_to_alloc(&MYARENA);
#endif
    v = vec_init(int, 4 RK_IFALLOC(, used_alloc));
    triax_expect_nonnull(v);
    RK_IFALLOC(
        triax_expect_memeq((Allocator[]){vec_allocator(v)}, &used_alloc, sizeof(used_alloc));)
    triax_expect_eq(vec_allocation_size(v), sizeof(RKI_VecHdr) + sizeof(int) * 4);
    triax_expect_eq(vec_count(v), 0), triax_expect_true(vec_is_empty(v));
    vec_push(v, 99);
    triax_expect_eq(vec_count(v), 1u), triax_expect_false(vec_is_empty(v));
    (void)vec_pop(v);
    triax_expect_eq(vec_count(v), 0), triax_expect_true(vec_is_empty(v));
    triax_expect_eq(vec_cap(v), 4);
    triax_expect_eq(vec_allocation_size(v), sizeof(RKI_VecHdr) + sizeof(int) * 4);
    vec_release(v);
    triax_expect_null(v);
  }
}

triax_test(vec, init_list) {
  rk_unused Allocator used_alloc = alloc_ctx;
  Vec(int)            v;
  {
    v = vec_init_list(int, 0, 1, 2, 3, 4, 5);
    triax_expect_nonnull(v);
    RK_IFALLOC(
        triax_expect_memeq((Allocator[]){vec_allocator(v)}, &used_alloc, sizeof(used_alloc));)
    triax_expect_eq(vec_count(v), 6);
    triax_expect_eq(vec_allocation_size(v), sizeof(RKI_VecHdr) + sizeof(int) * vec_count(v));
    triax_expect_true(vec_cap(v) >= vec_count(v));
    for (int i = 0; i < 6; ++i) { triax_expect_eq(v[i], i); }
    vec_release(v);
  }
  {
#if RK_CUSTOM_ALLOCATORS
    used_alloc = arena_to_alloc(&MYARENA);
#endif
    v = vec_init_list(int, RK_IFALLOC(used_alloc, ) 0, 1, 2, 3, 4, 5);
    triax_expect_nonnull(v);
    RK_IFALLOC(
        triax_expect_memeq((Allocator[]){vec_allocator(v)}, &used_alloc, sizeof(used_alloc));)
    triax_expect_eq(vec_count(v), 6);
    triax_expect_eq(vec_allocation_size(v), sizeof(RKI_VecHdr) + sizeof(int) * vec_count(v));
    triax_expect_true(vec_cap(v) >= vec_count(v));
    for (int i = 0; i < 6; ++i) { triax_expect_eq(v[i], i); }
    vec_release(v);
  }
}

triax_test(vec, vec_from) {
  // constructing from a plain array, default alloc_ctx
  {
    int      arr[] = {1, 2, 3, 4, 5};
    Vec(int) v     = vec_from(arr, 5);
    triax_expect_nonnull(v);
    triax_expect_eq(vec_count(v), 5u);
    triax_expect_memeq(v, arr, sizeof(arr));
    RK_IFALLOC(triax_expect_memeq((Allocator[]){vec_allocator(v)}, &alloc_ctx, sizeof(alloc_ctx));)
    vec_release(v);
  }

  // count == 0 yields an empty (NULL) Vec, without touching arr
  {
    Vec(int) v = vec_from((int*)rk_null, 0);
    triax_expect_null(v);
  }

  // constructing from a plain array with an explicit allocator
  rk_unused Allocator used_alloc = alloc_ctx;
  {
#if RK_CUSTOM_ALLOCATORS
    used_alloc = arena_to_alloc(&MYARENA);
#endif
    int      arr[] = {10, 20, 30};
    Vec(int) v     = vec_from(arr, 3 RK_IFALLOC(, used_alloc));
    triax_expect_nonnull(v);
    triax_expect_eq(vec_count(v), 3u);
    triax_expect_memeq(v, arr, sizeof(arr));
    RK_IFALLOC(
        triax_expect_memeq((Allocator[]){vec_allocator(v)}, &used_alloc, sizeof(used_alloc));)
    vec_release(v);
  }

  // cloning an existing Vec while preserving its own allocator: since
  // vec_from() has no source Vec to default from (arr is just a pointer),
  // the caller passes vec_count()/vec_allocator() explicitly.
  {
    Vec(int) v  = vec_init_list(int, RK_IFALLOC(used_alloc, ) 0, 1, 2, 3, 4, 5);
    Vec(int) v2 = vec_from(v, vec_count(v) RK_IFALLOC(, vec_allocator(v)));
    triax_expect_eq(vec_count(v2), vec_count(v));
    triax_expect_memeq(v, v2, vec_count(v2) * sizeof(int));
    RK_IFALLOC(
        triax_expect_memeq((Allocator[]){vec_allocator(v2)}, &used_alloc, sizeof(used_alloc));)
    vec_release(v2);
    vec_release(v);
  }
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

triax_test(vec, try_pop_nonempty_removes_last) {
  Vec(int) v   = vec_init_list(int, 5, 10, 15);
  int      out = -1;
  triax_expect_true(vec_try_pop(v, &out));
  triax_expect_eq(out, 15);
  triax_expect_eq(vec_count(v), 2u);
  triax_expect_true(vec_try_pop(v, &out));
  triax_expect_true(vec_try_pop(v, &out));
  triax_expect_eq(out, 5);
  triax_expect_eq(vec_count(v), 0u);
  vec_release(v);
}

triax_test(vec, try_pop_empty_or_uninit_fails_and_leaves_out) {
  Vec(int) empty = vec_init(int, 4);
  int      out   = -1;
  triax_expect_false(vec_try_pop(empty, &out));
  triax_expect_eq(out, -1);
  triax_expect_eq(vec_count(empty), 0u);
  vec_release(empty);

  Vec(int) uninit = rk_null;
  triax_expect_false(vec_try_pop(uninit, &out));
  triax_expect_eq(out, -1);
}

// `out` is evaluated at most once: once when a value is removed, not at all when empty.
triax_test(vec, try_pop_evaluates_out_at_most_once) {
  Vec(int) v       = vec_init_list(int, 1, 2);
  int      outs[2] = {0, 0};
  int*     cursor  = outs;
  triax_expect_true(vec_try_pop(v, cursor++));
  triax_expect_eq(cursor, outs + 1);
  triax_expect_eq(outs[0], 2);
  vec_release(v);

  Vec(int) empty = vec_init(int, 4);
  triax_expect_false(vec_try_pop(empty, cursor++));
  triax_expect_eq(cursor, outs + 1);
  vec_release(empty);
}

#ifdef RKLIB_DEBUG
triax_test(vec, pop_of_empty_asserts, .isolation = TRIAX_ISOLATION_ON) {
  Vec(int) v = vec_init(int, 4);
  triax_assert_fault(TRIAX_FAULT_ABORT, { (void)vec_pop(v); });
}
#endif

// ---- accessors: _front/_back/_at are asserted lvalues, _peek_* are nullable pointers ----

triax_test(vec, at_front_back_are_writable_lvalues) {
  Vec(int) v = vec_init_list(int, 5, 10, 15);
  vec_front(v) = 1;
  vec_at(v, 1) += 1;
  vec_back(v)  = 3;
  triax_expect_eq(v[0], 1);
  triax_expect_eq(v[1], 11);
  triax_expect_eq(v[2], 3);
  triax_expect_eq(&vec_at(v, 2), &vec_back(v));
  vec_release(v);
}

triax_test(vec, peek_in_bounds_returns_element_pointer) {
  Vec(int) v = vec_init_list(int, 5, 10, 15);
  triax_expect_eq(vec_peek_front(v), &v[0]);
  triax_expect_eq(vec_peek_back(v), &v[2]);
  triax_expect_eq(vec_peek_at(v, 1), &v[1]);
  *vec_peek_at(v, 1) = 7;
  triax_expect_eq(v[1], 7);
  vec_release(v);
}

triax_test(vec, peek_out_of_bounds_returns_null) {
  Vec(int) v = vec_init_list(int, 5, 10, 15);
  triax_expect_null(vec_peek_at(v, 3));
  triax_expect_null(vec_peek_at(v, (size_t)-1));
  vec_release(v);

  Vec(int) empty = vec_init(int, 4);
  triax_expect_null(vec_peek_front(empty));
  triax_expect_null(vec_peek_back(empty));
  triax_expect_null(vec_peek_at(empty, 0));
  vec_release(empty);

  Vec(int) uninit = rk_null;
  triax_expect_null(vec_peek_front(uninit));
  triax_expect_null(vec_peek_back(uninit));
  triax_expect_null(vec_peek_at(uninit, 0));
}

triax_test(vec, accessors_preserve_element_constness) {
  Vec(int)   v  = vec_init_list(int, 5, 10, 15);
  const int* cv = v;
  static_assert(_Generic(&vec_at(cv, 0), const int*: 1, default: 0), "const Vec yields const");
  static_assert(_Generic(&vec_front(cv), const int*: 1, default: 0), "const Vec yields const");
  static_assert(_Generic(&vec_back(cv), const int*: 1, default: 0), "const Vec yields const");
  static_assert(_Generic(vec_peek_at(cv, 0), const int*: 1, default: 0), "const Vec yields const");
  static_assert(_Generic(&vec_at(v, 0), int*: 1, default: 0), "mutable Vec yields mutable");
  static_assert(_Generic(vec_peek_back(v), int*: 1, default: 0), "mutable Vec yields mutable");
  triax_expect_eq(vec_at(cv, 2), 15);
  triax_expect_eq(*vec_peek_front(cv), 5);
  vec_release(v);
}

// An expected fault ends the test process, so each case needs its own test.
#ifdef RKLIB_DEBUG
triax_test(vec, at_out_of_bounds_asserts, .isolation = TRIAX_ISOLATION_ON) {
  Vec(int) v = vec_init_list(int, 5, 10, 15);
  triax_assert_fault(TRIAX_FAULT_ABORT, { (void)vec_at(v, 3); });
}

triax_test(vec, front_of_empty_asserts, .isolation = TRIAX_ISOLATION_ON) {
  Vec(int) v = vec_init(int, 4);
  triax_assert_fault(TRIAX_FAULT_ABORT, { (void)vec_front(v); });
}

triax_test(vec, back_of_empty_asserts, .isolation = TRIAX_ISOLATION_ON) {
  Vec(int) v = vec_init(int, 4);
  triax_assert_fault(TRIAX_FAULT_ABORT, { (void)vec_back(v); });
}
#endif

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

triax_test(vec, foreach_empty_is_noop) {
  Vec(int) v     = vec_init(int, 4);
  int      count = 0;
  vec_foreach(v, it) {
    (void)it;
    ++count;
  }
  triax_expect_eq(count, 0);
  vec_release(v);
}

triax_test(vec, foreach_null_vec_is_noop) {
  Vec(int) v     = rk_null;
  int      count = 0;
  vec_foreach(v, it) {
    (void)it;
    ++count;
  }
  vec_foreach_reversed(v, it) {
    (void)it;
    ++count;
  }
  triax_expect_eq(count, 0);
}

static Vec(int) rki_mark_eval_vec(Vec(int) v, int* count) {
  ++*count;
  return v;
}

triax_test(vec, foreach_break_stops_iteration) {
  Vec(int) v   = vec_init_list(int, 0, 1, 2, 3, 4);
  int      sum = 0, visits = 0;
  vec_foreach(v, it) {
    if (*it == 3) { break; }
    sum += *it;
    ++visits;
  }
  triax_expect_eq(sum, 3); // 0+1+2, stops before 3
  triax_expect_eq(visits, 3);
  vec_release(v);
}

triax_test(vec, foreach_continue_skips_element) {
  Vec(int) v   = vec_init_list(int, 0, 1, 2, 3, 4);
  int      sum = 0, visits = 0;
  vec_foreach(v, it) {
    if (*it == 2) { continue; }
    sum += *it;
    ++visits;
  }
  triax_expect_eq(sum, 8); // 0+1+3+4
  triax_expect_eq(visits, 4);
  vec_release(v);
}

triax_test(vec, foreach_reversed_break_stops_iteration) {
  Vec(int) v   = vec_init_list(int, 0, 1, 2, 3, 4);
  int      sum = 0, visits = 0;
  vec_foreach_reversed(v, it) {
    if (*it == 1) { break; }
    sum += *it;
    ++visits;
  }
  triax_expect_eq(sum, 9); // 4+3+2, stops before 1
  triax_expect_eq(visits, 3);
  vec_release(v);
}

triax_test(vec, foreach_reversed_continue_skips_element) {
  Vec(int) v   = vec_init_list(int, 0, 1, 2, 3, 4);
  int      sum = 0, visits = 0;
  vec_foreach_reversed(v, it) {
    if (*it == 2) { continue; }
    sum += *it;
    ++visits;
  }
  triax_expect_eq(sum, 8); // 4+3+1+0
  triax_expect_eq(visits, 4);
  vec_release(v);
}

triax_test(vec, foreach_evaluates_vec_argument_once) {
  Vec(int) v     = vec_init_list(int, 1, 2, 3);
  int      evals = 0, sum = 0;
  vec_foreach(rki_mark_eval_vec(v, &evals), it) { sum += *it; }
  triax_expect_eq(evals, 1);
  triax_expect_eq(sum, 6);
  vec_release(v);
}

triax_test(vec, foreach_reversed_evaluates_vec_argument_once) {
  Vec(int) v     = vec_init_list(int, 1, 2, 3);
  int      evals = 0, sum = 0;
  vec_foreach_reversed(rki_mark_eval_vec(v, &evals), it) { sum += *it; }
  triax_expect_eq(evals, 1);
  triax_expect_eq(sum, 6);
  vec_release(v);
}

triax_test(vec, erase_if_evaluates_vec_argument_once) {
  Vec(int) v     = vec_init_list(int, 1, 2, 3, 4, 5);
  int      evals = 0;
  vec_erase_if(rki_mark_eval_vec(v, &evals), it, *it % 2 == 0);
  triax_expect_eq(evals, 1);
  triax_expect_eq(vec_count(v), 3u); // 1, 3, 5 remain
  vec_release(v);
}

triax_test(vec, erase_if_evaluates_predicate_once_per_element) {
  Vec(int) v          = vec_init_list(int, 1, 2, 3, 4, 5);
  int      pred_calls = 0;
  vec_erase_if(v, it, (++pred_calls, *it % 2 == 0));
  triax_expect_eq(pred_calls, 5);
  triax_expect_eq(vec_count(v), 3u);
  vec_release(v);
}

triax_test(vec, erase_if_empty_is_noop) {
  Vec(int) v          = vec_init(int, 4);
  int      pred_calls = 0;
  vec_erase_if(v, it, (++pred_calls, (void)it, true));
  triax_expect_eq(pred_calls, 0);
  triax_expect_eq(vec_count(v), 0u);
  vec_release(v);
}

triax_test(vec, erase_if_null_vec_is_noop) {
  Vec(int) v          = rk_null;
  int      pred_calls = 0;
  vec_erase_if(v, it, (++pred_calls, (void)it, true));
  triax_expect_eq(pred_calls, 0);
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

  vec_shrink_to_fit(v); // rounds up to the next power of two (bit_ceil(3) == 4)
  triax_expect_eq(vec_count(v), 3u);
  triax_expect_eq(vec_cap(v), 4u);
  triax_expect_eq(v[0], 1);
  triax_expect_eq(v[1], 2);
  triax_expect_eq(v[2], 3);

  vec_shrink_to_fit(v); // already at the minimal power-of-two capacity; no-op
  triax_expect_eq(vec_cap(v), 4u);
  vec_release(v);
}

triax_test(vec, vec_shrink_to_fit_empty) {
  Vec(int) v = vec_init(int, 16);
  triax_expect_eq(vec_count(v), 0u);
  vec_shrink_to_fit(v);
  triax_expect_null(v); // empty vec deallocated entirely
}

triax_test(vec, vec_shrink_to_fit_exact) {
  Vec(int) v = vec_init(int, 32);
  vec_push(v, 1);
  vec_push(v, 2);
  vec_push(v, 3);
  triax_expect_eq(vec_count(v), 3u);
  triax_expect_true(vec_cap(v) >= 32u);

  vec_shrink_to_fit_exact(v);
  triax_expect_eq(vec_count(v), 3u);
  triax_expect_eq(vec_cap(v), 3u);
  triax_expect_eq(v[0], 1);
  triax_expect_eq(v[1], 2);
  triax_expect_eq(v[2], 3);
  vec_release(v);
}

triax_test(vec, vec_shrink_to_fit_exact_empty) {
  Vec(int) v = vec_init(int, 16);
  triax_expect_eq(vec_count(v), 0u);
  vec_shrink_to_fit_exact(v);
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

RKI_IGNWARN_CLANG_END()

#endif
