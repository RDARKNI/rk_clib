#ifndef TEST_HEAP_H
#define TEST_HEAP_H
#include "conf.h"

#define RK_IMPL
#include "../include/rklib.h"

RK_HEADER_BEGIN
RK__IGNWARN_CLANG_BEG("-Wunused-variable")

extern_fun int heap_int_cmp(int a, int b) { return (a > b) - (a < b); }

HEAP_DEFINE(int, heap_int_cmp)

triax_test(heap, heaptest) {
  /* ------------------------------------------------------------------------ */
  /* init / count / empty / peek on empty                                     */
  /* ------------------------------------------------------------------------ */
  Heap(int) h = heap_init(int, 0);

  triax_assert_true(heap_is_empty(&h));
  triax_assert_eq(heap_count(&h), 0u);
  triax_assert_true(heap_peek(int, &h) == rk_null);

  /* ------------------------------------------------------------------------ */
  /* push out of order; peek always reflects the current minimum              */
  /* ------------------------------------------------------------------------ */
  heap_push(int, &h, 3);
  triax_assert_eq(*heap_peek(int, &h), 3);

  heap_push(int, &h, 1);
  triax_assert_eq(*heap_peek(int, &h), 1);

  heap_push(int, &h, 4);
  triax_assert_eq(*heap_peek(int, &h), 1);

  heap_push(int, &h, 2);
  triax_assert_eq(*heap_peek(int, &h), 1);

  triax_assert_true(!heap_is_empty(&h));
  triax_assert_eq(heap_count(&h), 4u);

  /* ------------------------------------------------------------------------ */
  /* pop drains in strictly ascending order                                   */
  /* ------------------------------------------------------------------------ */
  triax_assert_eq(heap_pop(int, &h), 1);
  triax_assert_eq(heap_count(&h), 3u);
  triax_assert_eq(heap_pop(int, &h), 2);
  triax_assert_eq(heap_pop(int, &h), 3);
  triax_assert_eq(heap_pop(int, &h), 4);
  triax_assert_eq(heap_count(&h), 0u);
  triax_assert_true(heap_is_empty(&h));
  triax_assert_true(heap_peek(int, &h) == rk_null);

  /* ------------------------------------------------------------------------ */
  /* release on empty / reused heap                                           */
  /* ------------------------------------------------------------------------ */
  heap_release(&h);
  triax_assert_true(heap_is_empty(&h));
  triax_assert_eq(heap_count(&h), 0u);

  /* heap should still be reusable after release */
  heap_push(int, &h, 42);
  triax_assert_eq(heap_count(&h), 1u);
  triax_assert_eq(*heap_peek(int, &h), 42);

  heap_release(&h);
  triax_assert_true(heap_is_empty(&h));
}

triax_test(heap, zero_initialized_is_empty) {
  /* A zero-initialized Heap(T) is documented as a valid, empty heap, with no
   * separate _init_static needed (unlike Bst/Avl/Rbt). */
  Heap(int) h = {0};
  triax_expect_true(heap_is_empty(&h));
  triax_expect_eq(heap_count(&h), 0u);
  triax_expect_null(heap_peek(int, &h));

  heap_push(int, &h, 7);
  triax_expect_eq(heap_count(&h), 1u);
  triax_expect_eq(*heap_peek(int, &h), 7);

  heap_release(&h);
  triax_expect_true(heap_is_empty(&h));
}

triax_test(heap, try_pop) {
  Heap(int) h = heap_init(int, 0);

  int out     = -1;
  triax_expect_false(heap_try_pop(int, &h, &out));
  triax_expect_eq(out, -1); // left untouched

  heap_push(int, &h, 5);
  heap_push(int, &h, 1);
  heap_push(int, &h, 9);

  triax_expect_true(heap_try_pop(int, &h, &out));
  triax_expect_eq(out, 1);
  triax_expect_eq(heap_count(&h), 2u);

  triax_expect_true(heap_try_pop(int, &h, &out));
  triax_expect_eq(out, 5);
  triax_expect_true(heap_try_pop(int, &h, &out));
  triax_expect_eq(out, 9);

  triax_expect_true(heap_is_empty(&h));
  triax_expect_false(heap_try_pop(int, &h, &out));
  triax_expect_eq(out, 9); // still untouched by the failed try_pop

  heap_release(&h);
}

triax_test(heap, replace_top) {
  Heap(int) h = heap_init(int, 0);
  heap_push(int, &h, 5);
  heap_push(int, &h, 1);
  heap_push(int, &h, 9);
  heap_push(int, &h, 3);
  triax_expect_eq(heap_count(&h), 4u);

  int*   before_ptr = h.data;
  size_t before_cnt = heap_count(&h);

  // replaces the minimum (1) with 100: count is unchanged, and since it never
  // shrinks/grows the backing Vec, the data pointer must be unchanged too.
  int    old_min = heap_replace_top(int, &h, 100);
  triax_expect_eq(old_min, 1);
  triax_expect_eq(heap_count(&h), before_cnt);
  triax_expect_eq(h.data, before_ptr);
  triax_expect_eq(*heap_peek(int, &h), 3);

  // replacing with a new minimum takes effect immediately
  old_min = heap_replace_top(int, &h, 0);
  triax_expect_eq(old_min, 3);
  triax_expect_eq(*heap_peek(int, &h), 0);

  int expect[] = {0, 5, 9, 100};
  for (size_t i = 0; i < 4; ++i) { triax_expect_eq(heap_pop(int, &h), expect[i]); }
  triax_expect_true(heap_is_empty(&h));

  heap_release(&h);
}

triax_test(heap, clear_and_reserve) {
  Heap(int) h = heap_init(int, 0);
  heap_push(int, &h, 3);
  heap_push(int, &h, 1);
  heap_push(int, &h, 2);
  triax_expect_eq(heap_count(&h), 3u);

  heap_clear(&h);
  triax_expect_true(heap_is_empty(&h));
  triax_expect_eq(heap_count(&h), 0u);
  triax_expect_null(heap_peek(int, &h));

  // still usable after clear
  heap_push(int, &h, 10);
  triax_expect_eq(*heap_peek(int, &h), 10);

  heap_reserve(&h, 64);
  triax_expect_true(vec_cap(h.data) >= 64u);
  triax_expect_eq(heap_count(&h), 1u); // reserve doesn't affect existing elements

  heap_release(&h);
}

triax_test(heap, duplicates) {
  Heap(int) h     = heap_init(int, 0);
  int    values[] = {5, 3, 5, 1, 3, 5, 1, 1};
  size_t n        = sizeof(values) / sizeof(values[0]);
  for (size_t i = 0; i < n; ++i) { heap_push(int, &h, values[i]); }
  triax_expect_eq(heap_count(&h), n);
  int prev = -1;
  for (size_t i = 0; i < n; ++i) {
    int v = heap_pop(int, &h);
    triax_expect_true(v >= prev);
    prev = v;
  }
  triax_expect_true(heap_is_empty(&h));

  heap_release(&h);
}

triax_test(heap, extend_appends_and_reorders) {
  // heap_extend appends to EXISTING contents (unlike heap_from/heap_assign,
  // which always start fresh or discard what was there) and re-heapifies the
  // combined set.
  Heap(int) h = heap_from(int, ((int[]){10, 20, 30}), 3);
  triax_expect_eq(heap_count(&h), 3u);

  int    arr[] = {9, 3, 7, 1, 8, 2, 6, 5, 4, 0, -3, 42, 17, -100, 8, 8};
  size_t n     = sizeof(arr) / sizeof(arr[0]);
  heap_extend(int, &h, arr, n);
  triax_expect_eq(heap_count(&h), n + 3);

  int    prev = -1000000;
  size_t got  = 0;
  while (!heap_is_empty(&h)) {
    int v = heap_pop(int, &h);
    triax_expect_true(v >= prev);
    prev = v;
    ++got;
  }
  triax_expect_eq(got, n + 3);

  heap_release(&h);
}

triax_test(heap, extend_empty_and_singleton) {
  Heap(int) h0 = heap_init(int, 0);
  heap_extend(int, &h0, (int*)rk_null, 0); // must not crash on an empty heap
  triax_expect_true(heap_is_empty(&h0));
  heap_release(&h0);

  Heap(int) h1 = heap_init(int, 0);
  heap_push(int, &h1, 5);
  heap_extend(int, &h1, (int*)rk_null, 0); // extending by zero is a no-op
  triax_expect_eq(heap_count(&h1), 1u);
  triax_expect_eq(*heap_peek(int, &h1), 5);
  heap_release(&h1);

  Heap(int) h2 = heap_init(int, 0);
  int arr[] = {3, 1, 2};
  heap_extend(int, &h2, arr, 3); // extending an empty heap == building fresh
  triax_expect_eq(heap_count(&h2), 3u);
  triax_expect_eq(heap_pop(int, &h2), 1);
  heap_release(&h2);
}

triax_test(heap, from_and_assign) {
  // heap_from: constructs a fresh, correctly heap-ordered Heap in one call
  {
    int arr[] = {9, 3, 7, 1, 8, 2, 6, 5, 4, 0};
    Heap(int) h = heap_from(int, arr, 10);
    triax_expect_eq(heap_count(&h), 10u);
    int prev = -1000000, got = 0;
    while (!heap_is_empty(&h)) {
      int v = heap_pop(int, &h);
      triax_expect_true(v >= prev);
      prev = v;
      ++got;
    }
    triax_expect_eq(got, 10);
    heap_release(&h);
  }

  // heap_from with n == 0 yields an empty heap
  {
    Heap(int) h = heap_from(int, (int*)rk_null, 0);
    triax_expect_true(heap_is_empty(&h));
    heap_release(&h);
  }

  // heap_from with an explicit allocator
  {
    Heap(int) h = heap_from(int, ((int[]){3, 1, 2}), 3 RK_IFALLOC(, alloc_ctx));
    triax_expect_eq(heap_count(&h), 3u);
    triax_expect_eq(heap_pop(int, &h), 1);
    heap_release(&h);
  }

  // heap_assign: replaces contents entirely, reusing the buffer when it
  // already fits (no growth needed)
  {
    Heap(int) h = heap_from(int, ((int[]){5, 1, 9, 3}), 4);
    int* before_ptr   = h.data;
    size_t before_cap = heap_cap(&h);

    int replacement[] = {100, 50};
    heap_assign(int, &h, replacement, 2);
    triax_expect_eq(heap_count(&h), 2u);
    triax_expect_eq(h.data, before_ptr);   // reused, no reallocation
    triax_expect_eq(heap_cap(&h), before_cap);
    triax_expect_eq(heap_pop(int, &h), 50);
    triax_expect_eq(heap_pop(int, &h), 100);
    triax_expect_true(heap_is_empty(&h));
    heap_release(&h);
  }

  // heap_assign growing beyond current capacity
  {
    Heap(int) h = heap_from(int, ((int[]){1, 2}), 2);
    int big[20];
    for (int i = 0; i < 20; ++i) { big[i] = 20 - i; }
    heap_assign(int, &h, big, 20);
    triax_expect_eq(heap_count(&h), 20u);
    triax_expect_true(heap_cap(&h) >= 20u);
    int prev = -1;
    while (!heap_is_empty(&h)) {
      int v = heap_pop(int, &h);
      triax_expect_true(v >= prev);
      prev = v;
    }
    heap_release(&h);
  }

  // heap_assign into a freshly-initialized, empty heap
  {
    Heap(int) h = heap_init(int, 0);
    int arr[] = {3, 1, 2};
    heap_assign(int, &h, arr, 3);
    triax_expect_eq(heap_count(&h), 3u);
    triax_expect_eq(heap_pop(int, &h), 1);
    heap_release(&h);
  }
}

triax_test(heap, adopt) {
  // heap_adopt takes ownership of an existing Vec with no copy: the Heap's
  // backing storage must be the exact same allocation the Vec had.
  {
    Vec(int) v = vec_init(int, 0);
    vec_push(v, 9);
    vec_push(v, 3);
    vec_push(v, 7);
    vec_push(v, 1);
    vec_push(v, 8);
    int* original_ptr = v;

    Heap(int) h = heap_adopt(int, v);
    triax_expect_eq(h.data, original_ptr); // same allocation, not a copy
    triax_expect_eq(heap_count(&h), 5u);

    int prev = -1000000, got = 0;
    while (!heap_is_empty(&h)) {
      int val = heap_pop(int, &h);
      triax_expect_true(val >= prev);
      prev = val;
      ++got;
    }
    triax_expect_eq(got, 5);
    heap_release(&h);
  }

  // adopting an empty (NULL) Vec yields an empty Heap
  {
    Vec(int) v = vec_init(int, 0);
    triax_expect_null(v);
    Heap(int) h = heap_adopt(int, v);
    triax_expect_true(heap_is_empty(&h));
    heap_release(&h);
  }

  // adopting a single-element Vec
  {
    Vec(int) v = vec_init(int, 0);
    vec_push(v, 42);
    Heap(int) h = heap_adopt(int, v);
    triax_expect_eq(heap_count(&h), 1u);
    triax_expect_eq(*heap_peek(int, &h), 42);
    heap_release(&h);
  }
}

triax_test(heap, large_worst_case_order) {
  Heap(int) h = heap_init(int, 0);

  // Push already-sorted descending data (the worst case for sift-up: every
  // push bubbles all the way to the root), then verify pop drains ascending.
  for (int i = 64; i >= 1; --i) { heap_push(int, &h, i); }
  triax_expect_eq(heap_count(&h), 64u);
  triax_expect_eq(*heap_peek(int, &h), 1);

  for (int expect = 1; expect <= 64; ++expect) { triax_expect_eq(heap_pop(int, &h), expect); }
  triax_expect_true(heap_is_empty(&h));

  heap_release(&h);
}

RK__IGNWARN_CLANG_END()
RK_HEADER_END
#endif
