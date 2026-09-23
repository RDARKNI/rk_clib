#ifndef TEST_DICT_H
#define TEST_DICT_H
#include "conf.h"

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

triax_test(dict, null) {
  Dict(int, cstr) d = {RK_ZINIT};
  triax_expect_eq(dict_cap(&d), 0u);
  triax_expect_false(dict_contains(int, cstr, &d, 0));
  triax_expect_eq(dict_count(&d), 0u);
  triax_expect_null(dict_get(int, cstr, &d, 10));
  triax_expect_true(dict_is_empty(&d));
  triax_assert_false(dict_remove(int, cstr, &d, 10));
  cstr ext;
  triax_assert_false(dict_extract(int, cstr, &d, 10, &ext));
  dict_clear(int, cstr, &d);
  dict_release(int, cstr, &d);
  triax_assert_nonnull(dict_add(int, cstr, &d, 0, "ok"));
  triax_assert_true(dict_contains(int, cstr, &d, 0));
  dict_release(int, cstr, &d);
  d = (Dict(int, cstr)){RK_ZINIT};
  triax_assert_true(dict_set(int, cstr, &d, 0, "ok"));
  triax_assert_true(dict_contains(int, cstr, &d, 0));
  dict_release(int, cstr, &d);
  d = (Dict(int, cstr)){RK_ZINIT};
  bool inserted;
  triax_assert_true(dict_get_or_add(int, cstr, &d, 0, "ok", &inserted));
}
triax_test(dict, tests1) {
  // ---- basic insert/get ----
  Dict(int, cstr) d1 = dict_init(int, cstr, 4);
  triax_expect_true(dict_set(int, cstr, &d1, 10, "ten"));
  triax_expect_true(dict_set(int, cstr, &d1, 20, "twenty"));
  triax_expect_streq(*dict_get(int, cstr, &d1, 10), "ten");

  // ---- update existing ----
  triax_expect_false(dict_set(int, cstr, &d1, 20, "twenty-updated"));
  triax_expect_streq(*dict_get(int, cstr, &d1, 20), "twenty-updated");

  // ---- contains / missing ----
  triax_expect_true(dict_contains(int, cstr, &d1, 10));
  triax_expect_false(dict_contains(int, cstr, &d1, 42));

  // ---- try_insert ----
  triax_expect_false(dict_add(int, cstr, &d1, 10, "fail")); // already exists
  triax_expect_true(dict_add(int, cstr, &d1, 30, "thirty"));
  triax_expect_streq(*dict_get(int, cstr, &d1, 30), "thirty");

  // ---- remove & reinsert ----
  triax_expect_true(dict_remove(int, cstr, &d1, 10));
  triax_expect_false(dict_contains(int, cstr, &d1, 10));
  triax_expect_true(dict_set(int, cstr, &d1, 10, "ten-again"));
  triax_expect_streq(*dict_get(int, cstr, &d1, 10), "ten-again");

  // ---- extract ----
  cstr out = rk_null;
  triax_expect_true(dict_extract(int, cstr, &d1, 30, &out));
  triax_expect_streq(out, "thirty");
  triax_expect_false(dict_contains(int, cstr, &d1, 30));

  // ---- clear ----
  dict_clear(int, cstr, &d1);
  triax_expect_eq(d1.count, 0);
  triax_expect_false(dict_contains(int, cstr, &d1, 10));

  // ---- growth test ----
  size_t init_cap = d1.cap;
  for (int i = 0; i < 1000; i++) {
    char* buf = (char*)malloc(32);
    snprintf(buf, 20, "val_%d", i);
    dict_set(int, cstr, &d1, i, buf);
  }
  triax_expect_eq(d1.count, 1000);
  triax_expect_true(d1.cap > init_cap); // grew
  triax_expect_streq(*dict_get(int, cstr, &d1, 123), "val_123");

  // ---- iteration correctness ----
  size_t seen = 0;
  dict_foreach(&d1, k, v) {
    triax_expect_nonnull(v);
    seen++;
  }
  triax_expect_eq(seen, d1.count);

  // ---- string→int dict ----
  Dict(cstr, int) d2 = dict_init(cstr, int, 2);
  triax_expect_true(dict_set(cstr, int, &d2, "apple", 1));
  triax_expect_true(dict_set(cstr, int, &d2, "banana", 2));
  triax_expect_true(dict_contains(cstr, int, &d2, "apple"));
  triax_expect_eq(*dict_get(cstr, int, &d2, "banana"), 2);

  // ---- delete non-existent ----
  triax_expect_false(dict_remove(cstr, int, &d2, "missing"));

  // ---- tombstone reuse ----
  triax_expect_true(dict_remove(cstr, int, &d2, "apple"));
  triax_expect_true(dict_set(cstr, int, &d2, "apricot", 3)); // should reuse slot
  triax_expect_true(dict_contains(cstr, int, &d2, "apricot"));

  // ---- reserve explicitly ----
  size_t old_cap = d2.cap;
  dict_reserve(cstr, int, &d2, 100);
  triax_expect_true(d2.cap >= 100);
  triax_expect_true(d2.cap >= old_cap);

  // ---- iteration ----
  seen = 0;
  dict_foreach(&d2, k, v) {
    triax_expect_true(k && v);
    seen++;
  }

  triax_expect_eq(seen, d2.count);

  // ---- cleanup ----
  dict_release(int, cstr, &d1);
  dict_release(cstr, int, &d2);
}

triax_test(dict, tests2) {
  // ---- init and insert ----
  Dict(int, cstr) dict = dict_init(int, cstr, 8);
  triax_expect_eq(dict.count, 0);

  triax_expect_true(dict_set(int, cstr, &dict, 1, "one"));
  triax_expect_true(dict_set(int, cstr, &dict, 2, "two"));
  triax_expect_true(dict_set(int, cstr, &dict, 3, "three"));
  triax_expect_eq(dict.count, 3);

  // ---- get / contains ----
  triax_expect_true(dict_contains(int, cstr, &dict, 1));
  triax_expect_streq(*dict_get(int, cstr, &dict, 2), "two");
  triax_expect_null(dict_get(int, cstr, &dict, 42));

  // ---- update existing key ----
  triax_expect_false(dict_set(int, cstr, &dict, 2, "two_updated")); // should update
  triax_expect_streq(*dict_get(int, cstr, &dict, 2), "two_updated");

  // ---- try_insert ----
  triax_expect_false(dict_add(int, cstr, &dict, 2, "two_fail")); // exists
  triax_expect_true(dict_add(int, cstr, &dict, 4, "four"));      // new
  triax_expect_eq(dict.count, 4);

  // ---- remove ----
  triax_expect_true(dict_remove(int, cstr, &dict, 3));
  triax_expect_false(dict_contains(int, cstr, &dict, 3));
  triax_expect_eq(dict.count, 3);

  // ---- extract ----
  const char* out = rk_null;
  triax_expect_true(dict_extract(int, cstr, &dict, 4, &out));
  triax_expect_streq(out, "four");
  triax_expect_false(dict_contains(int, cstr, &dict, 4));
  triax_expect_eq(dict.count, 2);

  // ---- clear ----
  dict_clear(int, cstr, &dict);
  triax_expect_eq(dict.count, 0);

  // ---- reserve/grow ----
  for (int i = 0; i < 100; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "val_%d", i);
    dict_set(int, cstr, &dict, i, strdup(buf)); // strdup for safety
  }
  triax_expect_eq(dict.count, 100);
  triax_expect_true(dict_contains(int, cstr, &dict, 42));

  // ---- foreach ----
  size_t seen = 0;

  dict_foreach(&dict, k, v) {
    triax_expect_nonnull(*v);
    seen++;
  }

  // dict_foreach_key(&dict, k) { *k = (typeof(*k)){0}; }

  triax_expect_eq(seen, dict.count);
  dict_foreach_val(&dict, v) { *v = (typeof(*v)){0}; }

  dict_release(int, cstr, &dict);
}

triax_test(dict, count_is_empty_cap_load_factor) {
  Dict(int, cstr) d = dict_init(int, cstr, 16);
  triax_expect_eq(dict_count(&d), 0u);
  triax_expect_true(dict_is_empty(&d));
  triax_expect_true(dict_cap(&d) >= 16u);
  // load factor is 0 on empty dict
  triax_expect_eq(dict_load_factor(&d), 0.0f);

  dict_set(int, cstr, &d, 1, "a");
  dict_set(int, cstr, &d, 2, "b");
  triax_expect_eq(dict_count(&d), 2u);
  triax_expect_false(dict_is_empty(&d));
  triax_expect_true(dict_load_factor(&d) > 0.0f);
  triax_expect_true(dict_load_factor(&d) <= 1.0f);

  dict_release(int, cstr, &d);
}

triax_test(dict, foreach_key_visits_all_keys) {
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
  triax_expect_eq(count, 3);
  triax_expect_eq(seen_10, 1);
  triax_expect_eq(seen_20, 1);
  triax_expect_eq(seen_30, 1);

  dict_release(int, cstr, &d);
}

triax_test(dict, foreach_val_visits_all_values) {
  Dict(int, cstr) d = dict_init(int, cstr, 8);
  dict_set(int, cstr, &d, 1, "one");
  dict_set(int, cstr, &d, 2, "two");
  dict_set(int, cstr, &d, 3, "three");

  int count = 0;
  dict_foreach_val(&d, v) {
    triax_expect_nonnull(v);
    triax_expect_nonnull(*v);
    ++count;
  }
  triax_expect_eq(count, 3);

  // Mutate all values via foreach_val
  dict_foreach_val(&d, v) { *v = "x"; }
  triax_expect_streq(*dict_get(int, cstr, &d, 1), "x");
  triax_expect_streq(*dict_get(int, cstr, &d, 2), "x");
  triax_expect_streq(*dict_get(int, cstr, &d, 3), "x");

  dict_release(int, cstr, &d);
}

triax_test(dict, foreach_key_val_empty_are_noop) {
  Dict(int, cstr) d     = dict_init(int, cstr, 8);
  int             count = 0;
  dict_foreach_key(&d, k) {
    (void)k;
    ++count;
  }
  triax_expect_eq(count, 0);
  dict_foreach_val(&d, v) {
    (void)v;
    ++count;
  }
  triax_expect_eq(count, 0);
  dict_release(int, cstr, &d);
}

triax_test(dict, ops_on_empty_dict) {
  Dict(int, cstr) d = dict_init(int, cstr, 4);
  triax_expect_null(dict_get(int, cstr, &d, 42));
  triax_expect_false(dict_contains(int, cstr, &d, 42));
  triax_expect_false(dict_remove(int, cstr, &d, 42));
  cstr out = rk_null;
  triax_expect_false(dict_extract(int, cstr, &d, 42, &out));
  triax_expect_null(out);
  triax_expect_eq(dict_count(&d), 0u);
  dict_release(int, cstr, &d);
}

triax_test(dict, reserve_noop_when_smaller) {
  Dict(int, cstr) d            = dict_init(int, cstr, 64);
  size_t          original_cap = dict_cap(&d);
  dict_reserve(int, cstr, &d, 4); // smaller than current — no change
  triax_expect_eq(dict_cap(&d), original_cap);
  dict_reserve(int, cstr, &d, original_cap); // equal — no change
  triax_expect_eq(dict_cap(&d), original_cap);
  dict_reserve(int, cstr, &d, original_cap * 2); // larger — grows
  triax_expect_true(dict_cap(&d) >= original_cap * 2);
  dict_release(int, cstr, &d);
}

static inline int rk_set_int_cmp(int x, int y) { return x != y; }
static inline int rk_set_int_hash(int x) { return x; }

SET_DEFINE(int, rk_set_int_hash, rk_set_int_cmp)
typedef unsigned char uchar;

static inline int     rk_uchar_cmp(uchar x, uchar y) { return x != y; }
static inline int     rk_uchar_hash(uchar x) { return x; }
SET_DEFINE(uchar, rk_uchar_hash, rk_uchar_cmp)
triax_test(set, tests0) {
  Set(uchar) s = {0};
  triax_assert_eq(set_cap(&s), 0u);
  triax_assert_eq(set_count(&s), 0u);
  set_clear(uchar, &s);
  set_release(uchar, &s);
  s = set_init(uchar, 256);
  for (int i = 0; i < 256; ++i) {
    set_add(uchar, &s, (uchar)i);
    triax_assert_true(set_contains(uchar, &s, (uchar)i));
  }
  triax_assert_eq(set_count(&s), 256);
  for (int i = 0; i < 256; ++i) { set_add(uchar, &s, (uchar)i); }
  triax_assert_eq(set_count(&s), 256);
}
triax_test(arrdup, t0) {
  const int src[5] = {1, 2, 3, 4, 5};
  int*      dst    = rk_arrdup(src, 5);
  triax_expect_memeq(dst, src, sizeof(src));
  int* dst2 = rk_arrdup(dst, 5);
  triax_expect_memeq(dst2, src, sizeof(src));
  triax_expect_neq(dst2, dst); // must be a distinct allocation, not an alias
  alloc_delete(dst, 5);
  alloc_delete(dst2, 5);
}
RK__IGNWARN_CLANG_END()
RK_HEADER_END
#endif
