// SPDX-License-Identifier: MIT
/// @file rk_string.h
/// @version 2.0
/// @defgroup rk_string String Library Interface
/// @brief A small custom header-only String library for working with dynamically allocated Strings
/// and String views. This library provides functionality to create, manipulate, and manage custom
/// String Structs (`Str`), which are dynamically allocated with automatic resizing based on the
/// length of the String. It also provides a simple immutable String view type, `Strv`.
///
/// @section Customisation Points Customisation points are the same as for all other rk_clib
/// headers, most notably the `RK_ALLOCMODE` macro which, if not set to `RK_ALLOCMODE_FULL`, turns
/// off custom allocators, removing the respective data members from the structs and parameters from
/// the functions, while discarding the allocator parameters in the user-facing macros, removing any
/// overhead.
///
/// @section Data Types This Header Provides two Data Types, each one having a superset of the
/// abilities of the former
/// - `Str`: A mutable String with dynamic memory allocation.
/// - `Strv`: A non-owning view over a String.
///
/// @section Function Parameters and Implicit Conversions Internally, most functions are defined as
/// taking a `Strv` parameter; the macros in this header allow for passing of any Stringlike object,
/// expanding them automatically to the required argument types.
/// @see rk_alloc.h
/// @see rk_defs.h
/// @{

#ifndef RK_STRING_H
#define RK_STRING_H
#include "rk_alloc.h"
#include <stdarg.h>
#include <string.h>
RK_HEADER_BEGIN

/// @brief Represents a non-owning, non-mutable view of a string.
/// @note `(Strv){NULL, 0}` is a valid `Strv` and will be treated accordingly by all functions
/// defined here.
typedef struct Strv {
  const char* str; ///< Pointer to the String str
  size_t      len; ///< The length of the String
} Strv;

/// @brief Represents a mutable, null-terminated string buffer. The string is represented by a
/// null-terminated dynamic array of characters along with its current length and allocated capacity
/// and an optional Allocator member. Every `Str` object can be safely cast into a Strv, either via
/// pointer cast or by taking its `v` member. A valid `Str` satisfies:
/// - `str` points to a buffer of at least `cap` bytes or is NULL if cap == 0
/// - if `str` is not null, it is null-terminated with `str[len] == '\0'`
/// - `len < cap` // whenever str != NULL, or len == cap == 0 else.
/// - Functions generally preserve null-termination unless explicitly stated. The only exceptions in
///   this API are `str_release()` and the `_raw()` functions.
typedef struct Str {
  union {
    Strv v; ///< The contained Strv
    struct {
      char*  str; ///< Pointer to the String str for ergonomics
      size_t len; ///< The length of the String
    };
  };
  size_t cap; ///< Capacity of the Str
#if RK_ALLOCMODE == RK_ALLOCMODE_FULL
  Allocator alloc; ///< Allocator (can be disabled)
#endif
} Str;

/// @defgroup stringlike Stringlike Types
/// @brief Types accepted by most string APIs.
///
/// A Stringlike is any of the following:
/// - `Str`
/// - `Strv`
/// - `char*`
/// - `const char*` These are implicitly converted to `Strv` via internal macros.

// --------------------------------
// Section: Strlike Basic Accessors
// --------------------------------

/// @brief Returns the string data ([const] char*) or any Stringlike.
#define str_dat(strlike)        RK__str_dat(strlike)

/// @brief `size_t str_len(strlike)` - Returns the length of any Stringlike.
#define str_len(strlike)        RK__str_len(strlike)

/// @brief `bool str_is_empty(strlike)` - Returns if stringlike is empty.
#define str_is_empty(strlike)   ((bool)(str_len(strlike) == 0))

/// @brief Returns an lvalue reference to the first character of a Stringlike.
/// @param strlike the Stringlike, by value
/// @return Lvalue reference to the first character in strlike
/// @note behaviour undefined for empty strings
#define str_front(strlike)      (*RK__qcharptr(strlike, RK__str_front_ptr(strv_from(strlike))))

/// @brief Returns an lvalue reference to the last character of a string.
/// @param strlike the Stringlike, by value
/// @return Lvalue reference to the last character in strlike
/// @note behaviour undefined for empty strings
#define str_back(strlike)       (*RK__qcharptr(strlike, RK__str_back_ptr(strv_from(strlike))))

// ----------------------------------------
// Section: String Lifetime/Ownership
// ----------------------------------------

/// @brief `Str str_init(size_t init_cap, Allocator alloc = alloc_ctx)` - Constructs a Str with a
/// given initial capacity.
/// @param init_cap size_t The initial capacity of the string (in elements)
/// @param alloc Allocator Optional parameter - The allocator; defaults to `alloc_ctx`
/// @return A `Str` object with the given capacity
#define str_init(init_cap, ...) rk_overload(RK__STR_INIT, init_cap, ##__VA_ARGS__)

/// @brief `Str str_from(Strlike strlike, Allocator alloc = alloc_ctx)` - Constructs a Str from a
/// Stringlike object, copying the data.
/// @param strlike     A Stringlike, by value
/// @param alloc Allocator Optional parameter - The allocator backing the new Str. If not provided,
/// this defaults to the Allocator of the cloned Strlike if it is a Str object, or alloc_ctx
/// otherwise.
/// @return A `Str` object with the copied string data
#define str_from(strlike, ...)  rk_overload(RK__STR_FROM, strlike, ##__VA_ARGS__)

/// @brief `Str str_from_literal(STRING_LITERAL, Allocator alloc = alloc_ctx)`
/// - Construct a Str from a string literal.
/// @param strlit  A string literal
/// @param alloc Allocator Optional parameter - The allocator; defaults to `alloc_ctx` if not
/// provided
/// @return A `Str` object initialised with the literal's contents
#define str_from_literal(strlit, ...) rk_overload(RK__STR_FROMLIT, strlit, ##__VA_ARGS__)

/// @brief Frees the underlying memory of `self`.
static_fun void str_release(Str* restrict self) {
  if (self->str) { alloc_delete(self->str, self->cap RK_IFALLOC(, self->alloc)); }
  self->len = self->cap = 0, self->str = rk_null;
}

/// @brief `Str str_join_strv_n(Strv* svs, size_t count, Strv sep, Allocator alloc = alloc_ctx)` -
/// Constructs a Str by concatenating `count` elements from `svs`, inserting `sep` between each
/// element.
/// @param svs    Strv*   Pointer to an array of Strv elements
/// @param count  size_t  Number of elements in the array
/// @param sep    Strv    Separator inserted between elements
/// @param alloc Allocator Optional parameter - The allocator; defaults to `alloc_ctx` if not
/// provided.
/// @return A `Str` object containing the joined string
#define str_join_strv_n(svs, count, sep, ...)                                                      \
  rk_overload(RK__STR_JOIN_STRV_N, svs, count, sep, ##__VA_ARGS__)

// ----------------------------------------
// Section: String Accessors
// ----------------------------------------

/// @brief Returns whether the String is null-terminated.
static_fun bool str_is_null_terminated(const Str* self) {
  return self->cap > self->len && self->str[self->len] == '\0';
}

/// @brief Returns a null-terminated C string view of the string.
/// @param self Pointer to the string object.
/// @return Pointer to a null-terminated string; never `NULL`.
///
/// If the string is already null-terminated, returns a pointer to its internal buffer. Otherwise,
/// returns a pointer to a static empty string ("").
///
/// @note The returned pointer remains valid as long as `self` is valid and not modified. If `self`
/// is not null-terminated, the returned pointer does not reference its contents.
///
/// @warning This function does not enforce null-termination or perform any allocation. Callers must
/// ensure null-termination if access to the full contents as a C string is required.
static_fun const char* str_cstr(const Str* self) {
  return str_is_null_terminated(self) ? self->str : "";
}

#if RK_ALLOCMODE == RK_ALLOCMODE_FULL
# define str_allocator(self) rk_to_rvalue((self)->alloc)
#else
# define str_allocator(self) rk_allocator_disabled()
#endif

// ----------------------------------------
// Section: String Capacity
// ----------------------------------------

/// @brief Ensures at least `new_cap` bytes of capacity are allocated for `self`, reallocating, if
/// necessary.
static_fun Str* str_reserve(Str* restrict self, size_t new_cap);

/// @brief Resizes the length of `self` to `new_len`, reallocating the memory if necessary and
/// null-terminating it.
static_fun Str* str_resize(Str* restrict self, size_t new_len);

/// @brief Resizes a Str's capacity to the next power of two larger than its length,
/// null-terminating it.
static_fun Str* str_shrink_to_fit(Str* restrict self);

/// @brief Resizes a Str's capacity to exactly its length + 1.
static_fun Str* str_shrink_to_fit_exact(Str* restrict self);

// ----------------------------------------
// Section: String Mutators
// ----------------------------------------
/// @brief Assigns a new value to a `Str` from a Stringlike, overriding its contents. The Str is
/// resized if necessary to fit the new data.
/// @param self A pointer to the destination `Str`
/// @param strlike A Stringlike
/// @return `self`, for chaining.
#define str_assign(self, strlike) str_assign_strv(self, strv_from(strlike))

/// @brief Null-Terminates a Str object if it is not, resizing if necessary.
/// @note Null-Terminators added via this function leave the length unchanged.
static_fun Str* str_null_terminate(Str* restrict self);

/// @brief Like `str_null_terminate()` without checking for capacity.
static_fun Str* str_null_terminate_unchecked(Str* restrict self);

/// @brief Terminates a `Str` with `suffix` if it does not already end with it and then
/// null-terminates it if it isn't.
#define str_terminate(self, suffix) RK__str_terminate(self, suffix)

/// @brief Adds a character to a Str object, resizing if necessary.
/// @note Appends int as a character of its value, doing `str_push(s, 1)` is likely a bug.
static_fun Str* str_push(Str* restrict self, char c);

/// @brief Like `str_push()` without checking for capacity.
static_fun Str* str_push_unchecked(Str* restrict self, char c);

/// @brief Like `str_push()` without null-terminating the Str.
static_fun Str* str_push_raw(Str* restrict self, char c);

/// @brief Like `str_push_unchecked()` but without null-terminating the Str.
static_fun Str* str_push_unchecked_raw(Str* restrict self, char c);

/// @brief Generic Convenience Macro for appending Strlikes or characters to a string, automatically
/// converting `strlike` to Strv and null-terminating `self`. May reallocate `self`.
/// @param self    The String to append to
/// @param strlike The Stringlike to append
/// @return The updated String.
/// @note Appends int as a character of its value, `str_cat(s, 1)` is likely wrong.
/// @attention Appending a substring of `self` (i.e. overlapping memory) results in undefined
/// behavior. Use `str_cat_mayalias()` in that case.
#define str_cat(self, strlike)          RK__str_cat(self, strlike)

/// @brief Like `str_cat()` but checks for overlap between `self` and `strlike`.
#define str_cat_mayalias(self, strlike) RK__str_cat_mayalias(self, strlike)

/// @brief Like `str_cat()`, specialised on string literals. Avoids a runtime `strlen()` call via
/// compile-time `sizeof()` and ensures a compile-time error if `strlit` is not a string literal.
#define str_cat_literal(self, strlit)   str_cat_strv(self, (Strv)strv_from_literal(strlit))

/// @brief Appends formatted output (printf-style) to the string. The format string and arguments
/// follow the rules of the `printf` family.
/// @param self The string to append to
/// @param fmt  A printf-style format string
/// @param ...  Format arguments
/// @return The updated string on success, or `NULL` on formatting error
static_fun Str* str_cat_fmt(Str* self, const char* fmt, ...);

/// @brief Inserts a Stringlike into a Str, resizing the str if necessary.
/// @return `self`, for chaining
/// @attention Appending a substring of `self` to itself via this is undefined
#define str_insert_at(self, idx, strlike)          RK__str_insert_at(self, idx, strlike)

/// @brief Like `str_insert_at()` but checks for pointer aliasing between self and strlike.
#define str_insert_at_mayalias(self, idx, strlike) RK__str_insert_at_mayalias(self, idx, strlike)

/// @brief Pops the last character off the Str, decreasing its length and null-terminating it.
/// @note Returns the `\0` if the Str is empty.
static_fun char str_pop(Str* restrict self) {
  if (self->len == 0) { return '\0'; }
  char tmp                    = self->str[--self->len];
  return self->str[self->len] = '\0', tmp;
}

/// @brief Pops the last n character off the Str, decreasing its length and null-terminating it.
/// @note no-op for size 0 strings. Popping more characters than the length of the Str is asserted
/// in debug builds.
static_fun void str_pop_n(Str* restrict self, size_t n) {
  if (!n) { return; }
  rk_assert(n <= self->len && "Attempted to pop more than Str length");
  self->len -= n, self->str[self->len] = '\0';
}

/// @brief Removes a single character at the given index from the string.
/// @param self The string to modify
/// @param idx  The position of the character to remove. Must be < str->len
/// @attention Passing an out-of-bounds index results in undefined behavior.
static_fun Str* str_erase_at(Str* restrict self, size_t idx);

/// @brief Removes `count` characters starting at `idx` from the string.
/// @param self  The string to modify
/// @param idx   The starting position of characters to erase
/// @param count The number of characters to remove
/// @attention Supplying an out-of-range index or count is undefined
static_fun Str* str_erase_at_n(Str* restrict self, size_t idx, size_t count);

/// @brief Convenience Macro to erase all elements in `self` that satisfy a predicate.
/// @param self        Str* The Str to loop over
/// @param it          The name of the iterator (access via *it)
/// @param pred        The predicate (an expression)
///
/// Usage:
/// ```c
/// Str str = str_from_literal("hello");
/// str_erase_if(&str, c, (*c == 'o'));
/// printf("%s\n", str.str); // prints "hell"
/// ```
#define str_erase_if(self, it, pred)                                                               \
  do {                                                                                             \
    Str* RK__STR = self;                                                                           \
    if (!RK__STR->str) { break; }                                                                  \
    char*             RK__WRITE = RK__STR->str;                                                    \
    const char* const RK__READ  = RK__STR->str;                                                    \
    for (const char *RK__IT = RK__READ, *const RK__END = RK__READ + RK__STR->len;                  \
         RK__IT < RK__END; ++RK__IT) {                                                             \
      const char* const it = RK__IT;                                                               \
      if (!(pred)) { *RK__WRITE++ = *RK__IT; }                                                     \
    }                                                                                              \
    RK__STR->len               = (size_t)(RK__WRITE - RK__STR->str);                               \
    RK__STR->str[RK__STR->len] = '\0';                                                             \
  } while (0)

/// @brief Clears the contents of `self`, setting its length to zero and null-terminating it, if it
/// owns an allocation.
static_fun Str* str_clear(Str* restrict self) {
  if (self->str) { self->str[self->len = 0] = '\0'; }
  return self;
}

/// @brief Replaces all instances of `oldc` in `self` with `newc`.
static_fun Str* str_replace(Str* restrict self, char oldc, char newc);

/// @brief Replaces the character at position `pos` in `self` with `c`. Does not modify the `len`
/// field, even if `c == '\0'`. This allows temporarily inserting a null terminator within the
/// string to treat a substring as a C-string, and later restoring the original character without
/// needing to modify the `len` field.
/// @return The previously stored character at position `pos`
/// @note Behaviour is undefined if `pos >= self->len`.
static_fun char str_replace_at(Str* restrict self, size_t pos, char c);

/// @brief Converts all lowercase letters in `self` to uppercase.
static_fun Str* str_to_upper(Str* restrict self);

/// @brief Converts all uppercase letters in `self` to lowercase.
static_fun Str* str_to_lower(Str* restrict self);

/// @brief Reverses `self` in place.
static_fun Str* str_reverse(Str* restrict self);

// ----------------------------------------
// Section: Strv Construction and View
// ----------------------------------------
/// @brief Constructs a Strv from a string literal.
#define strv_from_literal(strlit) {.str = strlit, .len = lenof(strlit)}

static_fun Strv strv_from_cstrn(const char* str, size_t len) {
  return (Strv){.str = str, .len = len};
}
/// @brief Constructs a Strv from a Stringlike.
#define strv_from(strlike)              RK__strv_from(strlike)

/// @brief Creates a Strv from a substring of a Stringlike. If `start > str_len(strlike)`, an empty
/// view is returned. if `start + count > str_len(strlike)` is larger than length, the String from
/// start to end is copied.
/// @param strlike The source Stringlike, by value
/// @param start   The starting position of the substring
/// @param end     The end position of the substring
/// @return A substring from the source String, type matches `strv` argument
#define strv_slice(strlike, start, end) strv_slice_strv(strv_from(strlike), start, end)

/// @brief Creates a Strv from a substring of a char array (or pointer) at compile time.
#define strv_slice_static(arr, start, end)                                                         \
  {.str = (const char*)((arr) + (start)), .len = (end) - (start)}

/// @brief Sentinel to indicate the end of `str_split()`.
#define STR_SPLIT_END              ((size_t)-1)

/// @brief Tokenises the string without modifying the underlying data. The function consumes the
/// view pointed to by `strvptr`, returning successive tokens separated by `delims`. The original
/// string data is never modified. Instead, `*strvptr` is advanced to point to the remaining
/// unconsumed portion of the view. When the final token has been returned, `strvptr->len` is set to
/// `STR_SPLIT_END`. Callers must stop iterating once this sentinel value is observed.
///
/// @param strvptr Pointer to the view being consumed The view is modified in place.
/// @param delims The delimiters used for tokenisation May be a character, integer (character
/// literal), or any Stringlike type. Each occurrence of a delimiter separates tokens.
/// @return A `Strv` representing the next token. The returned view refers directly into the
/// original string data.
///
/// Usage:
/// ```c
/// Str s = str_from_literal("this is a test");
/// Strv v = s.v;
/// Strv buf[64];
/// size_t i = 0;
/// while (v.len != STR_SPLIT_END) { buf[i++] = str_split(&v, ' '); }
/// rk_assert(i == 4);
/// ```
#define str_split(strvptr, delims) RK__str_split(strvptr, delims)

/// @brief Tokenises the Stringlike without modifying the underlying data, allocating an array of
/// Strv Objects.
/// @param strlike   The Stringlike to tokenise (by value)
/// @param delims    The delimiters of the tokenisation, a Stringlike by value
/// @param out_count size_t* an out-parameter in which the count of tokens will be stored.
/// @param alloc Optional; The Allocator that allocates the array of Strv objects; defaults to
/// `alloc_ctx` (or malloc_allocator, if `RK_ALLOCMODE` == `RK_ALLOCMODE_MALLOC_ONLY`)
/// @return A Strv* to the array of tokens.
#define str_split_alloc(strlike, delims, out_count, ...)                                           \
  rk_overload(RK__STR_SPLIT_ALLOC, strv_from(strlike), strv_from(delims), out_count, ##__VA_ARGS__)

/// @brief Trims a Stringlike into a string by adjusting its starting position to the first
/// non-space character it finds.
/// @param strlike The Stringlike to trim, by value
/// @return A Strv over the trimmed Stringlike
/// @note `c` counts as space if `(c == ' ' || (c >= '\t' && c <= '\r'))`
#define str_trimmed(strlike)            str_trimmed_strv(strv_from(strlike))
#define str_trimmed_left(strlike)       str_trimmed_left_strv(strv_from(strlike))
#define str_trimmed_right(strlike)      str_trimmed_right_strv(strv_from(strlike))

// ----------------------------------------
// Section: Strlike Query/Search Functions
// ----------------------------------------

/// @brief Compares two Stringlikes lexicographically.
/// @param strlike1 The first Stringlike, by value
/// @param strlike2 The second Stringlike, by value
/// @return An integer less than, equal to, or greater than zero if `s1` is lexicographically less
/// than, equal to, or greater than `s2`, respectively.
/// @note If the strings are equal up to the length of the shorter one, the result is the difference
/// in their lengths.
#define str_compare(strlike1, strlike2) str_compare_strv(strv_from(strlike1), strv_from(strlike2))

/// @brief Returns whether two Stringlikes are lexicographically equal.
#define str_equals(strlike1, strlike2)  str_equals_strv(strv_from(strlike1), strv_from(strlike2))

/// @brief Finds the first occurrence of `subs` in `strlike`.
/// @param strlike  A Stringlike, the haystack
/// @param subs     A Stringlike, the needle
/// @return A `(const) char*` to the first occurrence or a nullpointer if the substring is not
/// found.
#define str_find(strlike, subs)         RK__qcharptr(strlike, RK__str_find(strlike, subs))

/// @brief Finds the last occurrence of `subs` in `strlike`.
/// @param strlike  A Stringlike, the haystack
/// @param subs     A Stringlike, the needle
/// @return A `(const) char*` to the last occurrence or a nullpointer if the substring is not found.
#define str_findr(strlike, subs)        RK__qcharptr(strlike, RK__str_findr(strlike, subs))

/// @brief Checks whether a Stringlike contains another Stringlike.
/// @param strlike  A Stringlike, the haystack
/// @param subs     A Stringlike, the needle
/// @return `true`, if strlike contains subs, `false` otherwise
#define str_contains(strlike, subs)     RK__str_contains(strlike, subs)

/// @brief Tests if a `pre` is a prefix of `strlike`.
/// @param strlike A Stringlike, by value
/// @param pre    The potential prefix of strlike
/// @return `true` if pre is prefix of strlike, `false` otherwise
#define str_starts_with(strlike, pre)   RK__str_starts_with(strlike, pre)

/// @brief Tests if a `suf` is a suffix of `strlike`.
/// @param strlike A Stringlike, by value
/// @param suf  The potential suffix of strlike
/// @return `true` if suf is suffix of strlike, `false` otherwise
#define str_ends_with(strlike, suf)     RK__str_ends_with(strlike, suf)

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

#define RK__str_dat(_S)                                                                            \
  _Generic(_S,                                                                                     \
      Str: (char*)RK__contrav(Str, _S).str,                                                        \
      Strv: (const char*)RK__contrav(Strv, _S).str,                                                \
      char*: (char*)RK__contrav_p(char*, _S),                                                      \
      const char*: (const char*)RK__contrav_p(const char*, _S))

#define RK__str_len(_S)                                                                            \
  ((size_t)_Generic(_S,                                                                            \
       Str: RK__contrav(Str, _S).len,                                                              \
       Strv: RK__contrav(Strv, _S).len,                                                            \
       char*: strlen(RK__contrav_p(char*, _S)),                                                    \
       const char*: strlen(RK__contrav_p(const char*, _S)),                                        \
       int: 1,                                                                                     \
       char: 1))

// conditionally cast away const (const is default)
#define RK__qcharptr(_S, expr)                                                                     \
  _Generic(_S, Str: (char*)(expr), Strv: expr, char*: (char*)(expr), const char*: expr)

#define RK__strv_from(_S)                                                                          \
  _Generic(_S,                                                                                     \
      Str: RK__contrav(Str, _S).v,                                                                 \
      Strv: RK__contrav(Strv, _S),                                                                 \
      char*: strv_from_cstr(RK__contrav_p(char*, _S)),                                             \
      const char*: strv_from_cstr(RK__contrav_p(const char*, _S)))

// only to satisfy _Generic when the type cannot be int or char
#define RK__strv_from_fallback(_S)                                                                 \
  _Generic(_S,                                                                                     \
      Str: RK__contrav(Str, _S).v,                                                                 \
      Strv: RK__contrav(Strv, _S),                                                                 \
      char*: strv_from_cstr(RK__contrav_p(char*, _S)),                                             \
      const char*: strv_from_cstr(RK__contrav_p(const char*, _S)),                                 \
      int: (Strv){rk_null, 0},                                                                     \
      char: (Strv){rk_null, 0})

#define RK__IsCharLitLike(C, _if, _else)                                                           \
  rk_static_if(_Generic(C, char: 1, int: 1, default: 0), _if, _else)
#define RK__GetCharLitLike(_S) _Generic(_S, char: _S, int: _S, default: 0)

#define RK__str_insert_at(self, idx, strlike)                                                      \
  RK__IsCharLitLike(strlike, str_insert_at_char(self, idx, RK__GetCharLitLike(strlike)),           \
                    str_insert_at_strv(self, idx, RK__strv_from_fallback(strlike)))

#define RK__str_insert_at_mayalias(self, idx, strlike)                                             \
  RK__IsCharLitLike(strlike, str_insert_at_char(self, idx, RK__GetCharLitLike(strlike)),           \
                    str_insert_at_strv_mayalias(self, idx, RK__strv_from_fallback(strlike)))

#define RK__str_split(strvptr, delims)                                                             \
  RK__IsCharLitLike(delims, str_split_char(strvptr, RK__GetCharLitLike(delims)),                   \
                    str_split_strv(strvptr, RK__strv_from_fallback(delims)))

#define RK__str_terminate(self, suffix)                                                            \
  RK__IsCharLitLike(suffix, str_terminate_char(self, RK__GetCharLitLike(suffix)),                  \
                    str_terminate_strv(self, RK__strv_from_fallback(suffix)))

#define RK__str_cat(self, strlike)                                                                 \
  RK__IsCharLitLike(strlike, str_push(self, RK__GetCharLitLike(strlike)),                          \
                    str_cat_strv(self, strv_from(strlike)))

#define RK__str_cat_mayalias(self, strlike)                                                        \
  RK__IsCharLitLike(strlike, str_push(self, RK__GetCharLitLike(strlike)),                          \
                    str_cat_strv_mayalias(self, strv_from(strlike)))

#define RK__str_find(strlike, subs)                                                                \
  RK__IsCharLitLike(subs, str_find_char(strv_from(strlike), RK__GetCharLitLike(subs)),             \
                    str_find_strv(strv_from(strlike), RK__strv_from_fallback(subs)))

#define RK__str_findr(strlike, subs)                                                               \
  RK__IsCharLitLike(subs, str_findr_char(strv_from(strlike), RK__GetCharLitLike(subs)),            \
                    str_findr_strv(strv_from(strlike), RK__strv_from_fallback(subs)))

#define RK__str_contains(strlike, subs)                                                            \
  RK__IsCharLitLike(subs, str_contains_char(strv_from(strlike), RK__GetCharLitLike(subs)),         \
                    str_contains_strv(strv_from(strlike), RK__strv_from_fallback(subs)))

#define RK__str_starts_with(strlike, prefix)                                                       \
  RK__IsCharLitLike(prefix, str_starts_with_char(strv_from(strlike), RK__GetCharLitLike(prefix)),  \
                    str_starts_with_strv(strv_from(strlike), RK__strv_from_fallback(prefix)))

#define RK__str_ends_with(strlike, prefix)                                                         \
  RK__IsCharLitLike(prefix, str_ends_with_char(strv_from(strlike), RK__GetCharLitLike(prefix)),    \
                    str_ends_with_strv(strv_from(strlike), RK__strv_from_fallback(prefix)))

static_fun Strv strv_from_cstr(const char* s) { return (Strv){.str = s, .len = s ? strlen(s) : 0}; }

static_fun Strv strv_from_strv(Strv str) { return str; }

#define RK__STR_FROMLIT(a, alloc)  str_from_strv((Strv)strv_from_literal(a) RK_IFALLOC(, alloc))
#define RK__STR_FROMLIT2(a, alloc) rk_disable_if(RK__STR_FROMLIT(a, alloc))
#define RK__STR_FROMLIT1(a)        RK__STR_FROMLIT(a, alloc_ctx)

static_fun Strv strv_from_str(Str str) { return str.v; }
static_fun bool str_equals_strv(Strv s1, Strv s2) {
  size_t mlen = rk_MIN(s1.len, s2.len);
  return s1.len == s2.len && !rk_memcmp(s1.str, s2.str, mlen);
}
static_fun int str_compare_strv(Strv s1, Strv s2) {
  size_t mlen = rk_MIN(s1.len, s2.len);
  int    res  = rk_memcmp(s1.str, s2.str, mlen);
  return res ? res : (s1.len < s2.len ? -1 : (s1.len > s2.len ? 1 : 0));
}

static_fun bool str_starts_with_char(Strv sv, char c) { return sv.len && sv.str[0] == c; }
static_fun bool str_starts_with_strv(Strv sv, Strv pref) {
  return pref.len <= sv.len && !rk_memcmp(sv.str, pref.str, pref.len);
}
static_fun bool str_ends_with_char(Strv sv, char c) { return sv.len && sv.str[sv.len - 1] == c; }

static_fun bool str_ends_with_strv(Strv sv, Strv suf) {
  return suf.len <= sv.len && !rk_memcmp(sv.str + sv.len - suf.len, suf.str, suf.len);
}

static_fun const char* str_find_char(Strv sv, char c) {
  return sv.str ? (const char*)memchr(sv.str, (unsigned char)c, sv.len) : sv.str;
}

static_fun const char* str_find_strv(Strv hs, Strv ne) {
  if (ne.len == 0) { return hs.str; }
  if (ne.len > hs.len) { return rk_null; }
  if (ne.len <= 3) {
    for (size_t i = 0; i <= hs.len - ne.len; ++i) {
      if (!memcmp(hs.str + i, ne.str, ne.len)) { return hs.str + i; }
    }
    return rk_null;
  } else {
    const unsigned char* hstr = (const unsigned char*)hs.str;
    const unsigned char* nstr = (const unsigned char*)ne.str;
    size_t               skip[256];
    for (size_t j = 0; j < 256; ++j) { skip[j] = ne.len; }
    for (size_t j = 0; j < ne.len - 1; ++j) { skip[nstr[j]] = ne.len - j - 1; }
    for (size_t i = 0; i <= hs.len - ne.len;) {
      const unsigned char last = hstr[i + ne.len - 1];
      if (last == nstr[ne.len - 1] && !memcmp(hstr + i, nstr, ne.len - 1)) { return hs.str + i; }
      i += skip[last];
    }
    return rk_null;
  }
}
static_fun const char* str_findr_char(Strv sv, char c) {
  while (sv.len--) {
    if (sv.str[sv.len] == c) { return sv.str + sv.len; }
  }
  return rk_null;
}
static_fun const char* str_findr_strv(Strv hs, Strv ne) {
  if (ne.len == 0) { return hs.str; }
  if (ne.len > hs.len) { return rk_null; }
  for (size_t i = hs.len - ne.len + 1, j; i-- > 0;) {
    for (j = 0; j < ne.len; ++j) {
      if (hs.str[i + j] != ne.str[j]) { break; }
    }
    if (j == ne.len) { return hs.str + i; }
  }
  return rk_null;
}

static_fun bool str_contains_char(Strv sv, char c) {
  for (size_t i = 0; i < sv.len; ++i) {
    if (sv.str[i] == c) { return true; }
  }
  return false;
}

static_fun bool str_contains_strv(Strv s1, Strv s2) {
  for (size_t i = 0, j; i < s1.len && s1.len - i >= s2.len; ++i) {
    for (j = 0; j < s2.len; ++j) {
      if (s2.str[j] != s1.str[i + j]) { break; }
    }
    if (j == s2.len) { return true; }
  }
  return false;
}

static_fun Str RK__str_init(size_t cap RK_IFALLOC(, Allocator alloc)) {
  rk_assert_allocator_valid(alloc);
  cap       = stdc_bit_ceil(cap); // 1 if cap == 0 (-> space for terminator)
  char* str = alloc_new(char, cap RK_IFALLOC(, alloc));
  str[0]    = '\0';
  return (Str){.str = str, .len = 0, .cap = cap RK_IFALLOC(, .alloc = alloc)};
}
#define RK__STR_INIT(cap, alloc)  RK__str_init(cap RK_IFALLOC(, alloc))
#define RK__STR_INIT2(cap, alloc) rk_disable_if(RK__STR_INIT(cap, alloc))
#define RK__STR_INIT1(cap)        RK__STR_INIT(cap, alloc_ctx)

static_fun void RK__str_change_cap(Str* restrict self, size_t new_cap) {
  rk_set_alloc_fallback(self->alloc);
  self->str = alloc_renew(self->str, self->cap, new_cap RK_IFALLOC(, self->alloc));
  self->cap = new_cap;
}
static_fun void RK__str_ensure_cap(Str* restrict self, size_t new_cap) {
  if (new_cap > self->cap) { RK__str_change_cap(self, stdc_bit_ceil(new_cap)); }
}

static_fun const char* RK__str_front_ptr(Strv sv) {
  rk_assert(sv.len > 0 && "Cannot access first element of empty string");
  return sv.str;
}

static_fun const char* RK__str_back_ptr(Strv sv) {
  rk_assert(sv.len > 0 && "Cannot access last element of empty string");
  return sv.str + sv.len - 1;
}

static_fun Str str_from_strv(Strv sv RK_IFALLOC(, Allocator alloc)) {
  rk_assert_allocator_valid(alloc);
  size_t cap = stdc_bit_ceil(sv.len + 1);
  char*  str = alloc_new(char, cap RK_IFALLOC(, alloc));
  if (sv.len) { memcpy(str, sv.str, sv.len); }
  str[sv.len] = '\0';
  return (Str){.str = str, .len = sv.len, .cap = cap RK_IFALLOC(, .alloc = alloc)};
}
#define RK__STR_FROM(_S, alloc)  str_from_strv(strv_from(_S) RK_IFALLOC(, alloc))
#define RK__STR_FROM2(_S, alloc) rk_disable_if(RK__STR_FROM(_S, alloc))
#define RK__STR_FROM1(_S)                                                                          \
  RK__STR_FROM(_S, RK_IFALLOC(_Generic(_S, Str: RK__contrav(Str, _S).alloc, default: alloc_ctx)))

static_fun Str RK__str_join_strv_n(Strv* svs, size_t count,
                                   Strv sep RK_IFALLOC(, Allocator alloc)) {
  rk_assert_allocator_valid(alloc);
  Str res;
  res.len = 0;
  RK_IFALLOC(res.alloc = alloc;)
  if (count) {
    for (size_t i = 0; i < count; ++i) { res.len += svs[i].len; }
    res.len += sep.len * (count - 1);
  }
  res.cap    = stdc_bit_ceil(res.len + 1);
  res.str    = alloc_new(char, res.cap RK_IFALLOC(, alloc));
  size_t pos = 0;
  for (size_t i = 0; i < count; ++i) {
    if (svs[i].len) { memcpy(res.str + pos, svs[i].str, svs[i].len), pos += svs[i].len; }
    if (i + 1 < count && sep.len) { memcpy(res.str + pos, sep.str, sep.len), pos += sep.len; }
  }
  res.str[res.len] = '\0';
  return res;
}
// rk_disable_if

#define RK__STR_JOIN_STRV_N(svs, count, sep, alloc)                                                \
  RK__str_join_strv_n(svs, count, sep RK_IFALLOC(, alloc))
#define RK__STR_JOIN_STRV_N4(svs, count, sep, alloc)                                               \
  rk_disable_if(RK__STR_JOIN_STRV_N(svs, count, sep, alloc))
#define RK__STR_JOIN_STRV_N3(svs, count, sep) RK__STR_JOIN_STRV_N(svs, count, sep, alloc_ctx)

static_fun Str* str_assign_strv(Str* restrict self, Strv sv) {
  RK__str_ensure_cap(self, sv.len + 1);
  if (sv.len) { memcpy(self->str, sv.str, sv.len); }
  self->str[self->len = sv.len] = '\0';
  return self;
}

static_fun Strv strv_slice_strv(Strv sv, size_t start, size_t end) {
  if (start > sv.len || end < start) {
    sv.len = 0;
  } else {
    sv.str += start, sv.len = rk_MIN(end, sv.len) - start;
  }
  return sv;
}

static_fun Str* str_reserve(Str* restrict self, size_t new_cap) {
  RK__str_ensure_cap(self, new_cap);
  return self;
}

static_fun Str* str_resize(Str* restrict self, size_t new_len) {
  RK__str_ensure_cap(self, new_len + 1);
  return self->str[self->len = new_len] = '\0', self;
}

static_fun Str* str_shrink_to_fit(Str* restrict self) {
  size_t new_cap = stdc_bit_ceil(self->len + 1);
  if (self->cap > new_cap) { RK__str_change_cap(self, new_cap); }
  return self;
}
static_fun Str* str_shrink_to_fit_exact(Str* restrict self) {
  if (self->cap > self->len + 1) { RK__str_change_cap(self, self->len + 1); }
  return self;
}

static_fun Str* str_cat_strv(Str* restrict self, Strv sv) {
  if (!sv.len) { return self; }
  size_t new_len = sv.len + self->len;
  RK__str_ensure_cap(self, new_len + 1);
  memcpy(self->str + self->len, sv.str, sv.len);
  self->str[self->len = new_len] = '\0';
  return self;
}

static_fun Str* str_cat_strv_mayalias(Str* restrict self, Strv sv) {
  if (!sv.len) { return self; }
  size_t new_len = sv.len + self->len;
  if (new_len + 1 > self->cap) {
    uptr sbeg = (uptr)self->str, send = sbeg + self->len, cbeg = (uptr)sv.str;
    RK__str_ensure_cap(self, new_len + 1);
    if (cbeg >= sbeg && cbeg < send) {
      // offset of the char* into the str mem since a Str is not a
      // substring, char is always at higher address
      sv.str = self->str + (cbeg - sbeg);
    }
  }
  memcpy(self->str + self->len, sv.str, sv.len);
  self->str[self->len = new_len] = '\0';
  return self;
}

static_fun Str* str_insert_at_strv(Str* restrict self, size_t at, Strv sv) {
  rk_assert(at <= self->len && "Attempted to insert of out Str bounds");
  if (!sv.len) { return self; }
  if (at == self->len) { return str_cat_strv(self, sv); }
  size_t new_len = sv.len + self->len;
  RK__str_ensure_cap(self, new_len + 1);
  memmove(self->str + at + sv.len, self->str + at, self->len + 1 - at);
  memcpy(self->str + at, sv.str, sv.len);
  self->len = new_len;
  return self;
}

static_fun Str* str_insert_at_strv_mayalias(Str* restrict self, size_t idx, Strv sv) {
  rk_assert(idx <= self->len && "Attempted to insert out of Str bounds");
  if (!sv.len) { return self; }
  if (idx == self->len) { return str_cat_strv_mayalias(self, sv); }
  uptr  sbeg = (uptr)self->str, send = sbeg + self->len, cbeg = (uptr)sv.str;
  bool  alias = cbeg >= sbeg && cbeg < send;
  char* from;
  if (alias) {
    rk_set_alloc_fallback(self->alloc);
    from = alloc_new(char, sv.len RK_IFALLOC(, self->alloc));
    memcpy(from, sv.str, sv.len);
  } else {
    from = (char*)sv.str;
  }
  size_t new_len = sv.len + self->len;
  RK__str_ensure_cap(self, new_len + 1);
  memmove(self->str + idx + sv.len, self->str + idx, self->len + 1 - idx);
  memcpy(self->str + idx, from, sv.len);
  if (alias) { alloc_delete(from, sv.len RK_IFALLOC(, self->alloc)); }
  self->len = new_len;
  return self;
}

static_fun Str* str_insert_at_char(Str* restrict self, size_t at, char chr) {
  rk_assert(at <= self->len && "Attempted to insert out of Str bounds");
  if (at == self->len) { return str_push(self, chr); }
  RK__str_ensure_cap(self, self->len + 2);
  memmove(self->str + at + 1, self->str + at, self->len + 1 - at);
  self->str[at] = chr;
  ++self->len;
  return self;
}

static_fun Str* str_null_terminate(Str* restrict self) {
  RK__str_ensure_cap(self, self->len + 1);
  return str_null_terminate_unchecked(self);
}

static_fun Str* str_null_terminate_unchecked(Str* restrict self) {
  rk_assert(self->cap > self->len && "Attempted to push beyond the capacity of the Str");
  return self->str[self->len] = '\0', self;
}

static_fun Str* str_terminate_char(Str* restrict self, char suf) {
  if (!str_ends_with_char(self->v, suf)) { str_push(self, suf); }
  return self;
}

static_fun Str* str_terminate_strv(Str* restrict self, Strv suf) {
  if (!str_ends_with_strv(self->v, suf)) { str_cat_strv(self, suf); }
  return self;
}

static_fun Str* str_push(Str* restrict self, char c) {
  RK__str_ensure_cap(self, self->len + 2);
  return str_push_unchecked(self, c);
}
static_fun Str* str_push_unchecked(Str* restrict self, char c) {
  rk_assert(self->cap > self->len + 1 && "Attempted to push beyond the capacity of the Str");
  return self->str[self->len++] = c, str_null_terminate_unchecked(self);
}
static_fun Str* str_push_raw(Str* restrict self, char c) {
  RK__str_ensure_cap(self, self->len + 1);
  return str_push_unchecked_raw(self, c);
}
static_fun Str* str_push_unchecked_raw(Str* restrict self, char c) {
  rk_assert(self->cap > self->len && "Attempted to push beyond the capacity of the Str");
  return self->str[self->len++] = c, self;
}

static_fun Str* str_erase_at(Str* restrict self, size_t idx) {
  rk_assert(idx < self->len && "Erase out of bounds");
  memmove(self->str + idx, self->str + idx + 1, self->len - idx);
  return --self->len, self;
}

static_fun Str* str_erase_at_n(Str* restrict self, size_t idx, size_t count) {
  rk_assert(idx <= self->len && count <= self->len - idx && "Erase out of bounds");
  if (count) {
    memmove(self->str + idx, self->str + idx + count, self->len - idx - count + 1);
    self->len -= count;
  }
  return self;
}

static_fun Str* str_replace(Str* restrict self, char oldc, char newc) {
  for (size_t i = 0, len = self->len; i < len; ++i) {
    if (self->str[i] == oldc) { self->str[i] = newc; }
  }
  return self;
}

static_fun char str_replace_at(Str* restrict self, size_t pos, char c) {
  rk_assert(pos < self->len);
  rk_SWAP(c, self->str[pos]);
  return c;
}

static_fun Str* str_to_upper(Str* restrict self) {
  char* s = self->str;
  for (size_t i = 0, len = self->len; i < len; ++i) {
    s[i] -= (s[i] >= 'a' && s[i] <= 'z') * ('a' - 'A');
  }
  return self;
}
static_fun Str* str_to_lower(Str* restrict self) {
  char* s = self->str;
  for (size_t i = 0, len = self->len; i < len; ++i) {
    s[i] += (s[i] >= 'A' && s[i] <= 'Z') * ('a' - 'A');
  }
  return self;
}
static_fun Str* str_reverse(Str* restrict self) {
  for (size_t i = 0, len = self->len; i < len / 2; ++i) {
    rk_SWAP(self->str[i], self->str[len - 1 - i]);
  }
  return self;
}

#define RK__STR_CHAR_ISSPACE(c) ((c) == ' ' || ((c) >= '\t' && (c) <= '\r'))
static_fun Strv str_trimmed_left_strv(Strv sv) {
  size_t i = 0;
  for (; i < sv.len && RK__STR_CHAR_ISSPACE(sv.str[i]); ++i);
  if (sv.str) { sv.str += i, sv.len -= i; }
  return sv;
}
static_fun Strv str_trimmed_right_strv(Strv sv) {
  for (; sv.len && RK__STR_CHAR_ISSPACE(sv.str[sv.len - 1]); --sv.len);
  return sv;
}
static_fun Strv str_trimmed_strv(Strv sv) {
  return str_trimmed_right_strv(str_trimmed_left_strv(sv));
}
#undef RK__STR_CHAR_ISSPACE

static_fun Strv*(str_split_alloc)(Strv str, Strv dels,
                                  size_t* restrict out_count RK_IFALLOC(, Allocator alloc)) {
  rk_assert_allocator_valid(alloc);
  typedef unsigned char uchar;
  uchar                 dbits[256] = {RK_ZINIT};
  for (size_t i = 0; i < dels.len; ++i) { dbits[(uchar)dels.str[i]] = 1; }
  size_t count = 1;
  for (size_t i = 0; i < str.len; ++i) { count += dbits[(uchar)str.str[i]]; }
  Strv*  data = alloc_new(Strv, count RK_IFALLOC(, alloc));
  size_t cidx = 0, start = 0;
  for (size_t i = 0; i < str.len; ++i) {
    if (dbits[(uchar)str.str[i]]) {
      data[cidx++] = (Strv){.str = str.str + start, .len = i - start};
      start        = i + 1;
    }
  }
  data[cidx++] = (Strv){.str = str.str + start, .len = str.len - start};
  *out_count   = count;
  return data;
}
#define RK__STR_SPLIT_ALLOC(str, delims, count, alloc)                                             \
  str_split_alloc(str, delims, count RK_IFALLOC(, alloc))
#define RK__STR_SPLIT_ALLOC4(str, delims, count, alloc)                                            \
  rk_disable_if(RK__STR_SPLIT_ALLOC(str, delims, count, alloc))
#define RK__STR_SPLIT_ALLOC3(str, delims, count) RK__STR_SPLIT_ALLOC(str, delims, count, alloc_ctx)

rk_attr_printf(2, 3) static_fun Str* str_cat_fmt(Str* self, const char* fmt, ...) {
  size_t  len = self->len, rem = self->cap - len;
  va_list ap, ap_probe;
  va_start(ap, fmt), va_copy(ap_probe, ap);
  int n = vsnprintf(self->str ? self->str + len : rk_null, rem, fmt, ap_probe);
  va_end(ap_probe);
  if (n < 0) { return va_end(ap), rk_null; }
  if (rem < (size_t)n + 1) {
    RK__str_ensure_cap(self, len + (size_t)n + 1);
    n = vsnprintf(self->str + len, self->cap - len, fmt, ap);
    if (n < 0) { return va_end(ap), rk_null; }
  }
  self->len = len + (size_t)n;
  return va_end(ap), self;
}

static_fun Strv str_split_char(Strv* self, char delim) {
  Strv tok = {.str = self->str, .len = 0};
  if (!self->len) { return self->len = STR_SPLIT_END, tok; } // uses unsigned wraparound
  const char* p  = str_find_char(*self, delim);
  tok.len        = p ? (size_t)(p - tok.str) : self->len;
  self->str     += tok.len + (p != rk_null);
  self->len     -= tok.len + 1;
  return tok;
}
static_fun Strv str_split_strv(Strv* self, Strv dels) {
  Strv tok = {.str = self->str, .len = 0};
  if (!self->len) { return self->len = STR_SPLIT_END, tok; }
  size_t i = 0;
  for (; i < self->len && !str_contains_char(dels, tok.str[i]); ++i);
  tok.len = i;
  if (i != self->len) {
    self->str += tok.len + 1, self->len -= tok.len + 1;
  } else {
    self->str += tok.len, self->len = STR_SPLIT_END;
  }
  return tok;
}

/// @endcond

RK_HEADER_END
/// @}
#endif // RK_STRING_H

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
