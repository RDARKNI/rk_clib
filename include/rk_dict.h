// SPDX-License-Identifier: MIT
/// @file rk_dict.h
/// @version 2.0
/// @defgroup rk_dict Hash Table (Dict) and Set Interfaces
/// @brief Header-only, type-generic open-addressing hash table and hash set, sharing a single
/// implementation via linear probing with fingerprint-accelerated lookup.
///
/// This header provides two data structures built on the same open-addressing hash table engine:
///
/// - **Dict(K, V)** — a key-to-value map (hash table).
/// - **Set(K)** — a key-only collection (hash set); the `vals` array is omitted entirely.
///
/// Both are type-generic and instantiated with a single macro call (`DICT_DEFINE` / `SET_DEFINE`).
/// Collisions are resolved via linear probing. A separate byte array stores a 7-bit fingerprint
/// (high bits of the hash) per slot, allowing most non-matching slots to be skipped without a full
/// key comparison. Deleted slots are marked as tombstones; a dedicated counter tracks their
/// accumulation and triggers a grow-or-compact rehash when the combined live+tombstone load exceeds
/// the configured threshold.
///
/// Key Features:
/// - Linear probing with 7-bit fingerprints to minimise key comparisons.
/// - Tombstone deletion with automatic compaction to prevent probe degradation.
/// - Header-only; no external linking required.
/// - Type-generic via macros, supporting custom key/value types.
/// - Custom allocator support for flexible memory management.
/// - Automatic resize/compact when the load factor exceeds a configurable threshold (default 0.7).
/// - SOA layout (separate arrays for metadata, keys, and optionally values).
/// - Dict and Set share the same underlying implementation.
///
/// @par Dict Usage
/// 1. Define a hash function: `hash_t hash_f(K key);`
///
/// 2. Define a comparison function: `bool cmp_f(K a, K b);` (returns 0/false if equal; any scalar
///    type that converts to bool is fine)
///
/// 3. Typedef pointer/struct types if needed (required by the preprocessor):
///    ```c
///    typedef char* cstr;
///    ```
///
/// 4. Instantiate a specialised Dict:
///    ```c
///    DICT_DEFINE(int, cstr, int_hash, int_cmp);
///    Dict(int, cstr) my_dict = dict_init(int, cstr, 16, allocator);
///    ```
///
/// 5. Insert, remove, or query elements:
///    ```c
///    dict_set(int, cstr, &my_dict, 42, "Answer");
///    const char* answer = *dict_get(int, cstr, &my_dict, 42);
///    bool exists = dict_contains(int, cstr, &my_dict, 42);
///    ```
///
/// 6. Free resources when done:
///    ```c
///    dict_release(int, cstr, &my_dict);
///    ```
///
/// @par Set Usage
/// 1. Define the same hash and comparison functions as for Dict.
///
/// 2. Instantiate a specialised Set:
///    ```c
///    SET_DEFINE(int, int_hash, int_cmp);
///    Set(int) my_set = set_init(int, 16, allocator);
///    ```
///
/// 3. Insert, test membership, or remove elements:
///    ```c
///    set_add(int, &my_set, 42);
///    bool exists = set_contains(int, &my_set, 42);
///    set_remove(int, &my_set, 42);
///    ```
///
/// 4. Free resources when done:
///    ```c
///    set_release(int, &my_set);
///    ```
///
/// @par Slot Encoding (`data[]` array, one byte per slot)
/// - `0x80`: empty — slot has never been used; probe chains stop here.
/// - `0xFE`: deleted (tombstone) — slot was occupied then removed; probe chains continue through
///   it.
/// - `0x00–0x7F`: occupied — value is the 7-bit fingerprint (top 7 bits of the hash). Fingerprints
///   let the probe loop skip non-matching slots without a full key comparison.
///
/// @note On insert, if `(count + ndeleted + 1) * RK_DICT_LOAD_DEN > cap * RK_DICT_LOAD_NUM`, the
/// table doubles in capacity (when live entries are dense) or rehashes to the same capacity to
/// flush accumulated tombstones.
///
/// @see rk_alloc.h
/// @see rk_defs.h
/// @{

#ifndef RK_DICT_H
#define RK_DICT_H
#include "rk_alloc.h"
RK_HEADER_BEGIN

/// @brief Define a dict type and functions for a given key/value pair.
///
/// This macro generates a complete, type-specific hash table API for the given key-value
/// combinations.
///
/// @param key_t  Name of the key Type
/// @param val_t  Name of the value Type
/// @param hash_f Hash function (`hash_t hash_f(key_t key)`)
/// @param cmp_f  Comparison function (`bool cmp_f(key_t a, key_t b)`)
/// @attention `cmp_f` must return 0/false if the two elements are equal
#define DICT_DEFINE(key_t, val_t, hash_f, cmp_f) RK__DICT_DEF(key_t, val_t, hash_f, cmp_f)

/// @brief Generates a type-specific dict struct name.
#define Dict(K, V)                               Dict##_##K##_##V

/// @brief `Dict(K, V) dict_init(K, V, size_t cap, Allocator alloc = alloc_ctx)` - Convenience Macro
/// to create a Dict.
/// @param K           Name of the key Type
/// @param V           Name of the value Type
/// @param init_cap    size_t Initial Capacity of the Hash table
/// @param allocator   Optional allocator; defaults to `alloc_ctx`
///
/// Usage:
/// ```c
/// Dict(int, cstr) tab =  dict_init(int, cstr, 10);
/// Allocator alloc = (...);
/// Dict(int, cstr) tab =  dict_init(int, cstr, 10, alloc);
/// ```
/// @return An initialised Dict
#define dict_init(K, V, cap, ...) rk_overload(RK__DICT_INIT, K, V, cap, ##__VA_ARGS__)

/// @brief `void dict_release(K, V, Dict(K, V)* self)` - Frees the underlying memory of the Dict.
#define dict_release(K, V, self)  RK__DICT_PUB(K, V, release)(self)

/// @brief `size_t dict_count(Dict(K, V)* self)` - Returns the number of live key-value pairs stored
/// in the Dict.
#define dict_count(self)          ((size_t)((self)->count))

#if RK_CUSTOM_ALLOCATORS
# define dict_allocator(self) rk_to_rvalue((self)->alloc)
#else
# define dict_allocator(self) ((void)(self), alloc_ctx)
#endif

/// @brief `bool dict_is_empty(Dict(K, V)* self)` - Returns `true` iff the dict contains no
/// elements.
#define dict_is_empty(self)               ((bool)(dict_count(self) == 0))

/// @brief `size_t dict_cap(Dict(K, V)* self)` - Returns the current slot capacity of the Dict.
/// Always a power of two.
#define dict_cap(self)                    ((size_t)((self)->cap))

/// @brief `float dict_load_factor(Dict(K, V)* self)` - Returns the current load factor (live
/// entries / capacity). Rehash is triggered when the combined live-and-tombstone load exceeds
/// `RK_DICT_LOAD_NUM / RK_DICT_LOAD_DEN`.
#define dict_load_factor(self)            ((float)RK__ds_load_factor(&(self)->hdr))

/// @brief `Dict(K, V)* dict_clear(K, V, Dict(K, V)* self)` - Marks all slots in the Dict as free,
/// allowing reuse of its memory.
/// @return `self`, for chaining.
#define dict_clear(K, V, self)            RK__DICT_PUB(K, V, clear)(self)

/// @brief `Dict(K, V)* dict_reserve(K, V, Dict(K, V)* self, size_t new_cap)` - Reserves and
/// rehashes the Dict to ensure at least `new_cap` capacity.
/// @return `self`, for chaining.
/// @note Rounds up new_cap to the next power of two
#define dict_reserve(K, V, self, new_cap) RK__DICT_PUB(K, V, reserve)(self, new_cap)

/// @brief `bool dict_set(K, V, Dict(K, V)* self, K key, V val)` - Sets the value at `key` in the
/// Dict to `val`, updating it if it is present or inserting a new one if not; resizes the Dict if
/// necessary.
/// @return `true` if inserted, `false` if updated
#define dict_set(K, V, self, key, val)    RK__DICT_PUB(K, V, set)(self, key, val)

/// @brief `V* dict_add(K, V, Dict(K, V)* self, K key, V val)` - Inserts a value into the Dict only
/// if the key is not already present; resizes the Dict if necessary.
/// @return Pointer to the inserted value, or `NULL` if the key already existed
#define dict_add(K, V, self, key, val)    RK__DICT_PUB(K, V, add)(self, key, val)

/// @brief `V* dict_get(K, V, Dict(K, V)* self, K key)` - Retrieves a pointer to the value if the
/// key was found or `NULL` otherwise.
#define dict_get(K, V, self, key)         RK__DICT_PUB(K, V, get)(self, key)

/// @brief `V* dict_get_or_add(K, V, Dict(K, V)* self, K key, V default_value, bool* inserted_out)`
/// - Returns a pointer to the value for `key`, inserting `default_value` first if the key is
/// absent; resizes the Dict if necessary.
/// @param default_value Value to insert if `key` is not present.
/// @param inserted_out Set to `true` if a new entry was inserted, `false` if the key already
/// existed. Must not be `NULL`.
/// @return Pointer to the value for `key` (never `NULL`).
#define dict_get_or_add(K, V, self, key, default_value, inserted_out)                              \
  RK__DICT_PUB(K, V, get_or_add)(self, key, default_value, inserted_out)

/// @brief `bool dict_contains(K, V, const Dict(K, V)* self, K key)` - Checks whether the given key
/// is present in the Dict.
/// @return `true` if `self` contains the key, `false` otherwise
#define dict_contains(K, V, self, key)         RK__DICT_PUB(K, V, contains)(self, key)

/// @brief `bool dict_extract(K, V, Dict(K, V)* self, K key, V* out_ptr)` - Removes a key from the
/// Dict and stores the value in `out_ptr`.
/// @param out_ptr Pointer to where the removed value should be written if found.
/// @return `true` if key was found and removed, `false` otherwise
#define dict_extract(K, V, self, key, out_ptr) RK__DICT_PUB(K, V, extract)(self, key, out_ptr)

/// @brief `bool dict_remove(K, V, Dict(K, V)* self, K key)` - Removes a key from the Dict if it is
/// present.
/// @return `true` if the value was found and removed, `false` otherwise
#define dict_remove(K, V, self, key)           RK__DICT_PUB(K, V, remove)(self, key)

/// @brief Iterates over all key-value pairs in the Dict, skipping empty slots.
/// @param self     Pointer to the Dict to iterate over
/// @param key      Chosen name of the key pointer that will point to each key
/// @param val      Chosen name of the value pointer that will point to each key
///
/// Usage:
/// ```c
/// dict_foreach(&mydict, k, v) {
///     printf("key: %d, value: %s\n", *k, *v);
/// }
/// ```
/// @warning Adding or removing values via this macro leads to incorrect iteration.
/// @note Iteration skips empty slots in the underlying storage.
#define dict_foreach(self, _key, _val)                                                             \
  for (typeof(self) RK___dict = (self); RK___dict; RK___dict = rk_null)                            \
    for (size_t RK___c = RK___dict->cap, RK___i = 0; RK___i < RK___c; ++RK___i)                    \
      for (const typeof(*(RK___dict->keys))*const _key                                             \
           = !RK__DS_SLOT_EMPTY_OR_DELETED(RK___dict->data[RK___i]) ? &(RK___dict->keys[RK___i])   \
                                                                    : rk_null,                     \
           *RK__ONCE          = _key;                                                              \
           RK__ONCE; RK__ONCE = 0)                                                                 \
        for (typeof(*(RK___dict->vals))*const _val       = &(RK___dict->vals[RK___i]),             \
                                              *RK__ONCE1 = _val;                                   \
             RK__ONCE1; RK__ONCE1                        = 0)

/// @brief Iterates over all keys in the Dict, skipping empty and deleted slots.
/// @param self  Pointer to the Dict to iterate over
/// @param _key  Chosen name of the key pointer (`const K*`) for each iteration
///
/// Usage:
/// ```c
/// dict_foreach_key(&mydict, k) {
///     printf("key: %d\n", *k);
/// }
/// ```
/// @warning Adding or removing values during iteration leads to incorrect behaviour.
#define dict_foreach_key(self, _key)                                                               \
  for (typeof(self) RK___dict = (self); RK___dict; RK___dict = rk_null)                            \
    for (size_t RK___c = RK___dict->cap, RK___i = 0; RK___i < RK___c; ++RK___i)                    \
      for (const typeof(*(RK___dict->keys))*const _key                                             \
           = !RK__DS_SLOT_EMPTY_OR_DELETED(RK___dict->data[RK___i]) ? &(RK___dict->keys[RK___i])   \
                                                                    : rk_null,                     \
           *RK__ONCE          = _key;                                                              \
           RK__ONCE; RK__ONCE = 0)

/// @brief Iterates over all values in the Dict, skipping empty and deleted slots.
/// @param self  Pointer to the Dict to iterate over
/// @param _val  Chosen name of the value pointer (`V*`) for each iteration
///
/// Usage:
/// ```c
/// dict_foreach_val(&mydict, v) {
///     printf("value: %s\n", *v);
/// }
/// ```
/// @warning Adding or removing values during iteration leads to incorrect behaviour.
#define dict_foreach_val(self, _val)                                                               \
  for (typeof(self) RK___dict = (self); RK___dict; RK___dict = rk_null)                            \
    for (size_t RK___c = RK___dict->cap, RK___i = 0; RK___i < RK___c; ++RK___i)                    \
      for (typeof(*(RK___dict->vals))*const _val                                                   \
           = !RK__DS_SLOT_EMPTY_OR_DELETED(RK___dict->data[RK___i]) ? &(RK___dict->vals[RK___i])   \
                                                                    : rk_null,                     \
           *RK__ONCE          = _val;                                                              \
           RK__ONCE; RK__ONCE = 0)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @name Set Interface
////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Define a Set type and functions for a given key type.
/// @param key_t  Name of the key Type
/// @param hash_f Hash function (`hash_t hash_f(key_t key)`)
/// @param cmp_f  Comparison function (`bool cmp_f(key_t a, key_t b)`)
/// @attention `cmp_f` must return 0/false if the two elements are equal
#define SET_DEFINE(key_t, hash_f, cmp_f) RK__SET_DEF(key_t, hash_f, cmp_f)

/// @brief Generates a type-specific set struct name.
#define Set(K)                           Set##_##K

/// @brief `Set(K) set_init(K, size_t cap, Allocator alloc = alloc_ctx)` - Creates a Set.
/// @param K           Name of the key Type
/// @param init_cap    size_t Initial Capacity of the Hash table
/// @param allocator   Optional allocator; defaults to `alloc_ctx`
///
/// Usage:
/// ```c
/// Set(int) tab = set_init(int, 10);
/// Allocator alloc = (...);
/// Set(int) tab = set_init(int, 10, alloc);
/// ```
/// @return An initialised Set
#define set_init(K, cap, ...)            rk_overload(RK__SET_INIT, K, cap, ##__VA_ARGS__)

/// @brief `void set_release(K, Set(K)* self)` - Frees the underlying memory of the Set.
#define set_release(K, self)             RK__SET_PUB(K, release)(self)

/// @brief `size_t set_count(Set(K)* self)` - Returns the number of live keys in the Set.
#define set_count(self)                  ((size_t)((self)->count))

#if RK_CUSTOM_ALLOCATORS
# define set_allocator(self) rk_to_rvalue((self)->alloc)
#else
# define set_allocator(self) ((void)(self), alloc_ctx)
#endif

/// @brief `bool set_is_empty(Set(K)* self)` - Returns `true` iff the set contains no elements.
#define set_is_empty(self)            ((bool)(set_count(self) == 0))

/// @brief `size_t set_cap(Set(K)* self)` - Returns the current slot capacity of the Set. Always a
/// power of two.
#define set_cap(self)                 ((size_t)((self)->cap))

/// @brief `float set_load_factor(Set(K)* self)` - Returns the current load factor (live entries /
/// capacity). Rehash is triggered when the combined live-and-tombstone load exceeds
/// `RK_DICT_LOAD_NUM / RK_DICT_LOAD_DEN`.
#define set_load_factor(self)         ((float)RK__ds_load_factor(&(self)->hdr))

/// @brief `Set(K)* set_clear(K, Set(K)* self)` - Marks all slots in the Set as free, allowing reuse
/// of its memory.
/// @return `self`, for chaining.
#define set_clear(K, self)            RK__SET_PUB(K, clear)(self)

/// @brief `Set(K)* set_reserve(K, Set(K)* self, size_t new_cap)` - Reserves and rehashes the Set to
/// ensure at least `new_cap` capacity.
/// @return `self`, for chaining.
/// @note Rounds up new_cap to the next power of two
#define set_reserve(K, self, new_cap) RK__SET_PUB(K, reserve)(self, new_cap)

/// @brief `bool set_add(K, Set(K)* self, K key)` - Ensures a key is present in a set; resizes the
/// Set if necessary.
/// @return `true` if the key was inserted, `false` if it was already present.
#define set_add(K, self, key)         RK__SET_PUB(K, add)(self, key)

/// @brief `bool set_contains(K, const Set(K)* self, K key)`
/// - Checks whether the given key is present in the Set.
/// @return `true` if `self` contains the key, `false` otherwise
#define set_contains(K, self, key)    RK__SET_PUB(K, contains)(self, key)

/// @brief `bool set_remove(K, Set(K)* self, K key)` - Removes a key from the Set if it is present.
/// @return `true` if the value was found and removed, `false` otherwise
#define set_remove(K, self, key)      RK__SET_PUB(K, remove)(self, key)

/// @brief Iterates over all keys in the Set, skipping empty slots.
/// @param self     Pointer to the Set to iterate over
/// @param key      Chosen name of the key pointer that will point to each key
///
/// Usage:
/// ```c
/// set_foreach(&myset, k) {
///     printf("key: %d\n", *k);
/// }
/// ```
/// @warning Adding or removing keys via this macro leads to incorrect iteration.
/// @note Iteration skips empty slots in the underlying storage.
#define set_foreach(self, key)                                                                     \
  for (typeof(self) RK___set = (self); RK___set; RK___set = rk_null)                               \
    for (size_t RK___c = RK___set->cap, RK___i = 0; RK___i < RK___c; ++RK___i)                     \
      for (const typeof(*(RK___set->keys))*const key                                               \
           = !RK__DS_SLOT_EMPTY_OR_DELETED(RK___set->data[RK___i]) ? &(RK___set->keys[RK___i])     \
                                                                   : rk_null,                      \
           *RK__ONCE          = key;                                                               \
           RK__ONCE; RK__ONCE = 0)

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

typedef struct RK__ds_header { size_t cap, count, ndeleted; } RK__ds_header;

// Probe result packed into a single size_t: bits[1:0] = flags, bits[N:2] = slot index. bit 0: found
// — key exists at the returned slot. bit 1: tombstone — insert slot was a deleted slot (only
// meaningful when !found).
typedef size_t RK__hashprobe_t;
#define RK__PROBE_MAKE(found, tomb, idx)                                                           \
  (((size_t)(idx) << 2) | ((size_t)(tomb) << 1) | (size_t)(found))
#define RK__PROBE_FOUND(r)     ((r) & 1u)
#define RK__PROBE_TOMBSTONE(r) ((r) & 2u)
#define RK__PROBE_IDX(r)       ((r) >> 2)

#define RK__IGNORE(...)
#define RK__EXPAND(...)                       __VA_ARGS__

/// @brief Sentinel value returned by internal index lookups when the key is not present.
#define RK_DS_NOTIN                           ((size_t)-1)

// internal helper macros
#define RK__DS_home(MASK, hash)               ((size_t)(hash) & (MASK))
#define RK__DS_next(MASK, i)                  (((i) + 1) & (MASK))
#define RK__DS_fp(hash)                       ((hash) >> (bitsof(hash) - 7))

#define RK__DS_SLOT_EMPTY                     ((u8)0x80)
#define RK__DS_SLOT_DELETED                   ((u8)0xFE)
#define RK__DS_SLOT_EMPTY_OR_DELETED(x)       ((x) & 0x80)

#define RK__DICT_PUB(K, V, FNAME)             dictf_##FNAME##_##K##_##V
#define RK__DICT_PRI(K, V, FNAME)             RK__dict##_##K##_##V##_##FNAME

#define RK__DICT_INIT(K, V, init_cap, alloc)  RK__DICT_PUB(K, V, init)(init_cap RK_IFALLOC(, alloc))
#define RK__DICT_INIT4(K, V, init_cap, alloc) rk_disable_if(RK__DICT_INIT(K, V, init_cap, alloc))
#define RK__DICT_INIT3(K, V, init_cap)        RK__DICT_INIT(K, V, init_cap, alloc_ctx)

#define RK__SET_INIT(K, init_cap, alloc)      RK__SET_PUB(K, init)(init_cap RK_IFALLOC(, alloc))
#define RK__SET_INIT3(K, init_cap, alloc)     rk_disable_if(RK__SET_INIT(K, init_cap, alloc))
#define RK__SET_INIT2(K, init_cap)            RK__SET_INIT(K, init_cap, alloc_ctx)

#define RK__SET_PUB(K, FNAME)                 setf_##FNAME##_##K
#define RK__SET_PRI(K, FNAME)                 RK__set##_##K##_##FNAME

#define RK__SET_PUB_I(K, V, FNAME)            RK__SET_PUB(K, FNAME)
#define RK__SET_PRI_I(K, V, FNAME)            RK__SET_PRI(K, FNAME)
#define RK__SET(K, V)                         Set(K)

#define RK__DICT_DEF(key_t, val_t, hash_f, cmp_f)                                                  \
  RK__DS_DEF(key_t, val_t, hash_f, cmp_f, RK__EXPAND, RK__IGNORE, Dict, RK__DICT_PUB, RK__DICT_PRI)
#define RK__SET_DEF(key_t, hash_f, cmp_f)                                                          \
  RK__DS_DEF(key_t, , hash_f, cmp_f, RK__IGNORE, RK__EXPAND, RK__SET, RK__SET_PUB_I, RK__SET_PRI_I)
#define RK__DS_DEF(K, V, hash_f, cmp_f, IF_DICT, IF_SET, DSTYPE, PUBF, PRIF)                       \
  RK_EXTERNC_BEG                                                                                   \
  typedef struct DSTYPE(K, V) {                                                                    \
    union {                                                                                        \
      RK__ds_header hdr;                                                                           \
      struct { size_t cap, count, ndeleted; };                                                     \
    };                                                                                             \
    u8* data;                                                                                      \
    K*  keys;                                                                                      \
    IF_DICT(V* vals;)                                                                              \
    RK_IFALLOC(Allocator alloc;)                                                                   \
  } DSTYPE(K, V);                                                                                  \
  static_fun DSTYPE(K, V) PUBF(K, V, init)(size_t cap RK_IFALLOC(, Allocator alloc)) {             \
    rk_assert_allocator_valid(alloc);                                                              \
    cap = stdc_bit_ceil(rk_MAX(16u, cap));                                                         \
    return (DSTYPE(K, V)){.hdr  = {.cap = cap, .count = 0, .ndeleted = 0},                         \
                          .data = (u8*)memset(alloc_allocate(cap, align_max RK_IFALLOC(, alloc)),  \
                                              RK__DS_SLOT_EMPTY, cap),                             \
                          .keys = alloc_new(K, cap RK_IFALLOC(, alloc)),                           \
                          IF_DICT(.vals = alloc_new(V, cap RK_IFALLOC(, alloc)), )                 \
                              RK_IFALLOC(.alloc = alloc)};                                         \
  }                                                                                                \
  static_fun void PUBF(K, V, release)(DSTYPE(K, V) * self) {                                       \
    if rk_unlikely (!self->cap) { return; }                                                        \
    alloc_deallocate(self->data, self->cap, align_max RK_IFALLOC(, self->alloc));                  \
    self->data = rk_null;                                                                          \
    alloc_delete(self->keys, self->cap RK_IFALLOC(, self->alloc));                                 \
    self->keys = rk_null;                                                                          \
    IF_DICT(alloc_delete(self->vals, self->cap RK_IFALLOC(, self->alloc)), self->vals = rk_null;)  \
    self->cap = self->count = self->ndeleted = 0;                                                  \
  }                                                                                                \
  static_fun void PRIF(K, V, grow)(DSTYPE(K, V) * self, size_t new_cap) {                          \
    const DSTYPE(K, V) old_self = *self;                                                           \
    DSTYPE(K, V)                                                                                   \
    new_self                                                                                       \
        = {.hdr  = {.cap = new_cap, .count = old_self.count, .ndeleted = 0},                       \
           .data = (u8*)memset(alloc_allocate(new_cap, align_max RK_IFALLOC(, old_self.alloc)),    \
                               RK__DS_SLOT_EMPTY, new_cap),                                        \
           .keys = alloc_new(K, new_cap RK_IFALLOC(, old_self.alloc)),                             \
           IF_DICT(.vals = alloc_new(V, new_cap RK_IFALLOC(, old_self.alloc)), )                   \
               RK_IFALLOC(.alloc = old_self.alloc)};                                               \
    const size_t mask = new_self.cap - 1;                                                          \
    for (size_t oldcap = old_self.cap, i = 0; i < oldcap; ++i) {                                   \
      if (RK__DS_SLOT_EMPTY_OR_DELETED(old_self.data[i])) { continue; }                            \
      K      key  = old_self.keys[i];                                                              \
      u64    hash = (u64)hash_f(key);                                                              \
      size_t j    = RK__DS_home(mask, hash);                                                       \
      for (; new_self.data[j] != RK__DS_SLOT_EMPTY; j = RK__DS_next(mask, j));                     \
      new_self.data[j] = RK__DS_fp(hash);                                                          \
      new_self.keys[j] = key;                                                                      \
      IF_DICT(new_self.vals[j] = old_self.vals[i];)                                                \
    }                                                                                              \
    PUBF(K, V, release)(self);                                                                     \
    *self = new_self;                                                                              \
  }                                                                                                \
  static_fun void PRIF(K, V, ensure_cap)(DSTYPE(K, V) * self) {                                    \
    if (!self->cap) { *self = PUBF(K, V, init)(16u RK_IFALLOC(, alloc_ctx)); };                    \
    if (RK__ds_needs_rehash(&self->hdr)) {                                                         \
      PRIF(K, V, grow)(self, (self->count * 2 > self->cap) ? self->cap * 2 : self->cap);           \
    }                                                                                              \
  }                                                                                                \
  static_fun RK__hashprobe_t PRIF(K, V, probe_f)(const DSTYPE(K, V)* restrict self, K key,         \
                                                 u64 hash) {                                       \
    u8           fp   = RK__DS_fp(hash);                                                           \
    const size_t mask = self->cap - 1;                                                             \
    size_t       i = RK__DS_home(mask, hash), fd = RK_DS_NOTIN;                                    \
    u8* const restrict data = self->data;                                                          \
    K* const restrict keys  = self->keys;                                                          \
    for (; data[i] != RK__DS_SLOT_EMPTY; i = RK__DS_next(mask, i)) {                               \
      if (data[i] == RK__DS_SLOT_DELETED) {                                                        \
        if (fd == RK_DS_NOTIN) { fd = i; }                                                         \
      } else if (data[i] == fp && !cmp_f(key, keys[i])) {                                          \
        return RK__PROBE_MAKE(1, 0, i);                                                            \
      }                                                                                            \
    }                                                                                              \
    return (fd != RK_DS_NOTIN) ? RK__PROBE_MAKE(0, 1, fd) : RK__PROBE_MAKE(0, 0, i);               \
  }                                                                                                \
  static_fun bool PUBF(K, V, contains)(const DSTYPE(K, V)* restrict self, K key) {                 \
    if rk_unlikely (!self->cap) { return false; }                                                  \
    return RK__PROBE_FOUND(PRIF(K, V, probe_f)(self, key, (u64)hash_f(key)));                      \
  }                                                                                                \
  static_fun DSTYPE(K, V) * PUBF(K, V, clear)(DSTYPE(K, V)* restrict self) {                       \
    rk_memset(self->data, RK__DS_SLOT_EMPTY, self->cap);                                           \
    self->count = self->ndeleted = 0;                                                              \
    return self;                                                                                   \
  }                                                                                                \
  static_fun DSTYPE(K, V) * PUBF(K, V, reserve)(DSTYPE(K, V)* restrict self, size_t cap) {         \
    if (!self->cap && cap) {                                                                       \
      RK_IFALLOC(rk_set_alloc_fallback(self->alloc);)                                              \
      *self = PUBF(K, V, init)(cap RK_IFALLOC(, self->alloc));                                     \
    } else if (cap > self->cap) {                                                                  \
      PRIF(K, V, grow)(self, stdc_bit_ceil(rk_MAX(16u, cap)));                                     \
    }                                                                                              \
    return self;                                                                                   \
  }                                                                                                \
  IF_DICT(                                                                                         \
      static_fun void PRIF(K, V, insert_f)(DSTYPE(K, V)* restrict self, K key, V val, u8 fp,       \
                                           bool used_tombstone, size_t i) {                        \
        ++self->count;                                                                             \
        if (used_tombstone) { --self->ndeleted; }                                                  \
        self->data[i] = fp;                                                                        \
        self->keys[i] = key;                                                                       \
        self->vals[i] = val;                                                                       \
      } /*                                                           */                            \
      static_fun bool PUBF(K, V, set)(DSTYPE(K, V)* restrict self, K key, V val) {                 \
        PRIF(K, V, ensure_cap)(self);                                                              \
        u64             hash = (u64)hash_f(key);                                                   \
        RK__hashprobe_t r    = PRIF(K, V, probe_f)(self, key, hash);                               \
        if (!RK__PROBE_FOUND(r)) {                                                                 \
          PRIF(K, V, insert_f)(self, key, val, RK__DS_fp(hash), RK__PROBE_TOMBSTONE(r),            \
                               RK__PROBE_IDX(r));                                                  \
        } else {                                                                                   \
          self->vals[RK__PROBE_IDX(r)] = val;                                                      \
        }                                                                                          \
        return !RK__PROBE_FOUND(r);                                                                \
      } /*                                                           */                            \
      static_fun V* PUBF(K, V, add)(DSTYPE(K, V)* restrict self, K key, V val) {                   \
        PRIF(K, V, ensure_cap)(self);                                                              \
        u64             hash = (u64)hash_f(key);                                                   \
        RK__hashprobe_t r    = PRIF(K, V, probe_f)(self, key, hash);                               \
        if (!RK__PROBE_FOUND(r)) {                                                                 \
          PRIF(K, V, insert_f)(self, key, val, RK__DS_fp(hash), RK__PROBE_TOMBSTONE(r),            \
                               RK__PROBE_IDX(r));                                                  \
          return &self->vals[RK__PROBE_IDX(r)];                                                    \
        }                                                                                          \
        return rk_null;                                                                            \
      } /*                                                           */                            \
      static_fun V* PUBF(K, V, get_or_add)(DSTYPE(K, V)* restrict self, K key, V val,              \
                                           bool* restrict inserted_out) {                          \
        rk_assert_ptr_nonnull(inserted_out);                                                       \
        PRIF(K, V, ensure_cap)(self);                                                              \
        u64             hash = (u64)hash_f(key);                                                   \
        RK__hashprobe_t r    = PRIF(K, V, probe_f)(self, key, hash);                               \
        if (!RK__PROBE_FOUND(r)) {                                                                 \
          PRIF(K, V, insert_f)(self, key, val, RK__DS_fp(hash), RK__PROBE_TOMBSTONE(r),            \
                               RK__PROBE_IDX(r));                                                  \
          *inserted_out = true;                                                                    \
        } else {                                                                                   \
          *inserted_out = false;                                                                   \
        }                                                                                          \
        return &self->vals[RK__PROBE_IDX(r)];                                                      \
      } /*                                                           */                            \
      static_fun V* PUBF(K, V, get)(const DSTYPE(K, V)* restrict self, K key) {                    \
        if rk_unlikely (!self->cap) { return rk_null; }                                            \
        RK__hashprobe_t r = PRIF(K, V, probe_f)(self, key, (u64)hash_f(key));                      \
        return RK__PROBE_FOUND(r) ? &self->vals[RK__PROBE_IDX(r)] : rk_null;                       \
      } /*                                                           */                            \
      static_fun bool PUBF(K, V, extract)(DSTYPE(K, V)* restrict self, K key, V * out_ptr) {       \
        rk_assert_ptr_nonnull(out_ptr);                                                            \
        if rk_unlikely (!self->cap) { return false; }                                              \
        RK__hashprobe_t r = PRIF(K, V, probe_f)(self, key, (u64)hash_f(key));                      \
        if (!RK__PROBE_FOUND(r)) { return false; }                                                 \
        --self->count;                                                                             \
        ++self->ndeleted;                                                                          \
        self->data[RK__PROBE_IDX(r)] = RK__DS_SLOT_DELETED;                                        \
        *out_ptr                     = self->vals[RK__PROBE_IDX(r)];                               \
        return true;                                                                               \
      } /*                                                           */                            \
      static_fun bool PUBF(K, V, remove)(DSTYPE(K, V)* restrict self, K key) {                     \
        V _;                                                                                       \
        return PUBF(K, V, extract)(self, key, &_);                                                 \
      })                                                                                           \
  IF_SET(                                                                                          \
      static_fun bool PUBF(K, V, add)(DSTYPE(K, V)* restrict self, K key) {                        \
        PRIF(K, V, ensure_cap)(self);                                                              \
        u64             hash = (u64)hash_f(key);                                                   \
        RK__hashprobe_t r    = PRIF(K, V, probe_f)(self, key, hash);                               \
        if (!RK__PROBE_FOUND(r)) {                                                                 \
          ++self->count;                                                                           \
          if (RK__PROBE_TOMBSTONE(r)) { --self->ndeleted; }                                        \
          self->data[RK__PROBE_IDX(r)] = RK__DS_fp(hash);                                          \
          self->keys[RK__PROBE_IDX(r)] = key;                                                      \
          return true;                                                                             \
        }                                                                                          \
        return false;                                                                              \
      } /*                                                           */                            \
      static_fun bool PUBF(K, V, remove)(DSTYPE(K, V)* restrict self, K key) {                     \
        if rk_unlikely (!self->cap) { return false; }                                              \
        RK__hashprobe_t r = PRIF(K, V, probe_f)(self, key, (u64)hash_f(key));                      \
        if (!RK__PROBE_FOUND(r)) { return false; }                                                 \
        --self->count;                                                                             \
        ++self->ndeleted;                                                                          \
        self->data[RK__PROBE_IDX(r)] = RK__DS_SLOT_DELETED;                                        \
        return true;                                                                               \
      })                                                                                           \
  RK_EXTERNC_END

/// @endcond

static_fun rk_pure float RK__ds_load_factor(const RK__ds_header* hdr) {
  rk_assert(hdr && "hdr must not be null");
  return hdr->cap ? (float)hdr->count / (float)hdr->cap : 0.0f;
}
static_fun rk_pure bool RK__ds_needs_rehash(const RK__ds_header* hdr) {
  return (hdr->count + hdr->ndeleted + 1) * RK_DICT_LOAD_DEN > hdr->cap * RK_DICT_LOAD_NUM;
}

RK_HEADER_END
/// @}
#endif // RK_DICT_H

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
