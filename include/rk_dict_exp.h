/// @file rk_dict.h
/// @version 2.0
/// @defgroup rk_dict Hash Table (Dict) Interface
/// @brief Header-only, type-generic open-addressing hash table using linear
/// probing with fingerprint-accelerated lookup.
///
/// This header defines a type-safe, open-addressing hash table in C. Collisions
/// are resolved via linear probing. A separate byte array stores a 7-bit
/// fingerprint (high bits of the hash) per slot, allowing most non-matching
/// slots to be skipped without a full key comparison. Deleted slots are marked
/// as tombstones; a dedicated counter tracks their accumulation and triggers a
/// grow-or-compact rehash when combined live+tombstone load exceeds the
/// configured threshold. The implementation is type-generic, enabling
/// specialised versions for arbitrary key/value types via macros.
///
/// Key Features:
/// - Linear probing with 7-bit fingerprints to minimise key comparisons.
/// - Tombstone deletion with automatic compaction to prevent probe degradation.
/// - Header-only; no external linking required.
/// - Type-generic via macros, supporting custom key/value types.
/// - Custom allocator support for flexible memory management.
/// - Automatic resize/compact when the load factor exceeds a configurable
///   threshold (default 0.7).
/// - SOA layout (separate arrays for metadata, keys, and values).
/// - Fast lookup, insertion, and deletion operations.
///
/// Usage:
/// 1. Define a hash function with signature: `H hash_f(K key);`
///    (where H is the desired hash type).
///
/// 2. Define a comparison function with :
///        `bool cmp_f(K a, Kb);` as the signature
///        (strcmp-like: returns 0 if equal), return value may be any scalar
///        type as long as it implicitly converts to boolean.
///
/// 3. Create typedefs for key and value types as needed (pointers structs etc.
/// need typedefs due to the nature of the C preprocessor)
///    ```typedef char* cstr;```
///
/// 4. Declare and instantiate a specialized hash table:
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
/// Slot Encoding (data[] array, one byte per slot):
/// - `0x80`: empty — slot has never been used; probe chains stop here.
/// - `0xFE`: deleted (tombstone) — slot was occupied then removed; probe chains
///           continue through it.
/// - `0x00–0x7F`: occupied — value is the 7-bit fingerprint (top 7 bits of the
///                hash). Fingerprints let the probe loop skip non-matching
///                slots without a full key comparison.
///
/// Notes:
/// - On insert, if `count + n_deleted + 1 > RK_DICT_MAX_LOAD_FACTOR * cap`,
///   the table doubles in capacity (if live entries are dense) or rehashes to
///   the same capacity to flush accumulated tombstones.
///
/// @see rk_alloc.h
/// @see rk_defs.h
/// @{

#ifndef RK_DICT_H
# define RK_DICT_H
# include "rk_alloc.h"
RK_HEADER_BEGIN

/// @brief Define a dict type and functions for a given key/value pair.
///
/// This macro generates a complete, type-specific hash table API for the given
/// key-value combinations.
///
/// @param key_t  Key type
/// @param val_t  Value type
/// @param hash_f Hash function (`hash_t hash_f(key_t key)`)
/// @param cmp_f  Comparison function (`bool cmp_f(key_t a, key_t b)`)
/// @attention `cmp_f` must return 0/false if the two elements are equal
# define DICT_DEFINE(key_t, val_t, hash_f, cmp_f)                              \
    RK__DICT_DEF(key_t, val_t, hash_f, cmp_f)

/// @brief Sentinel value returned by internal index lookups when the key is not
/// present. Equal to `(size_t)-1`.
# define RK_DICT_NOTIN ((size_t)-1)

# define Dict(K, V)    dict##_##K##_##V

/// @brief Generates a type-specific dict struct name.

/// @brief `Dict(K, V) dict_init(Dict(K, V), size_t cap, Allocator alloc =
/// alloc_ctx)`
/// - Convenience Macro to create a Dict.
/// @param K       Name of the key Type
/// @param V       Name of the value Type
/// @param init_cap    size_t Initial Capacity of the Hash table
/// @param allocator   Allocator Optional parameter - Allocator; defaults to
/// `alloc_ctx` if not provided Example usage:
/// ```c
/// Dict(int, cstr) tab =  dict_init(int, cstr, 10);
/// Allocator alloc = (...);
/// Dict(int, cstr) tab =  hashself_init(int, cstr, 10, alloc);
/// ```
/// @return An initialised Dict
# define dict_init(TYPENAME, cap, ...)                                         \
    rk_overload(RK__dict_init, TYPENAME, cap, ##__VA_ARGS__)

# define dict_count(self)       ((self)->count)
# define dict_cap(self)         ((self)->cap)
# define dict_load_factor(self) ((float)dict_count(self) / dict_cap(self))
# define dict_is_empty(self)    ((bool)(dict_count(self) == 0))

/// @brief `void dict_release(TYPETYPENAME, TYPETYPENAME* self)` - Frees
/// the underlying memory of the dict.
# define dict_release(TYPETYPENAME, self)                                      \
    RK__DICT_PUB(TYPETYPENAME, release)(self)

/// @brief `bool dict_set(TYPETYPENAME, TYPETYPENAME* self, K key, V val)` -
/// Inserts a value into the dict, updating the value if it is present or
/// inserting a new one if not; resizes the dict if necessary.
/// @return `true` if inserted, `false` if updated
# define dict_set(TYPENAME, self, key, val)                                    \
    RK__DICT_PUB(TYPENAME, insert)(self, key, val)

/// @brief `bool dict_add(TYPENAME, TYPENAME* self, K key, V val)` -
/// Inserts a value into the dict only if the key is not already present;
/// resizes the dict if necessary.
/// @return `true` if inserted, `false` otherwise
# define dict_add(TYPENAME, self, key, val)                                    \
    RK__DICT_PUB(TYPENAME, add)(self, key, val)

/// @brief `V* dict_get(TYPENAME, TYPENAME* self, K key)` - Retrieves a pointer
/// to the value if the key was found or a `NULL` otherwise.
# define dict_get(TYPENAME, self, key) RK__DICT_PUB(TYPENAME, get)(self, key)

/// @brief `bool dict_contains(TYPENAME, const TYPENAME* self, K key)`
/// - Checks whether the given key is present in the dict.
/// @return `true` self contains the key, `false` otherwise
# define dict_contains(TYPENAME, self, key)                                    \
    RK__DICT_PUB(TYPENAME, contains)(self, key)

/// @brief `bool dict_extract(TYPENAME, TYPENAME* self, K key, V* out_ptr)` -
/// Removes a key from the dict and stores the value in `out_ptr`.
/// @param out_ptr Pointer to where the removed value should be written if
///                found.
/// @return `true` if key was found and removed, `false` otherwise
# define dict_extract(TYPENAME, self, key, out_ptr)                            \
    RK__DICT_PUB(TYPENAME, extract)(self, key, out_ptr)

/// @brief `bool dict_remove(TYPENAME, TYPENAME* self, K key)` - Removes a key
/// from the dict if it is present.
/// @return `true` if the value was found and removed, `false` otherwise
# define dict_remove(TYPENAME, self, key)                                      \
    RK__DICT_PUB(TYPENAME, remove)(self, key)

/// @brief `TYPENAME* dict_clear(TYPENAME, TYPENAME* self)` - Marks all slots in
/// the dict as free, allowing reuse of its memory.
/// @return `self`, for chaining.
# define dict_clear(TYPENAME, self) RK__DICT_PUB(TYPENAME, clear)(self)

/// @brief `TYPENAME* dict_reserve(TYPENAME, TYPENAME* self, size_t new_cap)` -
/// Reserves and rehashes the dict to ensure at least `new_cap` capacity.
/// @return `self`, for chaining.
/// @note Rounds up new_cap to the next power of two
# define dict_reserve(TYPENAME, self, new_cap)                                 \
    RK__DICT_PUB(TYPENAME, reserve)(self, new_cap)

/// @brief Iterates over all key-value pairs in the dict, skipping empty slots
/// @param self     Pointer to the dict to iterate over
/// @param key      Chosen name of the key pointer that will point to each key
/// @param val      Chosen name of the value pointer that will point to each key
///
/// Usage:
/// ```c
/// dict_foreach(&mydict, k, v) {
///     printf("key: %d, value: %s\n", *k, *v);
/// }
/// ```
/// @warning Adding or removing values via this macro leads to incorrect
/// iteration.
/// @note Iteration skips empty slots in the underlying storage.
# define dict_foreach(self, _key, _val)                                        \
    for (typeof(self) RK___dict = (self); RK___dict; RK___dict = rk_null)      \
      for (size_t RK___c = RK___dict->cap, RK___i = 0; RK___i < RK___c;        \
           ++RK___i)                                                           \
        for (const typeof(*(RK___dict->keys))*const _key                       \
             = !RK__DICT_SLOT_EMPTY_OR_DELETED(RK___dict->data[RK___i])        \
                 ? &(RK___dict->keys[RK___i])                                  \
                 : rk_null,                                                    \
             *RK__ONCE = _key;                                                 \
             RK__ONCE;)                                                        \
          for (typeof(*(RK___dict->vals))* const _val                          \
               = &(RK___dict->vals[RK___i]);                                   \
               RK__ONCE; RK__ONCE = 0)

# define dict_foreach_key(self, _key)                                          \
    for (typeof(self) RK___dict = (self); RK___dict; RK___dict = rk_null)      \
      for (size_t RK___c = RK___dict->cap, RK___i = 0; RK___i < RK___c;        \
           ++RK___i)                                                           \
        for (const typeof(*(RK___dict->keys))*const _key                       \
             = !RK__DICT_SLOT_EMPTY_OR_DELETED(RK___dict->data[RK___i])        \
                 ? &(RK___dict->keys[RK___i])                                  \
                 : rk_null,                                                    \
             *RK__ONCE          = _key;                                        \
             RK__ONCE; RK__ONCE = 0)

# define dict_foreach_val(self, _val)                                          \
    for (typeof(self) RK___dict = (self); RK___dict; RK___dict = rk_null)      \
      for (size_t RK___c = RK___dict->cap, RK___i = 0; RK___i < RK___c;        \
           ++RK___i)                                                           \
        for (typeof(*(RK___dict->vals))*const _val                             \
             = !RK__DICT_SLOT_EMPTY_OR_DELETED(RK___dict->data[RK___i])        \
                 ? &(RK___dict->vals[RK___i])                                  \
                 : rk_null,                                                    \
             *RK__ONCE          = _val;                                        \
             RK__ONCE; RK__ONCE = 0)
////////////////////////////////////////////////////////////////////////////////
/////////////////////////Implementation Details/////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

# define RK__DICT_PUB(TYPENAME, function_name)                                 \
    rk_CONC(rk_CONC(TYPENAME, _), function_name)
# define RK__DICT_PRI(TYPENAME, function_name)                                 \
    rk_CONC(rk_CONC(RK__, rk_CONC(TYPENAME, _)), function_name)

// internal helper macros
# define RK__DICT_home(self, hash) ((hash) & ((self)->cap - 1))
# define RK__DICT_next(self, i)    (((i) + 1) & ((self)->cap - 1))
# define RK__DICT_fp(hash)         ((hash) >> (bitsof(hash) - 7))
// Probe distance of slot curr from its home slot — reserved for future use.
# define RK__Ddst(self, hash, curr)                                            \
    (((curr) + (self)->cap - RK__DICT_home(self, hash)) & ((self)->cap - 1))

# define RK__DICT_SLOT_EMPTY               ((u8)0x80)
# define RK__DICT_SLOT_DELETED             ((u8)0xFE)
# define RK__DICT_SLOT_EMPTY_OR_DELETED(x) ((x) & 0x80)

# define RK__dict_init3(TYPENAME, init_cap, alloc)                             \
    RK__DICT_PUB(TYPENAME, init)(init_cap RK_IFALLOC(, alloc))
# define RK__dict_init2(TYPENAME, init_cap)                                    \
    RK__dict_init3(TYPENAME, init_cap, alloc_ctx)
# define RK__DICT_DEF(K, V, hash_f, cmp_f)                                     \
    RK_EXTERNC_BEG                                                             \
    typedef struct Dict(K, V) {                                                \
      size_t cap;                                                              \
      size_t count;                                                            \
      size_t n_deleted;                                                        \
      u8*    data;                                                             \
      K*     keys;                                                             \
      V*     vals;                                                             \
      RK_IFALLOC(Allocator alloc;)                                             \
    } Dict(K, V);                                                              \
    extern_fun Dict(K, V) RK__DICT_PUB(Dict(K, V), init)(                      \
        size_t cap RK_IFALLOC(, Allocator alloc)) {                            \
      rk_assert_allocator_valid(alloc);                                        \
      cap = stdc_bit_ceil(rk_MAX(16u, cap));                                   \
      return (Dict(K, V)){.cap       = cap,                                    \
                          .count     = 0,                                      \
                          .n_deleted = 0,                                      \
                          .data      = (u8*)memset(alloc_new(u8, cap, alloc),  \
                                                   RK__DICT_SLOT_EMPTY, cap),  \
                          .keys      = alloc_new(K, cap, alloc),               \
                          .vals      = alloc_new(V, cap, alloc),               \
                          RK_IFALLOC(.alloc = alloc)};                         \
    }                                                                          \
    extern_fun void RK__DICT_PUB(Dict(K, V), release)(Dict(K, V)* self) {      \
      if (!self->cap) { return; }                                              \
      alloc_delete(self->data, self->cap, self->alloc);                        \
      alloc_delete(self->keys, self->cap, self->alloc);                        \
      alloc_delete(self->vals, self->cap, self->alloc);                        \
      self->cap = self->count = self->n_deleted = 0;                           \
      self->data = rk_null, self->keys = rk_null, self->vals = rk_null;        \
    }                                                                          \
    extern_fun void RK__DICT_PRI(Dict(K, V), grow)(Dict(K, V)* self,           \
                                                   size_t      new_cap) {           \
      Dict(K, V) oldself = *self;                                              \
      Dict(K, V) newself                                                       \
          = {.cap       = new_cap,                                             \
             .count     = oldself.count,                                       \
             .n_deleted = 0,                                                   \
             .data      = (u8*)memset(alloc_new(u8, new_cap, oldself.alloc),   \
                                      RK__DICT_SLOT_EMPTY, new_cap),           \
             .keys      = alloc_new(K, new_cap, oldself.alloc),                \
             .vals      = alloc_new(V, new_cap, oldself.alloc),                \
             RK_IFALLOC(.alloc = oldself.alloc)};                              \
      for (size_t oldcap = oldself.cap, i = 0; i < oldcap; ++i) {              \
        if (RK__DICT_SLOT_EMPTY_OR_DELETED(oldself.data[i])) { continue; }     \
        K                   key  = oldself.keys[i];                            \
        V                   val  = oldself.vals[i];                            \
        typeof(hash_f(key)) hash = hash_f(key);                                \
        u8                  fp   = RK__DICT_fp(hash);                          \
        size_t              j    = RK__DICT_home(&newself, hash);              \
        for (; newself.data[j] != RK__DICT_SLOT_EMPTY;                         \
             j = RK__DICT_next(&newself, j));                                  \
        newself.keys[j] = key;                                                 \
        newself.vals[j] = val;                                                 \
        newself.data[j] = fp;                                                  \
      }                                                                        \
      RK__DICT_PUB(Dict(K, V), release)(self);                                 \
      *self = newself;                                                         \
    }                                                                          \
    extern_fun bool RK__DICT_PRI(Dict(K, V), insert)(                          \
        const bool always_insert, Dict(K, V)* restrict self, K key, V val) {   \
      if ((float)(self->count + self->n_deleted + 1) / self->cap               \
          > RK_DICT_MAX_LOAD_FACTOR) {                                         \
        size_t new_cap                                                         \
            = (self->count * 2 > self->cap) ? self->cap * 2 : self->cap;       \
        RK__DICT_PRI(Dict(K, V), grow)(self, new_cap);                         \
      }                                                                        \
                                                                               \
      typeof(hash_f(key)) hash          = hash_f(key);                         \
      u8                  fp            = RK__DICT_fp(hash);                   \
      size_t              i             = RK__DICT_home(self, hash);           \
      size_t              first_deleted = (size_t)-1;                          \
      for (; self->data[i] != RK__DICT_SLOT_EMPTY;                             \
           i = RK__DICT_next(self, i)) {                                       \
        if (self->data[i] == RK__DICT_SLOT_DELETED) {                          \
          if (first_deleted == (size_t)-1) { first_deleted = i; }              \
        } else if (self->data[i] == fp && !cmp_f(key, self->keys[i])) {        \
          if (always_insert) { self->vals[i] = val; }                          \
          return false;                                                        \
        }                                                                      \
      }                                                                        \
      if (first_deleted != (size_t)-1) {                                       \
        --self->n_deleted, i = first_deleted;                                  \
      }                                                                        \
      self->data[i] = fp;                                                      \
      self->keys[i] = key, self->vals[i] = val;                                \
      ++self->count;                                                           \
      return true;                                                             \
    }                                                                          \
    extern_fun bool RK__DICT_PUB(Dict(K, V), insert)(                          \
        Dict(K, V)* restrict self, K key, V val) {                             \
      if (!self->cap) { *self = dict_init(Dict(K, V), 16); };                  \
      return RK__DICT_PRI(Dict(K, V), insert)(true, self, key, val);           \
    }                                                                          \
    extern_fun bool RK__DICT_PUB(Dict(K, V), add)(Dict(K, V)* restrict self,   \
                                                  K key, V val) {              \
      if (!self->cap) { *self = dict_init(Dict(K, V), 16); };                  \
      return RK__DICT_PRI(Dict(K, V), insert)(false, self, key, val);          \
    }                                                                          \
    extern_fun size_t RK__DICT_PRI(Dict(K, V), get_index)(                     \
        const Dict(K, V)* restrict self, K key) {                              \
      typeof(hash_f(key)) hash = hash_f(key);                                  \
      u8                  fp   = RK__DICT_fp(hash);                            \
      size_t              n    = self->cap;                                    \
      for (size_t i = RK__DICT_home(self, hash); n;                            \
           i        = RK__DICT_next(self, i), --n) {                                  \
        if (self->data[i] == RK__DICT_SLOT_EMPTY) { return RK_DICT_NOTIN; }    \
        if (fp == self->data[i] && !cmp_f(key, self->keys[i])) { return i; }   \
      }                                                                        \
      return RK_DICT_NOTIN;                                                    \
    }                                                                          \
    extern_fun bool RK__DICT_PUB(Dict(K, V), contains)(                        \
        const Dict(K, V)* restrict self, K key) {                              \
      return RK__DICT_PRI(Dict(K, V), get_index)(self, key) != RK_DICT_NOTIN;  \
    }                                                                          \
    extern_fun V* RK__DICT_PUB(Dict(K, V),                                     \
                               get)(const Dict(K, V)* restrict self, K key) {  \
      size_t i = RK__DICT_PRI(Dict(K, V), get_index)(self, key);               \
      return i != RK_DICT_NOTIN ? &self->vals[i] : (V*)0;                      \
    }                                                                          \
    extern_fun Dict(K, V)* RK__DICT_PUB(Dict(K, V), reserve)(                  \
        Dict(K, V)* restrict self, size_t cap) {                               \
      if (cap > self->cap) {                                                   \
        RK__DICT_PRI(Dict(K, V), grow)(self, stdc_bit_ceil(rk_MAX(16u, cap))); \
      }                                                                        \
      return self;                                                             \
    }                                                                          \
    extern_fun bool RK__DICT_PUB(Dict(K, V), extract)(                         \
        Dict(K, V)* restrict self, K key, V * out_ptr) {                       \
      rk_assert_ptr_nonnull(out_ptr);                                          \
      size_t i = RK__DICT_PRI(Dict(K, V), get_index)(self, key);               \
      if (i == RK_DICT_NOTIN) { return false; }                                \
      *out_ptr      = self->vals[i];                                           \
      self->data[i] = RK__DICT_SLOT_DELETED;                                   \
      --self->count, ++self->n_deleted;                                        \
      return true;                                                             \
    }                                                                          \
    extern_fun bool RK__DICT_PUB(Dict(K, V),                                   \
                                 remove)(Dict(K, V)* restrict self, K key) {   \
      V _;                                                                     \
      return RK__DICT_PUB(Dict(K, V), extract)(self, key, &_);                 \
    }                                                                          \
    extern_fun Dict(K, V)* RK__DICT_PUB(Dict(K, V),                            \
                                        clear)(Dict(K, V)* restrict self) {    \
      if (!self->cap) { return self; };                                        \
      memset(self->data, RK__DICT_SLOT_EMPTY, self->cap);                      \
      self->count = self->n_deleted = 0;                                       \
      return self;                                                             \
    }                                                                          \
    RK_EXTERNC_END

/// @endcond

RK_HEADER_END
/// @}
#endif /* RK_DICT_H */
// int int_hash(int);
// int int_cmp(int, int);
//
// DICT_DEFINE(int, int, int_hash, int_cmp)
