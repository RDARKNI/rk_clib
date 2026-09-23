#ifndef TEST_POOL_H
#define TEST_POOL_H
#include "conf.h"

RK_HEADER_BEGIN
RK__IGNWARN_CLANG_BEG("-Wunused-variable")
typedef struct PoolIntPair { int a, b; } PoolIntPair;

triax_test(pool, dynamic_init_empty_state) {
  POOL_DEFINE(int);
  Pool(int) p = pool_init_dynamic(int, 8);
  triax_expect_eq((size_t)8, pool_cap(&p));
  triax_expect_eq((size_t)0, pool_used(&p));
  triax_expect_eq((size_t)8, pool_remaining(&p));
  triax_expect_true(pool_is_empty(&p));
  triax_expect_false(pool_is_full(&p));
  pool_release(&p);
}

triax_test(pool, static_init_empty_state) {
  POOL_DEFINE(int, 4);
  Pool(int, 4) p = pool_init_static;
  triax_expect_eq((size_t)4, pool_cap(&p));
  triax_expect_eq((size_t)0, pool_used(&p));
  triax_expect_eq((size_t)4, pool_remaining(&p));
  triax_expect_true(pool_is_empty(&p));
  triax_expect_false(pool_is_full(&p));
}

triax_test(pool, dynamic_new_single_element) {
  POOL_DEFINE(int);
  Pool(int) p = pool_init_dynamic(int, 4);
  int* x      = pool_new(&p);
  triax_expect_nonnull(x);
  *x = 1234;
  triax_expect_eq((size_t)1, pool_used(&p));
  triax_expect_eq((size_t)3, pool_remaining(&p));
  triax_expect_false(pool_is_empty(&p));
  triax_expect_false(pool_is_full(&p));
  triax_expect_eq(1234, *x);
  pool_release(&p);
}

triax_test(pool, static_new_single_element) {
  POOL_DEFINE(int, 4);
  Pool(int, 4) p = pool_init_static;
  int* x         = pool_new(&p);
  triax_expect_nonnull(x);
  *x = 55;
  triax_expect_eq((size_t)1, pool_used(&p));
  triax_expect_eq((size_t)3, pool_remaining(&p));
  triax_expect_eq(55, *x);
}

triax_test(pool, dynamic_insert_copies_value) {
  POOL_DEFINE(PoolIntPair);
  Pool(PoolIntPair) p = pool_init_dynamic(PoolIntPair, 4);
  PoolIntPair  in     = {11, 22};
  PoolIntPair* out    = pool_put(&p, in);
  triax_expect_nonnull(out);
  triax_expect_eq(11, out->a);
  triax_expect_eq(22, out->b);
  triax_expect_eq((size_t)1, pool_used(&p));
  triax_expect_eq((size_t)3, pool_remaining(&p));
  pool_release(&p);
}

triax_test(pool, static_insert_copies_value) {
  POOL_DEFINE(PoolIntPair, 4);
  Pool(PoolIntPair, 4) p = pool_init_static;
  PoolIntPair in = {7, 9}, *out = pool_put(&p, in);
  triax_expect_nonnull(out);
  triax_expect_eq(7, out->a);
  triax_expect_eq(9, out->b);
  triax_expect_eq((size_t)1, pool_used(&p));
  triax_expect_eq((size_t)3, pool_remaining(&p));
}

triax_test(pool, dynamic_fill_to_capacity) {
  POOL_DEFINE(int);
  Pool(int) p = pool_init_dynamic(int, 4);
  int *a = pool_new(&p), *b = pool_new(&p), *c = pool_new(&p), *d = pool_new(&p);
  triax_expect_nonnull(a);
  triax_expect_nonnull(b);
  triax_expect_nonnull(c);
  triax_expect_nonnull(d);
  triax_expect_eq((size_t)4, pool_used(&p));
  triax_expect_eq((size_t)0, pool_remaining(&p));
  triax_expect_false(pool_is_empty(&p));
  triax_expect_true(pool_is_full(&p));
  pool_release(&p);
}

triax_test(pool, static_fill_to_capacity) {
  POOL_DEFINE(int, 4);
  Pool(int, 4) p = pool_init_static;
  int* a         = pool_new(&p);
  int* b         = pool_new(&p);
  int* c         = pool_new(&p);
  int* d         = pool_new(&p);
  triax_expect_nonnull(a);
  triax_expect_nonnull(b);
  triax_expect_nonnull(c);
  triax_expect_nonnull(d);
  triax_expect_eq((size_t)4, pool_used(&p));
  triax_expect_eq((size_t)0, pool_remaining(&p));
  triax_expect_true(pool_is_full(&p));
}

triax_test(pool, dynamic_new_crash, .isolation = TRIAX_ISOLATION_ON) {
  POOL_DEFINE(int);
  Pool(int) p = pool_init_dynamic(int, 4);
  triax_expect_nonnull(pool_new(&p));
  triax_expect_nonnull(pool_new(&p));
  triax_expect_nonnull(pool_new(&p));
  triax_expect_nonnull(pool_new(&p));
  triax_assert_fault(SIGABRT, (void)pool_new(&p));
}

triax_test(pool, static_new_crash, .isolation = TRIAX_ISOLATION_ON) {
  POOL_DEFINE(int, 4);
  Pool(int, 4) p = pool_init_static;
  triax_expect_nonnull(pool_new(&p));
  triax_expect_nonnull(pool_new(&p));
  triax_expect_nonnull(pool_new(&p));
  triax_expect_nonnull(pool_new(&p));
  triax_assert_fault(SIGABRT, (void)pool_new(&p));
}

triax_test(pool, dynamic_insert_crash, .isolation = TRIAX_ISOLATION_ON) {
  POOL_DEFINE(PoolIntPair);
  Pool(PoolIntPair) p = pool_init_dynamic(PoolIntPair, 4);
  PoolIntPair v       = {1, 2};
  triax_expect_nonnull(pool_put(&p, v));
  triax_expect_nonnull(pool_put(&p, v));
  triax_expect_nonnull(pool_put(&p, v));
  triax_expect_nonnull(pool_put(&p, v));
  triax_assert_fault(SIGABRT, (void)pool_new(&p));
}

triax_test(pool, delete_reclaims_slot_dynamic) {
  POOL_DEFINE(int);
  Pool(int) p = pool_init_dynamic(int, 4);
  int* a      = pool_new(&p);
  int* b      = pool_new(&p);
  int* c      = pool_new(&p);
  triax_expect_nonnull(a);
  triax_expect_nonnull(b);
  triax_expect_nonnull(c);
  *a = 10;
  *b = 20;
  *c = 30;
  pool_delete(&p, b);
  triax_expect_eq((size_t)2, pool_used(&p));
  triax_expect_eq((size_t)2, pool_remaining(&p));
  triax_expect_false(pool_is_full(&p));
  int* d = pool_new(&p);
  triax_expect_nonnull(d);
  *d = 99;
  triax_expect_eq((size_t)3, pool_used(&p));
  triax_expect_eq((size_t)1, pool_remaining(&p));
  triax_expect_eq(10, *a);
  triax_expect_eq(30, *c);
  triax_expect_eq(99, *d);
  pool_release(&p);
}

triax_test(pool, delete_reclaims_slot_static) {
  POOL_DEFINE(int, 4);
  Pool(int, 4) p = pool_init_static;
  int* a         = pool_new(&p);
  int* b         = pool_new(&p);
  int* c         = pool_new(&p);
  triax_expect_nonnull(a);
  triax_expect_nonnull(b);
  triax_expect_nonnull(c);
  *a = 1;
  *b = 2;
  *c = 3;
  pool_delete(&p, b);
  triax_expect_eq((size_t)2, pool_used(&p));
  triax_expect_eq((size_t)2, pool_remaining(&p));
  int* d = pool_new(&p);
  triax_expect_nonnull(d);
  *d = 42;
  triax_expect_eq((size_t)3, pool_used(&p));
  triax_expect_eq(1, *a);
  triax_expect_eq(3, *c);
  triax_expect_eq(42, *d);
}

triax_test(pool, clear_resets_usage_but_preserves_capacity_dynamic) {
  POOL_DEFINE(int);
  Pool(int) p = pool_init_dynamic(int, 8);
  int* a      = pool_new(&p);
  int* b      = pool_new(&p);
  triax_expect_nonnull(a);
  triax_expect_nonnull(b);
  *a             = 5;
  *b             = 6;
  Pool(int)* ret = pool_clear(&p);
  triax_expect_eq(&p, ret);
  triax_expect_eq((size_t)8, pool_cap(&p));
  triax_expect_eq((size_t)0, pool_used(&p));
  triax_expect_eq((size_t)8, pool_remaining(&p));
  triax_expect_true(pool_is_empty(&p));
  triax_expect_false(pool_is_full(&p));
  int* c = pool_new(&p);
  triax_expect_nonnull(c);
  *c = 77;
  triax_expect_eq(77, *c);
  triax_expect_eq((size_t)1, pool_used(&p));
  pool_release(&p);
}

triax_test(pool, clear_resets_usage_but_preserves_capacity_static) {
  POOL_DEFINE(int, 4);
  Pool(int, 4) p = pool_init_static;
  triax_expect_nonnull(pool_new(&p));
  triax_expect_nonnull(pool_new(&p));
  triax_expect_eq((size_t)2, pool_used(&p));
  Pool(int, 4)* ret = pool_clear(&p);
  triax_expect_eq(&p, ret);
  triax_expect_eq((size_t)4, pool_cap(&p));
  triax_expect_eq((size_t)0, pool_used(&p));
  triax_expect_eq((size_t)4, pool_remaining(&p));
  triax_expect_true(pool_is_empty(&p));
  triax_expect_false(pool_is_full(&p));
}

triax_test(pool, release_resets_dynamic_pool, .isolation = TRIAX_ISOLATION_ON) {
  POOL_DEFINE(int);
  Pool(int) p = pool_init_dynamic(int, 8);
  int* a      = pool_new(&p);
  int* b      = pool_new(&p);
  triax_expect_nonnull(a);
  triax_expect_nonnull(b);
  *a = 111;
  *b = 222;
  pool_release(&p);
  triax_expect_eq((size_t)0, pool_cap(&p));
  triax_expect_eq((size_t)0, pool_used(&p));
  triax_expect_eq((size_t)0, pool_remaining(&p));
  triax_expect_true(pool_is_empty(&p));
  triax_expect_true(pool_is_full(&p));
  triax_assert_fault(SIGABRT, (void)pool_new(&p));
}

triax_test(pool, release_resets_static_pool_usage) {
  POOL_DEFINE(int, 4);
  Pool(int, 4) p = pool_init_static;
  triax_expect_nonnull(pool_new(&p));
  triax_expect_nonnull(pool_new(&p));
  triax_expect_eq((size_t)2, pool_used(&p));
  pool_release(&p);
  triax_expect_eq((size_t)4, pool_cap(&p));
  triax_expect_eq((size_t)0, pool_used(&p));
  triax_expect_eq((size_t)4, pool_remaining(&p));
  triax_expect_true(pool_is_empty(&p));
  triax_expect_false(pool_is_full(&p));
  triax_expect_nonnull(pool_new(&p));
  triax_expect_eq((size_t)1, pool_used(&p));
}

triax_test(pool, foreach_visits_all_inserted_dynamic) {
  POOL_DEFINE(int);
  Pool(int) p = pool_init_dynamic(int, 8);
  int* a      = pool_put(&p, 3);
  int* b      = pool_put(&p, 5);
  int* c      = pool_put(&p, 7);
  triax_expect_nonnull(a);
  triax_expect_nonnull(b);
  triax_expect_nonnull(c);
  int sum   = 0;
  int count = 0;
  pool_foreach(&p, it) { sum += *it, ++count; }
  triax_expect_eq(15, sum);
  triax_expect_eq(3, count);
  pool_release(&p);
}

triax_test(pool, foreach_skips_deleted_slots_dynamic) {
  POOL_DEFINE(int);
  Pool(int) p = pool_init_dynamic(int, 8);
  int* a      = pool_put(&p, 10);
  int* b      = pool_put(&p, 20);
  int* c      = pool_put(&p, 30);
  int* d      = pool_put(&p, 40);
  triax_expect_nonnull(a);
  triax_expect_nonnull(b);
  triax_expect_nonnull(c);
  triax_expect_nonnull(d);
  pool_delete(&p, b);
  pool_delete(&p, d);
  int sum   = 0;
  int count = 0;
  pool_foreach(&p, it) { sum += *it, ++count; }
  triax_expect_eq(40, sum);
  triax_expect_eq(2, count);
  pool_release(&p);
}

triax_test(pool, foreach_allows_mutation_static) {
  POOL_DEFINE(int, 4);
  Pool(int, 4) p = pool_init_static;
  triax_expect_nonnull(pool_put(&p, 1));
  triax_expect_nonnull(pool_put(&p, 2));
  triax_expect_nonnull(pool_put(&p, 3));
  pool_foreach(&p, it) { *it *= 10; }
  int sum   = 0;
  int count = 0;
  pool_foreach(&p, it) { sum += *it, ++count; }
  triax_expect_eq(60, sum);
  triax_expect_eq(3, count);
}

triax_test(pool, foreach_empty_pool_is_noop_dynamic) {
  POOL_DEFINE(int);
  Pool(int) p = pool_init_dynamic(int, 4);
  int count   = 0;
  pool_foreach(&p, it) { (void)it, ++count; }
  triax_expect_eq(0, count);
  triax_expect_true(pool_is_empty(&p));
  pool_release(&p);
}

triax_test(pool, addresses_of_distinct_live_elements_are_distinct_dynamic) {
  POOL_DEFINE(int);
  Pool(int) p = pool_init_dynamic(int, 4);
  int* a      = pool_new(&p);
  int* b      = pool_new(&p);
  int* c      = pool_new(&p);
  triax_expect_nonnull(a);
  triax_expect_nonnull(b);
  triax_expect_nonnull(c);
  triax_expect_neq(a, b);
  triax_expect_neq(a, c);
  triax_expect_neq(b, c);
  pool_release(&p);
}

triax_test(pool, addresses_of_distinct_live_elements_are_distinct_static) {
  POOL_DEFINE(int, 4);
  Pool(int, 4) p = pool_init_static;
  int* a         = pool_new(&p);
  int* b         = pool_new(&p);
  int* c         = pool_new(&p);
  triax_expect_nonnull(a);
  triax_expect_nonnull(b);
  triax_expect_nonnull(c);
  triax_expect_neq(a, b);
  triax_expect_neq(a, c);
  triax_expect_neq(b, c);
}

triax_test(pool, pair_values_roundtrip_through_foreach_dynamic) {
  POOL_DEFINE(PoolIntPair);
  Pool(PoolIntPair) p = pool_init_dynamic(PoolIntPair, 4);
  triax_expect_nonnull(pool_put(&p, ((PoolIntPair){1, 2})));
  triax_expect_nonnull(pool_put(&p, ((PoolIntPair){3, 4})));
  triax_expect_nonnull(pool_put(&p, ((PoolIntPair){5, 6})));
  int sum_a = 0;
  int sum_b = 0;
  int count = 0;
  pool_foreach(&p, it) {
    sum_a += it->a;
    sum_b += it->b;
    ++count;
  }
  triax_expect_eq(9, sum_a);
  triax_expect_eq(12, sum_b);
  triax_expect_eq(3, count);
  pool_release(&p);
}

triax_test(pool, clear_after_full_allows_refill_dynamic) {
  POOL_DEFINE(int);
  Pool(int) p = pool_init_dynamic(int, 4);
  triax_expect_nonnull(pool_new(&p));
  triax_expect_nonnull(pool_new(&p));
  triax_expect_nonnull(pool_new(&p));
  triax_expect_nonnull(pool_new(&p));
  triax_expect_true(pool_is_full(&p));
  pool_clear(&p);
  triax_expect_true(pool_is_empty(&p));
  triax_expect_false(pool_is_full(&p));
  triax_expect_eq((size_t)4, pool_remaining(&p));
  triax_expect_nonnull(pool_new(&p));
  triax_expect_nonnull(pool_new(&p));
  triax_expect_eq((size_t)2, pool_used(&p));
  pool_release(&p);
}

triax_test(pool, clear_after_full_allows_refill_static) {
  POOL_DEFINE(int, 4);
  Pool(int, 4) p = pool_init_static;
  triax_expect_nonnull(pool_new(&p));
  triax_expect_nonnull(pool_new(&p));
  triax_expect_nonnull(pool_new(&p));
  triax_expect_nonnull(pool_new(&p));
  triax_expect_true(pool_is_full(&p));
  pool_clear(&p);
  triax_expect_true(pool_is_empty(&p));
  triax_expect_false(pool_is_full(&p));
  triax_expect_eq((size_t)4, pool_remaining(&p));
  triax_expect_nonnull(pool_new(&p));
  triax_expect_nonnull(pool_new(&p));
  triax_expect_eq((size_t)2, pool_used(&p));
}

// ---- pool_try_new ----

triax_test(pool, try_new_returns_null_when_full_dynamic) {
  POOL_DEFINE(int);
  Pool(int) p = pool_init_dynamic(int, 3);
  triax_expect_nonnull(pool_try_new(&p));
  triax_expect_nonnull(pool_try_new(&p));
  triax_expect_nonnull(pool_try_new(&p));
  triax_expect_true(pool_is_full(&p));
  triax_expect_null(pool_try_new(&p)); // full — no crash
  triax_expect_eq(pool_used(&p), (size_t)3);
  pool_release(&p);
}

triax_test(pool, try_new_returns_null_when_full_static) {
  POOL_DEFINE(int, 3);
  Pool(int, 3) p = pool_init_static;
  triax_expect_nonnull(pool_try_new(&p));
  triax_expect_nonnull(pool_try_new(&p));
  triax_expect_nonnull(pool_try_new(&p));
  triax_expect_true(pool_is_full(&p));
  triax_expect_null(pool_try_new(&p));
  triax_expect_eq(pool_used(&p), (size_t)3);
}

triax_test(pool, try_new_after_delete_succeeds_dynamic) {
  POOL_DEFINE(int);
  Pool(int) p = pool_init_dynamic(int, 2);
  int* a      = pool_try_new(&p);
  int* b      = pool_try_new(&p);
  triax_expect_nonnull(a);
  triax_expect_nonnull(b);
  triax_expect_null(pool_try_new(&p));

  pool_delete(&p, a);
  triax_expect_eq(pool_used(&p), (size_t)1);
  int* c = pool_try_new(&p);
  triax_expect_nonnull(c);
  triax_expect_eq(pool_used(&p), (size_t)2);
  triax_expect_null(pool_try_new(&p));
  pool_release(&p);
}

// ---- pool_try_put ----

triax_test(pool, try_put_returns_null_when_full_dynamic) {
  POOL_DEFINE(PoolIntPair);
  Pool(PoolIntPair) p = pool_init_dynamic(PoolIntPair, 2);
  PoolIntPair v       = {1, 2};
  triax_expect_nonnull(pool_try_put(&p, v));
  triax_expect_nonnull(pool_try_put(&p, v));
  triax_expect_true(pool_is_full(&p));
  triax_expect_null(pool_try_put(&p, v)); // full — no crash
  triax_expect_eq(pool_used(&p), (size_t)2);
  pool_release(&p);
}

triax_test(pool, try_put_returns_null_when_full_static) {
  POOL_DEFINE(PoolIntPair, 2);
  Pool(PoolIntPair, 2) p = pool_init_static;
  PoolIntPair v          = {3, 4};
  triax_expect_nonnull(pool_try_put(&p, v));
  triax_expect_nonnull(pool_try_put(&p, v));
  triax_expect_true(pool_is_full(&p));
  triax_expect_null(pool_try_put(&p, v));
  triax_expect_eq(pool_used(&p), (size_t)2);
}

triax_test(pool, try_put_copies_value_dynamic) {
  POOL_DEFINE(PoolIntPair);
  Pool(PoolIntPair) p = pool_init_dynamic(PoolIntPair, 4);
  PoolIntPair* out    = pool_try_put(&p, ((PoolIntPair){99, 77}));
  triax_expect_nonnull(out);
  triax_expect_eq(out->a, 99);
  triax_expect_eq(out->b, 77);
  triax_expect_eq(pool_used(&p), (size_t)1);
  pool_release(&p);
}

RK__IGNWARN_CLANG_END()
RK_HEADER_END
#endif
