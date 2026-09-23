/// @file rk_bitset.h
/// @version 1.0
/// @defgroup rk_bitset Bitset Utilities
/// @brief Bitset utilities.
///
/// This header provides a lightweight, bitset representation and helper functions for bit
/// manipulation. Bit numbering is **0-based**: bit index 0 refers to the **least-significant bit
/// (LSB)** of `bs[0]`.
///
/// ## Storage model A bitset is stored as an array of `bitset_word` (by default `unsigned long
/// long`):
/// - `bs[0]` holds bits `[0 .. W-1]`
/// - `bs[1]` holds bits `[W .. 2W-1]`
/// - etc., where `W = bitsof(bitset_word)`
///
/// ## Padding bits (important) For a logical bitset size `nbits`, the last storage word may contain
/// *padding bits* with indices `>= nbits`. Many operations in this header (e.g. `bitset_any`,
/// `bitset_count_ones`, `bitset_equals`) operate on whole words, so callers, if manipulating the
/// bitset outside of the functions defined here, must maintain the invariant.
///
/// @par Padding invariant **All padding bits (indices `>= nbits`) are zero.**
///
/// Functions that write whole words and are documented to preserve correctness will clear padding
/// bits on return (e.g. `bitset_set_all`, shifts, `bitset_not`, `bitset_sub`). If you introduce
/// data by other means (e.g. uninitialized storage, raw `memcpy`, manual word writes), call
/// `bitset_clear_padding()`.
///
/// ## Relationship to C23 stdbit.h conventions Single-bit and range operations use 0-based bit
/// indices. The query functions `bitset_first_*` follow the C23 `<stdbit.h>` / common builtin
/// convention of returning a **1-based position**, with **0** as the sentinel value meaning “not
/// found”.
///
/// @see stdbit.h
/// @see rk_defs.h
/// @{

#ifndef RK_BITSET_H
#define RK_BITSET_H
#include "rk_defs.h"
RK_HEADER_BEGIN

/// @brief Storage word used by all bitset operations.
typedef unsigned long long bitset_word;

/// @brief Mutable bitset pointer (points to the first word).
typedef bitset_word*       bitset;

/// @brief Const bitset pointer (points to the first word).
typedef const bitset_word* cbitset;

/// @brief Declares a fixed-size bitset object type with storage sufficient for `nbits` bits while
/// preserving the size in the type system.
/// @param nbits Logical size of the bitset in bits. Must be > 0.
///
/// Usage:
/// ```c
/// bitset(128) bs = {0}; // 128-bit bitset (array of bitset_word)
/// ```
/// @note This macro expects `nbits` to be an integer constant expression when used for object
/// declarations.
#define bitset(nbits)                                                                              \
  typeof(bitset_word[(static_assert_expr((nbits) > 0, "Bitset must have at least one bit")         \
                      + bitset_words(nbits))])

/// @brief Returns the number of storage words required for a `nbits`-bit bitset.
/// @param nbits Logical size in bits
#define bitset_words(nbits)    (((nbits) + bitsof(bitset_word) - 1) / bitsof(bitset_word))

/// @brief Returns the number of bytes required for a `nbits`-bit bitset.
/// @param nbits Logical size in bits
#define bitset_bytes(nbits)    (sizeof(bitset_word) * bitset_words(nbits))

/// @brief Returns the number of bits in each bitset word.
#define bitset_word_bits       bitsof(bitset_word)

/// @brief Returns the 0-based storage word index containing bit `idx`.
/// @param idx Bit index (0-based)
#define bitset_word_index(idx) ((idx) / bitsof(bitset_word))

/// @brief Mask for the bit within its storage word.
/// @param idx Global bit index (0-based)
/// @return A word mask with that bit set.
#define bitset_word_mask(idx)  ((bitset_word)1 << ((idx) % bitsof(bitset_word)))

/// @brief Copies a bitset.
/// @param dst Destination bitset (at least `bitset_words(nbits)` words).
/// @param nbits Logical size of the bitset
/// @param src Source bitset storage (at least `bitset_words(nbits)` words)
/// @return `dst` (for convenience).
/// @note This copies whole words. If you rely on the padding invariant, ensure `src` has cleared
/// padding.
static_fun bitset bitset_copy(bitset restrict dst, size_t nbits, cbitset restrict src) {
  return rk_copy(dst, src, bitset_words(nbits));
}

#define rk_assert_bitset_in_bounds(idx, nbits)                                                     \
  rk_assert((idx) < (nbits) && "Index out of Bitset bounds.")

/// @brief Tests whether bit `idx` is set.
/// @param bs,nbits Bitset and its logical size
/// @param idx Bit index (0-based)
/// @return `true` if bit `idx` is 1, otherwise `false`.
/// @pre `idx < nbits`.
static_fun bool bitset_test(cbitset bs, size_t nbits, size_t idx) {
  rk_assert_bitset_in_bounds(idx, nbits), (void)nbits;
  return (bs[bitset_word_index(idx)] & bitset_word_mask(idx)) != 0;
}

/// @brief Clears bit `idx` (sets it to 0).
/// @param bs,nbits Bitset and its logical size
/// @param idx Bit index (0-based)
/// @return `bs` (for chaining).
/// @pre `idx < nbits`.
static_fun bitset bitset_clear(bitset bs, size_t nbits, size_t idx) {
  rk_assert_bitset_in_bounds(idx, nbits), (void)nbits;
  bs[bitset_word_index(idx)] &= ~bitset_word_mask(idx);
  return bs;
}

/// @brief Sets bit `idx` (sets it to 1).
/// @param bs,nbits Bitset and its logical size
/// @param idx Bit index (0-based)
/// @return `bs` (for chaining).
/// @pre `idx < nbits`.
static_fun bitset bitset_set(bitset bs, size_t nbits, size_t idx) {
  rk_assert_bitset_in_bounds(idx, nbits), (void)nbits;
  bs[bitset_word_index(idx)] |= bitset_word_mask(idx);
  return bs;
}

/// @brief Writes bit `idx` to `value`.
/// @param bs,nbits Bitset and its logical size
/// @param idx Bit index (0-based)
/// @param value New bit value (`false` -> 0, `true` -> 1)
/// @return `bs` (for chaining).
/// @pre `idx < nbits`.
static_fun bitset bitset_write(bitset bs, size_t nbits, size_t idx, bool value) {
  rk_assert_bitset_in_bounds(idx, nbits), (void)nbits;
  bitset_word mask = bitset_word_mask(idx);
  size_t      w    = bitset_word_index(idx);
  bs[w]            = (bs[w] & ~mask) | (-((bitset_word)value) & mask);
  return bs;
}

/// @brief Toggles bit `idx`.
/// @param bs,nbits Bitset and its logical size
/// @param idx Bit index (0-based)
/// @return `bs` (for chaining).
/// @pre `idx < nbits`.
static_fun bitset bitset_flip(bitset bs, size_t nbits, size_t idx) {
  rk_assert_bitset_in_bounds(idx, nbits), (void)nbits;
  bs[bitset_word_index(idx)] ^= bitset_word_mask(idx);
  return bs;
}

static_fun rk_forceinline bitset RK__internal_bitset_range_op(bitset, size_t, size_t, size_t, int);

/// @brief Clears all bits in the half-open interval `[start, end)`.
/// @param bs,nbits Bitset and its logical size
/// @param start First bit index (inclusive)
/// @param end Last bit index (exclusive)
/// @return `bs` (for chaining).
/// @pre `start <= end && end <= nbits`.
static_fun bitset bitset_clear_range(bitset bs, size_t nbits, size_t start, size_t end) {
  return RK__internal_bitset_range_op(bs, nbits, start, end, 0);
}

/// @brief Sets all bits in the half-open interval `[start, end)`.
/// @param bs,nbits Bitset and its logical size
/// @param start First bit index (inclusive)
/// @param end Last bit index (exclusive)
/// @return `bs` (for chaining).
/// @pre `start <= end && end <= nbits`.
static_fun bitset bitset_set_range(bitset bs, size_t nbits, size_t start, size_t end) {
  return RK__internal_bitset_range_op(bs, nbits, start, end, 1);
}

/// @brief Writes all bits in `[start, end)` to `value`.
/// @param bs,nbits Bitset and its logical size
/// @param start First bit index (inclusive)
/// @param end Last bit index (exclusive)
/// @param value New bit value (`false` -> 0, `true` -> 1)
/// @return `bs` (for chaining).
/// @pre `start <= end && end <= nbits`.
static_fun bitset bitset_write_range(bitset bs, size_t nbits, size_t start, size_t end,
                                     bool value) {
  return value ? bitset_set_range(bs, nbits, start, end)
               : bitset_clear_range(bs, nbits, start, end);
}

/// @brief Toggles all bits in `[start, end)`.
/// @param bs,nbits Bitset and its logical size
/// @param start First bit index (inclusive)
/// @param end Last bit index (exclusive)
/// @return `bs` (for chaining).
/// @pre `start <= end && end <= nbits`.
static_fun bitset bitset_flip_range(bitset bs, size_t nbits, size_t start, size_t end) {
  return RK__internal_bitset_range_op(bs, nbits, start, end, -1);
}

/// @brief Clears any padding bits (indices `>= nbits`) in the last storage word.
/// @param bs,nbits Bitset and its logical size
/// @return `bs` (for chaining).
/// @note Call this if `bs` may contain nonzero padding bits (e.g. after uninitialized allocation or
/// raw word operations).
static_fun bitset bitset_clear_padding(bitset bs, size_t nbits) {
  size_t words = bitset_words(nbits), rest = nbits % bitset_word_bits;
  if (rest) { bs[words - 1] &= (((bitset_word)1 << rest) - 1); }
  return bs;
}

/// @brief Clears all bits to 0.
/// @param bs,nbits Bitset and its logical size
/// @return `bs` (for chaining).
static_fun bitset bitset_clear_all(bitset bs, size_t nbits) {
  return (bitset)memset(bs, 0, sizeof_n(*bs, bitset_words(nbits)));
}

/// @brief Sets all bits to 1 (and clears padding bits).
/// @param bs,nbits Bitset and its logical size
/// @return `bs` (for chaining).
static_fun bitset bitset_set_all(bitset bs, size_t nbits) {
  memset(bs, 0xFF, sizeof_n(*bs, bitset_words(nbits)));
  return bitset_clear_padding(bs, nbits);
}

/// @brief Sets all bits to `value`.
/// @param bs,nbits Bitset and its logical size
/// @param value New bit value (`false` -> 0, `true` -> 1)
/// @return `bs` (for chaining).
static_fun bitset bitset_write_all(bitset bs, size_t nbits, bool value) {
  return value ? bitset_set_all(bs, nbits) : bitset_clear_all(bs, nbits);
}

/// @brief Toggles all bits (and clears padding bits).
/// @param bs,nbits Bitset and its logical size
/// @return `bs` (for chaining).
static_fun bitset bitset_flip_all(bitset bs, size_t nbits) {
  size_t words = bitset_words(nbits);
  for (size_t w = 0; w < words; ++w) { bs[w] = ~bs[w]; }
  return bitset_clear_padding(bs, nbits);
}

/// @brief Finds the first set bit when scanning from MSB to LSB.
/// @param bs,nbits Bitset and its logical size
/// @return A **1-based** position of the first leading one, or 0 if none.
/// @note This matches the “1-based with 0 sentinel” convention used by C23 `<stdbit.h>` query
/// functions and several compiler builtins.
static_fun size_t bitset_first_leading_one(cbitset bs, size_t nbits) {
  rk_assert(nbits > 0);
  size_t rest = nbits % bitset_word_bits, w = bitset_words(nbits) - 1, pos;
  if (rest) {
    pos = stdc_first_leading_one(bs[w] << (bitset_word_bits - rest));
    if (pos || !w--) { return pos; }
  }
  for (;; --w, rest += bitset_word_bits) {
    if ((pos = stdc_first_leading_one(bs[w]))) { return pos + rest; }
    if (!w) { break; }
  }
  return 0;
}

/// @brief Finds the first zero bit when scanning from MSB to LSB.
/// @param bs,nbits Bitset and its logical size
/// @return A **1-based** position of the first leading zero, or 0 if none.
/// @note If all valid bits are 1, returns 0.
static_fun size_t bitset_first_leading_zero(cbitset bs, size_t nbits) {
  rk_assert(nbits > 0);
  size_t rest = nbits % bitset_word_bits, w = bitset_words(nbits) - 1, pos;
  if (rest) {
    pos = stdc_first_leading_zero((bs[w] << (bitset_word_bits - rest))
                                  | (((bitset_word)1 << (bitset_word_bits - rest)) - 1));
    if (pos) { return pos; }
    if (!w--) { return 0; }
  }
  for (;; --w, rest += bitset_word_bits) {
    if ((pos = stdc_first_leading_zero(bs[w]))) { return pos + rest; }
    if (!w) { break; }
  }
  return 0;
}

/// @brief Finds the first set bit when scanning from LSB to MSB.
/// @param bs,nbits Bitset and its logical size
/// @return A **1-based** position of the first trailing one, or 0 if none.
static_fun size_t bitset_first_trailing_one(cbitset bs, size_t nbits) {
  rk_assert(nbits > 0);
  for (size_t w = 0, words = bitset_words(nbits); w < words; ++w) {
    size_t pos = stdc_first_trailing_one(bs[w]);
    if (pos) { return pos + bitset_word_bits * w; }
  }
  return 0;
}

/// @brief Finds the first zero bit when scanning from LSB to MSB.
/// @param bs,nbits Bitset and its logical size
/// @return A **1-based** position of the first trailing zero, or 0 if none.
/// @note Padding bits are treated as 1 (not eligible as “zero” results).
static_fun size_t bitset_first_trailing_zero(cbitset bs, size_t nbits) {
  rk_assert(nbits > 0);
  size_t rest = nbits % bitset_word_bits, w = 0, pos;
  for (size_t words = bitset_words(nbits); w < words - (rest != 0); ++w) {
    pos = stdc_first_trailing_zero(bs[w]);
    if (pos) { return pos + bitset_word_bits * w; }
  }
  if (rest) {
    bitset_word lw = bs[w] | (bitset_word) ~(((bitset_word)1 << rest) - 1);
    pos            = stdc_first_trailing_zero(lw);
    if (pos) { return pos + bitset_word_bits * w; }
  }
  return 0;
}

/// @brief Counts leading zeros (from MSB toward LSB).
/// @param bs,nbits Bitset and its logical size
/// @return Number of consecutive zero bits starting at the MSB.
static_fun size_t bitset_leading_zeros(cbitset bs, size_t nbits) {
  size_t pos = bitset_first_leading_one(bs, nbits);
  return pos ? pos - 1 : nbits;
}

/// @brief Counts leading ones (from MSB toward LSB).
/// @param bs,nbits Bitset and its logical size
/// @return Number of consecutive one bits starting at the MSB.
static_fun size_t bitset_leading_ones(cbitset bs, size_t nbits) {
  size_t pos = bitset_first_leading_zero(bs, nbits);
  return pos ? pos - 1 : nbits;
}

/// @brief Counts trailing zeros (from LSB toward MSB).
/// @param bs,nbits Bitset and its logical size
/// @return Number of consecutive zero bits starting at the LSB.
static_fun size_t bitset_trailing_zeros(cbitset bs, size_t nbits) {
  size_t pos = bitset_first_trailing_one(bs, nbits);
  return pos ? pos - 1 : nbits;
}

/// @brief Counts trailing ones (from LSB toward MSB).
/// @param bs,nbits Bitset and its logical size
/// @return Number of consecutive one bits starting at the LSB.
static_fun size_t bitset_trailing_ones(cbitset bs, size_t nbits) {
  size_t pos = bitset_first_trailing_zero(bs, nbits);
  return pos ? pos - 1 : nbits;
}

/// @brief Counts the number of 1 bits.
/// @param bs,nbits Bitset and its logical size
/// @return Number of set bits.
static_fun size_t bitset_count_ones(cbitset bs, size_t nbits) {
  size_t words = bitset_words(nbits), count = 0;
  for (size_t w = 0; w < words; ++w) { count += stdc_count_ones(bs[w]); }
  return count;
}

/// @brief Counts the number of 0 bits.
/// @param bs,nbits Bitset and its logical size
/// @return Number of zero bits.
static_fun size_t bitset_count_zeros(cbitset bs, size_t nbits) {
  return nbits - bitset_count_ones(bs, nbits);
}

/// @brief Returns whether any bit is set.
/// @param bs,nbits Bitset and its logical size
/// @return `true` if at least one valid bit is 1, else `false`.
static_fun bool bitset_any(cbitset bs, size_t nbits) {
  for (size_t w = 0, words = bitset_words(nbits); w < words; ++w) {
    if (bs[w]) { return true; }
  }
  return false;
}

/// @brief Returns whether no bits are set.
/// @param bs,nbits Bitset and its logical size
/// @return `true` if all valid bits are 0, else `false`.
static_fun bool bitset_none(cbitset bs, size_t nbits) { return !bitset_any(bs, nbits); }

/// @brief Returns whether all bits are set.
/// @param bs,nbits Bitset and its logical size
/// @return `true` if all valid bits are 1, else `false`.
static_fun bool bitset_all(cbitset bs, size_t nbits) {
  size_t rest = nbits % bitset_word_bits, w = 0;
  for (size_t words = bitset_words(nbits); w < words - (rest != 0); ++w) {
    if (bs[w] != ((bitset_word)~0)) { return false; }
  }
  if (rest) { return bs[w] == (((bitset_word)1 << rest) - 1); }
  return true;
}

/// @brief Returns whether exactly one bit is set.
/// @param bs,nbits Bitset and its logical size
/// @return `true` if exactly one valid bit is 1, else `false`.
static_fun bool bitset_has_single_bit(cbitset bs, size_t nbits) {
  size_t count = 0;
  for (size_t w = 0, words = bitset_words(nbits); w < words; ++w) {
    if ((count += stdc_count_ones(bs[w])) > 1) { return false; }
  }
  return count == 1;
}

/// @brief Compares two bitsets for equality.
/// @param a First bitset
/// @param nbits Logical size of both bitsets
/// @param b Second bitset
/// @return `true` if all valid bits match, else `false`.
static_fun bool bitset_equals(cbitset a, size_t nbits, cbitset b) {
  return rk_memcmp(a, b, sizeof_n(*a, bitset_words(nbits))) == 0;
}

/// @brief Bitwise OR (in-place): `dst |= src`.
/// @param dst Destination bitset (modified)
/// @param nbits Logical size of both bitsets
/// @param src Source bitset
/// @return `dst` (for chaining).
/// @pre `dst` and `src` are both valid for `nbits` bits.
static_fun bitset bitset_or(bitset dst, size_t nbits, cbitset src) {
  for (size_t w = 0, words = bitset_words(nbits); w < words; ++w) { dst[w] |= src[w]; }
  return dst;
}

/// @brief Bitwise AND (in-place): `dst &= src`.
/// @param dst Destination bitset (modified)
/// @param nbits Logical size of both bitsets
/// @param src Source bitset
/// @return `dst` (for chaining).
static_fun bitset bitset_and(bitset dst, size_t nbits, cbitset src) {
  for (size_t w = 0, words = bitset_words(nbits); w < words; ++w) { dst[w] &= src[w]; }
  return dst;
}

/// @brief Bitwise XOR (in-place): `dst ^= src`.
/// @param dst Destination bitset (modified)
/// @param nbits Logical size of both bitsets
/// @param src Source bitset
/// @return `dst` (for chaining).
/// @note With the padding invariant, padding remains zero because `0 ^ 0 == 0`.
static_fun bitset bitset_xor(bitset dst, size_t nbits, cbitset src) {
  for (size_t w = 0, words = bitset_words(nbits); w < words; ++w) { dst[w] ^= src[w]; }
  return dst;
}

/// @brief Bitwise subtraction (in-place): clears bits present in `src` (`dst &= ~src`).
/// @param dst Destination bitset (modified)
/// @param nbits Logical size of both bitsets
/// @param src Source bitset
/// @return `dst` (for chaining).
static_fun bitset bitset_sub(bitset dst, size_t nbits, cbitset src) {
  for (size_t w = 0, words = bitset_words(nbits); w < words; ++w) { dst[w] &= ~src[w]; }
  return bitset_clear_padding(dst, nbits);
}

/// @brief Bitwise NOT: `dst = ~src` (clears padding on return).
/// @param dst Destination bitset (modified)
/// @param nbits Logical size of both bitsets
/// @param src Source bitset
/// @return `dst` (for chaining).
static_fun bitset bitset_not(bitset dst, size_t nbits, cbitset src) {
  for (size_t w = 0, words = bitset_words(nbits); w < words; ++w) { dst[w] = ~src[w]; }
  return bitset_clear_padding(dst, nbits);
}

/// @brief Left-shifts `src` by `sh` bits into `dst`.
/// @param dst Destination bitset (modified)
/// @param nbits Logical size of both bitsets
/// @param src Source bitset
/// @param sh Shift amount in bits
/// @return `dst` (for chaining).
/// @note If `sh >= nbits`, the result is all zeros.
/// @note `dst` may alias `src`.
static_fun bitset bitset_shift_left_into(bitset dst, size_t nbits, cbitset src, size_t sh) {
  if (sh >= nbits) { return bitset_clear_all(dst, nbits), dst; }
  if (!sh) { return (cbitset)dst != src ? bitset_copy(dst, nbits, src) : dst; }
  size_t ws = sh / bitsof(*dst), bs = sh % bitsof(*dst);
  for (size_t i = bitset_words(nbits); i-- > 0;) {
    if (i < ws) {
      dst[i] = 0;
      continue;
    }
    size_t      si = i - ws;
    bitset_word v  = (bitset_word)src[si] << bs;
    if (bs && si > 0) { v |= (bitset_word)src[si - 1] >> (bitsof(*dst) - bs); }
    dst[i] = v;
  }
  return bitset_clear_padding(dst, nbits);
}

/// @brief In-place left shift: `bs <<= sh`.
/// @param bs,nbits Bitset and its logical size
/// @param sh Shift amount in bits
/// @return `bs` (for chaining).
static_fun bitset bitset_shift_left(bitset bs, size_t nbits, size_t sh) {
  return bitset_shift_left_into(bs, nbits, bs, sh);
}

/// @brief Right-shifts `src` by `sh` bits into `dst`.
/// @param dst Destination bitset (modified)
/// @param nbits Logical size of both bitsets
/// @param src Source bitset
/// @param sh Shift amount in bits
/// @return `dst` (for chaining).
/// @note If `sh >= nbits`, the result is all zeros.
/// @note `dst` may alias `src`.
static_fun bitset bitset_shift_right_into(bitset dst, size_t nbits, cbitset src, size_t sh) {
  if (sh >= nbits) { return bitset_clear_all(dst, nbits), dst; }
  if (!sh) { return (cbitset)dst != src ? bitset_copy(dst, nbits, src) : dst; }
  size_t ws = sh / bitsof(*dst), bs = sh % bitsof(*dst);
  for (size_t i = 0, words = bitset_words(nbits); i < words; ++i) {
    size_t si = i + ws;
    if (si >= words) {
      dst[i] = 0;
      continue;
    }
    bitset_word v = (bitset_word)src[si] >> bs;
    if (bs && (si + 1) < words) { v |= (bitset_word)src[si + 1] << (bitsof(*dst) - bs); }
    dst[i] = v;
  }
  bitset_clear_padding(dst, nbits);
  return dst;
}

/// @brief In-place right shift: `bs >>= sh`.
/// @param bs,nbits Bitset and its logical size
/// @param sh Shift amount in bits
/// @return `bs` (for chaining).
static_fun bitset bitset_shift_right(bitset bs, size_t nbits, size_t sh) {
  return bitset_shift_right_into(bs, nbits, bs, sh);
}

#define BITSET_NPOS SIZE_MAX

/// @brief Finds the next set bit after `cur` when scanning from LSB to MSB.
/// @param cur Previously visited bit index, or `BITSET_NPOS` to start from the beginning
/// @return The index (0-based) of the first set bit with index `> cur`, or `BITSET_NPOS` if no such
/// bit exists.
/// @note This is an efficient iteration primitive for walking only the set bits. It scans by
/// storage word and uses trailing-one queries, so it is typically much faster than testing every
/// bit individually.
///
/// Usage:
/// ```c
/// bitset(128) bs = {0};
/// bitset_set(bs, 128, 1), bitset_set(bs, 128, 5), bitset_set(bs, 128, 64);
/// size_t i = BITSET_NPOS;
/// for (;(i = bitset_find_next_set(bs, 128, i)) != BITSET_NPOS;) {
///    printf("set bit: %zu\n", i);
/// }
/// // prints: 1, 5, 64
/// ```
static_fun size_t bitset_find_next_set(cbitset bs, size_t nbits, size_t cur) {
  enum { W = bitsof(*bs) }; // NOLINT
  if (++cur >= nbits) { return BITSET_NPOS; }
  size_t res, w = bitset_word_index(cur);
  if ((res = stdc_first_trailing_one(bs[w] & (~(bitset_word)0 << (cur % W))))) {
    return w * W + res - 1;
  }
  for (size_t words = bitset_words(nbits); ++w < words;) {
    if ((res = stdc_first_trailing_one(bs[w]))) { return w * W + res - 1; }
  }
  return BITSET_NPOS;
}
/// @brief Finds the next zero bit after `cur` when scanning from LSB to MSB.
/// @param bs,nbits Bitset and its logical size.
/// @param cur Previously visited bit index, or `BITSET_NPOS` to start from the beginning.
/// @return The index (0-based) of the first zero bit with index `> cur`, or `BITSET_NPOS` if no
/// such bit exists.
static_fun size_t bitset_find_next_clear(cbitset bs, size_t nbits, size_t cur) {
  enum { W = bitsof(*bs) }; // NOLINT
  if (++cur >= nbits) { return BITSET_NPOS; }
  size_t      res, rest = nbits % W, w = bitset_word_index(cur);
  bitset_word valid
      = (rest && w == bitset_words(nbits) - 1) ? (((bitset_word)1 << rest) - 1) : ~(bitset_word)0;
  if ((res = stdc_first_trailing_one(~bs[w] & valid & (~(bitset_word)0 << (cur % W))))) {
    return w * W + res - 1;
  }
  for (size_t words = bitset_words(nbits); ++w < words;) {
    valid = (rest && w == words - 1) ? (((bitset_word)1 << rest) - 1) : ~(bitset_word)0;
    if ((res = stdc_first_trailing_one(~bs[w] & valid))) { return w * W + res - 1; }
  }
  return BITSET_NPOS;
}

/// @brief Finds the previous set bit before `cur` when scanning from MSB to LSB.
/// @param bs,nbits Bitset and its logical size.
/// @param cur Current bit index, or `nbits` to start from the end.
/// @return The index (0-based) of the last set bit with index `< cur`, or `BITSET_NPOS` if no such
/// bit exists.
static_fun size_t bitset_find_prev_set(cbitset bs, size_t nbits rk_unused, size_t cur) {
  enum { W = bitsof(*bs) }; // NOLINT
  if (!cur) { return BITSET_NPOS; }
  --cur;
  size_t      res, w = bitset_word_index(cur);
  bitset_word mask = ~(bitset_word)0 >> (W - 1 - (cur % W));
  if ((res = stdc_first_leading_one(bs[w] & mask))) { return w * W + (W - res); }
  for (; w--;) {
    if ((res = stdc_first_leading_one(bs[w]))) { return w * W + (W - res); }
  }
  return BITSET_NPOS;
}

/// @brief Finds the previous zero bit before `cur` when scanning from MSB to LSB.
/// @param bs,nbits Bitset and its logical size.
/// @param cur Current bit index, or `nbits` to start from the end.
/// @return The index (0-based) of the last zero bit with index `< cur`, or `BITSET_NPOS` if no such
/// bit exists.
static_fun size_t bitset_find_prev_clear(cbitset bs, size_t nbits, size_t cur) {
  enum { W = bitsof(*bs) }; // NOLINT
  if (!cur) { return BITSET_NPOS; }
  --cur;
  size_t      res, rest = nbits % W, w = bitset_word_index(cur);
  bitset_word valid
      = (rest && w == bitset_words(nbits) - 1) ? (((bitset_word)1 << rest) - 1) : ~(bitset_word)0;
  bitset_word mask = ~(bitset_word)0 >> (W - 1 - (cur % W));
  if ((res = stdc_first_leading_one(~bs[w] & valid & mask))) { return w * W + (W - res); }
  for (; w--;) {
    if ((res = stdc_first_leading_one(~bs[w]))) { return w * W + (W - res); }
  }
  return BITSET_NPOS;
}

/// @brief Finds the first set bit when scanning from LSB to MSB.
/// @param bs,nbits Bitset and its logical size.
/// @return The index (0-based) of the first set bit, or `BITSET_NPOS` if none.
static_fun size_t bitset_find_first_set(cbitset bs, size_t nbits) {
  return bitset_find_next_set(bs, nbits, BITSET_NPOS);
}

/// @brief Finds the first zero bit when scanning from LSB to MSB.
/// @param bs,nbits Bitset and its logical size.
/// @return The index (0-based) of the first zero bit, or `BITSET_NPOS` if none.
static_fun size_t bitset_find_first_clear(cbitset bs, size_t nbits) {
  return bitset_find_next_clear(bs, nbits, BITSET_NPOS);
}

/// @brief Finds the last set bit when scanning from MSB to LSB.
/// @param bs,nbits Bitset and its logical size.
/// @return The index (0-based) of the last set bit, or `BITSET_NPOS` if none.
static_fun size_t bitset_find_last_set(cbitset bs, size_t nbits) {
  return bitset_find_prev_set(bs, nbits, nbits);
}

/// @brief Finds the last zero bit when scanning from MSB to LSB.
/// @param bs,nbits Bitset and its logical size.
/// @return The index (0-based) of the last zero bit, or `BITSET_NPOS` if none.
static_fun size_t bitset_find_last_clear(cbitset bs, size_t nbits) {
  return bitset_find_prev_clear(bs, nbits, nbits);
}

/// @brief Converts a bitset to a binary string (MSB first).
/// @param dst Destination char buffer of size at least `nbits + 1`
/// @param src Source bitset
/// @param nbits Logical size of the bitset
/// @return `dst` (for convenience). The output is `nbits` characters of `'0'`/`'1'`, plus a
/// trailing `'\0'`.
static_fun char* bitset_tostr(char* restrict dst, cbitset restrict src, size_t nbits) {
  for (size_t w = 0; w < nbits; ++w) { dst[w] = '0' + bitset_test(src, nbits, nbits - 1 - w); }
  dst[nbits] = '\0';
  return dst;
}

/// @brief Parses a binary string (MSB first) into a `len`-bit bitset.
/// @param dst Destination bitset storage for a **len-bit** bitset (at least `bitset_words(len)`
/// words).
/// @param src Source string of at least `len` characters, consisting only of `'0'` and `'1'`. The
/// first character corresponds to the MSB.
/// @param len Number of bits to parse; also the logical size of the resulting bitset.
/// @return `dst` (for chaining).
/// @note This function treats `dst` as a `len`-bit bitset for this call. It writes all bits `[0,
/// len)` and clears padding bits on return.
static_fun bitset bitset_fromstr(bitset dst, const char* restrict src, size_t len) {
  bitset_clear_all(dst, len);
  for (size_t w = 0; w < len; ++w) {
    rk_assert((src[w] == '0' || src[w] == '1') && "Invalid character in bitset string");
    bitset_write(dst, len, len - 1 - w, src[w] != '0');
  }
  return bitset_clear_padding(dst, len);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

/// @brief Internal helper implementing range operations.
/// @param bs Bitset to modify
/// @param nbits Logical size of the bitset
/// @param start First bit index (inclusive)
/// @param end Last bit index (exclusive)
/// @param op Operation selector: -1 toggle, 0 clear, 1 set
/// @return `bs`.
/// @pre `start <= end && end <= nbits`.
/// @warning Internal API.
static_fun rk_forceinline bitset RK__internal_bitset_range_op(bitset bs, size_t nbits, size_t start,
                                                              size_t end, int _op) {
  enum optype { FLIP = -1, SET = 1, CLEAR = 0 } op = (enum optype)_op;
  rk_assert(start <= end && end <= nbits), (void)nbits;
  if (start == end) { return bs; }
  size_t      sw = start / bitset_word_bits, ew = (end - 1) / bitset_word_bits;
  size_t      sb = start % bitset_word_bits, eb = (end - 1) % bitset_word_bits;
  bitset_word sm = (~(bitset_word)0 << sb), em = (~(bitset_word)0 >> (bitset_word_bits - (eb + 1)));
  if (sw == ew) {
    switch (op) {
    case FLIP : bs[sw] ^= (sm & em); break;
    case CLEAR: bs[sw] &= ~(sm & em); break;
    case SET  : bs[sw] |= (sm & em); break;
    }
    return bs;
  }
  if (sb) {
    switch (op) {
    case FLIP : bs[sw] ^= sm; break;
    case CLEAR: bs[sw] &= ~sm; break;
    case SET  : bs[sw] |= sm; break;
    }
    ++sw;
  }
  switch (op) {
  case FLIP:
    for (size_t w = sw; w < ew; ++w) { bs[w] ^= (bitset_word) ~(bitset_word)0; }
    bs[ew] ^= em;
    break;
  case CLEAR: memset(&bs[sw], 0, sizeof_n(*bs, (ew - sw))), bs[ew] &= ~em; break;
  case SET  : memset(&bs[sw], 0xFF, sizeof_n(*bs, (ew - sw))), bs[ew] |= em; break;
  }
  return bs;
}
/// @endcond
RK_HEADER_END
/// @}
#endif // RK_BITSET_H
