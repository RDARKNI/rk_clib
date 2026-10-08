// SPDX-License-Identifier: MIT
/// @file rk_trees.h
/// @version 1.0.0
/// @defgroup rk_trees Tree Interfaces (Bst, Avl, Rbt)
/// @brief Type-safe, generic binary search trees for C: a plain unbalanced `Bst`, a height-balanced
/// `Avl`, and a left-leaning red-black `Rbt`. All three share the same node-walking, release, and
/// iteration primitives (the `tree_*` names below) and expose the same shaped API (`_init`,
/// `_set`, `_add`, `_get`, `_get_or_add`, `_contains`, `_extract`, `_remove`, `_min`,
/// `_max`, `_release`, `_foreach`, `_foreach_reversed`, `_erase_if`), differing only in their
/// rebalancing strategy and therefore their worst-case complexity.
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
///    TreeNode* stack[64];
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
RKI_HEADER_BEGIN

/// @brief Shared node header embedded as the first member of every concrete tree node.
/// A caller uses `TreeNode* stack[64]` as traversal workspace. The links point to actual
/// embedded headers; concrete algorithms convert their values back to concrete node pointers.
/// @note Keep this header first in every node. No additional allocation or over-alignment is used.
/// Concrete C++ node types must be standard-layout; the instantiation macros enforce this.
/// The shared state is an actual `base` member: access fields as `tree.base.root`,
/// `tree.base.count`, and (with custom allocators) `tree.base.alloc`.
/// Tree roots store TreeNode* values. Convert a root to the concrete node type before accessing
/// its entry. A tree's rki_node_type member is type metadata for sizeof/typeof only and must
/// never be read or written at runtime.
typedef struct TreeNode { struct TreeNode *l, *r; } TreeNode;

/// @brief Frees all nodes in the tree and resets it to an empty state. Identical across
/// `Bst`/`Avl`/`Rbt` (also reachable as `bst_release`/`avl_release`/`rbt_release`) since it only
/// ever needs to walk `l`/`r` and deallocate -- no rebalancing-specific logic applies here.
/// @note `self` is evaluated once. Preserves the stored allocator.
#define tree_release(self)                                                                         \
  rki_tree_release(sizeof(*(self)->rki_node_type), alignof(typeof(*(self)->rki_node_type)),        \
                   &(self)->base)

/// @brief `size_t tree_count(self)` - Returns the number of key-value pairs stored. Identical
/// across `Bst`/`Avl`/`Rbt` (also reachable as `bst_count`/`avl_count`/`rbt_count`); lookup and
/// mutation are the only operations that differ by rebalancing strategy and therefore stay
/// variant-prefixed.
#define tree_count(self)     ((size_t)(self)->base.count)

/// @brief `Allocator tree_allocator(self)` - Returns the Allocator the tree was constructed with,
/// or `alloc_ctx` if the tree was never initialized or custom allocators are disabled. Identical
/// across `Bst`/`Avl`/`Rbt` (also reachable as `bst_allocator`/`avl_allocator`/`rbt_allocator`).
#define tree_allocator(self) RKI_allocatorof(&(self)->base)

/// @brief `bool tree_is_empty(self)` - Returns `true` iff the tree contains no elements.
#define tree_is_empty(self)  (tree_count(self) == 0)

/// @brief Returns the entry with the smallest key as an lvalue: mutable for a mutable tree, const
/// for a const tree. The entry's `key` is always const; its `val` is writable through a mutable
/// tree.
/// @param self Pointer to a `Bst`, `Avl`, or `Rbt`. Evaluated more than once.
/// @pre The tree is nonempty; use `tree_peek_min()` to check safely.
/// @note The precondition is asserted in debug builds; invalid access is undefined in release.
/// @note Works identically for `Bst`/`Avl`/`Rbt` (also reachable as `bst_min`/`avl_min`/`rbt_min`):
/// the real entry offset is computed via `offsetof` rather than assumed, so it doesn't matter that
/// `Avl`/`Rbt` nodes carry extra bookkeeping (height/color) that `Bst` nodes don't.
/// @note For Bst/Rbt, removal or extraction can invalidate references to other entries too:
/// deletion may copy a successor's entry and free its node. For Avl, only references to the
/// removed entry are invalidated. Releasing any tree invalidates all its entries. O(height).
/// @see tree_peek_min
#define tree_min(self)                                                                             \
  (*(typeof(RKI_TREE_ENTRY_PTR(self)))rki_tree_check_min_off(                                      \
      (self)->base.root, offsetof(typeof(*(self)->rki_node_type), entry)))

/// @brief Like `tree_min()`, but for the largest key.
/// @pre The tree is nonempty; use `tree_peek_max()` to check safely.
/// @see tree_peek_max
#define tree_max(self)                                                                             \
  (*(typeof(RKI_TREE_ENTRY_PTR(self)))rki_tree_check_max_off(                                      \
      (self)->base.root, offsetof(typeof(*(self)->rki_node_type), entry)))

/// @brief Returns a pointer to the entry with the smallest key, or `NULL` if the tree is empty.
/// @param self Pointer to a `Bst`, `Avl`, or `Rbt`. Evaluated more than once.
/// @return `Entry*` for a mutable tree, `const Entry*` for a const tree.
/// @note Inherits the reference invalidation rules of `tree_min()`. O(height).
/// @see tree_min
#define tree_peek_min(self)                                                                        \
  ((typeof(RKI_TREE_ENTRY_PTR(self)))rki_tree_min_off(                                             \
      (self)->base.root, offsetof(typeof(*(self)->rki_node_type), entry)))

/// @brief Like `tree_peek_min()`, but for the largest key.
/// @see tree_max
#define tree_peek_max(self)                                                                        \
  ((typeof(RKI_TREE_ENTRY_PTR(self)))rki_tree_max_off(                                             \
      (self)->base.root, offsetof(typeof(*(self)->rki_node_type), entry)))

/// @brief Iterates over all entries in ascending key order.
///
/// Works with Bst, Avl, and Rbt. Performs an in-order traversal using a caller-supplied stack,
/// without allocating memory.
///
/// The loop variable is a pointer to the tree's corresponding entry type: `BstEntry(K, V)*`,
/// `AvlEntry(K, V)*`, or `RbtEntry(K, V)*`.
/// Entries are const when traversing a pointer to a const tree.
///
/// @param self      Pointer to the tree. If NULL, the loop body is not executed.
/// @param stack_buf Writable array of `TreeNode*` used as traversal workspace.
/// @param stack_cap Number of pointer slots available in `stack_buf`. Must be at least the tree
///                  height, measured in nodes.
/// @param entry     Name of the entry-pointer variable declared by the macro.
///
/// @note `self` is evaluated once. `stack_buf` and `stack_cap` are evaluated once if `self` is
///       non-NULL, and are not evaluated otherwise.
/// @note `break` stops traversal; `continue` advances to the next entry.
/// @note Do not insert, remove, release, or otherwise restructure the tree
///       during traversal. Entry keys are read-only; values may be modified
///       where their type permits it.
/// @note Each concurrent or nested traversal requires its own stack buffer.
/// @note A complete traversal takes O(n) time and uses O(h) stack slots,
///       where n is the entry count and h is the tree height.
///
/// @warning Insufficient stack capacity is checked by `rk_assert` only.
///          If assertions are disabled, exceeding the buffer capacity causes
///          undefined behaviour.
///
/// Example:
/// ```c
/// // Assumes the tree height is at most 64 nodes.
/// TreeNode* stack[64];
/// tree_foreach(&tree, stack, 64, entry) {
///     printf("%d -> %s\n", entry->key, entry->val);
/// }
/// ```
///
/// @see tree_foreach_reversed
#define tree_foreach(self, stack_buf, stack_cap, entry_)                                           \
  RKI_TREE_FOREACH(self, stack_buf, stack_cap, entry_)

/// @brief Like `tree_foreach()`, but iterates in descending key order.
#define tree_foreach_reversed(self, stack_buf, stack_cap, entry_)                                  \
  RKI_TREE_FOREACH_REVERSED(self, stack_buf, stack_cap, entry_)

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
#define bst_init(K, V, ...)             RKI_OVERLOAD(RKI_BST_INIT, K, V, ##__VA_ARGS__)

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

/// @brief `BstEntry(K, V) bst_min(Bst(K, V)* self)` - Returns the entry with the smallest key as an
/// lvalue (const for a const Bst). Asserts the BST is nonempty. Alias for `tree_min()`.
#define bst_min(self)                   tree_min(self)

/// @brief `BstEntry(K, V) bst_max(Bst(K, V)* self)` - Returns the entry with the largest key as an
/// lvalue (const for a const Bst). Asserts the BST is nonempty. Alias for `tree_max()`.
#define bst_max(self)                   tree_max(self)

/// @brief `BstEntry(K, V)* bst_peek_min(Bst(K, V)* self)` - Returns a pointer to the entry with
/// the smallest key, or `NULL` if the BST is empty. Alias for `tree_peek_min()`.
#define bst_peek_min(self)              tree_peek_min(self)

/// @brief `BstEntry(K, V)* bst_peek_max(Bst(K, V)* self)` - Returns a pointer to the entry with
/// the largest key, or `NULL` if the BST is empty. Alias for `tree_peek_max()`.
#define bst_peek_max(self)              tree_peek_max(self)

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
/// @param stack_buf     Array of `TreeNode*` used as the traversal stack
/// @param stack_cap     Number of elements in `stack_buf`; must be at least the number of nodes on
/// the tree's longest root-to-leaf path (its height, counting nodes rather than edges) to avoid
/// writing past `stack_buf`. Checked via `rk_assert` in debug builds only; violating this in a
/// release build is undefined behaviour, not a caught error.
/// @param entry         Name for the loop variable (a `const BstEntry(K, V)*`)
///
/// @note break stops traversal; continue advances to the next entry.
/// @note Do not insert or remove elements during iteration.
/// @warning If `stack_cap` is less than the tree height, an assertion fires.
///
/// Example:
/// ```c
/// TreeNode* stack[64];
/// bst_foreach(&tree, stack, 64, e) {
///     printf("%d -> %s\n", e->key, e->val);
/// }
/// ```
#define bst_foreach(self, stack_buf, stack_cap, entry)                                             \
  tree_foreach(self, stack_buf, stack_cap, entry)

/// @brief Like `bst_foreach()`, but iterates in descending key order. Same parameters and contract.
#define bst_foreach_reversed(self, stack_buf, stack_cap, entry)                                    \
  tree_foreach_reversed(self, stack_buf, stack_cap, entry)

/// @brief Erases every entry satisfying `pred`.
///
/// Performs an in-order traversal using the given stack buffer (same contract as `bst_foreach()`)
/// to find every entry satisfying `pred`, then removes each one by key.
/// @param K,V             Key/value types, as passed to `BST_DEFINE()`
/// @param self            Pointer to a mutable `Bst(K, V)`
/// @param stack_buf       Array of `TreeNode*` used as the traversal stack (see `bst_foreach()`)
/// @param stack_cap       Number of elements in `stack_buf`
/// @param entry           Name for the loop variable (a `const BstEntry(K, V)*`)
/// @param pred            Predicate expression, evaluated once per entry present at the start of
/// the call
/// @note The predicate must not insert or remove entries.
///
/// Usage:
/// ```c
/// TreeNode* stack[64];
/// bst_erase_if(int, cstr, &tree, stack, 64, e, e->val[0] == 'x');
/// ```
#define bst_erase_if(K, V, self, stack_buf, stack_cap, entry, pred)                                \
  RKI_TREE_ERASE_IF(self, stack_buf, stack_cap, entry, pred, RKI_BST_PUB(K, V, remove))

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
#define avl_init(K, V, ...)             RKI_OVERLOAD(RKI_AVL_INIT, K, V, ##__VA_ARGS__)

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

/// @brief `AvlEntry(K, V) avl_min(Avl(K, V)* self)` - Returns the entry with the smallest key as an
/// lvalue (const for a const Avl). Asserts the tree is nonempty. Alias for `tree_min()`.
#define avl_min(self)                   tree_min(self)

/// @brief `AvlEntry(K, V) avl_max(Avl(K, V)* self)` - Returns the entry with the largest key as an
/// lvalue (const for a const Avl). Asserts the tree is nonempty. Alias for `tree_max()`.
#define avl_max(self)                   tree_max(self)

/// @brief `AvlEntry(K, V)* avl_peek_min(Avl(K, V)* self)` - Returns a pointer to the entry with
/// the smallest key, or `NULL` if the tree is empty. Alias for `tree_peek_min()`.
#define avl_peek_min(self)              tree_peek_min(self)

/// @brief `AvlEntry(K, V)* avl_peek_max(Avl(K, V)* self)` - Returns a pointer to the entry with
/// the largest key, or `NULL` if the tree is empty. Alias for `tree_peek_max()`.
#define avl_peek_max(self)              tree_peek_max(self)

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

/// @brief Like `avl_foreach()`, but iterates in descending key order. Same parameters and contract.
#define avl_foreach_reversed(self, stack_buf, stack_cap, entry)                                    \
  tree_foreach_reversed(self, stack_buf, stack_cap, entry)

/// @brief Erases every entry satisfying `pred`, rebalancing as needed. See `bst_erase_if()`.
#define avl_erase_if(K, V, self, stack_buf, stack_cap, entry, pred)                                \
  RKI_TREE_ERASE_IF(self, stack_buf, stack_cap, entry, pred, RKI_AVL_PUB(K, V, remove))

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
#define rbt_init(K, V, ...)             RKI_OVERLOAD(RKI_RBT_INIT, K, V, ##__VA_ARGS__)

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

/// @brief `RbtEntry(K, V) rbt_min(Rbt(K, V)* self)` - Returns the entry with the smallest key as an
/// lvalue (const for a const Rbt). Asserts the tree is nonempty. Alias for `tree_min()`.
#define rbt_min(self)                   tree_min(self)

/// @brief `RbtEntry(K, V) rbt_max(Rbt(K, V)* self)` - Returns the entry with the largest key as an
/// lvalue (const for a const Rbt). Asserts the tree is nonempty. Alias for `tree_max()`.
#define rbt_max(self)                   tree_max(self)

/// @brief `RbtEntry(K, V)* rbt_peek_min(Rbt(K, V)* self)` - Returns a pointer to the entry with
/// the smallest key, or `NULL` if the tree is empty. Alias for `tree_peek_min()`.
#define rbt_peek_min(self)              tree_peek_min(self)

/// @brief `RbtEntry(K, V)* rbt_peek_max(Rbt(K, V)* self)` - Returns a pointer to the entry with
/// the largest key, or `NULL` if the tree is empty. Alias for `tree_peek_max()`.
#define rbt_peek_max(self)              tree_peek_max(self)

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

/// @brief Like `rbt_foreach()`, but iterates in descending key order. Same parameters and contract.
#define rbt_foreach_reversed(self, stack_buf, stack_cap, entry)                                    \
  tree_foreach_reversed(self, stack_buf, stack_cap, entry)

/// @brief Erases every entry satisfying `pred`, rebalancing as needed. See `bst_erase_if()`.
#define rbt_erase_if(K, V, self, stack_buf, stack_cap, entry, pred)                                \
  RKI_TREE_ERASE_IF(self, stack_buf, stack_cap, entry, pred, RKI_RBT_PUB(K, V, remove))

#pragma region implementation
////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL
// Internal: actual shared state embedded in every concrete tree as `base`.
typedef struct RKI_TreeData {
  RK_IFALLOC(Allocator alloc;)
  size_t    count;
  TreeNode* root;
} RKI_TreeData;

// Internal: in-order traversal state used by `bst_foreach()`/`avl_foreach()`/`rbt_foreach()`. The
// `_foreach` macros build one internally from the stack buffer and capacity you pass in.
typedef struct RKI_TreeIter {
  TreeNode **stack, *curr;
  size_t     cap, top;
} RKI_TreeIter;

////////////////////////////// Shared node/iterator primitives (`tree_*`) //////////////////////////

/// @brief Shared release primitive operating on the embedded TreeNode headers.
/// Every header is at offset zero, so its address is also the allocation's start address.
rklib_fun void rki_tree_release_nodes(size_t nodesize, size_t nodealign,
                                      TreeNode* restrict node RK_IFALLOC(, Allocator alloc)) {
  while (node) {
    if (node->l) {
      TreeNode* left = node->l;
      node->l        = left->r;
      left->r        = node;
      node           = left;
    } else {
      TreeNode* right = node->r;
      alloc_deallocate(node, nodesize, nodealign RK_IFALLOC(, alloc));
      node = right;
    }
  }
}

// Operates on the actual embedded base object; no state overlay or field offsets.
rklib_fun void rki_tree_release(size_t nodesize, size_t nodealign, RKI_TreeData* self) {
  rki_tree_release_nodes(nodesize, nodealign, self->root RK_IFALLOC(, self->alloc));
  self->root  = rk_null;
  self->count = 0;
}

/// @brief Returns a pointer to the leftmost (minimum) node's entry, `entry_off` bytes into the
/// node. Passing the real offset (rather than assuming entry data sits right after `l`/`r`, as a
/// bare `TreeNode*` would) is what makes this safe to reuse for node types that carry extra
/// bookkeeping (e.g. an AVL height or a red-black color bit) between the pointers and the entry.
rklib_fun rk_pure void* rki_tree_min_off(TreeNode* node, size_t entry_off) {
  if (!node) { return rk_null; }
  while (node->l) { node = node->l; }
  return (char*)node + entry_off;
}

/// @brief Like `rki_tree_min_off()`, but for the rightmost (maximum) node.
rklib_fun rk_pure void* rki_tree_max_off(TreeNode* node, size_t entry_off) {
  if (!node) { return rk_null; }
  while (node->r) { node = node->r; }
  return (char*)node + entry_off;
}

// Not rk_pure: the asserts are side effects, and a pure call whose result is discarded (e.g.
// `(void)tree_min(t)`) may be removed entirely, silently skipping the emptiness check.
rklib_fun void* rki_tree_check_min_off(TreeNode* node, size_t entry_off) {
  rk_assert(node && "Cannot access min of empty tree");
  return rki_tree_min_off(node, entry_off);
}

rklib_fun void* rki_tree_check_max_off(TreeNode* node, size_t entry_off) {
  rk_assert(node && "Cannot access max of empty tree");
  return rki_tree_max_off(node, entry_off);
}

// Entry pointer type matching the tree's constness. Constness is detected through `count`, a
// member of a named type, as for the Dict/Deque iteration pointers (avoids `const const`, C4114).
#define RKI_TREE_ENTRY_PTR(self)                                                                   \
  _Generic(&(self)->base.count,                                                                    \
      const size_t*: (const typeof((self)->rki_node_type->entry)*)0,                               \
      default: (typeof((self)->rki_node_type->entry)*)0)

rklib_fun TreeNode* rki_tree_iter_next(RKI_TreeIter* restrict it) {
  while (it->curr) {
    rk_assert(it->top < it->cap && "Tree iterator stack overflow");
    it->stack[it->top++] = it->curr;
    it->curr             = it->curr->l;
  }
  if (!it->top) { return rk_null; }
  TreeNode* node = it->stack[--it->top];
  it->curr       = node->r;
  return node;
}

/// @brief Like `rki_tree_iter_next()`, but walks the tree right-to-left (descending order).
rklib_fun TreeNode* rki_tree_iter_next_reversed(RKI_TreeIter* restrict it) {
  while (it->curr) {
    rk_assert(it->top < it->cap && "Tree iterator stack overflow");
    it->stack[it->top++] = it->curr;
    it->curr             = it->curr->r;
  }
  if (!it->top) { return rk_null; }
  TreeNode* node = it->stack[--it->top];
  it->curr       = node->l;
  return node;
}

/// @brief Shared in-order-traversal loop backing `bst_foreach`/`avl_foreach`/`rbt_foreach` and
/// their
/// `_reversed` counterparts. Not normally used directly -- prefer the tree-specific macro, which
/// documents its own parameters; the shape is identical across all three.
///
/// `entry_` is the loop variable itself (computed directly in the condition from whichever node
/// `rki_tree_iter_next[_reversed]` just returned), so this is a single real loop: `break` exits
/// traversal immediately and `continue` re-evaluates the condition to advance to the next node,
/// exactly like a plain array loop. (An earlier version exposed `entry_` through an innermost
/// "do-once" loop nested inside the real traversal loop; that inner loop always ran to completion
/// regardless of the body, so a user's `break` only ever exited it and fell through to the real
/// loop's own re-check -- which unconditionally advances -- making `break` silently behave like
/// `continue`.)
#define RKI_TREE_FOREACH(self, stack_buf, stack_cap, entry_)                                       \
  for (struct {                                                                                    \
         typeof(*(self))* tree;                                                                    \
         RKI_TreeIter     it;                                                                      \
         TreeNode*        node;                                                                    \
       } RKI_state = {(self), {0}, rk_null};                                                       \
       RKI_state.tree                                                                              \
       && (RKI_state.it = (RKI_TreeIter){.stack = (stack_buf),                                     \
                                         .curr  = (TreeNode*)RKI_state.tree->base.root,            \
                                         .cap   = (stack_cap),                                     \
                                         .top   = 0},                                              \
          1);                                                                                      \
       RKI_state.tree = rk_null)                                                                   \
    for (typeof(RKI_TREE_ENTRY_PTR(RKI_state.tree)) entry_ = rk_null;                              \
         (RKI_state.node = rki_tree_iter_next(&RKI_state.it))                                      \
         && (entry_ = &((typeof(RKI_state.tree->rki_node_type))(void*)RKI_state.node)->entry, 1);)

#define RKI_TREE_FOREACH_REVERSED(self, stack_buf, stack_cap, entry_)                              \
  for (struct {                                                                                    \
         typeof(*(self))* tree;                                                                    \
         RKI_TreeIter     it;                                                                      \
         TreeNode*        node;                                                                    \
       } RKI_state = {(self), {0}, rk_null};                                                       \
       RKI_state.tree                                                                              \
       && (RKI_state.it = (RKI_TreeIter){.stack = (stack_buf),                                     \
                                         .curr  = (TreeNode*)RKI_state.tree->base.root,            \
                                         .cap   = (stack_cap),                                     \
                                         .top   = 0},                                              \
          1);                                                                                      \
       RKI_state.tree = rk_null)                                                                   \
    for (typeof(RKI_TREE_ENTRY_PTR(RKI_state.tree)) entry_ = rk_null;                              \
         (RKI_state.node = rki_tree_iter_next_reversed(&RKI_state.it))                             \
         && (entry_ = &((typeof(RKI_state.tree->rki_node_type))(void*)RKI_state.node)->entry, 1);)

/// @brief Shared erase_if implementation backing `bst_erase_if`/`avl_erase_if`/`rbt_erase_if`. Not
/// normally used directly.
///
/// Deleting a tree node can trigger rotations that would invalidate an in-progress traversal stack,
/// so this makes two passes instead of deleting while walking: first it traverses the tree once
/// (via `tree_foreach`, which does not mutate it) collecting the key of every entry satisfying
/// `pred` into a scratch buffer, then it removes each collected key through `remove_fn` -- the
/// variant's own, already-correct `remove` function, which rebalances exactly as it would for a
/// standalone `_remove()` call.
#define RKI_TREE_ERASE_IF(self, stack_buf, stack_cap, entry_, pred, remove_fn)                     \
  do {                                                                                             \
    typeof(*(self))* const RKI_eif_self = (self);                                                  \
    if (!RKI_eif_self->base.count) { break; }                                                      \
    const size_t RKI_eif_count = RKI_eif_self->base.count;                                         \
    typeof(RKI_eif_self->rki_node_type->entry_mod.key)* const RKI_eif_keys                         \
        = alloc_new(typeof(RKI_eif_self->rki_node_type->entry_mod.key),                            \
                    RKI_eif_count RK_IFALLOC(, RKI_eif_self->base.alloc));                         \
    size_t RKI_eif_n = 0;                                                                          \
    tree_foreach(RKI_eif_self, stack_buf, stack_cap, entry_) {                                     \
      if (pred) { RKI_eif_keys[RKI_eif_n++] = entry_->key; }                                       \
    }                                                                                              \
    for (size_t RKI_eif_i = 0; RKI_eif_i < RKI_eif_n; ++RKI_eif_i) {                               \
      remove_fn(RKI_eif_self, RKI_eif_keys[RKI_eif_i]);                                            \
    }                                                                                              \
    alloc_delete(RKI_eif_keys, RKI_eif_count RK_IFALLOC(, RKI_eif_self->base.alloc));              \
  } while (0)

///// These macros convert pointer values only. The stored links and root remain TreeNode*.
#define RKI_TREE_LEFT(node)             ((typeof(node))(void*)(node)->links.l)
#define RKI_TREE_RIGHT(node)            ((typeof(node))(void*)(node)->links.r)
#define RKI_TREE_ROOT(self)             ((typeof((self)->rki_node_type))(void*)(self)->base.root)
#define RKI_TREE_SET_LEFT(node, child)  ((node)->links.l = (TreeNode*)(child))
#define RKI_TREE_SET_RIGHT(node, child) ((node)->links.r = (TreeNode*)(child))
#define RKI_TREE_SET_ROOT(self, node)   ((self)->base.root = (TreeNode*)(node))

#ifdef __cplusplus
# define RKI_TREE_CHECK_NODE_LAYOUT(Node)                                                          \
   static_assert(std::is_standard_layout<Node>::value, "Tree nodes must be standard-layout")
#else
# define RKI_TREE_CHECK_NODE_LAYOUT(Node)                                                          \
   static_assert(offsetof(Node, links) == 0, "Tree node header must be the first member")
#endif

///////////////////////////////////////// Bst internals /////////////////////////////////////////

#define RKI_BstEntryPriv(K, V) RKI_bst_entry_##K##_##V

#define RKI_BST_INIT(K, V, A)                                                                      \
  ((Bst(K, V)){.base = {RK_IFALLOC(.alloc = A, ).count = 0, .root = rk_null}})
#define RKI_BST_INIT3(K, V, A)   RKI_REQUIRE_CUSTOM_ALLOCATORS(RKI_BST_INIT(K, V, A))
#define RKI_BST_INIT2(K, V)      RKI_BST_INIT(K, V, alloc_ctx)

#define RKI_BstNode(K, V)        RKI_bst_node_##K##_##V
#define RKI_BST_PUB(K, V, FNAME) bst_##K##_##V##_##FNAME
#define RKI_BST_PRI(K, V, FNAME) rki_bst_##K##_##V##_##FNAME

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
    TreeNode links;                                                                                \
    union {                                                                                        \
      RKI_BstEntryPriv(K, V) entry_mod;                                                            \
      BstEntry(K, V) entry;                                                                        \
    };                                                                                             \
  };                                                                                               \
  RKI_TREE_CHECK_NODE_LAYOUT(struct RKI_BstNode(K, V));                                            \
  typedef struct Bst(K, V) {                                                                       \
    union {                                                                                        \
      RKI_TreeData base;                                                                           \
      struct RKI_BstNode(K, V) * rki_node_type; /* Unevaluated type metadata only. */              \
    };                                                                                             \
  } Bst(K, V);                                                                                     \
  /* The only call site of the user's comparator: its parameters use reserved names, so a  */      \
  /* comparator named like a local of the functions below (c, key, n, ...) is not shadowed. */     \
  rklib_fun int RKI_BST_PRI(K, V, cmp)(K rki_var_a, K rki_var_b) {                                 \
    return CMP_FUN(rki_var_a, rki_var_b);                                                          \
  }                                                                                                \
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
  rklib_fun rk_pure TreeNode** RKI_BST_PRI(K, V, search_ptr)(Bst(K, V) * self, K key) {            \
    typedef struct RKI_BstNode(K, V) node_t;                                                       \
    TreeNode** curr = &self->base.root;                                                            \
    while (*curr) {                                                                                \
      node_t* node    = (node_t*)(void*)*curr;                                                     \
      int     cmp_res = RKI_BST_PRI(K, V, cmp)(key, node->entry.key);                              \
      if (!cmp_res) { break; }                                                                     \
      curr = cmp_res < 0 ? &node->links.l : &node->links.r;                                        \
    }                                                                                              \
    return curr;                                                                                   \
  }                                                                                                \
  rklib_fun rk_pure V* RKI_BST_PUB(K, V, get)(Bst(K, V) * self, K key) {                           \
    typedef struct RKI_BstNode(K, V) node_t;                                                       \
    node_t* node = (node_t*)(void*)*RKI_BST_PRI(K, V, search_ptr)(self, key);                      \
    return node ? &node->entry_mod.val : rk_null;                                                  \
  }                                                                                                \
  rklib_fun rk_pure bool RKI_BST_PUB(K, V, contains)(Bst(K, V) * self, K key) {                    \
    return !!*RKI_BST_PRI(K, V, search_ptr)(self, key);                                            \
  }                                                                                                \
  rklib_fun V* RKI_BST_PRI(K, V, set_add)(bool always_insert, Bst(K, V) * self, K key, V val) {    \
    typedef struct RKI_BstNode(K, V) node_t;                                                       \
    TreeNode** lnk = RKI_BST_PRI(K, V, search_ptr)(self, key);                                     \
    if (*lnk) {                                                                                    \
      if (always_insert) { ((node_t*)(void*)*lnk)->entry_mod.val = val; }                          \
      return rk_null;                                                                              \
    }                                                                                              \
    RKI_set_alloc_fallback(self->base.alloc);                                                      \
    node_t* node  = alloc_new(node_t, 1 RK_IFALLOC(, self->base.alloc));                           \
    node->links.l = node->links.r = rk_null;                                                       \
    node->entry_mod               = (typeof(node->entry_mod)){.key = key, .val = val};             \
    *lnk                          = &node->links;                                                  \
    ++self->base.count;                                                                            \
    return &node->entry_mod.val;                                                                   \
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
    TreeNode** lnk = RKI_BST_PRI(K, V, search_ptr)(self, key);                                     \
    if (*lnk) {                                                                                    \
      if (inserted_out) { *inserted_out = false; }                                                 \
      return &((node_t*)(void*)*lnk)->entry_mod.val;                                               \
    }                                                                                              \
    RKI_set_alloc_fallback(self->base.alloc);                                                      \
    node_t* node  = alloc_new(node_t, 1 RK_IFALLOC(, self->base.alloc));                           \
    node->links.l = node->links.r = rk_null;                                                       \
    node->entry_mod               = (typeof(node->entry_mod)){.key = key, .val = val};             \
    *lnk                          = &node->links;                                                  \
    ++self->base.count;                                                                            \
    if (inserted_out) { *inserted_out = true; }                                                    \
    return &node->entry_mod.val;                                                                   \
  }                                                                                                \
  rklib_fun bool RKI_BST_PUB(K, V, extract)(Bst(K, V) * self, K key, V * val_out) {                \
    rk_assert_ptr_nonnull(val_out);                                                                \
    typedef struct RKI_BstNode(K, V) node_t;                                                       \
    TreeNode** lnk = RKI_BST_PRI(K, V, search_ptr)(self, key);                                     \
    if (!*lnk) { return false; }                                                                   \
    --self->base.count;                                                                            \
    node_t* curr = (node_t*)(void*)*lnk;                                                           \
    *val_out     = curr->entry_mod.val;                                                            \
    if (curr->links.l && curr->links.r) {                                                          \
      lnk = &curr->links.r;                                                                        \
      while ((*lnk)->l) { lnk = &(*lnk)->l; }                                                      \
      node_t* succ    = (node_t*)(void*)*lnk;                                                      \
      curr->entry_mod = succ->entry_mod;                                                           \
      *lnk            = succ->links.r;                                                             \
      alloc_delete(succ, 1 RK_IFALLOC(, self->base.alloc));                                        \
      return true;                                                                                 \
    }                                                                                              \
    *lnk = curr->links.l ? curr->links.l : curr->links.r;                                          \
    alloc_delete(curr, 1 RK_IFALLOC(, self->base.alloc));                                          \
    return true;                                                                                   \
  }                                                                                                \
  rklib_fun bool RKI_BST_PUB(K, V, remove)(Bst(K, V) * self, K key) {                              \
    V ignored;                                                                                     \
    return RKI_BST_PUB(K, V, extract)(self, key, &ignored);                                        \
  }                                                                                                \
  RK_EXTERNC_END

//////////////////////////////////////////// Avl internal /////////////////////////////////////////

#define RKI_AVL_INIT(K, V, A)                                                                      \
  ((Avl(K, V)){.base = {RK_IFALLOC(.alloc = A, ).count = 0, .root = rk_null}})
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
    TreeNode links;                                                                                \
    int      height;                                                                               \
    union {                                                                                        \
      RKI_AVL_PRI(K, V, entry) entry_mod;                                                          \
      AvlEntry(K, V) entry;                                                                        \
    };                                                                                             \
  } RKI_AvlNode(K, V);                                                                             \
  RKI_TREE_CHECK_NODE_LAYOUT(RKI_AvlNode(K, V));                                                   \
  typedef struct Avl(K, V) {                                                                       \
    union {                                                                                        \
      RKI_TreeData base;                                                                           \
      RKI_AvlNode(K, V) * rki_node_type; /* Unevaluated type metadata only. */                     \
    };                                                                                             \
  } Avl(K, V);                                                                                     \
  /* The only call site of the user's comparator: its parameters use reserved names, so a  */      \
  /* comparator named like a local of the functions below (c, key, n, ...) is not shadowed. */     \
  rklib_fun int RKI_AVL_PRI(K, V, cmp)(K rki_var_a, K rki_var_b) {                                 \
    return CMP_FUN(rki_var_a, rki_var_b);                                                          \
  }                                                                                                \
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
    int a = RKI_AVL_PRI(K, V, h)(RKI_TREE_LEFT(n)), b = RKI_AVL_PRI(K, V, h)(RKI_TREE_RIGHT(n));   \
    n->height = 1 + (a > b ? a : b);                                                               \
  }                                                                                                \
  rklib_fun RKI_AvlNode(K, V) * RKI_AVL_PRI(K, V, rotl)(RKI_AvlNode(K, V) * x) {                   \
    RKI_AvlNode(K, V)* y = RKI_TREE_RIGHT(x);                                                      \
    RKI_TREE_SET_RIGHT(x, RKI_TREE_LEFT(y));                                                       \
    RKI_TREE_SET_LEFT(y, x);                                                                       \
    RKI_AVL_PRI(K, V, fixh)(x);                                                                    \
    RKI_AVL_PRI(K, V, fixh)(y);                                                                    \
    return y;                                                                                      \
  }                                                                                                \
  rklib_fun RKI_AvlNode(K, V) * RKI_AVL_PRI(K, V, rotr)(RKI_AvlNode(K, V) * y) {                   \
    RKI_AvlNode(K, V)* x = RKI_TREE_LEFT(y);                                                       \
    RKI_TREE_SET_LEFT(y, RKI_TREE_RIGHT(x));                                                       \
    RKI_TREE_SET_RIGHT(x, y);                                                                      \
    RKI_AVL_PRI(K, V, fixh)(y);                                                                    \
    RKI_AVL_PRI(K, V, fixh)(x);                                                                    \
    return x;                                                                                      \
  }                                                                                                \
  rklib_fun RKI_AvlNode(K, V) * RKI_AVL_PRI(K, V, balance)(RKI_AvlNode(K, V) * n) {                \
    RKI_AVL_PRI(K, V, fixh)(n);                                                                    \
    int bf = RKI_AVL_PRI(K, V, h)(RKI_TREE_LEFT(n)) - RKI_AVL_PRI(K, V, h)(RKI_TREE_RIGHT(n));     \
    if (bf > 1) {                                                                                  \
      if (RKI_AVL_PRI(K, V, h)(RKI_TREE_LEFT(RKI_TREE_LEFT(n)))                                    \
          < RKI_AVL_PRI(K, V, h)(RKI_TREE_RIGHT(RKI_TREE_LEFT(n)))) {                              \
        RKI_TREE_SET_LEFT(n, RKI_AVL_PRI(K, V, rotl)(RKI_TREE_LEFT(n)));                           \
      }                                                                                            \
      return RKI_AVL_PRI(K, V, rotr)(n);                                                           \
    }                                                                                              \
    if (bf < -1) {                                                                                 \
      if (RKI_AVL_PRI(K, V, h)(RKI_TREE_RIGHT(RKI_TREE_RIGHT(n)))                                  \
          < RKI_AVL_PRI(K, V, h)(RKI_TREE_LEFT(RKI_TREE_RIGHT(n)))) {                              \
        RKI_TREE_SET_RIGHT(n, RKI_AVL_PRI(K, V, rotr)(RKI_TREE_RIGHT(n)));                         \
      }                                                                                            \
      return RKI_AVL_PRI(K, V, rotl)(n);                                                           \
    }                                                                                              \
    return n;                                                                                      \
  }                                                                                                \
  rklib_fun RKI_AvlNode(K, V)                                                                      \
      * RKI_AVL_PRI(K, V, put)(Avl(K, V) * self, RKI_AvlNode(K, V) * n, K key, V val,              \
                               bool overwrite, V** out, bool* added) {                             \
    if (!n) {                                                                                      \
      RKI_set_alloc_fallback(self->base.alloc);                                                    \
      n          = alloc_new(RKI_AvlNode(K, V), 1 RK_IFALLOC(, self->base.alloc));                 \
      n->links.l = n->links.r = rk_null;                                                           \
      n->height               = 1;                                                                 \
      n->entry_mod            = (typeof(n->entry_mod)){.key = key, .val = val};                    \
      *out                    = &n->entry_mod.val;                                                 \
      *added                  = true;                                                              \
      return n;                                                                                    \
    }                                                                                              \
    int c = RKI_AVL_PRI(K, V, cmp)(key, n->entry.key);                                             \
    if (!c) {                                                                                      \
      if (overwrite) { n->entry_mod.val = val; }                                                   \
      *out = &n->entry_mod.val;                                                                    \
      return n;                                                                                    \
    }                                                                                              \
    if (c < 0) {                                                                                   \
      RKI_TREE_SET_LEFT(                                                                           \
          n, RKI_AVL_PRI(K, V, put)(self, RKI_TREE_LEFT(n), key, val, overwrite, out, added));     \
    } else {                                                                                       \
      RKI_TREE_SET_RIGHT(                                                                          \
          n, RKI_AVL_PRI(K, V, put)(self, RKI_TREE_RIGHT(n), key, val, overwrite, out, added));    \
    }                                                                                              \
    return RKI_AVL_PRI(K, V, balance)(n);                                                          \
  }                                                                                                \
  rklib_fun rk_pure V* RKI_AVL_PUB(K, V, get)(Avl(K, V) * self, K key) {                           \
    RKI_AvlNode(K, V)* n = RKI_TREE_ROOT(self);                                                    \
    while (n) {                                                                                    \
      int c = RKI_AVL_PRI(K, V, cmp)(key, n->entry.key);                                           \
      if (!c) { return &n->entry_mod.val; }                                                        \
      n = c < 0 ? RKI_TREE_LEFT(n) : RKI_TREE_RIGHT(n);                                            \
    }                                                                                              \
    return rk_null;                                                                                \
  }                                                                                                \
  rklib_fun V* RKI_AVL_PRI(K, V, setadd)(Avl(K, V) * self, K key, V val, bool overwrite,           \
                                         bool* added) {                                            \
    V* out = rk_null;                                                                              \
    *added = false;                                                                                \
    RKI_TREE_SET_ROOT(self, RKI_AVL_PRI(K, V, put)(self, RKI_TREE_ROOT(self), key, val, overwrite, \
                                                   &out, added));                                  \
    if (*added) { ++self->base.count; }                                                            \
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
    if (!RKI_TREE_LEFT(n)) {                                                                       \
      *out = n;                                                                                    \
      return RKI_TREE_RIGHT(n);                                                                    \
    }                                                                                              \
    RKI_TREE_SET_LEFT(n, RKI_AVL_PRI(K, V, detach_min)(RKI_TREE_LEFT(n), out));                    \
    return RKI_AVL_PRI(K, V, balance)(n);                                                          \
  }                                                                                                \
  rklib_fun RKI_AvlNode(K, V)                                                                      \
      * RKI_AVL_PRI(K, V, erase)(Avl(K, V) * self, RKI_AvlNode(K, V) * n, K key, V * out,          \
                                 bool* removed) {                                                  \
    if (!n) return rk_null;                                                                        \
    int c = RKI_AVL_PRI(K, V, cmp)(key, n->entry.key);                                             \
    if (c < 0) {                                                                                   \
      RKI_TREE_SET_LEFT(n, RKI_AVL_PRI(K, V, erase)(self, RKI_TREE_LEFT(n), key, out, removed));   \
    } else if (c > 0) {                                                                            \
      RKI_TREE_SET_RIGHT(n, RKI_AVL_PRI(K, V, erase)(self, RKI_TREE_RIGHT(n), key, out, removed)); \
    } else {                                                                                       \
      *out                 = n->entry_mod.val;                                                     \
      *removed             = true;                                                                 \
      RKI_AvlNode(K, V)* l = RKI_TREE_LEFT(n), *r = RKI_TREE_RIGHT(n);                             \
      if (!r) {                                                                                    \
        alloc_delete(n, 1 RK_IFALLOC(, self->base.alloc));                                         \
        return l;                                                                                  \
      }                                                                                            \
      RKI_AvlNode(K, V) * m;                                                                       \
      r = RKI_AVL_PRI(K, V, detach_min)(r, &m);                                                    \
      RKI_TREE_SET_LEFT(m, l);                                                                     \
      RKI_TREE_SET_RIGHT(m, r);                                                                    \
      alloc_delete(n, 1 RK_IFALLOC(, self->base.alloc));                                           \
      return RKI_AVL_PRI(K, V, balance)(m);                                                        \
    }                                                                                              \
    return *removed ? RKI_AVL_PRI(K, V, balance)(n) : n;                                           \
  }                                                                                                \
  rklib_fun bool RKI_AVL_PUB(K, V, extract)(Avl(K, V) * self, K key, V * out) {                    \
    rk_assert_ptr_nonnull(out);                                                                    \
    bool removed = false;                                                                          \
    RKI_TREE_SET_ROOT(self,                                                                        \
                      RKI_AVL_PRI(K, V, erase)(self, RKI_TREE_ROOT(self), key, out, &removed));    \
    if (removed) { --self->base.count; }                                                           \
    return removed;                                                                                \
  }                                                                                                \
  rklib_fun bool RKI_AVL_PUB(K, V, remove)(Avl(K, V) * self, K key) {                              \
    V tmp;                                                                                         \
    return RKI_AVL_PUB(K, V, extract)(self, key, &tmp);                                            \
  }                                                                                                \
  RK_EXTERNC_END

//////////////////////////////////////////// Rbt internals /////////////////////////////////////////

#define RKI_RBT_INIT(K, V, A)                                                                      \
  ((Rbt(K, V)){.base = {RK_IFALLOC(.alloc = A, ).count = 0, .root = rk_null}})
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
    TreeNode links;                                                                                \
    bool     red;                                                                                  \
    union {                                                                                        \
      RKI_RBT_PRI(K, V, E) entry_mod;                                                              \
      RbtEntry(K, V) entry;                                                                        \
    };                                                                                             \
  } RKI_RbtNode(K, V);                                                                             \
  RKI_TREE_CHECK_NODE_LAYOUT(RKI_RbtNode(K, V));                                                   \
  typedef struct Rbt(K, V) {                                                                       \
    union {                                                                                        \
      RKI_TreeData base;                                                                           \
      RKI_RbtNode(K, V) * rki_node_type; /* Unevaluated type metadata only. */                     \
    };                                                                                             \
  } Rbt(K, V);                                                                                     \
  /* The only call site of the user's comparator: its parameters use reserved names, so a  */      \
  /* comparator named like a local of the functions below (c, key, n, ...) is not shadowed. */     \
  rklib_fun int RKI_RBT_PRI(K, V, cmp)(K rki_var_a, K rki_var_b) {                                 \
    return CMP(rki_var_a, rki_var_b);                                                              \
  }                                                                                                \
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
    RKI_RbtNode(K, V)* x = RKI_TREE_RIGHT(h);                                                      \
    RKI_TREE_SET_RIGHT(h, RKI_TREE_LEFT(x));                                                       \
    RKI_TREE_SET_LEFT(x, h);                                                                       \
    x->red = h->red;                                                                               \
    h->red = true;                                                                                 \
    return x;                                                                                      \
  }                                                                                                \
  rklib_fun RKI_RbtNode(K, V) * RKI_RBT_PRI(K, V, rr)(RKI_RbtNode(K, V) * h) {                     \
    RKI_RbtNode(K, V)* x = RKI_TREE_LEFT(h);                                                       \
    RKI_TREE_SET_LEFT(h, RKI_TREE_RIGHT(x));                                                       \
    RKI_TREE_SET_RIGHT(x, h);                                                                      \
    x->red = h->red;                                                                               \
    h->red = true;                                                                                 \
    return x;                                                                                      \
  }                                                                                                \
  rklib_fun void RKI_RBT_PRI(K, V, flip)(RKI_RbtNode(K, V) * h) {                                  \
    h->red                 = !h->red;                                                              \
    RKI_TREE_LEFT(h)->red  = !RKI_TREE_LEFT(h)->red;                                               \
    RKI_TREE_RIGHT(h)->red = !RKI_TREE_RIGHT(h)->red;                                              \
  }                                                                                                \
  rklib_fun RKI_RbtNode(K, V) * RKI_RBT_PRI(K, V, fix)(RKI_RbtNode(K, V) * h) {                    \
    if (RKI_RBT_PRI(K, V, red)(RKI_TREE_RIGHT(h))) { h = RKI_RBT_PRI(K, V, rl)(h); }               \
    if (RKI_RBT_PRI(K, V, red)(RKI_TREE_LEFT(h))                                                   \
        && RKI_RBT_PRI(K, V, red)(RKI_TREE_LEFT(RKI_TREE_LEFT(h)))) {                              \
      h = RKI_RBT_PRI(K, V, rr)(h);                                                                \
    }                                                                                              \
    if (RKI_RBT_PRI(K, V, red)(RKI_TREE_LEFT(h)) && RKI_RBT_PRI(K, V, red)(RKI_TREE_RIGHT(h))) {   \
      RKI_RBT_PRI(K, V, flip)(h);                                                                  \
    }                                                                                              \
    return h;                                                                                      \
  }                                                                                                \
  rklib_fun RKI_RbtNode(K, V) * RKI_RBT_PRI(K, V, ml)(RKI_RbtNode(K, V) * h) {                     \
    RKI_RBT_PRI(K, V, flip)(h);                                                                    \
    if (RKI_RBT_PRI(K, V, red)(RKI_TREE_LEFT(RKI_TREE_RIGHT(h)))) {                                \
      RKI_TREE_SET_RIGHT(h, RKI_RBT_PRI(K, V, rr)(RKI_TREE_RIGHT(h)));                             \
      h = RKI_RBT_PRI(K, V, rl)(h);                                                                \
      RKI_RBT_PRI(K, V, flip)(h);                                                                  \
    }                                                                                              \
    return h;                                                                                      \
  }                                                                                                \
  rklib_fun RKI_RbtNode(K, V) * RKI_RBT_PRI(K, V, mr)(RKI_RbtNode(K, V) * h) {                     \
    RKI_RBT_PRI(K, V, flip)(h);                                                                    \
    if (RKI_RBT_PRI(K, V, red)(RKI_TREE_LEFT(RKI_TREE_LEFT(h)))) {                                 \
      h = RKI_RBT_PRI(K, V, rr)(h);                                                                \
      RKI_RBT_PRI(K, V, flip)(h);                                                                  \
    }                                                                                              \
    return h;                                                                                      \
  }                                                                                                \
  rklib_fun RKI_RbtNode(K, V)                                                                      \
      * RKI_RBT_PRI(K, V, put)(Rbt(K, V) * s, RKI_RbtNode(K, V) * h, K k, V v, bool ow, V** out,   \
                               bool* added) {                                                      \
    if (!h) {                                                                                      \
      RKI_set_alloc_fallback(s->base.alloc);                                                       \
      h          = alloc_new(RKI_RbtNode(K, V), 1 RK_IFALLOC(, s->base.alloc));                    \
      h->links.l = h->links.r = rk_null;                                                           \
      h->red                  = true;                                                              \
      h->entry_mod            = (typeof(h->entry_mod)){.key = k, .val = v};                        \
      *out                    = &h->entry_mod.val;                                                 \
      *added                  = true;                                                              \
      return h;                                                                                    \
    }                                                                                              \
    int c = RKI_RBT_PRI(K, V, cmp)(k, h->entry.key);                                               \
    if (c < 0) {                                                                                   \
      RKI_TREE_SET_LEFT(h, RKI_RBT_PRI(K, V, put)(s, RKI_TREE_LEFT(h), k, v, ow, out, added));     \
    } else if (c > 0) {                                                                            \
      RKI_TREE_SET_RIGHT(h, RKI_RBT_PRI(K, V, put)(s, RKI_TREE_RIGHT(h), k, v, ow, out, added));   \
    } else {                                                                                       \
      if (ow) { h->entry_mod.val = v; }                                                            \
      *out = &h->entry_mod.val;                                                                    \
    }                                                                                              \
    return RKI_RBT_PRI(K, V, fix)(h);                                                              \
  }                                                                                                \
  rklib_fun rk_pure V* RKI_RBT_PUB(K, V, get)(Rbt(K, V) * s, K k) {                                \
    RKI_RbtNode(K, V)* n = RKI_TREE_ROOT(s);                                                       \
    while (n) {                                                                                    \
      int c = RKI_RBT_PRI(K, V, cmp)(k, n->entry.key);                                             \
      if (!c) { return &n->entry_mod.val; }                                                        \
      n = c < 0 ? RKI_TREE_LEFT(n) : RKI_TREE_RIGHT(n);                                            \
    }                                                                                              \
    return rk_null;                                                                                \
  }                                                                                                \
  rklib_fun V* RKI_RBT_PRI(K, V, insert)(Rbt(K, V) * s, K k, V v, bool ow, bool* added) {          \
    V* out = rk_null;                                                                              \
    *added = false;                                                                                \
    RKI_TREE_SET_ROOT(s, RKI_RBT_PRI(K, V, put)(s, RKI_TREE_ROOT(s), k, v, ow, &out, added));      \
    RKI_TREE_ROOT(s)->red = false;                                                                 \
    if (*added) { ++s->base.count; }                                                               \
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
    while (RKI_TREE_LEFT(h)) { h = RKI_TREE_LEFT(h); }                                             \
    return h;                                                                                      \
  }                                                                                                \
  rklib_fun RKI_RbtNode(K, V) * RKI_RBT_PRI(K, V, dm)(Rbt(K, V) * s, RKI_RbtNode(K, V) * h) {      \
    if (!RKI_TREE_LEFT(h)) {                                                                       \
      alloc_delete(h, 1 RK_IFALLOC(, s->base.alloc));                                              \
      return rk_null;                                                                              \
    }                                                                                              \
    if (!RKI_RBT_PRI(K, V, red)(RKI_TREE_LEFT(h))                                                  \
        && !RKI_RBT_PRI(K, V, red)(RKI_TREE_LEFT(RKI_TREE_LEFT(h)))) {                             \
      h = RKI_RBT_PRI(K, V, ml)(h);                                                                \
    }                                                                                              \
    RKI_TREE_SET_LEFT(h, RKI_RBT_PRI(K, V, dm)(s, RKI_TREE_LEFT(h)));                              \
    return RKI_RBT_PRI(K, V, fix)(h);                                                              \
  }                                                                                                \
  rklib_fun RKI_RbtNode(K, V)                                                                      \
      * RKI_RBT_PRI(K, V, del)(Rbt(K, V) * s, RKI_RbtNode(K, V) * h, K k, V * out, bool* gone) {   \
    if (RKI_RBT_PRI(K, V, cmp)(k, h->entry.key) < 0) {                                             \
      if (RKI_TREE_LEFT(h)) {                                                                      \
        if (!RKI_RBT_PRI(K, V, red)(RKI_TREE_LEFT(h))                                              \
            && !RKI_RBT_PRI(K, V, red)(RKI_TREE_LEFT(RKI_TREE_LEFT(h)))) {                         \
          h = RKI_RBT_PRI(K, V, ml)(h);                                                            \
        }                                                                                          \
        RKI_TREE_SET_LEFT(h, RKI_RBT_PRI(K, V, del)(s, RKI_TREE_LEFT(h), k, out, gone));           \
      }                                                                                            \
    } else {                                                                                       \
      if (RKI_RBT_PRI(K, V, red)(RKI_TREE_LEFT(h))) { h = RKI_RBT_PRI(K, V, rr)(h); }              \
      int c = RKI_RBT_PRI(K, V, cmp)(k, h->entry.key);                                             \
      if (!c && !RKI_TREE_RIGHT(h)) {                                                              \
        *out  = h->entry_mod.val;                                                                  \
        *gone = true;                                                                              \
        alloc_delete(h, 1 RK_IFALLOC(, s->base.alloc));                                            \
        return rk_null;                                                                            \
      }                                                                                            \
      if (RKI_TREE_RIGHT(h)) {                                                                     \
        if (!RKI_RBT_PRI(K, V, red)(RKI_TREE_RIGHT(h))                                             \
            && !RKI_RBT_PRI(K, V, red)(RKI_TREE_LEFT(RKI_TREE_RIGHT(h)))) {                        \
          h = RKI_RBT_PRI(K, V, mr)(h);                                                            \
        }                                                                                          \
        c = RKI_RBT_PRI(K, V, cmp)(k, h->entry.key);                                               \
        if (!c) {                                                                                  \
          RKI_RbtNode(K, V)* m = RKI_RBT_PRI(K, V, mn)(RKI_TREE_RIGHT(h));                         \
          *out                 = h->entry_mod.val;                                                 \
          *gone                = true;                                                             \
          h->entry_mod         = m->entry_mod;                                                     \
          RKI_TREE_SET_RIGHT(h, RKI_RBT_PRI(K, V, dm)(s, RKI_TREE_RIGHT(h)));                      \
        } else {                                                                                   \
          RKI_TREE_SET_RIGHT(h, RKI_RBT_PRI(K, V, del)(s, RKI_TREE_RIGHT(h), k, out, gone));       \
        }                                                                                          \
      }                                                                                            \
    }                                                                                              \
    return RKI_RBT_PRI(K, V, fix)(h);                                                              \
  }                                                                                                \
  rklib_fun bool RKI_RBT_PUB(K, V, extract)(Rbt(K, V) * s, K k, V * out) {                         \
    rk_assert_ptr_nonnull(out);                                                                    \
    if (!RKI_TREE_ROOT(s) || !RKI_RBT_PUB(K, V, get)(s, k)) { return false; }                      \
    bool gone = false;                                                                             \
    if (!RKI_RBT_PRI(K, V, red)(RKI_TREE_LEFT(RKI_TREE_ROOT(s)))                                   \
        && !RKI_RBT_PRI(K, V, red)(RKI_TREE_RIGHT(RKI_TREE_ROOT(s)))) {                            \
      RKI_TREE_ROOT(s)->red = true;                                                                \
    }                                                                                              \
    RKI_TREE_SET_ROOT(s, RKI_RBT_PRI(K, V, del)(s, RKI_TREE_ROOT(s), k, out, &gone));              \
    if (RKI_TREE_ROOT(s)) { RKI_TREE_ROOT(s)->red = false; }                                       \
    if (gone) { --s->base.count; }                                                                 \
    return gone;                                                                                   \
  }                                                                                                \
  rklib_fun bool RKI_RBT_PUB(K, V, remove)(Rbt(K, V) * s, K k) {                                   \
    V x;                                                                                           \
    return RKI_RBT_PUB(K, V, extract)(s, k, &x);                                                   \
  }                                                                                                \
  RK_EXTERNC_END

/// @endcond
#pragma endregion implementation
RKI_HEADER_END
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
