#ifndef TEST_BST_H
#define TEST_BST_H
#include "conf.h"

#define RK_IMPL
#include "../include/rklib.h"
#include <limits.h>

RKI_HEADER_BEGIN
RKI_IGNWARN_CLANG_BEG("-Wunused-variable")

extern_fun int int_cmp2(int a, int b) { return a < b ? -1 : (a == b ? 0 : 1); }

BST_DEFINE(int, char, int_cmp2)

triax_test(bst, empty) {
  Bst(int, char) b = bst_init(int, char);

  triax_expect_true(bst_is_empty(&b));
  triax_expect_eq(bst_count(&b), 0u);
  triax_expect_null(bst_peek_min(&b));
  triax_expect_null(bst_peek_max(&b));
  triax_expect_null(bst_get(int, char, &b, 123));
  triax_expect_false(bst_contains(int, char, &b, 123));

  char out = 0;
  triax_expect_false(bst_extract(int, char, &b, 123, &out));

  bst_release(int, char, &b);
}

triax_test(bst, zero_initialized) {
  Bst(int, char) b = {0};
  triax_expect_true(bst_is_empty(&b));
  triax_expect_eq(bst_count(&b), 0u);
  triax_expect_null(bst_peek_min(&b));
  triax_expect_null(bst_peek_max(&b));

  triax_expect_true(bst_set(int, char, &b, 7, 'g'));
  triax_expect_eq(bst_count(&b), 1u);
  triax_expect_true(bst_contains(int, char, &b, 7));
  triax_expect_eq(*bst_get(int, char, &b, 7), 'g');

  bst_release(int, char, &b);
  triax_expect_true(bst_is_empty(&b));
}

triax_test(bst, insert_and_lookup) {
  Bst(int, char) b = bst_init(int, char);

  triax_expect_true(bst_set(int, char, &b, 3, 'c'));
  triax_expect_true(bst_set(int, char, &b, 1, 'a'));
  triax_expect_true(bst_set(int, char, &b, 4, 'd'));
  triax_expect_true(bst_set(int, char, &b, 2, 'b'));

  triax_expect_false(bst_is_empty(&b));
  triax_expect_eq(bst_count(&b), 4u);

  char* p1 = bst_get(int, char, &b, 1);
  char* p2 = bst_get(int, char, &b, 2);
  char* p3 = bst_get(int, char, &b, 3);
  char* p4 = bst_get(int, char, &b, 4);
  triax_expect_nonnull(p1), triax_expect_nonnull(p2);
  triax_expect_nonnull(p3), triax_expect_nonnull(p4);
  if (p1) { triax_expect_eq(*p1, 'a'); }
  if (p2) { triax_expect_eq(*p2, 'b'); }
  if (p3) { triax_expect_eq(*p3, 'c'); }
  if (p4) { triax_expect_eq(*p4, 'd'); }
  triax_expect_null(bst_get(int, char, &b, 99));

  triax_expect_true(bst_contains(int, char, &b, 1));
  triax_expect_true(bst_contains(int, char, &b, 2));
  triax_expect_true(bst_contains(int, char, &b, 3));
  triax_expect_true(bst_contains(int, char, &b, 4));
  triax_expect_false(bst_contains(int, char, &b, 99));

  BstEntry(int, char)* mn = bst_peek_min(&b);
  BstEntry(int, char)* mx = bst_peek_max(&b);
  triax_expect_nonnull(mn), triax_expect_nonnull(mx);
  if (mn) { triax_expect_eq(mn->key, 1), triax_expect_eq(mn->val, 'a'); }
  if (mx) { triax_expect_eq(mx->key, 4), triax_expect_eq(mx->val, 'd'); }

  bst_release(int, char, &b);
}

// ---- min/max: asserted entry lvalues; peek_min/peek_max: nullable entry pointers ----

triax_test(bst, min_max_are_entry_lvalues) {
  Bst(int, char) b = bst_init(int, char);
  bst_set(int, char, &b, 5, 'e');
  bst_set(int, char, &b, 1, 'a');
  bst_set(int, char, &b, 9, 'i');
  triax_expect_eq(bst_min(&b).key, 1);
  triax_expect_eq(bst_max(&b).key, 9);
  triax_expect_eq(&bst_min(&b), bst_peek_min(&b));
  triax_expect_eq(&bst_max(&b), bst_peek_max(&b));
  bst_min(&b).val = 'A'; // values are writable; keys stay const
  bst_max(&b).val = 'I';
  triax_expect_eq(*bst_get(int, char, &b, 1), 'A');
  triax_expect_eq(*bst_get(int, char, &b, 9), 'I');
  bst_release(int, char, &b);
}

triax_test(bst, min_max_follow_tree_constness) {
  Bst(int, char) b = bst_init(int, char);
  bst_set(int, char, &b, 2, 'b');
  const Bst(int, char)* c = &b;
  static_assert(_Generic(&bst_min(&b), BstEntry(int, char)*: 1, default: 0), "mutable");
  static_assert(_Generic(&bst_max(c), const BstEntry(int, char)*: 1, default: 0), "const");
  static_assert(_Generic(bst_peek_min(&b), BstEntry(int, char)*: 1, default: 0), "mutable");
  static_assert(_Generic(bst_peek_max(c), const BstEntry(int, char)*: 1, default: 0), "const");
  static_assert(_Generic(&bst_min(&b).key, const int*: 1, default: 0), "keys are always const");
  triax_expect_eq(bst_min(c).key, 2);
  triax_expect_eq(bst_peek_max(c)->val, 'b');
  bst_release(int, char, &b);
}

// An expected fault ends the test process, so each case needs its own test. The results are
// discarded on purpose: the emptiness check must still run.
#ifdef RKLIB_DEBUG
triax_test(bst, min_of_empty_asserts, .isolation = TRIAX_ISOLATION_ON) {
  Bst(int, char) b = bst_init(int, char);
  triax_assert_fault(TRIAX_FAULT_ABORT, { (void)bst_min(&b); });
}

triax_test(bst, max_of_empty_asserts, .isolation = TRIAX_ISOLATION_ON) {
  Bst(int, char) b = bst_init(int, char);
  triax_assert_fault(TRIAX_FAULT_ABORT, { (void)bst_max(&b); });
}
#endif

triax_test(bst, add_does_not_overwrite_set_does) {
  Bst(int, char) b = bst_init(int, char);
  bst_set(int, char, &b, 2, 'b');

  triax_expect_false(bst_add(int, char, &b, 2, 'X'));
  triax_expect_eq(*bst_get(int, char, &b, 2), 'b');

  triax_expect_false(bst_set(int, char, &b, 2, 'B'));
  triax_expect_eq(*bst_get(int, char, &b, 2), 'B');
  triax_expect_eq(bst_count(&b), 1u);

  bst_release(int, char, &b);
}

triax_test(bst, get_or_add) {
  Bst(int, char) b = bst_init(int, char);

  bool  inserted   = false;
  char* p1         = bst_get_or_add(int, char, &b, 5, 'e', &inserted);
  triax_expect_true(inserted);
  triax_expect_eq(*p1, 'e');
  triax_expect_eq(bst_count(&b), 1u);

  // key already present: returns existing value, does not overwrite, does not insert
  inserted = true;
  char* p2 = bst_get_or_add(int, char, &b, 5, 'z', &inserted);
  triax_expect_false(inserted);
  triax_expect_eq(*p2, 'e');
  triax_expect_eq(bst_count(&b), 1u);
  // cast to void*: triax_expect_eq treats char* as a C-string (via strlen), but p1/p2 point at a
  // single, non-null-terminated char field inside the Bst node -- this must be a pointer-identity
  // check (same address returned for an already-present key), not a string comparison.
  triax_expect_eq((void*)p1, (void*)p2);

  // a second, distinct key still triggers a real insertion
  inserted = false;
  char* p3 = bst_get_or_add(int, char, &b, 9, 'i', &inserted);
  triax_expect_true(inserted);
  triax_expect_eq(*p3, 'i');
  triax_expect_eq(bst_count(&b), 2u);

  triax_expect_eq(*bst_get(int, char, &b, 5), 'e');
  triax_expect_eq(*bst_get(int, char, &b, 9), 'i');

  bst_release(int, char, &b);
}

triax_test(bst, foreach_empty) {
  Bst(int, char) b = bst_init(int, char);
  TreeNode* stack[4];
  int        visited = 0;
  bst_foreach(&b, stack, 4, e) {
    (void)e;
    ++visited;
  }
  triax_expect_eq(visited, 0);
  bst_release(int, char, &b);
}

triax_test(bst, foreach_inorder) {
  Bst(int, char) b = bst_init(int, char);
  bst_set(int, char, &b, 3, 'c');
  bst_set(int, char, &b, 1, 'a');
  bst_set(int, char, &b, 4, 'd');
  bst_set(int, char, &b, 2, 'B'); // overwritten below to exercise the overwrite path too
  bst_set(int, char, &b, 2, 'B');

  TreeNode* stack[4];
  int        keys[4] = {0};
  char       vals[4] = {0};
  int        i       = 0;

  bst_foreach(&b, stack, 4, e) {
    keys[i] = e->key;
    vals[i] = e->val;
    ++i;
  }
  triax_expect_eq(i, 4);

  static const int  expect_keys[4] = {1, 2, 3, 4};
  static const char expect_vals[4] = {'a', 'B', 'c', 'd'};
  triax_expect_arreq(keys, expect_keys);
  triax_expect_arreq(vals, expect_vals);

  bst_release(int, char, &b);
}

triax_test(bst, foreach_reversed_empty) {
  Bst(int, char) b = bst_init(int, char);
  TreeNode* stack[4];
  int        visited = 0;
  bst_foreach_reversed(&b, stack, 4, e) {
    (void)e;
    ++visited;
  }
  triax_expect_eq(visited, 0);
  bst_release(int, char, &b);
}

triax_test(bst, foreach_reversed_visits_descending) {
  Bst(int, char) b = bst_init(int, char);
  bst_set(int, char, &b, 3, 'c');
  bst_set(int, char, &b, 1, 'a');
  bst_set(int, char, &b, 4, 'd');
  bst_set(int, char, &b, 2, 'b');

  TreeNode* stack[4];
  int        keys[4] = {0};
  int        i       = 0;
  bst_foreach_reversed(&b, stack, 4, e) { keys[i++] = e->key; }
  triax_expect_eq(i, 4);

  static const int expect_keys[4] = {4, 3, 2, 1};
  triax_expect_arreq(keys, expect_keys);

  bst_release(int, char, &b);
}

// Regression test: bst_foreach's loop variable used to be exposed through an innermost "do-once"
// loop nested inside the real traversal loop. That inner loop always ran to completion regardless
// of the body, so a user's `break` only ever exited it and fell through to the real loop's own
// re-check (which unconditionally advances to the next node) -- making `break` silently behave
// like `continue` instead of stopping traversal.
triax_test(bst, foreach_break_stops_iteration) {
  Bst(int, char) b = bst_init(int, char);
  bst_set(int, char, &b, 1, 'a');
  bst_set(int, char, &b, 2, 'b');
  bst_set(int, char, &b, 3, 'c');
  bst_set(int, char, &b, 4, 'd');

  TreeNode* stack[4];
  int        visits = 0;
  bst_foreach(&b, stack, 4, e) {
    if (e->key == 2) { break; }
    ++visits;
  }
  triax_expect_eq(visits, 1); // just key 1

  bst_release(int, char, &b);
}

triax_test(bst, foreach_continue_skips_entry) {
  Bst(int, char) b = bst_init(int, char);
  bst_set(int, char, &b, 1, 'a');
  bst_set(int, char, &b, 2, 'b');
  bst_set(int, char, &b, 3, 'c');
  bst_set(int, char, &b, 4, 'd');

  TreeNode* stack[4];
  int        visits = 0;
  bst_foreach(&b, stack, 4, e) {
    if (e->key == 2) { continue; }
    ++visits;
  }
  triax_expect_eq(visits, 3); // all but key 2

  bst_release(int, char, &b);
}

triax_test(bst, foreach_reversed_break_stops_iteration) {
  Bst(int, char) b = bst_init(int, char);
  bst_set(int, char, &b, 1, 'a');
  bst_set(int, char, &b, 2, 'b');
  bst_set(int, char, &b, 3, 'c');
  bst_set(int, char, &b, 4, 'd');

  TreeNode* stack[4];
  int        visits = 0;
  bst_foreach_reversed(&b, stack, 4, e) {
    if (e->key == 3) { break; }
    ++visits;
  }
  triax_expect_eq(visits, 1); // just key 4

  bst_release(int, char, &b);
}

triax_test(bst, foreach_reversed_continue_skips_entry) {
  Bst(int, char) b = bst_init(int, char);
  bst_set(int, char, &b, 1, 'a');
  bst_set(int, char, &b, 2, 'b');
  bst_set(int, char, &b, 3, 'c');
  bst_set(int, char, &b, 4, 'd');

  TreeNode* stack[4];
  int        visits = 0;
  bst_foreach_reversed(&b, stack, 4, e) {
    if (e->key == 3) { continue; }
    ++visits;
  }
  triax_expect_eq(visits, 3); // all but key 3

  bst_release(int, char, &b);
}

triax_test(bst, erase_if_removes_matching_entries) {
  Bst(int, char) b = bst_init(int, char);
  bst_set(int, char, &b, 1, 'a');
  bst_set(int, char, &b, 2, 'b');
  bst_set(int, char, &b, 3, 'c');
  bst_set(int, char, &b, 4, 'd');

  TreeNode* stack[4];
  bst_erase_if(int, char, &b, stack, 4, e, e->key % 2 == 0);

  triax_expect_eq(bst_count(&b), 2u);
  triax_expect_false(bst_contains(int, char, &b, 2));
  triax_expect_false(bst_contains(int, char, &b, 4));
  triax_expect_true(bst_contains(int, char, &b, 1));
  triax_expect_true(bst_contains(int, char, &b, 3));

  // remaining entries must still traverse in sorted order
  int keys[2] = {0};
  int i       = 0;
  bst_foreach(&b, stack, 4, e) { keys[i++] = e->key; }
  triax_expect_eq(i, 2);
  static const int expect_keys[2] = {1, 3};
  triax_expect_arreq(keys, expect_keys);

  bst_release(int, char, &b);
}

triax_test(bst, foreach_const_entries) {
  Bst(int, char) t = bst_init(int, char);
  bst_set(int, char, &t, 1, 'a');
  TreeNode* stack[2];
  tree_foreach(&t, stack, 2, e) {
    static_assert(_Generic(e, BstEntry(int, char)*: 1, default: 0), "Mutable tree entry");
    e->val = 'b';
  }
  const Bst(int, char)* ct = &t;
  int seen = 0;
  tree_foreach(ct, stack, 2, e) {
    static_assert(_Generic(e, const BstEntry(int, char)*: 1, default: 0), "Const tree entry");
    triax_expect_eq(e->val, 'b');
    ++seen;
  }
  tree_foreach_reversed(ct, stack, 2, e) {
    static_assert(_Generic(e, const BstEntry(int, char)*: 1, default: 0), "Const tree entry");
    triax_expect_eq(e->val, 'b');
    ++seen;
  }
  triax_expect_eq(seen, 2);
  bst_release(int, char, &t);
}

triax_test(bst, erase_if_changes_root_then_release) {
  Bst(int, char) t = bst_init(int, char);
  bst_set(int, char, &t, 2, 'b');
  bst_set(int, char, &t, 1, 'a');
  bst_set(int, char, &t, 3, 'c');
  TreeNode* stack[3];
  bst_erase_if(int, char, &t, stack, 3, e, e->key < 3);
  triax_expect_eq(tree_count(&t), 1u);
  triax_expect_eq(tree_min(&t).key, 3);
  bst_release(int, char, &t);
  triax_expect_null(t.root);
}

triax_test(bst, erase_if_removes_all) {
  Bst(int, char) t = bst_init(int, char);
  for (int i = 1; i <= 4; ++i) { bst_set(int, char, &t, i, 'a'); }
  TreeNode* stack[4];
  bst_erase_if(int, char, &t, stack, 4, e, ((void)e, true));
  triax_expect_true(tree_is_empty(&t));
  triax_expect_null(t.root);
  bst_release(int, char, &t);
}

#if RK_CUSTOM_ALLOCATORS
triax_test(bst, erase_if_scratch_allocation_size) {
  alignas_max unsigned char storage[1024];
  Arena arena = arena_init_static(storage);
  Bst(int, char) t = bst_init(int, char, arena_to_alloc(&arena));
  for (int i = 1; i <= 4; ++i) { bst_set(int, char, &t, i, 'a'); }
  size_t before = arena_used(&arena);
  TreeNode* stack[4];
  // Nodes precede the scratch allocation and cannot be reclaimed while it is live.
  // Freeing the scratch buffer with its original size must restore the cursor.
  bst_erase_if(int, char, &t, stack, 4, e, e->key % 2 == 0);
  triax_expect_eq(arena_used(&arena), before);
  bst_release(int, char, &t);
}
#endif

triax_test(bst, erase_if_empty_is_noop) {
  Bst(int, char) b = bst_init(int, char);
  TreeNode* stack[4];
  int        pred_calls = 0;
  bst_erase_if(int, char, &b, stack, 4, e, (++pred_calls, (void)e, true));
  triax_expect_eq(pred_calls, 0);
  triax_expect_eq(bst_count(&b), 0u);
  bst_release(int, char, &b);
}

triax_test(bst, erase_if_evaluates_predicate_once_per_entry) {
  Bst(int, char) b = bst_init(int, char);
  bst_set(int, char, &b, 1, 'a');
  bst_set(int, char, &b, 2, 'b');
  bst_set(int, char, &b, 3, 'c');

  TreeNode* stack[4];
  int        pred_calls = 0;
  bst_erase_if(int, char, &b, stack, 4, e, (++pred_calls, (void)e, false));
  triax_expect_eq(pred_calls, 3);
  triax_expect_eq(bst_count(&b), 3u);

  bst_release(int, char, &b);
}

triax_test(bst, extract) {
  Bst(int, char) b = bst_init(int, char);
  bst_set(int, char, &b, 1, 'a');
  bst_set(int, char, &b, 2, 'b');

  char out = 0;
  triax_expect_true(bst_extract(int, char, &b, 1, &out));
  triax_expect_eq(out, 'a');
  triax_expect_false(bst_contains(int, char, &b, 1));
  triax_expect_eq(bst_count(&b), 1u);

  triax_expect_false(bst_extract(int, char, &b, 1, &out));

  bst_release(int, char, &b);
}

triax_test(bst, remove_sequence) {
  Bst(int, char) b = bst_init(int, char);
  bst_set(int, char, &b, 1, 'a');
  bst_set(int, char, &b, 2, 'b');
  bst_set(int, char, &b, 3, 'c');
  bst_set(int, char, &b, 4, 'd');

  triax_expect_true(bst_contains(int, char, &b, 2));
  bst_remove(int, char, &b, 2);
  triax_expect_false(bst_contains(int, char, &b, 2));
  triax_expect_eq(bst_count(&b), 3u);

  triax_expect_true(bst_contains(int, char, &b, 3));
  bst_remove(int, char, &b, 3);
  triax_expect_false(bst_contains(int, char, &b, 3));
  triax_expect_eq(bst_count(&b), 2u);

  triax_expect_true(bst_contains(int, char, &b, 4));
  bst_remove(int, char, &b, 4);
  triax_expect_false(bst_contains(int, char, &b, 4));
  triax_expect_eq(bst_count(&b), 1u);

  triax_expect_true(bst_contains(int, char, &b, 1));
  bst_remove(int, char, &b, 1);
  triax_expect_eq(bst_count(&b), 0u);
  triax_expect_true(bst_is_empty(&b));
  triax_expect_null(bst_peek_min(&b));
  triax_expect_null(bst_peek_max(&b));

  // removing a missing key from an empty tree is harmless
  bst_remove(int, char, &b, 999);
  triax_expect_eq(bst_count(&b), 0u);

  bst_release(int, char, &b);
}

triax_test(bst, release_and_reuse) {
  Bst(int, char) b = bst_init(int, char);
  bst_set(int, char, &b, 1, 'a');

  bst_release(int, char, &b);
  triax_expect_true(bst_is_empty(&b));
  triax_expect_eq(bst_count(&b), 0u);
  triax_expect_null(bst_peek_min(&b));
  triax_expect_null(bst_peek_max(&b));

  // tree should still be usable after release
  triax_expect_true(bst_set(int, char, &b, 42, 'x'));
  triax_expect_eq(bst_count(&b), 1u);
  triax_expect_true(bst_contains(int, char, &b, 42));
  triax_expect_eq(*bst_get(int, char, &b, 42), 'x');

  bst_release(int, char, &b);
  triax_expect_true(bst_is_empty(&b));
  triax_expect_eq(bst_count(&b), 0u);
}

triax_test(bst, remove_two_children) {
  /* Build:        5
                 /   \
                3     7
               / \   / \
              1   4 6   8 */
  Bst(int, char) b = bst_init(int, char);
  bst_set(int, char, &b, 5, 'e');
  bst_set(int, char, &b, 3, 'c');
  bst_set(int, char, &b, 7, 'g');
  bst_set(int, char, &b, 1, 'a');
  bst_set(int, char, &b, 4, 'd');
  bst_set(int, char, &b, 6, 'f');
  bst_set(int, char, &b, 8, 'h');
  triax_expect_eq(bst_count(&b), 7u);

  // Remove 3 (two children: 1 and 4); in-order successor (4) replaces it
  triax_expect_true(bst_remove(int, char, &b, 3));
  triax_expect_eq(bst_count(&b), 6u);
  triax_expect_false(bst_contains(int, char, &b, 3));
  triax_expect_true(bst_contains(int, char, &b, 1));
  triax_expect_eq(*bst_get(int, char, &b, 1), 'a');
  triax_expect_true(bst_contains(int, char, &b, 4));
  triax_expect_eq(*bst_get(int, char, &b, 4), 'd');

  // Remove 7 (two children: 6 and 8); in-order successor (8) replaces it
  triax_expect_true(bst_remove(int, char, &b, 7));
  triax_expect_eq(bst_count(&b), 5u);
  triax_expect_false(bst_contains(int, char, &b, 7));
  triax_expect_true(bst_contains(int, char, &b, 6));
  triax_expect_eq(*bst_get(int, char, &b, 6), 'f');
  triax_expect_true(bst_contains(int, char, &b, 8));
  triax_expect_eq(*bst_get(int, char, &b, 8), 'h');

  // BST property: in-order traversal must still be sorted
  TreeNode* stack[8];
  int        keys[5];
  int        i = 0;
  bst_foreach(&b, stack, 8, e) { keys[i++] = e->key; }
  triax_expect_eq(i, 5);
  for (int j = 1; j < 5; ++j) { triax_expect_gt(keys[j], keys[j - 1]); }

  bst_release(int, char, &b);
}

triax_test(bst, large_sorted_order) {
  Bst(int, char) b = bst_init(int, char);

  // Insert 1..32 in descending order (worst-case skewed tree)
  for (int i = 32; i >= 1; --i) { bst_set(int, char, &b, i, (char)('a' + (i - 1) % 26)); }
  triax_expect_eq(bst_count(&b), 32u);
  triax_expect_eq(bst_min(&b).key, 1);
  triax_expect_eq(bst_max(&b).key, 32);

  // In-order traversal must produce strictly ascending keys
  TreeNode* stack[32];
  int        prev = -1, count = 0;
  bst_foreach(&b, stack, 32, e) {
    triax_expect_gt(e->key, prev);
    prev = e->key;
    ++count;
  }
  triax_expect_eq(count, 32);

  bst_release(int, char, &b);
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
} BstRoundtripCase;

static const BstRoundtripCase bst_roundtrip_cases[] = {
    {"positive", 42, 'x'},     {"zero", 0, '0'},          {"negative", -17, 'n'},
    {"int_min", INT_MIN, 'm'}, {"int_max", INT_MAX, 'M'},
};

triax_test(bst, single_key_roundtrip, .params = triax_as_params(bst_roundtrip_cases)) {
  const BstRoundtripCase* c = triax_param(BstRoundtripCase);
  Bst(int, char) b          = bst_init(int, char);

  triax_expect(bst_set(int, char, &b, c->key, c->value), "case: %s", c->name);
  triax_expect_eq(bst_count(&b), 1u);
  triax_expect_true(bst_contains(int, char, &b, c->key));

  char* p = bst_get(int, char, &b, c->key);
  triax_expect_nonnull(p);
  if (p) { triax_expect_eq(*p, c->value); }

  BstEntry(int, char)* mn = bst_peek_min(&b);
  BstEntry(int, char)* mx = bst_peek_max(&b);
  triax_expect_nonnull(mn), triax_expect_nonnull(mx);
  if (mn) { triax_expect_eq(mn->key, c->key); }
  if (mx) { triax_expect_eq(mx->key, c->key); }

  triax_expect_true(bst_remove(int, char, &b, c->key));
  triax_expect_true(bst_is_empty(&b));

  bst_release(int, char, &b);
}

/* ------------------------------------------------------------------------ */
/* bst_foreach's traversal stack overflow is an rk_assert (abort) in debug  */
/* builds, not a caught/returned error -- exercise it via triax_assert_fault */
/* rather than skip it just because it's a crash, matching the convention   */
/* already used for arena/pool allocator-failure paths. Isolation is set    */
/* explicitly (matching that same convention) even though it's already the  */
/* framework default, since getting this wrong would crash the whole suite. */
/* ------------------------------------------------------------------------ */
triax_test(bst, foreach_stack_too_small_asserts, .isolation = TRIAX_ISOLATION_ON) {
  Bst(int, char) b = bst_init(int, char);
  // Descending insertion order builds a purely left-skewed chain (see
  // large_sorted_order above), so the in-order traversal's stack depth
  // requirement equals the full height: 8.
  for (int i = 8; i >= 1; --i) { bst_set(int, char, &b, i, (char)('a' + i - 1)); }

  TreeNode* stack[1]; // deliberately smaller than the tree's height
  triax_assert_fault(TRIAX_FAULT_ABORT, {
    bst_foreach(&b, stack, 1, e) { (void)e;
}
});

bst_release(int, char, &b);
}

RKI_IGNWARN_CLANG_END()
RKI_HEADER_END
#endif
