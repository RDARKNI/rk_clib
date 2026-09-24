// SPDX-License-Identifier: MIT
/// @file rk_defs.h
/// @version 1.0
/// @defgroup rk_defs Common Definitions and Compatibility Layer
/// @brief Common definitions and platform compatibility layer for rk_clib
///
/// Provides platform-specific adjustments, feature detection, typedefs, utility macros, and small
/// functions for consistent RK library usage.
///
/// Requires C11 or later, as well as some version of typeof (supported by GCC and Clang on all
/// Versions and Msvc 17.9+). C++ compatibility for clang only.
///
/// @date 2025
/// @{

#ifndef RK_DEFS_H
#define RK_DEFS_H
#include "rk_config.h"

#ifdef __has_include
# define rk_has_include(x) __has_include(x)
#else
# define rk_has_include(x) 0
#endif

#ifdef __has_builtin
# define rk_has_builtin(x) __has_builtin(x)
#else
# define rk_has_builtin(x) 0
#endif

#ifdef __has_attribute
# define rk_has_gnu_attribute(x) __has_attribute(x)
#else
# define rk_has_gnu_attribute(x) 0
#endif

#if defined(__has_c_attribute) && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
# define rk_has_c_attribute(x) (__has_c_attribute(x))
#else
# define rk_has_c_attribute(x) 0
#endif

#if defined(__has_cpp_attribute) && defined(__cplusplus)
# define rk_has_cpp_attribute(x) __has_cpp_attribute(x)
#else
# define rk_has_cpp_attribute(x) 0
#endif

#define rk_has_c_cpp_attribute(x) (rk_has_c_attribute(x) || rk_has_cpp_attribute(x))

#if defined(__has_attribute)
# define rk___attribute__(attr) __attribute__((attr))
#else
# define rk___attribute__(attr)
#endif

#ifdef _MSC_VER
# define rk___declspec(attr) __declspec(attr)
#else
# define rk___declspec(attr)
#endif

#ifdef _MSC_VER
# include <intrin.h>
#endif
#ifdef __cplusplus
# include <type_traits>
# include <utility>
# if __cplusplus >= 202002L
#  include <bit>
# endif
#else
# include <stdalign.h>
# include <stdbool.h>
# if rk_has_include(<stdcountof.h>)
#  include <stdcountof.h>
# endif
#endif

#if rk_has_include(<stdbit.h>)
# include <stdbit.h>
#endif

#ifndef __STDC_VERSION_STDBIT_H__
/* has_include can be true but in C++ it may have an include guard*/
# define RK_STDBIT_FALLBACK 1
#else
# define RK_STDBIT_FALLBACK 0
#endif

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tgmath.h>

#define RK_DO_PRAGMA(a) _Pragma(#a)
#ifdef __clang__
/// Disables warnings about:
/// - Zero-argument variadic macro arguments that work on all targeted platforms
/// - Overriding keywords as macros (typeof with certain flags)
/// - Unknown warning options (for the next warning on older compiler versions)
/// - Warnings about c2x, c2y and c23 extensions (namely countof, for which a fallback exists and
///   attribute syntax), since they're used conditionally and portably.
/// - Warnings about unknown attributes (for the previous two)
# define RK__SILENCE_WARNINGS_BEG                                                                  \
   RK_DO_PRAGMA(clang diagnostic push)                                                             \
   RK_DO_PRAGMA(clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments")                    \
   RK_DO_PRAGMA(clang diagnostic ignored "-Wkeyword-macro")                                        \
   RK_DO_PRAGMA(clang diagnostic ignored "-Wunknown-warning-option")                               \
   RK_DO_PRAGMA(clang diagnostic ignored "-Wc2x-extensions")                                       \
   RK_DO_PRAGMA(clang diagnostic ignored "-Wc23-extensions")                                       \
   RK_DO_PRAGMA(clang diagnostic ignored "-Wc2y-extensions")                                       \
   RK_DO_PRAGMA(clang diagnostic ignored "-Wunknown-attributes")                                   \
   RK_DO_PRAGMA(clang diagnostic ignored "-Wc++17-extensions")

# define RK__SILENCE_WARNINGS_END RK_DO_PRAGMA(clang diagnostic pop)

# define RK__IGNWARN_CLANG_BEG(warn)                                                               \
   RK_DO_PRAGMA(clang diagnostic push)                                                             \
   RK_DO_PRAGMA(clang diagnostic ignored warn)
# define RK__IGNWARN_CLANG_END() RK_DO_PRAGMA(clang diagnostic pop)
# define RK__IGNWARN_CLANG(warn, ...)                                                              \
   RK__IGNWARN_CLANG_BEG(warn)                                                                     \
   (__VA_ARGS__) RK__IGNWARN_CLANG_END()
#else
# define RK__IGNWARN_CLANG_BEG(warn)
# define RK__IGNWARN_CLANG_END()
# define RK__IGNWARN_CLANG(warn, ...) (__VA_ARGS__)
#endif

#if defined(_MSC_VER) && !defined(__clang__)
/// disable the following (in this case) erroneous warnings: 4200 flexible array members (supported
/// on every targeted platform) 4116 Warnings about anonymous compound literals (not an issue in C11
/// mode) 4141 Warnings about inline used twice (due to forceinline macro)
/// @note excludes clang-cl (defines both __clang__ and _MSC_VER): it already gets
/// RK__SILENCE_WARNINGS_BEG/END from the __clang__ branch above, and this branch must not redefine
/// them with different (MSVC-pragma) bodies.
# define RK__SILENCE_WARNINGS_BEG                                                                  \
   __pragma(warning(push)) __pragma(warning(disable : 4200 4116 4141))
# define RK__SILENCE_WARNINGS_END   __pragma(warning(pop))
# define RK__IGNWARN_MSC_BEG(warn)  __pragma(warning(push)) __pragma(warning(disable : warn))
# define RK__IGNWARN_MSC_END()      __pragma(warning(pop))

# define RK__IGNWARN_MSC(warn, ...) RK__IGNWARN_MSC_BEG(warn) __VA_ARGS__ __pragma(warning(pop))
#else
# define RK__IGNWARN_MSC(warn, ...) __VA_ARGS__
# ifndef RK__SILENCE_WARNINGS_BEG
#  define RK__SILENCE_WARNINGS_BEG
#  define RK__SILENCE_WARNINGS_END
# endif
#endif

#ifdef __cplusplus
# define RK_EXTERNC_BEG extern "C" {
# define RK_EXTERNC_END }
#else
# define RK_EXTERNC_BEG
# define RK_EXTERNC_END
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// Linkage Compatibility //////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
# define static_fun inline
# define extern_fun inline
# if defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606L
#  define extern_var      inline
#  define extern_def(...) = __VA_ARGS__
# else
#  if !defined(RK_MULTI_TU)
#   define extern_var      static
#   define extern_def(...) = __VA_ARGS__
#  elif defined(RK_IMPL)
#   define extern_var
#   define extern_def(...) = __VA_ARGS__
#  else
#   define extern_var extern
#   define extern_def(...)
#  endif
# endif
#else
# define static_fun /*rk_unused*/ static inline
# if !defined(RK_MULTI_TU)
#  define extern_fun      extern inline
#  define extern_var      static
#  define extern_def(...) = __VA_ARGS__
# else
#  if defined(RK_IMPL)
#   define extern_var
#   define extern_def(...) = __VA_ARGS__
#   ifndef __GNUC_GNU_INLINE__
#    define extern_fun extern inline
#   else
#    define extern_fun inline
#   endif
#  else
#   define extern_var extern
#   define extern_def(...)
#   ifndef __GNUC_GNU_INLINE__
#    define extern_fun inline
#   else
#    define extern_fun extern inline
#   endif
#  endif
# endif
#endif

#define RK_HEADER_BEGIN RK__SILENCE_WARNINGS_BEG RK_EXTERNC_BEG
#define RK_HEADER_END   RK_EXTERNC_END RK__SILENCE_WARNINGS_END

RK_HEADER_BEGIN

/// @name Attribute Wrappers
/// @brief Portable wrappers for common attributes and compiler-specific extensions. These macros
/// abstract away compiler differences and provide a consistent interface for using attributes
/// across different platforms and compilers. The `rk_has_c_cpp_attribute` macro is used to check
/// for the presence of a specific attribute with the [[]] syntax in either C or C++. If the
/// attribute is supported, the corresponding wrapper macro is defined to use it; otherwise, it is
/// defined as empty. This allows developers to use attributes in a portable way without worrying
/// about compiler-specific syntax or support. The provided wrappers include common attributes such
/// as `deprecated`, `fallthrough`, `maybe_unused`, `nodiscard`, `noreturn`, `reproducible`,
/// `unsequenced`, `assume`, `likely`, and `unlikely`. Additionally, there are compiler-specific
/// attributes for function purity, inlining, and unused variables. These wrappers enable developers
/// to write cleaner and more efficient code while maintaining compatibility across different
/// compilers and platforms.
///
/// @{

#if rk_has_gnu_attribute(malloc)
# define rk_malloc_fun __attribute__((malloc))
#elif defined(_MSC_VER)
# define rk_malloc_fun __declspec(restrict)
#else
# define rk_malloc_fun
#endif

#ifdef __GNUC__
# define rk_attr_printf(_beg, _end) __attribute__((format(printf, _beg, _end)))
#else
# define rk_attr_printf(...)
#endif

#if rk_has_gnu_attribute(alloc_size)
# define rk_alloc_size(...) __attribute__((alloc_size(__VA_ARGS__)))
#else
# define rk_alloc_size(...)
#endif

#if rk_has_gnu_attribute(alloc_align)
# define rk_alloc_align(align) __attribute__((alloc_align(align)))
#else
# define rk_alloc_align(align)
#endif

#if rk_has_gnu_attribute(alloc_align) && rk_has_gnu_attribute(alloc_size)
# define rk_alloc_alignsize(align, ...) __attribute__((alloc_align(align), alloc_size(__VA_ARGS__)))
#else
# define rk_alloc_alignsize(align, ...)
#endif

#if rk_has_c_cpp_attribute(deprecated)
# define rk_deprecated(...) [[deprecated("" __VA_ARGS__)]]
#elif rk_has_gnu_attribute(deprecated)
# define rk_deprecated(...) __attribute__((deprecated("" __VA_ARGS__)))
#else
# define rk_deprecated(...)
#endif

#if rk_has_c_cpp_attribute(nodiscard)
# define rk_nodiscard(...) [[nodiscard("" __VA_ARGS__)]]
#elif rk_has_gnu_attribute(warn_unused_result)
# define rk_nodiscard(...) __attribute__((warn_unused_result))
#else
# define rk_nodiscard(...)
#endif

#if rk_has_c_cpp_attribute(fallthrough)
# define rk_fallthrough [[fallthrough]]
#elif rk_has_gnu_attribute(fallthrough)
# define rk_fallthrough __attribute__((fallthrough))
#else
# define rk_fallthrough
#endif

#if rk_has_c_cpp_attribute(maybe_unused)
# define rk_unused [[maybe_unused]]
#elif rk_has_gnu_attribute(unused)
# define rk_unused __attribute__((unused))
#else
# define rk_unused
#endif

#if rk_has_c_cpp_attribute(reproducible)
# define rk_reproducible [[reproducible]]
#else
# define rk_reproducible
#endif

#if rk_has_c_cpp_attribute(unsequenced)
# define rk_unsequenced [[unsequenced]]
#else
# define rk_unsequenced
#endif

#if rk_has_gnu_attribute(pure)
# define rk_pure __attribute__((pure))
#else
# define rk_pure
#endif

#if rk_has_gnu_attribute(const)
# define rk_const __attribute__((const))
#else
# define rk_const rk_unsequenced
#endif

#if rk_has_c_cpp_attribute(likely)
# define rk_attr_likely   [[likely]]
# define rk_attr_unlikely [[unlikely]]
#else
# define rk_attr_likely
# define rk_attr_unlikely
#endif

#if rk_has_builtin(__builtin_expect)
# define rk_likely(...)   (__builtin_expect(!!(__VA_ARGS__), 1))
# define rk_unlikely(...) (__builtin_expect(!!(__VA_ARGS__), 0))
#else
# define rk_likely(...)   ((__VA_ARGS__))
# define rk_unlikely(...) ((__VA_ARGS__))
#endif

#if rk_has_gnu_attribute(always_inline)
# define rk_forceinline __attribute__((always_inline))
#elif defined(_MSC_VER)
# define rk_forceinline __forceinline
#else
# define rk_forceinline
#endif

#if rk_has_c_cpp_attribute(noreturn)
# define rk_noreturn [[noreturn]]
#elif rk_has_gnu_attribute(noreturn)
# define rk_noreturn __attribute__((noreturn))
#elif defined(_MSC_VER)
# define rk_noreturn __declspec(noreturn)
#elif __STDC_VERSION__ >= 201112L
# define rk_noreturn _Noreturn
#else
# define rk_noreturn
#endif

#if rk_has_gnu_attribute(assume)
# define rk_assume(...) __attribute__((assume((__VA_ARGS__))))
#elif defined(_MSC_VER)
# define rk_assume(...) __assume((__VA_ARGS__))
#elif rk_has_builtin(__builtin_assume)
# define rk_assume(...)                                                                            \
   do { __builtin_assume((__VA_ARGS__)); } while (0)
#elif rk_has_c_cpp_attribute(assume)
# define rk_assume(...) [[assume((__VA_ARGS__))]]
#else
# define rk_assume(...)                                                                            \
   do { (void)sizeof((__VA_ARGS__)); } while (0)
#endif

/// @}

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   Keyword Compatibility Layer   ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#if !defined(thread_local) && !defined(__cplusplus)
# define thread_local _Thread_local
#endif

#if !defined(restrict) && (defined(_MSC_VER) || defined(__cplusplus))
# define restrict __restrict
#endif

#if !defined(typeof) && (defined(__cplusplus) || __STDC_VERSION__ < 202311L)
# ifdef __cplusplus
#  define typeof(...) std::remove_reference<__typeof__(__VA_ARGS__)>::type
# else
#  define typeof __typeof__
# endif
#endif

#ifndef countof
# ifndef __cplusplus
#  define rk_COUNTOF(...) (sizeof(__VA_ARGS__) / sizeof((__VA_ARGS__)[0]))
#  define countof(...)    (static_assert_expr(rk_is_array((__VA_ARGS__))) + rk_COUNTOF(__VA_ARGS__))
# else
#  define countof(...)    RK__countof(__VA_ARGS__)
#  define rk_COUNTOF(...) countof(__VA_ARGS__)
# endif
#else
# if __STDC_VERSION__ <= 202000L && defined(__GNUC__)
#  define rk_COUNTOF __extension__ countof
# else
#  define rk_COUNTOF countof
# endif
#endif

#ifndef unreachable
# if defined(__cpp_lib_unreachable) && __cpp_lib_unreachable >= 202202L
#  define unreachable() std::unreachable()
# elif defined(__GNUC__)
#  define unreachable() __builtin_unreachable()
# elif defined(_MSC_VER)
static_fun __forceinline rk_noreturn void RK__unreachable_impl(void) {
  __assume(0);
#  if defined(_DEBUG)
  __debugbreak();
#  else
  __fastfail(1);
#  endif
}
#  define unreachable() RK__unreachable_impl()
# else
#  define unreachable() (assert(!"unreachable code reached"), abort())
# endif
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   Pseudo  -  Keywords   ////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(__cplusplus) || (!defined(_MSC_VER) && __STDC_VERSION__ >= 202000L)
# define rk_null nullptr
#else
# define rk_null ((void*)0)
#endif
/// @brief The maximum fundamental alignment.
#define align_max     alignof(RK__max_align_type)

/// @brief Aligns an object to the maximum fundamental alignment.
#define alignas_max   alignas(align_max)

/// @brief Returns the bits in a type or expression.
#define bitsof(...)   (sizeof(__VA_ARGS__) * CHAR_BIT)

/// @brief Returns the string length of a string literal.
#define lenof(strlit) (sizeof("" strlit "") - 1)

/// @brief Returns the minimum value of any C integral type.
#define minof(T)      RK__minof_impl(T)

/// @brief Returns the maximum value of any C integral type.
#define maxof(T)      RK__maxof_impl(T)

/// @brief Like the Kernel's container_of_const macro but portable
#define containerof(ptr, type, member)                                                             \
  ((typeof(_Generic(ptr,                                                                           \
               const typeof(*(ptr))*: (const type*)0,                                              \
               default: (type*)0)))((char*)(typeof(((const type*)0)->member)*){ptr}                \
                                    - offsetof(type, member)))

#ifndef rk_mult
# define rk_mult(x, y) ((x) * (y))
#endif

/// @brief todo docs, attribute
static_fun rk_forceinline size_t rk_mult_safe(size_t x, size_t y) {
  return rk_likely(x == 0 || y <= SIZE_MAX / x) ? x * y : (abort(), (size_t)0);
}

/// @brief Returns the byte size of n objects of type T.
#define sizeof_n(T, count) rk_mult(sizeof(T), count)

#define sizeof_n_static(T, count)                                                                  \
  (static_assert_expr(sizeof(T) == 0 || (count) <= SIZE_MAX / sizeof(T), "overflow")               \
   + (sizeof(T) * (count)))

#ifndef __cplusplus
/// @brief static_assert-like check within expressions, evaluates to 0 if true and causes
/// compile-time error if false.
/// @param condition The condition to check for; must be a constant expression
/// @param msg The message to show upon compile error (optional since C23/C++17)
# define static_assert_expr(...)                                                                   \
   (0 * sizeof(union {                                                                             \
     static_assert(__VA_ARGS__);                                                                   \
     char _;                                                                                       \
    }))

#elif __cplusplus >= 202002L
# define static_assert_expr(...) (0 * sizeof([]() { static_assert(__VA_ARGS__); }))
#else
# define static_assert_expr(first, ...) (0 * sizeof(char[1 - 2 * !(first)]) && "" __VA_ARGS__)
#endif

#ifdef __GNUC__
# define try_static_assert_expr(expr, ...)                                                         \
   static_assert_expr((!__builtin_constant_p(expr) || !!(expr)), ##__VA_ARGS__)
#else
# define try_static_assert_expr(expr, ...) ((void)0)
#endif

/// @brief For static expression dispatch.
#if defined(__GNUC__) && !defined(__cplusplus)
# define rk_static_if(cond, _if, _else) __builtin_choose_expr(cond, _if, _else)
#else
# define rk_static_if(cond, _if, _else)                                                            \
   _Generic(((char (*)[1 + !!(cond)])0), char (*)[2]: _if, char (*)[1]: _else)
#endif

#ifndef __cplusplus
# define RK__pun_cast(to_type, expr)                                                               \
   rk_static_if(!rk_is_array(expr),                                                                \
                (union {                                                                           \
                 static_assert(sizeof(typeof(expr)) == sizeof(to_type),                            \
                               "Types must be the same size.");                                    \
                 typeof(expr) f;                                                                   \
                 to_type      t;                                                                   \
                }){(expr)}                                                                         \
                    .t,                                                                            \
                *(to_type*)rk_memcpy(rk_dummyofp(to_type),                                         \
                                     (union {                                                      \
                                      typeof_decayed(expr) _v2;                                    \
                                      void* _v;                                                    \
                                     }){(expr)}                                                    \
                                         ._v,                                                      \
                                     sizeof(to_type)))

# ifndef _MSC_VER
#  define pun_cast(to_type, ...) RK__pun_cast(to_type, (__VA_ARGS__))
# else
#  define pun_cast(to_type, expr) RK__IGNWARN_MSC(4116, RK__pun_cast(to_type, expr))
# endif

#elif __cplusplus >= 202002L
# define pun_cast(to_type, expr) (std::bit_cast<to_type>((expr)))
#else
# define pun_cast(to_type, expr)                                                                   \
   ([](const typename std::remove_reference<decltype(expr)>::type& _e) {                           \
    typedef typename std::remove_reference<decltype(expr)>::type RK_SRC_T;                         \
    static_assert(std::is_trivially_copyable<RK_SRC_T>::value,                                     \
                  #expr " must be trivially copyable.");                                           \
    static_assert(std::is_trivially_copyable<to_type>::value,                                      \
                  #to_type " must be trivially copyable.");                                        \
    static_assert(sizeof(_e) == sizeof(to_type), "Types must be the same size.");                  \
    to_type RK_TMP;                                                                                \
    memcpy(&RK_TMP, &_e, sizeof(_e));                                                              \
    return RK_TMP;                                                                                 \
   }((expr)))
#endif

#define VA_FIRST(first, ...) first
#define VA_REST(first, ...)  __VA_ARGS__

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////        Typedefs         ////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
typedef uintptr_t uptr;
typedef intptr_t  sptr;
typedef uint8_t   u8;
typedef int8_t    s8;
typedef uint16_t  u16;
typedef int16_t   s16;
typedef uint32_t  u32;
typedef int32_t   s32;
typedef uint64_t  u64;
typedef int64_t   s64;

#ifdef __SIZEOF_INT128__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wpedantic" /* guarded-against already */
__extension__ typedef unsigned __int128 u128;
__extension__ typedef __int128          s128;
# pragma GCC diagnostic pop
# define RK__IFHAS_INT128(...) __VA_ARGS__
#else
# define RK__IFHAS_INT128(...)
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   Function Wrappers   //////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

static_fun rk_const size_t rk_align_up(size_t size, size_t align);

static_fun rk_const size_t rk_align_pad(const void* ptr, size_t align);

static_fun rk_const bool   rk_ptrs_overlap(const void* beg1, const void* end1, const void* beg2,
                                           const void* end2);

static_fun rk_const bool   rk_ptr_in_range(const void* ptr, const void* beg, const void* end);

static_fun rk_forceinline void* rk_memcpy(void* const restrict dst, const void* const restrict src,
                                          size_t nbytes) {
  return nbytes ? memcpy(dst, src, nbytes) : dst;
}
static_fun rk_forceinline void* rk_memmove(void* dst, const void* src, size_t nbytes) {
  return nbytes ? memmove(dst, src, nbytes) : dst;
}
static_fun rk_forceinline void* rk_memset(void* dst, int value, size_t nbytes) {
  return nbytes ? memset(dst, value, nbytes) : dst;
}
static_fun rk_forceinline int rk_memcmp(const void* a, const void* b, size_t nbytes) {
  return nbytes ? memcmp(a, b, nbytes) : 0;
}

/// @brief `T* rk_copy(T* dst, T* src, size_t count) ` - Typed memcpy for copying between arrays of
/// the same type.
/// @param dst       the memory location to copy to
/// @param src       the memory location to copy from (must be typed)
/// @param count     the count of objects to copy
/// @return the dst pointer, with unchanged type
#define rk_copy(dst, src, count)                                                                   \
  ((typeof((dst)[0])*)(rk_ensure_ptrs_copy_compatible(dst, src),                                   \
                       rk_memcpy(dst, src, sizeof_n((dst)[0], (count)))))

/// @brief like rk_copy() for memmove.
#define rk_move(dst, src, count)                                                                   \
  ((typeof((dst)[0])*)(rk_ensure_ptrs_copy_compatible(dst, src),                                   \
                       rk_memmove(dst, src, sizeof_n((dst)[0], (count)))))

#ifdef RKLIB_DEBUG
# define rk_assert(...)                                                                            \
   (rk_likely((__VA_ARGS__)) ? (void)0 : RK_assertfail(#__VA_ARGS__, __FILE__, __LINE__, __func__))
/// print to stderr if RKLIB_DEBUG is defined
rk_noreturn static_fun void RK_assertfail(const char* expr, const char* file, int line,
                                          const char* func) {
  fprintf(stderr, "Assertion failed: (%s), function %s, file %s, line %d.\n", expr, func, file,
          line);
  abort();
}
#else
# define rk_assert(...) ((void)sizeof(!(__VA_ARGS__))) // rk_assume((__VA_ARGS__))
// no assume since we need an expression
/// print to stderr if RKLIB_DEBUG is defined
#endif
#ifdef RKLIB_DEBUG
# define rk_log(...) ((void)fprintf(stderr, __VA_ARGS__)) /* NOLINT */
#else
# define rk_log(...) ((void)0)
#endif

/// @name Numeric helpers
/// @brief Type-safe numeric operations implemented via `_Generic` dispatch. Arguments are evaluated
/// exactly once. Macros with uppercase names are provided for use in macros that require
/// compile-time constant expressions, but they evaluate their arguments multiple times and/or are
/// less type-safe and should be used with caution.
///
/// All lowercase macros require the parameters to be in the same numeric category:
/// - both signed integers, or
/// - both unsigned integers, or
/// - both floating-point.
/// @{

/// @brief Returns the absolute value of `x`. For signed integers, the result is undefined/overflow
/// if `x` is the minimum representable value (e.g. `INT_MIN`), matching typical `abs` semantics.
#define rk_abs(x)                RK__abs_impl(x)

/// @brief Returns the smaller of `x` and `y`.
#define rk_min(x, y)             RK__twonum_macro(min_, RK_NUM_TYPES, x, y)
/// @brief Like `rk_min()` but not type-safe and may double-evaluate args.
#define rk_MIN(a, b)             ((a) < (b) ? (a) : (b))

/// @brief Returns the larger of `x` and `y`.
#define rk_max(x, y)             RK__twonum_macro(max_, RK_NUM_TYPES, x, y)
/// @brief Like `rk_max()` but not type-safe and may double-evaluate args.
#define rk_MAX(a, b)             ((a) > (b) ? (a) : (b))

/// @brief Clamps `num` to the inclusive range [`low`, `high`]. Requires `low <= high`.
#define rk_clamp(num, low, high) RK__threenum_macro(clamp_, RK_NUM_TYPES, num, low, high)
/// @brief Like `rk_clamp()` but not type-safe and may double-evaluate args.
#define rk_CLAMP(num, low, high) ((num) < (low) ? (low) : ((num) > (high) ? (high) : (num)))

/// @brief Saturating addition, clamps to `[TYPE_MIN, TYPE_MAX]` of the common type.
#define rk_sat_add(x, y)         RK__twonum_macro(rk_sat_add_, RK_SU_TYPES, x, y)

/// @brief Saturating subtraction, clamps to `[TYPE_MIN, TYPE_MAX]` of the common type.
#define rk_sat_sub(x, y)         RK__twonum_macro(rk_sat_sub_, RK_SU_TYPES, x, y)

/// @brief Saturating multiplication, clamps to `[TYPE_MIN, TYPE_MAX]` of the common type.
#define rk_sat_mul(x, y)         RK__twonum_macro(rk_sat_mul_, RK_SU_TYPES, x, y)

#define rk_SWAP(a, b)                                                                              \
  do {                                                                                             \
    typeof(a)* _a      = &(a);                                                                     \
    typeof(b)* _b      = &(b);                                                                     \
    typeof(a)  RK__TMP = *_a;                                                                      \
    *_a                = *_b;                                                                      \
    *_b                = RK__TMP;                                                                  \
  } while (0)

/// @}

#if RK_STDBIT_FALLBACK
# define stdc_leading_zeros(...)                                                                   \
   _Generic((__VA_ARGS__)RK_U_TYPES(RK__GENCASE, stdc_leading_zeros_))(__VA_ARGS__)
# define stdc_leading_ones(...)                                                                    \
   _Generic((__VA_ARGS__)RK_U_TYPES(RK__GENCASE, stdc_leading_ones_))(__VA_ARGS__)
# define stdc_trailing_zeros(...)                                                                  \
   _Generic((__VA_ARGS__)RK_U_TYPES(RK__GENCASE, stdc_trailing_zeros_))(__VA_ARGS__)
# define stdc_trailing_ones(...)                                                                   \
   _Generic((__VA_ARGS__)RK_U_TYPES(RK__GENCASE, stdc_trailing_ones_))(__VA_ARGS__)
# define stdc_count_zeros(...)                                                                     \
   _Generic((__VA_ARGS__)RK_U_TYPES(RK__GENCASE, stdc_count_zeros_))(__VA_ARGS__)
# define stdc_count_ones(...)                                                                      \
   _Generic((__VA_ARGS__)RK_U_TYPES(RK__GENCASE, stdc_count_ones_))(__VA_ARGS__)
# define stdc_first_leading_zero(...)                                                              \
   _Generic((__VA_ARGS__)RK_U_TYPES(RK__GENCASE, stdc_first_leading_zero_))(__VA_ARGS__)
# define stdc_first_leading_one(...)                                                               \
   _Generic((__VA_ARGS__)RK_U_TYPES(RK__GENCASE, stdc_first_leading_one_))(__VA_ARGS__)
# define stdc_first_trailing_zero(...)                                                             \
   _Generic((__VA_ARGS__)RK_U_TYPES(RK__GENCASE, stdc_first_trailing_zero_))(__VA_ARGS__)
# define stdc_first_trailing_one(...)                                                              \
   _Generic((__VA_ARGS__)RK_U_TYPES(RK__GENCASE, stdc_first_trailing_one_))(__VA_ARGS__)
# define stdc_bit_floor(...)                                                                       \
   _Generic((__VA_ARGS__)RK_U_TYPES(RK__GENCASE, stdc_bit_floor_))(__VA_ARGS__)
# define stdc_bit_ceil(...)                                                                        \
   _Generic((__VA_ARGS__)RK_U_TYPES(RK__GENCASE, stdc_bit_ceil_))(__VA_ARGS__)
# define stdc_bit_width(...)                                                                       \
   _Generic((__VA_ARGS__)RK_U_TYPES(RK__GENCASE, stdc_bit_width_))(__VA_ARGS__)
# define stdc_has_single_bit(...)                                                                  \
   _Generic((__VA_ARGS__)RK_U_TYPES(RK__GENCASE, stdc_has_single_bit_))(__VA_ARGS__)
#endif /* RK_STDBIT_FALLBACK */

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

#define RK__CONC(a, b)         a##b

#define rk_EXP(x)              x
#define rk_CONC(a, b)          RK__CONC(a, b)
#define rk_UNIQUE_NAME(prefix) rk_CONC(prefix, __LINE__)

#define RK__ARGCOUNT(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16,    \
                     _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31,    \
                     _32, N, ...)                                                                  \
  N
#if __STDC_VERSION__ >= 202000L || (defined(__cplusplus) && __cplusplus >= 202002L)
# define rk_ARGCOUNT(...)                                                                          \
   RK__ARGCOUNT(dummy __VA_OPT__(, ) __VA_ARGS__, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21,  \
                20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#else
# define rk_ARGCOUNT(...)                                                                          \
   RK__ARGCOUNT(dummy, ##__VA_ARGS__, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18,  \
                17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#endif

#ifndef __cplusplus
# define RK_ZINIT 0
#else
# define RK_ZINIT
#endif

#define RK__GENCASE(T, N, fun_name) , T : fun_name##N

#define RK__contrav(T, x)           _Generic(x, T: x, default: (T){RK_ZINIT})
#define RK__contrav_p(T, x)         _Generic(x, T: x, default: (T)1)

#ifndef __cplusplus
# define rk_dummyof(v)     ((typeof(v)){RK_ZINIT})
# define rk_dummyofp(v)    ((typeof(v)*)0)
# define typeof_decayed(v) typeof((void)0, rk_dummyof(v))
#else
# define rk_dummyof(v)     ((std::remove_reference<decltype(v)>::type*)0)
# define rk_dummyofp(v)    ((typeof(v)*)0)
# define typeof_decayed(v) std::decay<typeof(v)>::type
#endif

#ifndef __cplusplus
# define rk_to_rvalue(obj) ((void)0, (obj))
#else
# define rk_to_rvalue(obj) ((typeof(obj))(obj))
#endif

// Real MSVC (not clang-cl) never implemented `_Atomic` as a core-language
// type qualifier -- it only partially supports the separate <stdatomic.h>
// library -- so `_Atomic`-qualified types are simply inexpressible there.
// Since nothing can ever reach those _Generic associations on that
// compiler, omitting them is exact, not an approximation.
#if defined(_MSC_VER) && !defined(__clang__)
# define rk_is_array(v)                                                                            \
   _Generic(rk_dummyofp(v),                                                                        \
       typeof_decayed(v)*: 0,                                                                      \
       const typeof_decayed(v)*: 0,                                                                \
       volatile typeof_decayed(v)*: 0,                                                             \
       const volatile typeof_decayed(v)*: 0,                                                       \
       default: 1)
#else
# define rk_is_array(v)                                                                            \
   _Generic(rk_dummyofp(v),                                                                        \
       typeof_decayed(v)*: 0,                                                                      \
       const typeof_decayed(v)*: 0,                                                                \
       volatile typeof_decayed(v)*: 0,                                                             \
       const volatile typeof_decayed(v)*: 0,                                                       \
       _Atomic typeof_decayed(v)*: 0,                                                              \
       _Atomic const typeof_decayed(v)*: 0,                                                        \
       _Atomic volatile typeof_decayed(v)*: 0,                                                     \
       _Atomic const volatile typeof_decayed(v)*: 0,                                               \
       default: 1)
#endif

#define rk_is_const(v)    _Generic(rk_dummyofp(v), const typeof(v)*: 1, default: 0)
#define rk_is_volatile(v) _Generic(rk_dummyofp(v), volatile typeof(v)*: 1, default: 0)
#if defined(_MSC_VER) && !defined(__clang__)
# define rk_is_atomic(v) ((void)rk_dummyofp(v), 0)
#else
# define rk_is_atomic(v) _Generic(rk_dummyofp(v), _Atomic typeof(v)*: 1, default: 0)
#endif

#define rk_is_same_type(T, U) _Generic(rk_dummyofp(T), typeof(U)*: 1, default: 0)

#define rk_ptrs_copy_compatible(dst, src)                                                          \
  (!rk_is_const(*(dst)) && sizeof((dst)[0]) == sizeof((src)[0]))

#define rk_ensure_ptrs_copy_compatible(dst, src)                                                   \
  ((void)static_assert_expr(rk_ptrs_copy_compatible(dst, src), #dst " and " #src " not "           \
                                                                    "compatible"))

#define rk_ensure_type_is_num(T) ((T)((T)0 * 0))

#define rk_ensure_numclass_compatible(x, y)                                                        \
  static_assert_expr(RK_numclassof(x) & RK_numclassof(y), "Incompatible numeric types")

#define rk_ensure_malloc_align(T)                                                                  \
  static_assert_expr(alignof(T) <= RK_malloc_align, "Type alignment too "                          \
                                                    "large.")

/// ensures the backing array is legitimate storage, evaluates to 0
#define rk_ensure_valid_storage_type(arr)                                                          \
  static_assert_expr(rk_is_same_type(&(arr), unsigned char (*)[sizeof(arr)]),                      \
                     "Backing storage must be an unsigned char array")

/// Function overloading by argument count
#define rk_overload(m, ...)   rk_CONC(m, rk_ARGCOUNT(__VA_ARGS__))(__VA_ARGS__)
#define rk_overload_(m, ...)  rk_CONC(m, rk_ARGCOUNT(__VA_ARGS__))(__VA_ARGS__)
#define rk_overload__(m, ...) rk_CONC(m, rk_ARGCOUNT(__VA_ARGS__))(__VA_ARGS__)

static_fun rk_const bool rk_ptrs_overlap(const void* beg1, const void* end1, const void* beg2,
                                         const void* end2) {
  rk_assert(((uptr)beg1 <= (uptr)end1 && (uptr)beg2 <= (uptr)end2) && "Invalid Memory Region");
  return (uptr)beg1 < (uptr)end2 && (uptr)end1 > (uptr)beg2;
}
static_fun rk_const bool rk_ptr_in_range(const void* ptr, const void* beg, const void* end) {
  rk_assert((uptr)beg <= (uptr)end && "Invalid Memory Region");
  return (uptr)ptr >= (uptr)beg && (uptr)ptr < (uptr)end;
}

#if defined(_MSC_VER)
# if defined(_WIN64)
#  define RK_malloc_align 16u
# endif
#elif defined(__GLIBC__)
# if defined(__LP64__) || defined(_LP64)
#  define RK_malloc_align 16u
# endif
#elif defined(__APPLE__) && defined(__MACH__)
# if defined(__LP64__) || defined(_LP64)
#  define RK_malloc_align 16u
# endif
#endif
#ifndef RK_malloc_align
# define RK_malloc_align align_max
#endif

#if defined(_MSC_VER) || defined(__TINYC__)
typedef struct {
  long double ld;
  long long   ll;
  void*       vp;
} RK__max_align_type;
#else
typedef max_align_t RK__max_align_type;
#endif

/// bug prior to 17.44 that treated char == (un)signed char for _Generic
#if defined(_MSC_VER) && _MSC_VER < 1944
# define RK__IFNMSVC_CHARBUG(...)
#else
# define RK__IFNMSVC_CHARBUG(...) __VA_ARGS__
#endif

enum {                 // NOLINT
  RK_numclass_b = 0xF, ///< Boolean types  (0b1111)
  RK_numclass_o = 0x0, ///< Other types    (0b0000)
  RK_numclass_u = 0x1, ///< Unsigned types (0b0001)
  RK_numclass_s = 0x2, ///< Signed types   (0b0010)
  RK_numclass_f = 0x4, ///< Float types    (0b0100)
  RK_numclass_c = 0x8, ///< Char type      (0b1000)
};

#define RK__numclassof_(T, N, class)                                                               \
T:                                                                                                 \
  class,

#define RK_numclassof(x)                                                                           \
  _Generic(rk_ensure_type_is_num(typeof(x)),                                                       \
      RK_F_TYPES(RK__numclassof_, RK_numclass_f) bool: RK_numclass_b,                              \
      RK__IFNMSVC_CHARBUG(char : RK_numclass_c, ) default: (1 + !(((typeof(x))-1) > 0)))

#define RK_TOSIGNED(x)                                                                             \
  _Generic((x),                                                                                    \
      unsigned char: (signed char)(x),                                                             \
      unsigned short: (short)(x),                                                                  \
      unsigned: (int)(x),                                                                          \
      unsigned long: (long)(x),                                                                    \
      unsigned long long: (long long)(x)RK__IFHAS_INT128(, u128 : (s128)(x)))

#define RK_U_TYPES(X, ...)                                                                         \
  X(unsigned char, uc, ##__VA_ARGS__)                                                              \
  X(unsigned short, us, ##__VA_ARGS__)                                                             \
  X(unsigned, ui, ##__VA_ARGS__)                                                                   \
  X(unsigned long, ul, ##__VA_ARGS__)                                                              \
  X(unsigned long long, ull, ##__VA_ARGS__)                                                        \
  RK__IFHAS_INT128(X(u128, ullx, ##__VA_ARGS__))

#define RK_S_TYPES(X, ...)                                                                         \
  X(signed char, sc, ##__VA_ARGS__)                                                                \
  X(short, ss, ##__VA_ARGS__)                                                                      \
  X(int, si, ##__VA_ARGS__)                                                                        \
  X(long, sl, ##__VA_ARGS__)                                                                       \
  X(long long, sll, ##__VA_ARGS__)                                                                 \
  RK__IFHAS_INT128(X(s128, sllx, ##__VA_ARGS__))

#define RK_SU_TYPES(X, ...)                                                                        \
  RK_U_TYPES(X, ##__VA_ARGS__)                                                                     \
  RK_S_TYPES(X, ##__VA_ARGS__)

#define RK_INTEGRAL_TYPES(X, ...)                                                                  \
  RK__IFNMSVC_CHARBUG(X(char, c, ##__VA_ARGS__))                                                   \
  X(bool, b, ##__VA_ARGS__)                                                                        \
  RK_SU_TYPES(X, ##__VA_ARGS__)

#define RK_F_TYPES(X, ...)                                                                         \
  X(float, f, ##__VA_ARGS__)                                                                       \
  X(double, d, ##__VA_ARGS__)                                                                      \
  X(long double, ld, ##__VA_ARGS__)

#define RK_NUM_TYPES(X, ...)                                                                       \
  RK_INTEGRAL_TYPES(X, ##__VA_ARGS__)                                                              \
  RK_F_TYPES(X, ##__VA_ARGS__)

#define RK__wider_t(x, y)                                                                          \
  rk_static_if(rk_ensure_numclass_compatible(x, y) + sizeof(typeof(x)) >= sizeof(typeof(y)),       \
               (typeof(x))0, (typeof(y))0)

#define RK__wider_t3(a1, a2, a3)                                                                   \
  rk_static_if(sizeof(RK__wider_t(a1, a2)) >= sizeof(typeof(a3)), RK__wider_t(a1, a2),             \
               (typeof(a3))0)

#define RK__twonum_macro(pref, classes, x, y)                                                      \
  _Generic(RK__wider_t(x, y) classes(RK__GENCASE, pref))(x, y)

#define RK__threenum_macro(pref, classes, x, y, z)                                                 \
  _Generic(RK__wider_t3(x, y, z) classes(RK__GENCASE, pref))(x, y, z)

RK__IFHAS_INT128(static_fun rk_forceinline s128 abs_llx(s128 x) {
  return x < 0 ? -x : x; // UB if v == I128_MIN
})

#define RK__abs_impl(x)                                                                            \
  _Generic(rk_ensure_type_is_num(typeof(x)),                                                       \
      signed char: (signed char)abs((signed char)(x)),                                             \
      short: (short)abs((short)(x)),                                                               \
      int: abs((int)(x)),                                                                          \
      long: labs((long)(x)),                                                                       \
      long long: llabs((long long)(x)),                                                            \
      float: fabsf((float)(x)),                                                                    \
      double: fabs((double)(x)),                                                                   \
      long double: fabsl((long double)(x)),                                                        \
      RK__IFHAS_INT128(s128 : abs_llx(x), ) default: (x))

#define RK__minof_impl(T)                                                                          \
  ((T) _Generic(rk_ensure_type_is_num(T),                                                          \
       signed char: SCHAR_MIN,                                                                     \
       short: SHRT_MIN,                                                                            \
       int: INT_MIN,                                                                               \
       long: LONG_MIN,                                                                             \
       long long: LLONG_MIN,                                                                       \
       RK__IFNMSVC_CHARBUG(char : CHAR_MIN, )                                                      \
           RK__IFHAS_INT128(s128 : -((s128)(((u128) - 1) >> 1)) - 1, ) default: 0))

#define RK__maxof_impl(T)                                                                          \
  ((T) _Generic(rk_ensure_type_is_num(T),                                                          \
       signed char: SCHAR_MAX,                                                                     \
       short: SHRT_MAX,                                                                            \
       int: INT_MAX,                                                                               \
       long: LONG_MAX,                                                                             \
       long long: LLONG_MAX,                                                                       \
       RK__IFNMSVC_CHARBUG(char : CHAR_MAX, )                                                      \
           RK__IFHAS_INT128(s128 : ((u128)(~(u128)0)) >> 1, ) default: ((T)(~(T)0))))

#define RK__CHELPER         static_fun rk_const rk_forceinline
#define RK__UNSEQUENCED_NOW rk_unsequenced
#define RK_DEFINE_STUFF(T, N)                                                                      \
  RK__CHELPER T min_##N(T x, T y) RK__UNSEQUENCED_NOW { return rk_MIN(x, y); }                     \
  RK__CHELPER T max_##N(T x, T y) RK__UNSEQUENCED_NOW { return rk_MAX(x, y); }                     \
  RK__CHELPER T clamp_##N(T arg, T low, T high) RK__UNSEQUENCED_NOW {                              \
    return rk_CLAMP(arg, low, high);                                                               \
  }

// RK_INTEGRAL_TYPES
RK_INTEGRAL_TYPES(RK_DEFINE_STUFF)
#undef RK__UNSEQUENCED_NOW
#define RK__UNSEQUENCED_NOW
#undef RK__CHELPER
#define RK__CHELPER static_fun rk_forceinline
RK_F_TYPES(RK_DEFINE_STUFF)
#undef RK_DEFINE_STUFF
#undef RK__CHELPER
#define RK__CHELPER static_fun rk_const rk_forceinline

#define RK__DEF_SAT_U(T, N)                                                                        \
  RK__CHELPER T rk_sat_add_##N(T glob_a, T b) rk_unsequenced {                                     \
    T sum = (T)(glob_a + b);                                                                       \
    return sum >= glob_a ? sum : (T) - 1;                                                          \
  }                                                                                                \
  RK__CHELPER T rk_sat_sub_##N(T glob_a, T b) rk_unsequenced {                                     \
    return (T)(glob_a < b ? 0 : glob_a - b);                                                       \
  }                                                                                                \
  RK__CHELPER T rk_sat_mul_##N(T glob_a, T b) rk_unsequenced {                                     \
    return (b != 0 && glob_a > (T)(maxof(T) / b)) ? maxof(T) : (T)(glob_a * b);                    \
  }
RK_U_TYPES(RK__DEF_SAT_U)
#undef RK__DEF_SAT_U

#if rk_has_builtin(__builtin_add_overflow)
# define RK__DEF_SA_S_(T)                                                                          \
   T s;                                                                                            \
   if (__builtin_add_overflow(glob_a, b, &s)) { return (b < 0) ? minof(T) : maxof(T); }            \
   return s;
#else
# define RK__DEF_SA_S_(T)                                                                          \
   T min = minof(T), max = maxof(T);                                                               \
   if (b > 0 && glob_a > max - b) { return max; }                                                  \
   if (b < 0 && glob_a < min - b) { return min; }                                                  \
   return (T)(glob_a + b);
#endif

#if rk_has_builtin(__builtin_sub_overflow)
# define RK__DEF_SS_S_(T)                                                                          \
   T s;                                                                                            \
   if (__builtin_sub_overflow(glob_a, b, &s)) { return (b < 0) ? maxof(T) : minof(T); }            \
   return s;
#else
# define RK__DEF_SS_S_(T)                                                                          \
   const T min = minof(T), max = maxof(T);                                                         \
   if (b > 0 && glob_a < min + b) { return min; }                                                  \
   if (b < 0 && glob_a > max + b) { return max; }                                                  \
   return (T)(glob_a - b);
#endif
#if rk_has_builtin(__builtin_mul_overflow)
# define RK__DEF_SM_S_(T)                                                                          \
   if (glob_a == 0 || b == 0) return (T)0;                                                         \
   T glob_point;                                                                                   \
   if (__builtin_mul_overflow(glob_a, b, &glob_point)) {                                           \
     return ((glob_a < 0) ^ (b < 0)) ? minof(T) : maxof(T);                                        \
   }                                                                                               \
   return glob_point;
#else
# define RK__DEF_SM_S_(T)                                                                          \
   const T min = minof(T), max = maxof(T);                                                         \
   if (glob_a == 0 || b == 0) return (T)0;                                                         \
   if (glob_a == (T) - 1) { return b == min ? max : (T)(-b); }                                     \
   if (b == (T) - 1) { return glob_a == min ? max : (T)(-glob_a); }                                \
   if (glob_a > 0) {                                                                               \
     if (b > 0) {                                                                                  \
       if (glob_a > (T)(max / b)) return max;                                                      \
     } else {                                                                                      \
       if (b < (T)(min / glob_a)) return min;                                                      \
     }                                                                                             \
   } else {                                                                                        \
     if (b > 0) {                                                                                  \
       if (glob_a < (T)(min / b)) return min;                                                      \
     } else {                                                                                      \
       if (glob_a < (T)(max / b)) return max;                                                      \
     }                                                                                             \
   }                                                                                               \
   return (T)(glob_a * b);
#endif

#define RK__DEF_SAT_S(T, N)                                                                        \
  RK__CHELPER T                                          rk_sat_add_##N(T glob_a, T b)             \
      rk_unsequenced{RK__DEF_SA_S_(T)} RK__CHELPER T     rk_sat_sub_##N(T glob_a, T b)             \
          rk_unsequenced{RK__DEF_SS_S_(T)} RK__CHELPER T rk_sat_mul_##N(T glob_a, T b)             \
              rk_unsequenced {                                                                     \
    RK__DEF_SM_S_(T)                                                                               \
  }

RK_S_TYPES(RK__DEF_SAT_S)
#undef RK__DEF_SS_S_
#undef RK__DEF_SA_S_
#undef RK__DEF_SM_S_
#undef RK__DEF_SAT_S

#if RK_STDBIT_FALLBACK
# ifdef __GNUC__
#  if rk_has_builtin(__builtin_clzg)
#   define RK__DEF_LZ__(V) __builtin_clzg(V)
#  else
#   define RK__DEF_LZ__(V)                                                                         \
     _Generic(V,                                                                                   \
         default: (unsigned)__builtin_clz(V) - (bitsof(unsigned) - bitsof(V)),                     \
         unsigned long: __builtin_clzl(V),                                                         \
         unsigned long long: __builtin_clzll(V) RK__IFHAS_INT128(                                  \
                  , u128 : (u64)((u128)V >> 64) ? __builtin_clzll((u64)((u128)V >> 64))            \
                                                : 64 + __builtin_clzll((u64)V)))
#  endif
#  define RK__DEF_LZ_(T, V) return V ? (unsigned)RK__DEF_LZ__(V) : bitsof(V);

# elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#  ifdef _M_X64
#   define RK__DEF_LZ__(V) _BitScanReverse64(&idx, (unsigned long long)V), 63u - (unsigned)idx
#  elif defined(_M_IX86)
#   define RK__DEF_LZ__(V)                                                                         \
     (hi = (unsigned)((unsigned long long)V >> 32))                                                \
         ? (_BitScanReverse(&idx, (unsigned long)hi), 31u - (unsigned)idx)                         \
         : (_BitScanReverse(&idx, (unsigned long)(unsigned)V), 63u - (unsigned)idx)
#  endif
#  define RK__DEF_LZ_(T, V)                                                                        \
    if (!V) return bitsof(V);                                                                      \
    unsigned long idx;                                                                             \
    unsigned      hi;                                                                              \
    (void)hi;                                                                                      \
    return rk_static_if(sizeof(T) <= 4,                                                            \
                        (_BitScanReverse(&idx, (unsigned long)V),                                  \
                         31u - (unsigned)idx - (bitsof(unsigned) - bitsof(T))),                    \
                        (RK__DEF_LZ__(V)));
# else
#  define RK__DEF_LZ_(T, V)                                                                        \
    if (!V) { return bitsof(T); }                                                                  \
    unsigned count = 0;                                                                            \
    T        mask  = (T)1 << (bitsof(T) - 1);                                                      \
    while (!(V & mask)) { ++count, V <<= 1; }                                                      \
    return count;
# endif

# if rk_has_builtin(__builtin_ctzg)
#  define RK__DEF_TZ_(V) return V ? (unsigned)__builtin_ctzg(V) : bitsof(V);
# elif defined(__GNUC__)
#  define RK__DEF_TZ_(V)                                                                           \
    return V ? (unsigned)_Generic(V,                                                               \
                   default: __builtin_ctz(V),                                                      \
                   unsigned long: __builtin_ctzl(V),                                               \
                   unsigned long long: __builtin_ctzll(V)                                          \
                       RK__IFHAS_INT128(, u128 : (u64)(V)                                          \
                                              ? __builtin_ctzll((u64)V)                            \
                                              : 64 + __builtin_ctzll((u64)((u128)V >> 64))))       \
             : bitsof(V);

# elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#  ifdef _M_X64
#   define RK__DEF_TZ__(V) _BitScanForward64(&idx, (unsigned long long)V), (unsigned)idx

#  elif defined(_M_IX86)
#   define RK__DEF_TZ__(V)                                                                         \
     (lo = (unsigned)(V))                                                                          \
         ? (_BitScanForward(&idx, (unsigned long)lo), (unsigned)idx)                               \
         : (_BitScanForward(&idx, (unsigned long)(unsigned)((unsigned long long)V >> 32)),         \
            32u + (unsigned)idx)
#  endif
#  define RK__DEF_TZ_(V)                                                                           \
    if (!V) { return bitsof(V); }                                                                  \
    unsigned long idx;                                                                             \
    unsigned      lo;                                                                              \
    (void)lo;                                                                                      \
    return rk_static_if(sizeof(V) <= 4,                                                            \
                        (_BitScanForward(&idx, (unsigned long)(unsigned)V), (unsigned)idx),        \
                        (RK__DEF_TZ__(V)));
# else
#  define RK__DEF_TZ_(V)                                                                           \
    if (!V) { return bitsof(V); }                                                                  \
    unsigned count = 0;                                                                            \
    while (!(V & 1)) { ++count, V >>= 1; }                                                         \
    return count;
# endif

# if rk_has_builtin(__builtin_popcountg)
#  define RK__DEF_CO_(V) return (unsigned)__builtin_popcountg(V);
# elif defined(__GNUC__)
#  define RK__DEF_CO_(V)                                                                           \
    return (unsigned)_Generic(V,                                                                   \
        default: __builtin_popcount(V),                                                            \
        unsigned long: __builtin_popcountl(V),                                                     \
        unsigned long long: __builtin_popcountll(V)                                                \
            RK__IFHAS_INT128(, u128 : __builtin_popcountll((u64)V)                                 \
                                   + __builtin_popcountll((u64)((u128)V >> 64))));
# else
#  define RK__DEF_CO_(V)                                                                           \
    unsigned count = 0;                                                                            \
    while (V) { count++, V &= (V - 1); }                                                           \
    return count;
# endif

# define RK__DEF_STDCBIT_FUNS(T, N)                                                                 \
   static_fun rk_const unsigned stdc_leading_zeros_##N(T value) rk_unsequenced{RK__DEF_LZ_(         \
       T, value)} static_fun rk_const unsigned stdc_trailing_zeros_##N(T value) rk_unsequenced{     \
       RK__DEF_TZ_(value)} static_fun rk_const unsigned                stdc_count_ones_##N(T value) \
       rk_unsequenced{RK__DEF_CO_(value)} static_fun rk_const unsigned stdc_count_zeros_##N(        \
           T value) rk_unsequenced {                                                                \
     return bitsof(T) - stdc_count_ones_##N(value);                                                 \
   }                                                                                                \
   static_fun rk_const unsigned stdc_first_trailing_one_##N(T value) rk_unsequenced {               \
     return value ? stdc_trailing_zeros_##N(value) + 1u : 0u;                                       \
   }                                                                                                \
   static_fun rk_const unsigned stdc_leading_ones_##N(T value) rk_unsequenced {                     \
     return stdc_leading_zeros_##N((T)~value);                                                      \
   }                                                                                                \
   static_fun rk_const unsigned stdc_trailing_ones_##N(T value) rk_unsequenced {                    \
     return stdc_trailing_zeros_##N((T)~value);                                                     \
   }                                                                                                \
   static_fun rk_const unsigned stdc_first_leading_zero_##N(T value) rk_unsequenced {               \
     return value == (T) ~(T)0u ? 0u : stdc_leading_zeros_##N((T)~value) + 1u;                      \
   }                                                                                                \
   static_fun rk_const unsigned stdc_first_leading_one_##N(T value) rk_unsequenced {                \
     return value ? stdc_leading_zeros_##N(value) + 1u : 0u;                                        \
   }                                                                                                \
   static_fun rk_const unsigned stdc_first_trailing_zero_##N(T value) rk_unsequenced {              \
     return value == (T) ~(T)0u ? 0u : stdc_trailing_zeros_##N((T)~value) + 1u;                     \
   }                                                                                                \
   static_fun rk_const bool stdc_has_single_bit_##N(T value) rk_unsequenced {                       \
     return stdc_count_ones_##N(value) == 1u;                                                       \
   }                                                                                                \
   static_fun rk_const unsigned stdc_bit_width_##N(T value) rk_unsequenced {                        \
     return bitsof(T) - stdc_leading_zeros_##N(value);                                              \
   }                                                                                                \
   static_fun rk_const T stdc_bit_floor_##N(T value) rk_unsequenced {                               \
     return (T)(value ? ((T)1u << (stdc_bit_width_##N(value) - 1u)) : (T)0u);                       \
   }                                                                                                \
   static_fun rk_const T stdc_bit_ceil_##N(T value) rk_unsequenced {                                \
     if (!value) { return (T)1u; }                                                                  \
     size_t shift = bitsof(T) - stdc_leading_zeros_##N((T)(value - 1u));                            \
     return shift < bitsof(T) ? (T)((T)1u << shift) : (T)0u;                                        \
   }

RK_U_TYPES(RK__DEF_STDCBIT_FUNS)
# undef RK__DEF_LZ_
# undef RK__DEF_LZ__
# undef RK__DEF_TZ_
# undef RK__DEF_TZ__
# undef RK__DEF_CO_
# undef RK__DEF_STDCBIT_FUNS

#endif /* RK_STDBIT_FALLBACK */

/// @endcond

#define rk_assert_ptr_nonnull(ptr) rk_assert(((ptr) != rk_null) && #ptr " must not be rk_null.")

#define rk_assert_align_pow2(align)                                                                \
  rk_assert(stdc_has_single_bit(align) && #align " must be a power of two.")

#define rk_assert_valid_align(T, align)                                                            \
  rk_assert(alignof(T) <= (align) && #align " must be >= alignof(" #T ").")

static_fun rk_const size_t rk_align_up(size_t size, size_t align) {
  rk_assert_align_pow2(align);
#if rk_has_builtin(__builtin_align_up)
  return __builtin_align_up(size, align);
#else
  size_t mask = align - 1;
  rk_assert(size <= SIZE_MAX - mask && "Size overflow");
  return (size + mask) & ~mask;
#endif
}

static_fun rk_const size_t rk_align_pad(const void* ptr, size_t align) {
  rk_assert_align_pow2(align);
  return (-(uintptr_t)ptr) & (size_t)(align - 1);
}
RK_HEADER_END

#ifdef __cplusplus
template <class T, size_t N>
constexpr inline size_t RK__countof(T (&)[N]) noexcept {
  return N;
}
#endif
/// @}

#endif // RK_DEFS_H

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
