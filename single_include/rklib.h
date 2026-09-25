/* BEGIN INLINE: include/rklib.h */
// SPDX-License-Identifier: MIT
#ifndef RK_LIB_H
#define RK_LIB_H
// clang-format off
/* inlined from include/rklib.h:5: #include "rk_config.h" */
/* BEGIN INLINE: include/rk_config.h */
// SPDX-License-Identifier: MIT
/// @file rk_config.h
/// @version 1.0.0
/// @defgroup rk_config rklib Configuration Macros
/// @brief Configuration macros for rklib. All options relating to a non-included header are
/// ignored.
///
/// @warning All translation units using this library **must** share the same configuration.
/// Mismatches cause **ill-formed, no diagnostic required** (IFNDR) behaviour due to differing
/// struct layouts and function definitions.
///
/// ---
///
/// - **RK_IMPL** Must be `#define`d in exactly **one** translation unit before including any rklib
///   header. This provides the definitions for `extern inline` functions. All other translation
///   units must omit it.
///
/// - **RK_CUSTOM_ALLOCATORS** default: `1` Set to `1` to enable custom allocators. Every owning
///   library object stores the `Allocator` selected when it is initialised, and accepts an optional
///   allocator argument. `alloc_ctx` supplies the default for objects created without an explicit
///   allocator; changing it affects only subsequently created objects. Set to `0` to remove
///   allocator members and custom-allocator arguments and use direct `malloc`/`free` wrappers.
///
/// - **RKLIB_DEBUG** default: not defined Define to enable debug logging. `rk_log()` will print
///   diagnostics to `stderr` and `rk_assert()` will print the failed expression before calling
///   `abort()`. Has no runtime cost when `0`.
///
/// - **RK_ALLOC_MULTITHREADED** default: `0` Set to `1` to give `alloc_ctx` thread-local storage,
///   enabling a distinct default allocator per thread. Requires `RK_CUSTOM_ALLOCATORS == 1`.
///
/// - **RK_MALLOC_FAIL**(`cond, ctx, old_ptr, align, new_size`) default: `rk_assert(cond)` +
///   `abort()` Called inside the malloc-based allocator when an allocation returns `NULL`. `cond`
///   is the (null) pointer returned by the allocator. The remaining arguments provide context for
///   custom handlers (e.g. logging or fallback allocation). The handler must not return normally;
///   it must either abort or longjmp.
///
/// - **RK_MMAP_FAIL**(`cond, ctx, old_ptr, align, new_size`) default: `rk_assert(cond)` + `abort()`
///   Same as `RK_MALLOC_FAIL` but called by the page allocator (`mmap` /
///   `VirtualAlloc`) on failure.
///
/// - **RK_ARENA_FAIL**(`cond, ctx, old_ptr, align, new_size`) default: `rk_assert(cond)` +
///   `abort()` Called inside `rk_arena.h` allocators when the arena has no space left. `ctx` is the
///   `Arena*`. The handler must not return normally.
///
/// - **RK_POOL_FAIL**(`cond, ctx, old_ptr, align, new_size`) default: `rk_assert(cond)` + `abort()`
///   Called inside `rk_pool.h` allocators when all pool slots are occupied. `ctx` is the pool
///   pointer. The handler must not return normally.
///
/// - **RK_DICT_LOAD_NUM** / **RK_DICT_LOAD_DEN** default: `3` / `4` Integer fraction in `(0, 1)`
///   controlling when `rk_dict` rehashes. Lower values reduce collisions at the cost of more
///   memory; higher values do the opposite. Applies globally to all `Dict` instances.
///
/// - **rk_mult**(`x, y`) default: `((x) * (y))` Multiplication used internally for `size_t`
///   size/count computations (`sizeof_n`, `Vec`/`Pool` capacity sizing, etc.). The default is a
///   raw, unchecked multiply: an overflowing `count * sizeof(T)` silently wraps to a small value,
///   so a too-large `count` can lead to a successful but undersized allocation. Define as
///   `rk_mult_safe` (declared in `rk_defs.h`) to `abort()` on overflow instead, at the cost of a
///   runtime check (a branch and, on the overflowing path, a division) on every multiplication.
/// @{

#ifndef RK_CONFIG_H
#define RK_CONFIG_H
#if defined(__cplusplus) && !defined(__clang__)
# error "C++ support only for Clang"
#elif defined(_MSC_VER) && _MSC_VER < 1939
# error "Unsupported MSVC version"
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// Configuration Macros ///////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

// clang-format off
// Quick-reference: define any of these before including rklib headers.
// Full documentation is in the Doxygen block above.
//
// #define RK_IMPL                                           // one TU only
// #define RKLIB_DEBUG                                      // enable logging
// #define RK_CUSTOM_ALLOCATORS          1                  // enable custom allocators
// #define RK_ALLOC_MULTITHREADED        1                  // thread_local alloc_ctx
// #define RK_MALLOC_FAIL(cond, ctx, old_ptr, align, new_size)  ...
// #define RK_MMAP_FAIL( cond, ctx, old_ptr, align, new_size)   ...
// #define RK_ARENA_FAIL(cond, ctx, old_ptr, align, new_size)   ...
// #define RK_POOL_FAIL( cond, ctx, old_ptr, align, new_size)   ...
// #define RK_DICT_LOAD_NUM              3
// #define RK_DICT_LOAD_DEN              4
// #define rk_mult(x, y)                 ((x) * (y))  // or rk_mult_safe(x, y) to abort on overflow
// clang-format on

/// @brief Enables per-object custom allocators. Set to 0 to remove allocator members and use the
/// built-in malloc allocator directly.
#ifndef RK_CUSTOM_ALLOCATORS
# define RK_CUSTOM_ALLOCATORS 1
#elif RK_CUSTOM_ALLOCATORS != 0 && RK_CUSTOM_ALLOCATORS != 1
# error "RK_CUSTOM_ALLOCATORS must be 0 or 1"
#endif

#ifndef RK_ALLOC_MULTITHREADED
/// @brief Set to 1 to give `alloc_ctx` `thread_local` storage, enabling a per-thread default
/// allocator. Requires custom allocators to be enabled.
# define RK_ALLOC_MULTITHREADED 0
#elif RK_ALLOC_MULTITHREADED != 0 && RK_ALLOC_MULTITHREADED != 1
# error "RK_ALLOC_MULTITHREADED must be 0 or 1"
#endif
#if RK_ALLOC_MULTITHREADED && !RK_CUSTOM_ALLOCATORS
# error "RK_ALLOC_MULTITHREADED requires RK_CUSTOM_ALLOCATORS"
#elif RK_ALLOC_MULTITHREADED
# define RK_alloc_tl thread_local
#else
# define RK_alloc_tl /* no thread local storage */
#endif

/// @defgroup alloc_failure_handlers Allocator Failure Handlers
/// @brief Customizable failure handlers for each allocator type. All handlers receive `cond` (the
/// null pointer returned on failure) plus context arguments for custom logging or fallback logic.
/// The handler must not return normally — it must `abort()` or `longjmp()`.
/// @{
#ifndef RK_MALLOC_FAIL
/// @brief Malloc-based allocator failure handler.
# define RK_MALLOC_FAIL(cond, ctx, old_ptr, align, new_size)                                       \
   do {                                                                                            \
     (void)(ctx), (void)(old_ptr), (void)(align), (void)(new_size);                                \
     bool RK___failcond = !!(cond);                                                                \
     rk_assert(RK___failcond && "malloc allocation failure");                                      \
     RK___failcond ? (void)0 : abort();                                                            \
   } while (0)
#endif

#ifndef RK_MMAP_FAIL
/// @brief Page allocation failure handler (mmap / VirtualAlloc).
# define RK_MMAP_FAIL(cond, ctx, old_ptr, align, new_size)                                         \
   do {                                                                                            \
     (void)(ctx), (void)(old_ptr), (void)(align), (void)(new_size);                                \
     bool RK___failcond = !!(cond);                                                                \
     rk_assert(RK___failcond && "map allocation failure");                                         \
     RK___failcond ? (void)0 : abort();                                                            \
   } while (0)
#endif

#ifndef RK_ARENA_FAIL
/// @brief Arena allocator failure handler (`ctx` is the `Arena*`).
# define RK_ARENA_FAIL(cond, ctx, old_ptr, align, new_size)                                        \
   do {                                                                                            \
     (void)(ctx), (void)(old_ptr), (void)(align), (void)(new_size);                                \
     bool RK___failcond = !!(cond);                                                                \
     rk_assert(RK___failcond && "arena allocation failed");                                        \
     RK___failcond ? (void)0 : abort();                                                            \
   } while (0)
#endif

#ifndef RK_POOL_FAIL
/// @brief Pool allocator failure handler (`ctx` is the pool pointer).
# define RK_POOL_FAIL(cond, ctx, old_ptr, align, new_size)                                         \
   do {                                                                                            \
     (void)(ctx), (void)(old_ptr), (void)(align), (void)(new_size);                                \
     bool RK___failcond = !!(cond);                                                                \
     rk_assert(RK___failcond && "pool allocation failed");                                         \
     RK___failcond ? (void)0 : abort();                                                            \
   } while (0)
#endif
/// @}

/// @brief Maximum load factor for `rk_dict` hash tables, expressed as the integer fraction
/// `RK_DICT_LOAD_NUM / RK_DICT_LOAD_DEN` in `(0, 1)`. The table rehashes when `(count + n_deleted)
/// / cap` exceeds this value. Kept as an integer ratio (not a float) so the growth check is exact
/// cross-multiplication against `cap`, with no float conversion of `size_t` on every insert.
#ifndef RK_DICT_LOAD_NUM
# define RK_DICT_LOAD_NUM 3
#endif
#ifndef RK_DICT_LOAD_DEN
# define RK_DICT_LOAD_DEN 4
#endif

/// @brief `size_t` multiplication used for size/count computations throughout the library
/// (`sizeof_n`, `Vec`/`Pool` capacity sizing, etc.). Unchecked by default — an overflowing
/// `count * sizeof(T)` silently wraps, which can turn a too-large `count` into a small, successful
/// allocation. Define this as `rk_mult_safe` (declared in `rk_defs.h`) before including any rklib
/// header to `abort()` on overflow instead, at the cost of a runtime check on every multiplication.
#ifndef rk_mult
# define rk_mult(x, y) ((x) * (y))
#endif

/// @}
#endif // RK_CONFIG_H

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
/* END INLINE: include/rk_config.h */
/* inlined from include/rklib.h:6: #include "rk_defs.h" */
/* BEGIN INLINE: include/rk_defs.h */
// SPDX-License-Identifier: MIT
/// @file rk_defs.h
/// @version 1.0.0
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
/* inlined from include/rk_defs.h:18: #include "rk_config.h" */
/* skipped already-included: "include/rk_config.h" */

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
# define RK__IGNWARN_MSC_BEG(warn)
# define RK__IGNWARN_MSC_END()
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
#  if defined(_DEBUG)
  __debugbreak();
#  endif
  __assume(0);
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

/// @brief Overflow-checked `size_t` multiplication; `abort()`s instead of wrapping. Not used by
/// default — see the `rk_mult` config hook in `rk_config.h` to opt every size/count computation in
/// the library into this behaviour (`#define rk_mult(x, y) rk_mult_safe(x, y)`).
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
                 typeof_decayed(expr) f;                                                           \
                 to_type t;                                                                        \
                }){(expr)}                                                                         \
                    .t,                                                                            \
                *(to_type*)rk_memcpy(&(to_type){RK_ZINIT},                                         \
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
  rk_static_if(rk_ensure_numclass_compatible(RK__wider_t(a1, a2), a3)                              \
                       + sizeof(RK__wider_t(a1, a2))                                               \
                   >= sizeof(typeof(a3)),                                                          \
               RK__wider_t(a1, a2), (typeof(a3))0)

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
/* END INLINE: include/rk_defs.h */
/* inlined from include/rklib.h:7: #include "rk_trees.h" */
/* BEGIN INLINE: include/rk_trees.h */
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
/* inlined from include/rk_trees.h:65: #include "rk_alloc.h" */
/* BEGIN INLINE: include/rk_alloc.h */
// SPDX-License-Identifier: MIT
/// @file rk_alloc.h
/// @version 1.0.0
/// @defgroup rk_alloc Allocator Interface
/// @brief Customizable memory allocator abstraction for C.
///
/// Provides an allocator interface built around `Allocator` — a vtable pointer plus an optional
/// context pointer. Two predefined allocators are provided: `alloc_malloc_allocator` and
/// `alloc_page_allocator`. Custom allocators can be created by filling
/// an `AllocatorVTable` and constructing an `Allocator`.
///
/// Allocation failures are handled inside the allocator, not at call sites. The provided allocators
/// invoke the overridable failure macros from `rk_config.h` (`RK_MALLOC_FAIL`, `RK_MMAP_FAIL`,
/// etc.), which by default assert and abort. Callers never need to NULL-check allocation results.
///
/// `RK_CUSTOM_ALLOCATORS` controls whether allocators are threaded through objects — see
/// `rk_config.h`. When disabled, per-object `Allocator` fields, custom-allocator arguments, and
/// function-pointer dispatch are compiled out.
///
/// When `RK_ALLOC_MULTITHREADED == 1`, `alloc_ctx` has thread-local storage duration, giving each
/// thread its own construction-time default allocator.
/// @{
#ifndef RK_ALLOC_H
#define RK_ALLOC_H

#ifndef _MSC_VER
# include <sys/mman.h>
# include <unistd.h>

# ifndef MAP_ANONYMOUS
#  ifdef MAP_ANON
#   define MAP_ANONYMOUS MAP_ANON
#  elif defined(__linux__)                                                                         \
      && (defined(__x86_64__) || defined(__i386__) || defined(__arm__) || defined(__aarch64__)     \
          || defined(__riscv) || defined(__powerpc__) || defined(__powerpc64__)                    \
          || defined(__s390__))
// The kernel exposes MAP_ANONYMOUS as 0x20 on these architectures (the "asm-generic" layout).
// Some Linux architectures override it (e.g. MIPS uses 0x0800, PA-RISC/Alpha use 0x10) — do not
// extend this list to an architecture without confirming its own uapi/asm/mman.h value; a wrong
// hardcoded value here is a silent runtime bug, not a build failure.
#   define MAP_ANONYMOUS 0x20
#  else
#   error "RK_MAP_ANONYMOUS unknown on this platform, change posix feature test macro"
#  endif
# endif
#endif
/* inlined from include/rk_alloc.h:47: #include "rk_defs.h" */
/* skipped already-included: "include/rk_defs.h" */
RK_HEADER_BEGIN

/// @brief Allocation logging macros. Emit a tagged source location to `stderr` when `RKLIB_DEBUG
/// defined`; expand to nothing otherwise. Can be used by custom allocators to get the same logging
/// behaviour as the built-in ones.
#define alloc_log_new                  rk_log("[alloc]  %s:%d ", __FILE__, __LINE__)
#define alloc_log_renew                rk_log("[renew]  %s:%d ", __FILE__, __LINE__)
#define alloc_log_delete               rk_log("[delete] %s:%d ", __FILE__, __LINE__)
#define rk_allocator_disabled_assert() static_assert_expr(0, "Allocators Disabled")
#define rk_allocator_disabled()        ((Allocator){.ctx = (void*)rk_allocator_disabled_assert()})

#if RK_CUSTOM_ALLOCATORS
# define rk_disable_if(...) __VA_ARGS__
#else
# define rk_disable_if(...) ((void*)rk_allocator_disabled_assert())
#endif

/// @struct Allocator
/// @brief General-purpose allocator handle: a vtable pointer plus an optional context pointer. Pass
/// by value to init functions; pass by pointer to allocator-generic macros.
///
/// When custom allocators are enabled, rklib macros that accept an optional allocator argument
/// default to `alloc_ctx`. Objects capture that allocator when initialised, so changing `alloc_ctx`
/// affects only subsequently created objects.
///
/// Two predefined `Allocator` instances are provided:
///   - `alloc_malloc_allocator` — thin wrappers over `malloc`/`free` (or `_aligned_malloc` on MSVC
///     for over-aligned types). This is the default `alloc_ctx`.
///   - `alloc_page_allocator` — OS page allocation (`mmap` / `VirtualAlloc`). All allocations are
///     page-aligned; alignments larger than the page size are not supported.
///
/// @note Custom allocators must handle failures locally (via the failure macros in `rk_config.h`).
/// Returning `NULL` from an allocator leads to immediate undefined behaviour at the call site.

/// @brief Allocation Function.
/// @param size  Desired size of the allocation in bytes.
/// @param align Desired Alignment of the allocation. Must be a power of two.
/// @param ctx   Allocator context. May be `NULL` depending on the allocator.
/// @return A **valid** pointer to the allocated memory. May only be `NULL` if `size` is zero.
/// @note Allocation failure is expected to be handled locally by the function via the respective
/// failure macros defined in `rk_config.h`.
typedef void*(alloc_allocation_f)(size_t size, size_t align, void* ctx);

/// @brief Reallocation Function.
/// @param old_ptr The pointer to the allocation to be deallocated. If `NULL`, this function shall
/// act like the corresponding `alloc_allocation_f` of the same allocator.
/// @param old_size The size of the allocation to be deallocated. In some allocators such as
/// `alloc_malloc_allocator`, this parameter is discarded.
/// @param new_size The desired new size of the allocation. If this is zero, this function shall act
/// like the corresponding `alloc_deallocation_f` of the same allocator.
/// @param align Desired Alignment of the allocation. Must match the alignment of the corresponding
/// allocation function call.
/// @param ctx      Allocator context. May be `NULL` depending on the allocator.
/// @return A **valid** pointer to the allocated memory. May only be `NULL` if `new_size` is zero.
typedef void*(alloc_reallocation_f)(void* old_ptr, size_t old_size, size_t new_size, size_t align,
                                    void* ctx);

/// @brief Deallocation Function.
/// @param ptr The pointer to the allocation to be freed. If `NULL`, this function shall be a no-op.
/// @param old_size The size of the allocation to be deallocated. In some allocators such as
/// `alloc_malloc_allocator`, this parameter is discarded.
/// @param align Desired Alignment of the allocation. Must match the alignment of the corresponding
/// allocation function call.
/// @param ctx      Allocator context. May be `NULL` depending on the allocator.
typedef void(alloc_deallocation_f)(void* ptr, size_t old_size, size_t align, void* ctx);

/// @brief Vtable for an allocator. Holds function pointers for allocation, deallocation, and
/// reallocation. Shared across all `Allocator` instances that use the same strategy (e.g. all arena
/// allocators share one vtable). Implementations of each slot must follow the contracts described
/// on the `alloc_allocation_f`, `alloc_reallocation_f` and `alloc_deallocation_f` typedefs below.
typedef struct AllocatorVTable {
  alloc_allocation_f*   rk_alloc_alignsize(2, 1) alloc_f;
  alloc_reallocation_f* rk_alloc_alignsize(4, 3) realloc_f;
  alloc_deallocation_f* dealloc_f;
} AllocatorVTable;

typedef struct Allocator {
  const AllocatorVTable* vtab; ///< Vtable pointer
  void*                  ctx;  ///< Optional Context Pointer
} Allocator;

static_fun alloc_allocation_f   RK__malloc_allocate;
static_fun alloc_reallocation_f RK__malloc_reallocate;
static_fun alloc_deallocation_f RK__malloc_deallocate;
static const AllocatorVTable alloc_malloc_allocator_vtable = {.alloc_f   = RK__malloc_allocate,
                                                              .realloc_f = RK__malloc_reallocate,
                                                              .dealloc_f = RK__malloc_deallocate};

/// @brief Default allocator using `malloc`/`free` (or `_aligned_malloc` on MSVC for over-aligned
/// requests). Set as the initial value of `alloc_ctx`.
rk_unused static const Allocator alloc_malloc_allocator
    = {.vtab = &alloc_malloc_allocator_vtable, .ctx = rk_null};

static_fun alloc_allocation_f          RK__page_allocate;
static_fun alloc_reallocation_f        RK__page_reallocate;
static_fun alloc_deallocation_f        RK__page_deallocate;
rk_unused static const AllocatorVTable alloc_page_allocator_vtable
    = {.alloc_f   = RK__page_allocate,
       .realloc_f = RK__page_reallocate,
       .dealloc_f = RK__page_deallocate};

/// @brief Allocator backed by OS page mapping (`mmap` / `VirtualAlloc`). All allocations are
/// page-aligned and zero-initialized. Alignments greater than the system page size are not
/// supported.
rk_unused static const Allocator alloc_page_allocator
    = {.vtab = &alloc_page_allocator_vtable, .ctx = rk_null};

#if RK_CUSTOM_ALLOCATORS
# define RK__ALLOCCTX_STORAGE   extern_var RK_alloc_tl
# define RK__ALLOCCTX_INIT(...) extern_def({__VA_ARGS__})
#else
# define RK__ALLOCCTX_STORAGE   static const
# define RK__ALLOCCTX_INIT(...) = {__VA_ARGS__}
#endif

/// @brief Default allocator used by all rklib macros when no explicit allocator argument is
/// provided. Defaults to `alloc_malloc_allocator`. Objects capture its value when initialised, so
/// replacing it affects only subsequently created objects. When `RK_ALLOC_MULTITHREADED == 1`, it
/// is thread-local. Must always contain a valid, fully initialised `Allocator`.
RK__ALLOCCTX_STORAGE Allocator alloc_ctx RK__ALLOCCTX_INIT(.vtab = &alloc_malloc_allocator_vtable,
                                                           .ctx  = rk_null);

/// @brief `void* alloc_allocate(size_t bytes, size_t align, Allocator alloc = alloc_ctx)` - Raw
/// allocation: allocates `bytes` bytes with the given alignment. Prefer `alloc_new` for typed
/// allocations.
/// @param bytes Number of bytes to allocate
/// @param align Alignment; must be a power of two
/// @param alloc Optional allocator; defaults to `alloc_ctx`
/// @return pointer to the allocated memory. `NULL` iff `bytes` is zero.
#define alloc_allocate(bytes, align, ...)                                                          \
  ((void*)rk_overload(RK__alloc_ALLOCATE, bytes, align, ##__VA_ARGS__))

/// @brief `void* alloc_reallocate(void* ptr, size_t old_bytes, size_t new_bytes, size_t align,
/// Allocator alloc = alloc_ctx)` - Raw reallocation. If `ptr` is `NULL`, behaves like
/// `alloc_allocate`. If `new_bytes` is zero, behaves like `alloc_deallocate`. Prefer `alloc_renew`
/// for typed use.
/// @param ptr       Existing allocation (or `NULL`)
/// @param old_bytes Size of the existing allocation in bytes
/// @param new_bytes Desired new size in bytes
/// @param align     Alignment; must match the original allocation
/// @param alloc     Optional allocator; defaults to `alloc_ctx`
/// @return pointer to the allocated memory. `NULL` iff `new_bytes` is zero.
#define alloc_reallocate(ptr, old_bytes, new_bytes, align, ...)                                    \
  ((void*)rk_overload(RK__alloc_REALLOCATE, ptr, old_bytes, new_bytes, align, ##__VA_ARGS__))

/// @brief `void alloc_deallocate(void* ptr, size_t bytes, size_t align, Allocator alloc =
/// alloc_ctx)` - Raw deallocation. If `ptr` is `NULL`, this is a no-op. Prefer `alloc_delete` for
/// typed use.
/// @param ptr   Pointer to the memory to free (or `NULL`)
/// @param bytes Size of the allocation in bytes
/// @param align Alignment; must match the original allocation
/// @param alloc Optional allocator; defaults to `alloc_ctx`
#define alloc_deallocate(ptr, bytes, align, ...)                                                   \
  ((void)rk_overload(RK__alloc_DEALLOCATE, ptr, bytes, align, ##__VA_ARGS__))

/// @brief `T* alloc_new(T, size_t count, Allocator alloc = alloc_ctx)` - Allocates memory for an
/// array of `count` elements of type `T` using the specified allocator.
/// @param T         The type of elements to allocate
/// @param count     Count of elements to allocate
/// @param allocator The Allocator to use (defaults to `alloc_ctx`)
/// @return Pointer to allocated and aligned memory block, cast to `T*`.
#define alloc_new(T, count, ...) ((T*)rk_overload(RK__alloc_NEW, T, count, ##__VA_ARGS__))

/// @brief `T* alloc_renew(T* ptr, size_t old_count, size_t new_count, Allocator alloc = alloc_ctx)`
/// - Resizes (reallocates) memory block to hold `new_count` elements of the same type, for standard
/// alignment according to the Allocator.
/// @param ptr       Pointer to the existing allocated memory
/// @param old_count Number of elements of type T previously allocated
/// @param new_count Number of elements of type T to allocate after resizing
/// @param allocator The Allocator to use (defaults to `alloc_ctx`)
/// @return Pointer to the reallocated and aligned memory block, cast to the same pointer type.
/// @warning Must not be used on pointers from over-aligned allocations
#define alloc_renew(ptr, old_count, new_count, ...)                                                \
  ((typeof(ptr))rk_overload(RK__alloc_RENEW, ptr, old_count, new_count, ##__VA_ARGS__))

/// @brief `void alloc_delete(T* ptr, size_t old_count, Allocator alloc = alloc_ctx)` - Deallocates
/// memory.
/// @param ptr       Pointer to the memory to deallocate
/// @param old_count Number of elements of type T originally allocated
/// @param allocator The Allocator to use (defaults to `alloc_ctx`)
#define alloc_delete(ptr, old_count, ...)                                                          \
  ((void)rk_overload(RK__alloc_DELETE, ptr, old_count, ##__VA_ARGS__))

/// @brief `T* alloc_new_aligned(T, size_t count, size_t align, Allocator alloc = alloc_ctx)` -
/// Allocates memory for an array of `count` elements of type T with specified alignment.
/// @param T         The type of elements to allocate
/// @param count     Number of elements to allocate
/// @param align     Desired alignment of the memory, must be a power of two
/// @param allocator The Allocator to use (defaults to `alloc_ctx`)
/// @return Pointer to allocated and aligned memory block, cast to `T*`.
#define alloc_new_aligned(T, count, align, ...)                                                    \
  ((T*)rk_overload(RK__alloc_ALIGNED_NEW, T, count, align, ##__VA_ARGS__))

/// @brief `T* alloc_renew_aligned(T* ptr, size_t old_count, size_t new_count, size_t align,
/// Allocator alloc = alloc_ctx)` - Resizes (reallocates) memory block to hold `new_count` elements
/// of the same type.
/// @param ptr       Pointer to the existing allocated memory
/// @param old_count Number of elements of type T previously allocated
/// @param new_count Number of elements of type T to allocate after resizing
/// @param align     Alignment of the memory; must match the original allocation
/// @param allocator The Allocator to use (defaults to `alloc_ctx`)
/// @return Pointer to reallocated and aligned memory block, cast to the same pointer type.
#define alloc_renew_aligned(ptr, old_count, new_count, align, ...)                                 \
  ((typeof(ptr))rk_overload(RK__alloc_ALIGNED_RENEW, ptr, old_count, new_count,                    \
                            align, ##__VA_ARGS__))

/// @brief `void alloc_delete_aligned(T* ptr, size_t old_count, size_t align, Allocator alloc =
/// alloc_ctx)` - Deallocates aligned memory.
/// @param ptr       Pointer to the memory to deallocate
/// @param old_count Number of elements of type T originally allocated
/// @param align     Alignment of the memory; must match the original allocation
/// @param allocator The Allocator to use (defaults to `alloc_ctx`)
#define alloc_delete_aligned(ptr, old_count, align, ...)                                           \
  ((void)rk_overload(RK__alloc_ALIGNED_DELETE, ptr, old_count, align, ##__VA_ARGS__))

/// @brief `void* malloc_allocate(size_t nbytes, size_t align)` - Allocates `nbytes` bytes of memory
/// with the specified alignment.
/// @note Zero-sized allocations are guaranteed to return a null pointer. Adjusts size to be a
/// multiple of alignment on some platforms.
/// @param nbytes Number of bytes to allocate
/// @param align  Desired alignment of the memory; must be a power of two
/// @return Pointer to allocated and aligned memory block, or `NULL` iff `nbytes` is zero.
/// @attention Do **not** mix these macros defined here with regular `malloc`/`free` for the same
/// pointers.
#define malloc_allocate(nbytes, align) ((void*)RK__malloc_ALLOCATE(nbytes, align))

/// @brief `void* malloc_reallocate(void* ptr, size_t obytes, size_t nbytes, size_t align)` -
/// Resizes an aligned memory block from `obytes` to `nbytes` bytes.
/// @note On MSVC, calls `_aligned_realloc`. On other platforms, allocates a new block, copies, and
/// frees the old one (no in-place realloc available). For standard-aligned allocations prefer
/// `malloc_renew`; for over-aligned allocations this is required.
/// @param ptr    Pointer to the existing allocated memory
/// @param obytes Old size of the allocation in bytes
/// @param nbytes New size of the allocation in bytes
/// @param align  Alignment of the memory; must match the original allocation
/// @return Pointer to reallocated and aligned memory block.
/// @attention Do **not** mix these macros defined here with regular `malloc`/`free` for the same
/// pointers.
#define malloc_reallocate(ptr, obytes, nbytes, align)                                              \
  ((void*)RK__malloc_REALLOCATE(ptr, obytes, nbytes, align))

/// @brief `void malloc_deallocate(void* ptr, size_t align)` - Deallocates an aligned memory block
/// previously allocated with `malloc_allocate` or `malloc_reallocate`.
/// @param ptr   Pointer to the memory to deallocate
/// @param align Alignment of the memory; must match the original allocation
/// @attention Do **not** mix these macros defined here with regular `malloc`/`free` for the same
/// pointers.
#define malloc_deallocate(ptr, align)       ((void)RK__malloc_DEALLOCATE(ptr, align))

/// @brief `T* malloc_new(T, size_t count)` - Allocates memory for an array of `count` elements of
/// type `T` using `malloc`.
/// @param T     The type of elements to allocate
/// @param count Number of elements to allocate
/// @return Pointer to allocated memory block, cast to `T*`.
/// @attention Do **not** mix these macros defined here with regular `malloc`/`free` for the same
/// pointers.
/// @note Zero-sized allocations are guaranteed to return a null pointer. Errors are handled via the
/// `RK_MALLOC_FAIL` macro that may be redefined by the user.
#define malloc_new(T, count)                ((T*)RK__malloc_NEW(T, count))

/// @brief `T* malloc_renew(T* ptr, size_t count)` - Resizes (reallocates) memory block to hold
/// `count` elements of the same type.
/// @note Passing `count == 0` frees the memory. Passing `ptr == NULL` is equivalent to calling
/// `malloc_new`.
/// @param ptr   Pointer to the existing allocated memory
/// @param count Number of elements of type T to allocate after resizing
/// @return Pointer to reallocated memory block, cast to the same pointer type
/// @warning Must not be used on pointers from over-aligned allocations
#define malloc_renew(ptr, count)            ((typeof(ptr))RK__malloc_RENEW(ptr, count))

/// @brief `void malloc_delete(T* ptr)` - Deallocates memory previously allocated with one of the
/// macros defined in this interface.
/// @param ptr Pointer to the memory to deallocate
/// @attention Do **not** mix these macros defined here with regular `malloc`/`free` for the same
/// pointers.
#define malloc_delete(ptr)                  ((void)RK__malloc_DELETE(ptr))

/// @brief `T* malloc_new_aligned(T, size_t count, size_t align)` - Allocates memory for an array of
/// `count` elements of type `T` with specified alignment using malloc (or _aligned_malloc with
/// standard alignment on Msvc).
/// @note Zero-sized allocations are guaranteed to return a null pointer. Adjusts size to be a
/// multiple of alignment on some platforms.
/// @param T     The type of elements to allocate
/// @param count Number of elements to allocate
/// @param align Desired alignment of the memory, must be a power of two
/// @return Pointer to allocated memory block, cast to `T*`, or `NULL` iff `count` is zero.
#define malloc_new_aligned(T, count, align) ((T*)RK__malloc_ALIGNED_NEW(T, count, align))

/// @brief `T* malloc_renew_aligned(T* ptr, size_t old_count, size_t new_count, size_t align)` -
/// Resizes (reallocates) an aligned memory block to hold `new_count` elements of the same type.
/// @note On MSVC, calls `_aligned_realloc`. On other platforms, allocates a new block, copies, and
/// frees the old one (no in-place realloc available). For standard-aligned types prefer
/// `malloc_renew`; for over-aligned types this is required.
/// @param ptr       Pointer to the existing allocated memory
/// @param old_count Old number of elements of type T
/// @param new_count New number of elements of type T
/// @param align     Alignment of the memory; must be a power of two
/// @return Pointer to reallocated memory block. `NULL` iff `new_count` is zero.
#define malloc_renew_aligned(ptr, old_count, new_count, align)                                     \
  ((typeof(ptr))RK__malloc_ALIGNED_RENEW(ptr, old_count, new_count, align))

/// @brief `void malloc_delete_aligned(T* ptr)` - Deallocates memory previously allocated with
/// malloc_new_aligned or with malloc_new for an over-aligned type. On non-MSVC it's always
/// identical to malloc_delete; on MSVC it uses _aligned_free instead of free
/// @param ptr Pointer to the memory to deallocate
/// @attention Do **not** mix these macros defined here with regular `malloc`/`free` for the same
/// pointers.
#define malloc_delete_aligned(ptr) ((void)RK__malloc_ALIGNED_DELETE(ptr))

/// @brief Allocate memory using OS-backed page mapping (`mmap` / `VirtualAlloc`). The returned
/// memory is zero-initialized and page-aligned. Allocation failures invoke `RK_MMAP_FAIL`, which
/// aborts by default.
/// @param size Size in bytes. Rounded up to the next page boundary internally.
/// @note Passing 0 returns `NULL` without invoking the failure handler.
/// @return Pointer to the allocated memory.
static_fun void* page_alloc(size_t size);

/// @brief Reallocate memory previously allocated with `page_alloc()`. On Linux, uses `mremap`
/// (in-place when possible). On other POSIX platforms, allocates a new region, copies, and unmaps
/// the old one. On Windows, uses `VirtualAlloc` + copy + `VirtualFree`.
/// @param ptr Pointer to the existing block (or `NULL` to act like `page_alloc`)
/// @param old_size Current size in bytes
/// @param new_size New size in bytes (or 0 to act like `page_free`)
/// @return Pointer to the reallocated memory block.
static_fun void* page_realloc(void* ptr, size_t old_size, size_t new_size);

/// @brief Free memory allocated via `page_alloc()`.
/// @param ptr  Pointer to the memory block to free
/// @param size Size of the block being freed, in bytes (must match allocation)
/// @note Calling this with `size == 0` is a no-op.
static_fun void  page_free(void* ptr, size_t size);

/// @brief `T* rk_arrdup(T* src, size_t count, Allocator alloc = alloc_ctx)` - Copies an array of
/// objects from `src` onto allocated storage
/// @param src       The address of the array (must be typed correctly)
/// @param count     The count of objects to copy
/// @param allocator The Allocator to use (defaults to `alloc_ctx`)
/// @return A pointer to the allocated array
#define rk_arrdup(src, count, ...)                                                                 \
  ((typeof(((void)0, (src)[0]))*)rk_overload(RK__ARRDUP, src, count, ##__VA_ARGS__))

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

#if RK_CUSTOM_ALLOCATORS
# define rk_assert_allocator_valid(_alloc) rk_assert((_alloc).vtab && "Invalid Allocator")
#else
# define rk_assert_allocator_valid(_alloc) ((void)0)
#endif

#if RK_CUSTOM_ALLOCATORS
# define RK_IFALLOC(...) __VA_ARGS__
# define rk_set_alloc_fallback(_alloc)                                                             \
   ((void)(rk_likely((_alloc).vtab)                                                                \
               ? alloc_ctx                                                                         \
               : (rk_assert_allocator_valid(alloc_ctx), (_alloc) = alloc_ctx)))
#else
# define RK_IFALLOC(...)
# define rk_set_alloc_fallback(_alloc) ((void)0)
#endif

///////////////////////// Page Allocator /////////////////////////////////
#if defined(_MSC_VER) && !defined(_WINDOWS_)
__declspec(dllimport) void* __stdcall VirtualAlloc(void* lpAddress, size_t dwSize,
                                                   unsigned long flAllocationType,
                                                   unsigned long flProtect);
__declspec(dllimport) int __stdcall   VirtualFree(void* lpAddress, size_t dwSize,
                                                  unsigned long dwFreeType);
#endif

static_fun size_t RK__mmap_page_size(void) {
#ifndef _MSC_VER
  long ps = sysconf(_SC_PAGESIZE);
  RK_MMAP_FAIL(ps != -1, ps, rk_null, 0, 0);
  return (size_t)ps;
#else
  return 4096;
#endif
}

static_fun rk_malloc_fun rk_alloc_size(1) void* page_alloc(size_t size) {
  if rk_unlikely (!size) { return rk_null; }
  size_t ps = RK__mmap_page_size();
  size      = rk_align_up(size, ps);
#ifndef _MSC_VER
  void* res = mmap(rk_null, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  RK_MMAP_FAIL(res != MAP_FAILED, ps, rk_null, 0, size);
#else
  void* res = VirtualAlloc(rk_null, size, 0x00001000 | 0x00002000, 0x04);
  RK_MMAP_FAIL(res, ps, rk_null, 0, size);
#endif
  return res;
}

static_fun void page_free(void* ptr, size_t size) {
  if rk_unlikely (!size) { return; }
  size_t ps = RK__mmap_page_size();
  size      = rk_align_up(size, ps);
#ifndef _MSC_VER
  int r = munmap(ptr, size);
  RK_MMAP_FAIL(r == 0, ps, ptr, 0, size);
#else
  int r = VirtualFree(ptr, 0, 0x00008000);
  RK_MMAP_FAIL(r != 0, ps, ptr, 0, size);
#endif
}

static_fun rk_alloc_size(3) void* page_realloc(void* ptr, size_t old_size, size_t new_size) {
  if (!old_size) { return page_alloc(new_size); }
  if (!new_size) { return page_free(ptr, old_size), rk_null; }
  size_t ps      = RK__mmap_page_size();
  size_t al_size = rk_align_up(new_size, ps), al_oldsize = rk_align_up(old_size, ps);
  if (al_size == al_oldsize) {
    return ptr;
  } else if (al_size < al_oldsize) {
#ifndef _MSC_VER
    int r = munmap((char*)ptr + al_size, al_oldsize - al_size);
    RK_MMAP_FAIL(r == 0, ps, ptr, 0, new_size);
#else
    int r = VirtualFree((char*)ptr + al_size, al_oldsize - al_size, 0x4000);
    RK_MMAP_FAIL(r != 0, ps, ptr, 0, new_size);
#endif
    return ptr;
  } else {
#if defined(__linux__) && defined(MREMAP_MAYMOVE)
    void* res = mremap(ptr, al_oldsize, al_size, MREMAP_MAYMOVE);
    RK_MMAP_FAIL(res != MAP_FAILED, ps, ptr, 0, new_size);
#elif !defined(_MSC_VER)
    void* res = mmap(rk_null, al_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    RK_MMAP_FAIL(res != MAP_FAILED, ps, rk_null, 0, new_size);
    rk_memcpy(res, ptr, old_size);
    int r = munmap(ptr, al_oldsize);
    RK_MMAP_FAIL(r == 0, ps, ptr, 0, new_size);
#else
    void* res = VirtualAlloc(rk_null, al_size, 0x00001000 | 0x00002000, 0x04);
    RK_MMAP_FAIL(res, ps, rk_null, 0, new_size);
    rk_memcpy(res, ptr, old_size);
    int r = VirtualFree(ptr, 0, 0x00008000);
    RK_MMAP_FAIL(r != 0, ps, ptr, 0, new_size);
#endif
    return res;
  }
}

static_fun rk_malloc_fun rk_alloc_alignsize(2, 1) void* RK__page_allocate(size_t       size,
                                                                          size_t align rk_unused,
                                                                          void* ctx    rk_unused) {
  rk_assert(align <= RK__mmap_page_size() && "Wrong alignment");
  return page_alloc(size);
}

static_fun rk_alloc_alignsize(4, 3) void* RK__page_reallocate(void* ptr, size_t old_size,
                                                              size_t       new_size,
                                                              size_t align rk_unused,
                                                              void* ctx    rk_unused) {
  rk_assert(align <= RK__mmap_page_size() && "Wrong alignment");
  return page_realloc(ptr, old_size, new_size);
}

static_fun void RK__page_deallocate(void* ptr, size_t old_size, size_t align rk_unused,
                                    void* ctx rk_unused) {
  rk_assert(align <= RK__mmap_page_size() && "Wrong alignment");
  page_free(ptr, old_size);
}

///////////////////////////////////    Malloc wrappers   ///////////////////////////////////////////
static_fun rk_forceinline rk_malloc_fun rk_alloc_size(1) void* RK__malloc_f(size_t size) {
  if rk_unlikely (!size) { return rk_null; }
  void* res = malloc(size);
  RK_MALLOC_FAIL(res, rk_null, rk_null, RK_malloc_align, size);
  return res;
}

static_fun rk_forceinline void RK__free_f(void* ptr) {
  if rk_unlikely (ptr == rk_null) { return; }
  free(ptr);
}

static_fun rk_forceinline rk_alloc_size(2) void* RK__realloc_f(void* ptr, size_t size) {
  if (!size) { return RK__free_f(ptr), rk_null; }
  if (ptr == rk_null) { return RK__malloc_f(size); }
  void* res = realloc(ptr, size);
  RK_MALLOC_FAIL(res, rk_null, ptr, RK_malloc_align, size);
  return res;
}

static_fun rk_malloc_fun rk_alloc_alignsize(2, 1) void* RK__aligned_alloc_f(size_t size,
                                                                            size_t align) {
  rk_assert_align_pow2(align);
  if rk_unlikely (!size) { return rk_null; }
  align = rk_max(align, RK_malloc_align);
  size  = rk_align_up(size, align);
#ifndef _MSC_VER
  void* res = aligned_alloc(align, size);
#else
  void* res = _aligned_malloc(size, align);
#endif
  RK_MALLOC_FAIL(res, rk_null, rk_null, align, size);
  return res;
}

#ifndef _MSC_VER
# define RK__aligned_free_f RK__free_f
#else
static_fun rk_forceinline void RK__aligned_free_f(void* ptr) {
  if (ptr != rk_null) { _aligned_free(ptr); }
}
#endif

static_fun rk_forceinline rk_alloc_alignsize(4, 3) void* RK__aligned_realloc_f(void*  ptr,
                                                                               size_t old_size,
                                                                               size_t new_size,
                                                                               size_t align) {
  if (!old_size) { return RK__aligned_alloc_f(new_size, align); }
  if (!new_size) { return RK__aligned_free_f(ptr), rk_null; }
  rk_assert_align_pow2(align);
  align    = rk_max(align, RK_malloc_align);
  new_size = rk_align_up(new_size, align);
#ifndef _MSC_VER
  void* res = aligned_alloc(align, new_size);
  RK_MALLOC_FAIL(res, rk_null, ptr, align, new_size);
  memcpy(res, ptr, rk_min(new_size, old_size));
  free(ptr);
#else
  void* res = _aligned_realloc(ptr, new_size, align);
  RK_MALLOC_FAIL(res, rk_null, ptr, align, new_size);
  (void)old_size;
#endif
  return res;
}

static_fun rk_malloc_fun rk_alloc_alignsize(2, 1) void* RK__malloc_allocate(size_t    size,
                                                                            size_t    align,
                                                                            void* ctx rk_unused) {
  return align <= RK_malloc_align ? RK__malloc_f(size) : RK__aligned_alloc_f(size, align);
}

static_fun rk_alloc_alignsize(4, 3) void* RK__malloc_reallocate(void* ptr, size_t old_size,
                                                                size_t new_size, size_t align,
                                                                void* ctx rk_unused) {
  return align <= RK_malloc_align ? RK__realloc_f(ptr, new_size)
                                  : RK__aligned_realloc_f(ptr, old_size, new_size, align);
}

static_fun void RK__malloc_deallocate(void* ptr, size_t old_size rk_unused, size_t align rk_unused,
                                      void* ctx rk_unused) {
  align <= RK_malloc_align ? RK__free_f(ptr) : RK__aligned_free_f(ptr);
}

// dynamically chose whether malloc or aligned_alloc
#define RK__malloc_ALLOCATE(bytes, align)                                                          \
  (alloc_log_new, RK__malloc_allocate(bytes, align, rk_null))
#define RK__malloc_REALLOCATE(ptr, obytes, nbytes, align)                                          \
  (alloc_log_renew, RK__malloc_reallocate(ptr, obytes, nbytes, align, rk_null))
#define RK__malloc_DEALLOCATE(ptr, align)                                                          \
  (alloc_log_delete, RK__malloc_deallocate(ptr, 0, align, rk_null))

// always call malloc, compiler error if over-aligned
#define RK__malloc_NEW(T, count)                                                                   \
  (alloc_log_new, rk_ensure_malloc_align(T), RK__malloc_f(sizeof_n(T, count)))
#define RK__malloc_RENEW(ptr, count)                                                               \
  (alloc_log_renew, rk_ensure_malloc_align(typeof(*(ptr))),                                        \
   RK__realloc_f(ptr, sizeof_n(*(ptr), count)))
#define RK__malloc_DELETE(ptr)                                                                     \
  (alloc_log_delete, rk_ensure_malloc_align(typeof(*(ptr))), RK__free_f(ptr))

// always call aligned_alloc, check if alignment is enough for type
#define RK__malloc_ALIGNED_NEW(T, count, align)                                                    \
  (alloc_log_new, rk_assert_valid_align(T, align), RK__aligned_alloc_f(sizeof_n(T, count), align))
#define RK__malloc_ALIGNED_RENEW(ptr, old_count, new_count, align)                                 \
  (alloc_log_renew, rk_assert_valid_align(typeof(*(ptr)), align),                                  \
   RK__aligned_realloc_f(ptr, sizeof_n(*(ptr), old_count), sizeof_n(*(ptr), new_count), align))
#define RK__malloc_ALIGNED_DELETE(ptr) (alloc_log_delete, RK__aligned_free_f(ptr))

///////////////////////////////////  Alloc Wrappers ////////////////////////////////////////////////

#if RK_CUSTOM_ALLOCATORS
static_fun rk_forceinline rk_alloc_alignsize(2, 1) void* RK__call_alloc(size_t nbytes, size_t align,
                                                                        Allocator alloc) {
  rk_assert(alloc.vtab && "Invalid Allocator");
  if (!nbytes) { return rk_null; }
  return alloc.vtab->alloc_f(nbytes, align, alloc.ctx);
}
static_fun rk_forceinline rk_alloc_alignsize(4, 3) void* RK__call_realloc(void* ptr, size_t obytes,
                                                                          size_t    nbytes,
                                                                          size_t    align,
                                                                          Allocator alloc) {
  rk_assert(alloc.vtab && "Invalid Allocator");
  if (!nbytes) {
    if (ptr) {
      rk_assert(obytes && "Non-NULL allocation has zero size");
      alloc.vtab->dealloc_f(ptr, obytes, align, alloc.ctx);
    } else {
      rk_assert(!obytes && "NULL allocation has nonzero size");
    }
    return rk_null;
  }
  if (!ptr) {
    rk_assert(!obytes && "NULL allocation has nonzero size");
    return alloc.vtab->alloc_f(nbytes, align, alloc.ctx);
  }
  rk_assert(obytes && "Non-NULL allocation has zero size");
  return alloc.vtab->realloc_f(ptr, obytes, nbytes, align, alloc.ctx);
}

static_fun rk_forceinline void RK__call_dealloc(void* ptr, size_t obytes, size_t align,
                                                Allocator alloc) {
  rk_assert(alloc.vtab && "Invalid Allocator");
  if (!ptr) {
    rk_assert(!obytes && "NULL allocation has nonzero size");
    return;
  }
  rk_assert(obytes && "Non-NULL allocation has zero size");
  alloc.vtab->dealloc_f(ptr, obytes, align, alloc.ctx);
}
#endif

#if RK_CUSTOM_ALLOCATORS
# define RK__alloc_ALLOCATE(bytes, align, all) (alloc_log_new, RK__call_alloc(bytes, align, all))
# define RK__alloc_REALLOCATE(ptr, obytes, nbytes, align, all)                                     \
   (alloc_log_renew, RK__call_realloc(ptr, obytes, nbytes, align, all))
# define RK__alloc_DEALLOCATE(ptr, obytes, align, all)                                             \
   (alloc_log_delete, RK__call_dealloc(ptr, obytes, align, all))
#else
# define RK__alloc_ALLOCATE(bytes, align, all) RK__malloc_ALLOCATE(bytes, align)
# define RK__alloc_REALLOCATE(ptr, obytes, nbytes, align, all)                                     \
   RK__malloc_REALLOCATE(ptr, obytes, nbytes, align)
# define RK__alloc_DEALLOCATE(ptr, obytes, align, all)                                             \
   ((void)(obytes), RK__malloc_DEALLOCATE(ptr, align))

#endif

#define RK__alloc_NEW(T, count, all) RK__alloc_ALLOCATE(sizeof_n(T, count), alignof(T), all)
#define RK__alloc_ALIGNED_NEW(T, count, align, all)                                                \
  (rk_assert_valid_align(T, align), RK__alloc_ALLOCATE(sizeof_n(T, count), align, all))

#define RK__alloc_RENEW(ptr, ocount, ncount, all)                                                  \
  RK__alloc_REALLOCATE(ptr, sizeof_n(*(ptr), ocount), sizeof_n(*(ptr), ncount),                    \
                       alignof(typeof(*(ptr))), all)
#define RK__alloc_ALIGNED_RENEW(ptr, ocount, ncount, align, all)                                   \
  (rk_assert_valid_align(typeof(*(ptr)), align),                                                   \
   RK__alloc_REALLOCATE(ptr, sizeof_n(*(ptr), ocount), sizeof_n(*(ptr), ncount), align, all))

#define RK__alloc_DELETE(ptr, ocount, all)                                                         \
  RK__alloc_DEALLOCATE(ptr, sizeof_n(*(ptr), ocount), alignof(typeof(*(ptr))), all)
#define RK__alloc_ALIGNED_DELETE(ptr, ocount, align, all)                                          \
  (rk_assert_valid_align(typeof(*(ptr)), align),                                                   \
   RK__alloc_DEALLOCATE(ptr, sizeof_n(*(ptr), ocount), align, all))

// macros with allocator parameter; disabled if no local allocators enabled
#define RK__alloc_ALLOCATE3(bytes, align, all) rk_disable_if(RK__alloc_ALLOCATE(bytes, align, all))
#define RK__alloc_REALLOCATE5(ptr, obytes, nbytes, align, all)                                     \
  rk_disable_if(RK__alloc_REALLOCATE(ptr, obytes, nbytes, align, all))
#define RK__alloc_DEALLOCATE4(ptr, obytes, align, all)                                             \
  rk_disable_if(RK__alloc_DEALLOCATE(ptr, obytes, align, all))

#define RK__alloc_NEW3(T, count, all) rk_disable_if(RK__alloc_NEW(T, count, all))
#define RK__alloc_ALIGNED_NEW4(T, count, align, all)                                               \
  rk_disable_if(RK__alloc_ALIGNED_NEW(T, count, align, all))

#define RK__alloc_RENEW4(ptr, ocount, ncount, all)                                                 \
  rk_disable_if(RK__alloc_RENEW(ptr, ocount, ncount, all))
#define RK__alloc_ALIGNED_RENEW5(ptr, ocount, ncount, align, all)                                  \
  rk_disable_if(RK__alloc_ALIGNED_RENEW(ptr, ocount, ncount, align, all))

#define RK__alloc_DELETE3(ptr, ocount, all) rk_disable_if(RK__alloc_DELETE(ptr, ocount, all))
#define RK__alloc_ALIGNED_DELETE4(ptr, ocount, align, all)                                         \
  rk_disable_if(RK__alloc_ALIGNED_DELETE(ptr, ocount, align, all))
// get_alloc_ctx
//  macros with fewer parameters (might default to alloc_ctx)
#define RK__alloc_ALLOCATE2(bytes, align)       RK__alloc_ALLOCATE(bytes, align, alloc_ctx)
#define RK__alloc_ALIGNED_NEW3(T, count, align) RK__alloc_ALIGNED_NEW(T, count, align, alloc_ctx)
#define RK__alloc_NEW2(T, count)                RK__alloc_NEW(T, count, alloc_ctx)

#define RK__alloc_REALLOCATE4(ptr, obytes, nbytes, align)                                          \
  RK__alloc_REALLOCATE(ptr, obytes, nbytes, align, alloc_ctx)
#define RK__alloc_ALIGNED_RENEW4(ptr, ocount, ncount, align)                                       \
  RK__alloc_ALIGNED_RENEW(ptr, ocount, ncount, align, alloc_ctx)
#define RK__alloc_RENEW3(ptr, ocount, ncount) RK__alloc_RENEW(ptr, ocount, ncount, alloc_ctx)

#define RK__alloc_DEALLOCATE3(ptr, obytes, align)                                                  \
  RK__alloc_DEALLOCATE(ptr, obytes, align, alloc_ctx)
#define RK__alloc_ALIGNED_DELETE3(ptr, ocount, align)                                              \
  RK__alloc_ALIGNED_DELETE(ptr, ocount, align, alloc_ctx)
#define RK__alloc_DELETE2(ptr, ocount) RK__alloc_DELETE(ptr, ocount, alloc_ctx)

static_fun rk_alloc_alignsize(3, 2) void* RK__arrdup_f(const void* src, size_t size,
                                                       size_t align RK_IFALLOC(, Allocator alloc)) {
  return rk_memcpy(alloc_allocate(size, align RK_IFALLOC(, alloc)), src, size);
}
#define RK__ARRDUP(src, count, alloc)                                                              \
  RK__arrdup_f(src, sizeof_n(typeof(*(src)), count), alignof(typeof(*(src))) RK_IFALLOC(, alloc))
#define RK__ARRDUP3(src, count, alloc) rk_disable_if(RK__ARRDUP(src, count, alloc))
#define RK__ARRDUP2(src, count)        RK__ARRDUP(src, count, alloc_ctx)

#undef RK__ALLOCCTX_STORAGE
#undef RK__ALLOCCTX_INIT
/// @endcond

RK_HEADER_END

/// @}
#endif // RK_ALLOC_H

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
/* END INLINE: include/rk_alloc.h */
RK_HEADER_BEGIN

/// @brief Type-erased node header shared by every tree type's concrete node (`RK__BstNode`,
/// `RK__AvlNode`, `RK__RbtNode` all start with the same `l`/`r` layout). This is the type a caller
/// declares a traversal stack buffer as, e.g. `tree_node* stack[64];` for `bst_foreach()`.
/// @note Deliberately just the two pointers, with no `alignas_max`-forced over-alignment: the
/// shared primitives below only ever touch `l`/`r` through this type (entry data is reached via an
/// explicit byte offset into the real, per-(K,V) node -- see `RK__tree_min_off`/`RK__tree_max_off`
/// -- never via a member of `tree_node` itself). A concrete node's actual allocation is only ever
/// guaranteed to meet *its own* alignment (e.g. `alignof(RK__AvlNode(K, V))`, which can be less
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
  RK__tree_release(sizeof(*(self)->root), alignof(typeof(*(self)->root)), &(self)->_tree)

/// @brief `size_t tree_count(self)` - Returns the number of key-value pairs stored. Identical
/// across `Bst`/`Avl`/`Rbt` (also reachable as `bst_count`/`avl_count`/`rbt_count`); lookup and
/// mutation are the only operations that differ by rebalancing strategy and therefore stay
/// variant-prefixed.
#define tree_count(self) ((size_t)(self)->count)

#if RK_CUSTOM_ALLOCATORS
# define tree_allocator(self) rk_to_rvalue((self)->alloc)
#else
# define tree_allocator(self) ((void)(self), alloc_ctx)
#endif

/// @brief `bool tree_is_empty(self)` - Returns `true` iff the tree contains no elements.
#define tree_is_empty(self) (tree_count(self) == 0)

/// @brief Returns a pointer to the entry with the smallest key, or `NULL` if the tree is empty.
/// Works identically for `Bst`/`Avl`/`Rbt` (also reachable as `bst_min`/`avl_min`/`rbt_min`): the
/// real entry offset is computed via `offsetof` rather than assumed, so it doesn't matter that
/// `Avl`/`Rbt` nodes carry extra bookkeeping (height/color) that `Bst` nodes don't.
#define tree_min(self)                                                                             \
  ((typeof((self)->root->entry)*)RK__tree_min_off((self)->_tree.root,                              \
                                                  offsetof(typeof(*(self)->root), entry)))

/// @brief Like `tree_min()`, but for the largest key.
#define tree_max(self)                                                                             \
  ((typeof((self)->root->entry)*)RK__tree_max_off((self)->_tree.root,                              \
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
#define BST_DEFINE(K, V, CMP_FUN)       RK__BST_DEFINE(K, V, CMP_FUN)

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
#define bst_get(K, V, self, key)        RK__BST_PUB(K, V, get)(self, key)

/// @brief `bool bst_contains(K, V, Bst(K, V)* self, K key)` - Returns `true` iff the BST contains
/// an entry with the given key.
#define bst_contains(K, V, self, key)   RK__BST_PUB(K, V, contains)(self, key)

/// @brief `bool bst_set(K, V, Bst(K, V)* self, K key, V value)` - Inserts or updates a key-value
/// pair. If `key` is already present, its value is overwritten. If not, a new node is allocated and
/// inserted.
/// @return `true` if a new node was inserted, `false` if an existing value was updated
#define bst_set(K, V, self, key, value) RK__BST_PUB(K, V, set)(self, key, value)

/// @brief `V* bst_add(K, V, Bst(K, V)* self, K key, V value)` - Inserts a key-value pair only if
/// `key` is not already present. Existing values are not overwritten.
/// @return Pointer to the added object, if added, or `NULL`, if not
#define bst_add(K, V, self, key, value) RK__BST_PUB(K, V, add)(self, key, value)

/// @brief `V* bst_get_or_add(K, V, Bst(K, V)* self, K key, V default_value, bool* inserted_out)` -
/// Returns a pointer to the value for `key`, inserting `default_value` first if the key is absent.
/// Performs a single tree traversal, unlike a separate `bst_get()`/`bst_add()` pair.
/// @param default_value Value to insert if `key` is not present.
/// @param inserted_out Set to `true` if a new node was inserted, `false` if the key already
/// existed. Must not be `NULL`.
/// @return Pointer to the value for `key` (never `NULL`).
#define bst_get_or_add(K, V, self, key, default_value, inserted_out)                               \
  RK__BST_PUB(K, V, get_or_add)(self, key, default_value, inserted_out)

/// @brief `bool bst_extract(K, V, Bst(K, V)* self, K key, V* out)` - Removes the entry
/// with `key` from the BST and writes its value to `out`.
/// @param out Non-null pointer; where the removed value is written, if found
/// @return `true` if the key was found and removed, `false` otherwise
#define bst_extract(K, V, self, key, out) RK__BST_PUB(K, V, extract)(self, key, out)

/// @brief `bool bst_remove(K, V, Bst(K, V)* self, K key)` - Removes the entry with `key` from the
/// BST, discarding its value.
/// @return `true` if the key was found and removed, `false` otherwise
#define bst_remove(K, V, self, key)       RK__BST_PUB(K, V, remove)(self, key)

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
#define AVL_DEFINE(K, V, CMP_FUN)       RK__AVL_DEFINE(K, V, CMP_FUN)

/// @brief Generates a type-specific Avl struct name.
#define Avl(K, V)                       avl_##K##_##V
/// @brief Generates a type-specific Avl entry struct name.
#define AvlEntry(K, V)                  avl_entry_##K##_##V

/// @brief `Avl(K, V) avl_init(K, V, Allocator alloc = alloc_ctx)` - Initialises and returns an
/// empty Avl tree.
/// @param K     Key type name
/// @param V     Value type name
/// @param alloc Optional allocator; defaults to `alloc_ctx`
/// @return An initialised, empty `Avl(K, V)`
#define avl_init(K, V, ...)             rk_overload(RK__avl_init, K, V, ##__VA_ARGS__)

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
#define avl_get(K, V, self, key)        RK__AVL_PUB(K, V, get)(self, key)

/// @brief `bool avl_contains(K, V, Avl(K, V)* self, K key)` - See `bst_contains()`.
#define avl_contains(K, V, self, key)   (!!avl_get(K, V, self, key))

/// @brief `bool avl_set(K, V, Avl(K, V)* self, K key, V value)` - Inserts or updates a key-value
/// pair, rebalancing as needed.
/// @return `true` if a new node was inserted, `false` if an existing value was updated
#define avl_set(K, V, self, key, value) RK__AVL_PUB(K, V, set)(self, key, value)

/// @brief `V* avl_add(K, V, Avl(K, V)* self, K key, V value)` - See `bst_add()`; rebalances as
/// needed.
/// @return Pointer to the added value, if added, or `NULL`, if not
#define avl_add(K, V, self, key, value) RK__AVL_PUB(K, V, add)(self, key, value)

/// @brief `V* avl_get_or_add(K, V, Avl(K, V)* self, K key, V default_value, bool* inserted_out)` -
/// See `bst_get_or_add()`; rebalances as needed.
/// @return Pointer to the value for `key` (never `NULL`).
#define avl_get_or_add(K, V, self, key, default_value, inserted_out)                               \
  RK__AVL_PUB(K, V, get_or_add)(self, key, default_value, inserted_out)

/// @brief `bool avl_extract(K, V, Avl(K, V)* self, K key, V* out)` - See `bst_extract()`;
/// rebalances as needed.
/// @return `true` if the key was found and removed, `false` otherwise
#define avl_extract(K, V, self, key, out) RK__AVL_PUB(K, V, extract)(self, key, out)

/// @brief `bool avl_remove(K, V, Avl(K, V)* self, K key)` - See `bst_remove()`; rebalances as
/// needed.
/// @return `true` if the key was found and removed, `false` otherwise
#define avl_remove(K, V, self, key)       RK__AVL_PUB(K, V, remove)(self, key)

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
#define RBT_DEFINE(K, V, CMP_FUN)       RK__RBT_DEFINE(K, V, CMP_FUN)

/// @brief Generates a type-specific Rbt struct name.
#define Rbt(K, V)                       rbt_##K##_##V
/// @brief Generates a type-specific Rbt entry struct name.
#define RbtEntry(K, V)                  rbt_entry_##K##_##V

/// @brief `Rbt(K, V) rbt_init(K, V, Allocator alloc = alloc_ctx)` - Initialises and returns an
/// empty Rbt tree.
#define rbt_init(K, V, ...)             rk_overload(RK__rbt_init, K, V, ##__VA_ARGS__)

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
#define rbt_get(K, V, self, key)        RK__RBT_F(K, V, get)(self, key)

/// @brief `bool rbt_contains(K, V, Rbt(K, V)* self, K key)` - See `bst_contains()`.
#define rbt_contains(K, V, self, key)   (!!rbt_get(K, V, self, key))

/// @brief `bool rbt_set(K, V, Rbt(K, V)* self, K key, V value)` - Inserts or updates a key-value
/// pair, rebalancing as needed.
/// @return `true` if a new node was inserted, `false` if an existing value was updated
#define rbt_set(K, V, self, key, value) RK__RBT_F(K, V, set)(self, key, value)

/// @brief `V* rbt_add(K, V, Rbt(K, V)* self, K key, V value)` - See `bst_add()`; rebalances as
/// needed.
/// @return Pointer to the added value, if added, or `NULL`, if not
#define rbt_add(K, V, self, key, value) RK__RBT_F(K, V, add)(self, key, value)

/// @brief `V* rbt_get_or_add(K, V, Rbt(K, V)* self, K key, V default_value, bool* inserted_out)` -
/// See `bst_get_or_add()`; rebalances as needed.
/// @return Pointer to the value for `key` (never `NULL`).
#define rbt_get_or_add(K, V, self, key, default_value, inserted_out)                               \
  RK__RBT_F(K, V, get_or_add)(self, key, default_value, inserted_out)

/// @brief `bool rbt_extract(K, V, Rbt(K, V)* self, K key, V* out)` - See `bst_extract()`;
/// rebalances as needed.
/// @return `true` if the key was found and removed, `false` otherwise
#define rbt_extract(K, V, self, key, out) RK__RBT_F(K, V, extract)(self, key, out)

/// @brief `bool rbt_remove(K, V, Rbt(K, V)* self, K key)` - See `bst_remove()`; rebalances as
/// needed.
/// @return `true` if the key was found and removed, `false` otherwise
#define rbt_remove(K, V, self, key)       RK__RBT_F(K, V, remove)(self, key)

/// @brief Iterates over all entries in the Rbt tree in ascending key order. Same parameters and
/// contract as `bst_foreach()`.
#define rbt_foreach(self, stack_buf, stack_cap, entry)                                             \
  tree_foreach(self, stack_buf, stack_cap, entry)

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

////////////////////////////// Shared node/iterator primitives (`tree_*`)
///////////////////////////////

/// @brief Type-erased view of any of this file's concrete node types (`RK__BstNode`,
/// `RK__AvlNode`, `RK__RbtNode`), used by the shared release/iteration primitives below, which only
/// ever need to walk `l`/`r` -- never touch `data`/`entry` directly (that's only ever done after
/// re-casting to the real, per-(K,V) node type, since the entry's offset differs by node type: it
/// sits right after `l`/`r` for `Bst`, but after an extra height/color bookkeeping field for
/// `Avl`/`Rbt`). Where the entry itself must be reached generically (`_min`/`_max`), the real
/// offset is passed explicitly rather than assumed -- see `RK__tree_min_off`/`RK__tree_max_off`.
static_fun void RK__tree_release_nodes(size_t nodesize, size_t nodealign,
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

static_fun void RK__tree_release(size_t nodesize, size_t nodealign, tree_data* self) {
  RK__tree_release_nodes(nodesize, nodealign, self->root RK_IFALLOC(, self->alloc));
  self->root = rk_null, self->count = 0;
}

/// @brief Returns a pointer to the leftmost (minimum) node's entry, `entry_off` bytes into the
/// node. Passing the real offset (rather than assuming entry data sits right after `l`/`r`, as a
/// bare `tree_node*` would) is what makes this safe to reuse for node types that carry extra
/// bookkeeping (e.g. an AVL height or a red-black color bit) between the pointers and the entry.
static_fun void* RK__tree_min_off(tree_node* node, size_t entry_off) {
  if (!node) { return rk_null; }
  while (node->l) { node = node->l; }
  return (char*)node + entry_off;
}

/// @brief Like `RK__tree_min_off()`, but for the rightmost (maximum) node.
static_fun void* RK__tree_max_off(tree_node* node, size_t entry_off) {
  if (!node) { return rk_null; }
  while (node->r) { node = node->r; }
  return (char*)node + entry_off;
}

static_fun bool RK__tree_iter_next(tree_iter* restrict it, tree_node** node_out) {
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
  for (typeof(*(self))*const RK__rs = (self), *RK__once = RK__rs; RK__once;)                       \
    for (tree_node * RK__node; RK__once; RK__once = 0)                                             \
      for (tree_iter RK__it = {.stack = (stack_buf),                                               \
                               .curr  = (tree_node*)RK__rs->root,                                  \
                               .cap   = (stack_cap),                                               \
                               .top   = 0};                                                        \
           RK__tree_iter_next(&RK__it, &RK__node);)                                                \
        for (typeof(RK__rs->root->entry)*const entry_ = &((typeof(RK__rs->root))RK__node)->entry,  \
                                               *RK__once1 = entry_;                                \
             RK__once1; RK__once1                         = 0)

//////////////////////////////////////////// Bst internals
//////////////////////////////////////////////

#define RK__BstEntryPriv(K, V)      RK__bst_entry_##K##_##V
#define RK__BST_PUB(K, V, FNAME)    bstf_##FNAME##_##K##_##V
#define RK__BST_PRI(K, V, FNAME)    RK__bst_##FNAME##_##K##_##V
#define RK__BstNode(K, V)           RK__bst_node_##K##_##V

#define RK__bst_init(K, V, _Alloc)  ((Bst(K, V)){.count = 0, RK_IFALLOC(.alloc = _Alloc)})
#define RK__bst_init3(K, V, _Alloc) rk_disable_if(RK__bst_init(K, V, _Alloc))
#define RK__bst_init2(K, V)         RK__bst_init(K, V, alloc_ctx)

#define RK__BST_DEFINE(K, V, CMP_FUN)                                                              \
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
      RK__BstEntryPriv(K, V) entry_mod;                                                            \
      BstEntry(K, V) entry;                                                                        \
    };                                                                                             \
  };                                                                                               \
  typedef struct Bst(K, V) {                                                                       \
    union {                                                                                        \
      tree_data _tree;                                                                             \
      struct {                                                                                     \
        RK_IFALLOC(Allocator alloc;)                                                               \
        size_t count;                                                                              \
        struct RK__BstNode(K, V) * root;                                                           \
      };                                                                                           \
    };                                                                                             \
  } Bst(K, V);                                                                                     \
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
    return !!(*RK__BST_PUB(K, V, search_ptr)(self, key));                                          \
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
    return !!RK__BST_PRI(K, V, set_add)(true, self, key, val);                                     \
  }                                                                                                \
  static_fun V* RK__BST_PUB(K, V, add)(Bst(K, V) * self, K key, V val) {                           \
    return RK__BST_PRI(K, V, set_add)(false, self, key, val);                                      \
  }                                                                                                \
  static_fun V* RK__BST_PUB(K, V, get_or_add)(Bst(K, V) * self, K key, V val,                      \
                                              bool* restrict inserted_out) {                       \
    rk_assert_ptr_nonnull(inserted_out);                                                           \
    typedef struct RK__BstNode(K, V) node_t;                                                       \
    node_t** lnk = RK__BST_PUB(K, V, search_ptr)(self, key);                                       \
    if (*lnk) {                                                                                    \
      *inserted_out = false;                                                                       \
      return &((*lnk)->entry_mod.val);                                                             \
    }                                                                                              \
    rk_set_alloc_fallback(self->alloc);                                                            \
    node_t* n = alloc_new(node_t, 1 RK_IFALLOC(, self->alloc));                                    \
    n->r = n->l  = rk_null;                                                                        \
    n->entry_mod = (typeof(n->entry_mod)){.key = key, .val = val};                                 \
    *lnk         = n;                                                                              \
    ++self->count;                                                                                 \
    *inserted_out = true;                                                                          \
    return &(n->entry_mod.val);                                                                    \
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

//////////////////////////////////////////// Avl internal /////////////////////////////////////////

#define RK__AvlNode(K, V)      RK__avl_node_##K##_##V
#define RK__AVL_PUB(K, V, F)   avlf_##F##_##K##_##V
#define RK__AVL_PRI(K, V, F)   RK__avl_##F##_##K##_##V

#define RK__avl_init(K, V, A)  ((Avl(K, V)){.count = 0, .root = rk_null, RK_IFALLOC(.alloc = A)})
#define RK__avl_init3(K, V, A) rk_disable_if(RK__avl_init(K, V, A))
#define RK__avl_init2(K, V)    RK__avl_init(K, V, alloc_ctx)

#define RK__AVL_DEFINE(K, V, CMP_FUN)                                                              \
  RK_EXTERNC_BEG                                                                                   \
  typedef struct AvlEntry(K, V) {                                                                  \
    K const key;                                                                                   \
    V       val;                                                                                   \
  } AvlEntry(K, V);                                                                                \
  typedef struct RK__AVL_PRI(K, V, entry) {                                                        \
    K key;                                                                                         \
    V val;                                                                                         \
  } RK__AVL_PRI(K, V, entry);                                                                      \
  typedef struct RK__AvlNode(K, V) {                                                               \
    struct RK__AvlNode(K, V) * l, *r;                                                              \
    int height;                                                                                    \
    union {                                                                                        \
      RK__AVL_PRI(K, V, entry) entry_mod;                                                          \
      AvlEntry(K, V) entry;                                                                        \
    };                                                                                             \
  } RK__AvlNode(K, V);                                                                             \
  typedef struct Avl(K, V) {                                                                       \
    union {                                                                                        \
      tree_data _tree;                                                                             \
      struct {                                                                                     \
        RK_IFALLOC(Allocator alloc;)                                                               \
        size_t count;                                                                              \
        RK__AvlNode(K, V) * root;                                                                  \
      };                                                                                           \
    };                                                                                             \
  } Avl(K, V);                                                                                     \
  static_fun int  RK__AVL_PRI(K, V, h)(RK__AvlNode(K, V) * n) { return n ? n->height : 0; }        \
  static_fun void RK__AVL_PRI(K, V, fixh)(RK__AvlNode(K, V) * n) {                                 \
    int a = RK__AVL_PRI(K, V, h)(n->l), b = RK__AVL_PRI(K, V, h)(n->r);                            \
    n->height = 1 + (a > b ? a : b);                                                               \
  }                                                                                                \
  static_fun RK__AvlNode(K, V) * RK__AVL_PRI(K, V, rotl)(RK__AvlNode(K, V) * x) {                  \
    RK__AvlNode(K, V)* y = x->r;                                                                   \
    x->r                 = y->l;                                                                   \
    y->l                 = x;                                                                      \
    RK__AVL_PRI(K, V, fixh)(x);                                                                    \
    RK__AVL_PRI(K, V, fixh)(y);                                                                    \
    return y;                                                                                      \
  }                                                                                                \
  static_fun RK__AvlNode(K, V) * RK__AVL_PRI(K, V, rotr)(RK__AvlNode(K, V) * y) {                  \
    RK__AvlNode(K, V)* x = y->l;                                                                   \
    y->l                 = x->r;                                                                   \
    x->r                 = y;                                                                      \
    RK__AVL_PRI(K, V, fixh)(y);                                                                    \
    RK__AVL_PRI(K, V, fixh)(x);                                                                    \
    return x;                                                                                      \
  }                                                                                                \
  static_fun RK__AvlNode(K, V) * RK__AVL_PRI(K, V, balance)(RK__AvlNode(K, V) * n) {               \
    RK__AVL_PRI(K, V, fixh)(n);                                                                    \
    int bf = RK__AVL_PRI(K, V, h)(n->l) - RK__AVL_PRI(K, V, h)(n->r);                              \
    if (bf > 1) {                                                                                  \
      if (RK__AVL_PRI(K, V, h)(n->l->l) < RK__AVL_PRI(K, V, h)(n->l->r)) {                         \
        n->l = RK__AVL_PRI(K, V, rotl)(n->l);                                                      \
      }                                                                                            \
      return RK__AVL_PRI(K, V, rotr)(n);                                                           \
    }                                                                                              \
    if (bf < -1) {                                                                                 \
      if (RK__AVL_PRI(K, V, h)(n->r->r) < RK__AVL_PRI(K, V, h)(n->r->l)) {                         \
        n->r = RK__AVL_PRI(K, V, rotr)(n->r);                                                      \
      }                                                                                            \
      return RK__AVL_PRI(K, V, rotl)(n);                                                           \
    }                                                                                              \
    return n;                                                                                      \
  }                                                                                                \
  static_fun RK__AvlNode(K, V)                                                                     \
      * RK__AVL_PRI(K, V, put)(Avl(K, V) * self, RK__AvlNode(K, V) * n, K key, V val,              \
                               bool overwrite, V** out, bool* added) {                             \
    if (!n) {                                                                                      \
      rk_set_alloc_fallback(self->alloc);                                                          \
      n    = alloc_new(RK__AvlNode(K, V), 1 RK_IFALLOC(, self->alloc));                            \
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
      n->l = RK__AVL_PRI(K, V, put)(self, n->l, key, val, overwrite, out, added);                  \
    } else {                                                                                       \
      n->r = RK__AVL_PRI(K, V, put)(self, n->r, key, val, overwrite, out, added);                  \
    }                                                                                              \
    return RK__AVL_PRI(K, V, balance)(n);                                                          \
  }                                                                                                \
  static_fun V* RK__AVL_PUB(K, V, get)(Avl(K, V) * self, K key) {                                  \
    RK__AvlNode(K, V)* n = self->root;                                                             \
    while (n) {                                                                                    \
      int c = CMP_FUN(key, n->entry.key);                                                          \
      if (!c) { return &n->entry_mod.val; }                                                        \
      n = c < 0 ? n->l : n->r;                                                                     \
    }                                                                                              \
    return rk_null;                                                                                \
  }                                                                                                \
  static_fun V* RK__AVL_PRI(K, V, setadd)(Avl(K, V) * self, K key, V val, bool overwrite,          \
                                          bool* added) {                                           \
    V* out     = rk_null;                                                                          \
    *added     = false;                                                                            \
    self->root = RK__AVL_PRI(K, V, put)(self, self->root, key, val, overwrite, &out, added);       \
    if (*added) { ++self->count; }                                                                 \
    return out;                                                                                    \
  }                                                                                                \
  static_fun bool RK__AVL_PUB(K, V, set)(Avl(K, V) * self, K key, V val) {                         \
    bool added;                                                                                    \
    (void)RK__AVL_PRI(K, V, setadd)(self, key, val, true, &added);                                 \
    return added;                                                                                  \
  }                                                                                                \
  static_fun V* RK__AVL_PUB(K, V, add)(Avl(K, V) * self, K key, V val) {                           \
    bool added;                                                                                    \
    V*   p = RK__AVL_PRI(K, V, setadd)(self, key, val, false, &added);                             \
    return added ? p : rk_null;                                                                    \
  }                                                                                                \
  static_fun V* RK__AVL_PUB(K, V, get_or_add)(Avl(K, V) * self, K key, V val,                      \
                                              bool* restrict inserted_out) {                       \
    rk_assert_ptr_nonnull(inserted_out);                                                           \
    return RK__AVL_PRI(K, V, setadd)(self, key, val, false, inserted_out);                         \
  }                                                                                                \
  static_fun RK__AvlNode(K, V)                                                                     \
      * RK__AVL_PRI(K, V, detach_min)(RK__AvlNode(K, V) * n, RK__AvlNode(K, V) * *out) {           \
    if (!n->l) {                                                                                   \
      *out = n;                                                                                    \
      return n->r;                                                                                 \
    }                                                                                              \
    n->l = RK__AVL_PRI(K, V, detach_min)(n->l, out);                                               \
    return RK__AVL_PRI(K, V, balance)(n);                                                          \
  }                                                                                                \
  static_fun RK__AvlNode(K, V)                                                                     \
      * RK__AVL_PRI(K, V, erase)(Avl(K, V) * self, RK__AvlNode(K, V) * n, K key, V * out,          \
                                 bool* removed) {                                                  \
    if (!n) return rk_null;                                                                        \
    int c = CMP_FUN(key, n->entry.key);                                                            \
    if (c < 0) {                                                                                   \
      n->l = RK__AVL_PRI(K, V, erase)(self, n->l, key, out, removed);                              \
    } else if (c > 0) {                                                                            \
      n->r = RK__AVL_PRI(K, V, erase)(self, n->r, key, out, removed);                              \
    } else {                                                                                       \
      *out                 = n->entry_mod.val;                                                     \
      *removed             = true;                                                                 \
      RK__AvlNode(K, V)* l = n->l, *r = n->r;                                                      \
      if (!r) {                                                                                    \
        alloc_delete(n, 1 RK_IFALLOC(, self->alloc));                                              \
        return l;                                                                                  \
      }                                                                                            \
      RK__AvlNode(K, V) * m;                                                                       \
      r    = RK__AVL_PRI(K, V, detach_min)(r, &m);                                                 \
      m->l = l;                                                                                    \
      m->r = r;                                                                                    \
      alloc_delete(n, 1 RK_IFALLOC(, self->alloc));                                                \
      return RK__AVL_PRI(K, V, balance)(m);                                                        \
    }                                                                                              \
    return *removed ? RK__AVL_PRI(K, V, balance)(n) : n;                                           \
  }                                                                                                \
  static_fun bool RK__AVL_PUB(K, V, extract)(Avl(K, V) * self, K key, V * out) {                   \
    rk_assert_ptr_nonnull(out);                                                                    \
    bool removed = false;                                                                          \
    self->root   = RK__AVL_PRI(K, V, erase)(self, self->root, key, out, &removed);                 \
    if (removed) { --self->count; }                                                                \
    return removed;                                                                                \
  }                                                                                                \
  static_fun bool RK__AVL_PUB(K, V, remove)(Avl(K, V) * self, K key) {                             \
    V tmp;                                                                                         \
    return RK__AVL_PUB(K, V, extract)(self, key, &tmp);                                            \
  }                                                                                                \
  RK_EXTERNC_END

//////////////////////////////////////////// Rbt internals /////////////////////////////////////////

#define RK__RbtNode(K, V)      RK__rbt_node_##K##_##V
#define RK__RBT_F(K, V, F)     rbtf_##F##_##K##_##V
#define RK__RBT_I(K, V, F)     RK__rbt_##F##_##K##_##V

#define RK__rbt_init(K, V, A)  ((Rbt(K, V)){.count = 0, .root = rk_null, RK_IFALLOC(.alloc = A)})
#define RK__rbt_init3(K, V, A) rk_disable_if(RK__rbt_init(K, V, A))
#define RK__rbt_init2(K, V)    RK__rbt_init(K, V, alloc_ctx)

/* Left-leaning red-black tree: red links lean left and no node has two red links in a row. */

#define RK__RBT_DEFINE(K, V, CMP)                                                                  \
  RK_EXTERNC_BEG                                                                                   \
  typedef struct RbtEntry(K, V) {                                                                  \
    K const key;                                                                                   \
    V       val;                                                                                   \
  } RbtEntry(K, V);                                                                                \
  typedef struct RK__RBT_I(K, V, E) {                                                              \
    K key;                                                                                         \
    V val;                                                                                         \
  } RK__RBT_I(K, V, E);                                                                            \
  typedef struct RK__RbtNode(K, V) {                                                               \
    struct RK__RbtNode(K, V) * l, *r;                                                              \
    bool red;                                                                                      \
    union {                                                                                        \
      RK__RBT_I(K, V, E) entry_mod;                                                                \
      RbtEntry(K, V) entry;                                                                        \
    };                                                                                             \
  } RK__RbtNode(K, V);                                                                             \
  typedef struct Rbt(K, V) {                                                                       \
    union {                                                                                        \
      tree_data _tree;                                                                             \
      struct {                                                                                     \
        RK_IFALLOC(Allocator alloc;)                                                               \
        size_t count;                                                                              \
        RK__RbtNode(K, V) * root;                                                                  \
      };                                                                                           \
    };                                                                                             \
  } Rbt(K, V);                                                                                     \
  static_fun bool RK__RBT_I(K, V, red)(RK__RbtNode(K, V) * n) { return n && n->red; }              \
  static_fun      RK__RbtNode(K, V) * RK__RBT_I(K, V, rl)(RK__RbtNode(K, V) * h) {                 \
    RK__RbtNode(K, V)* x = h->r;                                                                   \
    h->r                 = x->l;                                                                   \
    x->l                 = h;                                                                      \
    x->red               = h->red;                                                                 \
    h->red               = true;                                                                   \
    return x;                                                                                      \
  }                                                                                                \
  static_fun RK__RbtNode(K, V) * RK__RBT_I(K, V, rr)(RK__RbtNode(K, V) * h) {                      \
    RK__RbtNode(K, V)* x = h->l;                                                                   \
    h->l                 = x->r;                                                                   \
    x->r                 = h;                                                                      \
    x->red               = h->red;                                                                 \
    h->red               = true;                                                                   \
    return x;                                                                                      \
  }                                                                                                \
  static_fun void RK__RBT_I(K, V, flip)(RK__RbtNode(K, V) * h) {                                   \
    h->red    = !h->red;                                                                           \
    h->l->red = !h->l->red;                                                                        \
    h->r->red = !h->r->red;                                                                        \
  }                                                                                                \
  static_fun RK__RbtNode(K, V) * RK__RBT_I(K, V, fix)(RK__RbtNode(K, V) * h) {                     \
    if (RK__RBT_I(K, V, red)(h->r)) { h = RK__RBT_I(K, V, rl)(h); }                                \
    if (RK__RBT_I(K, V, red)(h->l) && RK__RBT_I(K, V, red)(h->l->l)) {                             \
      h = RK__RBT_I(K, V, rr)(h);                                                                  \
    }                                                                                              \
    if (RK__RBT_I(K, V, red)(h->l) && RK__RBT_I(K, V, red)(h->r)) { RK__RBT_I(K, V, flip)(h); }    \
    return h;                                                                                      \
  }                                                                                                \
  static_fun RK__RbtNode(K, V) * RK__RBT_I(K, V, ml)(RK__RbtNode(K, V) * h) {                      \
    RK__RBT_I(K, V, flip)(h);                                                                      \
    if (RK__RBT_I(K, V, red)(h->r->l)) {                                                           \
      h->r = RK__RBT_I(K, V, rr)(h->r);                                                            \
      h    = RK__RBT_I(K, V, rl)(h);                                                               \
      RK__RBT_I(K, V, flip)(h);                                                                    \
    }                                                                                              \
    return h;                                                                                      \
  }                                                                                                \
  static_fun RK__RbtNode(K, V) * RK__RBT_I(K, V, mr)(RK__RbtNode(K, V) * h) {                      \
    RK__RBT_I(K, V, flip)(h);                                                                      \
    if (RK__RBT_I(K, V, red)(h->l->l)) {                                                           \
      h = RK__RBT_I(K, V, rr)(h);                                                                  \
      RK__RBT_I(K, V, flip)(h);                                                                    \
    }                                                                                              \
    return h;                                                                                      \
  }                                                                                                \
  static_fun RK__RbtNode(K, V)                                                                     \
      * RK__RBT_I(K, V, put)(Rbt(K, V) * s, RK__RbtNode(K, V) * h, K k, V v, bool ow, V** out,     \
                             bool* added) {                                                        \
    if (!h) {                                                                                      \
      rk_set_alloc_fallback(s->alloc);                                                             \
      h    = alloc_new(RK__RbtNode(K, V), 1 RK_IFALLOC(, s->alloc));                               \
      h->l = h->r  = rk_null;                                                                      \
      h->red       = true;                                                                         \
      h->entry_mod = (typeof(h->entry_mod)){.key = k, .val = v};                                   \
      *out         = &h->entry_mod.val;                                                            \
      *added       = true;                                                                         \
      return h;                                                                                    \
    }                                                                                              \
    int c = CMP(k, h->entry.key);                                                                  \
    if (c < 0) {                                                                                   \
      h->l = RK__RBT_I(K, V, put)(s, h->l, k, v, ow, out, added);                                  \
    } else if (c > 0) {                                                                            \
      h->r = RK__RBT_I(K, V, put)(s, h->r, k, v, ow, out, added);                                  \
    } else {                                                                                       \
      if (ow) { h->entry_mod.val = v; }                                                            \
      *out = &h->entry_mod.val;                                                                    \
    }                                                                                              \
    return RK__RBT_I(K, V, fix)(h);                                                                \
  }                                                                                                \
  static_fun V* RK__RBT_F(K, V, get)(Rbt(K, V) * s, K k) {                                         \
    RK__RbtNode(K, V)* n = s->root;                                                                \
    while (n) {                                                                                    \
      int c = CMP(k, n->entry.key);                                                                \
      if (!c) { return &n->entry_mod.val; }                                                        \
      n = c < 0 ? n->l : n->r;                                                                     \
    }                                                                                              \
    return rk_null;                                                                                \
  }                                                                                                \
  static_fun V* RK__RBT_I(K, V, insert)(Rbt(K, V) * s, K k, V v, bool ow, bool* added) {           \
    V* out       = rk_null;                                                                        \
    *added       = false;                                                                          \
    s->root      = RK__RBT_I(K, V, put)(s, s->root, k, v, ow, &out, added);                        \
    s->root->red = false;                                                                          \
    if (*added) { ++s->count; }                                                                    \
    return out;                                                                                    \
  }                                                                                                \
  static_fun bool RK__RBT_F(K, V, set)(Rbt(K, V) * s, K k, V v) {                                  \
    bool a;                                                                                        \
    (void)RK__RBT_I(K, V, insert)(s, k, v, true, &a);                                              \
    return a;                                                                                      \
  }                                                                                                \
  static_fun V* RK__RBT_F(K, V, add)(Rbt(K, V) * s, K k, V v) {                                    \
    bool a;                                                                                        \
    V*   p = RK__RBT_I(K, V, insert)(s, k, v, false, &a);                                          \
    return a ? p : rk_null;                                                                        \
  }                                                                                                \
  static_fun V* RK__RBT_F(K, V, get_or_add)(Rbt(K, V) * s, K k, V v,                               \
                                            bool* restrict inserted_out) {                         \
    rk_assert_ptr_nonnull(inserted_out);                                                           \
    return RK__RBT_I(K, V, insert)(s, k, v, false, inserted_out);                                  \
  }                                                                                                \
  static_fun RK__RbtNode(K, V) * RK__RBT_I(K, V, mn)(RK__RbtNode(K, V) * h) {                      \
    while (h->l) { h = h->l; }                                                                     \
    return h;                                                                                      \
  }                                                                                                \
  static_fun RK__RbtNode(K, V) * RK__RBT_I(K, V, dm)(Rbt(K, V) * s, RK__RbtNode(K, V) * h) {       \
    if (!h->l) {                                                                                   \
      alloc_delete(h, 1 RK_IFALLOC(, s->alloc));                                                   \
      return rk_null;                                                                              \
    }                                                                                              \
    if (!RK__RBT_I(K, V, red)(h->l) && !RK__RBT_I(K, V, red)(h->l->l)) {                           \
      h = RK__RBT_I(K, V, ml)(h);                                                                  \
    }                                                                                              \
    h->l = RK__RBT_I(K, V, dm)(s, h->l);                                                           \
    return RK__RBT_I(K, V, fix)(h);                                                                \
  }                                                                                                \
  static_fun RK__RbtNode(K, V)                                                                     \
      * RK__RBT_I(K, V, del)(Rbt(K, V) * s, RK__RbtNode(K, V) * h, K k, V * out, bool* gone) {     \
    if (CMP(k, h->entry.key) < 0) {                                                                \
      if (h->l) {                                                                                  \
        if (!RK__RBT_I(K, V, red)(h->l) && !RK__RBT_I(K, V, red)(h->l->l)) {                       \
          h = RK__RBT_I(K, V, ml)(h);                                                              \
        }                                                                                          \
        h->l = RK__RBT_I(K, V, del)(s, h->l, k, out, gone);                                        \
      }                                                                                            \
    } else {                                                                                       \
      if (RK__RBT_I(K, V, red)(h->l)) { h = RK__RBT_I(K, V, rr)(h); }                              \
      int c = CMP(k, h->entry.key);                                                                \
      if (!c && !h->r) {                                                                           \
        *out  = h->entry_mod.val;                                                                  \
        *gone = true;                                                                              \
        alloc_delete(h, 1 RK_IFALLOC(, s->alloc));                                                 \
        return rk_null;                                                                            \
      }                                                                                            \
      if (h->r) {                                                                                  \
        if (!RK__RBT_I(K, V, red)(h->r) && !RK__RBT_I(K, V, red)(h->r->l)) {                       \
          h = RK__RBT_I(K, V, mr)(h);                                                              \
        }                                                                                          \
        c = CMP(k, h->entry.key);                                                                  \
        if (!c) {                                                                                  \
          RK__RbtNode(K, V)* m = RK__RBT_I(K, V, mn)(h->r);                                        \
          *out                 = h->entry_mod.val;                                                 \
          *gone                = true;                                                             \
          h->entry_mod         = m->entry_mod;                                                     \
          h->r                 = RK__RBT_I(K, V, dm)(s, h->r);                                     \
        } else {                                                                                   \
          h->r = RK__RBT_I(K, V, del)(s, h->r, k, out, gone);                                      \
        }                                                                                          \
      }                                                                                            \
    }                                                                                              \
    return RK__RBT_I(K, V, fix)(h);                                                                \
  }                                                                                                \
  static_fun bool RK__RBT_F(K, V, extract)(Rbt(K, V) * s, K k, V * out) {                          \
    rk_assert_ptr_nonnull(out);                                                                    \
    if (!s->root || !RK__RBT_F(K, V, get)(s, k)) { return false; }                                 \
    bool gone = false;                                                                             \
    if (!RK__RBT_I(K, V, red)(s->root->l) && !RK__RBT_I(K, V, red)(s->root->r)) {                  \
      s->root->red = true;                                                                         \
    }                                                                                              \
    s->root = RK__RBT_I(K, V, del)(s, s->root, k, out, &gone);                                     \
    if (s->root) { s->root->red = false; }                                                         \
    if (gone) { --s->count; }                                                                      \
    return gone;                                                                                   \
  }                                                                                                \
  static_fun bool RK__RBT_F(K, V, remove)(Rbt(K, V) * s, K k) {                                    \
    V x;                                                                                           \
    return RK__RBT_F(K, V, extract)(s, k, &x);                                                     \
  }                                                                                                \
  RK_EXTERNC_END

/// @endcond

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
/* END INLINE: include/rk_trees.h */
/* inlined from include/rklib.h:8: #include "rk_bitset.h" */
/* BEGIN INLINE: include/rk_bitset.h */
// SPDX-License-Identifier: MIT
/// @file rk_bitset.h
/// @version 1.0.0
/// @defgroup rk_bitset Bitset Utilities
/// @brief Bitset utilities.
///
/// This header provides a lightweight, bitset representation and helper functions for bit
/// manipulation. Bit numbering is **0-based**: bit index 0 refers to the **least-significant bit
/// (LSB)** of `bs[0]`.
///
/// ## Storage model
///
/// A bitset is stored as an array of `bitset_word` (by default `unsigned long long`):
/// - `bs[0]` holds bits `[0 .. W-1]`
/// - `bs[1]` holds bits `[W .. 2W-1]`
/// - etc., where `W = bitsof(bitset_word)`
///
/// ## Padding bits (important)
///
/// For a logical bitset size `nbits`, the last storage word may contain
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
/// ## Relationship to C23 stdbit.h conventions
///
/// Single-bit and range operations use 0-based bit
/// indices. The query functions `bitset_first_*` follow the C23 `<stdbit.h>` / common builtin
/// convention of returning a **1-based position**, with **0** as the sentinel value meaning “not
/// found”.
///
/// @see stdbit.h
/// @see rk_defs.h
/// @{

#ifndef RK_BITSET_H
#define RK_BITSET_H
/* inlined from include/rk_bitset.h:45: #include "rk_defs.h" */
/* skipped already-included: "include/rk_defs.h" */
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
  return (bitset)rk_memset(bs, 0, sizeof_n(*bs, bitset_words(nbits)));
}

/// @brief Sets all bits to 1 (and clears padding bits).
/// @param bs,nbits Bitset and its logical size
/// @return `bs` (for chaining).
static_fun bitset bitset_set_all(bitset bs, size_t nbits) {
  rk_memset(bs, 0xFF, sizeof_n(*bs, bitset_words(nbits)));
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
/* END INLINE: include/rk_bitset.h */
/* inlined from include/rklib.h:9: #include "rk_alloc.h" */
/* skipped already-included: "include/rk_alloc.h" */
/* inlined from include/rklib.h:10: #include "rk_arena.h" */
/* BEGIN INLINE: include/rk_arena.h */
// SPDX-License-Identifier: MIT
/// @file rk_arena.h
/// @version 1.0.0
/// @defgroup rk_arena Arena Allocator Interface
/// @brief Arena Allocator Implementation
///
/// Provides a simple linear/stack-style arena allocator for fast temporary memory allocation.
/// Supports reset and simple realloc-like behavior. Also provides integration with the Allocator
/// interface defined in `rk_alloc.h` for use as a plug-in allocator.
///
/// Arena allocators are efficient when many small allocations are needed with the same lifetime, as
/// freeing all allocations at once is trivial.
///
/// @note This arena is not thread-safe, it is recommended to create several thread-local arenas
/// instead
/// @see rk_alloc.h
/// @see rk_defs.h
/// @{
#ifndef RK_ARENA_H
#define RK_ARENA_H
/* inlined from include/rk_arena.h:21: #include "rk_alloc.h" */
/* skipped already-included: "include/rk_alloc.h" */
RK_HEADER_BEGIN
RK__IGNWARN_CLANG_BEG("-Wreturn-type-c-linkage")

/// @brief Linear / stack allocator for fast, temporary memory management.
/// @details The Arena allocator manages a region of memory where allocations increment a pointer,
/// allowing fast allocation and reset.
typedef struct Arena {
  unsigned char* beg; ///< Start of arena memory block
  unsigned char* cur; ///< Current Position in arena memory block
  unsigned char* end; ///< End of arena memory block
} Arena;

/// @brief Initialises an Arena from an array as storage at runtime. Allows for any memory to use
/// the arena allocator and the more general allocator interface.
/// @param arr The byte array to serve as the arena's backing memory
/// @param len The length of `arr`, in bytes
/// @return New Arena using the array as backing storage
static_fun rk_pure Arena arena_init(unsigned char* arr, size_t len) {
  return (Arena){.beg = arr, .cur = arr, .end = arr ? arr + len : 0};
}

/// @brief `Arena arena_init_static(unsigned char arr[])` Initialises an Arena from an array as
/// storage at compile time. This macro allows for non-dynamically allocated memory to use the arena
/// allocator and the more general allocator interface.
/// @param array_non_compound_literal The byte array to serve as the arena's backing memory
/// @return New Arena with the array as backing storage
/// @warning Do not use with compound literals; use `arena_init()` instead.
#define arena_init_static(array_non_compound_literal)                                              \
  RK__arena_init_static(array_non_compound_literal)

/// @brief Resets the arena, marking all of its allocations as free.
/// @return `self`, for chaining
static_fun Arena*         arena_clear(Arena* self) { return self->cur = self->beg, self; }

/// @brief Returns the number of bytes an Arena can allocate in total.
static_fun rk_pure size_t arena_cap(const Arena* self) {
  return rk_likely(self && self->beg) ? (size_t)(self->end - self->beg) : 0;
}

/// @brief Returns the number of bytes an Arena has allocated.
static_fun rk_pure size_t arena_used(const Arena* self) {
  return rk_likely(self && self->beg) ? (size_t)(self->cur - self->beg) : 0;
}

/// @brief Returns the number of bytes an Arena can still allocate before running out of space.
static_fun rk_pure size_t arena_remaining(const Arena* self) {
  return rk_likely(self && self->beg) ? (size_t)(self->end - self->cur) : 0;
}

/// @brief Returns whether the arena has no allocations.
static_fun rk_pure bool arena_is_empty(const Arena* self) {
  return rk_likely(self && self->beg) ? self->cur == self->beg : true;
}

typedef struct ArenaMark ArenaMark;

/// @brief Returns the current position of the arena as an opaque marker. Pass to `arena_rewind_to`
/// to restore the arena to this state.
/// @return Pointer to the current position in the arena
static_fun ArenaMark     arena_mark(const Arena* self);

/// @brief Rewinds the arena's current pointer to `mark`, marking memory starting from `mark` as
/// free.
/// @return `self`, for chaining
/// @attention Behavior is undefined if `mark` was not allocated by the arena.
static_fun Arena*        arena_rewind_to(Arena* self, ArenaMark mark);

/// @brief Returns whether `ptr` is the most recently made allocation of the given `size`, i.e.
/// whether it ends exactly at the arena's current position.
/// @param ptr The allocation to check. Must be an allocation made by the arena.
/// @param size Size of the allocation in bytes
/// @return `true` if `ptr` is the top allocation, `false` otherwise
static_fun bool          arena_is_top_allocation(const Arena* self, const void* ptr, size_t size) {
  return (const unsigned char*)ptr + size == self->cur;
}

/// @brief `void* arena_allocate(size_t nbytes, size_t align, Arena* self)` - Allocates `nbytes`
/// bytes with the given alignment. Aborts on failure via `RK_ARENA_FAIL`. Prefer `arena_new` for
/// typed allocations.
/// @param nbytes Number of bytes to allocate
/// @param align  Desired alignment; must be a power of two
/// @param self   Pointer to the arena to allocate from
/// @return Pointer to the allocated memory
static_fun void* arena_allocate(size_t nbytes, size_t align, Arena* self);

/// @brief `void* arena_try_allocate(size_t nbytes, size_t align, Arena* self)`
/// - like `arena_allocate()` but returns NULL if the arena does not have enough space instead of
///   invoking the failure handler.
static_fun void* arena_try_allocate(size_t nbytes, size_t align, Arena* self);

/// @brief `void* arena_resize_top(size_t old_size, size_t new_size, Arena* self)` - Resizes the
/// most recent allocation in the arena by moving the cursor. Aborts on failure via `RK_ARENA_FAIL`.
/// Prefer `arena_extend` for typed resizes.
/// @param old_size Current size of the allocation in bytes
/// @param new_size Desired size of the allocation in bytes
/// @param self     Pointer to the arena owning the allocation
/// @return `ptr` on success
static_fun void* arena_resize_top(size_t old_size, size_t new_size, Arena* self);

/// @brief `void* arena_try_resize_top(size_t old_size, size_t new_size, Arena* self)` - Like
/// `arena_resize_top()` but returns NULL if the arena does not have enough space instead of
/// invoking the failure handler.
static_fun void* arena_try_resize_top(size_t old_size, size_t new_size, Arena* self);

/// @brief `T* arena_new(T, size_t count, Arena* arena)` - Creates a new allocation in the arena for
/// a given type and count.
/// @param  T     The type to allocate
/// @param  count Number of elements of type T to allocate
/// @param  arena Pointer to the arena to allocate from
/// @return T* Pointer to the allocated memory
#define arena_new(T, count, arena)                RK__arena_NEW(T, count, arena)

/// @brief `T* arena_try_new(T, size_t count, Arena* arena)` - Like `arena_new()`, but returns
/// `NULL` if the arena does not have enough space instead of invoking the failure handler.
#define arena_try_new(T, count, arena)            arena_try_new_aligned(T, count, alignof(T), arena)

/// @brief `T* arena_new_aligned(T, size_t count, size_t alignment, Arena* arena)` - Creates a new
/// allocation in the arena for a given type T and count with a given alignment independent of type.
/// @param T      The type to allocate
/// @param count  Number of elements of type T to allocate
/// @param align  Desired alignment of the allocation
/// @param arena  Pointer to the arena to allocate from
/// @return Pointer to the allocated memory.
/// @note Alignment must be a power of two.
#define arena_new_aligned(T, count, align, arena) RK__arena_ALIGNED_NEW(T, count, align, arena)

/// @brief `T* arena_try_new_aligned(T, size_t count, size_t align, Arena* arena)` like
/// `arena_new_aligned()`, but returns `NULL` if the arena does not have enough space instead of
/// invoking the failure handler.
#define arena_try_new_aligned(T, count, align, arena)                                              \
  (rk_assert_valid_align(T, align), (T*)arena_try_allocate(sizeof_n(T, count), align, arena))

/// @brief `T* arena_extend(T* ptr, size_t old_count, size_t new_count, Arena* arena)` - Resizes the
/// most recent allocation from `old_count` to `new_count` elements. Aborts on failure via
/// `RK_ARENA_FAIL`.
/// @param ptr Pointer to the allocation to extend; must be the most recent allocation in the arena
/// @param old_count Current number of allocated elements
/// @param new_count Desired number of elements after resizing
/// @param arena     Arena owning the allocation
/// @return `ptr` on success, cast to the same pointer type
#define arena_extend(ptr, old_count, new_count, arena)                                             \
  ((typeof(ptr))RK__arena_extend(ptr, sizeof_n(*(ptr), old_count), sizeof_n(*(ptr), new_count),    \
                                 arena))

/// @brief `T* arena_try_extend(T* ptr, size_t old_count, size_t new_count, Arena* arena)` - Like
/// `arena_extend()` but returns `NULL` if the arena does not have enough space instead of invoking
/// the failure handler.
#define arena_try_extend(ptr, old_count, new_count, arena)                                         \
  ((typeof(ptr))RK__arena_try_extend(ptr, sizeof_n(*(ptr), old_count),                             \
                                     sizeof_n(*(ptr), new_count), arena))

static_fun alloc_allocation_f   RK__arena_allocate;
static_fun alloc_reallocation_f RK__arena_reallocate;
static_fun alloc_deallocation_f RK__arena_deallocate;
static const AllocatorVTable    arena_allocator_vtable = {.alloc_f   = RK__arena_allocate,
                                                          .realloc_f = RK__arena_reallocate,
                                                          .dealloc_f = RK__arena_deallocate};

// todo important fix
/// @brief `Allocator arena_to_alloc_static(Arena* arena)` - Creates an Allocator from an Arena
/// allowing it to serve as backing allocator for other rk_clib types. Works at compile-time and can
/// be used for static initialisation.
/// @note Expands to a fully parenthesized compound literal (not a bare brace-list) specifically so
/// it stays a single, comma-safe expression when passed as an argument to another macro (e.g.
/// `dict_init_static(K, V, arena_to_alloc_static(&my_arena))`) -- a bare `{...}`'s internal comma
/// would otherwise be miscounted as an argument separator by the enclosing macro.
#define arena_to_alloc_static(arena) ((Allocator){.vtab = &arena_allocator_vtable, .ctx = (arena)})

/// @brief Creates an Allocator from an Arena at runtime, allowing it to serve as backing allocator
/// for other rk_clib types.
static_fun Allocator arena_to_alloc(Arena* arena) { return arena_to_alloc_static(arena); }

/// @brief Type of an Allocator object managing an array of size `size` using an Arena to manage its
/// memory.
#define arr_allocator(size)                                                                        \
  struct {                                                                                         \
    union {                                                                                        \
      const Allocator alloc;                                                                       \
      const struct {                                                                               \
        const AllocatorVTable* vtab;                                                               \
        void*                  ctx;                                                                \
      };                                                                                           \
    };                                                                                             \
    Arena                     arena;                                                               \
    alignas_max unsigned char arr[(size)];                                                         \
  }

/// @brief Initialise an arr_allocator object by passing its address.
#define arr_allocator_init(self)                                                                   \
  {.vtab = &arena_allocator_vtable, .ctx = &(self)->arena, .arena = arena_init_static((self)->arr)}

/// @brief Declares and initializes a static, stack-allocated Allocator instance with internal
/// arena-based storage.
/// @details This macro creates a compound struct on the stack (or in static scope) that embeds:
/// - a fixed-size memory buffer (`arr[size]`) as backing storage,
/// - and an `Arena` allocator using that buffer.
/// - an `Allocator` interface (via union for compatibility), The resulting variable can be treated
///   like an `Allocator` and used anywhere the `rk_clib` allocator interface is expected. This is
///   particularly useful for creating fast, temporary allocators with automatic storage duration.
/// @param name The name of the variable to declare
/// @param size The size (in bytes) of the internal backing memory buffer, must be a compile-time
/// constant.
///
/// Usage:
/// ```c
///     arr_allocator_create(temp_alloc, 4096);
///     int* ptr = alloc_new(int, 10, temp_alloc.alloc);
/// ```
#define arr_allocator_create(name, size) arr_allocator(size) name = arr_allocator_init(&name)

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

typedef struct ArenaMark { unsigned char* pos; } ArenaMark;

static_fun ArenaMark arena_mark(const Arena* self) { return (ArenaMark){.pos = self->cur}; }

#define rk_assert_ptr_in_arena(arena, ptr)                                                         \
  rk_assert(rk_ptr_in_range(ptr, (arena)->beg, (arena)->cur)                                       \
            && "Pointer was not allocated by this Arena")

static_fun Arena* arena_rewind_to(Arena* self, ArenaMark mark) {
  if (mark.pos == self->cur) { return self; }
  rk_assert_ptr_in_arena(self, mark.pos);
  self->cur = mark.pos;
  return self;
}

static_fun rk_alloc_alignsize(2, 1) void* arena_try_allocate(size_t nbytes, size_t align,
                                                             Arena* self) {
  if rk_unlikely (!self->cur) { return rk_null; }
  size_t pad = rk_align_pad(self->cur, align), avail = arena_remaining(self);
  if (avail < pad || avail - pad < nbytes) { return rk_null; }
  unsigned char* ptr = self->cur + pad;
  self->cur          = ptr + nbytes;
  return ptr;
}

static_fun rk_alloc_size(2) void* arena_try_resize_top(size_t old_size, size_t new_size,
                                                       Arena* self) {
  if (arena_remaining(self) + old_size < new_size) { return rk_null; }
  unsigned char* ptr = self->cur - old_size;
  self->cur          = ptr + new_size;
  return ptr;
}

static_fun rk_alloc_size(2) void* arena_resize_top(size_t old_size, size_t new_size, Arena* self) {
  void* res = arena_try_resize_top(old_size, new_size, self);
  RK_ARENA_FAIL(res, self, (self->cur - old_size), align_max, new_size);
  return res;
}

static_fun rk_alloc_alignsize(2, 1) void* RK__arena_allocate(size_t nbytes, size_t align,
                                                             void* ctx) {
  void* ptr = arena_try_allocate(nbytes, align, (Arena*)ctx);
  RK_ARENA_FAIL(ptr, (Arena*)ctx, rk_null, align, nbytes);
  return ptr;
}

static_fun void RK__arena_deallocate(void* ptr, size_t old_size, size_t align rk_unused,
                                     void* ctx) {
  if (arena_is_top_allocation((Arena*)ctx, ptr, old_size)) {
    (void)arena_try_resize_top(old_size, 0, (Arena*)ctx);
  }
}

static_fun rk_alloc_alignsize(4, 3) void* RK__arena_reallocate(void* ptr, size_t old_size,
                                                               size_t new_size, size_t align,
                                                               void* ctx) {
  rk_assert_align_pow2(align);
  Arena* self = (Arena*)ctx;
  if (!old_size) { return RK__arena_allocate(new_size, align, self); }
  if ((arena_is_top_allocation(self, ptr, old_size)
       && arena_try_resize_top(old_size, new_size, self))
      || new_size <= old_size) { // non-top shrinks are no-ops
    return ptr;
  }
  void* res = RK__arena_allocate(new_size, align, ctx);
  rk_memcpy(res, ptr, rk_min(old_size, new_size));
  return res;
}

static_fun rk_alloc_alignsize(2, 1) void* arena_allocate(size_t nbytes, size_t align, Arena* self) {
  return RK__arena_allocate(nbytes, align, self);
}
static_fun rk_alloc_size(3) void* RK__arena_try_extend(void* ptr, size_t old_size, size_t new_size,
                                                       Arena* self) {
  rk_assert(arena_is_top_allocation(self, ptr, old_size)
            && "Can only resize the top allocation of the arena");
  return arena_try_resize_top(old_size, new_size, self);
}

static_fun rk_alloc_size(3) void* RK__arena_extend(void* ptr, size_t old_size, size_t new_size,
                                                   Arena* self) {
  void* r = RK__arena_try_extend(ptr, old_size, new_size, self);
  RK_ARENA_FAIL(r, self, (self->cur - old_size), align_max, new_size);
  return r;
}

#define RK__arena_init_static(arr)                                                                 \
  {.beg = (arr) + rk_ensure_valid_storage_type(arr), .cur = (arr), .end = (arr) + sizeof(arr)}

#define RK__arena_ALIGNED_NEW(T, count, align, arena)                                              \
  ((typeof(T)*)(alloc_log_new, rk_assert_valid_align(T, align),                                    \
                arena_allocate(sizeof_n(T, count), align, arena)))
#define RK__arena_NEW(T, count, arena)                                                             \
  ((typeof(T)*)(alloc_log_new, arena_allocate(sizeof_n(T, count), alignof(T), arena)))

/// @endcond
RK__IGNWARN_CLANG_END()
RK_HEADER_END
/// @}
#endif // RK_ARENA_H

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
/* END INLINE: include/rk_arena.h */
/* inlined from include/rklib.h:11: #include "rk_vec.h" */
/* BEGIN INLINE: include/rk_vec.h */
// SPDX-License-Identifier: MIT
/// @file rk_vec.h
/// @version 1.0.0
/// @defgroup rk_vec Vec (Dynamic Array) Interface
/// @brief Header file for a heap-allocated dynamic array (vec) implementation inspired by Sean
/// Barrett's stretchy buffer.
///
/// This file provides macros and functions for creating and managing dynamic arrays in C. The
/// implementation supports an optional custom allocator interface (defined in `rk_alloc.h`, turned
/// off via setting `RK_CUSTOM_ALLOCATORS` to `0`) for flexible memory management.
///
/// Each Vec is represented as a simple dynamically allocated pointer to the element type, which can
/// be dereferenced like a regular C array. Metadata such as the Vec's length and capacity are
/// stored in a header located just before the user-facing data pointer. This metadata can be
/// accessed via functions like `vec_count()`.
///
/// Elements can be added to the Vec using `vec_push()`, which automatically resizes the underlying
/// memory and updates the pointer as needed.
///
/// None of the macros are safe against multiple/unintuitive evaluation of arguments; never use any
/// expression with side effects as arguments of any of the macros.
///
/// A `NULL` pointer is considered a valid, empty Vec with zero capacity. Most functions (unless
/// otherwise stated) handle `NULL` vecs gracefully, automatically initializing them as needed
/// (e.g., `Vec(int) v = NULL; vec_push(v, 5)` initializes the Vec and adds the element 5). Once
/// initialized, vecs maintain the invariant that capacity is always >= length.
///
/// Features:
/// - Automatic resizing and capacity management
/// - Optional custom allocator support
/// - Safe and convenient macros for push, pop, clear, insert, erase, and more
/// - Iteration macros for easy looping over Vec elements
///
/// Usage:
/// ```c
/// Vec(int) vec = vec_init(int, 10);  // Create Vec with initial capacity 10
/// vec_push(vec, 5);                  // Add element 5 to vec
/// rk_assert(vec[0] == 5);
/// vec_release(vec);                  // Free Vec memory
/// ```
/// @see rk_alloc.h
/// @see rk_defs.h
/// @{

#ifndef RK_VEC_H
#define RK_VEC_H
/* inlined from include/rk_vec.h:47: #include "rk_alloc.h" */
/* skipped already-included: "include/rk_alloc.h" */
RK_HEADER_BEGIN

/// @brief Macro to indicate that an object is a Vec.
/// @param T The type of elements stored in the Vec
/// @note Vec(void) indicates that a Vec of any type is accepted as a parameter, this does not hold
/// for macros which need type information. Using Vec as `Vec(void)` in any macro is undefined as
/// most vec macros rely on type information such as sizeof.
/// @note Implemented as `typeof(T*)` rather than `T*` so that array types such as `Vec(int[5])` are
/// supported.
#define Vec(T) typeof(T*)

/// @brief Header of a dynamically allocated Vec. The Vec is implemented as a contiguous block of
/// memory with a header (`VecHeader`) that stores metadata about the vec, such as its capacity,
/// length, and allocator.
typedef struct VecHeader {
#if RK_CUSTOM_ALLOCATORS
  Allocator alloc; ///< Allocator (can be disabled)
#endif
  size_t                    cap;    ///< Capacity of the Vec (in terms of elements)
  size_t                    count;  ///< Length of the Vec (in terms of elements)
  alignas_max unsigned char data[]; ///< Vec Data
} VecHeader;

/// @brief `Vec(T) vec_init(T, size_t cap, Allocator alloc = alloc_ctx)`
/// - Initialises a Vec from an initial capacity and an optional Allocator.
/// @param T           The desired type of the Vec's elements
/// @param init_cap    The initial capacity of the Vec (in elements)
/// @param allocator   Optional allocator; defaults to `alloc_ctx`.
/// @return Vec(T) the vec
/// @note Zero-Capacity vecs are always uninitialised
///
/// Usage:
/// ```c
/// Vec(int) v1  = vec_init(int, 10);           // create an int-Vec with 10 cap
///                                                using alloc_ctx
/// Vec(int) v2 = vec_init(int, 2, my_alloc);   // creates an int Vec with 2 cap
///                                             // using my_alloc as allocator
/// Vec(int) v3 = vec_init(int, 0);             // does nothing (0 cap)
/// ```
#define vec_init(T, init_cap, ...)                                                                 \
  ((Vec(T))((void)static_assert_expr(alignof(T) <= align_max,                                      \
                                     "Over-aligned Types not supported."),                         \
            rk_overload(RK__vec_init, T, init_cap, ##__VA_ARGS__)))

/// @brief `Vec(T) vec_init_list(T, Allocator alloc = alloc_ctx, T... values)` - Initialises a Vec
/// from a list of values.
/// @param T    The Type
/// @param alloc Allocator optional, defaults to alloc_ctx
/// @param values T, ... the values to initialise the Vec with
/// @return Vec(T) a new Vec, initialised with the values
///
/// Usage:
/// ```c
/// Vec(int) vec = vec_init_list(int, 1, 2, 3, 4, 5); /* uses alloc_ctx */
/// Vec(int) vec2 = vec_init_list(int, my_alloc, 1, 2, 3); /* uses my_alloc */
///
/// // Compound literals must be wrapped in parens
/// typedef struct Pair{ int x, y; } Pair;
/// Vec(struct Pair) vec3 = vec_init_list(Pair, ((Pair){1, 2}), ((Pair){3, 4}));
/// ```
#define vec_init_list(T, ...)                                                                      \
  ((Vec(T))((void)static_assert_expr(alignof(T) <= align_max,                                      \
                                     "Over-aligned Types not supported."),                         \
            RK__vec_init_list(T, ##__VA_ARGS__)))

/// @brief `Vec(T) vec_from(T* arr, size_t count, Allocator alloc = alloc_ctx)` - Constructs a new
/// Vec by copying `count` elements from `arr`.
/// @param arr   Source array of `count` elements; its element type becomes the new Vec's element
/// type (via `typeof(*arr)`)
/// @param count Number of elements to copy
/// @param alloc Allocator Optional, defaults to `alloc_ctx`
/// @return A Vec containing a copy of `arr`'s first `count` elements, or `NULL` if `count == 0`
/// @note To clone an existing Vec while preserving its own allocator, pass it directly along with
/// its own count/allocator: `vec_from(v, vec_count(v), vec_allocator(v))`. Unlike a Vec, a plain
/// array has no allocator of its own to default to, so `vec_from()` always defaults to `alloc_ctx`
/// when no allocator is given.
///
/// Usage:
/// ```c
/// int arr[] = {1, 2, 3};
/// Vec(int) v = vec_from(arr, 3);            // uses alloc_ctx
/// Vec(int) v2 = vec_from(arr, 3, my_alloc);  // uses my_alloc
/// ```
#define vec_from(arr, count, ...)                                                                  \
  ((typeof(*(arr))*)rk_overload(RK__vec_from, arr, count, ##__VA_ARGS__))

/// @brief `void vec_release(Vec(T)& self)` - Frees the underlying allocation and sets the Vec to
/// NULL.
#define vec_release(self) ((void)RK__vec_release(self))

/// @defgroup rk_vec_accessors_u Unchecked Vec Accessors
/// @ingroup rk_vec
/// @brief Unchecked Accessor functions and macros for Vec metadata. These macros do not perform
/// null-checks on the Vec and allow direct, unchecked member access.
///
/// @{

/// @brief Unchecked access to the header of `self` as an lvalue.
#define vec_HEADER(self)                                                                           \
  ((VecHeader*)(void*)((char*)(self)                                                               \
                       - offsetof(VecHeader, data))) // NOLINT(clang-analyzer-security.ArrayBound)

#if RK_CUSTOM_ALLOCATORS
# define vec_ALLOCATOR(V) (vec_HEADER(V)->alloc) // NOLINT(clang-analyzer-security.ArrayBound)

#else
# define vec_ALLOCATOR(V) ((void)(V), alloc_ctx)
#endif

/// @brief Unchecked access to the capacity of `self` as an lvalue.
#define vec_CAP(self)   (vec_HEADER(self)->cap) // NOLINT(clang-analyzer-security.ArrayBound)

/// @brief Unchecked access to the count of `self` as an lvalue.
#define vec_COUNT(self) (vec_HEADER(self)->count) // NOLINT(clang-analyzer-security.ArrayBound)
#define vec_LEN         vec_COUNT                 // NOLINT(clang-analyzer-security.ArrayBound)

/// @}

/// @defgroup rk_vec_accessors_c Checked Vec Accessors
/// @ingroup rk_vec
/// @brief Checked Accessor functions and macros for Vec metadata. These macros check for
/// unallocated (NULL) Vecs.
///
/// @{

/// @brief Returns a pointer to the header of the vec, for direct access to metadata
/// @return Pointer to the header of the Vec or NULL, if `self` is NULL
static_fun rk_const VecHeader* vec_header(const Vec(void) self) {
  return self ? vec_HEADER(self) : rk_null;
}

/// @brief Returns the number of elements in the vec, 0 if `self` is NULL.
static_fun rk_pure size_t    vec_count(const Vec(void) self) { return self ? vec_COUNT(self) : 0; }

/// @brief Alias for `vec_count()`
static_fun rk_pure size_t    vec_len(const Vec(void) self) { return vec_count(self); }

/// @brief Returns the current capacity of the Vec, 0 iff `self` is NULL.
static_fun rk_pure size_t    vec_cap(const Vec(void) self) { return self ? vec_CAP(self) : 0; }

/// @brief Returns the allocator of the vec or `alloc_ctx` if `self` is `NULL`.
static_fun rk_pure Allocator vec_allocator(const Vec(void) self) {
#if RK_CUSTOM_ALLOCATORS
  return self ? vec_ALLOCATOR(self) : alloc_ctx;
#else
  return (void)self, alloc_ctx;
#endif
}

/// @brief Returns whether the count of a Vec is zero.
static_fun rk_pure bool vec_is_empty(const Vec(void) self) { return vec_count(self) == 0; }

/// @brief Clears the contents of `self` by setting its count to 0.
static_fun void vec_clear(Vec(void) self) {
  if (self) { vec_COUNT(self) = 0; }
}

/// @brief `size_t vec_allocation_size(Vec(T) self)` - Returns the total size of memory allocated
/// for the Vec in bytes, including. its header, 0 iff `self` is NULL.
#define vec_allocation_size(self) RK__vec_allocation_size(self, sizeof(*(self)))

/// @brief Returns the remaining count of elements that can be pushed to a vec without reallocation.
static_fun rk_pure size_t vec_remaining(const Vec(void) self) {
  return self ? vec_CAP(self) - vec_COUNT(self) : 0;
}

/// @brief Returns whether an index is within the range of a Vec.
static_fun bool vec_index_in_range(const Vec(void) self, size_t idx) {
  return idx < vec_count(self);
}

/// @brief `void vec_reserve(Vec(T)& self, size_t new_cap)` - Grows the Vec to be able to hold at
/// least `new_cap` elements.
/// @attention **Arguments with side effects are not safe in `vec_` macros**
/// @note Reassigns `self`, if necessary
#define vec_reserve(self, new_cap)    ((void)RK__vec_reserve(self, new_cap))

/// @brief `void vec_resize(Vec(T)& self, size_t len)` - Resizes the Vec to `len`, expanding the
/// capacity by reallocating and creating uninitialised objects if necessary.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Reassigns `self`, if necessary
#define vec_resize(self, len)         ((void)RK__vec_resize(self, len))

/// @brief `void vec_shrink_to_fit(Vec(T)& self)` - Shrinks the Vec's capacity to the next power of
/// two greater than or equal to its length (matching `str_shrink_to_fit()`'s convention), leaving
/// some slack to reduce reallocation on subsequent growth.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Reassigns `self`, if necessary; deallocates the Vec if it is empty.
/// @note Use `vec_shrink_to_fit_exact()` for an exact-capacity shrink.
#define vec_shrink_to_fit(self)       ((void)RK__vec_shrink_to_fit(self))

/// @brief `void vec_shrink_to_fit_exact(Vec(T)& self)` - Shrinks the Vec's capacity to be exactly
/// equal to its length.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Reassigns `self`, if necessary; deallocates the Vec if it is empty.
#define vec_shrink_to_fit_exact(self) ((void)RK__vec_shrink_to_fit_exact(self))

/// @brief `void vec_assign(Vec(T)& self, T* arr, size_t count)` - Assigns `count` objects of `arr`
/// to the Vec, overriding its contents and expanding `self`, if necessary.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @attention `arr[0..count)` must not overlap the Vec's own backing allocation: if growth is
/// triggered, the old buffer is freed before the copy from `arr` happens, turning an `arr` that
/// points into it into a use-after-free; even without growth, the underlying copy is a plain
/// `memcpy`, which is undefined for overlapping source and destination.
/// @note Reassigns `self`, if necessary.
#define vec_assign(self, arr, count)  ((void)RK__vec_assign(self, arr, count))

/// @brief `T& vec_front(Vec(T) self)` - Returns an Lvalue reference to the first element of the
/// Vec.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Behaviour undefined for empty or uninitialised vec
#define vec_front(self)               (((typeof(self))RK__vec_check_front(self))[0])

/// @brief `T& vec_back(Vec(T) self)` - Returns an Lvalue reference to the last element of the Vec.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Behaviour undefined for empty or uninitialised vec
#define vec_back(self)                (*((typeof(self))RK__vec_check_back(sizeof(*(self)), self)))

/// @brief `void vec_push(Vec(T)& self, T obj)` - Pushes a value onto the Vec, resising the
/// allocation, if necessary.
/// @attention **`obj` must not modify the vec due to sequencing issues**
/// @note Reassigns `self`, if necessary
#define vec_push(self, obj)           ((void)RK__vec_push(self, obj)) // NOLINT

/// @brief `void vec_push_n(Vec(T)& self, T* arr, size_t count)` - Copies `count` values of `arr`
/// onto `self`. `arr` must be a pointer variable of type `T*`.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @attention `arr[0..count)` must not overlap the Vec's own backing allocation, for the same
/// reasons documented on `vec_assign()`.
/// @note Reassigns `self`, if necessary
#define vec_push_n(self, arr, count)  ((void)RK__vec_push_arr(self, arr, count))

/// @brief `void vec_push_unchecked(Vec(T)& self, T obj)` - Pushes a value onto the Vec, not
/// checking for capacity.
/// @attention **`obj` must not modify the vec due to sequencing issues**
#define vec_push_unchecked(self, obj) ((void)(RK__vec_push_u(self, obj)))

/// @brief `T vec_pop(Vec(T)& self)` - Pops the last value off the Vec and decreases its length.
/// @return The popped value
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Behaviour in case of empty or uninitialised Vec is undefined
#define vec_pop(self)                 ((self)[--vec_COUNT(RK__check_vec_pop(self))])

/// @brief `T* vec_pop(Vec(T)& self, size_t count)` - Pops 'count' values off the Vec, decreasing
/// its length.
/// @return A pointer to the popped memory region, to copy away from
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Behaviour in case of `count >= vec_count(self)` undefined
#define vec_pop_n(self, count)        (typeof(self))RK__vec_pop_n(sizeof(*(self)), self, count)

/// @brief `void vec_insert_at(Vec(T)& self, size_t idx, T obj)` - Inserts an object at index `idx`,
/// shifting subsequent elements back and expanding `self`, if necessary.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Reassigns `self`, if necessary. Behavior is undefined if `idx` is out of bounds.
#define vec_insert_at(self, idx, obj) ((void)(RK__vec_insert_at(self, idx, obj)))

/// @brief `void vec_insert_at_unordered(Vec(T)& self, size_t idx, T obj)` - Inserts `obj` at index
/// `idx` without preserving element order. The element currently at `idx` is moved to the back
/// before `obj` is placed at `idx`. O(1) (ignoring possible reallocation), unlike `vec_insert_at`
/// which is O(n).
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Reassigns `self`, if necessary. Behavior is undefined if `idx` > vec_count(self).
#define vec_insert_at_unordered(self, idx, obj) ((void)RK__vec_insert_at_unordered(self, idx, obj))

/// @brief `void vec_insert_arr_at(Vec(T)& self, size_t idx, T* obj, size_t count)` - Batched
/// `vec_insert()`, faster when adding multiple elements at once.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @attention `ptr[0..count)` must not overlap the Vec's own backing allocation, for the same
/// reasons documented on `vec_assign()`.
/// @param self  The Vec (must be an lvalue)
/// @param idx   The Index of the Vec to store in
/// @param ptr   A pointer to the array of objects to insert
/// @param count The amount of objects to push
/// @code Vec(int) vec = vec_init(int, 10); int arr[3] = {1, 2, 3}; vec_insert_arr_at(vec, 1, arr,
/// countof(arr)); // Alternatively, pass a compound literal enclosed in parens such as
/// vec_insert_arr_at(vec, 2, ((int[]){4, 5, 6}), countof((int[]){4, 5, 6})); rk_assert(vec[0] ==
/// 1);
/// @endcode
/// @note Behavior is undefined if `idx` > vec_count(self)
#define vec_insert_arr_at(self, idx, ptr, count)                                                   \
  ((void)(RK__vec_insert_arr_at(self, idx, ptr, count)))

/// @brief `void vec_erase_at(Vec(T) self, size_t idx)` - Erases an element from `self` at index
/// `idx`, shifting subsequent elements forward.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Behavior is undefined if `idx` is out of bounds.
#define vec_erase_at(self, idx) ((void)(RK__vec_erase_at(self, idx)))

/// @brief `void vec_erase_at_unordered(Vec(T) self, size_t idx)` - Erases an element at index `idx`
/// without preserving element order. The last element is moved into the erased slot. O(1), unlike
/// `vec_erase_at` which is O(n).
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Behavior is undefined if `idx` is out of bounds.
#define vec_erase_at_unordered(self, idx)                                                          \
  ((void)(((self)[idx] = (self)[RK__decrease_index_check(self)])))

/// @brief `void vec_erase_at_n(Vec(T) self, size_t idx, size_t count)` - Erases `count` elements at
/// index `idx` from `self`, shifting subsequent elements forward.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Behavior is undefined if `idx` + count > vec_count()
#define vec_erase_at_n(self, idx, count) ((void)(RK__vec_erase_at_n(self, idx, count)))

/// @brief `T* vec_end(Vec(T) self)` - Returns a pointer one past the end of a the elements of
/// `self` or `NULL` if `self` is `NULL`.
#define vec_end(self)                    ((self) ? ((self) + vec_COUNT(self)) : (self))

/// @brief Convenience Macro to loop over the elements of a vec.
/// @param vec The Vec to loop over
/// @param it  The name of the iterator (pointer to each element)
/// @note Do not erase or add elements while looping in this fashion. Use `vec_iterate()` in that
/// case.
///
/// Usage:
/// ```c
/// Vec(int) vec = vec_init(int, 10);
/// vec_push(vec, 1), vec_push(vec, 2);
/// vec_foreach(vec, it) { printf("%d\n", *it); }
/// ```
#define vec_foreach(vec, it)                                                                       \
  for (typeof(*(vec))*RK___VEC = (vec), *const RK___END = vec_end(RK___VEC); RK___VEC != RK___END; \
       ++RK___VEC)                                                                                 \
    for (typeof(*RK___VEC)*const it = RK___VEC, *RK___ONCE = RK___VEC; RK___ONCE; RK___ONCE = 0)

/// @brief Like vec_foreach(), iterating in reverse order.
#define vec_foreach_reversed(vec, it)                                                              \
  for (typeof(*(vec))*const RK___VEC = (vec), *RK___END = vec_end(RK___VEC);                       \
       RK___VEC != RK___END;)                                                                      \
    for (typeof(*RK___VEC)*const it = --RK___END, *RK___ONCE = it; RK___ONCE; RK___ONCE = 0)

/// @brief Convenience Macro to erase all elements in a Vec that satisfy a predicate.
/// @param vec         The Vec to loop over
/// @param it          The name of the iterator (access via *it)
/// @param pred        The predicate (an expression working on *it)
///
/// Usage:
/// ```c
/// Vec(int) vec = vec_init(int, 10);
/// vec_push(vec, 1), vec_push(vec, 2);
/// vec_erase_if(vec, it, *it % 2); // remove even numbers
/// ```
#define vec_erase_if(vec, it, pred)                                                                \
  do {                                                                                             \
    RK__IGNWARN_MSC_BEG(4114)                                                                      \
    typeof(vec) RK___VEC = (vec);                                                                  \
    if (!vec_count(RK___VEC)) { break; }                                                           \
    typeof(*RK___VEC)*RK___BEG = RK___VEC, *const RK___END = RK___BEG + vec_COUNT(RK___BEG);       \
    for (typeof(*RK___VEC)* RK___IT = RK___VEC; RK___IT != RK___END; ++RK___IT) {                  \
      typeof(*RK___VEC)* const it = RK___IT;                                                       \
      if (!(pred)) { *RK___BEG++ = *RK___IT; }                                                     \
    }                                                                                              \
    vec_COUNT(RK___VEC) = (size_t)(RK___BEG - RK___VEC);                                           \
    RK__IGNWARN_MSC_END()                                                                          \
  } while (0)

/// @brief Reverse the elements of a Vec in place.
#define vec_reverse(vec)                                                                           \
  do {                                                                                             \
    typeof(vec) RK___BEG = (vec);                                                                  \
    if (!vec_count(RK___BEG)) { break; }                                                           \
    typeof(RK___BEG) RK___END = RK___BEG + vec_COUNT(RK___BEG) - 1;                                \
    for (; RK___BEG < RK___END; ++RK___BEG, --RK___END) { rk_SWAP(*RK___BEG, *RK___END); }         \
  } while (0)

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

static_fun rk_forceinline size_t RK__decrease_index_check(void* self) {
  rk_assert(vec_count(self) && "Cannot decrease count of empty vec");
  return --vec_COUNT(self);
}

static_fun rk_forceinline void* RK__check_vec_push_u(void* self) {
  rk_assert(vec_remaining(self) && "Not enough capacity for unchecked push");
  return self;
}

static_fun rk_forceinline void* RK__check_vec_pop(void* self) {
  rk_assert(vec_count(self) && "Attempting to pop from zero-length vec");
  return self;
}

static_fun rk_forceinline void* RK__vec_pop_n(size_t elsize, void* self, size_t count) {
  rk_assert(vec_count(self) >= count && "Attempting to pop more than vec_count() elements");
  vec_COUNT(self) -= count;
  return (char*)self + rk_mult(elsize, vec_COUNT(self));
}

static_fun rk_forceinline void* RK__vec_check_front(void* self) {
  rk_assert(vec_count(self) && "Attempting to access front of zero-sized vec");
  return self;
}

static_fun rk_forceinline void* RK__vec_check_back(size_t elsize, void* self) {
  rk_assert(vec_count(self) && "Attempting to access back of zero-sized vec");
  return (char*)self + rk_mult(elsize, vec_COUNT(self) - 1);
}

static_fun rk_forceinline size_t RK__vec_assert_insertbounds(void* self, size_t i) {
  rk_assert(i <= vec_count(self) && "Attempting to insert into Vec at out of bounds index");
  return i;
}

static_fun rk_forceinline size_t RK__vec_assert_erasebounds_n(void* self, size_t i, size_t n) {
  rk_assert((i <= vec_count(self) && n <= vec_count(self) - i)
            && "Attempting to erase from Vec at out of bounds index");
  return i;
}

//  logical size of a vec if type information not available
#define RK__VECSIZE_UT(elsize, elcount) (offsetof(VecHeader, data) + rk_mult(elsize, elcount))
#define RK__VEC_COMPUTE_SIZE(V, C)      RK__VECSIZE_UT(sizeof(*(V)), C)
#define vec_ALLOCATION_SIZE(V)          RK__VEC_COMPUTE_SIZE(V, vec_CAP(V))
#define vec_LOGICAL_SIZE(V)             RK__VEC_COMPUTE_SIZE(V, vec_COUNT(V))

static_fun rk_forceinline rk_pure size_t RK__vec_allocation_size(const Vec(void) self,
                                                                 size_t          elsize) {
  return self ? RK__VECSIZE_UT(elsize, vec_CAP(self)) : 0;
}

static_fun rk_forceinline VecHeader* rk_alloc_size(2)
    RK__vec_init_f(size_t init_cap, size_t total_size,
                   size_t init_count RK_IFALLOC(, Allocator alloc)) {
  rk_assert_allocator_valid(alloc);
  VecHeader* v = (VecHeader*)alloc_allocate(total_size, align_max RK_IFALLOC(, alloc));
  v->cap = init_cap, v->count = init_count;
  RK_IFALLOC(v->alloc = alloc;)
  return v;
}
#define RK__VEC_NEW_NONZERO(T, cap, count, alloc)                                                  \
  ((typeof(T)*)(void*)(RK__vec_init_f(cap, offsetof(VecHeader, data) + sizeof_n(T, cap),           \
                                      count RK_IFALLOC(, alloc))                                   \
                           ->data))
#define RK__VEC_NEW(T, cap, count, alloc)                                                          \
  ((cap) ? RK__VEC_NEW_NONZERO(T, cap, count, alloc) : rk_null)

// initialises a Vec with positive cap (no cap 0 check) and assigns it to V
#define RK__VEC_INIT_ASSIGN(V, C, A) ((V) = RK__VEC_NEW_NONZERO(*(V), (C), 0, (A)))

#define RK__vec_init(T, C, A)        RK__VEC_NEW(T, (C), 0, (A))
#define RK__vec_init3(T, C, A)       rk_disable_if(RK__vec_init(T, C, A))
#define RK__vec_init2(T, C)          RK__vec_init(T, C, alloc_ctx)

#define RK__vec_from(arr, count, alloc)                                                            \
  ((count) ? rk_copy(RK__VEC_NEW(*(arr), (count), (count), (alloc)), (arr), (count)) : rk_null)

#define RK__vec_from3(arr, count, alloc) rk_disable_if(RK__vec_from(arr, count, alloc))
#define RK__vec_from2(arr, count)        RK__vec_from(arr, count, alloc_ctx)

#define RK__vec_release(V)                                                                         \
  ((V)                                                                                             \
   && (alloc_deallocate(vec_HEADER(V), vec_ALLOCATION_SIZE(V),                                     \
                        align_max RK_IFALLOC(, vec_ALLOCATOR(V))),                                 \
       (V) = rk_null))

/// always reallocates to a positive cap, sets cap accordingly
#define RK__VEC_CHANGE_CAP(V, C)                                                                   \
  ((V)                                                                                             \
   = (typeof(V))(void*)(((VecHeader*)alloc_reallocate(vec_HEADER(V), vec_ALLOCATION_SIZE(V),       \
                                                      RK__VEC_COMPUTE_SIZE(V, C),                  \
                                                      align_max RK_IFALLOC(, vec_ALLOCATOR(V))))   \
                            ->data),                                                               \
   vec_CAP(V) = (C), (V))

/*doubles capacity if at limit*/
#define RK__vec_reserve_1(V)                                                                       \
  ((V) ? (vec_COUNT(V) == vec_CAP(V) ? RK__VEC_CHANGE_CAP(V, rk_mult(vec_CAP(V), 2)) : (V))        \
       : RK__VEC_INIT_ASSIGN(V, 1, alloc_ctx))

#define RK__vec_reserve(V, C)                                                                      \
  ((V) ? ((C) > vec_CAP(V) ? RK__VEC_CHANGE_CAP(V, C) : (V))                                       \
       : ((C) ? RK__VEC_INIT_ASSIGN(V, C, alloc_ctx) : rk_null))

#define RK__vec_resize(V, C) (RK__vec_reserve(V, C), (V) && (vec_COUNT(V) = (C)))

#define RK__vec_shrink_to_fit_exact(V)                                                             \
  ((V) && vec_COUNT(V) < vec_CAP(V)                                                                \
   && (vec_COUNT(V) ? RK__VEC_CHANGE_CAP(V, vec_COUNT(V)) : (vec_release(V), (V) = rk_null)))

#define RK__vec_shrink_to_fit(V)                                                                   \
  ((V) && (vec_COUNT(V) ? stdc_bit_ceil(vec_COUNT(V)) : 0) < vec_CAP(V)                            \
   && (vec_COUNT(V) ? RK__VEC_CHANGE_CAP(V, stdc_bit_ceil(vec_COUNT(V)))                           \
                    : (vec_release(V), (V) = rk_null)))

#define RK__vec_push_u(V, O)        ((V)[vec_COUNT(RK__check_vec_push_u(V))++] = (O))
#define RK__vec_push(V, O)          (RK__vec_reserve_1(V), RK__vec_push_u(V, O))

#define RK__vec_push_arr_u(V, O, N) (rk_copy((V) + vec_COUNT(V), O, N), vec_COUNT(V) += (N))
#define RK__vec_push_arr(V, O, N)                                                                  \
  ((void)((N) && (RK__vec_reserve(V, vec_count(V) + (N)), RK__vec_push_arr_u(V, O, N), 1)))

static_fun rk_forceinline void RK__vec_insert_arr_at_f(size_t elsize, void* restrict v, size_t i,
                                                       const void* restrict arr, size_t n) {
  size_t old_count = vec_COUNT(v); // NOLINT(clang-analyzer-security.ArrayBound)
  char * src = (char*)v + rk_mult(i, elsize), *dst = src + rk_mult(n, elsize);
  memmove(dst, src, rk_mult(old_count - i, elsize));
  memcpy(src, arr, rk_mult(elsize, n));
  vec_COUNT(v) = old_count + n; // NOLINT(clang-analyzer-security.ArrayBound)
}

#define RK__vec_insert_arr_at_u(V, I, O, N)                                                        \
  RK__vec_insert_arr_at_f(sizeof(*(V)), (V), RK__vec_assert_insertbounds(V, I), (O), (N))
#define RK__vec_insert_at_u(V, I, O) RK__vec_insert_arr_at_u(V, I, ((typeof (*(V))[1]){(O)}), 1)

#define RK__vec_insert_at(V, I, O)   (RK__vec_reserve_1(V), RK__vec_insert_at_u(V, I, O))

#define RK__vec_insert_arr_at(V, I, O, N)                                                          \
  ((void)((N) && (RK__vec_reserve(V, vec_count(V) + (N)), RK__vec_insert_arr_at_u(V, I, O, N), 1)))

static_fun rk_forceinline void RK__vec_insert_at_unordered_f(size_t elsize, void* restrict vec,
                                                             size_t i, const void* restrict o) {
  size_t count = vec_COUNT(vec); // NOLINT(clang-analyzer-security.ArrayBound)
  char*  dst   = (char*)vec + rk_mult(elsize, count);
  if (i < count) {
    char* src = (char*)vec + rk_mult(elsize, i);
    memcpy(dst, src, elsize);
    memcpy(src, o, elsize);
  } else {
    memcpy(dst, o, elsize);
  }
  ++vec_COUNT(vec); // NOLINT(clang-analyzer-security.ArrayBound)
}

#define RK__vec_insert_at_unordered(V, I, O)                                                       \
  (RK__vec_reserve_1(V),                                                                           \
   RK__vec_insert_at_unordered_f(sizeof(*(V)), V, RK__vec_assert_insertbounds(V, I),               \
                                 ((typeof (*(V))[1]){(O)})))

static_fun rk_forceinline void RK__vec_erase_at_n_f(size_t elsize, void* v, size_t i, size_t n) {
  if (!n) { return; }
  char *dst = (char*)v + rk_mult(i, elsize), *src = dst + rk_mult(n, elsize);
  memmove(dst, src,
          rk_mult(((vec_COUNT(v) -= n) - i),
                  elsize)); // NOLINT(clang-analyzer-security.ArrayBound)
}
#define RK__vec_erase_at_n(V, I, N)                                                                \
  RK__vec_erase_at_n_f(sizeof(*(V)), (V), RK__vec_assert_erasebounds_n(V, I, N), (N))
#define RK__vec_erase_at(V, I) RK__vec_erase_at_n(V, I, 1)

#define RK__vec_assign(V, O, N)                                                                    \
  (RK__vec_reserve(V, N), (V) && (vec_COUNT(V) = (N), rk_copy(V, O, N)))

// msvc sizeof returns 0
#define RK__vec_init_list_(T, arr, alloc)                                                          \
  memcpy(RK__VEC_NEW_NONZERO(T, rk_COUNTOF(arr), rk_COUNTOF(arr), alloc), arr, sizeof(arr))

#define RK__VEC_contrav(T, x) _Generic(x, T: x, Allocator: (T){RK_ZINIT})

#if RK_CUSTOM_ALLOCATORS
# define RK__vec_init_list(T, ...)                                                                 \
   _Generic(VA_FIRST(__VA_ARGS__),                                                                 \
       Allocator: RK__vec_init_list_(T, ((const T[]){VA_REST(__VA_ARGS__)}),                       \
                                     RK__contrav(Allocator, VA_FIRST(__VA_ARGS__))),               \
       default: RK__vec_init_list_(                                                                \
                T, ((const T[]){RK__VEC_contrav(T, VA_FIRST(__VA_ARGS__)), VA_REST(__VA_ARGS__)}), \
                alloc_ctx))
#else
# define RK__vec_init_list(T, ...)                                                                 \
   RK__vec_init_list_(                                                                             \
       T, ((const T[]){RK__VEC_contrav(T, VA_FIRST(__VA_ARGS__)), VA_REST(__VA_ARGS__)}), 0)
#endif

/// @endcond

RK_HEADER_END
/// @}
#endif // RK_VEC_H

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
/* END INLINE: include/rk_vec.h */
/* inlined from include/rklib.h:12: #include "rk_heap.h" */
/* BEGIN INLINE: include/rk_heap.h */
// SPDX-License-Identifier: MIT
/// @file rk_heap.h
/// @version 1.0.0
/// @defgroup rk_heap Heap (Binary Min-Heap) Interface
/// @brief Type-safe binary min-heap backed by rk_vec.h.
///
/// Define a specialization once at file scope with `HEAP_DEFINE(T, CMP_FUN)`. `CMP_FUN` has the
/// same convention as the tree comparators: `int cmp(T a, T b)` returns negative, zero, or
/// positive. The smallest value is at the root. To make a max-heap, supply a comparator with the
/// opposite ordering.
///
/// Usage:
/// ```c
/// static int int_cmp(int a, int b) { return (a > b) - (a < b); }
/// HEAP_DEFINE(int, int_cmp);
/// Heap(int) h = heap_init(int, 0);
/// heap_push(int, &h, 4);
/// heap_push(int, &h, 2);
/// int smallest = heap_pop(int, &h); // 2
/// heap_release(&h);
/// ```
///
/// Push and pop take O(log n); peek takes O(1). Equal values are allowed. The order among equal
/// values is unspecified. As with `Vec`, pointers into the heap's data may be invalidated by
/// insertion or removal.
///
/// If all values are known up front, building the Heap in O(n) overall beats n separate
/// O(log n) `heap_push()` calls. Four entry points cover this, differing in whether they allocate
/// fresh storage or reuse existing storage, and whether prior contents are kept or discarded:
/// - `heap_from(T, arr, n, alloc?)` - fresh Heap, copying `arr`.
/// - `heap_adopt(T, vec)` - fresh Heap, taking ownership of an existing `Vec(T)` with no copy.
/// - `heap_assign(T, self, arr, n)` - replaces an existing Heap's contents with `arr`, reusing its
///   backing Vec.
/// - `heap_extend(T, self, arr, n)` - appends `arr` to an existing Heap's current contents, reusing
///   its backing Vec.
///
/// `heap_replace_top()` combines a pop and a push into a single sift-down; prefer it over a
/// separate `heap_pop()`/`heap_push()` pair when repeatedly replacing the minimum (e.g. k-way
/// merges, running top-k selection).
/// @see rk_vec.h
/// @{

#ifndef RK_HEAP_H
#define RK_HEAP_H
/* inlined from include/rk_heap.h:45: #include "rk_vec.h" */
/* skipped already-included: "include/rk_vec.h" */
RK_HEADER_BEGIN

/// @brief Define a heap type and functions for a given element type.
///
/// This macro generates a complete, type-specific binary min-heap API for the given element type.
///
/// @param T       Name of the element type. Must be an identifier; use a typedef for a pointer or
///                struct type.
/// @param CMP_FUN Comparison function (`int CMP_FUN(T a, T b)`), returning negative, zero, or
///                positive, following the same convention as `strcmp` and the tree comparators.
/// @attention Must be invoked at file scope, once per `T`.
#define HEAP_DEFINE(T, CMP_FUN)          RK__HEAP_DEFINE(T, CMP_FUN)

/// @brief Macro to indicate that an object is a Heap.
/// @param T The type of elements stored in the Heap
#define Heap(T)                          rk_heap_##T

/// @brief `Heap(T) heap_init(T, size_t cap, Allocator alloc = alloc_ctx)` - Initialises and
/// returns an empty Heap.
/// @param T        The desired type of the Heap's elements
/// @param cap      The initial capacity of the backing Vec (in elements)
/// @param alloc    Optional allocator; defaults to `alloc_ctx`. See `vec_init()`.
/// @return An initialised, empty `Heap(T)`
/// @note A zero-initialized `Heap(T)` is also a valid, empty heap.
#define heap_init(T, cap, ...)           ((Heap(T)){.data = vec_init(T, cap, ##__VA_ARGS__)})

/// @brief `Heap(T) heap_from(T, const T* arr, size_t n, Allocator alloc = alloc_ctx)` - Constructs
/// a new Heap by copying `n` values from `arr` and heapifying them, in O(n) overall.
/// @param T     Element type
/// @param arr   Source array of `n` values
/// @param n     Number of values to copy
/// @param alloc Optional allocator; defaults to `alloc_ctx`
/// @return A new `Heap(T)` containing a heap-ordered copy of `arr`'s first `n` values
#define heap_from(T, arr, n, ...)        rk_overload(RK__HEAP_FROM, T, arr, n, ##__VA_ARGS__)

/// @brief `Heap(T) heap_adopt(T, Vec(T) vec)` - Constructs a new Heap by taking ownership of `vec`
/// and heapifying it in place, with no copy.
/// @param T   Element type
/// @param vec An existing `Vec(T)`, passed by value
/// @return A `Heap(T)` wrapping `vec`'s own storage, now in heap order
/// @attention `vec` is consumed: its storage now belongs to the returned Heap. Do not read, mutate,
/// or `vec_release()` the original `vec` variable afterwards; release the Heap instead.
#define heap_adopt(T, vec)               RK__HEAP_F(T, adopt)(vec)

/// @brief `void heap_release(Heap(T)* self)` - Frees the backing Vec and resets the Heap to an
/// empty state.
#define heap_release(self)               vec_release((self)->data)

/// @brief `size_t heap_count(Heap(T)* self)` - Returns the number of elements stored in the Heap.
#define heap_count(self)                 vec_count((self)->data)

/// @brief `size_t heap_cap(Heap(T)* self)` - Returns the current capacity of the backing Vec.
#define heap_cap(self)                   vec_cap((self)->data)

/// @brief `Allocator heap_allocator(Heap(T)* self)` - Returns the Allocator the Heap's backing Vec
/// was constructed with.
#define heap_allocator(self)             vec_allocator((self)->data)

/// @brief `bool heap_is_empty(Heap(T)* self)` - Returns `true` iff the Heap contains no elements.
#define heap_is_empty(self)              (heap_count(self) == 0)

/// @brief `void heap_clear(Heap(T)* self)` - Removes all elements without freeing the backing Vec.
#define heap_clear(self)                 vec_clear((self)->data)

/// @brief `void heap_reserve(Heap(T)* self, size_t cap)` - Ensures the backing Vec can hold at
/// least `cap` elements without reallocating.
/// @param cap Minimum capacity to reserve (in elements)
/// @attention **Arguments with side effects are not safe in `vec_`-backed macros**
#define heap_reserve(self, cap)          vec_reserve((self)->data, cap)

/// @brief `void heap_shrink_to_fit(Heap(T)* self)` - Shrinks the backing Vec's capacity to the next
/// power of two greater than or equal to its length (matching `vec_shrink_to_fit()`'s convention).
/// @attention **Arguments with side effects are not safe in `vec_`-backed macros**
/// @note Safe to call at any time: shrinking never touches element order, so the heap invariant is
/// unaffected. Frees the backing Vec entirely if the Heap is empty.
#define heap_shrink_to_fit(self)         vec_shrink_to_fit((self)->data)

/// @brief `void heap_assign(T, Heap(T)* self, const T* arr, size_t n)` - Replaces the Heap's
/// contents with a heap-ordered copy of `arr`'s first `n` values, reusing the existing backing
/// Vec's buffer (growing it if necessary) rather than allocating a new one.
/// @param T   Element type
/// @param arr Source array of `n` values
/// @param n   Number of values to copy
/// @attention `arr[0..n)` must not overlap the Heap's own backing allocation: if growth is
/// triggered, the old buffer is freed before the copy from `arr` happens, turning an `arr` that
/// points into it into a use-after-free; even without growth, the underlying copy is a plain
/// `memcpy`, which is undefined for overlapping source and destination.
#define heap_assign(T, self, arr, n)     RK__HEAP_F(T, assign)((self), (arr), (n))

/// @brief `const T* heap_peek(T, const Heap(T)* self)` - Returns a pointer to the minimum element
/// without removing it.
/// @param T Element type
/// @return Pointer to the minimum element, or `NULL` if the Heap is empty
/// @note Invalidated by any later mutation of the Heap.
#define heap_peek(T, self)               RK__HEAP_F(T, peek)(self)

/// @brief `void heap_push(T, Heap(T)* self, T value)` - Inserts `value` into the Heap.
/// @param T     Element type
/// @param value Value to insert. Evaluated once.
/// @note A push may reallocate the backing Vec, invalidating prior pointers into it.
#define heap_push(T, self, value)        RK__HEAP_F(T, push)(self, value)

/// @brief `T heap_pop(T, Heap(T)* self)` - Removes and returns the minimum element.
/// @param T Element type
/// @return The (former) minimum element
/// @attention Requires a nonempty Heap.
#define heap_pop(T, self)                RK__HEAP_F(T, pop)(self)

/// @brief `bool heap_try_pop(T, Heap(T)* self, T* out)` - Removes the minimum element and writes
/// it to `*out`, if the Heap is nonempty.
/// @param T   Element type
/// @param out Destination for the removed value. Left untouched if the Heap is empty.
/// @return `true` if an element was removed, `false` if the Heap was empty
#define heap_try_pop(T, self, out)       RK__HEAP_F(T, try_pop)(self, out)

/// @brief `T heap_replace_top(T, Heap(T)* self, T value)` - Removes the minimum element and
/// inserts `value`, in a single sift-down.
/// @param T     Element type
/// @param value Value to insert in place of the removed minimum. Evaluated once.
/// @return The (former) minimum element
/// @attention Requires a nonempty Heap.
/// @note Equivalent to, but cheaper than, `heap_pop()` followed by `heap_push()`: it never shrinks
/// or reallocates the backing Vec.
#define heap_replace_top(T, self, value) RK__HEAP_F(T, replace_top)(self, value)

/// @brief `void heap_extend(T, Heap(T)* self, const T* arr, size_t n)` - Appends `arr`'s first `n`
/// values to the Heap's existing contents, then re-heapifies the combined set in O(count + n).
/// @param T   Element type
/// @param arr Source array of `n` values
/// @param n   Number of values to append
/// @note Prefer this over `n` individual `heap_push()` calls when `n` is a significant fraction of
/// the Heap's existing size; for a handful of new elements into an already-large Heap, looped
/// `heap_push()` (O(n log count)) stays cheaper than re-heapifying everything (O(count + n)).
/// @attention `arr[0..n)` must not overlap the Heap's own backing allocation, for the same reasons
/// documented on `heap_assign()`.
#define heap_extend(T, self, arr, n)     RK__HEAP_F(T, extend)((self), (arr), (n))

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

#define RK__HEAP_F(T, NAME)              rk_heapf_##NAME##_##T

#define RK__HEAP_DEFINE(T, CMP_FUN)                                                                \
  RK_EXTERNC_BEG                                                                                   \
  typedef struct Heap(T) { Vec(T) data; } Heap(T);                                                 \
  static_fun const T* RK__HEAP_F(T, peek)(const Heap(T) * self) {                                  \
    return vec_count(self->data) ? &self->data[0] : rk_null;                                       \
  }                                                                                                \
  static_fun void RK__HEAP_F(T, sift_down)(Heap(T) * self, size_t i, T value, size_t n) {          \
    while (i < n / 2) {                                                                            \
      size_t child = 2 * i + 1;                                                                    \
      if (child + 1 < n && CMP_FUN(self->data[child + 1], self->data[child]) < 0) { ++child; }     \
      if (CMP_FUN(value, self->data[child]) <= 0) { break; }                                       \
      self->data[i] = self->data[child];                                                           \
      i             = child;                                                                       \
    }                                                                                              \
    self->data[i] = value;                                                                         \
  }                                                                                                \
  static_fun void RK__HEAP_F(T, heapify)(Heap(T) * self) {                                         \
    const size_t n = vec_count(self->data);                                                        \
    for (size_t i = n / 2; i > 0;) {                                                               \
      --i;                                                                                         \
      RK__HEAP_F(T, sift_down)(self, i, self->data[i], n);                                         \
    }                                                                                              \
  }                                                                                                \
  static_fun Heap(T) RK__HEAP_F(T, from)(const T* arr, size_t n RK_IFALLOC(, Allocator alloc)) {   \
    Heap(T) h = heap_init(T, n RK_IFALLOC(, alloc));                                               \
    vec_push_n(h.data, arr, n);                                                                    \
    RK__HEAP_F(T, heapify)(&h);                                                                    \
    return h;                                                                                      \
  }                                                                                                \
  static_fun Heap(T) RK__HEAP_F(T, adopt)(Vec(T) vec) {                                            \
    Heap(T) h = {vec};                                                                             \
    RK__HEAP_F(T, heapify)(&h);                                                                    \
    return h;                                                                                      \
  }                                                                                                \
  static_fun void RK__HEAP_F(T, assign)(Heap(T) * self, const T* arr, size_t n) {                  \
    vec_assign(self->data, arr, n);                                                                \
    RK__HEAP_F(T, heapify)(self);                                                                  \
  }                                                                                                \
  static_fun void RK__HEAP_F(T, extend)(Heap(T) * self, const T* arr, size_t n) {                  \
    vec_push_n(self->data, arr, n);                                                                \
    RK__HEAP_F(T, heapify)(self);                                                                  \
  }                                                                                                \
  static_fun void RK__HEAP_F(T, push)(Heap(T) * self, T value) {                                   \
    vec_push(self->data, value);                                                                   \
    size_t i = vec_count(self->data) - 1;                                                          \
    while (i > 0) {                                                                                \
      size_t parent = (i - 1) / 2;                                                                 \
      if (CMP_FUN(value, self->data[parent]) >= 0) { break; }                                      \
      self->data[i] = self->data[parent];                                                          \
      i             = parent;                                                                      \
    }                                                                                              \
    self->data[i] = value;                                                                         \
  }                                                                                                \
  static_fun T RK__HEAP_F(T, pop)(Heap(T) * self) {                                                \
    rk_assert(vec_count(self->data) && "Cannot pop an empty heap");                                \
    const T      result = self->data[0], last = vec_pop(self->data);                               \
    const size_t n = vec_count(self->data);                                                        \
    if (n) { RK__HEAP_F(T, sift_down)(self, 0, last, n); }                                         \
    return result;                                                                                 \
  }                                                                                                \
  static_fun bool RK__HEAP_F(T, try_pop)(Heap(T) * self, T * out) {                                \
    if (!vec_count(self->data)) { return false; }                                                  \
    return *out = RK__HEAP_F(T, pop)(self), true;                                                  \
  }                                                                                                \
  static_fun T RK__HEAP_F(T, replace_top)(Heap(T) * self, T value) {                               \
    rk_assert(vec_count(self->data) && "Cannot replace_top an empty heap");                        \
    const T result = self->data[0];                                                                \
    RK__HEAP_F(T, sift_down)(self, 0, value, vec_count(self->data));                               \
    return result;                                                                                 \
  }                                                                                                \
  RK_EXTERNC_END

#define RK__HEAP_FROM(T, arr, n, alloc)  RK__HEAP_F(T, from)((arr), (n)RK_IFALLOC(, (alloc)))
#define RK__HEAP_FROM4(T, arr, n, alloc) rk_disable_if(RK__HEAP_FROM(T, arr, n, alloc))
#define RK__HEAP_FROM3(T, arr, n)        RK__HEAP_FROM(T, arr, n, alloc_ctx)

RK_HEADER_END

/// @}
#endif // RK_HEAP_H

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
/* END INLINE: include/rk_heap.h */
/* inlined from include/rklib.h:13: #include "rk_deque.h" */
/* BEGIN INLINE: include/rk_deque.h */
// SPDX-License-Identifier: MIT
/// @file rk_deque.h
/// @version 1.0.0
/// @defgroup rk_deque Deque (Double-Ended Queue) Interface
/// @brief Type-specific circular-buffer deque with amortized O(1) insertion and removal at either
/// end.
///
/// Define a specialization once at file scope with `DEQUE_DEFINE(T)`.
///
/// Usage:
/// ```c
/// DEQUE_DEFINE(int);
/// Deque(int) q = deque_init(int, 0); // optional initial capacity and allocator
/// deque_push_back(int, &q, 1);
/// deque_push_front(int, &q, 2);
/// int first = deque_pop_front(int, &q); // 2
/// deque_release(int, &q);
/// ```
///
/// Indices count from the front, not from the beginning of the allocation: the backing array is a
/// power-of-two-sized ring buffer, so its physical order does not necessarily match logical order
/// once it has wrapped. As with `Vec`, insertion or `deque_reserve()` can invalidate pointers into
/// the Deque.
/// @note If only one end is ever pushed/popped, `Vec` is simpler and has no head/wraparound
/// bookkeeping at all; reach for `Deque` specifically when both ends are needed.
/// @see rk_vec.h
/// @{
#ifndef RK_DEQUE_H
#define RK_DEQUE_H
/* inlined from include/rk_deque.h:30: #include "rk_alloc.h" */
/* skipped already-included: "include/rk_alloc.h" */
RK_HEADER_BEGIN

/// @brief Generates a deque type and its operations for T. Invoke once per T at file scope.
/// @param T Name of the element type. Must be a plain type identifier; use a typedef for a pointer
/// or struct type.
/// @attention Must be invoked at file scope, once per `T`.
/// @note Allocator functions handle allocation failures according to `rk_alloc.h`.
#define DEQUE_DEFINE(T)            RK__DEQUE_DEFINE(T)

/// @brief Macro to indicate that an object is a Deque.
/// @param T The type of elements stored in the Deque
#define Deque(T)                   rk_deque_##T

/// @brief `Deque(T) deque_init(T, size_t capacity, Allocator alloc = alloc_ctx)` - Initialises and
/// returns an empty Deque.
/// @param T        The desired type of the Deque's elements
/// @param capacity The initial capacity of the backing buffer (in elements)
/// @param alloc    Optional allocator; defaults to `alloc_ctx`
/// @return An initialised, empty `Deque(T)`
/// @note A zero-initialized `Deque(T)` is also a valid, empty deque; it allocates using `alloc_ctx`
/// on first insertion.
#define deque_init(T, cap, ...)    rk_overload(RK__DEQUE_INIT, T, cap, ##__VA_ARGS__)

/// @brief `Deque(T) deque_from(T, const T* arr, size_t n, Allocator alloc = alloc_ctx)` -
/// Constructs a new Deque by copying `n` values from `arr`, in front-to-back order.
/// @param T     Element type
/// @param arr   Source array of `n` values
/// @param n     Number of values to copy
/// @param alloc Optional allocator; defaults to `alloc_ctx`
/// @return A new `Deque(T)` containing a copy of `arr`'s first `n` values
#define deque_from(T, arr, n, ...) rk_overload(RK__DEQUE_FROM, T, arr, n, ##__VA_ARGS__)

/// @brief `void deque_release(T, Deque(T)* self)` - Frees the backing buffer and resets the Deque
/// to an empty state.
/// @param T Element type
/// @note Safe to call on a zero-initialized Deque.
#define deque_release(T, self)     RK__DEQUE_F(T, release)(self)

/// @brief `size_t deque_count(Deque(T)* self)` - Returns the number of elements stored in the
/// Deque.
#define deque_count(self)          ((size_t)(self)->count)

/// @brief `size_t deque_cap(Deque(T)* self)` - Returns the current capacity of the backing buffer.
/// Always a power of two (or zero).
#define deque_cap(self)            ((size_t)(self)->cap)

#if RK_CUSTOM_ALLOCATORS
/// @brief `Allocator deque_allocator(Deque(T)* self)` - Returns the Allocator the Deque was
/// constructed with.
# define deque_allocator(self) rk_to_rvalue((self)->alloc)
#else
/// @brief `Allocator deque_allocator(Deque(T)* self)` - Returns `alloc_ctx` (allocators disabled).
# define deque_allocator(self) ((void)(self), alloc_ctx)
#endif

/// @brief `bool deque_is_empty(Deque(T)* self)` - Returns `true` iff the Deque contains no
/// elements.
#define deque_is_empty(self)          (deque_count(self) == 0)

/// @brief `void deque_clear(T, Deque(T)* self)` - Removes all elements without freeing the backing
/// buffer.
/// @param T Element type
#define deque_clear(T, self)          RK__DEQUE_F(T, clear)(self)

/// @brief `void deque_reserve(T, Deque(T)* self, size_t cap)` - Ensures the backing buffer holds at
/// least `cap` elements without reallocating.
/// @param T   Element type
/// @param cap Minimum capacity to reserve (in elements)
/// @note Existing elements retain their logical order.
#define deque_reserve(T, self, cap)   RK__DEQUE_F(T, reserve)((self), (cap))

/// @brief `void deque_shrink_to_fit(T, Deque(T)* self)` - Shrinks the Deque's capacity to the next
/// power of two greater than or equal to its length, with a floor of 8 for a nonempty Deque
/// (matching the same minimum `deque_init()`/`deque_reserve()` enforce), leaving contents
/// unchanged.
/// @param T Element type
/// @note Frees the backing buffer entirely if the Deque is empty.
#define deque_shrink_to_fit(T, self)  RK__DEQUE_F(T, shrink_to_fit)(self)

/// @brief `void deque_assign(T, Deque(T)* self, const T* arr, size_t n)` - Replaces the Deque's
/// contents with a copy of `arr`'s first `n` values, reusing the existing backing buffer (growing
/// it if necessary) rather than allocating a new one.
/// @param T   Element type
/// @param arr Source array of `n` values
/// @param n   Number of values to copy
/// @attention `arr[0..n)` must not overlap the Deque's own backing allocation, for the same reasons
/// documented on `deque_push_back_n()`.
#define deque_assign(T, self, arr, n) RK__DEQUE_F(T, assign)((self), (arr), (n))

/// @brief Returns the first element as an lvalue, mutable for `Deque(T)* self` and const for
/// `const Deque(T)* self`. Like `vec_front()`, requires a nonempty Deque.
/// @param T Element type
/// @return Lvalue for the first element
/// @attention Requires a nonempty Deque; use `deque_peek_front()` to check safely.
/// @note Invalidated by any later mutation of the Deque.
#define deque_front(T, self)                                                                       \
  (*_Generic((self),                                                                               \
       const Deque(T)*: RK__DEQUE_F(T, front_const),                                               \
       default: RK__DEQUE_F(T, front))(self))

/// @brief Returns the last element as an lvalue, mutable for `Deque(T)* self` and const for
/// `const Deque(T)* self`. Like `vec_back()`, requires a nonempty Deque.
/// @param T Element type
/// @return Lvalue for the last element
/// @attention Requires a nonempty Deque; use `deque_peek_back()` to check safely.
/// @note Invalidated by any later mutation of the Deque.
#define deque_back(T, self)                                                                        \
  (*_Generic((self), const Deque(T)*: RK__DEQUE_F(T, back_const), default: RK__DEQUE_F(T, back))(  \
      self))

/// @brief Returns a pointer to the element at the zero-based logical index (counting from the
/// front): `T*` for `Deque(T)* self`, `const T*` for `const Deque(T)* self`.
/// @param T     Element type
/// @param index Zero-based logical index
/// @return Pointer to the element, or `NULL` if `index` is out of bounds
/// @note Invalidated by any later mutation of the Deque.
#define deque_at(T, self, index)                                                                   \
  _Generic((self), const Deque(T)*: RK__DEQUE_F(T, at_const), default: RK__DEQUE_F(T, at))(        \
      (self), (index))

/// @brief Returns a pointer to the first element: `T*` for `Deque(T)* self`, `const T*` for
/// `const Deque(T)* self`.
/// @param T Element type
/// @return Pointer to the first element, or `NULL` if the Deque is empty
/// @note Invalidated by any later mutation of the Deque.
#define deque_peek_front(T, self)                                                                  \
  _Generic((self),                                                                                 \
      const Deque(T)*: RK__DEQUE_F(T, peek_front_const),                                           \
      default: RK__DEQUE_F(T, peek_front))(self)

/// @brief Returns a pointer to the last element: `T*` for `Deque(T)* self`, `const T*` for
/// `const Deque(T)* self`.
/// @param T Element type
/// @return Pointer to the last element, or `NULL` if the Deque is empty
/// @note Invalidated by any later mutation of the Deque.
#define deque_peek_back(T, self)                                                                   \
  _Generic((self),                                                                                 \
      const Deque(T)*: RK__DEQUE_F(T, peek_back_const),                                            \
      default: RK__DEQUE_F(T, peek_back))(self)

/// @brief `void deque_push_front(T, Deque(T)* self, T value)` - Inserts `value` at the front of the
/// Deque.
/// @param T     Element type
/// @param value Value to insert. Evaluated once.
/// @note May reallocate the backing buffer, invalidating prior pointers into it.
#define deque_push_front(T, self, value)        RK__DEQUE_F(T, push_front)((self), (value))

/// @brief `void deque_push_front_n(T, Deque(T)* self, const T* arr, size_t count)` - Prepends
/// `count` values from `arr` to the front of the Deque, preserving `arr`'s own order (`arr[0]`
/// becomes the new first element).
/// @param T     Element type
/// @param arr   Source array of `count` values
/// @param count Number of values to prepend
/// @note This is NOT equivalent to `count` individual `deque_push_front()` calls, which would
/// insert them in reverse order; `arr`'s order is preserved instead, matching
/// `vec_insert_arr_at()`'s convention. Reserves once and copies in at most two segments, instead of
/// re-checking capacity per element.
/// @note May reallocate the backing buffer, invalidating prior pointers into it.
/// @attention `arr[0..count)` must not overlap the Deque's own backing allocation. If growth is
/// triggered, the old buffer is freed before the copy from `arr` happens, turning an `arr` that
/// points into it into a use-after-free; even without growth, the underlying copy is a plain
/// `memcpy`, which is undefined for overlapping source and destination. To insert elements taken
/// from the same Deque, copy them into a temporary buffer first.
#define deque_push_front_n(T, self, arr, count) RK__DEQUE_F(T, push_front_n)((self), (arr), (count))

/// @brief `void deque_push_back(T, Deque(T)* self, T value)` - Inserts `value` at the back of the
/// Deque.
/// @param T     Element type
/// @param value Value to insert. Evaluated once.
/// @note May reallocate the backing buffer, invalidating prior pointers into it.
#define deque_push_back(T, self, value)         RK__DEQUE_F(T, push_back)((self), (value))

/// @brief `void deque_push_back_n(T, Deque(T)* self, const T* arr, size_t count)` - Appends `count`
/// values from `arr` to the back of the Deque, in order, as a single bulk operation.
/// @param T     Element type
/// @param arr   Source array of `count` values
/// @param count Number of values to append
/// @note Reserves once and copies in at most two segments, instead of re-checking capacity per
/// element like `count` individual `deque_push_back()` calls would.
/// @note May reallocate the backing buffer, invalidating prior pointers into it.
/// @attention `arr[0..count)` must not overlap the Deque's own backing allocation, for the same
/// reasons documented on `deque_push_front_n()`.
#define deque_push_back_n(T, self, arr, count)  RK__DEQUE_F(T, push_back_n)((self), (arr), (count))

/// @brief `T deque_pop_front(T, Deque(T)* self)` - Removes and returns the first element.
/// @param T Element type
/// @return The (former) first element
/// @attention Requires a nonempty Deque.
#define deque_pop_front(T, self)                RK__DEQUE_F(T, pop_front)(self)

/// @brief `T deque_pop_back(T, Deque(T)* self)` - Removes and returns the last element.
/// @param T Element type
/// @return The (former) last element
/// @attention Requires a nonempty Deque.
#define deque_pop_back(T, self)                 RK__DEQUE_F(T, pop_back)(self)

/// @brief `bool deque_try_pop_front(T, Deque(T)* self, T* out)` - Removes the first element and
/// writes it to `*out`, if the Deque is nonempty.
/// @param T   Element type
/// @param out Destination for the removed value. Left untouched if the Deque is empty.
/// @return `true` if an element was removed, `false` if the Deque was empty
#define deque_try_pop_front(T, self, out)       RK__DEQUE_F(T, try_pop_front)((self), (out))

/// @brief `bool deque_try_pop_back(T, Deque(T)* self, T* out)` - Removes the last element and
/// writes it to `*out`, if the Deque is nonempty.
/// @param T   Element type
/// @param out Destination for the removed value. Left untouched if the Deque is empty.
/// @return `true` if an element was removed, `false` if the Deque was empty
#define deque_try_pop_back(T, self, out)        RK__DEQUE_F(T, try_pop_back)((self), (out))

/// @brief Visits every element of a Deque in front-to-back order.
/// @param self The Deque to loop over (a pointer). Evaluated once.
/// @param it   The name of the iterator (pointer to each element, const if `self` points to a
///             const Deque)
/// @note Do not push, pop, reserve, or shrink the Deque while looping in this fashion.
///
/// Usage:
/// ```c
/// deque_foreach(&q, it) { printf("%d\n", *it); }
/// ```
#define deque_foreach(self, it)                                                                    \
  for (typeof(self) RK___DEQUE = (self); RK___DEQUE; RK___DEQUE = rk_null)                         \
    for (size_t RK___i = 0; RK___i < RK___DEQUE->count; ++RK___i)                                  \
      for (typeof(RK__DEQUE_ITER_PTR(RK___DEQUE)) it                                               \
           = &RK___DEQUE->data[(RK___DEQUE->head + RK___i) & (RK___DEQUE->cap - 1)],               \
           RK___once            = it;                                                              \
           RK___once; RK___once = rk_null)

/// @brief Like `deque_foreach()`, visiting elements in back-to-front order. Iterator element
/// constness follows the constness of the Deque pointed to by `self`.
#define deque_foreach_reversed(self, it)                                                           \
  for (typeof(self) RK___DEQUE = (self); RK___DEQUE; RK___DEQUE = rk_null)                         \
    for (size_t RK___i = RK___DEQUE->count; RK___i-- > 0;)                                         \
      for (typeof(RK__DEQUE_ITER_PTR(RK___DEQUE)) it                                               \
           = &RK___DEQUE->data[(RK___DEQUE->head + RK___i) & (RK___DEQUE->cap - 1)],               \
           RK___once            = it;                                                              \
           RK___once; RK___once = rk_null)

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL
#define RK__DEQUE_F(T, NAME) rk_dequef_##NAME##_##T
// The member `data` is a mutable pointer even when the Deque is const; propagate the
// container's constness explicitly when choosing an iterator pointer type.
#define RK__DEQUE_ITER_PTR(self)                                                                   \
  _Generic((self),                                                                                 \
      const typeof(*(self))*: (const typeof((self)->data[0])*)0,                                   \
      default: (typeof((self)->data))0)
#define RK__DEQUE_DEFINE(T)                                                                        \
  RK_EXTERNC_BEG                                                                                   \
  typedef struct Deque(T) {                                                                        \
    T*     data;                                                                                   \
    size_t head, count, cap;                                                                       \
    RK_IFALLOC(Allocator alloc;)                                                                   \
  } Deque(T);                                                                                      \
  static_fun Deque(T) RK__DEQUE_F(T, init)(size_t cap RK_IFALLOC(, Allocator alloc)) {             \
    RK_IFALLOC(rk_assert_allocator_valid(alloc);)                                                  \
    Deque(T) result = {rk_null, 0, 0, 0 RK_IFALLOC(, alloc)};                                      \
    if (cap) {                                                                                     \
      cap = stdc_bit_ceil(rk_MAX((size_t)8, cap));                                                 \
      rk_assert(cap && "Deque capacity overflow");                                                 \
      result.data = alloc_new(T, cap RK_IFALLOC(, alloc));                                         \
      result.cap  = cap;                                                                           \
    }                                                                                              \
    return result;                                                                                 \
  }                                                                                                \
  static_fun void RK__DEQUE_F(T, release)(Deque(T) * self) {                                       \
    if (self->data) { alloc_delete(self->data, self->cap RK_IFALLOC(, self->alloc)); }             \
    self->data = rk_null, self->head = self->count = self->cap = 0;                                \
  }                                                                                                \
  static_fun void RK__DEQUE_F(T, clear)(Deque(T) * self) { self->head = self->count = 0; }         \
  static_fun void RK__DEQUE_F(T, realloc_to)(Deque(T) * self, size_t new_cap) {                    \
    T* data = alloc_new(T, new_cap RK_IFALLOC(, self->alloc));                                     \
    if (self->count) {                                                                             \
      size_t first = self->count < self->cap - self->head ? self->count : self->cap - self->head;  \
      rk_memcpy(data, self->data + self->head, sizeof_n(T, first));                                \
      if (first < self->count) {                                                                   \
        rk_memcpy(data + first, self->data, sizeof_n(T, self->count - first));                     \
      }                                                                                            \
    }                                                                                              \
    if (self->data) { alloc_delete(self->data, self->cap RK_IFALLOC(, self->alloc)); }             \
    self->data = data, self->head = 0, self->cap = new_cap;                                        \
  }                                                                                                \
  static_fun void RK__DEQUE_F(T, reserve)(Deque(T) * self, size_t requested) {                     \
    if (requested <= self->cap) { return; }                                                        \
    size_t cap = stdc_bit_ceil(rk_MAX((size_t)8, requested));                                      \
    rk_assert(cap && "Deque capacity overflow");                                                   \
    RK_IFALLOC(rk_set_alloc_fallback(self->alloc);)                                                \
    RK__DEQUE_F(T, realloc_to)(self, cap);                                                         \
  }                                                                                                \
  static_fun void RK__DEQUE_F(T, shrink_to_fit)(Deque(T) * self) {                                 \
    if (!self->count) {                                                                            \
      RK__DEQUE_F(T, release)(self);                                                               \
      return;                                                                                      \
    }                                                                                              \
    size_t cap = stdc_bit_ceil(rk_MAX((size_t)8, self->count));                                    \
    if (cap == self->cap) { return; }                                                              \
    RK_IFALLOC(rk_set_alloc_fallback(self->alloc);)                                                \
    RK__DEQUE_F(T, realloc_to)(self, cap);                                                         \
  }                                                                                                \
  static_fun T* RK__DEQUE_F(T, at)(Deque(T) * self, size_t i) {                                    \
    return i < self->count ? &self->data[(self->head + i) & (self->cap - 1)] : rk_null;            \
  }                                                                                                \
  static_fun const T* RK__DEQUE_F(T, at_const)(const Deque(T) * self, size_t i) {                  \
    return i < self->count ? &self->data[(self->head + i) & (self->cap - 1)] : rk_null;            \
  }                                                                                                \
  static_fun T* RK__DEQUE_F(T, peek_front)(Deque(T) * self) {                                      \
    return RK__DEQUE_F(T, at)(self, 0);                                                            \
  }                                                                                                \
  static_fun const T* RK__DEQUE_F(T, peek_front_const)(const Deque(T) * self) {                    \
    return RK__DEQUE_F(T, at_const)(self, 0);                                                      \
  }                                                                                                \
  static_fun T* RK__DEQUE_F(T, peek_back)(Deque(T) * self) {                                       \
    return self->count ? RK__DEQUE_F(T, at)(self, self->count - 1) : rk_null;                      \
  }                                                                                                \
  static_fun T const* RK__DEQUE_F(T, peek_back_const)(const Deque(T) * self) {                     \
    return self->count ? RK__DEQUE_F(T, at_const)(self, self->count - 1) : rk_null;                \
  }                                                                                                \
  static_fun T* RK__DEQUE_F(T, front)(Deque(T) * self) {                                           \
    rk_assert(self->count && "Cannot access front of empty deque");                                \
    return RK__DEQUE_F(T, at)(self, 0);                                                            \
  }                                                                                                \
  static_fun const T* RK__DEQUE_F(T, front_const)(const Deque(T) * self) {                         \
    rk_assert(self->count && "Cannot access front of empty deque");                                \
    return RK__DEQUE_F(T, at_const)(self, 0);                                                      \
  }                                                                                                \
  static_fun T* RK__DEQUE_F(T, back)(Deque(T) * self) {                                            \
    rk_assert(self->count && "Cannot access back of empty deque");                                 \
    return RK__DEQUE_F(T, at)(self, self->count - 1);                                              \
  }                                                                                                \
  static_fun const T* RK__DEQUE_F(T, back_const)(const Deque(T) * self) {                          \
    rk_assert(self->count && "Cannot access back of empty deque");                                 \
    return RK__DEQUE_F(T, at_const)(self, self->count - 1);                                        \
  }                                                                                                \
  static_fun void RK__DEQUE_F(T, push_front)(Deque(T) * self, T value) {                           \
    if (self->count == self->cap) {                                                                \
      rk_assert(self->cap <= SIZE_MAX / 2 && "Deque capacity overflow");                           \
      RK__DEQUE_F(T, reserve)(self, self->cap ? self->cap * 2 : 8);                                \
    }                                                                                              \
    self->head             = (self->head - 1) & (self->cap - 1);                                   \
    self->data[self->head] = value;                                                                \
    ++self->count;                                                                                 \
  }                                                                                                \
  static_fun void RK__DEQUE_F(T, push_back)(Deque(T) * self, T value) {                            \
    if (self->count == self->cap) {                                                                \
      rk_assert(self->cap <= SIZE_MAX / 2 && "Deque capacity overflow");                           \
      RK__DEQUE_F(T, reserve)(self, self->cap ? self->cap * 2 : 8);                                \
    }                                                                                              \
    self->data[(self->head + self->count) & (self->cap - 1)] = value;                              \
    ++self->count;                                                                                 \
  }                                                                                                \
  static_fun T RK__DEQUE_F(T, pop_front)(Deque(T) * self) {                                        \
    rk_assert(self->count && "Cannot pop an empty deque");                                         \
    T result   = self->data[self->head];                                                           \
    self->head = (self->head + 1) & (self->cap - 1);                                               \
    if (!--self->count) { self->head = 0; }                                                        \
    return result;                                                                                 \
  }                                                                                                \
  static_fun T RK__DEQUE_F(T, pop_back)(Deque(T) * self) {                                         \
    rk_assert(self->count && "Cannot pop an empty deque");                                         \
    T result = self->data[(self->head + self->count - 1) & (self->cap - 1)];                       \
    if (!--self->count) { self->head = 0; }                                                        \
    return result;                                                                                 \
  }                                                                                                \
  static_fun bool RK__DEQUE_F(T, try_pop_front)(Deque(T) * self, T * out) {                        \
    return self->count ? (*out = RK__DEQUE_F(T, pop_front)(self), true) : false;                   \
  }                                                                                                \
  static_fun bool RK__DEQUE_F(T, try_pop_back)(Deque(T) * self, T * out) {                         \
    return self->count ? (*out = RK__DEQUE_F(T, pop_back)(self), true) : false;                    \
  }                                                                                                \
  static_fun void RK__DEQUE_F(T, push_front_n)(Deque(T) * self, const T* arr, size_t n) {          \
    if (!n) { return; }                                                                            \
    rk_assert(n <= SIZE_MAX - self->count && "Deque capacity overflow");                           \
    RK__DEQUE_F(T, reserve)(self, self->count + n);                                                \
    self->head   = (self->head - n) & (self->cap - 1);                                             \
    size_t first = self->cap - self->head < n ? self->cap - self->head : n;                        \
    rk_memcpy(self->data + self->head, arr, sizeof_n(T, first));                                   \
    if (first < n) { rk_memcpy(self->data, arr + first, sizeof_n(T, n - first)); }                 \
    self->count += n;                                                                              \
  }                                                                                                \
  static_fun void RK__DEQUE_F(T, push_back_n)(Deque(T) * self, const T* arr, size_t n) {           \
    if (!n) { return; }                                                                            \
    rk_assert(n <= SIZE_MAX - self->count && "Deque capacity overflow");                           \
    RK__DEQUE_F(T, reserve)(self, self->count + n);                                                \
    size_t start = (self->head + self->count) & (self->cap - 1);                                   \
    size_t first = self->cap - start < n ? self->cap - start : n;                                  \
    rk_memcpy(self->data + start, arr, sizeof_n(T, first));                                        \
    if (first < n) { rk_memcpy(self->data, arr + first, sizeof_n(T, n - first)); }                 \
    self->count += n;                                                                              \
  }                                                                                                \
  static_fun void RK__DEQUE_F(T, assign)(Deque(T) * self, const T* arr, size_t n) {                \
    RK__DEQUE_F(T, clear)(self);                                                                   \
    RK__DEQUE_F(T, push_back_n)(self, arr, n);                                                     \
  }                                                                                                \
  static_fun Deque(T) RK__DEQUE_F(T, from)(const T* arr, size_t n RK_IFALLOC(, Allocator alloc)) { \
    Deque(T) d = RK__DEQUE_F(T, init)(n RK_IFALLOC(, alloc));                                      \
    RK__DEQUE_F(T, push_back_n)(&d, arr, n);                                                       \
    return d;                                                                                      \
  }                                                                                                \
  RK_EXTERNC_END

RK_HEADER_END

#define RK__DEQUE_INIT(T, cap, alloc)     RK__DEQUE_F(T, init)(cap RK_IFALLOC(, alloc))
#define RK__DEQUE_INIT3(T, cap, alloc)    rk_disable_if(RK__DEQUE_INIT(T, cap, alloc))
#define RK__DEQUE_INIT2(T, cap)           RK__DEQUE_INIT(T, cap, alloc_ctx)

#define RK__DEQUE_FROM(T, arr, n, alloc)  RK__DEQUE_F(T, from)((arr), (n)RK_IFALLOC(, (alloc)))
#define RK__DEQUE_FROM4(T, arr, n, alloc) rk_disable_if(RK__DEQUE_FROM(T, arr, n, alloc))
#define RK__DEQUE_FROM3(T, arr, n)        RK__DEQUE_FROM(T, arr, n, alloc_ctx)

/// @}
#endif // RK_DEQUE_H

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
/* END INLINE: include/rk_deque.h */
/* inlined from include/rklib.h:14: #include "rk_arenastack.h" */
/* BEGIN INLINE: include/rk_arenastack.h */
// SPDX-License-Identifier: MIT
/// @file rk_arenastack.h
/// @version 1.0.0
/// @defgroup rk_arenastack ArenaStack Allocator Interface
/// @brief Dynamic stack of arena allocators for growable, stack-like memory management with stable
/// allocation addresses.
///
/// The ArenaStack type manages a growable collection of fixed-size Arena allocators. It provides
/// convenience macros and functions for allocating, reallocating, clearing, and rewinding memory
/// from this stack of arenas.
///
/// Typical usage:
///   - Create an ArenaStack with arenastack_init(), specifying arena size and an optional
///     allocator.
///   - Allocate typed memory blocks using arenastack_new() or arenastack_new_aligned().
///   - Reclaim memory by rewinding with arenastack_rewind_to(), or by clearing all arenas via
///     `arenastack_clear()`.
///   - Release resources with arenastack_release().
///
/// Layout:
///   - Internally stores a Vec of Arena objects and a pointer to the currently active arena for
///     fast allocations.
///   - Designed for fast, stack-like allocation patterns with occasional rewinds and bulk clears.
///
/// Notes:
///   - Alignment arguments must be a power of two; undefined behaviour otherwise.
///   - Only one active ArenaStack instance is expected per intended allocation pool.
///
/// @see rk_alloc.h
/// @see rk_defs.h
/// @see rk_arena.h
/// @see rk_vec.h
/// @{

#ifndef RK_ARENASTACK_H
#define RK_ARENASTACK_H
/* inlined from include/rk_arenastack.h:37: #include "rk_arena.h" */
/* skipped already-included: "include/rk_arena.h" */
/* inlined from include/rk_arenastack.h:38: #include "rk_vec.h" */
/* skipped already-included: "include/rk_vec.h" */
RK_HEADER_BEGIN

/// @brief A dynamic stack of arenas used for memory allocation. Each arena is a fixed-size memory
/// block managed by the Arena allocator. The ArenaStack tracks a vec of arenas and the current
/// arena for allocation.
typedef struct ArenaStack {
  size_t     arena_size;
  Vec(Arena) arenas;
  size_t     cur;
} ArenaStack;

/// @brief `ArenaStack arenastack_init(size_t arena_size, Allocator alloc = alloc_ctx)` -
/// Initialises and returns a new ArenaStack with the desired capacity and allocator.
/// @param arena_size  The desired size of each arena
/// @param alloc       Optional allocator; defaults to `alloc_ctx`
/// @return A new ArenaStack
#define arenastack_init(arena_size, ...) rk_overload(RK__ARENASTACK_INIT, arena_size, ##__VA_ARGS__)

/// @brief Releases all arenas within the ArenaStack.
/// @note No-op if `self` is `NULL`.
static_fun void        arenastack_release(ArenaStack* self);

/// @brief Returns the allocator backing the ArenaStack's arenas, or `alloc_ctx` if `self` is
/// `NULL`.
static_fun Allocator arenastack_allocator(const ArenaStack* self) {
  return self ? vec_allocator(self->arenas) : alloc_ctx;
}

/// @brief Marks all Memory in the ArenaStack as reusable Clears all currently active arenas and
/// resets the current arena index. Memory in all arenas becomes available for reuse; arenas beyond
/// the current index are left unchanged until reused.
/// @return `self`, for chaining. No-op returning `self` if `self` is `NULL`.
static_fun ArenaStack* arenastack_clear(ArenaStack* self);

/// @brief Returns the current position of the active arena as an opaque marker. Pass to
/// `arenastack_rewind_to` to restore the ArenaStack to this state.
/// @note Returns a null marker if `self` is `NULL`.
static_fun ArenaMark   arenastack_mark(const ArenaStack* self) {
  return (self && self->arena_size) ? arena_mark(&self->arenas[self->cur]) : (ArenaMark){rk_null};
}
/// @brief Rewinds the ArenaStack to a specific mark returned by `arenastack_mark()`, marking every
/// allocation in every Arena of the Stack as free until the mark is reached.
/// @return `self`, for chaining. No-op returning `self` if `self` is `NULL`.
static_fun ArenaStack* arenastack_rewind_to(ArenaStack* restrict self, ArenaMark mark);

/// @brief `void* arenastack_allocate(size_t nbytes, size_t align, ArenaStack* self)` - Allocates
/// `nbytes` bytes with the given alignment from the ArenaStack, growing into a new arena if
/// necessary. Prefer `arenastack_new` for typed allocations.
/// @param nbytes Number of bytes to allocate
/// @param align  Desired alignment; must be a power of two
/// @param self   ArenaStack to allocate from
/// @return Pointer to the allocated memory
static_fun void*       arenastack_allocate(size_t nbytes, size_t align, ArenaStack* self);

/// @brief `T* arenastack_new(T, size_t count, ArenaStack* arena_stack)` - Create a new allocation
/// in the arena for a given type T and count.
/// @param  T           The type to allocate
/// @param  count       Number of elements of type T to allocate
/// @param  arena_stack Pointer to the ArenaStack to allocate from
/// @return Pointer to the allocated memory
#define arenastack_new(T, count, arena_stack) RK__arenastack_NEW(T, count, arena_stack)

/// @brief `T* arenastack_new_aligned(T, size_t count, size_t align, ArenaStack* arena_stack)` -
/// Create a new, allocation in the ArenaStack for a given type T and count with a given alignment.
/// @param T           The type to allocate
/// @param count       Number of elements of type T to allocate
/// @param align       The desired alignment (must be a power of two)
/// @param arena_stack Pointer to the ArenaStack to allocate from
/// @return Pointer to the allocated memory.
/// @note If alignment is not a power of two, behaviour is undefined
#define arenastack_new_aligned(T, count, align, arena_stack)                                       \
  RK__arenastack_ALIGNED_NEW(T, count, align, arena_stack)

static_fun alloc_allocation_f   RK__arenastack_allocate;
static_fun alloc_reallocation_f RK__arenastack_reallocate;
static_fun alloc_deallocation_f RK__arenastack_deallocate;
static const AllocatorVTable arenastack_allocator_vtable = {.alloc_f   = RK__arenastack_allocate,
                                                            .realloc_f = RK__arenastack_reallocate,
                                                            .dealloc_f = RK__arenastack_deallocate};

static_fun Allocator         arenastack_to_alloc(ArenaStack* self) {
  return (Allocator){.vtab = &arenastack_allocator_vtable, .ctx = self};
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

#define RK__arenastack_ALIGNED_NEW(T, count, align, arena_stack)                                   \
  ((typeof(T)*)(alloc_log_new, rk_assert_valid_align(T, align),                                    \
                arenastack_allocate(sizeof_n(T, count), align, arena_stack)))

#define RK__arenastack_NEW(T, count, arena_stack)                                                  \
  ((typeof(T)*)(alloc_log_new, arenastack_allocate(sizeof_n(T, count), alignof(T), arena_stack)))

static_fun void arenastack_release(ArenaStack* self) {
  if rk_unlikely (!self) { return; }
  RK_IFALLOC(Allocator alloc = vec_allocator(self->arenas);)
  vec_foreach(self->arenas, arena) {
    alloc_deallocate(arena->beg, (size_t)(arena->end - arena->beg), align_max RK_IFALLOC(, alloc));
  }
  vec_release(self->arenas);
  self->arena_size = 0, self->cur = 0;
}

static_fun ArenaStack* arenastack_clear(ArenaStack* self) {
  if rk_unlikely (!self || !self->arena_size) { return self; }
  for (size_t cur = self->cur, i = 0; i <= cur; ++i) { arena_clear(&self->arenas[i]); }
  return self->cur = 0, self;
}

static_fun ArenaStack* arenastack_rewind_to(ArenaStack* restrict self, ArenaMark mark) {
  const unsigned char* ptr = mark.pos;
  if rk_unlikely (!self || !ptr) { return self; }
  for (size_t i = self->cur + 1; i-- > 0;) {
    Arena* arena = &self->arenas[i];
    if (ptr == arena->cur || rk_ptr_in_range(ptr, arena->beg, arena->cur)) {
      arena_rewind_to(arena, mark);
      self->cur = i - (i > 0 && arena_is_empty(arena));
      return self;
    }
    arena_clear(arena);
  }
  rk_assert(0 && "Pointer was not allocated by this stack");
  unreachable();
}

static_fun ArenaStack RK__arenastack_init(size_t cap RK_IFALLOC(, Allocator alloc)) {
  rk_assert_allocator_valid(alloc);
  cap = stdc_bit_ceil(cap); /*1 if cap==0*/
  return (ArenaStack){
      .arena_size = cap,
      .arenas     = vec_init_list(
          Arena, RK_IFALLOC(alloc, ) arena_init(
                     (unsigned char*)alloc_allocate(cap, align_max RK_IFALLOC(, alloc)), cap)),
      .cur = 0};
}
#define RK__ARENASTACK_INIT(_cap, _alloc)  RK__arenastack_init(_cap RK_IFALLOC(, _alloc))
#define RK__ARENASTACK_INIT2(_cap, _alloc) rk_disable_if(RK__ARENASTACK_INIT(_cap, _alloc))
#define RK__ARENASTACK_INIT1(cap)          RK__ARENASTACK_INIT(cap, alloc_ctx)

#define RK__arena_alloc_init(_SIZE, _ALIGN, _ALLOC)                                                \
  arena_init((unsigned char*)alloc_allocate(_SIZE, _ALIGN RK_IFALLOC(, _ALLOC)), _SIZE)

static_fun rk_alloc_alignsize(2, 1) void* RK__arenastack_allocate(size_t nbytes, size_t align,
                                                                  void* ctx) {
  rk_assert_align_pow2(align);
  ArenaStack* self   = (ArenaStack*)ctx;
  size_t      needed = nbytes + (align - 1);
  if (!self->arena_size) { *self = arenastack_init(stdc_bit_ceil(needed)); }
  void* res = arena_try_allocate(nbytes, align, &self->arenas[self->cur]);
  if (res) { return res; }
  ++self->cur; // slow path: Check if there's a large-enough preallocated arena
  for (size_t i = self->cur, count = vec_count(self->arenas); i < count; ++i) {
    if (needed <= arena_cap(&self->arenas[i])) {
      if (i != self->cur) { rk_SWAP(self->arenas[i], self->arenas[self->cur]); }
      arena_clear(&self->arenas[self->cur]);
      return arena_allocate(nbytes, align, &self->arenas[self->cur]);
    }
  }
  // no large-enough arena found; create new one
  // The backing chunk only ever needs align_max: `needed` above already
  // reserves nbytes + (align - 1) slack, so arena_try_allocate() can carve
  // out an `align`-aligned pointer from any align_max-aligned chunk.
  // Matches the align_max used to deallocate arenas in arenastack_release().
  vec_insert_at_unordered(self->arenas, self->cur,
                          RK__arena_alloc_init(stdc_bit_ceil(rk_MAX(needed, self->arena_size)),
                                               align_max, vec_allocator(self->arenas)));
  return arena_allocate(nbytes, align, &self->arenas[self->cur]);
}

static_fun void RK__arenastack_deallocate(void* ptr, size_t old_size, size_t align, void* ctx) {
  ArenaStack* self = (ArenaStack*)ctx;
  RK__arena_deallocate(ptr, old_size, align, &self->arenas[self->cur]);
}

static_fun rk_alloc_alignsize(4, 3) void* RK__arenastack_reallocate(void* ptr, size_t old_size,
                                                                    size_t new_size, size_t align,
                                                                    void* ctx) {
  rk_assert_align_pow2(align);
  ArenaStack* self = (ArenaStack*)ctx;
  if (!old_size) { return arenastack_allocate(new_size, align, self); }
  if ((arena_is_top_allocation(&self->arenas[self->cur], ptr, old_size)
       && arena_try_resize_top(old_size, new_size, &self->arenas[self->cur]))
      || new_size <= old_size) {
    return ptr;
  }
  void* res = arenastack_allocate(new_size, align, self);
  rk_memcpy(res, ptr, rk_MIN(old_size, new_size));
  return res;
}

static_fun rk_alloc_alignsize(2, 1) void* arenastack_allocate(size_t nbytes, size_t align,
                                                              ArenaStack* self) {
  return RK__arenastack_allocate(nbytes, align, self);
}

#undef RK__arena_alloc_init

/// @endcond

RK_HEADER_END
/// @}
#endif // RK_ARENASTACK_H

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
/* END INLINE: include/rk_arenastack.h */
/* inlined from include/rklib.h:15: #include "rk_string.h" */
/* BEGIN INLINE: include/rk_string.h */
// SPDX-License-Identifier: MIT
/// @file rk_string.h
/// @version 1.0.0
/// @defgroup rk_string String Library Interface
/// @brief A small custom header-only String library for working with dynamically allocated Strings
/// and String views. This library provides functionality to create, manipulate, and manage custom
/// String Structs (`Str`), which are dynamically allocated with automatic resizing based on the
/// length of the String. It also provides a simple immutable String view type, `Strv`.
///
/// @section Customisation Points Customisation points are the same as for all other rk_clib
/// headers, most notably the `RK_CUSTOM_ALLOCATORS` macro which, if set to `0`, turns off custom
/// allocators, removing the respective data members from the structs and parameters from the
/// functions, while discarding the allocator parameters in the user-facing macros, removing any
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
/* inlined from include/rk_string.h:30: #include "rk_alloc.h" */
/* skipped already-included: "include/rk_alloc.h" */
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
#if RK_CUSTOM_ALLOCATORS
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

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @name Strlike Basic Accessors
////////////////////////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @name String Lifetime/Ownership
////////////////////////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @name String Accessors
////////////////////////////////////////////////////////////////////////////////////////////////////

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

/// @brief Returns the current capacity of `self`, in bytes, or 0 if `self` is NULL.
static_fun rk_pure size_t str_cap(const Str* self) { return self ? self->cap : 0; }

static_fun rk_pure Allocator str_allocator(const Str* self) {
#if RK_CUSTOM_ALLOCATORS
  return self ? self->alloc : alloc_ctx;
#else
  return (void)self, alloc_ctx;
#endif
}

/// @brief Clears the contents of `self`, setting its length to zero and null-terminating it, if it
/// owns an allocation.
static_fun Str* str_clear(Str* restrict self) {
  if (self->str) { self->str[self->len = 0] = '\0'; }
  return self;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @name String Capacity
////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Ensures at least `new_cap` bytes of capacity are allocated for `self`, reallocating, if
/// necessary.
static_fun Str*           str_reserve(Str* restrict self, size_t new_cap);

/// @brief Resizes the length of `self` to `new_len`, reallocating the memory if necessary and
/// null-terminating it.
static_fun Str*           str_resize(Str* restrict self, size_t new_len);

/// @brief Resizes a Str's capacity to the next power of two larger than its length,
/// null-terminating it (matching `vec_shrink_to_fit()`'s convention). Leaves some slack to reduce
/// reallocation on subsequent growth.
/// @note Use `str_shrink_to_fit_exact()` for an exact-capacity shrink.
static_fun Str*           str_shrink_to_fit(Str* restrict self);

/// @brief Resizes a Str's capacity to exactly its length + 1 (matching
/// `vec_shrink_to_fit_exact()`'s convention).
static_fun Str*           str_shrink_to_fit_exact(Str* restrict self);

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @name String Mutators
////////////////////////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @name Strv Construction and View
////////////////////////////////////////////////////////////////////////////////////////////////////

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
/// `alloc_ctx` (or malloc_allocator, if `RK_CUSTOM_ALLOCATORS` == `0`)
/// @return A Strv* to the array of tokens.
#define str_split_alloc(strlike, delims, out_count, ...)                                           \
  rk_overload(RK__STR_SPLIT_ALLOC, strv_from(strlike), strv_from(delims), out_count, ##__VA_ARGS__)

/// @brief Trims a Stringlike by adjusting both its starting and ending position past any leading
/// and trailing space characters.
/// @param strlike The Stringlike to trim, by value
/// @return A Strv over the trimmed Stringlike
/// @note `c` counts as space if `(c == ' ' || (c >= '\t' && c <= '\r'))`
#define str_trimmed(strlike)            str_trimmed_strv(strv_from(strlike))

/// @brief Like `str_trimmed()`, but only adjusts the starting position past any leading space
/// characters, leaving trailing space untouched.
#define str_trimmed_left(strlike)       str_trimmed_left_strv(strv_from(strlike))

/// @brief Like `str_trimmed()`, but only adjusts the ending position past any trailing space
/// characters, leaving leading space untouched.
#define str_trimmed_right(strlike)      str_trimmed_right_strv(strv_from(strlike))

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @name Strlike Query/Search Functions
////////////////////////////////////////////////////////////////////////////////////////////////////

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
  if (!suf.len) { return true; }
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
  if (!s2.len) { return true; }
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
  if (sv.len) { memmove(self->str, sv.str, sv.len); }
  self->str[self->len = sv.len] = '\0';
  return self;
}

static_fun Strv strv_slice_strv(Strv sv, size_t start, size_t end) {
  if (start > sv.len || end < start) {
    sv.len = 0;
  } else {
    if (sv.str) { sv.str += start; }
    sv.len = rk_MIN(end, sv.len) - start;
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
  uptr   sbeg = (uptr)self->str, send = sbeg + self->len, cbeg = (uptr)sv.str;
  bool   alias = cbeg >= sbeg && cbeg < send;
  size_t new_len;
  if (alias) {
    rk_set_alloc_fallback(self->alloc);
    char* from = alloc_new(char, sv.len RK_IFALLOC(, self->alloc));
    memcpy(from, sv.str, sv.len);
    new_len = sv.len + self->len;
    RK__str_ensure_cap(self, new_len + 1);
    memmove(self->str + idx + sv.len, self->str + idx, self->len + 1 - idx);
    memcpy(self->str + idx, from, sv.len);
    alloc_delete(from, sv.len RK_IFALLOC(, self->alloc));
  } else {
    const char* from = sv.str;
    new_len          = sv.len + self->len;
    RK__str_ensure_cap(self, new_len + 1);
    memmove(self->str + idx + sv.len, self->str + idx, self->len + 1 - idx);
    memcpy(self->str + idx, from, sv.len);
  }
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
    // The adjustment is always exactly 0 or 'a'-'A', so the result always
    // stays within a valid char; the cast just makes that narrowing explicit.
    s[i] = (char)(s[i] - (s[i] >= 'a' && s[i] <= 'z') * ('a' - 'A'));
  }
  return self;
}
static_fun Str* str_to_lower(Str* restrict self) {
  char* s = self->str;
  for (size_t i = 0, len = self->len; i < len; ++i) {
    s[i] = (char)(s[i] + (s[i] >= 'A' && s[i] <= 'Z') * ('a' - 'A'));
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
  data[cidx++] = (Strv){.str = str.str ? str.str + start : rk_null, .len = str.len - start};
  *out_count   = count;
  return data;
}

#define RK__STR_SPLIT_ALLOC(str, delims, count, alloc)                                             \
  str_split_alloc(str, delims, count RK_IFALLOC(, alloc))
#define RK__STR_SPLIT_ALLOC4(str, delims, count, alloc)                                            \
  rk_disable_if(RK__STR_SPLIT_ALLOC(str, delims, count, alloc))
#define RK__STR_SPLIT_ALLOC3(str, delims, count) RK__STR_SPLIT_ALLOC(str, delims, count, alloc_ctx)

static_fun Str* rk_attr_printf(2, 3) str_cat_fmt(Str* self, const char* fmt, ...) {
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
/* END INLINE: include/rk_string.h */
/* inlined from include/rklib.h:16: #include "rk_pool.h" */
/* BEGIN INLINE: include/rk_pool.h */
// SPDX-License-Identifier: MIT
/// @file rk_pool.h
/// @version 1.0.0
/// @defgroup rk_pool Pool Allocator Interface
/// @brief Type-safe generic pool allocator for C
///
/// This header provides a type-safe, generic pool allocator system for C. It supports:
/// - **Dynamic pools** (runtime capacity) using `Pool(T)`
/// - **Static pools** (compile-time fixed capacity) using `Pool(T, C)`
/// - Allocation and deallocation tracking using `bitset`
/// - Type-safe macros for creating, accessing, and releasing pool elements
/// - Iteration over active elements via `pool_foreach`
///
/// The system is heavily macro-based to simulate function overloading and type safety in plain C.
/// @see rk_alloc.h
/// @see rk_defs.h
/// @{

#ifndef RK_POOL_H
#define RK_POOL_H
/* inlined from include/rk_pool.h:21: #include "rk_alloc.h" */
/* skipped already-included: "include/rk_alloc.h" */
/* inlined from include/rk_pool.h:22: #include "rk_bitset.h" */
/* skipped already-included: "include/rk_bitset.h" */
RK_HEADER_BEGIN

/// @brief Defines a pool type and associated functions for type `T` Depending on the arguments,
/// this macro defines either a dynamic pool (`Pool(T)`) or a static/fixed pool (`Pool(T, C)`).
/// @param T Type of elements stored in the pool
/// @param C Capacity of the pool if static
#define POOL_DEFINE(T, ...)     RK__STATOVERLOAD(RK__POOL_DEFINE, T, ##__VA_ARGS__)

/// @brief Alias for the pool type (dynamic or static).
/// @param T Type of elements stored in the pool
/// @param C Capacity of the pool if static
/// @note Static Pools take a second capacity parameter
#define Pool(T, ...)            RK__STATOVERLOAD__(RK__POOL, T, ##__VA_ARGS__)
#define StaticPool(T, CAP)      Pool_static_##T##_##CAP
#define DynPool(T)              Pool_dynamic_##T

/// @brief `Pool(T)* pool_init(T, size_t cap, Allocator alloc = alloc_ctx)` - Initializes a dynamic
/// pool with given capacity.
/// @param T Element type
/// @param cap Desired capacity
/// @param alloc Optional allocator
/// @return Initialized pool struct
#define pool_init(T, _cap, ...) rk_overload(RK__DPOOL_INIT, T, _cap, ##__VA_ARGS__)

/// @brief Compile-time zero-initializer for a `Pool(T, C)` (`StaticPool`). Suitable for global and
/// static variables. No memory is allocated.
/// @note Named after `StaticPool`, the type it initializes — not to be confused with the (removed)
/// `_init_static` convention other containers used for static-storage-duration-safe initializers.
#define staticpool_init         {RK_ZINIT}

/// @brief `void pool_release(Pool(T, ...)* self)` - Releases the associated resources of the pool
/// (if the pool is dynamic) and resets its members. For static pools, this resets the allocation
/// bitset but does not modify the underlying element storage.
#define pool_release(self)      ((void)RK__pool_release(self))

/// @brief `size_t pool_cap(Pool(T, ...)* self)` - Returns the total capacity of the pool.
#define pool_cap(self)          ((size_t)RK__pool_cap(self))

#if RK_CUSTOM_ALLOCATORS
/// @brief `Allocator pool_allocator(Pool(T)* self)` - Returns the Allocator the (dynamic) pool was
/// constructed with.
# define pool_allocator(self) rk_to_rvalue((self)->_pool.alloc)
#else
/// @brief `Allocator pool_allocator(Pool(T)* self)` - Returns `alloc_ctx` (allocators disabled).
# define pool_allocator(self) ((void)(self), alloc_ctx)
#endif

/// @brief `size_t pool_used(Pool(T)* self)` - Returns the number of active (allocated) elements in
/// the pool.
#define pool_used(self)        ((size_t)RK__pool_used(self))

/// @brief `size_t pool_remaining(Pool(T)* self)` - Returns the number of free slots remaining in
/// the pool.
#define pool_remaining(self)   ((size_t)RK__pool_remaining(self))

/// @brief Returns `true` iff the pool is empty.
#define pool_is_empty(self)    ((bool)(pool_used(self) == 0))

/// @brief Returns `true` iff the pool is full.
#define pool_is_full(self)     ((bool)(pool_remaining(self) == 0))

/// @brief `Pool(T)* pool_clear(Pool(T)* self)` - Marks all elements in the pool as reusable.
/// @return `self`, for chaining
#define pool_clear(self)       ((typeof(self))RK__pool_clear(self))

/// @brief `T* pool_new(Pool(T)* self)` - Allocates a new element in the pool.
/// @return Pointer to the newly allocated element
#define pool_new(self)         ((RK__poolT(self)*)RK__pool_new(self))

/// @brief `T* pool_try_new(Pool(T)* self)` - Like `pool_new()`, but returns `NULL` if full instead
/// of running `RK_POOL_FAIL()`.
#define pool_try_new(self)     ((RK__poolT(self)*)RK__pool_try_new(self))

/// @brief `T* pool_put(Pool(T)* self, T el)` - Allocates a new element and stores a copy of the
/// value.
/// @return Pointer to the inserted element
#define pool_put(self, el)     ((RK__poolT(self)*)RK__pool_put(self, el))

/// @brief `T* pool_try_put(Pool(T)* self, T el)` - Like `pool_put()`, but returns `NULL` if full
/// instead of running `RK_POOL_FAIL()`.
#define pool_try_put(self, el) ((RK__poolT(self)*)RK__pool_try_put(self, el))

/// @brief `void pool_delete(Pool(T)* self, T* ptr)` - Frees an element in the pool.
#define pool_delete(self, ptr) ((void)RK__pool_delete(self, ptr))

/// @brief `pool_foreach(Pool(T)* self, it)` - Iterates over all allocated elements in the pool.
///
/// Example:
/// ```c
/// pool_foreach(&my_pool, elem) {
///     printf("%d\n", *elem);
/// }
/// ```
#define pool_foreach(self, it) RK__pool_foreach(self, it)

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

#define RK__POOL2              StaticPool
#define RK__POOL_DEFINE2(T, C)                                                                     \
  typedef struct StaticPool(T, C) {                                                                \
    bitset(C) data;                                                                                \
    union {                                                                                        \
      T els[C];                                                                                    \
      T RK__POOL_ELS[C];                 /* only for _Generic, never read */                       \
      union { char cap, els[1]; } _pool; /* only for _Generic, never read */                       \
    };                                                                                             \
  } StaticPool(T, C)

/// @brief to allow for type-generic access of dynamic pools todo c++ UB union
typedef struct RK__pool_dynamic {
  RK_IFALLOC(Allocator alloc;)
  bitset data;
  union {
    size_t cap;
    char   _[1]; /* to make the two types layout compatible for c++*/
  };
  void* els;
} RK__pool_dynamic;

#define RK__POOL_DEFINE1(T)                                                                        \
  typedef struct DynPool(T) {                                                                      \
    union {                                                                                        \
      struct {                                                                                     \
        RK_IFALLOC(Allocator alloc;)                                                               \
        bitset data;                                                                               \
        union {                                                                                    \
          size_t cap;             /* only for _Generic, never read */                              \
          char   RK__POOL_ELS[1]; /* only for _Generic, never read */                              \
        };                                                                                         \
        T* els;                                                                                    \
      };                                                                                           \
      RK__pool_dynamic _pool;                                                                      \
    };                                                                                             \
    static_assert(sizeof(T*) == sizeof(void*) && alignof(T*) == alignof(void*));                   \
  } DynPool(T)

#define RK__POOL1       DynPool

#define RK__poolT(self) typeof(*(self)->els)

#define RK__pool_dispatch(self, if_stat, if_dyn)                                                   \
  rk_static_if(sizeof((self)->_pool) == 1, if_stat, if_dyn)

static_fun rk_forceinline void* RK__Dpool_init(size_t elsize, size_t elalign,
                                               RK__pool_dynamic* self,
                                               size_t cap        RK_IFALLOC(, Allocator alloc)) {
  rk_assert_allocator_valid(alloc);
  self->cap = cap;
  RK_IFALLOC(self->alloc = alloc;)
  self->data
      = bitset_clear_all(alloc_new(bitset_word, bitset_words(cap) RK_IFALLOC(, self->alloc)), cap);
  self->els = alloc_allocate(rk_mult(elsize, cap), elalign RK_IFALLOC(, alloc));
  return self;
}
#define RK__DPOOL_INIT(T, _cap, _alloc)                                                            \
  (*((Pool(T)*)RK__Dpool_init(sizeof(T), alignof(T), (RK__pool_dynamic*)((Pool(T)[1]){RK_ZINIT}),  \
                              _cap RK_IFALLOC(, _alloc))))
#define RK__DPOOL_INIT3(T, _cap, _alloc) rk_disable_if(RK__DPOOL_INIT(T, _cap, _alloc))
#define RK__DPOOL_INIT2(T, _cap)         RK__DPOOL_INIT(T, _cap, alloc_ctx)

#define RK__pool_cap(self)                                                                         \
  RK__pool_dispatch(self, rk_COUNTOF((self)->RK__POOL_ELS), (size_t)(self)->_pool.cap)

static_fun rk_forceinline size_t RK__Dpool_used(const RK__pool_dynamic* self) {
  return bitset_count_ones(self->data, self->cap);
}
#define RK__pool_used(self)                                                                        \
  RK__pool_dispatch(self, bitset_count_ones((self)->data, rk_COUNTOF((self)->RK__POOL_ELS)),       \
                    RK__Dpool_used((RK__pool_dynamic*)&((self)->_pool)))

static_fun rk_forceinline size_t RK__Dpool_remaining(const RK__pool_dynamic* self) {
  return bitset_count_zeros(self->data, self->cap);
}
#define RK__pool_remaining(self)                                                                   \
  RK__pool_dispatch(self, bitset_count_zeros((self)->data, rk_COUNTOF((self)->RK__POOL_ELS)),      \
                    RK__Dpool_remaining((RK__pool_dynamic*)&((self)->_pool)))

static_fun rk_forceinline void* RK__Dpool_clear(RK__pool_dynamic* self) {
  bitset_clear_all(self->data, self->cap);
  return self;
}
#define RK__pool_clear(self)                                                                       \
  RK__pool_dispatch(self, bitset_clear_all((self)->data, rk_COUNTOF((self)->RK__POOL_ELS)),        \
                    RK__Dpool_clear((RK__pool_dynamic*)&((self)->_pool)))

static_fun rk_forceinline void RK__Spool_delete(size_t elsize, size_t align, size_t cap, void* self,
                                                void* ptr) {
  bitset_clear((bitset)self, cap,
               (size_t)((size_t)((char*)ptr - ((char*)self + rk_align_up(bitset_bytes(cap), align)))
                        / elsize));
}
static_fun rk_forceinline void RK__Dpool_delete(size_t elsize, RK__pool_dynamic* self, void* ptr) {
  bitset_clear(self->data, self->cap, (size_t)((size_t)((char*)ptr - (char*)self->els) / elsize));
}

#define RK__pool_delete(self, ptr)                                                                 \
  RK__pool_dispatch(                                                                               \
      self,                                                                                        \
      RK__Spool_delete(sizeof(*(self)->els), alignof(RK__poolT(self)),                             \
                       rk_COUNTOF((self)->RK__POOL_ELS), self, ptr),                               \
      RK__Dpool_delete(sizeof(*(self)->els), (RK__pool_dynamic*)&((self)->_pool), ptr))

static_fun rk_forceinline rk_alloc_alignsize(2, 1) void* RK__Spool_try_new(size_t elsize,
                                                                           size_t align, size_t cap,
                                                                           void* self) {
  size_t free_slot = bitset_first_trailing_zero((bitset)self, cap);
  if (!free_slot) { return rk_null; }
  bitset_set((bitset)self, cap, free_slot - 1);
  return (char*)self + rk_align_up(bitset_bytes(cap), align) + elsize * (free_slot - 1);
}

static_fun rk_forceinline rk_alloc_alignsize(2, 1) void* RK__Spool_new(size_t elsize, size_t align,
                                                                       size_t cap, void* self) {
  void* res = RK__Spool_try_new(elsize, align, cap, self);
  RK_POOL_FAIL(res, self, rk_null, align, elsize);
  return res;
}
static_fun rk_forceinline rk_alloc_alignsize(2, 1) void* RK__Dpool_try_new(size_t       elsize,
                                                                           size_t align rk_unused,
                                                                           RK__pool_dynamic* self) {
  if (!self->cap) { return rk_null; }
  size_t free_slot = bitset_first_trailing_zero(self->data, self->cap);
  if (!free_slot) { return rk_null; }
  bitset_set(self->data, self->cap, free_slot - 1);
  return (char*)self->els + elsize * (free_slot - 1);
}
static_fun rk_forceinline rk_alloc_alignsize(2, 1) void* RK__Dpool_new(size_t elsize, size_t align,
                                                                       RK__pool_dynamic* self) {
  void* res = RK__Dpool_try_new(elsize, align, self);
  RK_POOL_FAIL(res, self, rk_null, align, elsize);
  return res;
}

#define RK__pool_try_new(self)                                                                     \
  RK__pool_dispatch(self,                                                                          \
                    RK__Spool_try_new(sizeof(*(self)->els), alignof(RK__poolT(self)),              \
                                      rk_COUNTOF((self)->RK__POOL_ELS), self),                     \
                    RK__Dpool_try_new(sizeof(*(self)->els), alignof(RK__poolT(self)),              \
                                      (RK__pool_dynamic*)&((self)->_pool)))

#define RK__pool_new(self)                                                                         \
  RK__pool_dispatch(self,                                                                          \
                    RK__Spool_new(sizeof(*(self)->els), alignof(RK__poolT(self)),                  \
                                  rk_COUNTOF((self)->RK__POOL_ELS), self),                         \
                    RK__Dpool_new(sizeof(*(self)->els), alignof(RK__poolT(self)),                  \
                                  (RK__pool_dynamic*)&((self)->_pool)))

static_fun rk_forceinline rk_alloc_alignsize(2, 1) void* RK__Spool_try_put(size_t elsize,
                                                                           size_t align, size_t cap,
                                                                           void* restrict self,
                                                                           void* restrict obj) {
  bitset data      = (bitset)self;
  size_t free_slot = bitset_first_trailing_zero(data, cap);
  if (!free_slot) { return rk_null; }
  bitset_set(data, cap, free_slot - 1);
  void* ptr = (char*)self + rk_align_up(bitset_bytes(cap), align) + elsize * (free_slot - 1);
  rk_memcpy(ptr, obj, elsize);
  return ptr;
}
static_fun rk_forceinline rk_alloc_alignsize(2, 1) void* RK__Spool_put(size_t elsize, size_t align,
                                                                       size_t cap,
                                                                       void* restrict self,
                                                                       void* restrict obj) {
  void* res = RK__Spool_try_put(elsize, align, cap, self, obj);
  RK_POOL_FAIL(res, self, rk_null, align, elsize);
  return res;
}
static_fun rk_forceinline rk_alloc_alignsize(2, 1) void* RK__Dpool_try_put(
    size_t elsize, size_t align rk_unused, RK__pool_dynamic* restrict self, void* restrict obj) {
  if (!self->cap) { return rk_null; }
  size_t free_slot = bitset_first_trailing_zero(self->data, self->cap);
  if (!free_slot) { return rk_null; }
  bitset_set(self->data, self->cap, free_slot - 1);
  return rk_memcpy((char*)self->els + elsize * (free_slot - 1), obj, elsize);
}
static_fun rk_forceinline rk_alloc_alignsize(2,
                                             1) void* RK__Dpool_put(size_t elsize, size_t align,
                                                                    RK__pool_dynamic* restrict self,
                                                                    void* restrict obj) {
  void* res = RK__Dpool_try_put(elsize, align, self, obj);
  RK_POOL_FAIL(res, self, rk_null, align, elsize);
  return res;
}
#define RK__pool_try_put(self, el)                                                                 \
  RK__pool_dispatch(                                                                               \
      self,                                                                                        \
      RK__Spool_try_put(sizeof(*(self)->els), alignof(RK__poolT(self)),                            \
                        rk_COUNTOF((self)->RK__POOL_ELS), self, (RK__poolT(self)[1]){el}),         \
      RK__Dpool_try_put(sizeof(*(self)->els), alignof(RK__poolT(self)),                            \
                        (RK__pool_dynamic*)&((self)->_pool), (RK__poolT(self)[1]){el}))

#define RK__pool_put(self, el)                                                                     \
  RK__pool_dispatch(self,                                                                          \
                    RK__Spool_put(sizeof(*(self)->els), alignof(RK__poolT(self)),                  \
                                  rk_COUNTOF((self)->RK__POOL_ELS), self,                          \
                                  (RK__poolT(self)[1]){el}),                                       \
                    RK__Dpool_put(sizeof(*(self)->els), alignof(RK__poolT(self)),                  \
                                  (RK__pool_dynamic*)&((self)->_pool), (RK__poolT(self)[1]){el}))

static_fun rk_forceinline void RK__Dpool_release(size_t elsize, size_t align,
                                                 RK__pool_dynamic* self) {
  if (!self->cap) { return; }
  alloc_deallocate(self->els, rk_mult(elsize, self->cap), align RK_IFALLOC(, self->alloc));
  alloc_delete(self->data, bitset_words(self->cap) RK_IFALLOC(, self->alloc));
  self->cap = 0, self->data = rk_null, self->els = rk_null;
}

#define RK__pool_release(self)                                                                     \
  RK__pool_dispatch(self, (void)bitset_clear_all((self)->data, rk_COUNTOF((self)->RK__POOL_ELS)),  \
                    RK__Dpool_release(sizeof(*(self)->els), alignof(RK__poolT(self)),              \
                                      (RK__pool_dynamic*)&((self)->_pool)))

/// to prevent inactive union member access in c++
#define RK__pool_els(self)                                                                         \
  RK__pool_dispatch((self), (self)->els, (RK__poolT(self)*)(self)->_pool.els)

#define RK__pool_foreach(self, it)                                                                 \
  for (typeof(self) RK___pool = (self); RK___pool; RK___pool = rk_null)                            \
    for (size_t RK___cap = pool_cap(RK___pool), RK___i = (size_t)-1;                               \
         (RK___i = bitset_find_next_set(RK___pool->data, RK___cap, RK___i)) != (size_t)-1;)        \
      for (RK__poolT(RK___pool)*const it = RK__pool_els(RK___pool) + RK___i, *RK___once = it;      \
           RK___once; RK___once = 0)

#define RK__STATOVERLOAD__(m, ...) rk_CONC(m, rk_ARGCOUNT(__VA_ARGS__))(__VA_ARGS__)
#define RK__STATOVERLOAD(m, ...)   rk_CONC(m, rk_ARGCOUNT(__VA_ARGS__))(__VA_ARGS__)

/// @endcond

RK_HEADER_END
/// @}

#endif // RK_POOL_H

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
/* END INLINE: include/rk_pool.h */
/* inlined from include/rklib.h:17: #include "rk_dict.h" */
/* BEGIN INLINE: include/rk_dict.h */
// SPDX-License-Identifier: MIT
/// @file rk_dict.h
/// @version 1.0.0
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
/* inlined from include/rk_dict.h:97: #include "rk_alloc.h" */
/* skipped already-included: "include/rk_alloc.h" */
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

/// @brief `size_t dict_cap(Dict(K, V)* self)` - Returns the current slot capacity of the Dict.
/// Always a power of two.
#define dict_cap(self)            ((size_t)((self)->cap))

#if RK_CUSTOM_ALLOCATORS
# define dict_allocator(self) rk_to_rvalue((self)->alloc)
#else
# define dict_allocator(self) ((void)(self), alloc_ctx)
#endif

/// @brief `bool dict_is_empty(Dict(K, V)* self)` - Returns `true` iff the dict contains no
/// elements.
#define dict_is_empty(self)                    ((bool)(dict_count(self) == 0))

/// @brief `float dict_load_factor(Dict(K, V)* self)` - Returns the current load factor (live
/// entries / capacity). Rehash is triggered when the combined live-and-tombstone load exceeds
/// `RK_DICT_LOAD_NUM / RK_DICT_LOAD_DEN`.
#define dict_load_factor(self)                 ((float)RK__ds_load_factor(&(self)->hdr))

/// @brief `Dict(K, V)* dict_clear(K, V, Dict(K, V)* self)` - Marks all slots in the Dict as free,
/// allowing reuse of its memory.
/// @return `self`, for chaining.
#define dict_clear(K, V, self)                 RK__DICT_PUB(K, V, clear)(self)

/// @brief `Dict(K, V)* dict_reserve(K, V, Dict(K, V)* self, size_t n)` - Reserves and rehashes the
/// Dict so that it can hold at least `n` live entries without triggering another automatic rehash.
/// @return `self`, for chaining.
/// @note `n` counts live entries, not table slots — the underlying table capacity (see
/// `dict_cap`) is sized up to account for the load factor (`RK_DICT_LOAD_NUM`/`RK_DICT_LOAD_DEN`).
#define dict_reserve(K, V, self, n)            RK__DICT_PUB(K, V, reserve)(self, n)

/// @brief `Dict(K, V)* dict_shrink_to_fit(K, V, Dict(K, V)* self)` - Rehashes the Dict down to the
/// smallest table capacity that still keeps its live entries under the load factor threshold
/// (`RK_DICT_LOAD_NUM`/`RK_DICT_LOAD_DEN`), also clearing any accumulated tombstones.
/// @return `self`, for chaining.
/// @note Frees the table entirely if the Dict is empty. A no-op if already at or below the target
/// capacity.
#define dict_shrink_to_fit(K, V, self)         RK__DICT_PUB(K, V, shrink_to_fit)(self)

/// @brief `Dict(K, V)* dict_assign(K, V, Dict(K, V)* self, const K* keys, const V* vals, size_t n)`
/// - Replaces the Dict's contents with `n` key-value pairs from the parallel `keys`/`vals` arrays,
/// reusing the existing table (growing it if necessary) rather than allocating a new one.
/// @return `self`, for chaining.
#define dict_assign(K, V, self, keys, vals, n) RK__DICT_PUB(K, V, assign)(self, keys, vals, n)

/// @brief Retrieves the value for `key`, or `NULL` if absent. Returns `V*` for a mutable Dict
/// and `const V*` for a const Dict.
#define dict_get(K, V, self, key)                                                                  \
  _Generic((self),                                                                                 \
      const Dict(K, V)*: RK__DICT_PUB(K, V, get_const),                                            \
      default: RK__DICT_PUB(K, V, get))((self), (key))

/// @brief `bool dict_contains(K, V, const Dict(K, V)* self, K key)` - Checks whether the given key
/// is present in the Dict.
/// @return `true` if `self` contains the key, `false` otherwise
#define dict_contains(K, V, self, key) RK__DICT_PUB(K, V, contains)(self, key)

/// @brief `bool dict_set(K, V, Dict(K, V)* self, K key, V val)` - Sets the value at `key` in the
/// Dict to `val`, updating it if it is present or inserting a new one if not; resizes the Dict if
/// necessary.
/// @return `true` if inserted, `false` if updated
#define dict_set(K, V, self, key, val) RK__DICT_PUB(K, V, set)(self, key, val)

/// @brief `V* dict_add(K, V, Dict(K, V)* self, K key, V val)` - Inserts a value into the Dict only
/// if the key is not already present; resizes the Dict if necessary.
/// @return Pointer to the inserted value, or `NULL` if the key already existed
#define dict_add(K, V, self, key, val) RK__DICT_PUB(K, V, add)(self, key, val)

/// @brief `V* dict_get_or_add(K, V, Dict(K, V)* self, K key, V default_value, bool* inserted_out)`
/// - Returns a pointer to the value for `key`, inserting `default_value` first if the key is
/// absent; resizes the Dict if necessary.
/// @param default_value Value to insert if `key` is not present.
/// @param inserted_out Set to `true` if a new entry was inserted, `false` if the key already
/// existed. Must not be `NULL`.
/// @return Pointer to the value for `key` (never `NULL`).
#define dict_get_or_add(K, V, self, key, default_value, inserted_out)                              \
  RK__DICT_PUB(K, V, get_or_add)(self, key, default_value, inserted_out)

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
/// @param _key     Chosen name for each key pointer (`const K*`)
/// @param _val     Chosen name for each value pointer (`V*` for a mutable Dict, `const V*` for a
///                 const Dict)
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
        for (typeof(*RK__DICT_VALUE_PTR(RK___dict))*const _val       = &(RK___dict->vals[RK___i]), \
                                                          *RK__ONCE1 = _val;                       \
             RK__ONCE1; RK__ONCE1                                    = 0)

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
/// @param _val  Chosen name of the value pointer (`V*` for a mutable Dict, `const V*` for a const
///              Dict) for each iteration
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
      for (typeof(*RK__DICT_VALUE_PTR(RK___dict))*const _val                                       \
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

/// @brief `size_t set_cap(Set(K)* self)` - Returns the current slot capacity of the Set. Always a
/// power of two.
#define set_cap(self)                    ((size_t)((self)->cap))

#if RK_CUSTOM_ALLOCATORS
# define set_allocator(self) rk_to_rvalue((self)->alloc)
#else
# define set_allocator(self) ((void)(self), alloc_ctx)
#endif

/// @brief `bool set_is_empty(Set(K)* self)` - Returns `true` iff the set contains no elements.
#define set_is_empty(self)           ((bool)(set_count(self) == 0))

/// @brief `float set_load_factor(Set(K)* self)` - Returns the current load factor (live entries /
/// capacity). See `dict_load_factor()`.
#define set_load_factor(self)        ((float)RK__ds_load_factor(&(self)->hdr))

/// @brief `Set(K)* set_clear(K, Set(K)* self)` - Marks all slots in the Set as free, allowing reuse
/// of its memory.
/// @return `self`, for chaining.
#define set_clear(K, self)           RK__SET_PUB(K, clear)(self)

/// @brief `Set(K)* set_reserve(K, Set(K)* self, size_t n)` - Reserves and rehashes the Set so that
/// it can hold at least `n` live entries without triggering another automatic rehash. See
/// `dict_reserve()`.
/// @return `self`, for chaining.
#define set_reserve(K, self, n)      RK__SET_PUB(K, reserve)(self, n)

/// @brief `Set(K)* set_shrink_to_fit(K, Set(K)* self)` - Rehashes the Set down to the smallest
/// table capacity that still keeps its live entries under the load factor threshold. See
/// `dict_shrink_to_fit()`.
/// @return `self`, for chaining.
#define set_shrink_to_fit(K, self)   RK__SET_PUB(K, shrink_to_fit)(self)

/// @brief `Set(K)* set_assign(K, Set(K)* self, const K* keys, size_t n)` - Replaces the Set's
/// contents with `n` keys from `keys`, reusing the existing table (growing it if necessary) rather
/// than allocating a new one.
/// @return `self`, for chaining.
#define set_assign(K, self, keys, n) RK__SET_PUB(K, assign)(self, keys, n)

/// @brief `bool set_contains(K, const Set(K)* self, K key)`
/// - Checks whether the given key is present in the Set.
/// @return `true` if `self` contains the key, `false` otherwise
#define set_contains(K, self, key)   RK__SET_PUB(K, contains)(self, key)

/// @brief `bool set_add(K, Set(K)* self, K key)` - Ensures a key is present in a set; resizes the
/// Set if necessary.
/// @return `true` if the key was inserted, `false` if it was already present.
#define set_add(K, self, key)        RK__SET_PUB(K, add)(self, key)

/// @brief `bool set_remove(K, Set(K)* self, K key)` - Removes a key from the Set if it is present.
/// @return `true` if the value was found and removed, `false` otherwise
#define set_remove(K, self, key)     RK__SET_PUB(K, remove)(self, key)

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
#define set_foreach(self, key)       dict_foreach_key(self, key)

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

// `vals` remains a mutable pointer when the Dict object is const. Propagate the
// container's constness to the pointers exposed during iteration.
#define RK__DICT_VALUE_PTR(self)                                                                   \
  _Generic((self),                                                                                 \
      const typeof(*(self))*: (const typeof((self)->vals[0])*)0,                                   \
      default: (typeof((self)->vals))0)

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
// The shift always yields a 7-bit value (0-127), which always fits in u8 --
// every call site assigns straight into a u8, so the cast belongs here once
// rather than at each site.
#define RK__DS_fp(hash)                       ((u8)((hash) >> (bitsof(hash) - 7)))

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
#define RK__DS_DEF(K, V, hash_f, cmp_f, IF_DICT, IF_SET, DSTYPE, PUBF, PRIF)                         \
  RK_EXTERNC_BEG                                                                                     \
  typedef struct DSTYPE(K, V) {                                                                      \
    union {                                                                                          \
      RK__ds_header hdr;                                                                             \
      struct { size_t cap, count, ndeleted; };                                                       \
    };                                                                                               \
    u8* data;                                                                                        \
    K*  keys;                                                                                        \
    IF_DICT(V* vals;)                                                                                \
    RK_IFALLOC(Allocator alloc;)                                                                     \
  } DSTYPE(K, V);                                                                                    \
  static_fun DSTYPE(K, V) PUBF(K, V, init)(size_t cap RK_IFALLOC(, Allocator alloc)) {               \
    rk_assert_allocator_valid(alloc);                                                                \
    cap = stdc_bit_ceil(rk_MAX(16u, cap));                                                           \
    return (DSTYPE(K, V)){.hdr  = {.cap = cap, .count = 0, .ndeleted = 0},                           \
                          .data = (u8*)memset(alloc_allocate(cap, align_max RK_IFALLOC(, alloc)),    \
                                              RK__DS_SLOT_EMPTY, cap),                               \
                          .keys = alloc_new(K, cap RK_IFALLOC(, alloc)),                             \
                          IF_DICT(.vals = alloc_new(V, cap RK_IFALLOC(, alloc)), )                   \
                              RK_IFALLOC(.alloc = alloc)};                                           \
  }                                                                                                  \
  static_fun void PUBF(K, V, release)(DSTYPE(K, V) * self) {                                         \
    if rk_unlikely (!self->cap) { return; }                                                          \
    alloc_deallocate(self->data, self->cap, align_max RK_IFALLOC(, self->alloc));                    \
    self->data = rk_null;                                                                            \
    alloc_delete(self->keys, self->cap RK_IFALLOC(, self->alloc));                                   \
    self->keys = rk_null;                                                                            \
    IF_DICT(alloc_delete(self->vals, self->cap RK_IFALLOC(, self->alloc)), self->vals = rk_null;)    \
    self->cap = self->count = self->ndeleted = 0;                                                    \
  }                                                                                                  \
  static_fun void PRIF(K, V, grow)(DSTYPE(K, V) * self, size_t new_cap) {                            \
    const DSTYPE(K, V) old_self = *self;                                                             \
    DSTYPE(K, V)                                                                                     \
    new_self                                                                                         \
        = {.hdr  = {.cap = new_cap, .count = old_self.count, .ndeleted = 0},                         \
           .data = (u8*)memset(alloc_allocate(new_cap, align_max RK_IFALLOC(, old_self.alloc)),      \
                               RK__DS_SLOT_EMPTY, new_cap),                                          \
           .keys = alloc_new(K, new_cap RK_IFALLOC(, old_self.alloc)),                               \
           IF_DICT(.vals = alloc_new(V, new_cap RK_IFALLOC(, old_self.alloc)), )                     \
               RK_IFALLOC(.alloc = old_self.alloc)};                                                 \
    const size_t mask = new_self.cap - 1;                                                            \
    for (size_t oldcap = old_self.cap, i = 0; i < oldcap; ++i) {                                     \
      if (RK__DS_SLOT_EMPTY_OR_DELETED(old_self.data[i])) { continue; }                              \
      K      key  = old_self.keys[i];                                                                \
      u64    hash = (u64)hash_f(key);                                                                \
      size_t j    = RK__DS_home(mask, hash);                                                         \
      for (; new_self.data[j] != RK__DS_SLOT_EMPTY; j = RK__DS_next(mask, j));                       \
      new_self.data[j] = RK__DS_fp(hash);                                                            \
      new_self.keys[j] = key;                                                                        \
      IF_DICT(new_self.vals[j] = old_self.vals[i];)                                                  \
    }                                                                                                \
    PUBF(K, V, release)(self);                                                                       \
    *self = new_self;                                                                                \
  }                                                                                                  \
  static_fun void PRIF(K, V, ensure_cap)(DSTYPE(K, V) * self) {                                      \
    if (!self->cap) {                                                                                \
      RK_IFALLOC(rk_set_alloc_fallback(self->alloc);)                                                \
      *self = PUBF(K, V, init)(16u RK_IFALLOC(, self->alloc));                                       \
    };                                                                                               \
    if (RK__ds_needs_rehash(&self->hdr)) {                                                           \
      PRIF(K, V, grow)(self,                                                                         \
                       (rk_mult(self->count, 2) > self->cap) ? rk_mult(self->cap, 2) : self->cap);   \
    }                                                                                                \
  }                                                                                                  \
  static_fun RK__hashprobe_t PRIF(K, V, probe_f)(const DSTYPE(K, V)* restrict self, K key,           \
                                                 u64 hash) {                                         \
    u8           fp   = RK__DS_fp(hash);                                                             \
    const size_t mask = self->cap - 1;                                                               \
    size_t       i = RK__DS_home(mask, hash), fd = RK_DS_NOTIN;                                      \
    u8* const restrict data = self->data;                                                            \
    K* const restrict keys  = self->keys;                                                            \
    for (; data[i] != RK__DS_SLOT_EMPTY; i = RK__DS_next(mask, i)) {                                 \
      if (data[i] == RK__DS_SLOT_DELETED) {                                                          \
        if (fd == RK_DS_NOTIN) { fd = i; }                                                           \
      } else if (data[i] == fp && !cmp_f(key, keys[i])) {                                            \
        return RK__PROBE_MAKE(1, 0, i);                                                              \
      }                                                                                              \
    }                                                                                                \
    return (fd != RK_DS_NOTIN) ? RK__PROBE_MAKE(0, 1, fd) : RK__PROBE_MAKE(0, 0, i);                 \
  }                                                                                                  \
  static_fun bool PUBF(K, V, contains)(const DSTYPE(K, V)* restrict self, K key) {                   \
    if rk_unlikely (!self->cap) { return false; }                                                    \
    return RK__PROBE_FOUND(PRIF(K, V, probe_f)(self, key, (u64)hash_f(key)));                        \
  }                                                                                                  \
  static_fun DSTYPE(K, V) * PUBF(K, V, clear)(DSTYPE(K, V)* restrict self) {                         \
    rk_memset(self->data, RK__DS_SLOT_EMPTY, self->cap);                                             \
    self->count = self->ndeleted = 0;                                                                \
    return self;                                                                                     \
  }                                                                                                  \
  static_fun DSTYPE(K, V) * PUBF(K, V, reserve)(DSTYPE(K, V)* restrict self, size_t n) {             \
    if (!n) { return self; }                                                                         \
    size_t cap = stdc_bit_ceil(                                                                      \
        rk_MAX(16u, (n * RK_DICT_LOAD_DEN + RK_DICT_LOAD_NUM - 1) / RK_DICT_LOAD_NUM));              \
    if (!self->cap) {                                                                                \
      RK_IFALLOC(rk_set_alloc_fallback(self->alloc);)                                                \
      *self = PUBF(K, V, init)(cap RK_IFALLOC(, self->alloc));                                       \
    } else if (cap > self->cap) {                                                                    \
      PRIF(K, V, grow)(self, cap);                                                                   \
    }                                                                                                \
    return self;                                                                                     \
  }                                                                                                  \
  static_fun DSTYPE(K, V) * PUBF(K, V, shrink_to_fit)(DSTYPE(K, V)* restrict self) {                 \
    if (!self->count) {                                                                              \
      PUBF(K, V, release)(self);                                                                     \
      return self;                                                                                   \
    }                                                                                                \
    size_t target = stdc_bit_ceil(                                                                   \
        rk_MAX(16u, (self->count * RK_DICT_LOAD_DEN + RK_DICT_LOAD_NUM - 1) / RK_DICT_LOAD_NUM));    \
    if (target < self->cap) { PRIF(K, V, grow)(self, target); }                                      \
    return self;                                                                                     \
  }                                                                                                  \
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
      static_fun const V* PUBF(K, V, get_const)(const DSTYPE(K, V)* restrict self, K key) {        \
        if rk_unlikely (!self->cap) { return rk_null; }                                            \
        RK__hashprobe_t r = PRIF(K, V, probe_f)(self, key, (u64)hash_f(key));                      \
        return RK__PROBE_FOUND(r) ? &self->vals[RK__PROBE_IDX(r)] : rk_null;                       \
      } /*                                                           */                            \
      static_fun V* PUBF(K, V, get)(DSTYPE(K, V)* restrict self, K key) {                          \
        return (V*)PUBF(K, V, get_const)(self, key);                                               \
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
      } /*                                                           */                            \
      static_fun DSTYPE(K, V) * PUBF(K, V, assign)(DSTYPE(K, V)* restrict self, const K* keys,     \
                                                    const V* vals, size_t n) {                      \
        PUBF(K, V, clear)(self);                                                                   \
        for (size_t i = 0; i < n; ++i) { PUBF(K, V, set)(self, keys[i], vals[i]); }                 \
        return self;                                                                               \
      }) \
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
      } /*                                                           */                            \
      static_fun DSTYPE(K, V) * PUBF(K, V, assign)(DSTYPE(K, V)* restrict self, const K* keys,     \
                                                    size_t n) {                                     \
        PUBF(K, V, clear)(self);                                                                   \
        for (size_t i = 0; i < n; ++i) { PUBF(K, V, add)(self, keys[i]); }                          \
        return self;                                                                               \
      }) \
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
/* END INLINE: include/rk_dict.h */
// clang-format on
#endif // RK_LIB_H

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
/* END INLINE: include/rklib.h */
