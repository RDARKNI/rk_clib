#ifndef TEST_AVL_H
#define TEST_AVL_H
#include "conf.h"

#define RK_IMPL
#include "../include/rklib.h"

RK_HEADER_BEGIN
RK__IGNWARN_CLANG_BEG("-Wunused-variable")

extern_fun int avl_int_cmp(int a, int b) { return a < b ? -1 : (a == b ? 0 : 1); }

AVL_DEFINE(int, char, avl_int_cmp)

triax_test(avl, avltest) {
  /* ------------------------------------------------------------------------ */
  /* init / count / empty / min / max on empty                                */
  /* ------------------------------------------------------------------------ */
  Avl(int, char) a = avl_init(int, char);

  triax_assert_true(avl_is_empty(&a));
  triax_assert_eq(avl_count(&a), 0u);
  triax_assert_true(avl_min(&a) == rk_null);
  triax_assert_true(avl_max(&a) == rk_null);
  triax_assert_true(avl_get(int, char, &a, 123) == rk_null);
  triax_assert_true(!avl_contains(int, char, &a, 123));

  {
    char out = 0;
    triax_assert_true(!avl_extract(int, char, &a, 123, &out));
  }

  /* ------------------------------------------------------------------------ */
  /* insert / search / contains / min / max                                   */
  /* ------------------------------------------------------------------------ */
  triax_assert_true(avl_set(int, char, &a, 3, 'c'));
  triax_assert_true(avl_set(int, char, &a, 1, 'a'));
  triax_assert_true(avl_set(int, char, &a, 4, 'd'));
  triax_assert_true(avl_set(int, char, &a, 2, 'b'));

  triax_assert_true(!avl_is_empty(&a));
  triax_assert_eq(avl_count(&a), 4u);

  {
    char* p1 = avl_get(int, char, &a, 1);
    char* p2 = avl_get(int, char, &a, 2);
    char* p3 = avl_get(int, char, &a, 3);
    char* p4 = avl_get(int, char, &a, 4);
    char* px = avl_get(int, char, &a, 99);

    triax_assert_true(p1 && *p1 == 'a');
    triax_assert_true(p2 && *p2 == 'b');
    triax_assert_true(p3 && *p3 == 'c');
    triax_assert_true(p4 && *p4 == 'd');
    triax_assert_true(px == rk_null);
  }

  triax_assert_true(avl_contains(int, char, &a, 1));
  triax_assert_true(avl_contains(int, char, &a, 2));
  triax_assert_true(avl_contains(int, char, &a, 3));
  triax_assert_true(avl_contains(int, char, &a, 4));
  triax_assert_true(!avl_contains(int, char, &a, 99));

  {
    AvlEntry(int, char)* mn = avl_min(&a);
    AvlEntry(int, char)* mx = avl_max(&a);

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
  triax_assert_true(!avl_add(int, char, &a, 2, 'X'));
  triax_assert_true(avl_get(int, char, &a, 2) != rk_null);
  triax_assert_eq(*avl_get(int, char, &a, 2), 'b');

  triax_assert_true(!avl_set(int, char, &a, 2, 'B'));
  triax_assert_eq(*avl_get(int, char, &a, 2), 'B');

  triax_assert_eq(avl_count(&a), 4u);

  /* ------------------------------------------------------------------------ */
  /* foreach: sorted in-order traversal                                       */
  /* ------------------------------------------------------------------------ */
  {
    tree_node* stack[4];
    int        keys[4] = {0};
    char       vals[4] = {0};
    int        i       = 0;

    avl_foreach(&a, stack, 4, e) {
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

    triax_assert_true(avl_extract(int, char, &a, 1, &out));
    triax_assert_eq(out, 'a');
    triax_assert_true(!avl_contains(int, char, &a, 1));
    triax_assert_eq(avl_count(&a), 3u);

    triax_assert_true(!avl_extract(int, char, &a, 1, &out));
  }

  /* ------------------------------------------------------------------------ */
  /* remove */
  /* ------------------------------------------------------------------------ */
  triax_assert_true(avl_contains(int, char, &a, 2));
  avl_remove(int, char, &a, 2);
  triax_assert_true(!avl_contains(int, char, &a, 2));
  triax_assert_eq(avl_count(&a), 2u);

  triax_assert_true(avl_contains(int, char, &a, 3));
  avl_remove(int, char, &a, 3);
  triax_assert_true(!avl_contains(int, char, &a, 3));
  triax_assert_eq(avl_count(&a), 1u);

  triax_assert_true(avl_contains(int, char, &a, 4));
  avl_remove(int, char, &a, 4);
  triax_assert_true(!avl_contains(int, char, &a, 4));
  triax_assert_eq(avl_count(&a), 0u);
  triax_assert_true(avl_is_empty(&a));

  triax_assert_true(avl_min(&a) == rk_null);
  triax_assert_true(avl_max(&a) == rk_null);

  /* removing missing key should be harmless */
  avl_remove(int, char, &a, 999);
  triax_assert_eq(avl_count(&a), 0u);

  /* ------------------------------------------------------------------------ */
  /* release on empty / reused tree */
  /* ------------------------------------------------------------------------ */
  avl_release(int, char, &a);
  triax_assert_true(avl_is_empty(&a));
  triax_assert_eq(avl_count(&a), 0u);
  triax_assert_true(avl_min(&a) == rk_null);
  triax_assert_true(avl_max(&a) == rk_null);

  /* tree should still be reusable after release */
  triax_assert_true(avl_set(int, char, &a, 42, 'x'));
  triax_assert_eq(avl_count(&a), 1u);
  triax_assert_true(avl_contains(int, char, &a, 42));
  triax_assert_eq(*avl_get(int, char, &a, 42), 'x');

  avl_release(int, char, &a);
  triax_assert_true(avl_is_empty(&a));
  triax_assert_eq(avl_count(&a), 0u);
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

triax_test(avl, get_or_add) {
  Avl(int, char) a = avl_init(int, char);

  bool  inserted = false;
  char* p1       = avl_get_or_add(int, char, &a, 5, 'e', &inserted);
  triax_expect_true(inserted);
  triax_expect_eq(*p1, 'e');
  triax_expect_eq(avl_count(&a), 1u);

  // key already present: returns existing value, does not overwrite, does not insert
  inserted = true;
  char* p2 = avl_get_or_add(int, char, &a, 5, 'z', &inserted);
  triax_expect_false(inserted);
  triax_expect_eq(*p2, 'e');
  triax_expect_eq(avl_count(&a), 1u);
  triax_expect_eq(p1, p2);

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
  tree_node*     stack[4];
  int            visited = 0;
  avl_foreach(&a, stack, 4, e) {
    (void)e;
    ++visited;
  }
  triax_expect_eq(visited, 0);
  avl_release(int, char, &a);
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
  for (int j = 1; j < 5; ++j) { triax_expect_true(keys[j] > keys[j - 1]); }

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
    triax_expect_true(e->key > prev);
    prev = e->key;
    ++count;
  }
  triax_expect_eq(count, 32);

  avl_release(int, char, &a);
}

RK__IGNWARN_CLANG_END()
RK_HEADER_END
#endif
