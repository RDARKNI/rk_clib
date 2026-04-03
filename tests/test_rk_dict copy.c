#ifndef RK__TESTDUMMY
# include "../rk_test/rk_test.h"
#else
# include "../rk_test/rk_test_dummy.h"
#endif
#define RK_IMPL
#include "../include/rk_dict_exp.h"
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

typedef const char*     cstr;

extern_fun unsigned int str_hash(cstr s) {
  // djb2
  unsigned int h = 5381;
  for (; *s; s++) { h = ((h << 5) + h) ^ (unsigned char)(*s); }
  return h;
}
extern_fun int str_cmp(cstr a, cstr b) { return strcmp(a, b) != 0; }
// Instantiate the dict for int -> cstr
DICT_DEFINE(int, cstr, int_hash, int_cmp)
DICT_DEFINE(cstr, int, str_hash, str_cmp)
#define intcstr_dict Dict(int, cstr)
#define cstrint_dict Dict(cstr, int)

RK_REGISTER_TEST("dict", dict_tests1) {
  // ---- basic insert/get ----
  Dict(int, cstr) d1 = dict_init(intcstr_dict, 4);
  rk_expect_true(dict_set(intcstr_dict, &d1, 10, "ten"));
  rk_expect_true(dict_set(intcstr_dict, &d1, 20, "twenty"));
  rk_expect_streq(*dict_get(intcstr_dict, &d1, 10), "ten");

  // ---- update existing ----
  rk_expect_false(dict_set(intcstr_dict, &d1, 20, "twenty-updated"));
  rk_expect_streq(*dict_get(intcstr_dict, &d1, 20), "twenty-updated");

  // ---- contains / missing ----
  rk_expect_true(dict_contains(intcstr_dict, &d1, 10));
  rk_expect_false(dict_contains(intcstr_dict, &d1, 42));

  // ---- try_insert ----
  rk_expect_false(dict_add(intcstr_dict, &d1, 10, "fail")); // already exists
  rk_expect_true(dict_add(intcstr_dict, &d1, 30, "thirty"));
  rk_expect_streq(*dict_get(intcstr_dict, &d1, 30), "thirty");

  // ---- remove & reinsert ----
  rk_expect_true(dict_remove(intcstr_dict, &d1, 10));
  rk_expect_false(dict_contains(intcstr_dict, &d1, 10));
  rk_expect_true(dict_set(intcstr_dict, &d1, 10, "ten-again"));
  rk_expect_streq(*dict_get(intcstr_dict, &d1, 10), "ten-again");

  // ---- extract ----
  cstr out = rk_null;
  rk_expect_true(dict_extract(intcstr_dict, &d1, 30, &out));
  rk_expect_streq(out, "thirty");
  rk_expect_false(dict_contains(intcstr_dict, &d1, 30));

  // ---- clear ----
  dict_clear(intcstr_dict, &d1);
  rk_expect_eq(d1.count, 0);
  rk_expect_false(dict_contains(intcstr_dict, &d1, 10));

  // ---- growth test ----
  size_t init_cap = d1.cap;
  for (int i = 0; i < 1000; i++) {
    char* buf = (char*)malloc(32);
    snprintf(buf, 20, "val_%d", i);
    dict_set(intcstr_dict, &d1, i, buf);
  }
  rk_expect_eq(d1.count, 1000);
  rk_expect_true(d1.cap > init_cap); // grew
  rk_expect_streq(*dict_get(intcstr_dict, &d1, 123), "val_123");

  // ---- iteration correctness ----
  size_t seen = 0;
  dict_foreach(&d1, k, v) {
    rk_expect_nonnull(v);
    seen++;
  }
  rk_expect_eq(seen, d1.count);

  // ---- string→int dict ----
  Dict(cstr, int) d2 = dict_init(cstrint_dict, 2);
  rk_expect_true(dict_set(cstrint_dict, &d2, "apple", 1));
  rk_expect_true(dict_set(cstrint_dict, &d2, "banana", 2));
  rk_expect_true(dict_contains(cstrint_dict, &d2, "apple"));
  rk_expect_eq(*dict_get(cstrint_dict, &d2, "banana"), 2);

  // ---- delete non-existent ----
  rk_expect_false(dict_remove(cstrint_dict, &d2, "missing"));

  // ---- tombstone reuse ----
  rk_expect_true(dict_remove(cstrint_dict, &d2, "apple"));
  rk_expect_true(
      dict_set(cstrint_dict, &d2, "apricot", 3)); // should reuse slot
  rk_expect_true(dict_contains(cstrint_dict, &d2, "apricot"));

  // ---- reserve explicitly ----
  size_t old_cap = d2.cap;
  dict_reserve(cstrint_dict, &d2, 100);
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
  dict_release(intcstr_dict, &d1);
  dict_release(cstrint_dict, &d2);
}

RK_REGISTER_TEST("dict", dict_tests2) {
#define DT intcstr_dict
  // ---- init and insert ----
  Dict(int, cstr) dict = dict_init(DT, 8);
  rk_expect_eq(dict.count, 0);

  rk_expect_true(dict_set(DT, &dict, 1, "one"));
  rk_expect_true(dict_set(DT, &dict, 2, "two"));
  rk_expect_true(dict_set(DT, &dict, 3, "three"));
  rk_expect_eq(dict.count, 3);
  // ---- get / contains ----
  rk_expect_true(dict_contains(DT, &dict, 1));
  rk_expect_streq(*dict_get(DT, &dict, 2), "two");
  rk_expect_null(dict_get(DT, &dict, 42));

  // ---- update existing key ----
  rk_expect_false(dict_set(DT, &dict, 2, "two_updated")); // should update
  rk_expect_streq(*dict_get(DT, &dict, 2), "two_updated");

  // ---- try_insert ----
  rk_expect_false(dict_add(DT, &dict, 2, "two_fail")); // exists
  rk_expect_true(dict_add(DT, &dict, 4, "four"));      // new
  rk_expect_eq(dict.count, 4);

  // ---- remove ----
  rk_expect_true(dict_remove(DT, &dict, 3));
  rk_expect_false(dict_contains(DT, &dict, 3));
  rk_expect_eq(dict.count, 3);

  // ---- extract ----
  const char* out = rk_null;
  rk_expect_true(dict_extract(DT, &dict, 4, &out));
  rk_expect_streq(out, "four");
  rk_expect_false(dict_contains(DT, &dict, 4));
  rk_expect_eq(dict.count, 2);

  // ---- clear ----
  dict_clear(DT, &dict);
  rk_expect_eq(dict.count, 0);

  // ---- reserve/grow ----
  for (int i = 0; i < 100; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "val_%d", i);
    dict_set(DT, &dict, i, strdup(buf)); // strdup for safety
  }
  rk_expect_eq(dict.count, 100);
  rk_expect_true(dict_contains(DT, &dict, 42));

  // ---- foreach ----
  int seen = 0;

  dict_foreach(&dict, k, v) {
    rk_expect_nonnull(*v);
    seen++;
  }

  // dict_foreach_key(&dict, k) { *k = (typeof(*k)){0}; }

  rk_expect_eq(seen, dict.count);
  dict_foreach_val(&dict, v) { *v = (typeof(*v)){0}; }

  dict_release(DT, &dict);
}

RK__IGNWARN_CLANG_END()
RK_HEADER_END
