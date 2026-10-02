#ifndef TEST_AVL_H
#define TEST_AVL_H
#include "conf.h"

#define RK_IMPL
#include "../include/rklib.h"
#include <limits.h>

RK_HEADER_BEGIN
RKI_IGNWARN_CLANG_BEG("-Wunused-variable")

extern_fun int avl_int_cmp(int a, int b) { return a < b ? -1 : (a == b ? 0 : 1); }

AVL_DEFINE(int, char, avl_int_cmp)

triax_test(avl, empty) {
  Avl(int, char) a = avl_init(int, char);

  triax_expect_true(avl_is_empty(&a));
  triax_expect_eq(avl_count(&a), 0u);
  triax_expect_null(avl_min(&a));
  triax_expect_null(avl_max(&a));
  triax_expect_null(avl_get(int, char, &a, 123));
  triax_expect_false(avl_contains(int, char, &a, 123));

  char out = 0;
  triax_expect_false(avl_extract(int, char, &a, 123, &out));

  avl_release(int, char, &a);
}

triax_test(avl, zero_initialized) {
  Avl(int, char) a = {0};
  triax_expect_true(avl_is_empty(&a));
  triax_expect_eq(avl_count(&a), 0u);
  triax_expect_null(avl_min(&a));
  triax_expect_null(avl_max(&a));

  triax_expect_true(avl_set(int, char, &a, 7, 'g'));
  triax_expect_eq(avl_count(&a), 1u);
  triax_expect_true(avl_contains(int, char, &a, 7));
  triax_expect_eq(*avl_get(int, char, &a, 7), 'g');

  avl_release(int, char, &a);
  triax_expect_true(avl_is_empty(&a));
}

triax_test(avl, insert_and_lookup) {
  Avl(int, char) a = avl_init(int, char);

  triax_expect_true(avl_set(int, char, &a, 3, 'c'));
  triax_expect_true(avl_set(int, char, &a, 1, 'a'));
  triax_expect_true(avl_set(int, char, &a, 4, 'd'));
  triax_expect_true(avl_set(int, char, &a, 2, 'b'));

  triax_expect_false(avl_is_empty(&a));
  triax_expect_eq(avl_count(&a), 4u);

  char* p1 = avl_get(int, char, &a, 1);
  char* p2 = avl_get(int, char, &a, 2);
  char* p3 = avl_get(int, char, &a, 3);
  char* p4 = avl_get(int, char, &a, 4);
  triax_expect_nonnull(p1), triax_expect_nonnull(p2);
  triax_expect_nonnull(p3), triax_expect_nonnull(p4);
  if (p1) { triax_expect_eq(*p1, 'a'); }
  if (p2) { triax_expect_eq(*p2, 'b'); }
  if (p3) { triax_expect_eq(*p3, 'c'); }
  if (p4) { triax_expect_eq(*p4, 'd'); }
  triax_expect_null(avl_get(int, char, &a, 99));

  triax_expect_true(avl_contains(int, char, &a, 1));
  triax_expect_true(avl_contains(int, char, &a, 2));
  triax_expect_true(avl_contains(int, char, &a, 3));
  triax_expect_true(avl_contains(int, char, &a, 4));
  triax_expect_false(avl_contains(int, char, &a, 99));

  AvlEntry(int, char)* mn = avl_min(&a);
  AvlEntry(int, char)* mx = avl_max(&a);
  triax_expect_nonnull(mn), triax_expect_nonnull(mx);
  if (mn) { triax_expect_eq(mn->key, 1), triax_expect_eq(mn->val, 'a'); }
  if (mx) { triax_expect_eq(mx->key, 4), triax_expect_eq(mx->val, 'd'); }

  avl_release(int, char, &a);
}

triax_test(avl, add_does_not_overwrite_set_does) {
  Avl(int, char) a = avl_init(int, char);
  avl_set(int, char, &a, 2, 'b');

  triax_expect_false(avl_add(int, char, &a, 2, 'X'));
  triax_expect_eq(*avl_get(int, char, &a, 2), 'b');

  triax_expect_false(avl_set(int, char, &a, 2, 'B'));
  triax_expect_eq(*avl_get(int, char, &a, 2), 'B');
  triax_expect_eq(avl_count(&a), 1u);

  avl_release(int, char, &a);
}

triax_test(avl, get_or_add) {
  Avl(int, char) a = avl_init(int, char);

  bool  inserted   = false;
  char* p1         = avl_get_or_add(int, char, &a, 5, 'e', &inserted);
  triax_expect_true(inserted);
  triax_expect_eq(*p1, 'e');
  triax_expect_eq(avl_count(&a), 1u);

  // key already present: returns existing value, does not overwrite, does not insert
  inserted = true;
  char* p2 = avl_get_or_add(int, char, &a, 5, 'z', &inserted);
  triax_expect_false(inserted);
  triax_expect_eq(*p2, 'e');
  triax_expect_eq(avl_count(&a), 1u);
  // cast to void*: triax_expect_eq treats char* as a C-string (via strlen), but p1/p2 point at a
  // single, non-null-terminated char field inside the Avl node -- this must be a pointer-identity
  // check (same address returned for an already-present key), not a string comparison.
  triax_expect_eq((void*)p1, (void*)p2);

  // a second, distinct key still triggers a real insertion
  inserted = false;
  char* p3 = avl_get_or_add(int, char, &a, 9, 'i', &inserted);
  triax_expect_true(inserted);
  triax_expect_eq(*p3, 'i');
  triax_expect_eq(avl_count(&a), 2u);

  triax_expect_eq(*avl_get(int, char, &a, 5), 'e');
  triax_expect_eq(*avl_get(int, char, &a, 9), 'i');

  avl_release(int, char, &a);
}

triax_test(avl, foreach_empty) {
  Avl(int, char) a = avl_init(int, char);
  tree_node* stack[4];
  int        visited = 0;
  avl_foreach(&a, stack, 4, e) {
    (void)e;
    ++visited;
  }
  triax_expect_eq(visited, 0);
  avl_release(int, char, &a);
}

triax_test(avl, foreach_inorder) {
  Avl(int, char) a = avl_init(int, char);
  avl_set(int, char, &a, 3, 'c');
  avl_set(int, char, &a, 1, 'a');
  avl_set(int, char, &a, 4, 'd');
  avl_set(int, char, &a, 2, 'B');

  tree_node* stack[4];
  int        keys[4] = {0};
  char       vals[4] = {0};
  int        i       = 0;

  avl_foreach(&a, stack, 4, e) {
    keys[i] = e->key;
    vals[i] = e->val;
    ++i;
  }
  triax_expect_eq(i, 4);

  static const int  expect_keys[4] = {1, 2, 3, 4};
  static const char expect_vals[4] = {'a', 'B', 'c', 'd'};
  triax_expect_arreq(keys, expect_keys);
  triax_expect_arreq(vals, expect_vals);

  avl_release(int, char, &a);
}

triax_test(avl, foreach_reversed_empty) {
  Avl(int, char) a = avl_init(int, char);
  tree_node* stack[4];
  int        visited = 0;
  avl_foreach_reversed(&a, stack, 4, e) {
    (void)e;
    ++visited;
  }
  triax_expect_eq(visited, 0);
  avl_release(int, char, &a);
}

triax_test(avl, foreach_reversed_visits_descending) {
  Avl(int, char) a = avl_init(int, char);
  avl_set(int, char, &a, 3, 'c');
  avl_set(int, char, &a, 1, 'a');
  avl_set(int, char, &a, 4, 'd');
  avl_set(int, char, &a, 2, 'b');

  tree_node* stack[4];
  int        keys[4] = {0};
  int        i       = 0;
  avl_foreach_reversed(&a, stack, 4, e) { keys[i++] = e->key; }
  triax_expect_eq(i, 4);

  static const int expect_keys[4] = {4, 3, 2, 1};
  triax_expect_arreq(keys, expect_keys);

  avl_release(int, char, &a);
}

triax_test(avl, foreach_break_stops_iteration) {
  Avl(int, char) a = avl_init(int, char);
  avl_set(int, char, &a, 1, 'a');
  avl_set(int, char, &a, 2, 'b');
  avl_set(int, char, &a, 3, 'c');
  avl_set(int, char, &a, 4, 'd');

  tree_node* stack[4];
  int        visits = 0;
  avl_foreach(&a, stack, 4, e) {
    if (e->key == 2) { break; }
    ++visits;
  }
  triax_expect_eq(visits, 1); // just key 1

  avl_release(int, char, &a);
}

triax_test(avl, foreach_continue_skips_entry) {
  Avl(int, char) a = avl_init(int, char);
  avl_set(int, char, &a, 1, 'a');
  avl_set(int, char, &a, 2, 'b');
  avl_set(int, char, &a, 3, 'c');
  avl_set(int, char, &a, 4, 'd');

  tree_node* stack[4];
  int        visits = 0;
  avl_foreach(&a, stack, 4, e) {
    if (e->key == 2) { continue; }
    ++visits;
  }
  triax_expect_eq(visits, 3); // all but key 2

  avl_release(int, char, &a);
}

triax_test(avl, foreach_reversed_break_stops_iteration) {
  Avl(int, char) a = avl_init(int, char);
  avl_set(int, char, &a, 1, 'a');
  avl_set(int, char, &a, 2, 'b');
  avl_set(int, char, &a, 3, 'c');
  avl_set(int, char, &a, 4, 'd');

  tree_node* stack[4];
  int        visits = 0;
  avl_foreach_reversed(&a, stack, 4, e) {
    if (e->key == 3) { break; }
    ++visits;
  }
  triax_expect_eq(visits, 1); // just key 4

  avl_release(int, char, &a);
}

triax_test(avl, foreach_reversed_continue_skips_entry) {
  Avl(int, char) a = avl_init(int, char);
  avl_set(int, char, &a, 1, 'a');
  avl_set(int, char, &a, 2, 'b');
  avl_set(int, char, &a, 3, 'c');
  avl_set(int, char, &a, 4, 'd');

  tree_node* stack[4];
  int        visits = 0;
  avl_foreach_reversed(&a, stack, 4, e) {
    if (e->key == 3) { continue; }
    ++visits;
  }
  triax_expect_eq(visits, 3); // all but key 3

  avl_release(int, char, &a);
}

triax_test(avl, erase_if_removes_matching_entries) {
  Avl(int, char) a = avl_init(int, char);
  avl_set(int, char, &a, 1, 'a');
  avl_set(int, char, &a, 2, 'b');
  avl_set(int, char, &a, 3, 'c');
  avl_set(int, char, &a, 4, 'd');

  tree_node* stack[4];
  avl_erase_if(int, char, &a, stack, 4, e, e->key % 2 == 0);

  triax_expect_eq(avl_count(&a), 2u);
  triax_expect_false(avl_contains(int, char, &a, 2));
  triax_expect_false(avl_contains(int, char, &a, 4));
  triax_expect_true(avl_contains(int, char, &a, 1));
  triax_expect_true(avl_contains(int, char, &a, 3));

  int keys[2] = {0};
  int i       = 0;
  avl_foreach(&a, stack, 4, e) { keys[i++] = e->key; }
  triax_expect_eq(i, 2);
  static const int expect_keys[2] = {1, 3};
  triax_expect_arreq(keys, expect_keys);

  avl_release(int, char, &a);
}

triax_test(avl, erase_if_empty_is_noop) {
  Avl(int, char) a          = avl_init(int, char);
  tree_node*     stack[4];
  int            pred_calls = 0;
  avl_erase_if(int, char, &a, stack, 4, e, (++pred_calls, (void)e, true));
  triax_expect_eq(pred_calls, 0);
  triax_expect_eq(avl_count(&a), 0u);
  avl_release(int, char, &a);
}

triax_test(avl, erase_if_evaluates_predicate_once_per_entry) {
  Avl(int, char) a = avl_init(int, char);
  avl_set(int, char, &a, 1, 'a');
  avl_set(int, char, &a, 2, 'b');
  avl_set(int, char, &a, 3, 'c');

  tree_node* stack[4];
  int        pred_calls = 0;
  avl_erase_if(int, char, &a, stack, 4, e, (++pred_calls, (void)e, false));
  triax_expect_eq(pred_calls, 3);
  triax_expect_eq(avl_count(&a), 3u);

  avl_release(int, char, &a);
}

triax_test(avl, extract) {
  Avl(int, char) a = avl_init(int, char);
  avl_set(int, char, &a, 1, 'a');
  avl_set(int, char, &a, 2, 'b');

  char out = 0;
  triax_expect_true(avl_extract(int, char, &a, 1, &out));
  triax_expect_eq(out, 'a');
  triax_expect_false(avl_contains(int, char, &a, 1));
  triax_expect_eq(avl_count(&a), 1u);

  triax_expect_false(avl_extract(int, char, &a, 1, &out));

  avl_release(int, char, &a);
}

triax_test(avl, remove_sequence) {
  Avl(int, char) a = avl_init(int, char);
  avl_set(int, char, &a, 1, 'a');
  avl_set(int, char, &a, 2, 'b');
  avl_set(int, char, &a, 3, 'c');
  avl_set(int, char, &a, 4, 'd');

  triax_expect_true(avl_contains(int, char, &a, 2));
  avl_remove(int, char, &a, 2);
  triax_expect_false(avl_contains(int, char, &a, 2));
  triax_expect_eq(avl_count(&a), 3u);

  triax_expect_true(avl_contains(int, char, &a, 3));
  avl_remove(int, char, &a, 3);
  triax_expect_false(avl_contains(int, char, &a, 3));
  triax_expect_eq(avl_count(&a), 2u);

  triax_expect_true(avl_contains(int, char, &a, 4));
  avl_remove(int, char, &a, 4);
  triax_expect_false(avl_contains(int, char, &a, 4));
  triax_expect_eq(avl_count(&a), 1u);

  triax_expect_true(avl_contains(int, char, &a, 1));
  avl_remove(int, char, &a, 1);
  triax_expect_eq(avl_count(&a), 0u);
  triax_expect_true(avl_is_empty(&a));
  triax_expect_null(avl_min(&a));
  triax_expect_null(avl_max(&a));

  // removing a missing key from an empty tree is harmless
  avl_remove(int, char, &a, 999);
  triax_expect_eq(avl_count(&a), 0u);

  avl_release(int, char, &a);
}

triax_test(avl, release_and_reuse) {
  Avl(int, char) a = avl_init(int, char);
  avl_set(int, char, &a, 1, 'a');

  avl_release(int, char, &a);
  triax_expect_true(avl_is_empty(&a));
  triax_expect_eq(avl_count(&a), 0u);
  triax_expect_null(avl_min(&a));
  triax_expect_null(avl_max(&a));

  // tree should still be usable after release
  triax_expect_true(avl_set(int, char, &a, 42, 'x'));
  triax_expect_eq(avl_count(&a), 1u);
  triax_expect_true(avl_contains(int, char, &a, 42));
  triax_expect_eq(*avl_get(int, char, &a, 42), 'x');

  avl_release(int, char, &a);
  triax_expect_true(avl_is_empty(&a));
  triax_expect_eq(avl_count(&a), 0u);
}

triax_test(avl, remove_two_children) {
  /* Build:        5
                 /   \
                3     7
               / \   / \
              1   4 6   8 */
  Avl(int, char) a = avl_init(int, char);
  avl_set(int, char, &a, 5, 'e');
  avl_set(int, char, &a, 3, 'c');
  avl_set(int, char, &a, 7, 'g');
  avl_set(int, char, &a, 1, 'a');
  avl_set(int, char, &a, 4, 'd');
  avl_set(int, char, &a, 6, 'f');
  avl_set(int, char, &a, 8, 'h');
  triax_expect_eq(avl_count(&a), 7u);

  // Remove 3 (two children: 1 and 4); in-order successor replaces it
  triax_expect_true(avl_remove(int, char, &a, 3));
  triax_expect_eq(avl_count(&a), 6u);
  triax_expect_false(avl_contains(int, char, &a, 3));
  triax_expect_true(avl_contains(int, char, &a, 1));
  triax_expect_eq(*avl_get(int, char, &a, 1), 'a');
  triax_expect_true(avl_contains(int, char, &a, 4));
  triax_expect_eq(*avl_get(int, char, &a, 4), 'd');

  // Remove 7 (two children: 6 and 8); in-order successor replaces it
  triax_expect_true(avl_remove(int, char, &a, 7));
  triax_expect_eq(avl_count(&a), 5u);
  triax_expect_false(avl_contains(int, char, &a, 7));
  triax_expect_true(avl_contains(int, char, &a, 6));
  triax_expect_eq(*avl_get(int, char, &a, 6), 'f');
  triax_expect_true(avl_contains(int, char, &a, 8));
  triax_expect_eq(*avl_get(int, char, &a, 8), 'h');

  // BST property: in-order traversal must still be sorted
  tree_node* stack[8];
  int        keys[5];
  int        i = 0;
  avl_foreach(&a, stack, 8, e) { keys[i++] = e->key; }
  triax_expect_eq(i, 5);
  for (int j = 1; j < 5; ++j) { triax_expect_gt(keys[j], keys[j - 1]); }

  avl_release(int, char, &a);
}

triax_test(avl, large_sorted_order_stays_balanced) {
  Avl(int, char) a = avl_init(int, char);

  // Insert 1..32 in ascending order: the worst case for an unbalanced BST
  // (degenerates into a linked list of height 32), but AVL must keep this
  // bounded to O(log n). A stack sized for only 8 entries would overflow a
  // degenerate tree of height 32 but comfortably covers AVL's proven
  // worst-case height bound of ~1.44*log2(n+2) (7 for n=32).
  for (int i = 1; i <= 32; ++i) { avl_set(int, char, &a, i, (char)('a' + (i - 1) % 26)); }
  triax_expect_eq(avl_count(&a), 32u);
  triax_expect_eq(avl_min(&a)->key, 1);
  triax_expect_eq(avl_max(&a)->key, 32);

  tree_node* stack[8];
  int        prev = -1, count = 0;
  avl_foreach(&a, stack, 8, e) {
    triax_expect_gt(e->key, prev);
    prev = e->key;
    ++count;
  }
  triax_expect_eq(count, 32);

  avl_release(int, char, &a);
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
} AvlRoundtripCase;

static const AvlRoundtripCase avl_roundtrip_cases[] = {
    {"positive", 42, 'x'},     {"zero", 0, '0'},          {"negative", -17, 'n'},
    {"int_min", INT_MIN, 'm'}, {"int_max", INT_MAX, 'M'},
};

triax_test(avl, single_key_roundtrip, .params = triax_as_params(avl_roundtrip_cases)) {
  const AvlRoundtripCase* c = triax_param(AvlRoundtripCase);
  Avl(int, char) a          = avl_init(int, char);

  triax_expect(avl_set(int, char, &a, c->key, c->value), "case: %s", c->name);
  triax_expect_eq(avl_count(&a), 1u);
  triax_expect_true(avl_contains(int, char, &a, c->key));

  char* p = avl_get(int, char, &a, c->key);
  triax_expect_nonnull(p);
  if (p) { triax_expect_eq(*p, c->value); }

  AvlEntry(int, char)* mn = avl_min(&a);
  AvlEntry(int, char)* mx = avl_max(&a);
  triax_expect_nonnull(mn), triax_expect_nonnull(mx);
  if (mn) { triax_expect_eq(mn->key, c->key); }
  if (mx) { triax_expect_eq(mx->key, c->key); }

  triax_expect_true(avl_remove(int, char, &a, c->key));
  triax_expect_true(avl_is_empty(&a));

  avl_release(int, char, &a);
}

/* ------------------------------------------------------------------------ */
/* avl_foreach's traversal stack overflow is an rk_assert (abort) in debug  */
/* builds, not a caught/returned error -- exercise it via triax_assert_fault */
/* rather than skip it just because it's a crash, matching the convention   */
/* already used for arena/pool allocator-failure paths. Unlike Bst, AVL     */
/* actively rebalances, so 3 keys are already enough to force a height-2    */
/* tree (a chain would need 3 keys deep; AVL rotates that down to 2) --     */
/* a stack of 1 is smaller than any tree of height >= 2 regardless of shape.*/
/* Isolation is set explicitly even though it's already the framework       */
/* default, since getting this wrong would crash the whole suite.           */
/* ------------------------------------------------------------------------ */
triax_test(avl, foreach_stack_too_small_asserts, .isolation = TRIAX_ISOLATION_ON) {
  Avl(int, char) a = avl_init(int, char);
  avl_set(int, char, &a, 1, 'a');
  avl_set(int, char, &a, 2, 'b');
  avl_set(int, char, &a, 3, 'c');

  tree_node* stack[1]; // deliberately smaller than the tree's height
  triax_assert_fault(TRIAX_FAULT_ABORT, {
    avl_foreach(&a, stack, 1, e) { (void)e;
}
});

avl_release(int, char, &a);
}

RKI_IGNWARN_CLANG_END()
RK_HEADER_END
#endif
