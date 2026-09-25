#ifndef TEST_RBT_H
#define TEST_RBT_H
#include "conf.h"

#define RK_IMPL
#include "../include/rklib.h"

RK_HEADER_BEGIN
RK__IGNWARN_CLANG_BEG("-Wunused-variable")

extern_fun int rbt_int_cmp(int a, int b) { return a < b ? -1 : (a == b ? 0 : 1); }

RBT_DEFINE(int, char, rbt_int_cmp)

triax_test(rbt, rbttest) {
  /* ------------------------------------------------------------------------ */
  /* init / count / empty / min / max on empty                                */
  /* ------------------------------------------------------------------------ */
  Rbt(int, char) r = rbt_init(int, char);

  triax_assert_true(rbt_is_empty(&r));
  triax_assert_eq(rbt_count(&r), 0u);
  triax_assert_true(rbt_min(&r) == rk_null);
  triax_assert_true(rbt_max(&r) == rk_null);
  triax_assert_true(rbt_get(int, char, &r, 123) == rk_null);
  triax_assert_true(!rbt_contains(int, char, &r, 123));

  {
    char out = 0;
    triax_assert_true(!rbt_extract(int, char, &r, 123, &out));
  }

  /* ------------------------------------------------------------------------ */
  /* insert / search / contains / min / max                                   */
  /* ------------------------------------------------------------------------ */
  triax_assert_true(rbt_set(int, char, &r, 3, 'c'));
  triax_assert_true(rbt_set(int, char, &r, 1, 'a'));
  triax_assert_true(rbt_set(int, char, &r, 4, 'd'));
  triax_assert_true(rbt_set(int, char, &r, 2, 'b'));

  triax_assert_true(!rbt_is_empty(&r));
  triax_assert_eq(rbt_count(&r), 4u);

  {
    char* p1 = rbt_get(int, char, &r, 1);
    char* p2 = rbt_get(int, char, &r, 2);
    char* p3 = rbt_get(int, char, &r, 3);
    char* p4 = rbt_get(int, char, &r, 4);
    char* px = rbt_get(int, char, &r, 99);

    triax_assert_true(p1 && *p1 == 'a');
    triax_assert_true(p2 && *p2 == 'b');
    triax_assert_true(p3 && *p3 == 'c');
    triax_assert_true(p4 && *p4 == 'd');
    triax_assert_true(px == rk_null);
  }

  triax_assert_true(rbt_contains(int, char, &r, 1));
  triax_assert_true(rbt_contains(int, char, &r, 2));
  triax_assert_true(rbt_contains(int, char, &r, 3));
  triax_assert_true(rbt_contains(int, char, &r, 4));
  triax_assert_true(!rbt_contains(int, char, &r, 99));

  {
    RbtEntry(int, char)* mn = rbt_min(&r);
    RbtEntry(int, char)* mx = rbt_max(&r);

    triax_assert_true(mn != rk_null);
    triax_assert_true(mx != rk_null);
    triax_assert_eq(mn->key, 1);
    triax_assert_eq(mn->val, 'a');
    triax_assert_eq(mx->key, 4);
    triax_assert_eq(mx->val, 'd');
  }

  /* ------------------------------------------------------------------------ */
  /* try_insert / overwrite semantics of insert                               */
  /* ------------------------------------------------------------------------ */
  triax_assert_true(!rbt_add(int, char, &r, 2, 'X'));
  triax_assert_true(rbt_get(int, char, &r, 2) != rk_null);
  triax_assert_eq(*rbt_get(int, char, &r, 2), 'b');

  triax_assert_true(!rbt_set(int, char, &r, 2, 'B'));
  triax_assert_eq(*rbt_get(int, char, &r, 2), 'B');

  triax_assert_eq(rbt_count(&r), 4u);

  /* ------------------------------------------------------------------------ */
  /* foreach: sorted in-order traversal                                       */
  /* ------------------------------------------------------------------------ */
  {
    tree_node* stack[4];
    int        keys[4] = {0};
    char       vals[4] = {0};
    int        i       = 0;

    rbt_foreach(&r, stack, 4, e) {
      keys[i] = e->key;
      vals[i] = e->val;
      ++i;
    }

    triax_assert_eq(i, 4);
    triax_assert_eq(keys[0], 1);
    triax_assert_eq(keys[1], 2);
    triax_assert_eq(keys[2], 3);
    triax_assert_eq(keys[3], 4);

    triax_assert_eq(vals[0], 'a');
    triax_assert_eq(vals[1], 'B');
    triax_assert_eq(vals[2], 'c');
    triax_assert_eq(vals[3], 'd');
  }

  /* ------------------------------------------------------------------------ */
  /* extract */
  /* ------------------------------------------------------------------------ */
  {
    char out = 0;

    triax_assert_true(rbt_extract(int, char, &r, 1, &out));
    triax_assert_eq(out, 'a');
    triax_assert_true(!rbt_contains(int, char, &r, 1));
    triax_assert_eq(rbt_count(&r), 3u);

    triax_assert_true(!rbt_extract(int, char, &r, 1, &out));
  }

  /* ------------------------------------------------------------------------ */
  /* remove */
  /* ------------------------------------------------------------------------ */
  triax_assert_true(rbt_contains(int, char, &r, 2));
  rbt_remove(int, char, &r, 2);
  triax_assert_true(!rbt_contains(int, char, &r, 2));
  triax_assert_eq(rbt_count(&r), 2u);

  triax_assert_true(rbt_contains(int, char, &r, 3));
  rbt_remove(int, char, &r, 3);
  triax_assert_true(!rbt_contains(int, char, &r, 3));
  triax_assert_eq(rbt_count(&r), 1u);

  triax_assert_true(rbt_contains(int, char, &r, 4));
  rbt_remove(int, char, &r, 4);
  triax_assert_true(!rbt_contains(int, char, &r, 4));
  triax_assert_eq(rbt_count(&r), 0u);
  triax_assert_true(rbt_is_empty(&r));

  triax_assert_true(rbt_min(&r) == rk_null);
  triax_assert_true(rbt_max(&r) == rk_null);

  /* removing missing key should be harmless */
  rbt_remove(int, char, &r, 999);
  triax_assert_eq(rbt_count(&r), 0u);

  /* ------------------------------------------------------------------------ */
  /* release on empty / reused tree */
  /* ------------------------------------------------------------------------ */
  rbt_release(int, char, &r);
  triax_assert_true(rbt_is_empty(&r));
  triax_assert_eq(rbt_count(&r), 0u);
  triax_assert_true(rbt_min(&r) == rk_null);
  triax_assert_true(rbt_max(&r) == rk_null);

  /* tree should still be reusable after release */
  triax_assert_true(rbt_set(int, char, &r, 42, 'x'));
  triax_assert_eq(rbt_count(&r), 1u);
  triax_assert_true(rbt_contains(int, char, &r, 42));
  triax_assert_eq(*rbt_get(int, char, &r, 42), 'x');

  rbt_release(int, char, &r);
  triax_assert_true(rbt_is_empty(&r));
  triax_assert_eq(rbt_count(&r), 0u);
}

triax_test(rbt, zero_initialized) {
  Rbt(int, char) r = {0};
  triax_expect_true(rbt_is_empty(&r));
  triax_expect_eq(rbt_count(&r), 0u);
  triax_expect_null(rbt_min(&r));
  triax_expect_null(rbt_max(&r));

  triax_expect_true(rbt_set(int, char, &r, 7, 'g'));
  triax_expect_eq(rbt_count(&r), 1u);
  triax_expect_true(rbt_contains(int, char, &r, 7));
  triax_expect_eq(*rbt_get(int, char, &r, 7), 'g');

  rbt_release(int, char, &r);
  triax_expect_true(rbt_is_empty(&r));
}

triax_test(rbt, get_or_add) {
  Rbt(int, char) r = rbt_init(int, char);

  bool  inserted = false;
  char* p1       = rbt_get_or_add(int, char, &r, 5, 'e', &inserted);
  triax_expect_true(inserted);
  triax_expect_eq(*p1, 'e');
  triax_expect_eq(rbt_count(&r), 1u);

  // key already present: returns existing value, does not overwrite, does not insert
  inserted = true;
  char* p2 = rbt_get_or_add(int, char, &r, 5, 'z', &inserted);
  triax_expect_false(inserted);
  triax_expect_eq(*p2, 'e');
  triax_expect_eq(rbt_count(&r), 1u);
  triax_expect_eq(p1, p2);

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
  tree_node*     stack[4];
  int            visited = 0;
  rbt_foreach(&r, stack, 4, e) {
    (void)e;
    ++visited;
  }
  triax_expect_eq(visited, 0);
  rbt_release(int, char, &r);
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
  tree_node* stack[8];
  int        keys[5];
  int        i = 0;
  rbt_foreach(&r, stack, 8, e) { keys[i++] = e->key; }
  triax_expect_eq(i, 5);
  for (int j = 1; j < 5; ++j) { triax_expect_true(keys[j] > keys[j - 1]); }

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
  triax_expect_eq(rbt_min(&r)->key, 1);
  triax_expect_eq(rbt_max(&r)->key, 32);

  tree_node* stack[10];
  int        prev = -1, count = 0;
  rbt_foreach(&r, stack, 10, e) {
    triax_expect_true(e->key > prev);
    prev = e->key;
    ++count;
  }
  triax_expect_eq(count, 32);

  rbt_release(int, char, &r);
}

RK__IGNWARN_CLANG_END()
RK_HEADER_END
#endif
