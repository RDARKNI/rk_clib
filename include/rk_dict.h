// SPDX-License-Identifier: MIT
/// @file rk_dict.h
/// @version 1.0.1
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
/// - Automatic resize/compact when the load factor exceeds a configurable threshold (default 0.75).
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
/// - `0x00–0x7F`: occupied — the top 7 bits of the hash, converted to u64.
/// Fingerprints skip non-matching slots without a full key comparison. For best
/// performance, supply a well-distributed 64-bit hash including its high bits.
///
/// @note Before inserting a new key, rehash if the prospective live+tombstone
/// load exceeds RK_DICT_LOAD_NUM / RK_DICT_LOAD_DEN. Grow when live entries
/// require more slots; otherwise compact at the same capacity.
///
/// @par Load-factor configuration
/// Define RK_DICT_LOAD_NUM and RK_DICT_LOAD_DEN before inclusion to select the
/// maximum live+tombstone load (default 3/4). Require positive integers with
/// numerator < denominator <= 65535. Every translation unit must use the same setting.
///
/// @par Contracts and pointer validity
/// Keys must use single-token type names (typedef multi-token types first).
/// Equality must be an equivalence relation; equal keys must have equal hashes.
/// Hash/comparison callbacks must be side-effect-free and must not modify the table.
/// Stored keys, including referenced data used by hashing/comparison, must remain unchanged.
/// Stored objects are shallow copies: removal, clear, and release do not destroy pointees.
/// Key/value types must support the raw allocation and assignment model of rk_alloc.h;
/// this is not a general-purpose container for C++ objects requiring construction/destruction.
/// A zero-initialized table is valid and allocates lazily on insertion or positive reserve.
/// A non-null table pointer is required, except foreach/erase_if accept a null table.
///
/// Lookup and operations on existing keys do not rehash. Updating an existing value
/// preserves its address and the stored key representative. Inserting a new key can
/// rehash and invalidate every element pointer. reserve/shrink_to_fit invalidate all
/// element pointers when they rehash; clear/assign/release invalidate all old entries.
/// Removal invalidates only the removed entry; retained entries keep their addresses.
/// reserve(n) guarantees room for n live entries without automatic rehash during subsequent
/// insertions, provided no intervening removals or other structural operations occur.
///
/// Allocation failure is handled by the allocator, which must succeed or not return.
/// Impossible capacities/allocation-size overflow terminate via abort(), including in
/// release builds. Optional output pointers and assign input must not overlap table storage.
///
/// @see rk_alloc.h
/// @see rk_defs.h
/// @{

#ifndef RK_DICT_H
#define RK_DICT_H
#include "rk_alloc.h"
#include <stdint.h>
#include <stdlib.h>
#ifdef __cplusplus
# include <type_traits>
#endif
RKI_HEADER_BEGIN

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
#define DICT_DEFINE(key_t, val_t, hash_f, cmp_f) RKI_DICT_DEF(key_t, val_t, hash_f, cmp_f)

/// @brief Generates a type-specific dict struct name.
#define Dict(K, V)                               Dict_##K##_##V

/// @brief `Dict(K, V) dict_init(K, V, size_t cap, Allocator alloc = alloc_ctx)` - Convenience Macro
/// to create a Dict.
/// @param K           Name of the key Type
/// @param V           Name of the value Type
/// @param cap Initial slot count, rounded up to a power of two (minimum 16).
/// Use reserve() to request a live-element capacity instead.
/// @param allocator   Optional allocator; defaults to `alloc_ctx`
///
/// Usage:
/// ```c
/// Dict(int, cstr) tab =  dict_init(int, cstr, 10);
/// Allocator alloc = (...);
/// Dict(int, cstr) tab =  dict_init(int, cstr, 10, alloc);
/// ```
/// @return An initialised Dict
#define dict_init(K, V, cap, ...)              RKI_OVERLOAD(RKI_DICT_INIT, K, V, cap, ##__VA_ARGS__)

/// @brief `void dict_release(K, V, Dict(K, V)* self)` - Frees the underlying memory of the Dict.
#define dict_release(K, V, self)               RKI_DICT_PUB(K, V, release)(self)

/// @brief `size_t dict_count(Dict(K, V)* self)` - Returns the number of live key-value pairs stored
/// in the Dict.
#define dict_count(self)                       ((size_t)((self)->count))

/// @brief `size_t dict_cap(Dict(K, V)* self)` - Returns the current slot capacity of the Dict.
/// Zero when unallocated; otherwise a power of two.
#define dict_cap(self)                         ((size_t)((self)->cap))

/// @brief `Allocator dict_allocator(Dict(K, V)* self)` - Returns the Allocator the Dict was
/// constructed with, or `alloc_ctx` if the Dict was never initialized or custom allocators are
/// disabled.
#define dict_allocator(self)                   RKI_allocatorof(self)

/// @brief `bool dict_is_empty(Dict(K, V)* self)` - Returns `true` iff the dict contains no
/// elements.
#define dict_is_empty(self)                    ((bool)(dict_count(self) == 0))

/// @brief `float dict_load_factor(Dict(K, V)* self)` - Returns the current load factor (live
/// entries / capacity). Rehash is triggered when the combined live-and-tombstone load exceeds
/// `RK_DICT_LOAD_NUM / RK_DICT_LOAD_DEN`.
#define dict_load_factor(self)                 ((float)rki_ds_load_factor(&(self)->hdr))

/// @brief `Dict(K, V)* dict_clear(K, V, Dict(K, V)* self)` - Marks all slots in the Dict as free,
/// allowing reuse of its memory.
/// @return `self`, for chaining.
#define dict_clear(K, V, self)                 RKI_DICT_PUB(K, V, clear)(self)

/// @brief `Dict(K, V)* dict_reserve(K, V, Dict(K, V)* self, size_t n)` - Reserves and rehashes the
/// Dict so that it can hold at least `n` live entries without triggering another automatic rehash.
/// @return `self`, for chaining.
/// @note `n` counts live entries, not table slots — the underlying table capacity (see
/// `dict_cap`) is sized up to account for the load factor
/// (`RK_DICT_LOAD_NUM`/`RK_DICT_LOAD_DEN`).
#define dict_reserve(K, V, self, n)            RKI_DICT_PUB(K, V, reserve)(self, n)

/// @brief `Dict(K, V)* dict_shrink_to_fit(K, V, Dict(K, V)* self)` - Rehashes the Dict down to the
/// smallest table capacity that still keeps its live entries under the load factor threshold
/// (`RK_DICT_LOAD_NUM`/`RK_DICT_LOAD_DEN`), also clearing any accumulated tombstones.
/// @return `self`, for chaining.
/// @note Frees the table entirely if empty. A no-op only if capacity already equals
/// the target and there are no tombstones.
#define dict_shrink_to_fit(K, V, self)         RKI_DICT_PUB(K, V, shrink_to_fit)(self)

/// @brief `Dict(K, V)* dict_assign(K, V, Dict(K, V)* self, const K* keys, const V* vals, size_t n)`
/// - Replaces the Dict's contents with `n` key-value pairs from the parallel `keys`/`vals` arrays,
/// reusing the existing table (growing it if necessary) rather than allocating a new one.
/// @note Input arrays must not overlap the table object or any of its allocations.
/// Duplicate keys retain the first key representative and the last value.
/// @return `self`, for chaining.
#define dict_assign(K, V, self, keys, vals, n) RKI_DICT_PUB(K, V, assign)(self, keys, vals, n)

/// @brief Retrieves the value for `key`, or `NULL` if absent. Returns `V*` for a mutable Dict
/// and `const V*` for a const Dict.
/// @see dict_at
#ifdef __cplusplus
# define dict_get(K, V, self, key)                                                                 \
   ((typename std::conditional<std::is_const<typeof(*(self))>::value, const V*, V*>::type)         \
        RKI_DICT_PUB(K, V, get_const)(self, key))
#else
# define dict_get(K, V, self, key)                                                                 \
   _Generic((self),                                                                                \
       const Dict(K, V)*: RKI_DICT_PUB(K, V, get_const),                                           \
       Dict(K, V)*: RKI_DICT_PUB(K, V, get))((self), (key))
#endif

/// @brief Returns the value for `key` as an lvalue, mutable for a mutable Dict and const for a
/// const Dict.
/// @pre `key` is present; use `dict_get()` to check safely.
/// @note The precondition is asserted in debug builds; invalid access is undefined in release.
/// @note Never inserts, unlike `dict_get_or_add()`. Invalidated by any insertion that rehashes,
/// and by removal of `key`.
/// @see dict_get
#ifdef __cplusplus
# define dict_at(K, V, self, key)                                                                  \
   (*(typename std::conditional<std::is_const<typeof(*(self))>::value, const V*, V*>::type)        \
         RKI_DICT_PUB(K, V, at_const)(self, key))
#else
# define dict_at(K, V, self, key)                                                                  \
   (*_Generic((self),                                                                              \
        const Dict(K, V)*: RKI_DICT_PUB(K, V, at_const),                                           \
        Dict(K, V)*: RKI_DICT_PUB(K, V, at))((self), (key)))
#endif

/// @brief `bool dict_contains(K, V, const Dict(K, V)* self, K key)` - Checks whether the given key
/// is present in the Dict.
/// @return `true` if `self` contains the key, `false` otherwise
#define dict_contains(K, V, self, key) RKI_DICT_PUB(K, V, contains)(self, key)

/// @brief `bool dict_set(K, V, Dict(K, V)* self, K key, V val)` - Sets the value at `key` in the
/// Dict to `val`, updating it if it is present or inserting a new one if not; resizes the Dict if
/// necessary.
/// @return `true` if inserted, `false` if updated
#define dict_set(K, V, self, key, val) RKI_DICT_PUB(K, V, set)(self, key, val)

/// @brief `V* dict_add(K, V, Dict(K, V)* self, K key, V val)` - Inserts a value into the Dict only
/// if the key is not already present; resizes the Dict if necessary.
/// @return Pointer to the inserted value, or `NULL` if the key already existed
#define dict_add(K, V, self, key, val) RKI_DICT_PUB(K, V, add)(self, key, val)

/// @brief `V* dict_get_or_add(K, V, Dict(K, V)* self, K key, V default_value, bool* inserted_out)`
/// - Returns a pointer to the value associated with `key`, inserting `default_value` first if the
/// key is absent. Resizes the Dict if necessary.
/// @param default_value Value to insert if `key` is absent.
/// @param inserted_out Optional output set to `true` if a new entry was inserted or `false` if the
/// key already existed. May be `NULL`; otherwise must be disjoint from the table
/// and its allocations.
/// @return Pointer to the value associated with `key`; never `NULL`.
#define dict_get_or_add(K, V, self, key, default_value, inserted_out)                              \
  RKI_DICT_PUB(K, V, get_or_add)(self, key, default_value, inserted_out)

/// @brief `bool dict_extract(K, V, Dict(K, V)* self, K key, V* out_ptr)` - Removes a key from the
/// Dict and stores the value in `out_ptr`.
/// @param out_ptr Non-null output pointer, disjoint from the table and its allocations.
/// Written only if the key is found.
/// @return `true` if key was found and removed, `false` otherwise
#define dict_extract(K, V, self, key, out_ptr) RKI_DICT_PUB(K, V, extract)(self, key, out_ptr)

/// @brief `bool dict_remove(K, V, Dict(K, V)* self, K key)` - Removes a key from the Dict if it is
/// present.
/// @return `true` if the value was found and removed, `false` otherwise
#define dict_remove(K, V, self, key)           RKI_DICT_PUB(K, V, remove)(self, key)

/// @brief Visits every live key-value pair in unspecified slot order.
/// @param self Pointer to the Dict. Evaluated once.
/// @param _key Name of the key pointer (const K*).
/// @param _val Name of the value pointer (V*, or const V* for a const Dict).
/// @note break stops traversal; continue advances to the next live entry.
/// @note Reassigning iterator pointers does not change traversal.
/// @note Do not structurally modify the Dict during traversal.
///
/// Usage:
/// ```c
/// dict_foreach(&mydict, k, v) {
///     printf("key: %d, value: %s\n", *k, *v);
/// }
/// ```
#define dict_foreach(self, _key, _val)         RKI_DICT_FOREACH(self, _key, _val)

/// @brief Visits every live key in unspecified slot order.
/// @param self Pointer to the Dict or Set. Evaluated once.
/// @param _key Name of the key pointer (const K*).
/// @note break stops traversal; continue advances to the next live entry.
/// @note Reassigning the iterator pointer does not change traversal.
/// @note Do not structurally modify the table during traversal.
///
/// Usage:
/// ```c
/// dict_foreach_key(&mydict, k) { printf("key: %d\n", *k); }
/// ```
#define dict_foreach_key(self, _key)           RKI_DICT_FOREACH_KEY(self, _key)

/// @brief Visits every live value in unspecified slot order.
/// @param self Pointer to the Dict. Evaluated once.
/// @param _val Name of the value pointer (V*, or const V* for a const Dict).
/// @note break stops traversal; continue advances to the next live entry.
/// @note Reassigning the iterator pointer does not change traversal.
/// @note Do not structurally modify the Dict during traversal.
///
/// Usage:
/// ```c
/// dict_foreach_val(&mydict, v) { printf("value: %s\n", *v); }
/// ```
#define dict_foreach_val(self, _val)           RKI_DICT_FOREACH_VAL(self, _val)

/// @brief Erases live entries satisfying pred, without rehashing or changing capacity.
/// @param self Pointer to a mutable Dict. Evaluated once.
/// @param _key Name of the read-only key pointer.
/// @param _val Name of the read-only value pointer.
/// @param pred Predicate expression, evaluated once per original live entry.
/// @note The predicate must not structurally modify the Dict.
/// @note Removed slots become tombstones; retained entries keep their addresses.
///
/// Usage:
/// ```c
/// dict_erase_if(&mydict, k, v, *v == 0);
/// ```
#define dict_erase_if(self, _key, _val, pred)  RKI_DICT_ERASE_IF(self, _key, _val, pred)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @name Set Interface
////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Define a Set type and functions for a given key type.
/// @param key_t  Name of the key Type
/// @param hash_f Hash function (`hash_t hash_f(key_t key)`)
/// @param cmp_f  Comparison function (`bool cmp_f(key_t a, key_t b)`)
/// @attention `cmp_f` must return 0/false if the two elements are equal
#define SET_DEFINE(key_t, hash_f, cmp_f)       RKI_SET_DEF(key_t, hash_f, cmp_f)

/// @brief Generates a type-specific set struct name.
#define Set(K)                                 Set_##K

/// @brief `Set(K) set_init(K, size_t cap, Allocator alloc = alloc_ctx)` - Creates a Set.
/// @param K           Name of the key Type
/// @param cap Initial slot count, rounded up to a power of two (minimum 16).
/// Use reserve() to request a live-element capacity instead.
/// @param allocator   Optional allocator; defaults to `alloc_ctx`
///
/// Usage:
/// ```c
/// Set(int) tab = set_init(int, 10);
/// Allocator alloc = (...);
/// Set(int) tab = set_init(int, 10, alloc);
/// ```
/// @return An initialised Set
#define set_init(K, cap, ...)                  RKI_OVERLOAD(RKI_SET_INIT, K, cap, ##__VA_ARGS__)

/// @brief `void set_release(K, Set(K)* self)` - Frees the underlying memory of the Set.
#define set_release(K, self)                   RKI_SET_PUB(K, release)(self)

/// @brief `size_t set_count(Set(K)* self)` - Returns the number of live keys in the Set.
#define set_count(self)                        ((size_t)((self)->count))

/// @brief `size_t set_cap(Set(K)* self)` - Returns the current slot capacity of the Set. Always a
/// power of two when nonzero; zero when unallocated.
#define set_cap(self)                          ((size_t)((self)->cap))

/// @brief `Allocator set_allocator(Set(K)* self)` - Returns the Allocator the Set was constructed
/// with, or `alloc_ctx` if the Set was never initialized or custom allocators are disabled.
#define set_allocator(self)                    RKI_allocatorof(self)

/// @brief `bool set_is_empty(Set(K)* self)` - Returns `true` iff the set contains no elements.
#define set_is_empty(self)                     ((bool)(set_count(self) == 0))

/// @brief `float set_load_factor(Set(K)* self)` - Returns the current load factor (live entries /
/// capacity). See `dict_load_factor()`.
#define set_load_factor(self)                  ((float)rki_ds_load_factor(&(self)->hdr))

/// @brief `Set(K)* set_clear(K, Set(K)* self)` - Marks all slots in the Set as free, allowing reuse
/// of its memory.
/// @return `self`, for chaining.
#define set_clear(K, self)                     RKI_SET_PUB(K, clear)(self)

/// @brief `Set(K)* set_reserve(K, Set(K)* self, size_t n)` - Reserves and rehashes the Set so that
/// it can hold at least `n` live entries without triggering another automatic rehash. See
/// `dict_reserve()`.
/// @return `self`, for chaining.
#define set_reserve(K, self, n)                RKI_SET_PUB(K, reserve)(self, n)

/// @brief `Set(K)* set_shrink_to_fit(K, Set(K)* self)` - Rehashes the Set down to the smallest
/// table capacity that still keeps its live entries under the load factor threshold. See
/// `dict_shrink_to_fit()`.
/// @return `self`, for chaining.
#define set_shrink_to_fit(K, self)             RKI_SET_PUB(K, shrink_to_fit)(self)

/// @brief `Set(K)* set_assign(K, Set(K)* self, const K* keys, size_t n)` - Replaces the Set's
/// contents with `n` keys from `keys`, reusing the existing table (growing it if necessary) rather
/// than allocating a new one.
/// @note Input must not overlap the table object or any of its allocations.
/// Duplicate keys retain the first key representative.
/// @return `self`, for chaining.
#define set_assign(K, self, keys, n)           RKI_SET_PUB(K, assign)(self, keys, n)

/// @brief `bool set_contains(K, const Set(K)* self, K key)`
/// - Checks whether the given key is present in the Set.
/// @return `true` if `self` contains the key, `false` otherwise
#define set_contains(K, self, key)             RKI_SET_PUB(K, contains)(self, key)

/// @brief `bool set_add(K, Set(K)* self, K key)` - Ensures a key is present in a set; resizes the
/// Set if necessary.
/// @return `true` if the key was inserted, `false` if it was already present.
#define set_add(K, self, key)                  RKI_SET_PUB(K, add)(self, key)

/// @brief `const K* set_get(K, const Set(K)* self, K key)` - Retrieves the stored
/// key representative, or NULL if absent. The returned key must not be modified.
/// @see set_at
#define set_get(K, self, key)                  RKI_SET_PUB(K, get)(self, key)

/// @brief `const K set_at(K, const Set(K)* self, K key)` - Returns the stored key representative
/// for `key` as a const lvalue. Always const: modifying a stored key would break its hash slot.
/// @pre `key` is present; use `set_get()` to check safely.
/// @note The precondition is asserted in debug builds; invalid access is undefined in release.
/// @see set_get
#define set_at(K, self, key)                   (*RKI_SET_PUB(K, at)(self, key))

/// @brief `bool set_extract(K, Set(K)* self, K key, K* out_ptr)` - Removes a key
/// and copies its stored representative to out_ptr. Returns whether it was found.
/// @param out_ptr Non-null output pointer disjoint from table storage; unchanged on absence.
#define set_extract(K, self, key, out_ptr)     RKI_SET_PUB(K, extract)(self, key, out_ptr)

/// @brief `bool set_remove(K, Set(K)* self, K key)` - Removes a key from the Set if it is present.
/// @return `true` if the key was found and removed, `false` otherwise
#define set_remove(K, self, key)               RKI_SET_PUB(K, remove)(self, key)

/// @brief Visits every live key in unspecified slot order.
/// @param self Pointer to the Set. Evaluated once.
/// @param key Name of the read-only key pointer.
/// @note break stops traversal; continue advances to the next live key.
/// @note Do not structurally modify the Set during traversal.
///
/// Usage:
/// ```c
/// set_foreach(&myset, k) { printf("key: %d\n", *k); }
/// ```
#define set_foreach(self, key)                 dict_foreach_key(self, key)

/// @brief Erases live keys satisfying pred, without rehashing or changing capacity.
/// @param self Pointer to a mutable Set. Evaluated once.
/// @param key Name of the read-only key pointer.
/// @param pred Predicate expression, evaluated once per original live key.
/// @note The predicate must not structurally modify the Set.
/// @note Removed slots become tombstones; retained keys keep their addresses.
///
/// Usage:
/// ```c
/// set_erase_if(&myset, k, *k % 2 != 0);
/// ```
#define set_erase_if(self, key, pred)          RKI_SET_ERASE_IF(self, key, pred)

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

/// Configure before inclusion. Both must be positive integers, numerator < denominator,
/// and denominator <= 65535. The default maximum load is 3/4.
#ifndef RK_DICT_LOAD_NUM
# define RK_DICT_LOAD_NUM 3
#endif
#ifndef RK_DICT_LOAD_DEN
# define RK_DICT_LOAD_DEN 4
#endif
#if RK_DICT_LOAD_NUM <= 0 || RK_DICT_LOAD_NUM >= RK_DICT_LOAD_DEN || RK_DICT_LOAD_DEN > 65535
# error "Require 0 < RK_DICT_LOAD_NUM < RK_DICT_LOAD_DEN <= 65535"
#endif

// `vals` remains a mutable pointer when the Dict object is const. Propagate the
// container's constness to the pointers exposed during iteration.
// The C version detects constness through `count`, a member of a named type: matching on
// `const typeof(*(self))*` would spell `const const Dict` for a const Dict (MSVC C4114).
#ifdef __cplusplus
# define RKI_DICT_VALUE_PTR(self)                                                                  \
   ((typename std::conditional<std::is_const<typeof(*(self))>::value,                              \
                               const typeof((self)->vals[0])*, typeof((self)->vals)>::type)0)
#else
# define RKI_DICT_VALUE_PTR(self)                                                                  \
   _Generic(&(self)->count,                                                                        \
       const size_t*: (const typeof((self)->vals[0])*)0,                                           \
       default: (typeof((self)->vals))0)
#endif

typedef struct RKI_ds_header { size_t cap, count, ndeleted; } RKI_ds_header;

// Explicit fields avoid truncating large slot indices.
typedef struct RKI_hashprobe_t {
  size_t idx;
  bool   found, tombstone;
} RKI_hashprobe_t;
rklib_fun RKI_hashprobe_t rki_ds_probe_result(bool found, bool tombstone, size_t idx) {
  RKI_hashprobe_t r = {idx, found, tombstone};
  return r;
}
#define RKI_PROBE_MAKE(found, tomb, idx) rki_ds_probe_result((found), (tomb), (idx))
#define RKI_PROBE_FOUND(r)               ((r).found)
#define RKI_PROBE_TOMBSTONE(r)           ((r).tombstone)
#define RKI_PROBE_IDX(r)                 ((r).idx)

#define RKI_IGNORE(...)
#define RKI_EXPAND(...)                 __VA_ARGS__

/// @brief Sentinel value returned by internal index lookups when the key is not present.
#define RK_DS_NOTIN                     ((size_t)-1)

// internal helper macros
#define RKI_DS_HOME(MASK, hash)         ((size_t)(hash) & (MASK))
#define RKI_DS_NEXT(MASK, i)            (((i) + 1) & (MASK))
// The shift always yields a 7-bit value (0-127), which always fits in u8 --
// every call site assigns straight into a u8, so the cast belongs here once
// rather than at each site.
#define RKI_DS_FP(hash)                 ((u8)((hash) >> (bitsof(hash) - 7)))

#define RKI_DS_SLOT_EMPTY               ((u8)0x80)
#define RKI_DS_SLOT_DELETED             ((u8)0xFE)
#define RKI_DS_SLOT_EMPTY_OR_DELETED(x) ((x) & 0x80)

// Capture the table once and snapshot capacity for traversal.
#define RKI_DS_FOREACH_STATE(self)                                                                 \
  for (struct {                                                                                    \
         typeof(*(self))* table;                                                                   \
         size_t           idx, cap;                                                                \
         int              completed;                                                               \
       } rki_var_state = {(self), 0, 0, 0};                                                        \
       rki_var_state.table && (rki_var_state.cap = rki_var_state.table->cap, 1);                   \
       rki_var_state.table = rk_null)

// Inclusive search: cap is the sentinel and is safe for cap == 0, data == NULL.
rklib_fun rk_pure size_t rki_ds_next_live(const u8* data, size_t cap, size_t idx) {
  while (idx < cap && RKI_DS_SLOT_EMPTY_OR_DELETED(data[idx])) { ++idx; }
  return idx;
}

#define RKI_DICT_FOREACH(self, _key, _val)                                                         \
  RKI_DS_FOREACH_STATE(self)                                                                       \
  for (const typeof(*rki_var_state.table->keys)* _key = rk_null;                                   \
       (rki_var_state.idx                                                                          \
        = rki_ds_next_live(rki_var_state.table->data, rki_var_state.cap, rki_var_state.idx))       \
           < rki_var_state.cap                                                                     \
       && (_key                   = &rki_var_state.table->keys[rki_var_state.idx], (void)_key,     \
          rki_var_state.completed = 0, 1);                                                         \
       rki_var_state.idx = rki_var_state.completed ? rki_var_state.idx + 1 : rki_var_state.cap)    \
    for (typeof(*RKI_DICT_VALUE_PTR(rki_var_state.table))* _val                                    \
         = &rki_var_state.table->vals[rki_var_state.idx];                                          \
         ((void)_val, !rki_var_state.completed); rki_var_state.completed = 1)

#define RKI_DICT_FOREACH_KEY(self, _key)                                                           \
  RKI_DS_FOREACH_STATE(self)                                                                       \
  for (const typeof(*rki_var_state.table->keys)* _key = rk_null;                                   \
       (rki_var_state.idx                                                                          \
        = rki_ds_next_live(rki_var_state.table->data, rki_var_state.cap, rki_var_state.idx))       \
           < rki_var_state.cap                                                                     \
       && (_key = &rki_var_state.table->keys[rki_var_state.idx], (void)_key, 1);                   \
       ++rki_var_state.idx)

#define RKI_DICT_FOREACH_VAL(self, _val)                                                           \
  RKI_DS_FOREACH_STATE(self)                                                                       \
  for (typeof(*RKI_DICT_VALUE_PTR(rki_var_state.table))* _val = rk_null;                           \
       (rki_var_state.idx                                                                          \
        = rki_ds_next_live(rki_var_state.table->data, rki_var_state.cap, rki_var_state.idx))       \
           < rki_var_state.cap                                                                     \
       && (_val = &rki_var_state.table->vals[rki_var_state.idx], (void)_val, 1);                   \
       ++rki_var_state.idx)

#define RKI_DICT_ERASE_IF(self, _key, _val, pred)                                                  \
  do {                                                                                             \
    typeof(*(self))* const rki_var_dict = (self);                                                  \
    if (!rki_var_dict) { break; }                                                                  \
    const size_t rki_var_cap = rki_var_dict->cap;                                                  \
    for (size_t rki_var_idx = 0;                                                                   \
         (rki_var_idx = rki_ds_next_live(rki_var_dict->data, rki_var_cap, rki_var_idx))            \
         < rki_var_cap;                                                                            \
         ++rki_var_idx) {                                                                          \
      const typeof(*rki_var_dict->keys)* const _key = &rki_var_dict->keys[rki_var_idx];            \
      const typeof(*rki_var_dict->vals)* const _val = &rki_var_dict->vals[rki_var_idx];            \
      (void)_key;                                                                                  \
      (void)_val;                                                                                  \
      if (pred) {                                                                                  \
        rki_var_dict->data[rki_var_idx] = RKI_DS_SLOT_DELETED;                                     \
        --rki_var_dict->count;                                                                     \
        ++rki_var_dict->ndeleted;                                                                  \
      }                                                                                            \
    }                                                                                              \
  } while (0)

#define RKI_SET_ERASE_IF(self, key, pred)                                                          \
  do {                                                                                             \
    typeof(*(self))* const rki_var_set = (self);                                                   \
    if (!rki_var_set) { break; }                                                                   \
    const size_t rki_var_cap = rki_var_set->cap;                                                   \
    for (size_t rki_var_idx = 0;                                                                   \
         (rki_var_idx = rki_ds_next_live(rki_var_set->data, rki_var_cap, rki_var_idx))             \
         < rki_var_cap;                                                                            \
         ++rki_var_idx) {                                                                          \
      const typeof(*rki_var_set->keys)* const key = &rki_var_set->keys[rki_var_idx];               \
      (void)key;                                                                                   \
      if (pred) {                                                                                  \
        rki_var_set->data[rki_var_idx] = RKI_DS_SLOT_DELETED;                                      \
        --rki_var_set->count;                                                                      \
        ++rki_var_set->ndeleted;                                                                   \
      }                                                                                            \
    }                                                                                              \
  } while (0)

#define RKI_DICT_PUB(K, V, FNAME)            dict_##K##_##V##_##FNAME
#define RKI_DICT_PRI(K, V, FNAME)            rki_dict_##K##_##V##_##FNAME

#define RKI_DICT_INIT(K, V, init_cap, alloc) RKI_DICT_PUB(K, V, init)(init_cap RK_IFALLOC(, alloc))
#define RKI_DICT_INIT4(K, V, init_cap, alloc)                                                      \
  RKI_REQUIRE_CUSTOM_ALLOCATORS(RKI_DICT_INIT(K, V, init_cap, alloc))
#define RKI_DICT_INIT3(K, V, init_cap)   RKI_DICT_INIT(K, V, init_cap, alloc_ctx)

#define RKI_SET_INIT(K, init_cap, alloc) RKI_SET_PUB(K, init)(init_cap RK_IFALLOC(, alloc))
#define RKI_SET_INIT3(K, init_cap, alloc)                                                          \
  RKI_REQUIRE_CUSTOM_ALLOCATORS(RKI_SET_INIT(K, init_cap, alloc))
#define RKI_SET_INIT2(K, init_cap) RKI_SET_INIT(K, init_cap, alloc_ctx)

#define RKI_SET_PUB(K, FNAME)      set_##K##_##FNAME
#define RKI_SET_PRI(K, FNAME)      rki_set_##K##_##FNAME

#define RKI_SET_PUB_I(K, V, FNAME) RKI_SET_PUB(K, FNAME)
#define RKI_SET_PRI_I(K, V, FNAME) RKI_SET_PRI(K, FNAME)
#define RKI_SET(K, V)              Set(K)

#define RKI_DICT_DEF(key_t, val_t, hash_f, cmp_f)                                                  \
  RKI_DS_DEF(key_t, val_t, hash_f, cmp_f, RKI_EXPAND, RKI_IGNORE, Dict, RKI_DICT_PUB, RKI_DICT_PRI)
#define RKI_SET_DEF(key_t, hash_f, cmp_f)                                                          \
  RKI_DS_DEF(key_t, , hash_f, cmp_f, RKI_IGNORE, RKI_EXPAND, RKI_SET, RKI_SET_PUB_I, RKI_SET_PRI_I)

// The quotient product cannot exceed cap; the remainder product is widened to u64.
rklib_fun rk_pure size_t rki_ds_load_limit(size_t cap) {
  return (cap / RK_DICT_LOAD_DEN) * RK_DICT_LOAD_NUM
       + (size_t)(((u64)(cap % RK_DICT_LOAD_DEN) * RK_DICT_LOAD_NUM) / RK_DICT_LOAD_DEN);
}
rklib_fun size_t rki_ds_round_cap(size_t requested) {
  size_t cap = 16;
  while (cap < requested) {
    if (cap > SIZE_MAX / 2) { abort(); }
    cap *= 2;
  }
  return cap;
}
rklib_fun size_t rki_ds_cap_for_count(size_t n) {
  size_t cap = 16;
  while (rki_ds_load_limit(cap) < n) {
    if (cap > SIZE_MAX / 2) { abort(); }
    cap *= 2;
  }
  return cap;
}
rklib_fun void rki_ds_check_array_size(size_t cap, size_t element_size) {
  if (cap > SIZE_MAX / element_size) { abort(); }
}
rklib_fun rk_pure float rki_ds_load_factor(const RKI_ds_header* hdr) {
  return hdr->cap ? (float)hdr->count / (float)hdr->cap : 0.0f;
}
rklib_fun rk_pure bool rki_ds_needs_rehash(const RKI_ds_header* hdr) {
  // Invariant: count + ndeleted <= cap, so the sum does not overflow.
  return hdr->count + hdr->ndeleted >= rki_ds_load_limit(hdr->cap);
}

#define RKI_DS_DEF(K, V, hash_f, cmp_f, IF_DICT, IF_SET, DSTYPE, PUBF, PRIF)                        \
  RK_EXTERNC_BEG                                                                                    \
  typedef struct DSTYPE(K, V) {                                                                     \
    union {                                                                                         \
      RKI_ds_header hdr;                                                                            \
      struct { size_t cap, count, ndeleted; };                                                      \
    };                                                                                              \
    u8* data;                                                                                       \
    K*  keys;                                                                                       \
    IF_DICT(V* vals;)                                                                               \
    RK_IFALLOC(Allocator alloc;)                                                                    \
  } DSTYPE(K, V);                                                                                   \
  rklib_fun rk_pure u64  PRIF(K, V, hash_key)(K rki_var_key) { return (u64)hash_f(rki_var_key); }   \
  rklib_fun rk_pure bool PRIF(K, V, keys_equal)(K rki_var_a, K rki_var_b) {                         \
    return !cmp_f(rki_var_a, rki_var_b);                                                            \
  }                                                                                                 \
  rklib_fun rk_pure size_t    PUBF(K, V, count)(const DSTYPE(K, V) * self) { return self->count; }  \
  rklib_fun rk_pure size_t    PUBF(K, V, cap)(const DSTYPE(K, V) * self) { return self->cap; }      \
  rklib_fun rk_pure Allocator PUBF(K, V, allocator)(const DSTYPE(K, V) * self) {                    \
    return RKI_allocatorof(self);                                                                   \
  }                                                                                                 \
  rklib_fun rk_pure bool  PUBF(K, V, is_empty)(const DSTYPE(K, V) * self) { return !self->count; }  \
  rklib_fun rk_pure float PUBF(K, V, load_factor)(const DSTYPE(K, V) * self) {                      \
    return rki_ds_load_factor(&self->hdr);                                                          \
  }                                                                                                 \
  rklib_fun DSTYPE(K, V) PUBF(K, V, init)(size_t cap RK_IFALLOC(, Allocator alloc)) {               \
    RKI_assert_allocator_valid(alloc);                                                              \
    cap = rki_ds_round_cap(cap);                                                                    \
    rki_ds_check_array_size(cap, sizeof(K));                                                        \
    IF_DICT(rki_ds_check_array_size(cap, sizeof(V));)                                               \
    DSTYPE(K, V)                                                                                    \
    result      = {{{cap, 0, 0}}, rk_null, rk_null IF_DICT(, rk_null) RK_IFALLOC(, alloc)};         \
    result.data = (u8*)alloc_allocate(cap, align_max RK_IFALLOC(, alloc));                          \
    if (!result.data) { abort(); }                                                                  \
    rk_memset(result.data, RKI_DS_SLOT_EMPTY, cap);                                                 \
    result.keys = alloc_new(K, cap RK_IFALLOC(, alloc));                                            \
    if (!result.keys) { abort(); }                                                                  \
    IF_DICT(result.vals = alloc_new(V, cap RK_IFALLOC(, alloc)); if (!result.vals) { abort(); })    \
    return result;                                                                                  \
  }                                                                                                 \
  rklib_fun void PUBF(K, V, release)(DSTYPE(K, V) * self) {                                         \
    if (!self->cap) { return; }                                                                     \
    alloc_deallocate(self->data, self->cap, align_max RK_IFALLOC(, self->alloc));                   \
    alloc_delete(self->keys, self->cap RK_IFALLOC(, self->alloc));                                  \
    IF_DICT(alloc_delete(self->vals, self->cap RK_IFALLOC(, self->alloc)); self->vals = rk_null;)   \
    self->data = rk_null;                                                                           \
    self->keys = rk_null;                                                                           \
    self->cap = self->count = self->ndeleted = 0;                                                   \
  }                                                                                                 \
  rklib_fun void PRIF(K, V, grow)(DSTYPE(K, V) * self, size_t new_cap) {                            \
    const DSTYPE(K, V) old_self = *self;                                                            \
    DSTYPE(K, V) new_self       = PUBF(K, V, init)(new_cap RK_IFALLOC(, old_self.alloc));           \
    new_self.count              = old_self.count;                                                   \
    const size_t mask           = new_self.cap - 1;                                                 \
    for (size_t i = 0; i < old_self.cap; ++i) {                                                     \
      if (RKI_DS_SLOT_EMPTY_OR_DELETED(old_self.data[i])) { continue; }                             \
      K      key  = old_self.keys[i];                                                               \
      u64    hash = PRIF(K, V, hash_key)(key);                                                      \
      size_t j    = RKI_DS_HOME(mask, hash);                                                        \
      while (new_self.data[j] != RKI_DS_SLOT_EMPTY) { j = RKI_DS_NEXT(mask, j); }                   \
      new_self.data[j] = RKI_DS_FP(hash);                                                           \
      new_self.keys[j] = key;                                                                       \
      IF_DICT(new_self.vals[j] = old_self.vals[i];)                                                 \
    }                                                                                               \
    PUBF(K, V, release)(self);                                                                      \
    *self = new_self;                                                                               \
  }                                                                                                 \
  rklib_fun bool PRIF(K, V, ensure_cap)(DSTYPE(K, V) * self) {                                      \
    if (!self->cap) {                                                                               \
      RK_IFALLOC(RKI_set_alloc_fallback(self->alloc);)                                              \
      *self = PUBF(K, V, init)(rki_ds_cap_for_count(1) RK_IFALLOC(, self->alloc));                  \
      return true;                                                                                  \
    }                                                                                               \
    if (!rki_ds_needs_rehash(&self->hdr)) { return false; }                                         \
    size_t target = rki_ds_cap_for_count(self->count + 1);                                          \
    PRIF(K, V, grow)(self, target > self->cap ? target : self->cap);                                \
    return true;                                                                                    \
  }                                                                                                 \
  rklib_fun rk_pure RKI_hashprobe_t PRIF(K, V, probe_f)(const DSTYPE(K, V)* restrict self, K key,   \
                                                        u64 hash) {                                 \
    if (!self->cap) { return RKI_PROBE_MAKE(false, false, 0); }                                     \
    const u8     fp   = RKI_DS_FP(hash);                                                            \
    const size_t mask = self->cap - 1;                                                              \
    size_t       i = RKI_DS_HOME(mask, hash), fd = RK_DS_NOTIN;                                     \
    const u8* const restrict data = self->data;                                                     \
    const K* const restrict keys  = self->keys;                                                     \
    for (; data[i] != RKI_DS_SLOT_EMPTY; i = RKI_DS_NEXT(mask, i)) {                                \
      if (data[i] == RKI_DS_SLOT_DELETED) {                                                         \
        if (fd == RK_DS_NOTIN) { fd = i; }                                                          \
      } else if (data[i] == fp && PRIF(K, V, keys_equal)(key, keys[i])) {                           \
        return RKI_PROBE_MAKE(true, false, i);                                                      \
      }                                                                                             \
    }                                                                                               \
    return fd != RK_DS_NOTIN ? RKI_PROBE_MAKE(false, true, fd) : RKI_PROBE_MAKE(false, false, i);   \
  }                                                                                                 \
  rklib_fun RKI_hashprobe_t PRIF(K, V, prepare_insert)(DSTYPE(K, V) * self, K key, u64 hash) {      \
    RKI_hashprobe_t r = PRIF(K, V, probe_f)(self, key, hash);                                       \
    if (!RKI_PROBE_FOUND(r) && PRIF(K, V, ensure_cap)(self)) {                                      \
      r = PRIF(K, V, probe_f)(self, key, hash);                                                     \
    }                                                                                               \
    return r;                                                                                       \
  }                                                                                                 \
  rklib_fun rk_pure bool PUBF(K, V, contains)(const DSTYPE(K, V)* restrict self, K key) {           \
    return RKI_PROBE_FOUND(PRIF(K, V, probe_f)(self, key, PRIF(K, V, hash_key)(key)));              \
  }                                                                                                 \
  rklib_fun DSTYPE(K, V) * PUBF(K, V, clear)(DSTYPE(K, V)* restrict self) {                         \
    rk_memset(self->data, RKI_DS_SLOT_EMPTY, self->cap);                                            \
    self->count = self->ndeleted = 0;                                                               \
    return self;                                                                                    \
  }                                                                                                 \
  rklib_fun DSTYPE(K, V) * PUBF(K, V, reserve)(DSTYPE(K, V)* restrict self, size_t n) {             \
    if (!n) { return self; }                                                                        \
    size_t requested = n > self->count ? n : self->count;                                           \
    size_t cap       = rki_ds_cap_for_count(requested);                                             \
    if (!self->cap) {                                                                               \
      RK_IFALLOC(RKI_set_alloc_fallback(self->alloc);)                                              \
      *self = PUBF(K, V, init)(cap RK_IFALLOC(, self->alloc));                                      \
    } else if (cap > self->cap) {                                                                   \
      PRIF(K, V, grow)(self, cap);                                                                  \
    } else if (self->ndeleted > rki_ds_load_limit(self->cap) - requested) {                         \
      PRIF(K, V, grow)(self, self->cap);                                                            \
    }                                                                                               \
    return self;                                                                                    \
  }                                                                                                 \
  rklib_fun DSTYPE(K, V) * PUBF(K, V, shrink_to_fit)(DSTYPE(K, V)* restrict self) {                 \
    if (!self->count) {                                                                             \
      PUBF(K, V, release)(self);                                                                    \
      return self;                                                                                  \
    }                                                                                               \
    size_t target = rki_ds_cap_for_count(self->count);                                              \
    if (target < self->cap || self->ndeleted) { PRIF(K, V, grow)(self, target); }                   \
    return self;                                                                                    \
  }                                                                                                 \
  IF_DICT(                                                                                        \
    rklib_fun void PRIF(K, V, insert_f)(DSTYPE(K, V)* restrict self, K key, V val,                \
                                       u8 fp, bool used_tombstone, size_t i) {                    \
      ++self->count;                                                                              \
      if (used_tombstone) { --self->ndeleted; }                                                   \
      self->data[i] = fp;                                                                         \
      self->keys[i] = key;                                                                        \
      self->vals[i] = val;                                                                        \
    }                                                                                             \
    rklib_fun bool PUBF(K, V, set)(DSTYPE(K, V)* restrict self, K key, V val) {                   \
      u64 hash = PRIF(K, V, hash_key)(key);                                                       \
      RKI_hashprobe_t r = PRIF(K, V, prepare_insert)(self, key, hash);                            \
      if (RKI_PROBE_FOUND(r)) { self->vals[RKI_PROBE_IDX(r)] = val; }                             \
      else { PRIF(K, V, insert_f)(self, key, val, RKI_DS_FP(hash),                                \
                                  RKI_PROBE_TOMBSTONE(r), RKI_PROBE_IDX(r)); }                    \
      return !RKI_PROBE_FOUND(r);                                                                 \
    }                                                                                             \
    rklib_fun V* PUBF(K, V, add)(DSTYPE(K, V)* restrict self, K key, V val) {                     \
      u64 hash = PRIF(K, V, hash_key)(key);                                                       \
      RKI_hashprobe_t r = PRIF(K, V, prepare_insert)(self, key, hash);                            \
      if (RKI_PROBE_FOUND(r)) { return rk_null; }                                                 \
      PRIF(K, V, insert_f)(self, key, val, RKI_DS_FP(hash),                                       \
                           RKI_PROBE_TOMBSTONE(r), RKI_PROBE_IDX(r));                             \
      return &self->vals[RKI_PROBE_IDX(r)];                                                       \
    }                                                                                             \
    rklib_fun V* PUBF(K, V, get_or_add)(DSTYPE(K, V)* restrict self, K key, V val,                \
                                      bool* restrict inserted_out) {                              \
      u64 hash = PRIF(K, V, hash_key)(key);                                                       \
      RKI_hashprobe_t r = PRIF(K, V, prepare_insert)(self, key, hash);                            \
      if (!RKI_PROBE_FOUND(r)) {                                                                  \
        PRIF(K, V, insert_f)(self, key, val, RKI_DS_FP(hash),                                     \
                             RKI_PROBE_TOMBSTONE(r), RKI_PROBE_IDX(r));                           \
      }                                                                                           \
      if (inserted_out) { *inserted_out = !RKI_PROBE_FOUND(r); }                                  \
      return &self->vals[RKI_PROBE_IDX(r)];                                                       \
    }                                                                                             \
    rklib_fun rk_pure const V* PUBF(K, V, get_const)(const DSTYPE(K, V)* restrict self, K key) {  \
      RKI_hashprobe_t r = PRIF(K, V, probe_f)(self, key, PRIF(K, V, hash_key)(key));              \
      return RKI_PROBE_FOUND(r) ? &self->vals[RKI_PROBE_IDX(r)] : rk_null;                        \
    }                                                                                             \
    rklib_fun rk_pure V* PUBF(K, V, get)(DSTYPE(K, V)* restrict self, K key) {                    \
      RKI_hashprobe_t r = PRIF(K, V, probe_f)(self, key, PRIF(K, V, hash_key)(key));              \
      return RKI_PROBE_FOUND(r) ? &self->vals[RKI_PROBE_IDX(r)] : rk_null;                        \
    }                                                                                             \
    /* Not rk_pure: the assert must survive a discarded result, e.g. `(void)dict_at(...)`. */     \
    rklib_fun const V* PUBF(K, V, at_const)(const DSTYPE(K, V)* restrict self, K key) {           \
      RKI_hashprobe_t r = PRIF(K, V, probe_f)(self, key, PRIF(K, V, hash_key)(key));              \
      rk_assert(RKI_PROBE_FOUND(r) && "Key not present in Dict");                                 \
      return &self->vals[RKI_PROBE_IDX(r)];                                                       \
    }                                                                                             \
    rklib_fun V* PUBF(K, V, at)(DSTYPE(K, V)* restrict self, K key) {                             \
      RKI_hashprobe_t r = PRIF(K, V, probe_f)(self, key, PRIF(K, V, hash_key)(key));              \
      rk_assert(RKI_PROBE_FOUND(r) && "Key not present in Dict");                                 \
      return &self->vals[RKI_PROBE_IDX(r)];                                                       \
    }                                                                                             \
    rklib_fun bool PUBF(K, V, extract)(DSTYPE(K, V)* restrict self, K key, V* out_ptr) {          \
      rk_assert_ptr_nonnull(out_ptr);                                                             \
      RKI_hashprobe_t r = PRIF(K, V, probe_f)(self, key, PRIF(K, V, hash_key)(key));              \
      if (!RKI_PROBE_FOUND(r)) { return false; }                                                  \
      *out_ptr = self->vals[RKI_PROBE_IDX(r)];                                                    \
      --self->count;                                                                              \
      ++self->ndeleted;                                                                           \
      self->data[RKI_PROBE_IDX(r)] = RKI_DS_SLOT_DELETED;                                         \
      return true;                                                                                \
    }                                                                                             \
    rklib_fun bool PUBF(K, V, remove)(DSTYPE(K, V)* restrict self, K key) {                       \
      RKI_hashprobe_t r = PRIF(K, V, probe_f)(self, key, PRIF(K, V, hash_key)(key));              \
      if (!RKI_PROBE_FOUND(r)) { return false; }                                                  \
      --self->count;                                                                              \
      ++self->ndeleted;                                                                           \
      self->data[RKI_PROBE_IDX(r)] = RKI_DS_SLOT_DELETED;                                         \
      return true;                                                                                \
    }                                                                                             \
    rklib_fun DSTYPE(K, V)* PUBF(K, V, assign)(DSTYPE(K, V)* restrict self,                       \
                                             const K* keys, const V* vals, size_t n) {            \
      PUBF(K, V, clear)(self);                                                                    \
      PUBF(K, V, reserve)(self, n);                                                               \
      for (size_t i = 0; i < n; ++i) { PUBF(K, V, set)(self, keys[i], vals[i]); }                 \
      return self;                                                                                \
    }                                                                                             \
  ) \
  IF_SET(                                                                                         \
    rklib_fun bool PUBF(K, V, add)(DSTYPE(K, V)* restrict self, K key) {                          \
      u64 hash = PRIF(K, V, hash_key)(key);                                                       \
      RKI_hashprobe_t r = PRIF(K, V, prepare_insert)(self, key, hash);                            \
      if (RKI_PROBE_FOUND(r)) { return false; }                                                   \
      ++self->count;                                                                              \
      if (RKI_PROBE_TOMBSTONE(r)) { --self->ndeleted; }                                           \
      self->data[RKI_PROBE_IDX(r)] = RKI_DS_FP(hash);                                             \
      self->keys[RKI_PROBE_IDX(r)] = key;                                                         \
      return true;                                                                                \
    }                                                                                             \
    rklib_fun rk_pure const K* PUBF(K, V, get)(const DSTYPE(K, V)* restrict self, K key) {        \
      RKI_hashprobe_t r = PRIF(K, V, probe_f)(self, key, PRIF(K, V, hash_key)(key));              \
      return RKI_PROBE_FOUND(r) ? &self->keys[RKI_PROBE_IDX(r)] : rk_null;                        \
    }                                                                                             \
    /* Not rk_pure: the assert must survive a discarded result, e.g. `(void)set_at(...)`. */      \
    rklib_fun const K* PUBF(K, V, at)(const DSTYPE(K, V)* restrict self, K key) {                 \
      RKI_hashprobe_t r = PRIF(K, V, probe_f)(self, key, PRIF(K, V, hash_key)(key));              \
      rk_assert(RKI_PROBE_FOUND(r) && "Key not present in Set");                                  \
      return &self->keys[RKI_PROBE_IDX(r)];                                                       \
    }                                                                                             \
    rklib_fun bool PUBF(K, V, extract)(DSTYPE(K, V)* restrict self, K key, K* out_ptr) {          \
      rk_assert_ptr_nonnull(out_ptr);                                                             \
      RKI_hashprobe_t r = PRIF(K, V, probe_f)(self, key, PRIF(K, V, hash_key)(key));              \
      if (!RKI_PROBE_FOUND(r)) { return false; }                                                  \
      *out_ptr = self->keys[RKI_PROBE_IDX(r)];                                                    \
      --self->count;                                                                              \
      ++self->ndeleted;                                                                           \
      self->data[RKI_PROBE_IDX(r)] = RKI_DS_SLOT_DELETED;                                         \
      return true;                                                                                \
    }                                                                                             \
    rklib_fun bool PUBF(K, V, remove)(DSTYPE(K, V)* restrict self, K key) {                       \
      RKI_hashprobe_t r = PRIF(K, V, probe_f)(self, key, PRIF(K, V, hash_key)(key));              \
      if (!RKI_PROBE_FOUND(r)) { return false; }                                                  \
      --self->count;                                                                              \
      ++self->ndeleted;                                                                           \
      self->data[RKI_PROBE_IDX(r)] = RKI_DS_SLOT_DELETED;                                         \
      return true;                                                                                \
    }                                                                                             \
    rklib_fun DSTYPE(K, V)* PUBF(K, V, assign)(DSTYPE(K, V)* restrict self,                       \
                                             const K* keys, size_t n) {                           \
      PUBF(K, V, clear)(self);                                                                    \
      PUBF(K, V, reserve)(self, n);                                                               \
      for (size_t i = 0; i < n; ++i) { PUBF(K, V, add)(self, keys[i]); }                          \
      return self;                                                                                \
    }                                                                                             \
  ) \
  RK_EXTERNC_END

/// @endcond
RKI_HEADER_END
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
