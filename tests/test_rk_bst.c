#ifndef TEST_BST_H
#define TEST_BST_H
#include "conf.h"

#define RK_IMPL
#include "../include/rklib_includeall.h"

RK_HEADER_BEGIN
RK__IGNWARN_CLANG_BEG("-Wunused-variable")

extern_fun int int_cmp2(int a, int b) { return a < b ? -1 : (a == b ? 0 : 1); }

BST_DEFINE(int, char, int_cmp2)

triax_test(bst, bsttest) {
  /* ------------------------------------------------------------------------ */
  /* init / count / empty / min / max on empty                                */
  /* ------------------------------------------------------------------------ */
  Bst(int, char) b = bst_init(int, char);

  triax_assert_true(bst_is_empty(&b));
  triax_assert_eq(bst_count(&b), 0u);
  triax_assert_true(bst_min(&b) == rk_null);
  triax_assert_true(bst_max(&b) == rk_null);
  triax_assert_true(bst_get(int, char, &b, 123) == rk_null);
  triax_assert_true(!bst_contains(int, char, &b, 123));

  {
    char out = 0;
    triax_assert_true(!bst_extract(int, char, &b, 123, &out));
  }

  /* ------------------------------------------------------------------------ */
  /* insert / search / contains / min / max                                   */
  /* ------------------------------------------------------------------------ */
  triax_assert_true(bst_set(int, char, &b, 3, 'c'));
  triax_assert_true(bst_set(int, char, &b, 1, 'a'));
  triax_assert_true(bst_set(int, char, &b, 4, 'd'));
  triax_assert_true(bst_set(int, char, &b, 2, 'b'));

  triax_assert_true(!bst_is_empty(&b));
  triax_assert_eq(bst_count(&b), 4u);

  {
    char* p1 = bst_get(int, char, &b, 1);
    char* p2 = bst_get(int, char, &b, 2);
    char* p3 = bst_get(int, char, &b, 3);
    char* p4 = bst_get(int, char, &b, 4);
    char* px = bst_get(int, char, &b, 99);

    triax_assert_true(p1 && *p1 == 'a');
    triax_assert_true(p2 && *p2 == 'b');
    triax_assert_true(p3 && *p3 == 'c');
    triax_assert_true(p4 && *p4 == 'd');
    triax_assert_true(px == rk_null);
  }

  triax_assert_true(bst_contains(int, char, &b, 1));
  triax_assert_true(bst_contains(int, char, &b, 2));
  triax_assert_true(bst_contains(int, char, &b, 3));
  triax_assert_true(bst_contains(int, char, &b, 4));
  triax_assert_true(!bst_contains(int, char, &b, 99));

  {
    BstEntry(int, char)* mn = bst_min(&b);
    BstEntry(int, char)* mx = bst_max(&b);

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
  triax_assert_true(!bst_add(int, char, &b, 2, 'X'));
  triax_assert_true(bst_get(int, char, &b, 2) != rk_null);
  triax_assert_eq(*bst_get(int, char, &b, 2), 'b');

  triax_assert_true(!bst_set(int, char, &b, 2, 'B'));
  triax_assert_eq(*bst_get(int, char, &b, 2), 'B');

  triax_assert_eq(bst_count(&b), 4u);

  /* ------------------------------------------------------------------------ */
  /* foreach: sorted in-order traversal                                       */
  /* ------------------------------------------------------------------------ */
  {
    tree_node* stack[4];
    int       keys[4] = {0};
    char      vals[4] = {0};
    int       i       = 0;

    bst_foreach(&b, stack, 4, e) {
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

    triax_assert_true(bst_extract(int, char, &b, 1, &out));
    triax_assert_eq(out, 'a');
    triax_assert_true(!bst_contains(int, char, &b, 1));
    triax_assert_eq(bst_count(&b), 3u);

    triax_assert_true(!bst_extract(int, char, &b, 1, &out));
  }

  /* ------------------------------------------------------------------------ */
  /* remove */
  /* ------------------------------------------------------------------------ */
  triax_assert_true(bst_contains(int, char, &b, 2));
  bst_remove(int, char, &b, 2);
  triax_assert_true(!bst_contains(int, char, &b, 2));
  triax_assert_eq(bst_count(&b), 2u);

  triax_assert_true(bst_contains(int, char, &b, 3));
  bst_remove(int, char, &b, 3);
  triax_assert_true(!bst_contains(int, char, &b, 3));
  triax_assert_eq(bst_count(&b), 1u);

  triax_assert_true(bst_contains(int, char, &b, 4));
  bst_remove(int, char, &b, 4);
  triax_assert_true(!bst_contains(int, char, &b, 4));
  triax_assert_eq(bst_count(&b), 0u);
  triax_assert_true(bst_is_empty(&b));

  triax_assert_true(bst_min(&b) == rk_null);
  triax_assert_true(bst_max(&b) == rk_null);

  /* removing missing key should be harmless */
  bst_remove(int, char, &b, 999);
  triax_assert_eq(bst_count(&b), 0u);

  /* ------------------------------------------------------------------------ */
  /* release on empty / reused tree */
  /* ------------------------------------------------------------------------ */
  bst_release(int, char, &b);
  triax_assert_true(bst_is_empty(&b));
  triax_assert_eq(bst_count(&b), 0u);
  triax_assert_true(bst_min(&b) == rk_null);
  triax_assert_true(bst_max(&b) == rk_null);

  /* tree should still be reusable after release */
  triax_assert_true(bst_set(int, char, &b, 42, 'x'));
  triax_assert_eq(bst_count(&b), 1u);
  triax_assert_true(bst_contains(int, char, &b, 42));
  triax_assert_eq(*bst_get(int, char, &b, 42), 'x');

  bst_release(int, char, &b);
  triax_assert_true(bst_is_empty(&b));
  triax_assert_eq(bst_count(&b), 0u);
}

triax_test(bst, init_static) {
  Bst(int, char) b = bst_init_static(int, char);
  triax_expect_true(bst_is_empty(&b));
  triax_expect_eq(bst_count(&b), 0u);
  triax_expect_null(bst_min(&b));
  triax_expect_null(bst_max(&b));

  triax_expect_true(bst_set(int, char, &b, 7, 'g'));
  triax_expect_eq(bst_count(&b), 1u);
  triax_expect_true(bst_contains(int, char, &b, 7));
  triax_expect_eq(*bst_get(int, char, &b, 7), 'g');

  bst_release(int, char, &b);
  triax_expect_true(bst_is_empty(&b));
}

triax_test(bst, get_or_add) {
  Bst(int, char) b = bst_init(int, char);

  bool inserted = false;
  char* p1      = bst_get_or_add(int, char, &b, 5, 'e', &inserted);
  triax_expect_true(inserted);
  triax_expect_eq(*p1, 'e');
  triax_expect_eq(bst_count(&b), 1u);

  // key already present: returns existing value, does not overwrite, does not insert
  inserted  = true;
  char* p2  = bst_get_or_add(int, char, &b, 5, 'z', &inserted);
  triax_expect_false(inserted);
  triax_expect_eq(*p2, 'e');
  triax_expect_eq(bst_count(&b), 1u);
  triax_expect_eq(p1, p2);

  // a second, distinct key still triggers a real insertion
  inserted     = false;
  char* p3     = bst_get_or_add(int, char, &b, 9, 'i', &inserted);
  triax_expect_true(inserted);
  triax_expect_eq(*p3, 'i');
  triax_expect_eq(bst_count(&b), 2u);

  triax_expect_eq(*bst_get(int, char, &b, 5), 'e');
  triax_expect_eq(*bst_get(int, char, &b, 9), 'i');

  bst_release(int, char, &b);
}

triax_test(bst, foreach_empty) {
  Bst(int, char) b = bst_init(int, char);
  tree_node* stack[4];
  int       visited = 0;
  bst_foreach(&b, stack, 4, e) {
    (void)e;
    ++visited;
  }
  triax_expect_eq(visited, 0);
  bst_release(int, char, &b);
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
  tree_node* stack[8];
  int       keys[5];
  int       i = 0;
  bst_foreach(&b, stack, 8, e) { keys[i++] = e->key; }
  triax_expect_eq(i, 5);
  for (int j = 1; j < 5; ++j) { triax_expect_true(keys[j] > keys[j - 1]); }

  bst_release(int, char, &b);
}

triax_test(bst, large_sorted_order) {
  Bst(int, char) b = bst_init(int, char);

  // Insert 1..32 in descending order (worst-case skewed tree)
  for (int i = 32; i >= 1; --i) { bst_set(int, char, &b, i, (char)('a' + (i - 1) % 26)); }
  triax_expect_eq(bst_count(&b), 32u);
  triax_expect_eq(bst_min(&b)->key, 1);
  triax_expect_eq(bst_max(&b)->key, 32);

  // In-order traversal must produce strictly ascending keys
  tree_node* stack[32];
  int       prev = -1, count = 0;
  bst_foreach(&b, stack, 32, e) {
    triax_expect_true(e->key > prev);
    prev = e->key;
    ++count;
  }
  triax_expect_eq(count, 32);

  bst_release(int, char, &b);
}

RK__IGNWARN_CLANG_END()
RK_HEADER_END
#endif
