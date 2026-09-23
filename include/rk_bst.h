// SPDX-License-Identifier: MIT
/// @file rk_bst.h
/// @version 1.0
/// @defgroup rk_bst Binary Search Tree (BST) Interface
/// @brief Type-safe, generic binary search tree for C.
///
/// Provides an unbalanced binary search tree supporting key-value storage, lookup, insertion,
/// removal, and in-order traversal. The tree allocates nodes individually via the `Allocator`
/// interface.
///
/// Usage:
/// 1. Define a comparison function: `int cmp_f(K a, K b)` returning negative, zero, or positive
///    (like `strcmp`).
///
/// 2. Create typedefs for key and value types if needed (pointers and structs require typedefs due
///    to the C preprocessor):
///    ```c
///    typedef char* cstr;
///    ```
///
/// 3. Declare and instantiate a specialised BST:
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
///    bst_node* stack[64];
///    bst_foreach(&tree, stack, 64, entry) {
///        printf("%d -> %s\n", entry->key, entry->val);
///    }
///    ```
///
/// 6. Free resources when done:
///    ```c
///    bst_release(&tree);
///    ```
///
/// @note The tree is unbalanced. For highly skewed insertion order, consider a different data
/// structure.
/// @note Not thread-safe.
/// @see rk_alloc.h
/// @see rk_dict.h
/// @{

#ifndef RK_BST_H
#define RK_BST_H
#include "rk_alloc.h"
#include "rk_vec.h"
RK_HEADER_BEGIN

/// @brief Generates a type-specific BST struct name.
#define Bst(K, V)                       bst_##K##_##V
/// @brief Generates a type-specific BST entry struct name.
#define BstEntry(K, V)                  bst_entry_##K##_##V

/// @brief `Bst(K, V) bst_init(K, V, Allocator alloc = alloc_ctx)` - Initialises and returns an
/// empty BST.
/// @param K     Key type name
/// @param V     Value type name
/// @param alloc Optional allocator; defaults to `alloc_ctx`
/// @return An initialised, empty `Bst(K, V)`
#define bst_init(K, V, ...)             rk_overload(RK__bst_init, K, V, ##__VA_ARGS__)

/// @brief `Bst(K, V) bst_init_static(Allocator alloc = alloc_ctx)` - Static/compile-time
/// initializer for a BST. Suitable for global and static variables.
/// @param K     Key type name
/// @param V     Value type name
/// @param alloc Optional allocator; defaults to `alloc_ctx`
#define bst_init_static(K, V, ...)      rk_overload(RK__bst_init_static, K, V, ##__VA_ARGS__)

/// @brief `void bst_release(K, V, Bst(K, V)* self)` - Frees all nodes in the BST and resets it to
/// an empty state.
#define bst_release(K, V, self)         RK__BST_PUB(K, V, release)(self)

/// @brief `size_t bst_count(Bst(K, V)* self)` - Returns the number of key-value pairs stored in the
/// BST.
#define bst_count(self)                 ((size_t)((self)->count))

/// @brief `bool bst_is_empty(Bst(K, V)* self)` - Returns `true` iff the BST contains no elements.
#define bst_is_empty(self)              ((bool)(bst_count(self) == 0))

/// @brief `bool bst_set(K, V, Bst(K, V)* self, K key, V value)` - Inserts or updates a key-value
/// pair. If `key` is already present, its value is overwritten. If not, a new node is allocated and
/// inserted.
/// @return `true` if a new node was inserted, `false` if an existing value was updated
#define bst_set(K, V, self, key, value) RK__BST_PUB(K, V, set)(self, key, value)

/// @brief `V* bst_add(K, V, Bst(K, V)* self, K key, V value)` - Inserts a key-value pair only if
/// `key` is not already present. Existing values are not overwritten.
/// @return Pointer to the added object, if added, or `NULL`, if not
#define bst_add(K, V, self, key, value) RK__BST_PUB(K, V, add)(self, key, value)

/// @brief `V* bst_get(K, V, Bst(K, V)* self, K key)` - Looks up a key and returns a pointer to its
/// associated value, or `NULL` if not found.
/// @return Pointer to the value, or `NULL` if the key is absent
#define bst_get(K, V, self, key)        RK__BST_PUB(K, V, get)(self, key)

/// @brief `bool bst_contains(K, V, Bst(K, V)* self, K key)` - Returns `true` iff the BST contains
/// an entry with the given key.
#define bst_contains(K, V, self, key)   RK__BST_PUB(K, V, contains)(self, key)

/// @brief `bool bst_extract(K, V, Bst(K, V)* self, K key, V* value_outptr)` - Removes the entry
/// with `key` from the BST and writes its value to `value_outptr`.
/// @param value_outptr Non-null pointer; where the removed value is written, if found
/// @return `true` if the key was found and removed, `false` otherwise
#define bst_extract(K, V, self, key, value_outptr)                                                 \
  RK__BST_PUB(K, V, extract)(self, key, value_outptr)

/// @brief `bool bst_remove(K, V, Bst(K, V)* self, K key)` - Removes the entry with `key` from the
/// BST, discarding its value.
/// @return `true` if the key was found and removed, `false` otherwise
#define bst_remove(K, V, self, key) RK__BST_PUB(K, V, remove)(self, key)

/// @brief `BstEntry(K, V)* bst_min(Bst(K, V)* self)` - Returns a pointer to the entry with the
/// smallest key, or `NULL` if the BST is empty.
#define bst_min(self)               ((typeof((self)->root->entry)*)RK__bst_min(((self)->_bst.root)))

/// @brief `BstEntry(K, V)* bst_max(Bst(K, V)* self)` - Returns a pointer to the entry with the
/// largest key, or `NULL` if the BST is empty.
#define bst_max(self)               ((typeof((self)->root->entry)*)RK__bst_max(((self)->_bst.root)))

#if RK_CUSTOM_ALLOCATORS
# define bst_allocator(self) rk_to_rvalue((self)->alloc)
#else
# define bst_allocator(self) ((void)(self), alloc_ctx)
#endif

typedef struct bst_node {
  struct bst_node *l, *r;
  char             data[];
} bst_node;

typedef struct bst_data {
  RK_IFALLOC(Allocator alloc;)
  size_t    count;
  bst_node* root;
} bst_data;

typedef struct bst_iter {
  bst_node **stack, *curr;
  size_t     cap, top;
} bst_iter;

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
#define BST_DEFINE(K, V, CMP_FUN)                                                                  \
  RK_EXTERNC_BEG                                                                                   \
  typedef struct BstEntry(K, V) {                                                                  \
    K const key;                                                                                   \
    V       val;                                                                                   \
  } BstEntry(K, V);                                                                                \
  typedef struct RK__BstEntryPriv(K, V) {                                                          \
    K key;                                                                                         \
    V val;                                                                                         \
  } RK__BstEntryPriv(K, V);                                                                        \
  struct RK__BstNode(K, V) {                                                                       \
    struct RK__BstNode(K, V) * l, *r;                                                              \
    union {                                                                                        \
      alignas_max RK__BstEntryPriv(K, V) entry_mod;                                                \
      alignas_max BstEntry(K, V) entry;                                                            \
    };                                                                                             \
  };                                                                                               \
  typedef struct Bst(K, V) {                                                                       \
    union {                                                                                        \
      bst_data _bst;                                                                               \
      struct {                                                                                     \
        RK_IFALLOC(Allocator alloc;)                                                               \
        size_t count;                                                                              \
        struct RK__BstNode(K, V) * root;                                                           \
      };                                                                                           \
    };                                                                                             \
  } Bst(K, V);                                                                                     \
  static_fun void RK__BST_PUB(K, V, release_)(struct RK__BstNode(K, V)                             \
                                              * node RK_IFALLOC(, Allocator alloc)) {              \
    typedef struct RK__BstNode(K, V) node_t;                                                       \
    while (node) {                                                                                 \
      if (node->l) {                                                                               \
        node_t* l = node->l;                                                                       \
        node->l   = l->r;                                                                          \
        l->r      = node;                                                                          \
        node      = l;                                                                             \
      } else {                                                                                     \
        node_t* r = node->r;                                                                       \
        alloc_delete(node, 1 RK_IFALLOC(, alloc));                                                 \
        node = r;                                                                                  \
      }                                                                                            \
    }                                                                                              \
  }                                                                                                \
  static_fun void RK__BST_PUB(K, V, release)(Bst(K, V) * self) {                                   \
    RK__BST_PUB(K, V, release_)(self->root RK_IFALLOC(, self->alloc));                             \
    self->count = 0;                                                                               \
  }                                                                                                \
  static_fun struct RK__BstNode(K, V) * *RK__BST_PUB(K, V, search_ptr)(Bst(K, V) * self, K key) {  \
    typedef struct RK__BstNode(K, V) node_t;                                                       \
    node_t** curr = &self->root;                                                                   \
    for (; *curr;) {                                                                               \
      int cmp_res = CMP_FUN(key, (*curr)->entry.key);                                              \
      if (cmp_res == 0) { break; }                                                                 \
      curr = cmp_res < 0 ? &((*curr)->l) : &((*curr)->r);                                          \
    }                                                                                              \
    return curr;                                                                                   \
  }                                                                                                \
  static_fun V* RK__BST_PUB(K, V, get)(Bst(K, V) * self, K key) {                                  \
    typedef struct RK__BstNode(K, V) node_t;                                                       \
    node_t** node = RK__BST_PUB(K, V, search_ptr)(self, key);                                      \
    return *node ? &((*node)->entry.val) : rk_null;                                                \
  }                                                                                                \
  static_fun bool RK__BST_PUB(K, V, contains)(Bst(K, V) * self, K key) {                           \
    return (bool)(*RK__BST_PUB(K, V, search_ptr)(self, key));                                      \
  }                                                                                                \
  static_fun V* RK__BST_PRI(K, V, set_add)(const bool always_insert, Bst(K, V) * self, K key,      \
                                           V val) {                                                \
    typedef struct RK__BstNode(K, V) node_t;                                                       \
    node_t** lnk = RK__BST_PUB(K, V, search_ptr)(self, key);                                       \
    if (*lnk) {                                                                                    \
      if (always_insert) { (*lnk)->entry.val = val; }                                              \
      return rk_null;                                                                              \
    }                                                                                              \
    rk_set_alloc_fallback(self->alloc);                                                            \
    node_t* n = alloc_new(node_t, 1 RK_IFALLOC(, self->alloc));                                    \
    n->r = n->l  = rk_null;                                                                        \
    n->entry_mod = (typeof(n->entry_mod)){.key = key, .val = val};                                 \
    *lnk         = n;                                                                              \
    ++self->count;                                                                                 \
    return &(n->entry_mod.val);                                                                    \
  }                                                                                                \
  static_fun bool RK__BST_PUB(K, V, set)(Bst(K, V) * self, K key, V val) {                         \
    return (bool)RK__BST_PRI(K, V, set_add)(true, self, key, val);                                 \
  }                                                                                                \
  static_fun V* RK__BST_PUB(K, V, add)(Bst(K, V) * self, K key, V val) {                           \
    return RK__BST_PRI(K, V, set_add)(false, self, key, val);                                      \
  }                                                                                                \
  static_fun bool RK__BST_PUB(K, V, extract)(Bst(K, V) * self, K key, V * val_out) {               \
    rk_assert_ptr_nonnull(val_out);                                                                \
    typedef struct RK__BstNode(K, V) node_t;                                                       \
    node_t** lnk = RK__BST_PUB(K, V, search_ptr)(self, key);                                       \
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
  static_fun bool RK__BST_PUB(K, V, remove)(Bst(K, V) * self, K key) {                             \
    V _;                                                                                           \
    return RK__BST_PUB(K, V, extract)(self, key, &_);                                              \
  }                                                                                                \
  RK_EXTERNC_END

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL
#define RK__BstEntryPriv(K, V)             RK__bst_entry_##K##_##V

#define RK__BST_PUB(K, V, FNAME)           bstf_##FNAME##_##K##_##V
#define RK__BST_PRI(K, V, FNAME)           RK__bst_##FNAME##_##K##_##V

#define RK__BstNode(K, V)                  RK__bst_node_##K##_##V

#define RK__bst_init_static(K, V, _Alloc)  {.count = 0, RK_IFALLOC(.alloc = _Alloc)}

#define RK__bst_init_static3(K, V, _Alloc) rk_disable_if(RK__bst_init_static(K, V, _Alloc))
#define RK__bst_init_static2(K, V)         RK__bst_init_static(K, V, alloc_ctx)

#define RK__bst_init(K, V, _Alloc)         ((Bst(K, V))RK__bst_init_static(K, V, _Alloc))
#define RK__bst_init3(K, V, _Alloc)        rk_disable_if(RK__bst_init(K, V, _Alloc))
#define RK__bst_init2(K, V)                RK__bst_init(K, V, alloc_ctx)

static_fun void RK__bst_release_(size_t nodesize, size_t nodealign,
                                 bst_node* restrict node RK_IFALLOC(, Allocator alloc)) {
  while (node) {
    if (node->l) {
      bst_node* l = node->l;
      node->l     = l->r;
      l->r        = node;
      node        = l;
    } else {
      bst_node* r = node->r;
      alloc_deallocate(node, nodesize, nodealign RK_IFALLOC(, alloc));
      node = r;
    }
  }
}
static_fun void RK__bst_release(size_t nodesize, size_t nodealign, bst_data* self) {
  RK__bst_release_(nodesize, nodealign, self->root RK_IFALLOC(, self->alloc));
  self->root = rk_null, self->count = 0;
}

static_fun void* RK__bst_min(bst_node* cur) {
  if (!cur) { return rk_null; }
  while (cur->l) { cur = cur->l; }
  return cur->data;
}
static_fun void* RK__bst_max(bst_node* cur) {
  if (!cur) { return rk_null; }
  while (cur->r) { cur = cur->r; }
  return cur->data;
}

static_fun bool bst_iter_next(bst_iter* restrict it, bst_node** node_out) {
  while (it->curr) {
    rk_assert(it->top < it->cap && "BST iterator stack overflow");
    it->stack[it->top++] = it->curr;
    it->curr             = it->curr->l;
  }
  if (it->top == 0) { return false; }
  bst_node* node = it->stack[--it->top];
  *node_out      = node;
  it->curr       = node->r;
  return true;
}

/// @brief Iterates over all entries in the BST in ascending key order.
///
/// Performs an in-order traversal using a caller-supplied stack buffer. The loop variable `_entry`
/// is a `const BstEntry(K, V)*` pointing to each entry in turn.
///
/// @param self          Pointer to the `Bst(K, V)` to iterate
/// @param stack_buf     Array of `bst_node*` used as the traversal stack
/// @param stack_buf_cap Number of elements in `stack_buf`; must be at least the height of the tree
/// to avoid assertion failure
/// @param _entry        Name for the loop variable (a `const BstEntry(K, V)*`)
///
/// @warning Do not insert or remove elements during iteration.
/// @warning If `stack_buf_cap` is less than the tree height, an assertion fires.
///
/// Example:
/// ```c
/// bst_node* stack[64];
/// bst_foreach(&tree, stack, 64, e) {
///     printf("%d -> %s\n", e->key, e->val);
/// }
/// ```
#define bst_foreach(self, stack_buf, stack_buf_cap, _entry)                                        \
  for (typeof(*(self))*const RK__bs = (self), *RK__ONCE = RK__bs; RK__ONCE;)                       \
    for (bst_node * RK__node; RK__ONCE; RK__ONCE = 0)                                              \
      for (bst_iter it = {.stack = (stack_buf),                                                    \
                          .curr  = (bst_node*)RK__bs->root,                                        \
                          .cap   = (stack_buf_cap),                                                \
                          .top   = 0};                                                             \
           bst_iter_next(&it, &RK__node);)                                                         \
        for (typeof(RK__bs->root->entry)*const _entry                                              \
             = &(((typeof(RK__bs->root))RK__node)->entry),                                         \
             *RK__ONCE1           = _entry;                                                        \
             RK__ONCE1; RK__ONCE1 = 0)

RK_HEADER_END
/// @}
#endif

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
