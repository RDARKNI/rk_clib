#ifndef RK__TESTDUMMY
# include "../rk_test/rk_test.h"
#else
# include "../rk_test/rk_test_dummy.h"
#endif
#define RK_IMPL
#include "../include/rklib_includeall.h"
RK_HEADER_BEGIN
RK__IGNWARN_CLANG_BEG("-Wunused-variable")

// ---- Define concrete key/value dict: int -> const char* ----
typedef const char*     cstr;

extern_fun unsigned int int_hash(int key) {
  // simple multiplicative hash
  return (unsigned int)key * 2654435761u;
}

extern_fun int int_cmp(int a, int b) {
  return a != b; // returns 0 if equal
}

// Instantiate the dict for int -> cstr
DICT_DEFINE(int, cstr, int_hash, int_cmp)

typedef const char*     cstr;

extern_fun unsigned int str_hash(cstr s) {
  // djb2
  unsigned int h = 5381;
  for (; *s; s++) { h = ((h << 5) + h) ^ (unsigned char)(*s); }
  return h;
}
extern_fun int str_cmp(cstr a, cstr b) { return strcmp(a, b) != 0; }
// Instantiate two dicts
DICT_DEFINE(cstr, int, str_hash, str_cmp)

RK_REGISTER_TEST("dict", dict_tests1) {
  // ---- basic insert/get ----
  Dict(int, cstr) d1 = dict_init(int, cstr, 4);
  rk_expect_true(dict_set(int, cstr, &d1, 10, "ten"));
  rk_expect_true(dict_set(int, cstr, &d1, 20, "twenty"));
  rk_expect_streq(*dict_get(int, cstr, &d1, 10), "ten");

  // ---- update existing ----
  rk_expect_false(dict_set(int, cstr, &d1, 20, "twenty-updated"));
  rk_expect_streq(*dict_get(int, cstr, &d1, 20), "twenty-updated");

  // ---- contains / missing ----
  rk_expect_true(dict_contains(int, cstr, &d1, 10));
  rk_expect_false(dict_contains(int, cstr, &d1, 42));

  // ---- try_insert ----
  rk_expect_false(dict_add(int, cstr, &d1, 10, "fail")); // already exists
  rk_expect_true(dict_add(int, cstr, &d1, 30, "thirty"));
  rk_expect_streq(*dict_get(int, cstr, &d1, 30), "thirty");

  // ---- remove & reinsert ----
  rk_expect_true(dict_remove(int, cstr, &d1, 10));
  rk_expect_false(dict_contains(int, cstr, &d1, 10));
  rk_expect_true(dict_set(int, cstr, &d1, 10, "ten-again"));
  rk_expect_streq(*dict_get(int, cstr, &d1, 10), "ten-again");

  // ---- extract ----
  cstr out = rk_null;
  rk_expect_true(dict_extract(int, cstr, &d1, 30, &out));
  rk_expect_streq(out, "thirty");
  rk_expect_false(dict_contains(int, cstr, &d1, 30));

  // ---- clear ----
  dict_clear(int, cstr, &d1);
  rk_expect_eq(d1.count, 0);
  rk_expect_false(dict_contains(int, cstr, &d1, 10));

  // ---- growth test ----
  size_t init_cap = d1.cap;
  for (int i = 0; i < 1000; i++) {
    char* buf = (char*)malloc(32);
    snprintf(buf, 20, "val_%d", i);
    dict_set(int, cstr, &d1, i, buf);
  }
  rk_expect_eq(d1.count, 1000);
  rk_expect_true(d1.cap > init_cap); // grew
  rk_expect_streq(*dict_get(int, cstr, &d1, 123), "val_123");

  // ---- iteration correctness ----
  size_t seen = 0;
  dict_foreach(&d1, k, v) {
    rk_expect_nonnull(v);
    seen++;
  }
  rk_expect_eq(seen, d1.count);

  // ---- string→int dict ----
  Dict(cstr, int) d2 = dict_init(cstr, int, 2);
  rk_expect_true(dict_set(cstr, int, &d2, "apple", 1));
  rk_expect_true(dict_set(cstr, int, &d2, "banana", 2));
  rk_expect_true(dict_contains(cstr, int, &d2, "apple"));
  rk_expect_eq(*dict_get(cstr, int, &d2, "banana"), 2);

  // ---- delete non-existent ----
  rk_expect_false(dict_remove(cstr, int, &d2, "missing"));

  // ---- tombstone reuse ----
  rk_expect_true(dict_remove(cstr, int, &d2, "apple"));
  rk_expect_true(dict_set(cstr, int, &d2, "apricot", 3)); // should reuse slot
  rk_expect_true(dict_contains(cstr, int, &d2, "apricot"));

  // ---- reserve explicitly ----
  size_t old_cap = d2.cap;
  dict_reserve(cstr, int, &d2, 100);
  rk_expect_true(d2.cap >= 100);
  rk_expect_true(d2.cap >= old_cap);

  // ---- iteration ----
  seen = 0;
  dict_foreach(&d2, k, v) {
    rk_expect_true(k && v);
    seen++;
  }

  rk_expect_eq(seen, d2.count);

  // ---- cleanup ----
  dict_release(int, cstr, &d1);
  dict_release(cstr, int, &d2);
}

RK_REGISTER_TEST("dict", dict_tests2) {
  // ---- init and insert ----
  Dict(int, cstr) dict = dict_init(int, cstr, 8);
  rk_expect_eq(dict.count, 0);

  rk_expect_true(dict_set(int, cstr, &dict, 1, "one"));
  rk_expect_true(dict_set(int, cstr, &dict, 2, "two"));
  rk_expect_true(dict_set(int, cstr, &dict, 3, "three"));
  rk_expect_eq(dict.count, 3);

  // ---- get / contains ----
  rk_expect_true(dict_contains(int, cstr, &dict, 1));
  rk_expect_streq(*dict_get(int, cstr, &dict, 2), "two");
  rk_expect_null(dict_get(int, cstr, &dict, 42));

  // ---- update existing key ----
  rk_expect_false(
      dict_set(int, cstr, &dict, 2, "two_updated")); // should update
  rk_expect_streq(*dict_get(int, cstr, &dict, 2), "two_updated");

  // ---- try_insert ----
  rk_expect_false(dict_add(int, cstr, &dict, 2, "two_fail")); // exists
  rk_expect_true(dict_add(int, cstr, &dict, 4, "four"));      // new
  rk_expect_eq(dict.count, 4);

  // ---- remove ----
  rk_expect_true(dict_remove(int, cstr, &dict, 3));
  rk_expect_false(dict_contains(int, cstr, &dict, 3));
  rk_expect_eq(dict.count, 3);

  // ---- extract ----
  const char* out = rk_null;
  rk_expect_true(dict_extract(int, cstr, &dict, 4, &out));
  rk_expect_streq(out, "four");
  rk_expect_false(dict_contains(int, cstr, &dict, 4));
  rk_expect_eq(dict.count, 2);

  // ---- clear ----
  dict_clear(int, cstr, &dict);
  rk_expect_eq(dict.count, 0);

  // ---- reserve/grow ----
  for (int i = 0; i < 100; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "val_%d", i);
    dict_set(int, cstr, &dict, i, strdup(buf)); // strdup for safety
  }
  rk_expect_eq(dict.count, 100);
  rk_expect_true(dict_contains(int, cstr, &dict, 42));

  // ---- foreach ----
  int seen = 0;

  dict_foreach(&dict, k, v) {
    rk_expect_nonnull(*v);
    seen++;
  }

  // dict_foreach_key(&dict, k) { *k = (typeof(*k)){0}; }

  rk_expect_eq(seen, dict.count);
  dict_foreach_val(&dict, v) { *v = (typeof(*v)){0}; }

  dict_release(int, cstr, &dict);
}

RK_REGISTER_TEST("dict", dict_count_is_empty_cap_load_factor) {
  Dict(int, cstr) d = dict_init(int, cstr, 16);
  rk_expect_eq(dict_count(&d), 0u);
  rk_expect_true(dict_is_empty(&d));
  rk_expect_true(dict_cap(&d) >= 16u);
  // load factor is 0 on empty dict
  rk_expect_eq(dict_load_factor(&d), 0.0f);

  dict_set(int, cstr, &d, 1, "a");
  dict_set(int, cstr, &d, 2, "b");
  rk_expect_eq(dict_count(&d), 2u);
  rk_expect_false(dict_is_empty(&d));
  rk_expect_true(dict_load_factor(&d) > 0.0f);
  rk_expect_true(dict_load_factor(&d) <= 1.0f);

  dict_release(int, cstr, &d);
}

RK_REGISTER_TEST("dict", dict_foreach_key_visits_all_keys) {
  Dict(int, cstr) d = dict_init(int, cstr, 8);
  dict_set(int, cstr, &d, 10, "ten");
  dict_set(int, cstr, &d, 20, "twenty");
  dict_set(int, cstr, &d, 30, "thirty");

  int seen_10 = 0, seen_20 = 0, seen_30 = 0, count = 0;
  dict_foreach_key(&d, k) {
    ++count;
    if (*k == 10) { ++seen_10; }
    if (*k == 20) { ++seen_20; }
    if (*k == 30) { ++seen_30; }
  }
  rk_expect_eq(count, 3);
  rk_expect_eq(seen_10, 1);
  rk_expect_eq(seen_20, 1);
  rk_expect_eq(seen_30, 1);

  dict_release(int, cstr, &d);
}

RK_REGISTER_TEST("dict", dict_foreach_val_visits_all_values) {
  Dict(int, cstr) d = dict_init(int, cstr, 8);
  dict_set(int, cstr, &d, 1, "one");
  dict_set(int, cstr, &d, 2, "two");
  dict_set(int, cstr, &d, 3, "three");

  int count = 0;
  dict_foreach_val(&d, v) {
    rk_expect_nonnull(v);
    rk_expect_nonnull(*v);
    ++count;
  }
  rk_expect_eq(count, 3);

  // Mutate all values via foreach_val
  dict_foreach_val(&d, v) { *v = "x"; }
  rk_expect_streq(*dict_get(int, cstr, &d, 1), "x");
  rk_expect_streq(*dict_get(int, cstr, &d, 2), "x");
  rk_expect_streq(*dict_get(int, cstr, &d, 3), "x");

  dict_release(int, cstr, &d);
}

RK_REGISTER_TEST("dict", dict_foreach_key_val_empty_are_noop) {
  Dict(int, cstr) d     = dict_init(int, cstr, 8);
  int             count = 0;
  dict_foreach_key(&d, k) {
    (void)k;
    ++count;
  }
  rk_expect_eq(count, 0);
  dict_foreach_val(&d, v) {
    (void)v;
    ++count;
  }
  rk_expect_eq(count, 0);
  dict_release(int, cstr, &d);
}

RK_REGISTER_TEST("dict", dict_ops_on_empty_dict) {
  Dict(int, cstr) d = dict_init(int, cstr, 4);
  rk_expect_null(dict_get(int, cstr, &d, 42));
  rk_expect_false(dict_contains(int, cstr, &d, 42));
  rk_expect_false(dict_remove(int, cstr, &d, 42));
  cstr out = rk_null;
  rk_expect_false(dict_extract(int, cstr, &d, 42, &out));
  rk_expect_null(out);
  rk_expect_eq(dict_count(&d), 0u);
  dict_release(int, cstr, &d);
}

RK_REGISTER_TEST("dict", dict_reserve_noop_when_smaller) {
  Dict(int, cstr) d            = dict_init(int, cstr, 64);
  size_t          original_cap = dict_cap(&d);
  dict_reserve(int, cstr, &d, 4); // smaller than current — no change
  rk_expect_eq(dict_cap(&d), original_cap);
  dict_reserve(int, cstr, &d, original_cap); // equal — no change
  rk_expect_eq(dict_cap(&d), original_cap);
  dict_reserve(int, cstr, &d, original_cap * 2); // larger — grows
  rk_expect_true(dict_cap(&d) >= original_cap * 2);
  dict_release(int, cstr, &d);
}

RK__IGNWARN_CLANG_END()
RK_HEADER_END
