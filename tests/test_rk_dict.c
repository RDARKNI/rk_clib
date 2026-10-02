#ifndef TEST_DICT_H
#define TEST_DICT_H
#include "conf.h"
#include "rk_dict.h"
#include <limits.h>

RK_HEADER_BEGIN
RKI_IGNWARN_CLANG_BEG("-Wunused-variable")

// ---- Define concrete key/value dict: int -> const char* ----
typedef const char* cstr;

extern_fun unsigned int_hash(int key) {
  return (unsigned)key * 2654435761u;
} // simple multiplicative hash
extern_fun int int_cmp(int a, int b) { return a != b; } // returns 0 if equal
DICT_DEFINE(int, cstr, int_hash, int_cmp)

extern_fun unsigned int str_hash(cstr s) {
  unsigned int h = 5381; // djb2
  for (; *s; s++) { h = ((h << 5) + h) ^ (unsigned char)(*s); }
  return h;
}
extern_fun int str_cmp(cstr a, cstr b) { return strcmp(a, b) != 0; }
DICT_DEFINE(cstr, int, str_hash, str_cmp)

triax_test(dict, contains) {
  Dict(int, cstr) d = {RK_ZINIT};
  triax_assert_false(dict_contains(int, cstr, &d, 0));
  dict_add(int, cstr, &d, 0, "hello");
  triax_assert_true(dict_contains(int, cstr, &d, 0));
}

triax_test(dict, cap) {
  Dict(int, cstr) d = {RK_ZINIT};
  triax_assert_false(dict_contains(int, cstr, &d, 0));
  dict_add(int, cstr, &d, 0, "hello");
  triax_assert_true(dict_contains(int, cstr, &d, 0));
}
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

triax_test(dict, zero_initialized) {
  Dict(int, cstr) d = {RK_ZINIT};
  triax_expect_true(dict_is_empty(&d));
  triax_expect_eq(dict_count(&d), 0u);
  triax_expect_eq(dict_cap(&d), 0u);
  RK_IFALLOC(triax_expect_memeq((Allocator[]){dict_allocator(&d)}, &alloc_ctx, sizeof(alloc_ctx));)

  triax_expect_true(dict_set(int, cstr, &d, 7, "seven"));
  triax_expect_eq(dict_count(&d), 1u);
  triax_expect_true(dict_contains(int, cstr, &d, 7));
  triax_expect_streq(*dict_get(int, cstr, &d, 7), "seven");

  dict_release(int, cstr, &d);
  triax_expect_true(dict_is_empty(&d));
}

triax_test(dict, set_get_update) {
  Dict(int, cstr) d = dict_init(int, cstr, 4);
  triax_expect_true(dict_set(int, cstr, &d, 10, "ten"));
  triax_expect_true(dict_set(int, cstr, &d, 20, "twenty"));
  triax_expect_streq(*dict_get(int, cstr, &d, 10), "ten");

  // set on an existing key updates it and reports "not a new insert"
  triax_expect_false(dict_set(int, cstr, &d, 20, "twenty-updated"));
  triax_expect_streq(*dict_get(int, cstr, &d, 20), "twenty-updated");

  triax_expect_true(dict_contains(int, cstr, &d, 10));
  triax_expect_false(dict_contains(int, cstr, &d, 42));

  dict_release(int, cstr, &d);
}

triax_test(dict, add_does_not_overwrite) {
  Dict(int, cstr) d = dict_init(int, cstr, 4);
  dict_set(int, cstr, &d, 10, "ten");

  triax_expect_false(dict_add(int, cstr, &d, 10, "fail")); // already exists
  triax_expect_streq(*dict_get(int, cstr, &d, 10), "ten");
  triax_expect_true(dict_add(int, cstr, &d, 30, "thirty")); // new key
  triax_expect_streq(*dict_get(int, cstr, &d, 30), "thirty");

  dict_release(int, cstr, &d);
}

triax_test(dict, remove_and_reinsert) {
  Dict(int, cstr) d = dict_init(int, cstr, 4);
  dict_set(int, cstr, &d, 10, "ten");

  triax_expect_true(dict_remove(int, cstr, &d, 10));
  triax_expect_false(dict_contains(int, cstr, &d, 10));
  triax_expect_true(dict_set(int, cstr, &d, 10, "ten-again"));
  triax_expect_streq(*dict_get(int, cstr, &d, 10), "ten-again");

  dict_release(int, cstr, &d);
}

// Removing a key marks its slot a tombstone rather than freeing it immediately; a subsequent
// insert must be able to reuse that slot rather than leaking probe-chain length forever.
triax_test(dict, remove_then_reinsert_reuses_tombstone_slot) {
  Dict(int, cstr) d = dict_init(int, cstr, 4);
  dict_set(int, cstr, &d, 1, "apple");

  triax_expect_true(dict_remove(int, cstr, &d, 1));
  triax_expect_true(dict_set(int, cstr, &d, 2, "apricot")); // should reuse the tombstoned slot
  triax_expect_true(dict_contains(int, cstr, &d, 2));
  triax_expect_false(dict_contains(int, cstr, &d, 1));

  dict_release(int, cstr, &d);
}

triax_test(dict, extract) {
  Dict(int, cstr) d = dict_init(int, cstr, 4);
  dict_set(int, cstr, &d, 30, "thirty");

  cstr out = rk_null;
  triax_expect_true(dict_extract(int, cstr, &d, 30, &out));
  triax_expect_streq(out, "thirty");
  triax_expect_false(dict_contains(int, cstr, &d, 30));

  dict_release(int, cstr, &d);
}

triax_test(dict, clear_resets_count) {
  Dict(int, cstr) d = dict_init(int, cstr, 4);
  dict_set(int, cstr, &d, 10, "ten");
  dict_set(int, cstr, &d, 20, "twenty");

  dict_clear(int, cstr, &d);
  triax_expect_eq(dict_count(&d), 0u);
  triax_expect_false(dict_contains(int, cstr, &d, 10));

  dict_release(int, cstr, &d);
}

triax_test(dict, growth_preserves_entries) {
  static char     bufs[1000][20];
  Dict(int, cstr) d        = dict_init(int, cstr, 4);
  size_t          init_cap = dict_cap(&d);

  for (int i = 0; i < 1000; ++i) {
    snprintf(bufs[i], sizeof(bufs[i]), "val_%d", i);
    dict_set(int, cstr, &d, i, bufs[i]);
  }
  triax_expect_eq(dict_count(&d), 1000u);
  triax_expect_true(dict_cap(&d) > init_cap);
  triax_expect_streq(*dict_get(int, cstr, &d, 123), "val_123");

  size_t seen = 0;
  dict_foreach(&d, k, v) {
    (void)k;
    triax_expect_nonnull(v);
    ++seen;
  }
  triax_expect_eq(seen, dict_count(&d));

  dict_release(int, cstr, &d);
}

triax_test(dict, string_keyed_basic_ops) {
  Dict(cstr, int) d = dict_init(cstr, int, 2);
  triax_expect_true(dict_set(cstr, int, &d, "apple", 1));
  triax_expect_true(dict_set(cstr, int, &d, "banana", 2));
  triax_expect_true(dict_contains(cstr, int, &d, "apple"));
  triax_expect_eq(*dict_get(cstr, int, &d, "banana"), 2);
  triax_expect_false(dict_remove(cstr, int, &d, "missing"));

  dict_release(cstr, int, &d);
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

triax_test(dict, foreach_empty_is_noop) {
  Dict(int, cstr) d     = dict_init(int, cstr, 8);
  int             count = 0;
  dict_foreach(&d, k, v) {
    (void)k;
    (void)v;
    ++count;
  }
  triax_expect_eq(count, 0);
  dict_release(int, cstr, &d);
}

// A zero-initialized Dict has cap == 0 and data == NULL, a distinct edge case from an initialized
// but empty Dict (nonzero cap, all slots marked empty) -- exercises rki_ds_next_live's documented
// cap == 0 / data == NULL safety.
triax_test(dict, foreach_on_zero_initialized_dict_is_noop) {
  Dict(int, cstr) d     = {RK_ZINIT};
  int             count = 0;
  dict_foreach(&d, k, v) {
    (void)k;
    (void)v;
    ++count;
  }
  dict_foreach_key(&d, k) {
    (void)k;
    ++count;
  }
  dict_foreach_val(&d, v) {
    (void)v;
    ++count;
  }
  triax_expect_eq(count, 0);
}

triax_test(dict, foreach_visits_all_pairs) {
  Dict(int, cstr) d = dict_init(int, cstr, 8);
  dict_set(int, cstr, &d, 1, "one");
  dict_set(int, cstr, &d, 2, "two");
  dict_set(int, cstr, &d, 3, "three");

  int count = 0;
  dict_foreach(&d, k, v) {
    ++count;
    triax_expect_streq(*v, *dict_get(int, cstr, &d, *k));
  }
  triax_expect_eq(count, 3);

  dict_release(int, cstr, &d);
}

triax_test(dict, foreach_break_stops_iteration) {
  Dict(int, cstr) d = dict_init(int, cstr, 8);
  dict_set(int, cstr, &d, 1, "one");
  dict_set(int, cstr, &d, 2, "two");
  dict_set(int, cstr, &d, 3, "three");

  int visits = 0;
  dict_foreach(&d, k, v) {
    (void)k;
    (void)v;
    ++visits;
    break;
  }
  triax_expect_eq(visits, 1);

  dict_release(int, cstr, &d);
}

triax_test(dict, foreach_continue_skips_entry) {
  Dict(int, cstr) d = dict_init(int, cstr, 8);
  dict_set(int, cstr, &d, 1, "one");
  dict_set(int, cstr, &d, 2, "two");
  dict_set(int, cstr, &d, 3, "three");

  int visits = 0, skipped = 0;
  dict_foreach(&d, k, v) {
    (void)k;
    (void)v;
    if (!skipped) {
      skipped = 1;
      continue;
    }
    ++visits;
  }
  triax_expect_eq(skipped, 1);
  triax_expect_eq(visits, 2);

  dict_release(int, cstr, &d);
}

triax_test(dict, foreach_key_break_stops_iteration) {
  Dict(int, cstr) d = dict_init(int, cstr, 8);
  dict_set(int, cstr, &d, 1, "one");
  dict_set(int, cstr, &d, 2, "two");
  dict_set(int, cstr, &d, 3, "three");

  int visits = 0;
  dict_foreach_key(&d, k) {
    (void)k;
    ++visits;
    break;
  }
  triax_expect_eq(visits, 1);

  dict_release(int, cstr, &d);
}

triax_test(dict, foreach_key_continue_skips_entry) {
  Dict(int, cstr) d = dict_init(int, cstr, 8);
  dict_set(int, cstr, &d, 1, "one");
  dict_set(int, cstr, &d, 2, "two");
  dict_set(int, cstr, &d, 3, "three");

  int visits = 0, skipped = 0;
  dict_foreach_key(&d, k) {
    (void)k;
    if (!skipped) {
      skipped = 1;
      continue;
    }
    ++visits;
  }
  triax_expect_eq(skipped, 1);
  triax_expect_eq(visits, 2);

  dict_release(int, cstr, &d);
}

triax_test(dict, foreach_val_break_stops_iteration) {
  Dict(int, cstr) d = dict_init(int, cstr, 8);
  dict_set(int, cstr, &d, 1, "one");
  dict_set(int, cstr, &d, 2, "two");
  dict_set(int, cstr, &d, 3, "three");

  int visits = 0;
  dict_foreach_val(&d, v) {
    (void)v;
    ++visits;
    break;
  }
  triax_expect_eq(visits, 1);

  dict_release(int, cstr, &d);
}

triax_test(dict, foreach_val_continue_skips_entry) {
  Dict(int, cstr) d = dict_init(int, cstr, 8);
  dict_set(int, cstr, &d, 1, "one");
  dict_set(int, cstr, &d, 2, "two");
  dict_set(int, cstr, &d, 3, "three");

  int visits = 0, skipped = 0;
  dict_foreach_val(&d, v) {
    (void)v;
    if (!skipped) {
      skipped = 1;
      continue;
    }
    ++visits;
  }
  triax_expect_eq(skipped, 1);
  triax_expect_eq(visits, 2);

  dict_release(int, cstr, &d);
}

static Dict(int, cstr)* rki_mark_eval_dict(Dict(int, cstr)* d, int* count) {
  ++*count;
  return d;
}

triax_test(dict, foreach_evaluates_dict_argument_once) {
  Dict(int, cstr) d = dict_init(int, cstr, 8);
  dict_set(int, cstr, &d, 1, "one");
  dict_set(int, cstr, &d, 2, "two");

  int evals = 0, visits = 0;
  dict_foreach(rki_mark_eval_dict(&d, &evals), k, v) {
    (void)k;
    (void)v;
    ++visits;
  }
  triax_expect_eq(evals, 1);
  triax_expect_eq(visits, 2);

  dict_release(int, cstr, &d);
}

triax_test(dict, foreach_key_evaluates_dict_argument_once) {
  Dict(int, cstr) d = dict_init(int, cstr, 8);
  dict_set(int, cstr, &d, 1, "one");
  dict_set(int, cstr, &d, 2, "two");

  int evals = 0, visits = 0;
  dict_foreach_key(rki_mark_eval_dict(&d, &evals), k) {
    (void)k;
    ++visits;
  }
  triax_expect_eq(evals, 1);
  triax_expect_eq(visits, 2);

  dict_release(int, cstr, &d);
}

triax_test(dict, foreach_val_evaluates_dict_argument_once) {
  Dict(int, cstr) d = dict_init(int, cstr, 8);
  dict_set(int, cstr, &d, 1, "one");
  dict_set(int, cstr, &d, 2, "two");

  int evals = 0, visits = 0;
  dict_foreach_val(rki_mark_eval_dict(&d, &evals), v) {
    (void)v;
    ++visits;
  }
  triax_expect_eq(evals, 1);
  triax_expect_eq(visits, 2);

  dict_release(int, cstr, &d);
}

triax_test(dict, erase_if_removes_matching_entries) {
  Dict(int, cstr) d = dict_init(int, cstr, 8);
  dict_set(int, cstr, &d, 1, "one");
  dict_set(int, cstr, &d, 2, "two");
  dict_set(int, cstr, &d, 3, "three");

  dict_erase_if(&d, k, v, ((void)v, *k == 2));

  triax_expect_eq(dict_count(&d), 2u);
  triax_expect_false(dict_contains(int, cstr, &d, 2));
  triax_expect_true(dict_contains(int, cstr, &d, 1));
  triax_expect_true(dict_contains(int, cstr, &d, 3));

  dict_release(int, cstr, &d);
}

triax_test(dict, erase_if_empty_is_noop) {
  Dict(int, cstr) d     = dict_init(int, cstr, 8);
  int             calls = 0;
  dict_erase_if(&d, k, v, (++calls, (void)k, (void)v, true));
  triax_expect_eq(calls, 0);
  triax_expect_eq(dict_count(&d), 0u);
  dict_release(int, cstr, &d);
}

triax_test(dict, erase_if_on_zero_initialized_dict_is_noop) {
  Dict(int, cstr) d     = {RK_ZINIT};
  int             calls = 0;
  dict_erase_if(&d, k, v, (++calls, (void)k, (void)v, true));
  triax_expect_eq(calls, 0);
}

triax_test(dict, erase_if_evaluates_dict_argument_once) {
  Dict(int, cstr) d = dict_init(int, cstr, 8);
  dict_set(int, cstr, &d, 1, "one");
  dict_set(int, cstr, &d, 2, "two");

  int evals = 0;
  dict_erase_if(rki_mark_eval_dict(&d, &evals), k, v, ((void)v, *k == 1));
  triax_expect_eq(evals, 1);
  triax_expect_eq(dict_count(&d), 1u);

  dict_release(int, cstr, &d);
}

triax_test(dict, erase_if_evaluates_predicate_once_per_entry) {
  Dict(int, cstr) d = dict_init(int, cstr, 8);
  dict_set(int, cstr, &d, 1, "one");
  dict_set(int, cstr, &d, 2, "two");
  dict_set(int, cstr, &d, 3, "three");

  int pred_calls = 0;
  dict_erase_if(&d, k, v, (++pred_calls, (void)k, (void)v, false));
  triax_expect_eq(pred_calls, 3);
  triax_expect_eq(dict_count(&d), 3u);

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

// dict_reserve's n counts live entries, not raw slots (see reserve_counts_entries_not_slots
// below), so "reserve(current slot cap)" is NOT a no-op: that many entries need more than that
// many slots at the load factor. A true no-op needs an entry count comfortably under the current
// slots' load-factor threshold.
triax_test(dict, reserve_noop_when_smaller) {
  Dict(int, cstr) d            = dict_init(int, cstr, 64);
  size_t          original_cap = dict_cap(&d);
  dict_reserve(int, cstr, &d, 4); // well under the load-factor threshold — no change
  triax_expect_eq(dict_cap(&d), original_cap);
  dict_reserve(int, cstr, &d, 1); // even smaller — still no change
  triax_expect_eq(dict_cap(&d), original_cap);
  dict_reserve(int, cstr, &d, original_cap * 2); // clearly larger — grows
  triax_expect_true(dict_cap(&d) >= original_cap * 2);
  dict_release(int, cstr, &d);
}

// dict_reserve(K, V, self, n) counts live entries, not raw slots: after reserving room for n
// entries, inserting exactly n of them must not trigger another automatic rehash.
triax_test(dict, reserve_counts_entries_not_slots) {
  Dict(int, cstr) d = dict_init(int, cstr, 0);
  dict_reserve(int, cstr, &d, 100);
  size_t reserved_cap = dict_cap(&d);
  triax_expect_true(reserved_cap > 0u);
  for (int i = 0; i < 100; ++i) { dict_set(int, cstr, &d, i, "v"); }
  triax_expect_eq(dict_count(&d), 100u);
  triax_expect_eq(dict_cap(&d), reserved_cap); // no growth should have been needed
  dict_release(int, cstr, &d);
}

triax_test(dict, shrink_to_fit) {
  Dict(int, cstr) d = dict_init(int, cstr, 0);
  for (int i = 0; i < 200; ++i) { dict_set(int, cstr, &d, i, "v"); }
  size_t big_cap = dict_cap(&d);

  for (int i = 0; i < 190; ++i) { dict_remove(int, cstr, &d, i); }
  triax_expect_eq(dict_count(&d), 10u);

  dict_shrink_to_fit(int, cstr, &d);
  triax_expect_true(dict_cap(&d) < big_cap);
  triax_expect_eq(dict_count(&d), 10u);
  for (int i = 190; i < 200; ++i) { triax_expect_true(dict_contains(int, cstr, &d, i)); }

  // shrinking an empty Dict frees the table entirely
  for (int i = 190; i < 200; ++i) { dict_remove(int, cstr, &d, i); }
  dict_shrink_to_fit(int, cstr, &d);
  triax_expect_eq(dict_cap(&d), 0u);

  // still usable after
  dict_set(int, cstr, &d, 1, "one");
  triax_expect_true(dict_contains(int, cstr, &d, 1));

  dict_release(int, cstr, &d);
}

/* ------------------------------------------------------------------------ */
/* Parameterized: a single key/value round-trips through set/get/contains/  */
/* remove identically regardless of its actual value. Collapses what would  */
/* otherwise be one near-identical test per case into a single test run     */
/* once per row of the table.                                               */
/* ------------------------------------------------------------------------ */
typedef struct {
  const char* name;
  int         key;
  cstr        value;
} DictRoundtripCase;

static const DictRoundtripCase dict_roundtrip_cases[] = {
    {"positive", 42, "forty-two"},   {"zero", 0, "zero"},
    {"negative", -17, "neg-17"},     {"int_min", INT_MIN, "int-min"},
    {"int_max", INT_MAX, "int-max"},
};

triax_test(dict, single_key_roundtrip, .params = triax_as_params(dict_roundtrip_cases)) {
  const DictRoundtripCase* c = triax_param(DictRoundtripCase);
  Dict(int, cstr)          d = dict_init(int, cstr, 0);

  triax_expect(dict_set(int, cstr, &d, c->key, c->value), "case: %s", c->name);
  triax_expect_eq(dict_count(&d), 1u);
  triax_expect_true(dict_contains(int, cstr, &d, c->key));

  cstr* p = dict_get(int, cstr, &d, c->key);
  triax_expect_nonnull(p);
  if (p) { triax_expect_streq(*p, c->value); }

  triax_expect_true(dict_remove(int, cstr, &d, c->key));
  triax_expect_true(dict_is_empty(&d));

  dict_release(int, cstr, &d);
}

/* ------------------------------------------------------------------------ */
/* dict_get_or_add()'s inserted_out is optional and NULL-tolerant. dict_-    */
/* extract()'s out_ptr is not: it's the only place the removed value goes,  */
/* so it still asserts non-NULL (rk_assert_ptr_nonnull, an abort in debug   */
/* builds) -- exercised via triax_assert_fault rather than skipped just     */
/* because it crashes, matching the convention already used for arena/pool */
/* allocator-failure paths and the tree foreach stack-overflow checks.     */
/* Isolation is set explicitly even though it's already the framework      */
/* default, since getting this wrong would crash the suite.                */
/* ------------------------------------------------------------------------ */
triax_test(dict, get_or_add_tolerates_null_inserted_out) {
  Dict(int, cstr) d  = dict_init(int, cstr, 4);

  cstr*           v1 = dict_get_or_add(int, cstr, &d, 1, "one", rk_null);
  triax_expect_nonnull(v1);
  triax_expect_streq(*v1, "one");
  triax_expect_eq(dict_count(&d), 1u);

  // key already exists: still tolerates a NULL inserted_out, value is left unchanged
  cstr* v2 = dict_get_or_add(int, cstr, &d, 1, "changed", rk_null);
  triax_expect_nonnull(v2);
  triax_expect_streq(*v2, "one");
  triax_expect_eq(dict_count(&d), 1u);

  dict_release(int, cstr, &d);
}

triax_test(dict, extract_rejects_null_out_ptr, .isolation = TRIAX_ISOLATION_ON) {
  Dict(int, cstr) d = dict_init(int, cstr, 4);
  dict_set(int, cstr, &d, 1, "one");
  triax_assert_fault(TRIAX_FAULT_ABORT, { (void)dict_extract(int, cstr, &d, 1, rk_null); });
  dict_release(int, cstr, &d);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @name Set Tests
////////////////////////////////////////////////////////////////////////////////////////////////////

static inline int rk_set_int_cmp(int x, int y) { return x != y; }
static inline int rk_set_int_hash(int x) { return x; }

SET_DEFINE(int, rk_set_int_hash, rk_set_int_cmp)
typedef unsigned char uchar;

static inline int     rk_uchar_cmp(uchar x, uchar y) { return x != y; }
static inline int     rk_uchar_hash(uchar x) { return x; }
SET_DEFINE(uchar, rk_uchar_hash, rk_uchar_cmp)

triax_test(set, zero_initialized) {
  Set(uchar) s = {RK_ZINIT};
  triax_expect_true(set_is_empty(&s));
  triax_expect_eq(set_count(&s), 0u);
  triax_expect_eq(set_cap(&s), 0u);
  RK_IFALLOC(triax_expect_memeq((Allocator[]){set_allocator(&s)}, &alloc_ctx, sizeof(alloc_ctx));)

  triax_expect_true(set_add(uchar, &s, 7));
  triax_expect_eq(set_count(&s), 1u);
  triax_expect_true(set_contains(uchar, &s, 7));

  set_release(uchar, &s);
  triax_expect_true(set_is_empty(&s));
}

triax_test(set, add_all_byte_values) {
  Set(uchar) s = set_init(uchar, 256);
  for (int i = 0; i < 256; ++i) {
    triax_expect_true(set_add(uchar, &s, (uchar)i));
    triax_expect_true(set_contains(uchar, &s, (uchar)i));
  }
  triax_expect_eq(set_count(&s), 256u);

  // re-adding every value again is a no-op: still exactly 256 members
  for (int i = 0; i < 256; ++i) { set_add(uchar, &s, (uchar)i); }
  triax_expect_eq(set_count(&s), 256u);

  set_release(uchar, &s);
}

// set_reserve(K, self, n) counts live entries, not raw slots: after reserving room for n entries,
// inserting exactly n of them must not trigger another automatic rehash.
triax_test(set, reserve_counts_entries_not_slots) {
  Set(int) s = set_init(int, 0);
  set_reserve(int, &s, 100);
  size_t reserved_cap = set_cap(&s);
  triax_expect_true(reserved_cap > 0u);
  for (int i = 0; i < 100; ++i) { set_add(int, &s, i); }
  triax_expect_eq(set_count(&s), 100u);
  triax_expect_eq(set_cap(&s), reserved_cap); // no growth should have been needed
  set_release(int, &s);
}

triax_test(set, shrink_to_fit) {
  Set(int) s = set_init(int, 0);
  for (int i = 0; i < 200; ++i) { set_add(int, &s, i); }
  size_t big_cap = set_cap(&s);

  for (int i = 0; i < 190; ++i) { set_remove(int, &s, i); }
  triax_expect_eq(set_count(&s), 10u);

  set_shrink_to_fit(int, &s);
  triax_expect_true(set_cap(&s) < big_cap);
  triax_expect_eq(set_count(&s), 10u);
  for (int i = 190; i < 200; ++i) { triax_expect_true(set_contains(int, &s, i)); }

  // shrinking an empty Set frees the table entirely
  for (int i = 190; i < 200; ++i) { set_remove(int, &s, i); }
  set_shrink_to_fit(int, &s);
  triax_expect_eq(set_cap(&s), 0u);

  // still usable after
  set_add(int, &s, 1);
  triax_expect_true(set_contains(int, &s, 1));

  set_release(int, &s);
}

typedef struct {
  const char* name;
  int         key;
} SetRoundtripCase;

static const SetRoundtripCase set_roundtrip_cases[] = {
    {"positive", 42}, {"zero", 0}, {"negative", -17}, {"int_min", INT_MIN}, {"int_max", INT_MAX},
};

triax_test(set, single_key_roundtrip, .params = triax_as_params(set_roundtrip_cases)) {
  const SetRoundtripCase* c = triax_param(SetRoundtripCase);
  Set(int) s                = set_init(int, 0);

  triax_expect(set_add(int, &s, c->key), "case: %s", c->name);
  triax_expect_eq(set_count(&s), 1u);
  triax_expect_true(set_contains(int, &s, c->key));

  triax_expect_true(set_remove(int, &s, c->key));
  triax_expect_true(set_is_empty(&s));

  set_release(int, &s);
}

triax_test(set, foreach_visits_all_members) {
  Set(int) s = set_init(int, 8);
  set_add(int, &s, 1);
  set_add(int, &s, 2);
  set_add(int, &s, 3);

  int seen_1 = 0, seen_2 = 0, seen_3 = 0, count = 0;
  set_foreach(&s, k) {
    ++count;
    if (*k == 1) { ++seen_1; }
    if (*k == 2) { ++seen_2; }
    if (*k == 3) { ++seen_3; }
  }
  triax_expect_eq(count, 3);
  triax_expect_eq(seen_1, 1);
  triax_expect_eq(seen_2, 1);
  triax_expect_eq(seen_3, 1);

  set_release(int, &s);
}

triax_test(set, foreach_empty_is_noop) {
  Set(int) s     = set_init(int, 8);
  int      count = 0;
  set_foreach(&s, k) {
    (void)k;
    ++count;
  }
  triax_expect_eq(count, 0);
  set_release(int, &s);
}

triax_test(set, foreach_on_zero_initialized_set_is_noop) {
  Set(int) s     = {RK_ZINIT};
  int      count = 0;
  set_foreach(&s, k) {
    (void)k;
    ++count;
  }
  triax_expect_eq(count, 0);
}

triax_test(set, foreach_break_stops_iteration) {
  Set(int) s = set_init(int, 8);
  set_add(int, &s, 1);
  set_add(int, &s, 2);
  set_add(int, &s, 3);

  int visits = 0;
  set_foreach(&s, k) {
    (void)k;
    ++visits;
    break;
  }
  triax_expect_eq(visits, 1);

  set_release(int, &s);
}

triax_test(set, foreach_continue_skips_member) {
  Set(int) s = set_init(int, 8);
  set_add(int, &s, 1);
  set_add(int, &s, 2);
  set_add(int, &s, 3);

  int visits = 0, skipped = 0;
  set_foreach(&s, k) {
    (void)k;
    if (!skipped) {
      skipped = 1;
      continue;
    }
    ++visits;
  }
  triax_expect_eq(skipped, 1);
  triax_expect_eq(visits, 2);

  set_release(int, &s);
}

static Set(int)* rki_mark_eval_set(Set(int)* s, int* count) {
  ++*count;
  return s;
}

triax_test(set, foreach_evaluates_set_argument_once) {
  Set(int) s = set_init(int, 8);
  set_add(int, &s, 1);
  set_add(int, &s, 2);

  int evals = 0, visits = 0;
  set_foreach(rki_mark_eval_set(&s, &evals), k) {
    (void)k;
    ++visits;
  }
  triax_expect_eq(evals, 1);
  triax_expect_eq(visits, 2);

  set_release(int, &s);
}

triax_test(set, erase_if_removes_matching_members) {
  Set(int) s = set_init(int, 8);
  set_add(int, &s, 1);
  set_add(int, &s, 2);
  set_add(int, &s, 3);

  set_erase_if(&s, k, *k == 2);

  triax_expect_eq(set_count(&s), 2u);
  triax_expect_false(set_contains(int, &s, 2));
  triax_expect_true(set_contains(int, &s, 1));
  triax_expect_true(set_contains(int, &s, 3));

  set_release(int, &s);
}

triax_test(set, erase_if_empty_is_noop) {
  Set(int) s     = set_init(int, 8);
  int      calls = 0;
  set_erase_if(&s, k, (++calls, (void)k, true));
  triax_expect_eq(calls, 0);
  triax_expect_eq(set_count(&s), 0u);
  set_release(int, &s);
}

triax_test(set, erase_if_on_zero_initialized_set_is_noop) {
  Set(int) s     = {RK_ZINIT};
  int      calls = 0;
  set_erase_if(&s, k, (++calls, (void)k, true));
  triax_expect_eq(calls, 0);
}

triax_test(set, erase_if_evaluates_set_argument_once) {
  Set(int) s = set_init(int, 8);
  set_add(int, &s, 1);
  set_add(int, &s, 2);

  int evals = 0;
  set_erase_if(rki_mark_eval_set(&s, &evals), k, *k == 1);
  triax_expect_eq(evals, 1);
  triax_expect_eq(set_count(&s), 1u);

  set_release(int, &s);
}

triax_test(set, erase_if_evaluates_predicate_once_per_member) {
  Set(int) s = set_init(int, 8);
  set_add(int, &s, 1);
  set_add(int, &s, 2);
  set_add(int, &s, 3);

  int pred_calls = 0;
  set_erase_if(&s, k, (++pred_calls, (void)k, false));
  triax_expect_eq(pred_calls, 3);
  triax_expect_eq(set_count(&s), 3u);

  set_release(int, &s);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @name Misc
////////////////////////////////////////////////////////////////////////////////////////////////////

triax_test(arrdup, t0) {
  int  src[5] = {1, 2, 3, 4, 5};
  int* dst    = rk_arrdup(src, 5);
  triax_expect_memeq(dst, src, sizeof(src));
  int* dst2 = rk_arrdup(dst, 5);
  triax_expect_memeq(dst2, src, sizeof(src));
  triax_expect_neq(dst2, dst); // must be a distinct allocation, not an alias
  alloc_delete(dst, 5);
  alloc_delete(dst2, 5);
}

triax_test(memdup, t0) {
  int  src[5] = {1, 2, 3, 4, 5};
  int* dst    = rk_memdup(src, sizeof(src));
  triax_expect_memeq(dst, src, sizeof(src));
  int* dst2 = rk_memdup(dst, sizeof(src));
  triax_expect_memeq(dst2, src, sizeof(src));
  triax_expect_neq(dst2, dst); // must be a distinct allocation, not an alias
  alloc_deallocate(dst, sizeof(src), align_max);
  alloc_deallocate(dst2, sizeof(src), align_max);
}

triax_test(memdup, aligned) {
  int  src[5] = {1, 2, 3, 4, 5};
  int* dst    = rk_memdup_aligned(src, sizeof(src), 64);
  triax_expect_eq((uintptr_t)dst % 64, (uintptr_t)0);
  triax_expect_memeq(dst, src, sizeof(src));
  alloc_deallocate(dst, sizeof(src), 64);
}
RKI_IGNWARN_CLANG_END()
RK_HEADER_END
#endif
