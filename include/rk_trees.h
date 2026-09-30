// SPDX-License-Identifier: MIT
/// @file rk_trees.h
/// @version 1.0.0
/// @defgroup rk_trees Tree Interfaces (Bst, Avl, Rbt)
/// @brief Type-safe, generic binary search trees for C: a plain unbalanced `Bst`, a height-balanced
/// `Avl`, and a left-leaning red-black `Rbt`. All three share the same node-walking, release, and
/// iteration primitives (the `tree_*` names below) and expose the same shaped API (`_init`,
/// `_set`, `_add`, `_get`, `_get_or_add` for Bst, `_contains`, `_extract`, `_remove`, `_min`,
/// `_max`, `_release`, `_foreach`), differing only in their rebalancing strategy and therefore
/// their worst-case complexity.
///
/// - `Bst`: no rebalancing. O(log n) average, O(n) worst case (e.g. sorted insertion order).
/// - `Avl`: rotates to keep left/right subtree heights within 1 of each other. O(log n) worst case,
///   tighter balance than `Rbt` (faster lookups, slightly more rotations on insert/delete).
///   Prefer this when reads dominate writes.
/// - `Rbt`: rotates and recolors to keep the tree "balanced enough" (no root-to-leaf path more than
///   2x any other). O(log n) worst case, looser balance than `Avl` (fewer rotations on
///   insert/delete). Prefer this when writes are frequent.
///
/// Usage (identical shape across all three; substitute `bst`/`avl`/`rbt` and `Bst`/`Avl`/`Rbt`
/// throughout):
/// 1. Define a comparison function: `int cmp_f(K a, K b)` returning negative, zero, or positive
///    (like `strcmp`).
///
/// 2. Create typedefs for key and value types if needed (pointers and structs require typedefs due
///    to the C preprocessor):
///    ```c
///    typedef char* cstr;
///    ```
///
/// 3. Declare and instantiate a specialised tree:
///    ```c
///    BST_DEFINE(int, cstr, int_cmp);
///    Bst(int, cstr) tree = bst_init(int, cstr);
///    ```
///
/// 4. Insert, query, and remove elements:
///    ```c
///    bst_set(int, cstr, &tree, 42, "hello");
///    cstr* val = bst_get(int, cstr, &tree, 42);
///    bst_remove(int, cstr, &tree, 42);
///    ```
///
/// 5. Iterate in sorted order:
///    ```c
///    tree_node* stack[64];
///    bst_foreach(&tree, stack, 64, entry) {
///        printf("%d -> %s\n", entry->key, entry->val);
///    }
///    ```
///
/// 6. Free resources when done:
///    ```c
///    bst_release(int, cstr, &tree);
///    ```
///
/// @note `Bst` is unbalanced. For highly skewed insertion order, prefer `Avl` or `Rbt`.
/// @note None of the three are thread-safe.
/// @see rk_alloc.h
/// @see rk_dict.h
/// @{

#ifndef RK_TREES_H
#define RK_TREES_H
#include "rk_alloc.h"
RK_HEADER_BEGIN

/// @brief Type-erased node header shared by every tree type's concrete node (`RKI_BstNode`,
/// `RKI_AvlNode`, `RKI_RbtNode` all start with the same `l`/`r` layout). This is the type a caller
/// declares a traversal stack buffer as, e.g. `tree_node* stack[64];` for `bst_foreach()`.
/// @note Deliberately just the two pointers, with no `alignas_max`-forced over-alignment: the
/// shared primitives below only ever touch `l`/`r` through this type (entry data is reached via an
/// explicit byte offset into the real, per-(K,V) node -- see `rki_tree_min_off`/`rki_tree_max_off`
/// -- never via a member of `tree_node` itself). A concrete node's actual allocation is only ever
/// guaranteed to meet *its own* alignment (e.g. `alignof(RKI_AvlNode(K, V))`, which can be less
/// than `align_max`), so giving `tree_node` a stricter alignment than plain pointers would make
/// every `(tree_node*)` cast of such a node technically misaligned.
typedef struct tree_node { struct tree_node *l, *r; } tree_node;

/// @brief Type-erased tree header (allocator, count, root), aliased with each concrete tree
/// struct's own typed view. Not normally constructed directly.
typedef struct tree_data {
  RK_IFALLOC(Allocator alloc;)
  size_t     count;
  tree_node* root;
} tree_data;

/// @brief In-order traversal state used by `bst_foreach()`/`avl_foreach()`/`rbt_foreach()`. Not
/// normally constructed directly; the `_foreach` macros build one internally from the stack buffer
/// and capacity you pass in.
typedef struct tree_iter {
  tree_node **stack, *curr;
  size_t      cap, top;
} tree_iter;

/// @brief Frees all nodes in the tree and resets it to an empty state. Identical across
/// `Bst`/`Avl`/`Rbt` (also reachable as `bst_release`/`avl_release`/`rbt_release`) since it only
/// ever needs to walk `l`/`r` and deallocate -- no rebalancing-specific logic applies here.
#define tree_release(self)                                                                         \
  rki_tree_release(sizeof(*(self)->root), alignof(typeof(*(self)->root)), &(self)->_tree)

/// @brief `size_t tree_count(self)` - Returns the number of key-value pairs stored. Identical
/// across `Bst`/`Avl`/`Rbt` (also reachable as `bst_count`/`avl_count`/`rbt_count`); lookup and
/// mutation are the only operations that differ by rebalancing strategy and therefore stay
/// variant-prefixed.
#define tree_count(self)     ((size_t)(self)->count)

/// @brief `Allocator tree_allocator(self)` - Returns the Allocator the tree was constructed with,
/// or `alloc_ctx` if the tree was never initialized or custom allocators are disabled. Identical
/// across `Bst`/`Avl`/`Rbt` (also reachable as `bst_allocator`/`avl_allocator`/`rbt_allocator`).
#define tree_allocator(self) RKI_allocatorof(self)

/// @brief `bool tree_is_empty(self)` - Returns `true` iff the tree contains no elements.
#define tree_is_empty(self)  (tree_count(self) == 0)

/// @brief Returns a pointer to the entry with the smallest key, or `NULL` if the tree is empty.
/// Works identically for `Bst`/`Avl`/`Rbt` (also reachable as `bst_min`/`avl_min`/`rbt_min`): the
/// real entry offset is computed via `offsetof` rather than assumed, so it doesn't matter that
/// `Avl`/`Rbt` nodes carry extra bookkeeping (height/color) that `Bst` nodes don't.
#define tree_min(self)                                                                             \
  ((typeof((self)->root->entry)*)rki_tree_min_off((self)->_tree.root,                              \
                                                  offsetof(typeof(*(self)->root), entry)))

/// @brief Like `tree_min()`, but for the largest key.
#define tree_max(self)                                                                             \
  ((typeof((self)->root->entry)*)rki_tree_max_off((self)->_tree.root,                              \
                                                  offsetof(typeof(*(self)->root), entry)))

//////////////////////////////////// Bst: unbalanced BST //////////////////////////////////////////

/// @brief `BST_DEFINE(K, V, CMP_FUN)` - Generates a complete type-specific BST API for the given
/// key/value combination.
///
/// Must be invoked at file scope, once per `(K, V)` combination, before any use of the
/// corresponding `Bst(K, V)` type or its operations.
///
/// Generates:
/// - `BstEntry(K, V)` — public entry struct with `K const key` and `V val`
/// - `Bst(K, V)` — the tree struct holding the root pointer and element count
/// - Internal implementation functions for search, insert, remove, and release
///
/// @param K Key type (must be a plain identifier; use `typedef` for pointer or struct types)
/// @param V       Value type (same constraint as `K`)
/// @param CMP_FUN Comparison function with signature `int cmp(K a, K b)`. Must return negative if
/// `a < b`, zero if `a == b`, positive if `a > b` (same convention as `strcmp`).
///
/// Example:
/// ```c
/// typedef char* cstr;
/// int int_cmp(int a, int b) { return a - b; }
/// BST_DEFINE(int, cstr, int_cmp);
/// ```
#define BST_DEFINE(K, V, CMP_FUN)       RKI_BST_DEFINE(K, V, CMP_FUN)

/// @brief Generates a type-specific BST struct name.
#define Bst(K, V)                       Bst_##K##_##V
/// @brief Generates a type-specific BST entry struct name.
#define BstEntry(K, V)                  bst_entry_##K##_##V

/// @brief `Bst(K, V) bst_init(K, V, Allocator alloc = alloc_ctx)` - Initialises and returns an
/// empty BST.
/// @param K     Key type name
/// @param V     Value type name
/// @param alloc Optional allocator; defaults to `alloc_ctx`
/// @return An initialised, empty `Bst(K, V)`
#define bst_init(K, V, ...)             rk_overload(RKI_BST_INIT, K, V, ##__VA_ARGS__)

/// @brief `void bst_release(K, V, Bst(K, V)* self)` - Frees all nodes in the BST and resets it to
/// an empty state. Alias for `tree_release()`.
#define bst_release(K, V, self)         tree_release(self)

/// @brief `size_t bst_count(Bst(K, V)* self)` - Returns the number of key-value pairs stored in the
/// BST. Alias for `tree_count()`.
#define bst_count(self)                 tree_count(self)

/// @brief Alias for `tree_allocator()`.
#define bst_allocator(self)             tree_allocator(self)

/// @brief `bool bst_is_empty(Bst(K, V)* self)` - Returns `true` iff the BST contains no elements.
/// Alias for `tree_is_empty()`.
#define bst_is_empty(self)              tree_is_empty(self)

/// @brief `BstEntry(K, V)* bst_min(Bst(K, V)* self)` - Returns a pointer to the entry with the
/// smallest key, or `NULL` if the BST is empty. Alias for `tree_min()`.
#define bst_min(self)                   tree_min(self)

/// @brief `BstEntry(K, V)* bst_max(Bst(K, V)* self)` - Returns a pointer to the entry with the
/// largest key, or `NULL` if the BST is empty. Alias for `tree_max()`.
#define bst_max(self)                   tree_max(self)

/// @brief `V* bst_get(K, V, Bst(K, V)* self, K key)` - Looks up a key and returns a pointer to its
/// associated value, or `NULL` if not found.
/// @return Pointer to the value, or `NULL` if the key is absent
#define bst_get(K, V, self, key)        RKI_BST_PUB(K, V, get)(self, key)

/// @brief `bool bst_contains(K, V, Bst(K, V)* self, K key)` - Returns `true` iff the BST contains
/// an entry with the given key.
#define bst_contains(K, V, self, key)   RKI_BST_PUB(K, V, contains)(self, key)

/// @brief `bool bst_set(K, V, Bst(K, V)* self, K key, V value)` - Inserts or updates a key-value
/// pair. If `key` is already present, its value is overwritten. If not, a new node is allocated and
/// inserted.
/// @return `true` if a new node was inserted, `false` if an existing value was updated
#define bst_set(K, V, self, key, value) RKI_BST_PUB(K, V, set)(self, key, value)

/// @brief `V* bst_add(K, V, Bst(K, V)* self, K key, V value)` - Inserts a key-value pair only if
/// `key` is not already present. Existing values are not overwritten.
/// @return Pointer to the added object, if added, or `NULL`, if not
#define bst_add(K, V, self, key, value) RKI_BST_PUB(K, V, add)(self, key, value)

/// @brief `V* bst_get_or_add(K, V, Bst(K, V)* self, K key, V default_value, bool* inserted_out)` -
/// Returns a pointer to the value for `key`, inserting `default_value` first if the key is absent.
/// Performs a single tree traversal, unlike a separate `bst_get()`/`bst_add()` pair.
/// @param default_value Value to insert if `key` is not present.
/// @param inserted_out Optional output pointer. If non-`NULL`, set to `true` if a new node was
/// inserted and `false` if the key already existed.
/// @return Pointer to the value for `key` (never `NULL`).
#define bst_get_or_add(K, V, self, key, default_value, inserted_out)                               \
  RKI_BST_PUB(K, V, get_or_add)(self, key, default_value, inserted_out)

/// @brief `bool bst_extract(K, V, Bst(K, V)* self, K key, V* out)` - Removes the entry
/// with `key` from the BST and writes its value to `out`.
/// @param out Non-null pointer; where the removed value is written, if found
/// @return `true` if the key was found and removed, `false` otherwise
#define bst_extract(K, V, self, key, out) RKI_BST_PUB(K, V, extract)(self, key, out)

/// @brief `bool bst_remove(K, V, Bst(K, V)* self, K key)` - Removes the entry with `key` from the
/// BST, discarding its value.
/// @return `true` if the key was found and removed, `false` otherwise
#define bst_remove(K, V, self, key)       RKI_BST_PUB(K, V, remove)(self, key)

/// @brief Iterates over all entries in the BST in ascending key order.
///
/// Performs an in-order traversal using a caller-supplied stack buffer. The loop variable `entry`
/// is a `const BstEntry(K, V)*` pointing to each entry in turn.
///
/// @param self          Pointer to the `Bst(K, V)` to iterate
/// @param stack_buf     Array of `tree_node*` used as the traversal stack
/// @param stack_cap     Number of elements in `stack_buf`; must be at least the number of nodes on
/// the tree's longest root-to-leaf path (its height, counting nodes rather than edges) to avoid
/// writing past `stack_buf`. Checked via `rk_assert` in debug builds only; violating this in a
/// release build is undefined behaviour, not a caught error.
/// @param entry         Name for the loop variable (a `const BstEntry(K, V)*`)
///
/// @warning Do not insert or remove elements during iteration.
/// @warning If `stack_cap` is less than the tree height, an assertion fires.
///
/// Example:
/// ```c
/// tree_node* stack[64];
/// bst_foreach(&tree, stack, 64, e) {
///     printf("%d -> %s\n", e->key, e->val);
/// }
/// ```
#define bst_foreach(self, stack_buf, stack_cap, entry)                                             \
  tree_foreach(self, stack_buf, stack_cap, entry)

/////////////////////////////////////// Avl: AVL-balanced BST /////////////////////////////////////

/// @brief `AVL_DEFINE(K, V, CMP_FUN)` - Generates a complete type-specific AVL tree API for the
/// given key/value combination. See `BST_DEFINE()` for the shared usage pattern.
/// @param K Key type (must be a plain identifier; use `typedef` for pointer or struct types)
/// @param V       Value type (same constraint as `K`)
/// @param CMP_FUN Comparison function with signature `int cmp(K a, K b)`. Must return negative if
/// `a < b`, zero if `a == b`, positive if `a > b` (same convention as `strcmp`).
#define AVL_DEFINE(K, V, CMP_FUN)       RKI_AVL_DEFINE(K, V, CMP_FUN)

/// @brief Generates a type-specific Avl struct name.
#define Avl(K, V)                       Avl_##K##_##V
/// @brief Generates a type-specific Avl entry struct name.
#define AvlEntry(K, V)                  avl_entry_##K##_##V

/// @brief `Avl(K, V) avl_init(K, V, Allocator alloc = alloc_ctx)` - Initialises and returns an
/// empty Avl tree.
/// @param K     Key type name
/// @param V     Value type name
/// @param alloc Optional allocator; defaults to `alloc_ctx`
/// @return An initialised, empty `Avl(K, V)`
#define avl_init(K, V, ...)             rk_overload(RKI_AVL_INIT, K, V, ##__VA_ARGS__)

/// @brief `void avl_release(K, V, Avl(K, V)* self)` - Frees all nodes in the tree and resets it to
/// an empty state. Alias for `tree_release()`.
#define avl_release(K, V, self)         tree_release(self)

/// @brief `size_t avl_count(Avl(K, V)* self)` - Returns the number of key-value pairs stored.
/// Alias for `tree_count()`.
#define avl_count(self)                 tree_count(self)

/// @brief Alias for `tree_allocator()`.
#define avl_allocator(self)             tree_allocator(self)

/// @brief `bool avl_is_empty(Avl(K, V)* self)` - Returns `true` iff the tree contains no elements.
/// Alias for `tree_is_empty()`.
#define avl_is_empty(self)              tree_is_empty(self)

/// @brief `AvlEntry(K, V)* avl_min(Avl(K, V)* self)` - Returns a pointer to the entry with the
/// smallest key, or `NULL` if the tree is empty. Alias for `tree_min()`.
#define avl_min(self)                   tree_min(self)

/// @brief `AvlEntry(K, V)* avl_max(Avl(K, V)* self)` - Returns a pointer to the entry with the
/// largest key, or `NULL` if the tree is empty. Alias for `tree_max()`.
#define avl_max(self)                   tree_max(self)

/// @brief `V* avl_get(K, V, Avl(K, V)* self, K key)` - See `bst_get()`.
#define avl_get(K, V, self, key)        RKI_AVL_PUB(K, V, get)(self, key)

/// @brief `bool avl_contains(K, V, Avl(K, V)* self, K key)` - See `bst_contains()`.
#define avl_contains(K, V, self, key)   (!!avl_get(K, V, self, key))

/// @brief `bool avl_set(K, V, Avl(K, V)* self, K key, V value)` - Inserts or updates a key-value
/// pair, rebalancing as needed.
/// @return `true` if a new node was inserted, `false` if an existing value was updated
#define avl_set(K, V, self, key, value) RKI_AVL_PUB(K, V, set)(self, key, value)

/// @brief `V* avl_add(K, V, Avl(K, V)* self, K key, V value)` - See `bst_add()`; rebalances as
/// needed.
/// @return Pointer to the added value, if added, or `NULL`, if not
#define avl_add(K, V, self, key, value) RKI_AVL_PUB(K, V, add)(self, key, value)

/// @brief `V* avl_get_or_add(K, V, Avl(K, V)* self, K key, V default_value, bool* inserted_out)` -
/// See `bst_get_or_add()`; rebalances as needed.
/// @return Pointer to the value for `key` (never `NULL`).
#define avl_get_or_add(K, V, self, key, default_value, inserted_out)                               \
  RKI_AVL_PUB(K, V, get_or_add)(self, key, default_value, inserted_out)

/// @brief `bool avl_extract(K, V, Avl(K, V)* self, K key, V* out)` - See `bst_extract()`;
/// rebalances as needed.
/// @return `true` if the key was found and removed, `false` otherwise
#define avl_extract(K, V, self, key, out) RKI_AVL_PUB(K, V, extract)(self, key, out)

/// @brief `bool avl_remove(K, V, Avl(K, V)* self, K key)` - See `bst_remove()`; rebalances as
/// needed.
/// @return `true` if the key was found and removed, `false` otherwise
#define avl_remove(K, V, self, key)       RKI_AVL_PUB(K, V, remove)(self, key)

/// @brief Iterates over all entries in the Avl tree in ascending key order. Same parameters and
/// contract as `bst_foreach()`.
#define avl_foreach(self, stack_buf, stack_cap, entry)                                             \
  tree_foreach(self, stack_buf, stack_cap, entry)

////////////////////////////////// Rbt: left-leaning red-black tree ///////////////////////////////

/// @brief `RBT_DEFINE(K, V, CMP_FUN)` - Generates a complete type-specific left-leaning red-black
/// tree API for the given key/value combination. See `BST_DEFINE()` for the shared usage pattern.
/// @param K Key type (must be a plain identifier; use `typedef` for pointer or struct types)
/// @param V       Value type (same constraint as `K`)
/// @param CMP_FUN Comparison function with signature `int cmp(K a, K b)`. Must return negative if
/// `a < b`, zero if `a == b`, positive if `a > b` (same convention as `strcmp`).
#define RBT_DEFINE(K, V, CMP_FUN)       RKI_RBT_DEFINE(K, V, CMP_FUN)

/// @brief Generates a type-specific Rbt struct name.
#define Rbt(K, V)                       Rbt_##K##_##V
/// @brief Generates a type-specific Rbt entry struct name.
#define RbtEntry(K, V)                  rbt_entry_##K##_##V

/// @brief `Rbt(K, V) rbt_init(K, V, Allocator alloc = alloc_ctx)` - Initialises and returns an
/// empty Rbt tree.
#define rbt_init(K, V, ...)             rk_overload(RKI_RBT_INIT, K, V, ##__VA_ARGS__)

/// @brief `void rbt_release(K, V, Rbt(K, V)* self)` - Frees all nodes in the tree and resets it to
/// an empty state. Alias for `tree_release()`.
#define rbt_release(K, V, self)         tree_release(self)

/// @brief `size_t rbt_count(Rbt(K, V)* self)` - Returns the number of key-value pairs stored.
/// Alias for `tree_count()`.
#define rbt_count(self)                 tree_count(self)

/// @brief Alias for `tree_allocator()`.
#define rbt_allocator(self)             tree_allocator(self)

/// @brief `bool rbt_is_empty(Rbt(K, V)* self)` - Returns `true` iff the tree contains no elements.
/// Alias for `tree_is_empty()`.
#define rbt_is_empty(self)              tree_is_empty(self)

/// @brief `RbtEntry(K, V)* rbt_min(Rbt(K, V)* self)` - Returns a pointer to the entry with the
/// smallest key, or `NULL` if the tree is empty. Alias for `tree_min()`.
#define rbt_min(self)                   tree_min(self)

/// @brief `RbtEntry(K, V)* rbt_max(Rbt(K, V)* self)` - Returns a pointer to the entry with the
/// largest key, or `NULL` if the tree is empty. Alias for `tree_max()`.
#define rbt_max(self)                   tree_max(self)

/// @brief `V* rbt_get(K, V, Rbt(K, V)* self, K key)` - See `bst_get()`.
#define rbt_get(K, V, self, key)        RKI_RBT_PUB(K, V, get)(self, key)

/// @brief `bool rbt_contains(K, V, Rbt(K, V)* self, K key)` - See `bst_contains()`.
#define rbt_contains(K, V, self, key)   (!!rbt_get(K, V, self, key))

/// @brief `bool rbt_set(K, V, Rbt(K, V)* self, K key, V value)` - Inserts or updates a key-value
/// pair, rebalancing as needed.
/// @return `true` if a new node was inserted, `false` if an existing value was updated
#define rbt_set(K, V, self, key, value) RKI_RBT_PUB(K, V, set)(self, key, value)

/// @brief `V* rbt_add(K, V, Rbt(K, V)* self, K key, V value)` - See `bst_add()`; rebalances as
/// needed.
/// @return Pointer to the added value, if added, or `NULL`, if not
#define rbt_add(K, V, self, key, value) RKI_RBT_PUB(K, V, add)(self, key, value)

/// @brief `V* rbt_get_or_add(K, V, Rbt(K, V)* self, K key, V default_value, bool* inserted_out)` -
/// See `bst_get_or_add()`; rebalances as needed.
/// @return Pointer to the value for `key` (never `NULL`).
#define rbt_get_or_add(K, V, self, key, default_value, inserted_out)                               \
  RKI_RBT_PUB(K, V, get_or_add)(self, key, default_value, inserted_out)

/// @brief `bool rbt_extract(K, V, Rbt(K, V)* self, K key, V* out)` - See `bst_extract()`;
/// rebalances as needed.
/// @return `true` if the key was found and removed, `false` otherwise
#define rbt_extract(K, V, self, key, out) RKI_RBT_PUB(K, V, extract)(self, key, out)

/// @brief `bool rbt_remove(K, V, Rbt(K, V)* self, K key)` - See `bst_remove()`; rebalances as
/// needed.
/// @return `true` if the key was found and removed, `false` otherwise
#define rbt_remove(K, V, self, key)       RKI_RBT_PUB(K, V, remove)(self, key)

/// @brief Iterates over all entries in the Rbt tree in ascending key order. Same parameters and
/// contract as `bst_foreach()`.
#define rbt_foreach(self, stack_buf, stack_cap, entry)                                             \
  tree_foreach(self, stack_buf, stack_cap, entry)

#pragma region implementation
////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

////////////////////////////// Shared node/iterator primitives (`tree_*`) //////////////////////////

/// @brief Type-erased view of any of this file's concrete node types (`RKI_BstNode`,
/// `RKI_AvlNode`, `RKI_RbtNode`), used by the shared release/iteration primitives below, which only
/// ever need to walk `l`/`r` -- never touch `data`/`entry` directly (that's only ever done after
/// re-casting to the real, per-(K,V) node type, since the entry's offset differs by node type: it
/// sits right after `l`/`r` for `Bst`, but after an extra height/color bookkeeping field for
/// `Avl`/`Rbt`). Where the entry itself must be reached generically (`_min`/`_max`), the real
/// offset is passed explicitly rather than assumed -- see `rki_tree_min_off`/`rki_tree_max_off`.
rklib_fun void rki_tree_release_nodes(size_t nodesize, size_t nodealign,
                                      tree_node* restrict node RK_IFALLOC(, Allocator alloc)) {
  while (node) {
    if (node->l) {
      tree_node* left = node->l;
      node->l         = left->r;
      left->r         = node;
      node            = left;
    } else {
      tree_node* right = node->r;
      alloc_deallocate(node, nodesize, nodealign RK_IFALLOC(, alloc));
      node = right;
    }
  }
}

rklib_fun void rki_tree_release(size_t nodesize, size_t nodealign, tree_data* self) {
  rki_tree_release_nodes(nodesize, nodealign, self->root RK_IFALLOC(, self->alloc));
  self->root = rk_null, self->count = 0;
}

/// @brief Returns a pointer to the leftmost (minimum) node's entry, `entry_off` bytes into the
/// node. Passing the real offset (rather than assuming entry data sits right after `l`/`r`, as a
/// bare `tree_node*` would) is what makes this safe to reuse for node types that carry extra
/// bookkeeping (e.g. an AVL height or a red-black color bit) between the pointers and the entry.
rklib_fun rk_pure void* rki_tree_min_off(tree_node* node, size_t entry_off) {
  if (!node) { return rk_null; }
  while (node->l) { node = node->l; }
  return (char*)node + entry_off;
}

/// @brief Like `rki_tree_min_off()`, but for the rightmost (maximum) node.
rklib_fun rk_pure void* rki_tree_max_off(tree_node* node, size_t entry_off) {
  if (!node) { return rk_null; }
  while (node->r) { node = node->r; }
  return (char*)node + entry_off;
}

rklib_fun bool rki_tree_iter_next(tree_iter* restrict it, tree_node** node_out) {
  while (it->curr) {
    rk_assert(it->top < it->cap && "Tree iterator stack overflow");
    it->stack[it->top++] = it->curr;
    it->curr             = it->curr->l;
  }
  if (!it->top) { return false; }
  tree_node* node = it->stack[--it->top];
  *node_out       = node;
  it->curr        = node->r;
  return true;
}

/// @brief Shared in-order-traversal loop backing `bst_foreach`/`avl_foreach`/`rbt_foreach`. Not
/// normally used directly -- prefer the tree-specific macro, which documents its own parameters;
/// the shape is identical across all three.
#define tree_foreach(self, stack_buf, stack_cap, entry_)                                           \
  for (typeof(*(self))*const RKI_rs = (self), *RKI_once = RKI_rs; RKI_once;)                       \
    for (tree_node * RKI_node; RKI_once; RKI_once = 0)                                             \
      for (tree_iter RKI_it = {.stack = (stack_buf),                                               \
                               .curr  = (tree_node*)RKI_rs->root,                                  \
                               .cap   = (stack_cap),                                               \
                               .top   = 0};                                                        \
           rki_tree_iter_next(&RKI_it, &RKI_node);)                                                \
        for (typeof(RKI_rs->root->entry)*const entry_ = &((typeof(RKI_rs->root))RKI_node)->entry,  \
                                               *RKI_once1 = entry_;                                \
             RKI_once1; RKI_once1                         = 0)

//////////////////////////////////////////// Bst internals /////////////////////////////////////////

#define RKI_BstEntryPriv(K, V)      RKI_bst_entry_##K##_##V

#define RKI_BST_INIT(K, V, _Alloc)  ((Bst(K, V)){.count = 0, RK_IFALLOC(.alloc = _Alloc)})
#define RKI_BST_INIT3(K, V, _Alloc) RKI_REQUIRE_CUSTOM_ALLOCATORS(RKI_BST_INIT(K, V, _Alloc))
#define RKI_BST_INIT2(K, V)         RKI_BST_INIT(K, V, alloc_ctx)

#define RKI_BstNode(K, V)           RKI_bst_node_##K##_##V
#define RKI_BST_PUB(K, V, FNAME)    bst_##K##_##V##_##FNAME
#define RKI_BST_PRI(K, V, FNAME)    rki_bst_##K##_##V##_##FNAME

#define RKI_BST_DEFINE(K, V, CMP_FUN)                                                              \
  RK_EXTERNC_BEG                                                                                   \
  typedef struct BstEntry(K, V) {                                                                  \
    K const key;                                                                                   \
    V       val;                                                                                   \
  } BstEntry(K, V);                                                                                \
  typedef struct RKI_BstEntryPriv(K, V) {                                                          \
    K key;                                                                                         \
    V val;                                                                                         \
  } RKI_BstEntryPriv(K, V);                                                                        \
  struct RKI_BstNode(K, V) {                                                                       \
    struct RKI_BstNode(K, V) * l, *r;                                                              \
    union {                                                                                        \
      RKI_BstEntryPriv(K, V) entry_mod;                                                            \
      BstEntry(K, V) entry;                                                                        \
    };                                                                                             \
  };                                                                                               \
  typedef struct Bst(K, V) {                                                                       \
    union {                                                                                        \
      tree_data _tree;                                                                             \
      struct {                                                                                     \
        RK_IFALLOC(Allocator alloc;)                                                               \
        size_t count;                                                                              \
        struct RKI_BstNode(K, V) * root;                                                           \
      };                                                                                           \
    };                                                                                             \
  } Bst(K, V);                                                                                     \
  /* Real, typed functions purely for discoverability/direct use (IDE completion, taking their  */ \
  /* address, cross-container generic dispatch); bst_count()/bst_is_empty()/bst_allocator() */     \
  /* remain the untyped macros meant for everyday use. */                                          \
  rklib_fun rk_pure size_t RKI_BST_PUB(K, V, count)(const Bst(K, V) * self) {                      \
    return tree_count(self);                                                                       \
  }                                                                                                \
  rklib_fun rk_pure bool RKI_BST_PUB(K, V, is_empty)(const Bst(K, V) * self) {                     \
    return tree_is_empty(self);                                                                    \
  }                                                                                                \
  rklib_fun rk_pure Allocator RKI_BST_PUB(K, V, allocator)(const Bst(K, V) * self) {               \
    return tree_allocator(self);                                                                   \
  }                                                                                                \
  rklib_fun rk_pure struct RKI_BstNode(K, V)                                                       \
      * *RKI_BST_PRI(K, V, search_ptr)(Bst(K, V) * self, K key) {                                  \
    typedef struct RKI_BstNode(K, V) node_t;                                                       \
    node_t** curr = &self->root;                                                                   \
    for (; *curr;) {                                                                               \
      int cmp_res = CMP_FUN(key, (*curr)->entry.key);                                              \
      if (cmp_res == 0) { break; }                                                                 \
      curr = cmp_res < 0 ? &((*curr)->l) : &((*curr)->r);                                          \
    }                                                                                              \
    return curr;                                                                                   \
  }                                                                                                \
  rklib_fun rk_pure V* RKI_BST_PUB(K, V, get)(Bst(K, V) * self, K key) {                           \
    typedef struct RKI_BstNode(K, V) node_t;                                                       \
    node_t** node = RKI_BST_PRI(K, V, search_ptr)(self, key);                                      \
    return *node ? &((*node)->entry.val) : rk_null;                                                \
  }                                                                                                \
  rklib_fun rk_pure bool RKI_BST_PUB(K, V, contains)(Bst(K, V) * self, K key) {                    \
    return !!(*RKI_BST_PRI(K, V, search_ptr)(self, key));                                          \
  }                                                                                                \
  rklib_fun V* RKI_BST_PRI(K, V, set_add)(const bool always_insert, Bst(K, V) * self, K key,       \
                                          V val) {                                                 \
    typedef struct RKI_BstNode(K, V) node_t;                                                       \
    node_t** lnk = RKI_BST_PRI(K, V, search_ptr)(self, key);                                       \
    if (*lnk) {                                                                                    \
      if (always_insert) { (*lnk)->entry.val = val; }                                              \
      return rk_null;                                                                              \
    }                                                                                              \
    RKI_set_alloc_fallback(self->alloc);                                                           \
    node_t* n = alloc_new(node_t, 1 RK_IFALLOC(, self->alloc));                                    \
    n->r = n->l  = rk_null;                                                                        \
    n->entry_mod = (typeof(n->entry_mod)){.key = key, .val = val};                                 \
    *lnk         = n;                                                                              \
    ++self->count;                                                                                 \
    return &(n->entry_mod.val);                                                                    \
  }                                                                                                \
  rklib_fun bool RKI_BST_PUB(K, V, set)(Bst(K, V) * self, K key, V val) {                          \
    return !!RKI_BST_PRI(K, V, set_add)(true, self, key, val);                                     \
  }                                                                                                \
  rklib_fun V* RKI_BST_PUB(K, V, add)(Bst(K, V) * self, K key, V val) {                            \
    return RKI_BST_PRI(K, V, set_add)(false, self, key, val);                                      \
  }                                                                                                \
  rklib_fun V* RKI_BST_PUB(K, V, get_or_add)(Bst(K, V) * self, K key, V val,                       \
                                             bool* restrict inserted_out) {                        \
    typedef struct RKI_BstNode(K, V) node_t;                                                       \
    node_t** lnk = RKI_BST_PRI(K, V, search_ptr)(self, key);                                       \
    if (*lnk) {                                                                                    \
      if (inserted_out) { *inserted_out = false; }                                                 \
      return &((*lnk)->entry_mod.val);                                                             \
    }                                                                                              \
    RKI_set_alloc_fallback(self->alloc);                                                           \
    node_t* n = alloc_new(node_t, 1 RK_IFALLOC(, self->alloc));                                    \
    n->r = n->l  = rk_null;                                                                        \
    n->entry_mod = (typeof(n->entry_mod)){.key = key, .val = val};                                 \
    *lnk         = n;                                                                              \
    ++self->count;                                                                                 \
    if (inserted_out) { *inserted_out = true; }                                                    \
    return &(n->entry_mod.val);                                                                    \
  }                                                                                                \
  rklib_fun bool RKI_BST_PUB(K, V, extract)(Bst(K, V) * self, K key, V * val_out) {                \
    rk_assert_ptr_nonnull(val_out);                                                                \
    typedef struct RKI_BstNode(K, V) node_t;                                                       \
    node_t** lnk = RKI_BST_PRI(K, V, search_ptr)(self, key);                                       \
    if (!*lnk) { return false; }                                                                   \
    --self->count;                                                                                 \
    node_t* curr = *lnk;                                                                           \
    *val_out     = curr->entry.val;                                                                \
    if (curr->l && curr->r) {                                                                      \
      lnk = (node_t**)&curr->r;                                                                    \
      while ((*lnk)->l) { lnk = (node_t**)(&(*lnk)->l); }                                          \
      node_t* succ    = *lnk;                                                                      \
      curr->entry_mod = succ->entry_mod;                                                           \
      *lnk            = (node_t*)(succ->r);                                                        \
      alloc_delete(succ, 1 RK_IFALLOC(, self->alloc));                                             \
      return true;                                                                                 \
    }                                                                                              \
    *lnk = (node_t*)(curr->l ? curr->l : curr->r);                                                 \
    alloc_delete(curr, 1 RK_IFALLOC(, self->alloc));                                               \
    return true;                                                                                   \
  }                                                                                                \
  rklib_fun bool RKI_BST_PUB(K, V, remove)(Bst(K, V) * self, K key) {                              \
    V _;                                                                                           \
    return RKI_BST_PUB(K, V, extract)(self, key, &_);                                              \
  }                                                                                                \
  RK_EXTERNC_END

//////////////////////////////////////////// Avl internal /////////////////////////////////////////

#define RKI_AVL_INIT(K, V, A)    ((Avl(K, V)){.count = 0, .root = rk_null, RK_IFALLOC(.alloc = A)})
#define RKI_AVL_INIT3(K, V, A)   RKI_REQUIRE_CUSTOM_ALLOCATORS(RKI_AVL_INIT(K, V, A))
#define RKI_AVL_INIT2(K, V)      RKI_AVL_INIT(K, V, alloc_ctx)

#define RKI_AvlNode(K, V)        RKI_avl_node_##K##_##V
#define RKI_AVL_PUB(K, V, FNAME) avl_##K##_##V##_##FNAME
#define RKI_AVL_PRI(K, V, FNAME) rki_avl_##K##_##V##_##FNAME

#define RKI_AVL_DEFINE(K, V, CMP_FUN)                                                              \
  RK_EXTERNC_BEG                                                                                   \
  typedef struct AvlEntry(K, V) {                                                                  \
    K const key;                                                                                   \
    V       val;                                                                                   \
  } AvlEntry(K, V);                                                                                \
  typedef struct RKI_AVL_PRI(K, V, entry) {                                                        \
    K key;                                                                                         \
    V val;                                                                                         \
  } RKI_AVL_PRI(K, V, entry);                                                                      \
  typedef struct RKI_AvlNode(K, V) {                                                               \
    struct RKI_AvlNode(K, V) * l, *r;                                                              \
    int height;                                                                                    \
    union {                                                                                        \
      RKI_AVL_PRI(K, V, entry) entry_mod;                                                          \
      AvlEntry(K, V) entry;                                                                        \
    };                                                                                             \
  } RKI_AvlNode(K, V);                                                                             \
  typedef struct Avl(K, V) {                                                                       \
    union {                                                                                        \
      tree_data _tree;                                                                             \
      struct {                                                                                     \
        RK_IFALLOC(Allocator alloc;)                                                               \
        size_t count;                                                                              \
        RKI_AvlNode(K, V) * root;                                                                  \
      };                                                                                           \
    };                                                                                             \
  } Avl(K, V);                                                                                     \
  /* Real, typed functions purely for discoverability/direct use (IDE completion, taking their  */ \
  /* address, cross-container generic dispatch); avl_count()/avl_is_empty()/avl_allocator() */     \
  /* remain the untyped macros meant for everyday use. */                                          \
  rklib_fun rk_pure size_t RKI_AVL_PUB(K, V, count)(const Avl(K, V) * self) {                      \
    return tree_count(self);                                                                       \
  }                                                                                                \
  rklib_fun rk_pure bool RKI_AVL_PUB(K, V, is_empty)(const Avl(K, V) * self) {                     \
    return tree_is_empty(self);                                                                    \
  }                                                                                                \
  rklib_fun rk_pure Allocator RKI_AVL_PUB(K, V, allocator)(const Avl(K, V) * self) {               \
    return tree_allocator(self);                                                                   \
  }                                                                                                \
  rklib_fun rk_pure int RKI_AVL_PRI(K, V, h)(RKI_AvlNode(K, V) * n) { return n ? n->height : 0; }  \
  rklib_fun void        RKI_AVL_PRI(K, V, fixh)(RKI_AvlNode(K, V) * n) {                           \
    int a = RKI_AVL_PRI(K, V, h)(n->l), b = RKI_AVL_PRI(K, V, h)(n->r);                            \
    n->height = 1 + (a > b ? a : b);                                                               \
  }                                                                                                \
  rklib_fun RKI_AvlNode(K, V) * RKI_AVL_PRI(K, V, rotl)(RKI_AvlNode(K, V) * x) {                   \
    RKI_AvlNode(K, V)* y = x->r;                                                                   \
    x->r                 = y->l;                                                                   \
    y->l                 = x;                                                                      \
    RKI_AVL_PRI(K, V, fixh)(x);                                                                    \
    RKI_AVL_PRI(K, V, fixh)(y);                                                                    \
    return y;                                                                                      \
  }                                                                                                \
  rklib_fun RKI_AvlNode(K, V) * RKI_AVL_PRI(K, V, rotr)(RKI_AvlNode(K, V) * y) {                   \
    RKI_AvlNode(K, V)* x = y->l;                                                                   \
    y->l                 = x->r;                                                                   \
    x->r                 = y;                                                                      \
    RKI_AVL_PRI(K, V, fixh)(y);                                                                    \
    RKI_AVL_PRI(K, V, fixh)(x);                                                                    \
    return x;                                                                                      \
  }                                                                                                \
  rklib_fun RKI_AvlNode(K, V) * RKI_AVL_PRI(K, V, balance)(RKI_AvlNode(K, V) * n) {                \
    RKI_AVL_PRI(K, V, fixh)(n);                                                                    \
    int bf = RKI_AVL_PRI(K, V, h)(n->l) - RKI_AVL_PRI(K, V, h)(n->r);                              \
    if (bf > 1) {                                                                                  \
      if (RKI_AVL_PRI(K, V, h)(n->l->l) < RKI_AVL_PRI(K, V, h)(n->l->r)) {                         \
        n->l = RKI_AVL_PRI(K, V, rotl)(n->l);                                                      \
      }                                                                                            \
      return RKI_AVL_PRI(K, V, rotr)(n);                                                           \
    }                                                                                              \
    if (bf < -1) {                                                                                 \
      if (RKI_AVL_PRI(K, V, h)(n->r->r) < RKI_AVL_PRI(K, V, h)(n->r->l)) {                         \
        n->r = RKI_AVL_PRI(K, V, rotr)(n->r);                                                      \
      }                                                                                            \
      return RKI_AVL_PRI(K, V, rotl)(n);                                                           \
    }                                                                                              \
    return n;                                                                                      \
  }                                                                                                \
  rklib_fun RKI_AvlNode(K, V)                                                                      \
      * RKI_AVL_PRI(K, V, put)(Avl(K, V) * self, RKI_AvlNode(K, V) * n, K key, V val,              \
                               bool overwrite, V** out, bool* added) {                             \
    if (!n) {                                                                                      \
      RKI_set_alloc_fallback(self->alloc);                                                         \
      n    = alloc_new(RKI_AvlNode(K, V), 1 RK_IFALLOC(, self->alloc));                            \
      n->l = n->r  = rk_null;                                                                      \
      n->height    = 1;                                                                            \
      n->entry_mod = (typeof(n->entry_mod)){.key = key, .val = val};                               \
      *out         = &n->entry_mod.val;                                                            \
      *added       = true;                                                                         \
      return n;                                                                                    \
    }                                                                                              \
    int c = CMP_FUN(key, n->entry.key);                                                            \
    if (!c) {                                                                                      \
      if (overwrite) { n->entry_mod.val = val; }                                                   \
      *out = &n->entry_mod.val;                                                                    \
      return n;                                                                                    \
    }                                                                                              \
    if (c < 0) {                                                                                   \
      n->l = RKI_AVL_PRI(K, V, put)(self, n->l, key, val, overwrite, out, added);                  \
    } else {                                                                                       \
      n->r = RKI_AVL_PRI(K, V, put)(self, n->r, key, val, overwrite, out, added);                  \
    }                                                                                              \
    return RKI_AVL_PRI(K, V, balance)(n);                                                          \
  }                                                                                                \
  rklib_fun rk_pure V* RKI_AVL_PUB(K, V, get)(Avl(K, V) * self, K key) {                           \
    RKI_AvlNode(K, V)* n = self->root;                                                             \
    while (n) {                                                                                    \
      int c = CMP_FUN(key, n->entry.key);                                                          \
      if (!c) { return &n->entry_mod.val; }                                                        \
      n = c < 0 ? n->l : n->r;                                                                     \
    }                                                                                              \
    return rk_null;                                                                                \
  }                                                                                                \
  rklib_fun V* RKI_AVL_PRI(K, V, setadd)(Avl(K, V) * self, K key, V val, bool overwrite,           \
                                         bool* added) {                                            \
    V* out     = rk_null;                                                                          \
    *added     = false;                                                                            \
    self->root = RKI_AVL_PRI(K, V, put)(self, self->root, key, val, overwrite, &out, added);       \
    if (*added) { ++self->count; }                                                                 \
    return out;                                                                                    \
  }                                                                                                \
  rklib_fun bool RKI_AVL_PUB(K, V, set)(Avl(K, V) * self, K key, V val) {                          \
    bool added;                                                                                    \
    (void)RKI_AVL_PRI(K, V, setadd)(self, key, val, true, &added);                                 \
    return added;                                                                                  \
  }                                                                                                \
  rklib_fun V* RKI_AVL_PUB(K, V, add)(Avl(K, V) * self, K key, V val) {                            \
    bool added;                                                                                    \
    V*   p = RKI_AVL_PRI(K, V, setadd)(self, key, val, false, &added);                             \
    return added ? p : rk_null;                                                                    \
  }                                                                                                \
  rklib_fun V* RKI_AVL_PUB(K, V, get_or_add)(Avl(K, V) * self, K key, V val,                       \
                                             bool* restrict inserted_out) {                        \
    bool ignored;                                                                                  \
    return RKI_AVL_PRI(K, V, setadd)(self, key, val, false,                                        \
                                     inserted_out ? inserted_out : &ignored);                      \
  }                                                                                                \
  rklib_fun RKI_AvlNode(K, V)                                                                      \
      * RKI_AVL_PRI(K, V, detach_min)(RKI_AvlNode(K, V) * n, RKI_AvlNode(K, V) * *out) {           \
    if (!n->l) {                                                                                   \
      *out = n;                                                                                    \
      return n->r;                                                                                 \
    }                                                                                              \
    n->l = RKI_AVL_PRI(K, V, detach_min)(n->l, out);                                               \
    return RKI_AVL_PRI(K, V, balance)(n);                                                          \
  }                                                                                                \
  rklib_fun RKI_AvlNode(K, V)                                                                      \
      * RKI_AVL_PRI(K, V, erase)(Avl(K, V) * self, RKI_AvlNode(K, V) * n, K key, V * out,          \
                                 bool* removed) {                                                  \
    if (!n) return rk_null;                                                                        \
    int c = CMP_FUN(key, n->entry.key);                                                            \
    if (c < 0) {                                                                                   \
      n->l = RKI_AVL_PRI(K, V, erase)(self, n->l, key, out, removed);                              \
    } else if (c > 0) {                                                                            \
      n->r = RKI_AVL_PRI(K, V, erase)(self, n->r, key, out, removed);                              \
    } else {                                                                                       \
      *out                 = n->entry_mod.val;                                                     \
      *removed             = true;                                                                 \
      RKI_AvlNode(K, V)* l = n->l, *r = n->r;                                                      \
      if (!r) {                                                                                    \
        alloc_delete(n, 1 RK_IFALLOC(, self->alloc));                                              \
        return l;                                                                                  \
      }                                                                                            \
      RKI_AvlNode(K, V) * m;                                                                       \
      r    = RKI_AVL_PRI(K, V, detach_min)(r, &m);                                                 \
      m->l = l;                                                                                    \
      m->r = r;                                                                                    \
      alloc_delete(n, 1 RK_IFALLOC(, self->alloc));                                                \
      return RKI_AVL_PRI(K, V, balance)(m);                                                        \
    }                                                                                              \
    return *removed ? RKI_AVL_PRI(K, V, balance)(n) : n;                                           \
  }                                                                                                \
  rklib_fun bool RKI_AVL_PUB(K, V, extract)(Avl(K, V) * self, K key, V * out) {                    \
    rk_assert_ptr_nonnull(out);                                                                    \
    bool removed = false;                                                                          \
    self->root   = RKI_AVL_PRI(K, V, erase)(self, self->root, key, out, &removed);                 \
    if (removed) { --self->count; }                                                                \
    return removed;                                                                                \
  }                                                                                                \
  rklib_fun bool RKI_AVL_PUB(K, V, remove)(Avl(K, V) * self, K key) {                              \
    V tmp;                                                                                         \
    return RKI_AVL_PUB(K, V, extract)(self, key, &tmp);                                            \
  }                                                                                                \
  RK_EXTERNC_END

//////////////////////////////////////////// Rbt internals /////////////////////////////////////////

#define RKI_RBT_INIT(K, V, A)    ((Rbt(K, V)){.count = 0, .root = rk_null, RK_IFALLOC(.alloc = A)})
#define RKI_RBT_INIT3(K, V, A)   RKI_REQUIRE_CUSTOM_ALLOCATORS(RKI_RBT_INIT(K, V, A))
#define RKI_RBT_INIT2(K, V)      RKI_RBT_INIT(K, V, alloc_ctx)

/* Left-leaning red-black tree: red links lean left and no node has two red links in a row. */

#define RKI_RbtNode(K, V)        RKI_rbt_node_##K##_##V
#define RKI_RBT_PUB(K, V, FNAME) rbt_##K##_##V##_##FNAME
#define RKI_RBT_PRI(K, V, FNAME) rki_rbt_##K##_##V##_##FNAME

#define RKI_RBT_DEFINE(K, V, CMP)                                                                  \
  RK_EXTERNC_BEG                                                                                   \
  typedef struct RbtEntry(K, V) {                                                                  \
    K const key;                                                                                   \
    V       val;                                                                                   \
  } RbtEntry(K, V);                                                                                \
  typedef struct RKI_RBT_PRI(K, V, E) {                                                            \
    K key;                                                                                         \
    V val;                                                                                         \
  } RKI_RBT_PRI(K, V, E);                                                                          \
  typedef struct RKI_RbtNode(K, V) {                                                               \
    struct RKI_RbtNode(K, V) * l, *r;                                                              \
    bool red;                                                                                      \
    union {                                                                                        \
      RKI_RBT_PRI(K, V, E) entry_mod;                                                              \
      RbtEntry(K, V) entry;                                                                        \
    };                                                                                             \
  } RKI_RbtNode(K, V);                                                                             \
  typedef struct Rbt(K, V) {                                                                       \
    union {                                                                                        \
      tree_data _tree;                                                                             \
      struct {                                                                                     \
        RK_IFALLOC(Allocator alloc;)                                                               \
        size_t count;                                                                              \
        RKI_RbtNode(K, V) * root;                                                                  \
      };                                                                                           \
    };                                                                                             \
  } Rbt(K, V);                                                                                     \
  /* Real, typed functions purely for discoverability/direct use (IDE completion, taking their  */ \
  /* address, cross-container generic dispatch); rbt_count()/rbt_is_empty()/rbt_allocator() */     \
  /* remain the untyped macros meant for everyday use. */                                          \
  rklib_fun rk_pure size_t RKI_RBT_PUB(K, V, count)(const Rbt(K, V) * self) {                      \
    return tree_count(self);                                                                       \
  }                                                                                                \
  rklib_fun rk_pure bool RKI_RBT_PUB(K, V, is_empty)(const Rbt(K, V) * self) {                     \
    return tree_is_empty(self);                                                                    \
  }                                                                                                \
  rklib_fun rk_pure Allocator RKI_RBT_PUB(K, V, allocator)(const Rbt(K, V) * self) {               \
    return tree_allocator(self);                                                                   \
  }                                                                                                \
  rklib_fun rk_pure bool RKI_RBT_PRI(K, V, red)(RKI_RbtNode(K, V) * n) { return n && n->red; }     \
  rklib_fun              RKI_RbtNode(K, V) * RKI_RBT_PRI(K, V, rl)(RKI_RbtNode(K, V) * h) {        \
    RKI_RbtNode(K, V)* x = h->r;                                                                   \
    h->r                 = x->l;                                                                   \
    x->l                 = h;                                                                      \
    x->red               = h->red;                                                                 \
    h->red               = true;                                                                   \
    return x;                                                                                      \
  }                                                                                                \
  rklib_fun RKI_RbtNode(K, V) * RKI_RBT_PRI(K, V, rr)(RKI_RbtNode(K, V) * h) {                     \
    RKI_RbtNode(K, V)* x = h->l;                                                                   \
    h->l                 = x->r;                                                                   \
    x->r                 = h;                                                                      \
    x->red               = h->red;                                                                 \
    h->red               = true;                                                                   \
    return x;                                                                                      \
  }                                                                                                \
  rklib_fun void RKI_RBT_PRI(K, V, flip)(RKI_RbtNode(K, V) * h) {                                  \
    h->red    = !h->red;                                                                           \
    h->l->red = !h->l->red;                                                                        \
    h->r->red = !h->r->red;                                                                        \
  }                                                                                                \
  rklib_fun RKI_RbtNode(K, V) * RKI_RBT_PRI(K, V, fix)(RKI_RbtNode(K, V) * h) {                    \
    if (RKI_RBT_PRI(K, V, red)(h->r)) { h = RKI_RBT_PRI(K, V, rl)(h); }                            \
    if (RKI_RBT_PRI(K, V, red)(h->l) && RKI_RBT_PRI(K, V, red)(h->l->l)) {                         \
      h = RKI_RBT_PRI(K, V, rr)(h);                                                                \
    }                                                                                              \
    if (RKI_RBT_PRI(K, V, red)(h->l) && RKI_RBT_PRI(K, V, red)(h->r)) {                            \
      RKI_RBT_PRI(K, V, flip)(h);                                                                  \
    }                                                                                              \
    return h;                                                                                      \
  }                                                                                                \
  rklib_fun RKI_RbtNode(K, V) * RKI_RBT_PRI(K, V, ml)(RKI_RbtNode(K, V) * h) {                     \
    RKI_RBT_PRI(K, V, flip)(h);                                                                    \
    if (RKI_RBT_PRI(K, V, red)(h->r->l)) {                                                         \
      h->r = RKI_RBT_PRI(K, V, rr)(h->r);                                                          \
      h    = RKI_RBT_PRI(K, V, rl)(h);                                                             \
      RKI_RBT_PRI(K, V, flip)(h);                                                                  \
    }                                                                                              \
    return h;                                                                                      \
  }                                                                                                \
  rklib_fun RKI_RbtNode(K, V) * RKI_RBT_PRI(K, V, mr)(RKI_RbtNode(K, V) * h) {                     \
    RKI_RBT_PRI(K, V, flip)(h);                                                                    \
    if (RKI_RBT_PRI(K, V, red)(h->l->l)) {                                                         \
      h = RKI_RBT_PRI(K, V, rr)(h);                                                                \
      RKI_RBT_PRI(K, V, flip)(h);                                                                  \
    }                                                                                              \
    return h;                                                                                      \
  }                                                                                                \
  rklib_fun RKI_RbtNode(K, V)                                                                      \
      * RKI_RBT_PRI(K, V, put)(Rbt(K, V) * s, RKI_RbtNode(K, V) * h, K k, V v, bool ow, V** out,   \
                               bool* added) {                                                      \
    if (!h) {                                                                                      \
      RKI_set_alloc_fallback(s->alloc);                                                            \
      h    = alloc_new(RKI_RbtNode(K, V), 1 RK_IFALLOC(, s->alloc));                               \
      h->l = h->r  = rk_null;                                                                      \
      h->red       = true;                                                                         \
      h->entry_mod = (typeof(h->entry_mod)){.key = k, .val = v};                                   \
      *out         = &h->entry_mod.val;                                                            \
      *added       = true;                                                                         \
      return h;                                                                                    \
    }                                                                                              \
    int c = CMP(k, h->entry.key);                                                                  \
    if (c < 0) {                                                                                   \
      h->l = RKI_RBT_PRI(K, V, put)(s, h->l, k, v, ow, out, added);                                \
    } else if (c > 0) {                                                                            \
      h->r = RKI_RBT_PRI(K, V, put)(s, h->r, k, v, ow, out, added);                                \
    } else {                                                                                       \
      if (ow) { h->entry_mod.val = v; }                                                            \
      *out = &h->entry_mod.val;                                                                    \
    }                                                                                              \
    return RKI_RBT_PRI(K, V, fix)(h);                                                              \
  }                                                                                                \
  rklib_fun rk_pure V* RKI_RBT_PUB(K, V, get)(Rbt(K, V) * s, K k) {                                \
    RKI_RbtNode(K, V)* n = s->root;                                                                \
    while (n) {                                                                                    \
      int c = CMP(k, n->entry.key);                                                                \
      if (!c) { return &n->entry_mod.val; }                                                        \
      n = c < 0 ? n->l : n->r;                                                                     \
    }                                                                                              \
    return rk_null;                                                                                \
  }                                                                                                \
  rklib_fun V* RKI_RBT_PRI(K, V, insert)(Rbt(K, V) * s, K k, V v, bool ow, bool* added) {          \
    V* out       = rk_null;                                                                        \
    *added       = false;                                                                          \
    s->root      = RKI_RBT_PRI(K, V, put)(s, s->root, k, v, ow, &out, added);                      \
    s->root->red = false;                                                                          \
    if (*added) { ++s->count; }                                                                    \
    return out;                                                                                    \
  }                                                                                                \
  rklib_fun bool RKI_RBT_PUB(K, V, set)(Rbt(K, V) * s, K k, V v) {                                 \
    bool a;                                                                                        \
    (void)RKI_RBT_PRI(K, V, insert)(s, k, v, true, &a);                                            \
    return a;                                                                                      \
  }                                                                                                \
  rklib_fun V* RKI_RBT_PUB(K, V, add)(Rbt(K, V) * s, K k, V v) {                                   \
    bool a;                                                                                        \
    V*   p = RKI_RBT_PRI(K, V, insert)(s, k, v, false, &a);                                        \
    return a ? p : rk_null;                                                                        \
  }                                                                                                \
  rklib_fun V* RKI_RBT_PUB(K, V, get_or_add)(Rbt(K, V) * s, K k, V v,                              \
                                             bool* restrict inserted_out) {                        \
    bool ignored;                                                                                  \
    return RKI_RBT_PRI(K, V, insert)(s, k, v, false, inserted_out ? inserted_out : &ignored);      \
  }                                                                                                \
  rklib_fun rk_pure RKI_RbtNode(K, V) * RKI_RBT_PRI(K, V, mn)(RKI_RbtNode(K, V) * h) {             \
    while (h->l) { h = h->l; }                                                                     \
    return h;                                                                                      \
  }                                                                                                \
  rklib_fun RKI_RbtNode(K, V) * RKI_RBT_PRI(K, V, dm)(Rbt(K, V) * s, RKI_RbtNode(K, V) * h) {      \
    if (!h->l) {                                                                                   \
      alloc_delete(h, 1 RK_IFALLOC(, s->alloc));                                                   \
      return rk_null;                                                                              \
    }                                                                                              \
    if (!RKI_RBT_PRI(K, V, red)(h->l) && !RKI_RBT_PRI(K, V, red)(h->l->l)) {                       \
      h = RKI_RBT_PRI(K, V, ml)(h);                                                                \
    }                                                                                              \
    h->l = RKI_RBT_PRI(K, V, dm)(s, h->l);                                                         \
    return RKI_RBT_PRI(K, V, fix)(h);                                                              \
  }                                                                                                \
  rklib_fun RKI_RbtNode(K, V)                                                                      \
      * RKI_RBT_PRI(K, V, del)(Rbt(K, V) * s, RKI_RbtNode(K, V) * h, K k, V * out, bool* gone) {   \
    if (CMP(k, h->entry.key) < 0) {                                                                \
      if (h->l) {                                                                                  \
        if (!RKI_RBT_PRI(K, V, red)(h->l) && !RKI_RBT_PRI(K, V, red)(h->l->l)) {                   \
          h = RKI_RBT_PRI(K, V, ml)(h);                                                            \
        }                                                                                          \
        h->l = RKI_RBT_PRI(K, V, del)(s, h->l, k, out, gone);                                      \
      }                                                                                            \
    } else {                                                                                       \
      if (RKI_RBT_PRI(K, V, red)(h->l)) { h = RKI_RBT_PRI(K, V, rr)(h); }                          \
      int c = CMP(k, h->entry.key);                                                                \
      if (!c && !h->r) {                                                                           \
        *out  = h->entry_mod.val;                                                                  \
        *gone = true;                                                                              \
        alloc_delete(h, 1 RK_IFALLOC(, s->alloc));                                                 \
        return rk_null;                                                                            \
      }                                                                                            \
      if (h->r) {                                                                                  \
        if (!RKI_RBT_PRI(K, V, red)(h->r) && !RKI_RBT_PRI(K, V, red)(h->r->l)) {                   \
          h = RKI_RBT_PRI(K, V, mr)(h);                                                            \
        }                                                                                          \
        c = CMP(k, h->entry.key);                                                                  \
        if (!c) {                                                                                  \
          RKI_RbtNode(K, V)* m = RKI_RBT_PRI(K, V, mn)(h->r);                                      \
          *out                 = h->entry_mod.val;                                                 \
          *gone                = true;                                                             \
          h->entry_mod         = m->entry_mod;                                                     \
          h->r                 = RKI_RBT_PRI(K, V, dm)(s, h->r);                                   \
        } else {                                                                                   \
          h->r = RKI_RBT_PRI(K, V, del)(s, h->r, k, out, gone);                                    \
        }                                                                                          \
      }                                                                                            \
    }                                                                                              \
    return RKI_RBT_PRI(K, V, fix)(h);                                                              \
  }                                                                                                \
  rklib_fun bool RKI_RBT_PUB(K, V, extract)(Rbt(K, V) * s, K k, V * out) {                         \
    rk_assert_ptr_nonnull(out);                                                                    \
    if (!s->root || !RKI_RBT_PUB(K, V, get)(s, k)) { return false; }                               \
    bool gone = false;                                                                             \
    if (!RKI_RBT_PRI(K, V, red)(s->root->l) && !RKI_RBT_PRI(K, V, red)(s->root->r)) {              \
      s->root->red = true;                                                                         \
    }                                                                                              \
    s->root = RKI_RBT_PRI(K, V, del)(s, s->root, k, out, &gone);                                   \
    if (s->root) { s->root->red = false; }                                                         \
    if (gone) { --s->count; }                                                                      \
    return gone;                                                                                   \
  }                                                                                                \
  rklib_fun bool RKI_RBT_PUB(K, V, remove)(Rbt(K, V) * s, K k) {                                   \
    V x;                                                                                           \
    return RKI_RBT_PUB(K, V, extract)(s, k, &x);                                                   \
  }                                                                                                \
  RK_EXTERNC_END

/// @endcond
#pragma endregion implementation
RK_HEADER_END
/// @}
#endif // RK_TREES_H

// MIT License
//
// Copyright (c) 2026 Dariusch Knigge
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
