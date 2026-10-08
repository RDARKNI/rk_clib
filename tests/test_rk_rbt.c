#ifndef TEST_RBT_H
#define TEST_RBT_H
#include "conf.h"

#define RK_IMPL
#include "../include/rklib.h"
#include <limits.h>

RKI_HEADER_BEGIN
RKI_IGNWARN_CLANG_BEG("-Wunused-variable")

extern_fun int rbt_int_cmp(int a, int b) { return a < b ? -1 : (a == b ? 0 : 1); }

RBT_DEFINE(int, char, rbt_int_cmp)

triax_test(rbt, empty) {
  Rbt(int, char) r = rbt_init(int, char);

  triax_expect_true(rbt_is_empty(&r));
  triax_expect_eq(rbt_count(&r), 0u);
  triax_expect_null(rbt_peek_min(&r));
  triax_expect_null(rbt_peek_max(&r));
  triax_expect_null(rbt_get(int, char, &r, 123));
  triax_expect_false(rbt_contains(int, char, &r, 123));

  char out = 0;
  triax_expect_false(rbt_extract(int, char, &r, 123, &out));

  rbt_release(int, char, &r);
}

triax_test(rbt, zero_initialized) {
  Rbt(int, char) r = {0};
  triax_expect_true(rbt_is_empty(&r));
  triax_expect_eq(rbt_count(&r), 0u);
  triax_expect_null(rbt_peek_min(&r));
  triax_expect_null(rbt_peek_max(&r));

  triax_expect_true(rbt_set(int, char, &r, 7, 'g'));
  triax_expect_eq(rbt_count(&r), 1u);
  triax_expect_true(rbt_contains(int, char, &r, 7));
  triax_expect_eq(*rbt_get(int, char, &r, 7), 'g');

  rbt_release(int, char, &r);
  triax_expect_true(rbt_is_empty(&r));
}

triax_test(rbt, insert_and_lookup) {
  Rbt(int, char) r = rbt_init(int, char);

  triax_expect_true(rbt_set(int, char, &r, 3, 'c'));
  triax_expect_true(rbt_set(int, char, &r, 1, 'a'));
  triax_expect_true(rbt_set(int, char, &r, 4, 'd'));
  triax_expect_true(rbt_set(int, char, &r, 2, 'b'));

  triax_expect_false(rbt_is_empty(&r));
  triax_expect_eq(rbt_count(&r), 4u);

  char* p1 = rbt_get(int, char, &r, 1);
  char* p2 = rbt_get(int, char, &r, 2);
  char* p3 = rbt_get(int, char, &r, 3);
  char* p4 = rbt_get(int, char, &r, 4);
  triax_expect_nonnull(p1), triax_expect_nonnull(p2);
  triax_expect_nonnull(p3), triax_expect_nonnull(p4);
  if (p1) { triax_expect_eq(*p1, 'a'); }
  if (p2) { triax_expect_eq(*p2, 'b'); }
  if (p3) { triax_expect_eq(*p3, 'c'); }
  if (p4) { triax_expect_eq(*p4, 'd'); }
  triax_expect_null(rbt_get(int, char, &r, 99));

  triax_expect_true(rbt_contains(int, char, &r, 1));
  triax_expect_true(rbt_contains(int, char, &r, 2));
  triax_expect_true(rbt_contains(int, char, &r, 3));
  triax_expect_true(rbt_contains(int, char, &r, 4));
  triax_expect_false(rbt_contains(int, char, &r, 99));

  RbtEntry(int, char)* mn = rbt_peek_min(&r);
  RbtEntry(int, char)* mx = rbt_peek_max(&r);
  triax_expect_nonnull(mn), triax_expect_nonnull(mx);
  if (mn) { triax_expect_eq(mn->key, 1), triax_expect_eq(mn->val, 'a'); }
  if (mx) { triax_expect_eq(mx->key, 4), triax_expect_eq(mx->val, 'd'); }

  rbt_release(int, char, &r);
}

// ---- min/max: asserted entry lvalues; peek_min/peek_max: nullable entry pointers ----

triax_test(rbt, min_max_are_entry_lvalues) {
  Rbt(int, char) r = rbt_init(int, char);
  rbt_set(int, char, &r, 5, 'e');
  rbt_set(int, char, &r, 1, 'a');
  rbt_set(int, char, &r, 9, 'i');
  triax_expect_eq(rbt_min(&r).key, 1);
  triax_expect_eq(rbt_max(&r).key, 9);
  triax_expect_eq(&rbt_min(&r), rbt_peek_min(&r));
  triax_expect_eq(&rbt_max(&r), rbt_peek_max(&r));
  rbt_min(&r).val = 'A'; // values are writable; keys stay const
  rbt_max(&r).val = 'I';
  triax_expect_eq(*rbt_get(int, char, &r, 1), 'A');
  triax_expect_eq(*rbt_get(int, char, &r, 9), 'I');
  rbt_release(int, char, &r);
}

triax_test(rbt, min_max_follow_tree_constness) {
  Rbt(int, char) r = rbt_init(int, char);
  rbt_set(int, char, &r, 2, 'b');
  const Rbt(int, char)* c = &r;
  static_assert(_Generic(&rbt_min(&r), RbtEntry(int, char)*: 1, default: 0), "mutable");
  static_assert(_Generic(&rbt_max(c), const RbtEntry(int, char)*: 1, default: 0), "const");
  static_assert(_Generic(rbt_peek_min(&r), RbtEntry(int, char)*: 1, default: 0), "mutable");
  static_assert(_Generic(rbt_peek_max(c), const RbtEntry(int, char)*: 1, default: 0), "const");
  static_assert(_Generic(&rbt_min(&r).key, const int*: 1, default: 0), "keys are always const");
  triax_expect_eq(rbt_min(c).key, 2);
  triax_expect_eq(rbt_peek_max(c)->val, 'b');
  rbt_release(int, char, &r);
}

// An expected fault ends the test process, so each case needs its own test. The results are
// discarded on purpose: the emptiness check must still run.
#ifdef RKLIB_DEBUG
triax_test(rbt, min_of_empty_asserts, .isolation = TRIAX_ISOLATION_ON) {
  Rbt(int, char) r = rbt_init(int, char);
  triax_assert_fault(TRIAX_FAULT_ABORT, { (void)rbt_min(&r); });
}

triax_test(rbt, max_of_empty_asserts, .isolation = TRIAX_ISOLATION_ON) {
  Rbt(int, char) r = rbt_init(int, char);
  triax_assert_fault(TRIAX_FAULT_ABORT, { (void)rbt_max(&r); });
}
#endif

triax_test(rbt, add_does_not_overwrite_set_does) {
  Rbt(int, char) r = rbt_init(int, char);
  rbt_set(int, char, &r, 2, 'b');

  triax_expect_false(rbt_add(int, char, &r, 2, 'X'));
  triax_expect_eq(*rbt_get(int, char, &r, 2), 'b');

  triax_expect_false(rbt_set(int, char, &r, 2, 'B'));
  triax_expect_eq(*rbt_get(int, char, &r, 2), 'B');
  triax_expect_eq(rbt_count(&r), 1u);

  rbt_release(int, char, &r);
}

triax_test(rbt, get_or_add) {
  Rbt(int, char) r = rbt_init(int, char);

  bool  inserted   = false;
  char* p1         = rbt_get_or_add(int, char, &r, 5, 'e', &inserted);
  triax_expect_true(inserted);
  triax_expect_eq(*p1, 'e');
  triax_expect_eq(rbt_count(&r), 1u);

  // key already present: returns existing value, does not overwrite, does not insert
  inserted = true;
  char* p2 = rbt_get_or_add(int, char, &r, 5, 'z', &inserted);
  triax_expect_false(inserted);
  triax_expect_eq(*p2, 'e');
  triax_expect_eq(rbt_count(&r), 1u);
  // cast to void*: triax_expect_eq treats char* as a C-string (via strlen), but p1/p2 point at a
  // single, non-null-terminated char field inside the Rbt node -- this must be a pointer-identity
  // check (same address returned for an already-present key), not a string comparison.
  triax_expect_eq((void*)p1, (void*)p2);

  // a second, distinct key still triggers a real insertion
  inserted = false;
  char* p3 = rbt_get_or_add(int, char, &r, 9, 'i', &inserted);
  triax_expect_true(inserted);
  triax_expect_eq(*p3, 'i');
  triax_expect_eq(rbt_count(&r), 2u);

  triax_expect_eq(*rbt_get(int, char, &r, 5), 'e');
  triax_expect_eq(*rbt_get(int, char, &r, 9), 'i');

  rbt_release(int, char, &r);
}

triax_test(rbt, foreach_empty) {
  Rbt(int, char) r = rbt_init(int, char);
  TreeNode* stack[4];
  int        visited = 0;
  rbt_foreach(&r, stack, 4, e) {
    (void)e;
    ++visited;
  }
  triax_expect_eq(visited, 0);
  rbt_release(int, char, &r);
}

triax_test(rbt, foreach_inorder) {
  Rbt(int, char) r = rbt_init(int, char);
  rbt_set(int, char, &r, 3, 'c');
  rbt_set(int, char, &r, 1, 'a');
  rbt_set(int, char, &r, 4, 'd');
  rbt_set(int, char, &r, 2, 'B');

  TreeNode* stack[4];
  int        keys[4] = {0};
  char       vals[4] = {0};
  int        i       = 0;

  rbt_foreach(&r, stack, 4, e) {
    keys[i] = e->key;
    vals[i] = e->val;
    ++i;
  }
  triax_expect_eq(i, 4);

  static const int  expect_keys[4] = {1, 2, 3, 4};
  static const char expect_vals[4] = {'a', 'B', 'c', 'd'};
  triax_expect_arreq(keys, expect_keys);
  triax_expect_arreq(vals, expect_vals);

  rbt_release(int, char, &r);
}

triax_test(rbt, foreach_reversed_empty) {
  Rbt(int, char) r = rbt_init(int, char);
  TreeNode* stack[4];
  int        visited = 0;
  rbt_foreach_reversed(&r, stack, 4, e) {
    (void)e;
    ++visited;
  }
  triax_expect_eq(visited, 0);
  rbt_release(int, char, &r);
}

triax_test(rbt, foreach_reversed_visits_descending) {
  Rbt(int, char) r = rbt_init(int, char);
  rbt_set(int, char, &r, 3, 'c');
  rbt_set(int, char, &r, 1, 'a');
  rbt_set(int, char, &r, 4, 'd');
  rbt_set(int, char, &r, 2, 'b');

  TreeNode* stack[4];
  int        keys[4] = {0};
  int        i       = 0;
  rbt_foreach_reversed(&r, stack, 4, e) { keys[i++] = e->key; }
  triax_expect_eq(i, 4);

  static const int expect_keys[4] = {4, 3, 2, 1};
  triax_expect_arreq(keys, expect_keys);

  rbt_release(int, char, &r);
}

triax_test(rbt, foreach_break_stops_iteration) {
  Rbt(int, char) r = rbt_init(int, char);
  rbt_set(int, char, &r, 1, 'a');
  rbt_set(int, char, &r, 2, 'b');
  rbt_set(int, char, &r, 3, 'c');
  rbt_set(int, char, &r, 4, 'd');

  TreeNode* stack[4];
  int        visits = 0;
  rbt_foreach(&r, stack, 4, e) {
    if (e->key == 2) { break; }
    ++visits;
  }
  triax_expect_eq(visits, 1); // just key 1

  rbt_release(int, char, &r);
}

triax_test(rbt, foreach_continue_skips_entry) {
  Rbt(int, char) r = rbt_init(int, char);
  rbt_set(int, char, &r, 1, 'a');
  rbt_set(int, char, &r, 2, 'b');
  rbt_set(int, char, &r, 3, 'c');
  rbt_set(int, char, &r, 4, 'd');

  TreeNode* stack[4];
  int        visits = 0;
  rbt_foreach(&r, stack, 4, e) {
    if (e->key == 2) { continue; }
    ++visits;
  }
  triax_expect_eq(visits, 3); // all but key 2

  rbt_release(int, char, &r);
}

triax_test(rbt, foreach_reversed_break_stops_iteration) {
  Rbt(int, char) r = rbt_init(int, char);
  rbt_set(int, char, &r, 1, 'a');
  rbt_set(int, char, &r, 2, 'b');
  rbt_set(int, char, &r, 3, 'c');
  rbt_set(int, char, &r, 4, 'd');

  TreeNode* stack[4];
  int        visits = 0;
  rbt_foreach_reversed(&r, stack, 4, e) {
    if (e->key == 3) { break; }
    ++visits;
  }
  triax_expect_eq(visits, 1); // just key 4

  rbt_release(int, char, &r);
}

triax_test(rbt, foreach_reversed_continue_skips_entry) {
  Rbt(int, char) r = rbt_init(int, char);
  rbt_set(int, char, &r, 1, 'a');
  rbt_set(int, char, &r, 2, 'b');
  rbt_set(int, char, &r, 3, 'c');
  rbt_set(int, char, &r, 4, 'd');

  TreeNode* stack[4];
  int        visits = 0;
  rbt_foreach_reversed(&r, stack, 4, e) {
    if (e->key == 3) { continue; }
    ++visits;
  }
  triax_expect_eq(visits, 3); // all but key 3

  rbt_release(int, char, &r);
}

triax_test(rbt, erase_if_removes_matching_entries) {
  Rbt(int, char) r = rbt_init(int, char);
  rbt_set(int, char, &r, 1, 'a');
  rbt_set(int, char, &r, 2, 'b');
  rbt_set(int, char, &r, 3, 'c');
  rbt_set(int, char, &r, 4, 'd');

  TreeNode* stack[4];
  rbt_erase_if(int, char, &r, stack, 4, e, e->key % 2 == 0);

  triax_expect_eq(rbt_count(&r), 2u);
  triax_expect_false(rbt_contains(int, char, &r, 2));
  triax_expect_false(rbt_contains(int, char, &r, 4));
  triax_expect_true(rbt_contains(int, char, &r, 1));
  triax_expect_true(rbt_contains(int, char, &r, 3));

  int keys[2] = {0};
  int i       = 0;
  rbt_foreach(&r, stack, 4, e) { keys[i++] = e->key; }
  triax_expect_eq(i, 2);
  static const int expect_keys[2] = {1, 3};
  triax_expect_arreq(keys, expect_keys);

  rbt_release(int, char, &r);
}

triax_test(rbt, foreach_const_entries) {
  Rbt(int, char) t = rbt_init(int, char);
  rbt_set(int, char, &t, 1, 'a');
  TreeNode* stack[2];
  tree_foreach(&t, stack, 2, e) {
    static_assert(_Generic(e, RbtEntry(int, char)*: 1, default: 0), "Mutable tree entry");
    e->val = 'b';
  }
  const Rbt(int, char)* ct = &t;
  int seen = 0;
  tree_foreach(ct, stack, 2, e) {
    static_assert(_Generic(e, const RbtEntry(int, char)*: 1, default: 0), "Const tree entry");
    triax_expect_eq(e->val, 'b');
    ++seen;
  }
  tree_foreach_reversed(ct, stack, 2, e) {
    static_assert(_Generic(e, const RbtEntry(int, char)*: 1, default: 0), "Const tree entry");
    triax_expect_eq(e->val, 'b');
    ++seen;
  }
  triax_expect_eq(seen, 2);
  rbt_release(int, char, &t);
}

triax_test(rbt, erase_if_changes_root_then_release) {
  Rbt(int, char) t = rbt_init(int, char);
  rbt_set(int, char, &t, 2, 'b');
  rbt_set(int, char, &t, 1, 'a');
  rbt_set(int, char, &t, 3, 'c');
  TreeNode* stack[3];
  rbt_erase_if(int, char, &t, stack, 3, e, e->key < 3);
  triax_expect_eq(tree_count(&t), 1u);
  triax_expect_eq(tree_min(&t).key, 3);
  rbt_release(int, char, &t);
  triax_expect_null(t.root);
}

triax_test(rbt, erase_if_removes_all) {
  Rbt(int, char) t = rbt_init(int, char);
  for (int i = 1; i <= 4; ++i) { rbt_set(int, char, &t, i, 'a'); }
  TreeNode* stack[4];
  rbt_erase_if(int, char, &t, stack, 4, e, ((void)e, true));
  triax_expect_true(tree_is_empty(&t));
  triax_expect_null(t.root);
  rbt_release(int, char, &t);
}

// The generated tree functions have parameters and locals named `c`, `k`, `key`, `n`, ... . A
// comparator with one of those names used to be shadowed at its call sites (a compile error); it is
// now only ever called through a wrapper with reserved parameter names. Covers all three variants.
static int c(short a, short b) { return (a > b) - (a < b); }
static int key(short a, short b) { return (a > b) - (a < b); }
RBT_DEFINE(short, int, c)
AVL_DEFINE(short, int, c)
BST_DEFINE(short, int, key)

triax_test(rbt, comparator_named_like_internal_locals) {
  Rbt(short, int) r = rbt_init(short, int);
  Avl(short, int) a = avl_init(short, int);
  Bst(short, int) b = bst_init(short, int);
  for (short i = 0; i < 20; ++i) {
    rbt_set(short, int, &r, i, i * 2);
    avl_set(short, int, &a, i, i * 3);
    bst_set(short, int, &b, i, i * 4);
  }
  triax_expect_eq(*rbt_get(short, int, &r, 7), 14);
  triax_expect_eq(*avl_get(short, int, &a, 7), 21);
  triax_expect_eq(*bst_get(short, int, &b, 7), 28);
  int out = 0;
  triax_expect_true(rbt_extract(short, int, &r, 7, &out));
  triax_expect_true(avl_remove(short, int, &a, 7));
  triax_expect_true(bst_remove(short, int, &b, 7));
  triax_expect_false(rbt_contains(short, int, &r, 7));
  triax_expect_eq(tree_count(&a), 19u);
  triax_expect_eq(tree_min(&b).key, 0);
  rbt_release(short, int, &r);
  avl_release(short, int, &a);
  bst_release(short, int, &b);
}

triax_test(rbt, erase_if_empty_is_noop) {
  Rbt(int, char) r = rbt_init(int, char);
  TreeNode* stack[4];
  int        pred_calls = 0;
  rbt_erase_if(int, char, &r, stack, 4, e, (++pred_calls, (void)e, true));
  triax_expect_eq(pred_calls, 0);
  triax_expect_eq(rbt_count(&r), 0u);
  rbt_release(int, char, &r);
}

triax_test(rbt, erase_if_evaluates_predicate_once_per_entry) {
  Rbt(int, char) r = rbt_init(int, char);
  rbt_set(int, char, &r, 1, 'a');
  rbt_set(int, char, &r, 2, 'b');
  rbt_set(int, char, &r, 3, 'c');

  TreeNode* stack[4];
  int        pred_calls = 0;
  rbt_erase_if(int, char, &r, stack, 4, e, (++pred_calls, (void)e, false));
  triax_expect_eq(pred_calls, 3);
  triax_expect_eq(rbt_count(&r), 3u);

  rbt_release(int, char, &r);
}

triax_test(rbt, extract) {
  Rbt(int, char) r = rbt_init(int, char);
  rbt_set(int, char, &r, 1, 'a');
  rbt_set(int, char, &r, 2, 'b');

  char out = 0;
  triax_expect_true(rbt_extract(int, char, &r, 1, &out));
  triax_expect_eq(out, 'a');
  triax_expect_false(rbt_contains(int, char, &r, 1));
  triax_expect_eq(rbt_count(&r), 1u);

  triax_expect_false(rbt_extract(int, char, &r, 1, &out));

  rbt_release(int, char, &r);
}

triax_test(rbt, remove_sequence) {
  Rbt(int, char) r = rbt_init(int, char);
  rbt_set(int, char, &r, 1, 'a');
  rbt_set(int, char, &r, 2, 'b');
  rbt_set(int, char, &r, 3, 'c');
  rbt_set(int, char, &r, 4, 'd');

  triax_expect_true(rbt_contains(int, char, &r, 2));
  rbt_remove(int, char, &r, 2);
  triax_expect_false(rbt_contains(int, char, &r, 2));
  triax_expect_eq(rbt_count(&r), 3u);

  triax_expect_true(rbt_contains(int, char, &r, 3));
  rbt_remove(int, char, &r, 3);
  triax_expect_false(rbt_contains(int, char, &r, 3));
  triax_expect_eq(rbt_count(&r), 2u);

  triax_expect_true(rbt_contains(int, char, &r, 4));
  rbt_remove(int, char, &r, 4);
  triax_expect_false(rbt_contains(int, char, &r, 4));
  triax_expect_eq(rbt_count(&r), 1u);

  triax_expect_true(rbt_contains(int, char, &r, 1));
  rbt_remove(int, char, &r, 1);
  triax_expect_eq(rbt_count(&r), 0u);
  triax_expect_true(rbt_is_empty(&r));
  triax_expect_null(rbt_peek_min(&r));
  triax_expect_null(rbt_peek_max(&r));

  // removing a missing key from an empty tree is harmless
  rbt_remove(int, char, &r, 999);
  triax_expect_eq(rbt_count(&r), 0u);

  rbt_release(int, char, &r);
}

triax_test(rbt, release_and_reuse) {
  Rbt(int, char) r = rbt_init(int, char);
  rbt_set(int, char, &r, 1, 'a');

  rbt_release(int, char, &r);
  triax_expect_true(rbt_is_empty(&r));
  triax_expect_eq(rbt_count(&r), 0u);
  triax_expect_null(rbt_peek_min(&r));
  triax_expect_null(rbt_peek_max(&r));

  // tree should still be usable after release
  triax_expect_true(rbt_set(int, char, &r, 42, 'x'));
  triax_expect_eq(rbt_count(&r), 1u);
  triax_expect_true(rbt_contains(int, char, &r, 42));
  triax_expect_eq(*rbt_get(int, char, &r, 42), 'x');

  rbt_release(int, char, &r);
  triax_expect_true(rbt_is_empty(&r));
  triax_expect_eq(rbt_count(&r), 0u);
}

triax_test(rbt, remove_two_children) {
  /* Build:        5
                 /   \
                3     7
               / \   / \
              1   4 6   8 */
  Rbt(int, char) r = rbt_init(int, char);
  rbt_set(int, char, &r, 5, 'e');
  rbt_set(int, char, &r, 3, 'c');
  rbt_set(int, char, &r, 7, 'g');
  rbt_set(int, char, &r, 1, 'a');
  rbt_set(int, char, &r, 4, 'd');
  rbt_set(int, char, &r, 6, 'f');
  rbt_set(int, char, &r, 8, 'h');
  triax_expect_eq(rbt_count(&r), 7u);

  // Remove 3 (two children: 1 and 4); in-order successor replaces it
  triax_expect_true(rbt_remove(int, char, &r, 3));
  triax_expect_eq(rbt_count(&r), 6u);
  triax_expect_false(rbt_contains(int, char, &r, 3));
  triax_expect_true(rbt_contains(int, char, &r, 1));
  triax_expect_eq(*rbt_get(int, char, &r, 1), 'a');
  triax_expect_true(rbt_contains(int, char, &r, 4));
  triax_expect_eq(*rbt_get(int, char, &r, 4), 'd');

  // Remove 7 (two children: 6 and 8); in-order successor replaces it
  triax_expect_true(rbt_remove(int, char, &r, 7));
  triax_expect_eq(rbt_count(&r), 5u);
  triax_expect_false(rbt_contains(int, char, &r, 7));
  triax_expect_true(rbt_contains(int, char, &r, 6));
  triax_expect_eq(*rbt_get(int, char, &r, 6), 'f');
  triax_expect_true(rbt_contains(int, char, &r, 8));
  triax_expect_eq(*rbt_get(int, char, &r, 8), 'h');

  // BST property: in-order traversal must still be sorted
  TreeNode* stack[8];
  int        keys[5];
  int        i = 0;
  rbt_foreach(&r, stack, 8, e) { keys[i++] = e->key; }
  triax_expect_eq(i, 5);
  for (int j = 1; j < 5; ++j) { triax_expect_gt(keys[j], keys[j - 1]); }

  rbt_release(int, char, &r);
}

triax_test(rbt, large_sorted_order_stays_balanced) {
  Rbt(int, char) r = rbt_init(int, char);

  // Insert 1..32 in ascending order: the worst case for an unbalanced BST
  // (degenerates into a linked list of height 32), but a red-black tree must
  // keep this bounded to O(log n). A stack sized for only 10 entries would
  // overflow a degenerate tree of height 32 but comfortably covers a
  // red-black tree's proven worst-case height bound of 2*log2(n+1) (10 for
  // n=32).
  for (int i = 1; i <= 32; ++i) { rbt_set(int, char, &r, i, (char)('a' + (i - 1) % 26)); }
  triax_expect_eq(rbt_count(&r), 32u);
  triax_expect_eq(rbt_min(&r).key, 1);
  triax_expect_eq(rbt_max(&r).key, 32);

  TreeNode* stack[10];
  int        prev = -1, count = 0;
  rbt_foreach(&r, stack, 10, e) {
    triax_expect_gt(e->key, prev);
    prev = e->key;
    ++count;
  }
  triax_expect_eq(count, 32);

  rbt_release(int, char, &r);
}

/* ------------------------------------------------------------------------ */
/* Parameterized: a single key/value round-trips through set/get/contains/  */
/* min/max/remove identically regardless of its actual value, including     */
/* boundary ints. Collapses what would otherwise be one near-identical test */
/* per case into a single test run once per row of the table.               */
/* ------------------------------------------------------------------------ */
typedef struct {
  const char* name;
  int         key;
  char        value;
} RbtRoundtripCase;

static const RbtRoundtripCase rbt_roundtrip_cases[] = {
    {"positive", 42, 'x'},     {"zero", 0, '0'},          {"negative", -17, 'n'},
    {"int_min", INT_MIN, 'm'}, {"int_max", INT_MAX, 'M'},
};

triax_test(rbt, single_key_roundtrip, .params = triax_as_params(rbt_roundtrip_cases)) {
  const RbtRoundtripCase* c = triax_param(RbtRoundtripCase);
  Rbt(int, char) r          = rbt_init(int, char);

  triax_expect(rbt_set(int, char, &r, c->key, c->value), "case: %s", c->name);
  triax_expect_eq(rbt_count(&r), 1u);
  triax_expect_true(rbt_contains(int, char, &r, c->key));

  char* p = rbt_get(int, char, &r, c->key);
  triax_expect_nonnull(p);
  if (p) { triax_expect_eq(*p, c->value); }

  RbtEntry(int, char)* mn = rbt_peek_min(&r);
  RbtEntry(int, char)* mx = rbt_peek_max(&r);
  triax_expect_nonnull(mn), triax_expect_nonnull(mx);
  if (mn) { triax_expect_eq(mn->key, c->key); }
  if (mx) { triax_expect_eq(mx->key, c->key); }

  triax_expect_true(rbt_remove(int, char, &r, c->key));
  triax_expect_true(rbt_is_empty(&r));

  rbt_release(int, char, &r);
}

/* ------------------------------------------------------------------------ */
/* rbt_foreach's traversal stack overflow is an rk_assert (abort) in debug  */
/* builds, not a caught/returned error -- exercise it via triax_assert_fault */
/* rather than skip it just because it's a crash, matching the convention   */
/* already used for arena/pool allocator-failure paths. Any tree holding 3  */
/* distinct nodes must have height >= 2 regardless of balancing strategy (a */
/* height-1 tree is just a lone root), so a stack of 1 is guaranteed to     */
/* overflow here whether or not rotations/recoloring happened. Isolation is */
/* set explicitly even though it's already the framework default, since     */
/* getting this wrong would crash the whole suite.                          */
/* ------------------------------------------------------------------------ */
triax_test(rbt, foreach_stack_too_small_asserts, .isolation = TRIAX_ISOLATION_ON) {
  Rbt(int, char) r = rbt_init(int, char);
  rbt_set(int, char, &r, 1, 'a');
  rbt_set(int, char, &r, 2, 'b');
  rbt_set(int, char, &r, 3, 'c');

  TreeNode* stack[1]; // deliberately smaller than the tree's height
  triax_assert_fault(TRIAX_FAULT_ABORT, {
    rbt_foreach(&r, stack, 1, e) { (void)e;
}
});

rbt_release(int, char, &r);
}

RKI_IGNWARN_CLANG_END()
RKI_HEADER_END
#endif
