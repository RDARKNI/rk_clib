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
/// @defgroup rk_config rklib Configuration
/// @brief Compile-time configuration for rklib.
///
/// This file is the central configuration point for rklib. Options may either be changed directly
/// here or defined by the build system before including any rklib header.
///
/// Options relating only to headers that are not included have no effect.
///
/// @par Translation-unit configuration
///
/// By default, rklib is header-only. Functions and mutable library state have
/// translation-unit-local definitions where necessary.
///
/// Define `RK_MULTI_TU` to use the multi-translation-unit model instead. In that mode, rklib
/// entities that require one program-wide definition use external linkage.
///
/// When `RK_MULTI_TU` is enabled, exactly one translation unit must additionally define `RK_IMPL`
/// before including any rklib header:
///
/// @code
/// #define RK_IMPL
/// #include <rklib.h>
/// @endcode
///
/// `RK_IMPL` is intentionally not a global configuration option: it identifies the one translation
/// unit that provides rklib's external definitions.
///
/// @warning When `RK_MULTI_TU` is enabled, all translation units must use the same rklib
/// configuration. Configuration mismatches may produce incompatible object layouts or function
/// definitions.
///
/// Even in header-only mode, objects exchanged between translation units must of course have been
/// compiled with compatible layout-affecting options.
///
/// @par Allocators
///
/// `RK_CUSTOM_ALLOCATORS` controls whether the allocator abstraction is carried through owning
/// rklib objects.
///
/// When enabled, owning objects can store an `Allocator`, allocator-taking overloads are available,
/// and allocation is dispatched through the selected allocator.
///
/// When disabled, allocator fields and allocator-taking overloads are compiled out and allocations
/// use the malloc-backed implementation directly.
///
/// `RK_ALLOC_CTX_THREAD_LOCAL` controls the storage duration of `alloc_ctx`. When enabled, each
/// thread has its own default allocator context. It does not itself make allocator implementations
/// thread-safe.
///
/// @par Allocation failure
///
/// Ordinary allocation APIs are infallible from the caller's perspective. Allocation failure is
/// handled by the allocator-specific failure macros below.
///
/// A custom failure handler must not return normally. It should terminate execution, `longjmp`, or
/// otherwise transfer control away from the failed allocation.
///
/// @par Size overflow
///
/// `rk_mult(x, y)` is used for size calculations throughout the library. By default it performs
/// ordinary unchecked multiplication. It may be redefined to `rk_mult_safe(x, y)` to abort on size
/// overflow.
///
/// @{

#ifndef RK_CONFIG_H
#define RK_CONFIG_H

#if defined(__cplusplus) && !defined(__clang__)
# error "C++ support only for Clang"
#elif defined(_MSC_VER) && _MSC_VER < 1939
# error "Unsupported MSVC version"
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// User Configuration ////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

// This section lists every supported user-facing compile-time configuration option.
//
// Either edit the definitions here or provide equivalent definitions through the build system.
// Options with their default value shown do not need to be explicitly defined.

// Build model:
//
// #define RK_MULTI_TU
//
// When RK_MULTI_TU is enabled, define RK_IMPL separately in exactly one translation unit before
// including any rklib header. Do NOT define RK_IMPL here.

// Diagnostics:
//
// #define RKLIB_DEBUG
//
// Define RKLIB_DEBUG to enable debug logging and additional diagnostic output. Leave it undefined
// to disable debug mode.

// Allocators:
//
// #define RK_CUSTOM_ALLOCATORS       1
// #define RK_ALLOC_CTX_THREAD_LOCAL  0
//
// RK_CUSTOM_ALLOCATORS:
//   1 - owning objects support arbitrary Allocator instances.
//   0 - allocator state and allocator-taking overloads are compiled out; malloc is used directly.
//
// RK_ALLOC_CTX_THREAD_LOCAL:
//   1 - alloc_ctx is thread_local, giving each thread its own default allocator.
//   0 - alloc_ctx is shared normally.
// This option does not make the allocators themselves thread-safe.

// Allocation failure handlers:
//
// #define RK_MALLOC_FAIL(cond, ctx, old_ptr, align, new_size) ...
// #define RK_MMAP_FAIL( cond, ctx, old_ptr, align, new_size) ...
// #define RK_ARENA_FAIL(cond, ctx, old_ptr, align, new_size) ...
// #define RK_POOL_FAIL( cond, ctx, old_ptr, align, new_size) ...
//
// Defaults assert and abort. Custom handlers must not return normally.

// Dictionary:
//
// #define RK_DICT_LOAD_NUM 3
// #define RK_DICT_LOAD_DEN 4
//
// Maximum dictionary load factor, expressed as RK_DICT_LOAD_NUM / RK_DICT_LOAD_DEN.

// Size arithmetic:
//
// #define rk_mult(x, y) rk_mult_safe((x), (y))
//
// Uncomment to make dynamic size multiplication fail-fast on overflow.
// The default is unchecked multiplication.

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// End User Configuration //////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef RK_CUSTOM_ALLOCATORS
# define RK_CUSTOM_ALLOCATORS 1
#endif

#ifndef RK_ALLOC_CTX_THREAD_LOCAL
# define RK_ALLOC_CTX_THREAD_LOCAL 0
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// Allocation Failure Policy /////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

/// @defgroup alloc_failure_handlers Allocator Failure Handlers
/// @brief Customizable failure handlers used by the built-in allocators.
///
/// All handlers receive:
/// - `cond`:     the allocator's success condition;
/// - `ctx`:      allocator-specific context;
/// - `old_ptr`:  previous allocation when applicable;
/// - `align`:    requested alignment;
/// - `new_size`: requested allocation size.
///
/// A handler is invoked only when `cond` is false and must not return normally.
///
/// @{

#ifndef RK_MALLOC_FAIL

/// @brief Failure handler for the malloc-backed allocator.
# define RK_MALLOC_FAIL(cond, ctx, old_ptr, align, new_size)                                       \
   do {                                                                                            \
     (void)(ctx), (void)(old_ptr), (void)(align), (void)(new_size);                                \
     rk_assert((cond) && "malloc allocation failure");                                             \
     (cond) ? (void)0 : abort();                                                                   \
   } while (0)

#endif

#ifndef RK_MMAP_FAIL

/// @brief Failure handler for OS page allocation (`mmap` / `VirtualAlloc`).
# define RK_MMAP_FAIL(cond, ctx, old_ptr, align, new_size)                                         \
   do {                                                                                            \
     (void)(ctx), (void)(old_ptr), (void)(align), (void)(new_size);                                \
     rk_assert((cond) && "map allocation failure");                                                \
     (cond) ? (void)0 : abort();                                                                   \
   } while (0)

#endif

#ifndef RK_ARENA_FAIL

/// @brief Failure handler for arena allocation. `ctx` is the corresponding `Arena *`.
# define RK_ARENA_FAIL(cond, ctx, old_ptr, align, new_size)                                        \
   do {                                                                                            \
     (void)(ctx), (void)(old_ptr), (void)(align), (void)(new_size);                                \
     rk_assert((cond) && "arena allocation failure");                                              \
     (cond) ? (void)0 : abort();                                                                   \
   } while (0)

#endif

#ifndef RK_POOL_FAIL

/// @brief Failure handler for pool allocation. `ctx` is the corresponding pool pointer.
# define RK_POOL_FAIL(cond, ctx, old_ptr, align, new_size)                                         \
   do {                                                                                            \
     (void)(ctx), (void)(old_ptr), (void)(align), (void)(new_size);                                \
     rk_assert((cond) && "pool allocation failure");                                               \
     (cond) ? (void)0 : abort();                                                                   \
   } while (0)

#endif

/// @}

////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// Arithmetic Policy /////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Multiplication used for dynamic byte-size calculations.
///
/// The default performs unchecked `size_t` multiplication. Define `rk_mult(x, y)` yourself, for
/// example as `rk_mult_safe((x), (y))`, to use a checked policy instead.
#ifndef rk_mult
# define rk_mult(x, y) ((x) * (y))
#endif

/// @}
#endif // RK_CONFIG_H

// MIT License
//
// Copyright (c) 2026 Dariusch Knigge
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
// associated documentation files (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish, distribute,
// sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
// NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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
# define rk_has_c_attribute(x) __has_c_attribute(x)
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
# define rk_attribute(...) __attribute__((__VA_ARGS__))
#else
# define rk_attribute(...)
#endif

#ifdef _MSC_VER
# define rk_declspec(attr) __declspec(attr)
#else
# define rk_declspec(attr)
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
# define RKI_STDBIT_FALLBACK 1
#else
# define RKI_STDBIT_FALLBACK 0
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
# define RKI_SILENCE_WARNINGS_BEG                                                                  \
   RK_DO_PRAGMA(clang diagnostic push)                                                             \
   RK_DO_PRAGMA(clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments")                    \
   RK_DO_PRAGMA(clang diagnostic ignored "-Wkeyword-macro")                                        \
   RK_DO_PRAGMA(clang diagnostic ignored "-Wunknown-warning-option")                               \
   RK_DO_PRAGMA(clang diagnostic ignored "-Wc2x-extensions")                                       \
   RK_DO_PRAGMA(clang diagnostic ignored "-Wc23-extensions")                                       \
   RK_DO_PRAGMA(clang diagnostic ignored "-Wc2y-extensions")                                       \
   RK_DO_PRAGMA(clang diagnostic ignored "-Wunknown-attributes")                                   \
   RK_DO_PRAGMA(clang diagnostic ignored "-Wc++17-extensions")

# define RKI_SILENCE_WARNINGS_END RK_DO_PRAGMA(clang diagnostic pop)

# define RKI_IGNWARN_CLANG_BEG(warn)                                                               \
   RK_DO_PRAGMA(clang diagnostic push)                                                             \
   RK_DO_PRAGMA(clang diagnostic ignored warn)
# define RKI_IGNWARN_CLANG_END() RK_DO_PRAGMA(clang diagnostic pop)
# define RKI_IGNWARN_CLANG(warn, ...)                                                              \
   RKI_IGNWARN_CLANG_BEG(warn)                                                                     \
   (__VA_ARGS__) RKI_IGNWARN_CLANG_END()
#else
# define RKI_IGNWARN_CLANG_BEG(warn)
# define RKI_IGNWARN_CLANG_END()
# define RKI_IGNWARN_CLANG(warn, ...) (__VA_ARGS__)
#endif

#if defined(_MSC_VER) && !defined(__clang__)
/// disable the following (in this case) erroneous warnings: 4200 flexible array members (supported
/// on every targeted platform) 4116 Warnings about anonymous compound literals (not an issue in C11
/// mode) 4141 Warnings about inline used twice (due to forceinline macro)
/// @note excludes clang-cl (defines both __clang__ and _MSC_VER): it already gets
/// RKI_SILENCE_WARNINGS_BEG/END from the __clang__ branch above, and this branch must not redefine
/// them with different (MSVC-pragma) bodies.
# define RKI_SILENCE_WARNINGS_BEG                                                                  \
   __pragma(warning(push)) __pragma(warning(disable : 4200 4116 4141))
# define RKI_SILENCE_WARNINGS_END   __pragma(warning(pop))
# define RKI_IGNWARN_MSC_BEG(warn)  __pragma(warning(push)) __pragma(warning(disable : warn))
# define RKI_IGNWARN_MSC_END()      __pragma(warning(pop))
# define RKI_IGNWARN_MSC(warn, ...) RKI_IGNWARN_MSC_BEG(warn) __VA_ARGS__ __pragma(warning(pop))
#else
# define RKI_IGNWARN_MSC_BEG(warn)
# define RKI_IGNWARN_MSC_END()
# define RKI_IGNWARN_MSC(warn, ...) __VA_ARGS__
# ifndef RKI_SILENCE_WARNINGS_BEG
#  define RKI_SILENCE_WARNINGS_BEG
#  define RKI_SILENCE_WARNINGS_END
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

#if !defined(_WIN32) && defined(RK_MULTI_TU)
# define RKI_HIDDEN __attribute__((visibility("hidden")))
#else
# define RKI_HIDDEN
#endif

#define static_fun static inline
#if !defined(RK_MULTI_TU)
# define extern_fun      static inline
# define extern_var      static
# define extern_def(...) = __VA_ARGS__
# define rklib_fun       extern_fun
#else
# ifdef RK_IMPL
#  ifndef __cplusplus
#   ifndef __GNUC_GNU_INLINE__
#    define extern_fun extern inline
#   else
#    define extern_fun inline
#   endif
#  else
// C's `extern inline` (or, under the older GNU inline dialect, plain `inline`) guarantees that the
// RK_IMPL translation unit emits a real, callable definition regardless of whether that TU calls
// the function itself -- other TUs are relying on this one to provide it. C++ has no inline form
// with that guarantee: plain `inline` only *permits* emitting a definition, it never requires one,
// so a function this file happens not to call itself would silently vanish, breaking any other TU
// that calls it. __attribute__((used)) closes that gap by forcing emission unconditionally, while
// keeping `inline` itself so the definition stays ODR-mergeable if it ever does end up duplicated
// across translation units (this project's C++ support is Clang-only, so no non-GNU-attribute
// fallback is needed here).
#   if rk_has_gnu_attribute(used)
#    define extern_fun __attribute__((used)) inline
#   else
#    define extern_fun inline
#   endif
#  endif
#  if defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606L
#   define extern_var      inline
#   define extern_def(...) = __VA_ARGS__
#  else
#   define extern_var
#   define extern_def(...) = __VA_ARGS__
#  endif
# else
#  ifndef __cplusplus
#   ifndef __GNUC_GNU_INLINE__
#    define extern_fun inline
#   else
#    define extern_fun extern inline
#   endif
#  else
#   define extern_fun inline
#  endif
#  if defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606L
#   define extern_var      inline
#   define extern_def(...) = __VA_ARGS__
#  else
#   define extern_var extern
#   define extern_def(...)
#  endif
# endif
# define rklib_fun RKI_HIDDEN extern_fun
#endif

#define RKI_HEADER_BEGIN RKI_SILENCE_WARNINGS_BEG RK_EXTERNC_BEG
#define RKI_HEADER_END   RK_EXTERNC_END RKI_SILENCE_WARNINGS_END

RKI_HEADER_BEGIN

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

#if defined(__GNUC__) || defined(__clang__)
# define rk_malloc_fun            rk_attribute(__malloc__)
# define rk_attr_printf(beg, end) rk_attribute(__format__(__printf__, beg, end))
# define rk_pure                  rk_attribute(__pure__)
# define rk_const                 rk_attribute(__const__)
# define rk_forceinline           rk_attribute(__always_inline__)
#else
# define rk_attr_printf(beg, end)
# define rk_pure
# define rk_const rk_unsequenced

# ifdef _MSC_VER
#  define rk_malloc_fun  __declspec(restrict)
#  define rk_forceinline __forceinline
# else
#  define rk_malloc_fun
#  define rk_forceinline
# endif
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

#define rk_alloc_alignsize(align, ...) rk_alloc_align(align) rk_alloc_size(__VA_ARGS__)

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
#  define typeof(...) rki_remove_reference_t<__typeof__(__VA_ARGS__)>
# else
#  define typeof __typeof__
# endif
#endif

#ifndef countof
# ifndef __cplusplus
#  define rk_COUNTOF(...) (sizeof(__VA_ARGS__) / sizeof((__VA_ARGS__)[0]))
#  define countof(...)    (static_assert_expr(rk_is_array((__VA_ARGS__))) + rk_COUNTOF(__VA_ARGS__))
# else
#  define countof(...)    rki_countof(__VA_ARGS__)
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
#  define unreachable() (rk_assert(0 && "unreachable code reached"), std::unreachable())
# elif defined(__GNUC__)
#  define unreachable() (rk_assert(0 && "unreachable code reached"), __builtin_unreachable())
# elif defined(_MSC_VER)
rklib_fun __forceinline rk_noreturn void rki_unreachable_impl(void) {
#  if defined(_DEBUG)
  __debugbreak();
#  endif
  __assume(0);
}
#  define unreachable() rki_unreachable_impl()
# else
#  define unreachable() (rk_assert(0 && "unreachable code reached"), abort())
# endif
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   Pseudo  -  Keywords   ////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(__cplusplus)                                                                           \
    || (!defined(_MSC_VER) && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L)
# define rk_null nullptr
#else
# define rk_null ((void*)0)
#endif

/// @brief The maximum fundamental alignment.
#define align_max     alignof(rki_max_align_t)

/// @brief Aligns an object to the maximum fundamental alignment.
#define alignas_max   alignas(align_max)

/// @brief Returns the bits in a type or expression.
#define bitsof(...)   (sizeof(__VA_ARGS__) * CHAR_BIT)

/// @brief Returns the string length of a string literal.
#define lenof(strlit) (sizeof("" strlit "") - 1)

/// @brief Returns the minimum value of any C integral type.
#define minof(T)      RKI_MINOF(T)

/// @brief Returns the maximum value of any C integral type.
#define maxof(T)      RKI_MAXOF(T)

/// @brief Like the Kernel's container_of_const macro but portable
// For a const `ptr`, the `const typeof(*(ptr))*` association spells a doubled qualifier, which C
// permits but MSVC reports as C4114; `ptr` is generic, so it cannot be tested via a named member
// type the way the container iteration macros do.
#define containerof(ptr, type, member)                                                             \
  RKI_IGNWARN_MSC(                                                                                 \
      4114, ((typeof(_Generic((ptr),                                                               \
                         const typeof(*(ptr))*: (const type*)0,                                    \
                         default: (type*)0)))((char*)(typeof(((const type*)0)->member)*){ptr}      \
                                              - offsetof(type, member))))

/// @brief Overflow-checked `size_t` multiplication; `abort()`s instead of wrapping. Not used by
/// default — see the `rk_mult` config hook in `rk_config.h` to opt every size/count computation in
/// the library into this behaviour (`#define rk_mult(x, y) rk_mult_safe(x, y)`).
rklib_fun rk_forceinline size_t rk_mult_safe(size_t x, size_t y) {
  return rk_likely(x == 0 || y <= SIZE_MAX / x) ? x * y : (abort(), (size_t)0);
}

/// @brief Returns the byte size of n objects of type T.
#define sizeof_n(T, count) rk_mult(sizeof(T), count)

#define sizeof_n_static(T, count)                                                                  \
  (static_assert_expr(sizeof(T) == 0 || (count) <= SIZE_MAX / sizeof(T), "overflow")               \
   + (sizeof(T) * (count)))

/// @brief For static expression dispatch.
#if rk_has_builtin(__builtin_choose_expr)
# define rk_static_if(cond, _if, _else) __builtin_choose_expr(cond, _if, _else)
#else
# define rk_static_if(cond, _if, _else)                                                            \
   _Generic(((char (*)[1 + !!(cond)])0), char (*)[2]: _if, char (*)[1]: _else)
#endif

/// @brief static_assert-like check within expressions, evaluates to 0 if true and causes
/// compile-time error if false.
/// @param condition The condition to check for; must be a constant expression
/// @param msg The message to show upon compile error (optional since C23/C++17)
#define static_assert_expr(...) RKI_STATIC_ASSERT_EXPR(__VA_ARGS__)

// todo fix
#if rk_has_builtin(__builtin_constant_p)
# define try_static_assert_expr(expr, ...)                                                         \
   static_assert_expr(rk_static_if(__builtin_constant_p(expr), !!(expr), 1), ##__VA_ARGS__)
#else
# define try_static_assert_expr(expr, ...) ((void)0)
#endif

#ifndef __cplusplus
# define pun_cast(to_type, expr)                                                                   \
   RKI_IGNWARN_MSC(4116, rk_static_if(!rk_is_array(expr),                                          \
                                      (union {                                                     \
                                       static_assert(sizeof(typeof(expr)) == sizeof(to_type),      \
                                                     "Types must be the same size.");              \
                                       typeof_decayed(expr) f;                                     \
                                       to_type t;                                                  \
                                      }){(expr)}                                                   \
                                          .t,                                                      \
                                      *(to_type*)rk_memcpy(&(to_type){RKI_ZINIT},                  \
                                                           (union {                                \
                                                            typeof_decayed(expr) _v2;              \
                                                            const void* _v;                        \
                                                           }){(expr)}                              \
                                                               ._v,                                \
                                                           sizeof(to_type))))

#elif __cplusplus >= 202002L
# define pun_cast(to_type, expr) (std::bit_cast<to_type>((expr)))
#elif rk_has_builtin(__builtin_bit_cast)
# define pun_cast(to_type, expr) __builtin_bit_cast(to_type, expr)
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
# define RKI_IFHAS_INT128(...) __VA_ARGS__
#else
# define RKI_IFHAS_INT128(...)
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////   Function Wrappers   //////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief round a size to a power of two alignment
rklib_fun rk_const size_t      rk_align_up(size_t size, size_t align);

/// @brief todo docs
rklib_fun rk_const size_t      rk_align_pad(const void* ptr, size_t align);

rklib_fun rk_const bool        rk_ptrs_overlap(const void* beg1, const void* end1, const void* beg2,
                                               const void* end2);

rklib_fun rk_const bool        rk_ptr_in_range(const void* ptr, const void* beg, const void* end);

rklib_fun rk_forceinline void* rk_memcpy(void* const restrict dst, const void* const restrict src,
                                         size_t nbytes) {
  return nbytes ? memcpy(dst, src, nbytes) : dst;
}
rklib_fun rk_forceinline void* rk_memmove(void* dst, const void* src, size_t nbytes) {
  return nbytes ? memmove(dst, src, nbytes) : dst;
}
rklib_fun rk_forceinline void* rk_memset(void* dst, int value, size_t nbytes) {
  return nbytes ? memset(dst, value, nbytes) : dst;
}
rklib_fun rk_pure rk_forceinline int rk_memcmp(const void* a, const void* b, size_t nbytes) {
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
   (rk_likely((__VA_ARGS__)) ? (void)0 : rki_assertfail(#__VA_ARGS__, __FILE__, __LINE__, __func__))
/// print to stderr if RKLIB_DEBUG is defined
rk_noreturn rklib_fun void rki_assertfail(const char* expr, const char* file, int line,
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
#define rk_abs(x)                RKI_ABS(x)

/// @brief Returns the smaller of `x` and `y`.
#define rk_min(x, y)             RKI_TWONUMS(min_, RKI_NUM_TYPES, x, y)
/// @brief Like `rk_min()` but not type-safe and may double-evaluate args.
#define rk_MIN(a, b)             ((a) < (b) ? (a) : (b))

/// @brief Returns the larger of `x` and `y`.
#define rk_max(x, y)             RKI_TWONUMS(max_, RKI_NUM_TYPES, x, y)
/// @brief Like `rk_max()` but not type-safe and may double-evaluate args.
#define rk_MAX(a, b)             ((a) > (b) ? (a) : (b))

/// @brief Clamps `num` to the inclusive range [`low`, `high`]. Requires `low <= high`.
#define rk_clamp(num, low, high) RKI_THREENUMS(clamp_, RKI_NUM_TYPES, num, low, high)
/// @brief Like `rk_clamp()` but not type-safe and may double-evaluate args.
#define rk_CLAMP(num, low, high) ((num) < (low) ? (low) : ((num) > (high) ? (high) : (num)))

/// @brief Saturating addition, clamps to `[TYPE_MIN, TYPE_MAX]` of the common type.
#define rk_sat_add(x, y)         RKI_TWONUMS(rk_sat_add_, RKI_SU_TYPES, x, y)

/// @brief Saturating subtraction, clamps to `[TYPE_MIN, TYPE_MAX]` of the common type.
#define rk_sat_sub(x, y)         RKI_TWONUMS(rk_sat_sub_, RKI_SU_TYPES, x, y)

/// @brief Saturating multiplication, clamps to `[TYPE_MIN, TYPE_MAX]` of the common type.
#define rk_sat_mul(x, y)         RKI_TWONUMS(rk_sat_mul_, RKI_SU_TYPES, x, y)

#define rk_SWAP(a, b)                                                                              \
  do {                                                                                             \
    typeof(a)* _a      = &(a);                                                                     \
    typeof(b)* _b      = &(b);                                                                     \
    typeof(a)  RKI_TMP = *_a;                                                                      \
    *_a                = *_b;                                                                      \
    *_b                = RKI_TMP;                                                                  \
  } while (0)

/// @}

#if RKI_STDBIT_FALLBACK
# define stdc_leading_zeros(...)                                                                   \
   _Generic((__VA_ARGS__)RKI_U_TYPES(RKI_GENCASE, stdc_leading_zeros_))(__VA_ARGS__)
# define stdc_leading_ones(...)                                                                    \
   _Generic((__VA_ARGS__)RKI_U_TYPES(RKI_GENCASE, stdc_leading_ones_))(__VA_ARGS__)
# define stdc_trailing_zeros(...)                                                                  \
   _Generic((__VA_ARGS__)RKI_U_TYPES(RKI_GENCASE, stdc_trailing_zeros_))(__VA_ARGS__)
# define stdc_trailing_ones(...)                                                                   \
   _Generic((__VA_ARGS__)RKI_U_TYPES(RKI_GENCASE, stdc_trailing_ones_))(__VA_ARGS__)
# define stdc_count_zeros(...)                                                                     \
   _Generic((__VA_ARGS__)RKI_U_TYPES(RKI_GENCASE, stdc_count_zeros_))(__VA_ARGS__)
# define stdc_count_ones(...)                                                                      \
   _Generic((__VA_ARGS__)RKI_U_TYPES(RKI_GENCASE, stdc_count_ones_))(__VA_ARGS__)
# define stdc_first_leading_zero(...)                                                              \
   _Generic((__VA_ARGS__)RKI_U_TYPES(RKI_GENCASE, stdc_first_leading_zero_))(__VA_ARGS__)
# define stdc_first_leading_one(...)                                                               \
   _Generic((__VA_ARGS__)RKI_U_TYPES(RKI_GENCASE, stdc_first_leading_one_))(__VA_ARGS__)
# define stdc_first_trailing_zero(...)                                                             \
   _Generic((__VA_ARGS__)RKI_U_TYPES(RKI_GENCASE, stdc_first_trailing_zero_))(__VA_ARGS__)
# define stdc_first_trailing_one(...)                                                              \
   _Generic((__VA_ARGS__)RKI_U_TYPES(RKI_GENCASE, stdc_first_trailing_one_))(__VA_ARGS__)
# define stdc_bit_floor(...)                                                                       \
   _Generic((__VA_ARGS__)RKI_U_TYPES(RKI_GENCASE, stdc_bit_floor_))(__VA_ARGS__)
# define stdc_bit_ceil(...)                                                                        \
   _Generic((__VA_ARGS__)RKI_U_TYPES(RKI_GENCASE, stdc_bit_ceil_))(__VA_ARGS__)
# define stdc_bit_width(...)                                                                       \
   _Generic((__VA_ARGS__)RKI_U_TYPES(RKI_GENCASE, stdc_bit_width_))(__VA_ARGS__)
# define stdc_has_single_bit(...)                                                                  \
   _Generic((__VA_ARGS__)RKI_U_TYPES(RKI_GENCASE, stdc_has_single_bit_))(__VA_ARGS__)
#endif /* RKI_STDBIT_FALLBACK */

#define RKI_AMALG_IMPL_74750096303C1B2D 1
RKI_HEADER_END

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
/* inlined from include/rk_trees.h:65: #include "rk_alloc.h" */
/* BEGIN INLINE: include/rk_alloc.h */
// SPDX-License-Identifier: MIT

/// @file rk_alloc.h
/// @version 1.0.0
/// @defgroup rk_alloc Allocator Interface

/// @brief Customizable allocator abstraction for C and C++.
/// @details Allocator consists of a shared vtable and an optional context pointer. Built-in malloc
/// and page allocators are provided; custom allocators implement the callback contracts below.
/// Positive-size allocation and reallocation must return valid non-null storage or handle failure
/// locally. Built-in allocators invoke failure macros from rk_config.h, which abort by default.
/// Overrides that resume execution must preserve the operation's storage, alignment, and lifetime
/// contracts; returning NULL is not a supported allocation-failure result for positive sizes.
/// Public allocation wrappers return NULL for zero-size requests. Reallocation to zero deallocates
/// and returns NULL. Null deallocation is a no-op, subject to the documented size preconditions.
/// @note Optional allocator arguments are passed by value and default to alloc_ctx. When custom
/// allocators are disabled, per-object allocator fields and explicit allocator arguments are
/// disabled.
/// @note Calls that use an unset container allocator fall back to the current alloc_ctx; containers
/// that capture an allocator during initialization retain that handle.
/// @{
#ifndef RK_ALLOC_H
#define RK_ALLOC_H
#ifndef _MSC_VER
# include <sys/mman.h>
# include <unistd.h>
#endif
/* inlined from include/rk_alloc.h:28: #include "rk_defs.h" */
/* skipped already-included: "include/rk_defs.h" */
RKI_HEADER_BEGIN

/// @brief Allocation logging hooks using rk_log(), subject to its configured logging policy.
#define alloc_log_new()    rk_log("[alloc]  %s:%d ", __FILE__, __LINE__)
#define alloc_log_renew()  rk_log("[renew]  %s:%d ", __FILE__, __LINE__)
#define alloc_log_delete() rk_log("[delete] %s:%d ", __FILE__, __LINE__)

/// @brief Allocation callback; called with a positive size by generic wrappers.
/// @param size Requested size in bytes; must be nonzero.
/// @param align Required alignment; must be a supported nonzero power of two.
/// @param ctx Allocator context; may be NULL if the implementation permits it.
/// @return Non-null storage of at least size bytes, aligned to align.
/// @note Failure must be handled locally; the callback must not return NULL. Failure macros from
/// rk_config.h may be used. Generic wrappers handle zero sizes before dispatch.
typedef void*(alloc_allocation_f)(size_t size, size_t align, void* ctx);

/// @brief Reallocation function. May move the allocation.
/// @param old_ptr  Non-null pointer to a valid allocation owned by this allocator.
/// @param old_size Current requested size of the allocation in bytes; must be nonzero.
/// @param new_size Requested new size in bytes; must be nonzero.
/// @param align    Alignment used for the original allocation.
/// @param ctx      Allocator context. May be `NULL` depending on the allocator.
/// @return A non-null pointer to at least `new_size` bytes of storage aligned to `align`.
/// @note Preserves the first `min(old_size, new_size)` bytes. On success, the old allocation
/// is replaced by the returned allocation.
/// @note Allocation failure must be handled locally; the function must not return `NULL`.
/// The failure macros in `rk_config.h` may be used for this purpose.
typedef void*(alloc_reallocation_f)(void* old_ptr, size_t old_size, size_t new_size, size_t align,
                                    void* ctx);

/// @brief Deallocation function.
/// @param ptr      Non-null pointer to a valid allocation owned by this allocator.
/// @param old_size Current requested size of the allocation in bytes; must be nonzero.
/// @param align    Alignment used for the original allocation.
/// @param ctx      Allocator context. May be `NULL` depending on the allocator.
/// @note Releases the allocation according to the allocator's strategy.
/// Individual deallocations need not reclaim storage immediately.
typedef void(alloc_deallocation_f)(void* ptr, size_t old_size, size_t align, void* ctx);

/// @brief Shared allocation, reallocation, and deallocation callbacks.
/// @note Every callback must be non-null and obey its corresponding function typedef's contract.
/// Generic wrappers normalize zero-size and null-pointer cases before dispatch.
/// @see alloc_allocation_f
/// @see alloc_reallocation_f
/// @see alloc_deallocation_f
typedef struct AllocatorVTable {
  alloc_allocation_f*   rk_alloc_alignsize(2, 1) alloc_f;
  alloc_reallocation_f* rk_alloc_alignsize(4, 3) realloc_f;
  alloc_deallocation_f* dealloc_f;
} AllocatorVTable;

/// @brief Allocator handle containing a shared vtable pointer and an optional context pointer.
/// @note Pass handles by value. The vtable and context must remain valid while the handle is used;
/// copying a handle does not copy or take ownership of its context. A NULL context is permitted by
/// some allocators, including the built-in malloc and page allocators.
/// @note Explicit handles passed to alloc_* must have a valid vtable with all callbacks populated,
/// even for zero-size requests or null deallocation. An unset stored container handle may instead
/// use the library's internal fallback to alloc_ctx.
/// @note Custom callbacks must return non-null storage for positive allocation/reallocation sizes
/// or handle failure locally. Zero-size requests are handled by the public wrappers.
typedef struct Allocator {
  const AllocatorVTable* vtab; ///< Vtable pointer
  void*                  ctx;  ///< Optional Context Pointer
} Allocator;

rklib_fun alloc_allocation_f   rki_malloc_allocate;
rklib_fun alloc_reallocation_f rki_malloc_reallocate;
rklib_fun alloc_deallocation_f rki_malloc_deallocate;
static const AllocatorVTable   alloc_malloc_allocator_vtable = {.alloc_f   = rki_malloc_allocate,
                                                                .realloc_f = rki_malloc_reallocate,
                                                                .dealloc_f = rki_malloc_deallocate};

/// @brief Default allocator using `malloc`/`free` (or `_aligned_malloc` on MSVC for over-aligned
/// requests). Set as the initial value of `alloc_ctx`.
rk_unused static const Allocator alloc_malloc_allocator
    = {.vtab = &alloc_malloc_allocator_vtable, .ctx = rk_null};

rklib_fun alloc_allocation_f           rki_page_allocate;
rklib_fun alloc_reallocation_f         rki_page_reallocate;
rklib_fun alloc_deallocation_f         rki_page_deallocate;
rk_unused static const AllocatorVTable alloc_page_allocator_vtable
    = {.alloc_f   = rki_page_allocate,
       .realloc_f = rki_page_reallocate,
       .dealloc_f = rki_page_deallocate};

/// @brief Allocator backed by OS page mapping (`mmap` / `VirtualAlloc`). All allocations are
/// page-aligned; fresh allocations are zero-initialized, but bytes added by reallocation are not
/// guaranteed to be zero (see page_realloc()). Alignments greater than the system page size are not
/// supported.
rk_unused static const Allocator alloc_page_allocator
    = {.vtab = &alloc_page_allocator_vtable, .ctx = rk_null};

#if RK_CUSTOM_ALLOCATORS
# if RK_ALLOC_CTX_THREAD_LOCAL
#  define RKI_ALLOCCTX_STORAGE extern_var thread_local
# else
#  define RKI_ALLOCCTX_STORAGE extern_var
# endif
# define RKI_ALLOCCTX_INIT(...) extern_def({__VA_ARGS__})
#else
# define RKI_ALLOCCTX_STORAGE   static const
# define RKI_ALLOCCTX_INIT(...) = {__VA_ARGS__}
#endif

/// @brief Default handle used when an optional allocator argument is omitted.
/// @note Initially uses alloc_malloc_allocator. Must always contain a valid Allocator. Objects that
/// capture this handle retain it; later replacements affect subsequent captures and calls that
/// consult the default. Unset stored container handles fall back to its current value.
/// @note With custom allocators enabled, RK_ALLOC_CTX_THREAD_LOCAL selects thread-local storage.
/// With custom allocators disabled, this is a fixed static const malloc allocator handle.
RKI_ALLOCCTX_STORAGE Allocator alloc_ctx RKI_ALLOCCTX_INIT(.vtab = &alloc_malloc_allocator_vtable,
                                                           .ctx  = rk_null);

/// @brief `void* alloc_allocate(size_t bytes, size_t align, Allocator alloc = alloc_ctx)` -
/// Allocates raw storage through the selected allocator.
/// @param bytes Requested payload size; may be zero.
/// @param align Supported nonzero power-of-two alignment.
/// @param alloc Valid allocator handle; defaults to alloc_ctx. Explicit selection requires custom
/// allocators.
/// @return NULL if bytes is zero; otherwise non-null aligned storage or locally handled failure.
/// @note Zero size does not invoke the allocation callback. Does not initialize storage or
/// construct C++ objects. All size and alignment calculations must be representable.
/// @see alloc_new
#define alloc_allocate(bytes, align, ...)                                                          \
  ((void*)RKI_OVERLOAD(RKI_ALLOC_ALLOCATE, bytes, align, ##__VA_ARGS__))

/// @brief `void* alloc_reallocate(void* ptr, size_t old_bytes, size_t new_bytes, size_t align,
/// Allocator alloc = alloc_ctx)` - Resizes storage, possibly moving it.
/// @param ptr Valid allocation owned by alloc, or NULL with old_bytes equal to zero.
/// @param old_bytes Current requested payload size; positive for a non-null pointer, zero for NULL.
/// @param new_bytes Desired payload size; may be zero.
/// @param align Original allocation alignment; for NULL input, supported alignment for the new
/// allocation.
/// @param alloc Valid owning allocator; defaults to alloc_ctx.
/// @return NULL if new_bytes is zero; otherwise non-null storage aligned to align.
/// @note NULL input routes to allocation. Zero new size deallocates a non-null input and returns
/// NULL. Otherwise preserves min(old_bytes, new_bytes) bytes; added bytes are uninitialized. The
/// old allocation is replaced by the result, which becomes the allocation to use and subsequently
/// deallocate.
/// @note Pointer/old-size mismatches are contract violations, including for zero new size. Failure
/// must be handled locally. Does not construct or destroy C++ objects.
/// @see alloc_renew
#define alloc_reallocate(ptr, old_bytes, new_bytes, align, ...)                                    \
  ((void*)RKI_OVERLOAD(RKI_ALLOC_REALLOCATE, ptr, old_bytes, new_bytes, align, ##__VA_ARGS__))

/// @brief `void alloc_deallocate(void* ptr, size_t bytes, size_t align, Allocator alloc =
/// alloc_ctx)` - Releases an allocation according to the allocator's strategy.
/// @param ptr Valid allocation owned by alloc, or NULL with bytes equal to zero.
/// @param bytes Current requested payload size; positive for a non-null pointer, zero for NULL.
/// @param align Original allocation alignment; must be a supported nonzero power of two.
/// @param alloc Valid owning allocator; defaults to alloc_ctx.
/// @note NULL input with zero size is a no-op and does not invoke the deallocation callback.
/// Pointer/size mismatches are contract violations. Individual deallocation may reclaim no storage,
/// as with arena allocators. Does not invoke C++ destructors.
/// @see alloc_delete
#define alloc_deallocate(ptr, bytes, align, ...)                                                   \
  ((void)RKI_OVERLOAD(RKI_ALLOC_DEALLOCATE, ptr, bytes, align, ##__VA_ARGS__))

/// @brief Allocates raw storage for count elements of T with alignof(T).
/// @param T Element type.
/// @param count Element count; may be zero. The byte-size calculation must be representable.
/// @param allocator Valid allocator handle; defaults to alloc_ctx.
/// @return A T* to allocated storage, or NULL if count is zero.
/// @note Uses alloc_allocate() failure and zero-size semantics. Does not invoke C++ constructors.
/// @see alloc_allocate
#define alloc_new(T, count, ...) ((T*)RKI_OVERLOAD(RKI_ALLOC_NEW, T, count, ##__VA_ARGS__))

/// @brief Resizes typed storage to new_count elements, possibly moving the allocation.
/// @param ptr Valid allocation, or a typed NULL pointer with old_count equal to zero.
/// @param old_count Current requested element count; positive for a non-null pointer, zero for
/// NULL.
/// @param new_count Desired element count; may be zero. Byte-size calculations must be
/// representable.
/// @param allocator Valid owning allocator; defaults to alloc_ctx.
/// @return The resulting pointer, cast to the input pointer type; NULL if new_count is zero.
/// @note NULL input allocates; zero new count deallocates and returns NULL. Preserves retained
/// bytes; does not construct or destroy C++ objects. Inherits alloc_reallocate() contracts.
/// @note Uses the element type's alignment, which must match the original allocation request.
/// Use alloc_renew_aligned() when the original requested alignment differs.
/// @see alloc_reallocate
#define alloc_renew(ptr, old_count, new_count, ...)                                                \
  ((typeof(ptr))RKI_OVERLOAD(RKI_ALLOC_RENEW, ptr, old_count, new_count, ##__VA_ARGS__))

/// @brief Deallocates typed storage without invoking C++ destructors.
/// @param ptr Valid allocation, or a typed NULL pointer with old_count equal to zero.
/// @param old_count Current requested element count; positive for a non-null pointer, zero for
/// NULL. The byte-size calculation must be representable.
/// @param allocator Valid owning allocator; defaults to alloc_ctx.
/// @note NULL input with zero old count is a no-op. Inherits alloc_deallocate() contracts.
/// Uses the element type's alignment, which must match the original request. Use
/// alloc_delete_aligned() when the original requested alignment differs.
/// @see alloc_deallocate
#define alloc_delete(ptr, old_count, ...)                                                          \
  ((void)RKI_OVERLOAD(RKI_ALLOC_DELETE, ptr, old_count, ##__VA_ARGS__))

/// @brief Allocates raw storage for count elements of T with explicit alignment.
/// @param T Element type.
/// @param count Element count; may be zero. The byte-size calculation must be representable.
/// @param align Supported nonzero power of two, at least alignof(T).
/// @param allocator Valid allocator handle; defaults to alloc_ctx.
/// @return A T* to allocated storage, or NULL if count is zero.
/// @note Uses alloc_allocate() failure and zero-size semantics. Does not invoke C++ constructors.
/// @see alloc_allocate
#define alloc_new_aligned(T, count, align, ...)                                                    \
  ((T*)RKI_OVERLOAD(RKI_ALLOC_ALIGNED_NEW, T, count, align, ##__VA_ARGS__))

/// @brief Resizes typed storage to new_count elements, possibly moving the allocation.
/// @param ptr Valid allocation, or a typed NULL pointer with old_count equal to zero.
/// @param old_count Current requested element count; positive for a non-null pointer, zero for
/// NULL.
/// @param new_count Desired element count; may be zero. Byte-size calculations must be
/// representable.
/// @param align Original allocation alignment, at least the element type's alignment. For NULL
/// input, selects supported alignment for the new allocation.
/// @param allocator Valid owning allocator; defaults to alloc_ctx.
/// @return The resulting pointer, cast to the input pointer type; NULL if new_count is zero.
/// @note NULL input allocates; zero new count deallocates and returns NULL. Preserves retained
/// bytes; does not construct or destroy C++ objects. Inherits alloc_reallocate() contracts.
/// @see alloc_reallocate
#define alloc_renew_aligned(ptr, old_count, new_count, align, ...)                                 \
  ((typeof(ptr))RKI_OVERLOAD(RKI_ALLOC_ALIGNED_RENEW, ptr, old_count, new_count,                   \
                             align, ##__VA_ARGS__))

/// @brief Deallocates typed storage without invoking C++ destructors.
/// @param ptr Valid allocation, or a typed NULL pointer with old_count equal to zero.
/// @param old_count Current requested element count; positive for a non-null pointer, zero for
/// NULL. The byte-size calculation must be representable.
/// @param align Original allocation alignment, at least the element type's alignment.
/// @param allocator Valid owning allocator; defaults to alloc_ctx.
/// @note NULL input with zero old count is a no-op. Inherits alloc_deallocate() contracts.
/// @see alloc_deallocate
#define alloc_delete_aligned(ptr, old_count, align, ...)                                           \
  ((void)RKI_OVERLOAD(RKI_ALLOC_ALIGNED_DELETE, ptr, old_count, align, ##__VA_ARGS__))

/// @brief `void* malloc_allocate(size_t nbytes, size_t align)` - Allocates raw malloc-backed
/// storage.
/// @param nbytes Payload size; may be zero. Required alignment rounding must be representable.
/// @param align Supported nonzero power-of-two alignment.
/// @return NULL for zero size; otherwise non-null storage aligned to align.
/// @note Uses malloc for alignments up to RKI_MALLOC_ALIGN; larger alignments use aligned_alloc
/// on non-MSVC platforms or _aligned_malloc on MSVC, rounding the size as necessary.
/// Failure invokes RK_MALLOC_FAIL. Overrides must preserve the allocation contract.
/// @note Use matching malloc_* operations for reallocation/deallocation; on MSVC ordinary and
/// aligned allocation families must not be mixed.
/// @see malloc_reallocate
/// @see malloc_deallocate
#define malloc_allocate(nbytes, align) ((void*)RKI_MALLOC_ALLOCATE(nbytes, align))

/// @brief `void* malloc_reallocate(void* ptr, size_t obytes, size_t nbytes, size_t align)` -
/// Resizes malloc-backed storage, possibly moving it.
/// @param ptr Valid malloc-backed allocation, or NULL with obytes equal to zero.
/// @param obytes Current requested payload size; positive for a non-null pointer, zero for NULL.
/// @param nbytes New payload size; may be zero. Required alignment rounding must be representable.
/// @param align Original requested alignment; for NULL input, supported alignment for the new
/// allocation.
/// @return NULL for zero new size; otherwise non-null aligned storage.
/// @note NULL input allocates; zero new size frees and returns NULL. Preserves min(obytes, nbytes)
/// bytes. Size preconditions apply even where the underlying implementation ignores obytes.
/// @note Alignments up to RKI_MALLOC_ALIGN use realloc. Larger alignments use _aligned_realloc on
/// MSVC; elsewhere they allocate aligned storage, copy, and free. Failure invokes RK_MALLOC_FAIL.
/// Use matching malloc_* operations; do not mix ordinary and aligned allocation families on MSVC.
#define malloc_reallocate(ptr, obytes, nbytes, align)                                              \
  ((void*)RKI_MALLOC_REALLOCATE(ptr, obytes, nbytes, align))

/// @brief `void malloc_deallocate(void* ptr, size_t align)` - Frees malloc-backed storage.
/// @param ptr Valid allocation from the corresponding malloc_allocate()/malloc_reallocate() path,
/// or NULL for a no-op.
/// @param align Original requested alignment; must be a supported nonzero power of two.
/// @note Selects free or, for over-aligned requests on MSVC, _aligned_free. Do not mix ordinary
/// and aligned allocation families on MSVC.
#define malloc_deallocate(ptr, align)       ((void)RKI_MALLOC_DEALLOCATE(ptr, align))

/// @brief Allocates raw storage for count elements of T using ordinary malloc.
/// @param T Element type; its alignment must not exceed RKI_MALLOC_ALIGN.
/// @param count Element count; may be zero. The byte-size calculation must be representable.
/// @return A T* to uninitialized storage, or NULL if count is zero.
/// @note Failure invokes RK_MALLOC_FAIL. Does not construct C++ objects.
/// Use malloc_renew()/malloc_delete() for subsequent operations.
/// @see malloc_new_aligned
#define malloc_new(T, count)                ((T*)RKI_MALLOC_NEW(T, count))

/// @brief Resizes ordinary malloc-backed storage to count elements, possibly moving it.
/// @param ptr Valid ordinary malloc-backed allocation, or a typed NULL pointer.
/// @param count New element count; may be zero. The byte-size calculation must be representable.
/// @return The result cast to the input pointer type, or NULL if count is zero.
/// @note NULL input allocates; zero count frees and returns NULL. Preserves the retained bytes.
/// The element alignment must not exceed RKI_MALLOC_ALIGN. Does not construct or destroy C++
/// objects.
/// @warning On MSVC, must not be used for storage obtained through the aligned allocation family,
/// even if the element type itself has ordinary alignment.
/// @see malloc_renew_aligned
#define malloc_renew(ptr, count)            ((typeof(ptr))RKI_MALLOC_RENEW(ptr, count))

/// @brief Frees ordinary malloc-backed typed storage; a typed NULL pointer is a no-op.
/// @param ptr Valid ordinary malloc-backed allocation, or a typed NULL pointer.
/// @note The element alignment must not exceed RKI_MALLOC_ALIGN. Does not invoke C++ destructors.
/// @warning On MSVC, do not use for aligned-family storage; use malloc_delete_aligned() instead.
#define malloc_delete(ptr)                  ((void)RKI_MALLOC_DELETE(ptr))

/// @brief Allocates raw storage for count elements using the aligned allocation family.
/// @param T Element type.
/// @param count Element count; may be zero. Byte-size and alignment rounding must be representable.
/// @param align Supported nonzero power of two, at least alignof(T).
/// @return A T* to uninitialized aligned storage, or NULL if count is zero.
/// @note Always uses aligned_alloc on non-MSVC platforms or _aligned_malloc on MSVC, including
/// for ordinary alignment. Raises alignment to at least RKI_MALLOC_ALIGN and rounds size
/// accordingly. Failure invokes RK_MALLOC_FAIL. Does not invoke C++ constructors. Use
/// malloc_renew_aligned()/malloc_delete_aligned() for subsequent operations.
#define malloc_new_aligned(T, count, align) ((T*)RKI_MALLOC_ALIGNED_NEW(T, count, align))

/// @brief Resizes aligned-family typed storage to new_count elements, possibly moving it.
/// @param ptr Valid aligned-family allocation, or a typed NULL pointer with old_count equal to
/// zero.
/// @param old_count Current requested element count; positive for a non-null pointer, zero for
/// NULL.
/// @param new_count Desired element count; may be zero. Byte-size and rounding must be
/// representable.
/// @param align Original requested alignment, at least the element type's alignment. For NULL
/// input, selects supported alignment for the new allocation.
/// @return The result cast to the input pointer type, or NULL if new_count is zero.
/// @note NULL input allocates; zero new count frees and returns NULL. Preserves retained bytes.
/// Uses _aligned_realloc on MSVC; elsewhere allocates aligned storage, copies, and frees.
/// Failure invokes RK_MALLOC_FAIL. Does not construct or destroy C++ objects.
/// @see malloc_new_aligned
/// @see malloc_delete_aligned
#define malloc_renew_aligned(ptr, old_count, new_count, align)                                     \
  ((typeof(ptr))RKI_MALLOC_ALIGNED_RENEW(ptr, old_count, new_count, align))

/// @brief Frees aligned-family storage; a typed NULL pointer is a no-op.
/// @param ptr Valid allocation from malloc_new_aligned()/malloc_renew_aligned(), or a typed NULL
/// pointer. Also accepts over-aligned storage from malloc_allocate()/malloc_reallocate().
/// @note Uses free on non-MSVC platforms and _aligned_free on MSVC. Does not invoke C++
/// destructors. On MSVC, ordinary malloc-family storage must not be passed to this operation.
#define malloc_delete_aligned(ptr) ((void)RKI_MALLOC_ALIGNED_DELETE(ptr))

/// @brief Allocates zero-initialized, page-aligned storage using mmap or VirtualAlloc.
/// @param size Requested payload size; may be zero. Rounding up to a page boundary must be
/// representable.
/// @return NULL for zero size; otherwise non-null page-aligned storage.
/// @note Zero size invokes no failure handler. Positive-size failures invoke RK_MMAP_FAIL, which
/// aborts by default. Use page_realloc()/page_free() for subsequent operations.
rklib_fun void* page_alloc(size_t size);

/// @brief Resizes page-backed storage, possibly moving it.
/// @param ptr Valid page_alloc()/page_realloc() allocation, or NULL with old_size equal to zero.
/// @param old_size Current requested payload size; positive for a non-null pointer, zero for NULL.
/// @param new_size Desired payload size; may be zero. Page-size rounding must be representable.
/// @return NULL if new_size is zero; otherwise non-null page-aligned storage.
/// @note NULL input allocates. Zero new size frees and returns NULL. Preserves min(old_size,
/// new_size) bytes. Added bytes are not guaranteed to be zero, including when growth stays within
/// existing pages.
/// @note Reuses the address when both sizes round to the same page count, and shrinks by releasing
/// trailing pages. Growth uses mremap where enabled on Linux; otherwise allocates, copies, and
/// frees. Failures invoke RK_MMAP_FAIL. Pointer/old-size mismatches are contract violations even
/// for zero new size.

rklib_fun void* page_realloc(void* ptr, size_t old_size, size_t new_size);

/// @brief Frees page-backed storage according to its current requested size.
/// @param ptr Valid page allocation, or NULL.
/// @param size Current requested payload size, or zero. Page-size rounding must be representable.
/// @note Either NULL input or zero size is a no-op. In particular, passing zero with a non-null
/// pointer does not free the allocation. For actual deallocation, ptr must be a valid page
/// allocation and size must match its current requested size. Failures invoke RK_MMAP_FAIL.
rklib_fun void  page_free(void* ptr, size_t size);

/// @brief Copies nbytes bytes into newly allocated storage aligned to align_max.
/// @param src Source readable for nbytes bytes; may be NULL when nbytes is zero.
/// @param nbytes Byte count; may be zero.
/// @param allocator Valid allocator handle; defaults to alloc_ctx.
/// @return The allocated copy, or NULL if nbytes is zero.
/// @note Performs a bytewise copy, not a deep copy; does not invoke C++ constructors.
/// Zero-size requests do not access src. Failure follows alloc_allocate() semantics.
/// @see rk_memdup_aligned
#define rk_memdup(src, nbytes, ...)                                                                \
  ((void*)RKI_OVERLOAD(RKI_MEMDUP, src, nbytes, align_max, ##__VA_ARGS__))

/// @brief Copies nbytes bytes into newly allocated storage with explicit alignment.
/// @param src Source readable for nbytes bytes; may be NULL when nbytes is zero.
/// @param nbytes Byte count; may be zero.
/// @param align Supported nonzero power-of-two alignment.
/// @param allocator Valid allocator handle; defaults to alloc_ctx.
/// @return The allocated copy, or NULL if nbytes is zero.
/// @note Performs a bytewise copy, not a deep copy; does not invoke C++ constructors.
/// Zero-size requests do not access src. Failure follows alloc_allocate() semantics.
/// @see rk_memdup
#define rk_memdup_aligned(src, nbytes, align, ...)                                                 \
  ((void*)RKI_OVERLOAD(RKI_MEMDUP, src, nbytes, align, ##__VA_ARGS__))

/// @brief Copies count non-array elements into raw storage with their type's required alignment.
/// @param src Typed source pointer or array, readable for count elements; may be a typed NULL
/// pointer when count is zero. Array-valued elements are not supported by this macro's return type.
/// @param count Element count; may be zero. The byte-size calculation must be representable.
/// @param allocator Valid allocator handle; defaults to alloc_ctx.
/// @return A pointer to the copied elements, or NULL if count is zero. Element type is obtained
/// through typeof_decayed(*(src)); its qualification behavior follows that helper.
/// @note Uses bytewise copy semantics; does not construct C++ objects or perform a deep copy.
/// For array-valued elements, use rk_memdup_aligned() and an appropriate pointer-to-array type.
/// @see rk_memdup_aligned
#define rk_arrdup(src, count, ...)                                                                 \
  ((typeof_decayed(*(src))*)RKI_OVERLOAD(RKI_MEMDUP, src, sizeof_n(typeof(*(src)), count),         \
                                         alignof(typeof(*(src))), ##__VA_ARGS__))

#if RK_CUSTOM_ALLOCATORS
# define RK_IFALLOC(...)                    __VA_ARGS__
#else
# define RK_IFALLOC(...)
#endif
#define RKI_AMALG_IMPL_16C73346C0E1F501 1
RKI_HEADER_END
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
#define tree_is_empty(self)  ((bool)(tree_count(self) == 0))

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

#define RKI_AMALG_IMPL_28FAD6E6119BCEC1 1
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
RKI_HEADER_BEGIN

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
rklib_fun bitset       bitset_copy(bitset restrict dst, size_t nbits, cbitset restrict src);

/// @brief Tests whether bit `idx` is set.
/// @param bs,nbits Bitset and its logical size
/// @param idx Bit index (0-based)
/// @return `true` if bit `idx` is 1, otherwise `false`.
/// @pre `idx < nbits`.
rklib_fun rk_pure bool bitset_test(cbitset bs, size_t nbits, size_t idx);

/// @brief Clears bit `idx` (sets it to 0).
/// @param bs,nbits Bitset and its logical size
/// @param idx Bit index (0-based)
/// @return `bs` (for chaining).
/// @pre `idx < nbits`.
rklib_fun bitset       bitset_clear(bitset bs, size_t nbits, size_t idx);

/// @brief Sets bit `idx` (sets it to 1).
/// @param bs,nbits Bitset and its logical size
/// @param idx Bit index (0-based)
/// @return `bs` (for chaining).
/// @pre `idx < nbits`.
rklib_fun bitset       bitset_set(bitset bs, size_t nbits, size_t idx);

/// @brief Writes bit `idx` to `value`.
/// @param bs,nbits Bitset and its logical size
/// @param idx Bit index (0-based)
/// @param value New bit value (`false` -> 0, `true` -> 1)
/// @return `bs` (for chaining).
/// @pre `idx < nbits`.
rklib_fun bitset       bitset_write(bitset bs, size_t nbits, size_t idx, bool value);

/// @brief Toggles bit `idx`.
/// @param bs,nbits Bitset and its logical size
/// @param idx Bit index (0-based)
/// @return `bs` (for chaining).
/// @pre `idx < nbits`.
rklib_fun bitset       bitset_flip(bitset bs, size_t nbits, size_t idx);

/// @brief Clears all bits in the half-open interval `[start, end)`.
/// @param bs,nbits Bitset and its logical size
/// @param start First bit index (inclusive)
/// @param end Last bit index (exclusive)
/// @return `bs` (for chaining).
/// @pre `start <= end && end <= nbits`.
rklib_fun bitset       bitset_clear_range(bitset bs, size_t nbits, size_t start, size_t end);

/// @brief Sets all bits in the half-open interval `[start, end)`.
/// @param bs,nbits Bitset and its logical size
/// @param start First bit index (inclusive)
/// @param end Last bit index (exclusive)
/// @return `bs` (for chaining).
/// @pre `start <= end && end <= nbits`.
rklib_fun bitset       bitset_set_range(bitset bs, size_t nbits, size_t start, size_t end);

/// @brief Writes all bits in `[start, end)` to `value`.
/// @param bs,nbits Bitset and its logical size
/// @param start First bit index (inclusive)
/// @param end Last bit index (exclusive)
/// @param value New bit value (`false` -> 0, `true` -> 1)
/// @return `bs` (for chaining).
/// @pre `start <= end && end <= nbits`.
rklib_fun bitset bitset_write_range(bitset bs, size_t nbits, size_t start, size_t end, bool value);

/// @brief Toggles all bits in `[start, end)`.
/// @param bs,nbits Bitset and its logical size
/// @param start First bit index (inclusive)
/// @param end Last bit index (exclusive)
/// @return `bs` (for chaining).
/// @pre `start <= end && end <= nbits`.
rklib_fun bitset bitset_flip_range(bitset bs, size_t nbits, size_t start, size_t end);

/// @brief Clears any padding bits (indices `>= nbits`) in the last storage word.
/// @param bs,nbits Bitset and its logical size
/// @return `bs` (for chaining).
/// @note Call this if `bs` may contain nonzero padding bits (e.g. after uninitialized allocation or
/// raw word operations).
rklib_fun bitset bitset_clear_padding(bitset bs, size_t nbits);

/// @brief Clears all bits to 0.
/// @param bs,nbits Bitset and its logical size
/// @return `bs` (for chaining).
rklib_fun bitset bitset_clear_all(bitset bs, size_t nbits);

/// @brief Sets all bits to 1 (and clears padding bits).
/// @param bs,nbits Bitset and its logical size
/// @return `bs` (for chaining).
rklib_fun bitset bitset_set_all(bitset bs, size_t nbits);

/// @brief Sets all bits to `value`.
/// @param bs,nbits Bitset and its logical size
/// @param value New bit value (`false` -> 0, `true` -> 1)
/// @return `bs` (for chaining).
rklib_fun bitset bitset_write_all(bitset bs, size_t nbits, bool value);

/// @brief Toggles all bits (and clears padding bits).
/// @param bs,nbits Bitset and its logical size
/// @return `bs` (for chaining).
rklib_fun bitset bitset_flip_all(bitset bs, size_t nbits);

/// @brief Finds the first set bit when scanning from MSB to LSB.
/// @param bs,nbits Bitset and its logical size
/// @return A **1-based** position of the first leading one, or 0 if none.
/// @note This matches the “1-based with 0 sentinel” convention used by C23 `<stdbit.h>` query
/// functions and several compiler builtins.
rklib_fun rk_pure size_t bitset_first_leading_one(cbitset bs, size_t nbits);

/// @brief Finds the first zero bit when scanning from MSB to LSB.
/// @param bs,nbits Bitset and its logical size
/// @return A **1-based** position of the first leading zero, or 0 if none.
/// @note If all valid bits are 1, returns 0.
rklib_fun rk_pure size_t bitset_first_leading_zero(cbitset bs, size_t nbits);

/// @brief Finds the first set bit when scanning from LSB to MSB.
/// @param bs,nbits Bitset and its logical size
/// @return A **1-based** position of the first trailing one, or 0 if none.
rklib_fun rk_pure size_t bitset_first_trailing_one(cbitset bs, size_t nbits);

/// @brief Finds the first zero bit when scanning from LSB to MSB.
/// @param bs,nbits Bitset and its logical size
/// @return A **1-based** position of the first trailing zero, or 0 if none.
/// @note Padding bits are treated as 1 (not eligible as “zero” results).
rklib_fun rk_pure size_t bitset_first_trailing_zero(cbitset bs, size_t nbits);

/// @brief Counts leading zeros (from MSB toward LSB).
/// @param bs,nbits Bitset and its logical size
/// @return Number of consecutive zero bits starting at the MSB.
rklib_fun rk_pure size_t bitset_leading_zeros(cbitset bs, size_t nbits);

/// @brief Counts leading ones (from MSB toward LSB).
/// @param bs,nbits Bitset and its logical size
/// @return Number of consecutive one bits starting at the MSB.
rklib_fun rk_pure size_t bitset_leading_ones(cbitset bs, size_t nbits);

/// @brief Counts trailing zeros (from LSB toward MSB).
/// @param bs,nbits Bitset and its logical size
/// @return Number of consecutive zero bits starting at the LSB.
rklib_fun rk_pure size_t bitset_trailing_zeros(cbitset bs, size_t nbits);

/// @brief Counts trailing ones (from LSB toward MSB).
/// @param bs,nbits Bitset and its logical size
/// @return Number of consecutive one bits starting at the LSB.
rklib_fun rk_pure size_t bitset_trailing_ones(cbitset bs, size_t nbits);

/// @brief Counts the number of 1 bits.
/// @param bs,nbits Bitset and its logical size
/// @return Number of set bits.
rklib_fun rk_pure size_t bitset_count_ones(cbitset bs, size_t nbits);

/// @brief Counts the number of 0 bits.
/// @param bs,nbits Bitset and its logical size
/// @return Number of zero bits.
rklib_fun rk_pure size_t bitset_count_zeros(cbitset bs, size_t nbits);

/// @brief Returns whether any bit is set.
/// @param bs,nbits Bitset and its logical size
/// @return `true` if at least one valid bit is 1, else `false`.
rklib_fun rk_pure bool   bitset_any(cbitset bs, size_t nbits);

/// @brief Returns whether no bits are set.
/// @param bs,nbits Bitset and its logical size
/// @return `true` if all valid bits are 0, else `false`.
rklib_fun rk_pure bool   bitset_none(cbitset bs, size_t nbits);

/// @brief Returns whether all bits are set.
/// @param bs,nbits Bitset and its logical size
/// @return `true` if all valid bits are 1, else `false`.
rklib_fun rk_pure bool   bitset_all(cbitset bs, size_t nbits);

/// @brief Returns whether exactly one bit is set.
/// @param bs,nbits Bitset and its logical size
/// @return `true` if exactly one valid bit is 1, else `false`.
rklib_fun rk_pure bool   bitset_has_single_bit(cbitset bs, size_t nbits);

/// @brief Compares two bitsets for equality.
/// @param a First bitset
/// @param nbits Logical size of both bitsets
/// @param b Second bitset
/// @return `true` if all valid bits match, else `false`.
rklib_fun rk_pure bool   bitset_equals(cbitset a, size_t nbits, cbitset b);

/// @brief Bitwise OR (in-place): `dst |= src`.
/// @param dst Destination bitset (modified)
/// @param nbits Logical size of both bitsets
/// @param src Source bitset
/// @return `dst` (for chaining).
/// @pre `dst` and `src` are both valid for `nbits` bits.
rklib_fun bitset         bitset_or(bitset dst, size_t nbits, cbitset src);

/// @brief Bitwise AND (in-place): `dst &= src`.
/// @param dst Destination bitset (modified)
/// @param nbits Logical size of both bitsets
/// @param src Source bitset
/// @return `dst` (for chaining).
rklib_fun bitset         bitset_and(bitset dst, size_t nbits, cbitset src);

/// @brief Bitwise XOR (in-place): `dst ^= src`.
/// @param dst Destination bitset (modified)
/// @param nbits Logical size of both bitsets
/// @param src Source bitset
/// @return `dst` (for chaining).
/// @note With the padding invariant, padding remains zero because `0 ^ 0 == 0`.
rklib_fun bitset         bitset_xor(bitset dst, size_t nbits, cbitset src);

/// @brief Bitwise subtraction (in-place): clears bits present in `src` (`dst &= ~src`).
/// @param dst Destination bitset (modified)
/// @param nbits Logical size of both bitsets
/// @param src Source bitset
/// @return `dst` (for chaining).
rklib_fun bitset         bitset_sub(bitset dst, size_t nbits, cbitset src);

/// @brief Bitwise NOT: `dst = ~src` (clears padding on return).
/// @param dst Destination bitset (modified)
/// @param nbits Logical size of both bitsets
/// @param src Source bitset
/// @return `dst` (for chaining).
rklib_fun bitset         bitset_not(bitset dst, size_t nbits, cbitset src);

/// @brief Left-shifts `src` by `sh` bits into `dst`.
/// @param dst Destination bitset (modified)
/// @param nbits Logical size of both bitsets
/// @param src Source bitset
/// @param sh Shift amount in bits
/// @return `dst` (for chaining).
/// @note If `sh >= nbits`, the result is all zeros.
/// @note `dst` may alias `src`.
rklib_fun bitset         bitset_shift_left_into(bitset dst, size_t nbits, cbitset src, size_t sh);

/// @brief In-place left shift: `bs <<= sh`.
/// @param bs,nbits Bitset and its logical size
/// @param sh Shift amount in bits
/// @return `bs` (for chaining).
rklib_fun bitset         bitset_shift_left(bitset bs, size_t nbits, size_t sh);

/// @brief Right-shifts `src` by `sh` bits into `dst`.
/// @param dst Destination bitset (modified)
/// @param nbits Logical size of both bitsets
/// @param src Source bitset
/// @param sh Shift amount in bits
/// @return `dst` (for chaining).
/// @note If `sh >= nbits`, the result is all zeros.
/// @note `dst` may alias `src`.
rklib_fun bitset         bitset_shift_right_into(bitset dst, size_t nbits, cbitset src, size_t sh);

/// @brief In-place right shift: `bs >>= sh`.
/// @param bs,nbits Bitset and its logical size
/// @param sh Shift amount in bits
/// @return `bs` (for chaining).
rklib_fun bitset         bitset_shift_right(bitset bs, size_t nbits, size_t sh);

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
rklib_fun rk_pure size_t bitset_find_next_set(cbitset bs, size_t nbits, size_t cur);
/// @brief Finds the next zero bit after `cur` when scanning from LSB to MSB.
/// @param bs,nbits Bitset and its logical size.
/// @param cur Previously visited bit index, or `BITSET_NPOS` to start from the beginning.
/// @return The index (0-based) of the first zero bit with index `> cur`, or `BITSET_NPOS` if no
/// such bit exists.
rklib_fun rk_pure size_t bitset_find_next_clear(cbitset bs, size_t nbits, size_t cur);

/// @brief Finds the previous set bit before `cur` when scanning from MSB to LSB.
/// @param bs,nbits Bitset and its logical size.
/// @param cur Current bit index, or `nbits` to start from the end.
/// @return The index (0-based) of the last set bit with index `< cur`, or `BITSET_NPOS` if no such
/// bit exists.
rklib_fun rk_pure size_t bitset_find_prev_set(cbitset bs, size_t nbits rk_unused, size_t cur);

/// @brief Finds the previous zero bit before `cur` when scanning from MSB to LSB.
/// @param bs,nbits Bitset and its logical size.
/// @param cur Current bit index, or `nbits` to start from the end.
/// @return The index (0-based) of the last zero bit with index `< cur`, or `BITSET_NPOS` if no such
/// bit exists.
rklib_fun rk_pure size_t bitset_find_prev_clear(cbitset bs, size_t nbits, size_t cur);

/// @brief Finds the first set bit when scanning from LSB to MSB.
/// @param bs,nbits Bitset and its logical size.
/// @return The index (0-based) of the first set bit, or `BITSET_NPOS` if none.
rklib_fun rk_pure size_t bitset_find_first_set(cbitset bs, size_t nbits);

/// @brief Finds the first zero bit when scanning from LSB to MSB.
/// @param bs,nbits Bitset and its logical size.
/// @return The index (0-based) of the first zero bit, or `BITSET_NPOS` if none.
rklib_fun rk_pure size_t bitset_find_first_clear(cbitset bs, size_t nbits);

/// @brief Finds the last set bit when scanning from MSB to LSB.
/// @param bs,nbits Bitset and its logical size.
/// @return The index (0-based) of the last set bit, or `BITSET_NPOS` if none.
rklib_fun rk_pure size_t bitset_find_last_set(cbitset bs, size_t nbits);

/// @brief Finds the last zero bit when scanning from MSB to LSB.
/// @param bs,nbits Bitset and its logical size.
/// @return The index (0-based) of the last zero bit, or `BITSET_NPOS` if none.
rklib_fun rk_pure size_t bitset_find_last_clear(cbitset bs, size_t nbits);

/// @brief Converts a bitset to a binary string (MSB first).
/// @param dst Destination char buffer of size at least `nbits + 1`
/// @param src Source bitset
/// @param nbits Logical size of the bitset
/// @return `dst` (for convenience). The output is `nbits` characters of `'0'`/`'1'`, plus a
/// trailing `'\0'`.
rklib_fun char*          bitset_tostr(char* restrict dst, cbitset restrict src, size_t nbits);

/// @brief Parses a binary string (MSB first) into a `len`-bit bitset.
/// @param dst Destination bitset storage for a **len-bit** bitset (at least `bitset_words(len)`
/// words).
/// @param src Source string of at least `len` characters, consisting only of `'0'` and `'1'`. The
/// first character corresponds to the MSB.
/// @param len Number of bits to parse; also the logical size of the resulting bitset.
/// @return `dst` (for chaining).
/// @note This function treats `dst` as a `len`-bit bitset for this call. It writes all bits `[0,
/// len)` and clears padding bits on return.
rklib_fun bitset         bitset_fromstr(bitset dst, const char* restrict src, size_t len);

/// @brief Visits every set bit's index in increasing order.
/// @param self  Bitset storage, mutable or const. Evaluated once.
/// @param nbits Logical bit count. Evaluated once.
/// @param idx   Name of the size_t index variable.
/// @note break stops traversal; continue advances to the next set bit.
/// @note Clearing the current bit is supported. Each storage word is cached when reached;
/// changes to other bits in that word are not reflected in traversal. Words not yet
/// reached are read when traversal advances to them.
///
/// Usage:
/// ```c
/// bitset_foreach(bs, 128, i) { printf("set bit: %zu\n", i); }
/// ```
#define bitset_foreach(self, nbits, idx)          RKI_BITSET_FOREACH(self, nbits, idx)

/// @brief Like `bitset_foreach()`, but iterates in decreasing order. Same parameters and contract.
#define bitset_foreach_reversed(self, nbits, idx) RKI_BITSET_FOREACH_REVERSED(self, nbits, idx)

/// @brief Clears every set bit whose index satisfies `pred`.
/// @param self  Mutable bitset storage. Evaluated once.
/// @param nbits Logical bit count. Evaluated once.
/// @param idx   Name of the const size_t index variable.
/// @param pred  Predicate expression, evaluated once per original set bit.
/// @note The predicate must not modify the bitset.
///
/// Usage:
/// ```c
/// bitset_clear_if(bs, 128, i, i % 2 == 0); // clear even-indexed bits
/// ```
#define bitset_clear_if(self, nbits, idx, pred)   RKI_BITSET_CLEAR_IF(self, nbits, idx, pred)

#define RKI_AMALG_IMPL_EAF61C7F654781E3 1
RKI_HEADER_END
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
/// @brief Linear arena allocation over borrowed storage, with Allocator integration.
/// @details Allocations advance a cursor. Clearing or rewinding reclaims storage in bulk without
/// releasing the backing buffer. Direct arena operations and generic alloc_* wrappers have
/// different zero-size semantics; see arena_allocate() and arena_to_alloc_static().
/// @note Concurrent access involving cursor changes requires external synchronization. Separate
/// arenas may be used independently when their backing storage does not interfere.
/// @see rk_alloc.h
/// @see rk_defs.h
/// @{
#ifndef RK_ARENA_H
#define RK_ARENA_H
/* inlined from include/rk_arena.h:16: #include "rk_alloc.h" */
/* skipped already-included: "include/rk_alloc.h" */
RKI_HEADER_BEGIN
RKI_IGNWARN_CLANG_BEG("-Wreturn-type-c-linkage")

/// @brief Non-owning linear arena over a fixed backing buffer.
/// @details Allocation advances the cursor; marks, rewinding, and clearing allow bulk reuse.
/// @note The backing buffer must remain valid while the arena or its allocations are used.
/// Copying an Arena copies its cursor state and pointers; it does not copy the backing storage.
/// All Arena pointer arguments must be non-null and refer to initialized objects.
/// A zero-initialized Arena has no backing buffer; queries, clearing, marking, and
/// allocation/resize attempts support this state.
typedef struct Arena {
  unsigned char* beg; ///< Start of arena memory block
  unsigned char* cur; ///< Current Position in arena memory block
  unsigned char* end; ///< End of arena memory block
} Arena;

/// @brief Initializes an arena over borrowed byte storage.
/// @param arr Backing buffer, readable and writable for `len` bytes, or `NULL` for no backing
/// storage.
/// @param len Buffer length in bytes. Must not extend beyond the backing buffer.
/// @return An arena with its cursor at the beginning. A null buffer produces a null-backed arena.
/// @note No storage is allocated or owned by the arena; a null buffer ignores `len`.
rklib_fun rk_const Arena arena_init(unsigned char* arr, size_t len);

/// @brief `Arena arena_init_static(unsigned char arr[])` - Produces an initializer for an arena
/// using a byte array as borrowed backing storage; capacity is determined by `sizeof(arr)`.
/// @param array_non_compound_literal The backing byte array; must be an actual array, not a
/// pointer.
/// @return An initializer for a new Arena.
/// @warning Do not use with compound literals; use arena_init() instead.
/// @note Can be used for static initialization when the array has a suitable storage duration.
#define arena_init_static(array_non_compound_literal)                                              \
  RKI_ARENA_INIT_STATIC(array_non_compound_literal)

/// @brief Resets the cursor to the beginning, reclaiming all consumed storage, including padding.
/// @return `self`, for chaining.
/// @note Invalidates all previous allocations and marks. Does not release or clear the backing
/// buffer. A zero-initialized, null-backed arena is supported.
rklib_fun Arena*         arena_clear(Arena* self);

/// @brief Returns the backing-buffer capacity in bytes, or zero for a null-backed arena.
rklib_fun rk_pure size_t arena_cap(const Arena* self);

/// @brief Returns consumed storage in bytes, including alignment padding and unreclaimed storage.
/// @note Returns zero for a null-backed arena; this is not a count of live allocation bytes.
rklib_fun rk_pure size_t arena_used(const Arena* self);

/// @brief Returns unused backing-buffer bytes, or zero for a null-backed arena.
/// @note A request may require additional alignment padding, so this is not necessarily the largest
/// payload that can be allocated with a particular alignment.
rklib_fun rk_pure size_t arena_remaining(const Arena* self);

/// @brief Returns whether the cursor is at the beginning, or true for a null-backed arena.
/// @note Tests consumed storage, not the number of live allocations. Padding can keep the result
/// false after payload storage has been reclaimed.
rklib_fun rk_pure bool   arena_is_empty(const Arena* self);

typedef struct ArenaMark { unsigned char* pos; } ArenaMark;

/// @brief Saves the current cursor as a marker for arena_rewind_to().
/// @return A marker for this arena's current position; a null-backed arena yields a null-position
/// mark.
/// @note A mark neither allocates storage nor keeps allocations alive.
rklib_fun rk_pure ArenaMark arena_mark(const Arena* self);

/// @brief Restores a saved cursor position, reclaiming storage consumed after that position.
/// @param self Arena from which the mark was obtained.
/// @param mark A mark obtained from arena_mark() on this arena, valid for its current state and
/// not ahead of its current cursor.
/// @return `self`, for chaining.
/// @note Invalidates allocations discarded by the rewind and marks into the discarded region.
/// Rewinding a null-backed arena to its own null-position mark is supported. A null-position mark
/// used after the arena was given backing storage is stale, and therefore a contract violation.
rklib_fun Arena*            arena_rewind_to(Arena* self, ArenaMark mark);

/// @brief Returns whether the given allocation ends exactly at the current cursor.
/// @param self Arena owning the allocation.
/// @param ptr Non-null pointer to a valid allocation or zero-size position returned by this arena.
/// @param size Current allocation size in bytes; must describe valid storage at `ptr`.
/// @return True if `ptr + size` equals the current cursor, false otherwise.
/// @note This is an end-position check, not an ownership or allocation-history validator.
rklib_fun rk_pure bool arena_is_top_allocation(const Arena* self, const void* ptr, size_t size);

/// @brief `void* arena_allocate(size_t nbytes, size_t align, Arena* self)` - Allocates aligned
/// storage from the arena. Invokes `RK_ARENA_FAIL` on failure; the default handler aborts.
/// @param nbytes Requested payload size in bytes; may be zero.
/// @param align Required alignment; must be a nonzero power of two.
/// @param self Arena to allocate from; may be null-backed, in which case the request fails.
/// @return A non-null aligned pointer on success, provided any failure override preserves the
/// contract.
/// @note Zero-size requests reserve no payload but advance the cursor by any required alignment
/// padding. Their result may equal the end pointer and must not be dereferenced. Alignment padding
/// must fit.
/// @note Unlike the generic alloc_allocate() wrapper, this direct API does not return null for zero
/// bytes. Such a zero-size position must not be passed to alloc_deallocate()/alloc_reallocate(),
/// which require `NULL` for zero-size allocations.
/// @see arena_try_allocate
rklib_fun void*        arena_allocate(size_t nbytes, size_t align, Arena* self);

/// @brief `void* arena_try_allocate(size_t nbytes, size_t align, Arena* self)` - Like
/// arena_allocate(), but returns `NULL` on insufficient space or a null backing buffer.
/// @note Failure leaves the cursor unchanged. Zero-size requests succeed if the required alignment
/// padding fits, return an aligned position, and consume only that padding.
/// @see arena_allocate
rklib_fun void*        arena_try_allocate(size_t nbytes, size_t align, Arena* self);

/// @brief `void* arena_resize_top(size_t old_size, size_t new_size, Arena* self)` - Resizes the
/// current top allocation without moving its start. Invokes `RK_ARENA_FAIL` on failure.
/// @param old_size Current payload size of the top allocation in bytes; may be zero for a valid
/// zero-size position at the current cursor.
/// @param new_size Desired payload size in bytes; may be zero.
/// @param self Arena owning the top allocation or zero-size position. For a null-backed arena,
/// `old_size` must be zero and the request fails, including when `new_size` is zero.
/// @return The unchanged start pointer on success, including when `new_size` is zero.
/// @note A zero new size reclaims the payload but not preceding alignment padding; the returned
/// pointer then denotes only a zero-size position and must not be dereferenced.
/// @note Failure leaves the cursor unchanged before invoking the failure handler. Overrides that
/// resume must preserve the start address, arena state, and resize guarantees; `align_max` passed
/// to the handler is not necessarily the original allocation alignment.
/// @see arena_try_resize_top
rklib_fun void*        arena_resize_top(size_t old_size, size_t new_size, Arena* self);

/// @brief `void* arena_try_resize_top(size_t old_size, size_t new_size, Arena* self)` - Like
/// arena_resize_top(), but returns `NULL` if the requested size does not fit or the arena is
/// null-backed.
/// @note Failure leaves the cursor unchanged. Success returns the unchanged start pointer even for
/// zero new size. The top-allocation precondition still applies; a null-backed arena requires
/// `old_size == 0`.
/// @note Operates on the implicit top allocation, so `old_size` correctly describing it is a
/// precondition, not a checked failure. To resize a specific allocation that may not be on top,
/// use arena_try_resize() or arena_try_extend().
/// @see arena_resize_top
/// @see arena_try_resize
rklib_fun void*        arena_try_resize_top(size_t old_size, size_t new_size, Arena* self);

/// @brief `void* arena_try_resize(void* ptr, size_t old_size, size_t new_size, Arena* self)` -
/// Attempts to resize a specific allocation in place without moving it; it can grow or shrink.
/// @param ptr Non-null pointer to an allocation or zero-size position owned by this arena; it need
/// not be the top allocation.
/// @param old_size Current payload size in bytes; must match the allocation.
/// @param new_size Desired payload size in bytes; may be zero.
/// @param self Arena owning the allocation.
/// @return `ptr` on success, including when `new_size` is zero. `NULL` if `ptr` is not the top
/// allocation, the requested size does not fit, or the arena is null-backed.
/// @note Failure leaves the arena unchanged; a non-top allocation is an ordinary failure, not a
/// contract violation. Ownership and a correct `old_size` remain preconditions. Preserves the
/// retained bytes. A zero new size reclaims the payload but not preceding alignment padding; the
/// returned pointer then denotes only a zero-size position and must not be dereferenced.
/// @see arena_try_extend
/// @see arena_try_resize_top
rklib_fun void*        arena_try_resize(void* ptr, size_t old_size, size_t new_size, Arena* self);

/// @brief `T* arena_new(T, size_t count, Arena* arena)` - Allocates storage for `count` elements
/// with the type's required alignment.
/// @param T Element type.
/// @param count Element count; may be zero. The byte-size calculation must be representable.
/// @param arena Arena to allocate from.
/// @return A typed pointer to the allocated storage.
/// @note Allocates raw storage; does not invoke C++ constructors. Uses arena_allocate() zero-size
/// and failure semantics.
/// @see arena_allocate
#define arena_new(T, count, arena)                RKI_ARENA_NEW(T, count, arena)

/// @brief `T* arena_try_new(T, size_t count, Arena* arena)` - Like arena_new(), but returns
/// `NULL` on allocation failure without invoking the failure handler.
/// @note Zero count follows arena_try_allocate() semantics and may return a non-null pointer.
/// @see arena_new
/// @see arena_try_allocate
#define arena_try_new(T, count, arena)            arena_try_new_aligned(T, count, alignof(T), arena)

/// @brief `T* arena_new_aligned(T, size_t count, size_t align, Arena* arena)` - Like arena_new(),
/// with explicitly requested alignment.
/// @param T Element type.
/// @param count Element count; may be zero. The byte-size calculation must be representable.
/// @param align A nonzero power of two, at least the alignment required by T.
/// @param arena Arena to allocate from.
/// @return A typed pointer to the allocated storage.
/// @see arena_new
/// @see arena_allocate
#define arena_new_aligned(T, count, align, arena) RKI_ARENA_ALIGNED_NEW(T, count, align, arena)

/// @brief `T* arena_try_new_aligned(T, size_t count, size_t align, Arena* arena)` - Like
/// arena_new_aligned(), but returns `NULL` on allocation failure without invoking the failure
/// handler.
/// @note Zero count follows arena_try_allocate() semantics and may return a non-null pointer.
/// @see arena_new_aligned
/// @see arena_try_allocate
#define arena_try_new_aligned(T, count, align, arena)                                              \
  ((T*)(alloc_log_new(), rk_assert_valid_align(T, align),                                          \
        arena_try_allocate(sizeof_n(T, count), align, arena)))

/// @brief `T* arena_extend(T* ptr, size_t old_count, size_t new_count, Arena* arena)` - Like
/// arena_try_extend(), but invokes `RK_ARENA_FAIL` on failure, including when `ptr` is not the top
/// allocation.
/// @return `ptr` on success, cast to the same pointer type, including when `new_count` is zero.
/// @note Failure leaves the cursor unchanged before invoking the failure handler.
/// @see arena_try_extend
#define arena_extend(ptr, old_count, new_count, arena)                                             \
  ((typeof(ptr))rki_arena_extend(ptr, sizeof_n(*(ptr), old_count), sizeof_n(*(ptr), new_count),    \
                                 arena))

/// @brief `T* arena_try_extend(T* ptr, size_t old_count, size_t new_count, Arena* arena)` -
/// Typed arena_try_resize() using element counts; despite the name, it can also shrink.
/// @param ptr Non-null pointer to an allocation or zero-size position owned by this arena; it need
/// not be the top allocation.
/// @param old_count Current payload element count; must match the allocation.
/// @param new_count Desired element count; may be zero. Byte-size calculations must be
/// representable.
/// @param arena Arena owning the allocation.
/// @return `ptr` on success, cast to the same pointer type, including when `new_count` is zero.
/// `NULL` under the same conditions as arena_try_resize(), including a non-top `ptr`.
/// @note Inherits arena_try_resize() semantics. Does not construct or destroy C++ objects.
/// @see arena_try_resize
#define arena_try_extend(ptr, old_count, new_count, arena)                                         \
  ((typeof(ptr))arena_try_resize(ptr, sizeof_n(*(ptr), old_count), sizeof_n(*(ptr), new_count),    \
                                 arena))

/// @brief Internal allocation callback for the Arena allocator adapter.
/// @note Generic alloc_* wrappers handle zero-size and null-pointer cases before dispatch:
/// allocation callbacks receive positive sizes; reallocation/deallocation callbacks receive valid
/// non-null existing allocations with matching sizes and alignment, and reallocation receives a
/// positive new size. Allocation and reallocation callbacks must return valid storage or handle
/// failure locally. The allocation callback also serves arena_allocate(), whose direct zero-size
/// contract is broader.
rklib_fun alloc_allocation_f   rki_arena_allocate;
rklib_fun alloc_reallocation_f rki_arena_reallocate;
rklib_fun alloc_deallocation_f rki_arena_deallocate;
static const AllocatorVTable   arena_allocator_vtable = {.alloc_f   = rki_arena_allocate,
                                                         .realloc_f = rki_arena_reallocate,
                                                         .dealloc_f = rki_arena_deallocate};

/// @brief `Allocator arena_to_alloc_static(Arena* arena)` - Produces an Allocator initializer
/// borrowing an Arena as its context.
/// @note The Arena must remain at the same address and its backing buffer must remain valid while
/// this handle or allocations using it remain in use.
/// @note Generic alloc_* wrappers normalize zero-size and null-pointer cases before vtable
/// dispatch. Reallocation resizes top allocations in place where possible; non-top shrinks reclaim
/// no storage, and moving growth retains the old storage until it can be reclaimed by rewinding or
/// clearing. Deallocation reclaims only a top allocation's payload; preceding padding remains
/// consumed.
/// @note Clearing or rewinding must not discard storage still accessed by objects using this
/// handle.
/// @see arena_to_alloc
#define arena_to_alloc_static(arena) {.vtab = &arena_allocator_vtable, .ctx = (arena)}

/// @brief Returns an Allocator handle borrowing the given Arena as its context.
/// @note Does not allocate, copy, or take ownership of the arena or its backing buffer.
/// @note Only useful with custom allocators enabled: otherwise alloc_* wrappers accept no explicit
/// allocator and alloc_ctx cannot be replaced, so the handle cannot be passed anywhere. The same
/// applies to arena_to_alloc_static() and arr_allocator.
/// @see arena_to_alloc_static
static_fun rk_const Allocator arena_to_alloc(Arena* arena);

/// @brief Defines a type containing an Allocator handle, an Arena, and an embedded byte buffer.
/// @param size Positive compile-time buffer size in bytes.
/// @note Initialization stores pointers into the object itself. Keep the initialized object at its
/// original address; copying it does not retarget those pointers to the copy.
/// @note Only useful with custom allocators enabled; see arena_to_alloc().
/// @see arr_allocator_init
/// @see arr_allocator_create
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

/// @brief Produces an initializer for an arr_allocator object using its address.
/// @param self Address of the object being initialized.
/// @note This is an initializer, not a runtime reinitialization function. The embedded buffer is
/// borrowed by the embedded Arena, and the Allocator context points to that Arena.
#define arr_allocator_init(self)                                                                   \
  {.vtab = &arena_allocator_vtable, .ctx = &(self)->arena, .arena = arena_init_static((self)->arr)}

/// @brief Declares and initializes an allocator with an embedded arena and fixed-size backing
/// buffer.
/// @param name Name of the object to declare.
/// @param size Positive compile-time buffer size in bytes.
/// @note Use `name.alloc` as the Allocator handle. The object's storage duration follows its
/// declaration context; keep it at its initialized address and alive while dependent allocations
/// are used.
/// @code
/// arr_allocator_create(temp_alloc, 4096);
/// int* ptr = alloc_new(int, 10, temp_alloc.alloc);
/// @endcode
/// @see arr_allocator
#define arr_allocator_create(name, size) arr_allocator(size) name = arr_allocator_init(&name)

#define RKI_AMALG_IMPL_7A588F7E95C26AAF 1
RKI_IGNWARN_CLANG_END()
RKI_HEADER_END
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
/// Allocator binding: the constructors (`vec_init()`, `vec_from()`, `vec_init_list()`) always
/// return a non-NULL Vec bound to the given allocator (default `alloc_ctx`), even for a capacity or
/// count of 0. A non-NULL Vec always has capacity >= 1: a requested capacity of 0 is rounded up to
/// 1, and shrinking an empty Vec keeps one slot. `NULL` means never constructed, or released by
/// `vec_release()`; such a Vec grows lazily from the current `alloc_ctx`. Use `Vec(T) v = NULL`
/// for an empty Vec that performs no allocation.
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
/* inlined from include/rk_vec.h:54: #include "rk_alloc.h" */
/* skipped already-included: "include/rk_alloc.h" */
RKI_HEADER_BEGIN

/// @brief Macro to indicate that an object is a Vec.
/// @param T The type of elements stored in the Vec
/// @note Vec(void) indicates that a Vec of any type is accepted as a parameter, this does not hold
/// for macros which need type information. Using Vec as `Vec(void)` in any macro is undefined as
/// most vec macros rely on type information such as sizeof.
/// @note Array element types (e.g. `Vec(int[5])`) are not supported, typedef'd or not:
/// `vec_push()`, `vec_insert_at`, and every other operation that places a new element relies on
/// plain C assignment (`self[i] = value`), which C disallows for array types regardless of how
/// `Vec(T)` itself expands or whether the array type has a name of its own.
#define Vec(T) T*

/// @brief `Vec(T) vec_init(T, size_t cap, Allocator alloc = alloc_ctx)`
/// - Initialises a Vec from an initial capacity and an optional Allocator.
/// @param T           The desired type of the Vec's elements
/// @param init_cap    The initial capacity of the Vec (in elements); 0 is rounded up to 1.
/// @param allocator   Optional allocator; defaults to `alloc_ctx`.
/// @return A non-NULL, empty Vec bound to `allocator`.
/// @note Always allocates, so the allocator stays bound even for a capacity of 0. For an empty Vec
/// without any allocation, use `Vec(T) v = NULL` instead; it grows from `alloc_ctx`.
///
/// Usage:
/// ```c
/// Vec(int) v1 = vec_init(int, 10);            // int Vec with capacity 10, using alloc_ctx
/// Vec(int) v2 = vec_init(int, 2, my_alloc);   // int Vec with capacity 2, using my_alloc
/// Vec(int) v3 = vec_init(int, 0, my_alloc);   // capacity 1; later growth still uses my_alloc
/// Vec(int) v4 = NULL;                         // no allocation; grows from alloc_ctx
/// ```
#define vec_init(T, init_cap, ...)                                                                 \
  ((Vec(T))((void)static_assert_expr(alignof(T) <= align_max,                                      \
                                     "Over-aligned Types not supported."),                         \
            RKI_OVERLOAD(RKI_VEC_INIT, T, init_cap, ##__VA_ARGS__)))

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
/// typedef struct Pair { int x, y; } Pair;
/// Vec(struct Pair) vec3 = vec_init_list(Pair, ((Pair){1, 2}), ((Pair){3, 4}));
/// ```
#define vec_init_list(T, ...)                                                                      \
  ((Vec(T))((void)static_assert_expr(alignof(T) <= align_max,                                      \
                                     "Over-aligned Types not supported."),                         \
            RKI_VEC_INIT_LIST(T, ##__VA_ARGS__)))

/// @brief `Vec(T) vec_from(T* arr, size_t count, Allocator alloc = alloc_ctx)` - Constructs a new
/// Vec by copying `count` elements from `arr`.
/// @param arr   Source array of `count` elements; its element type becomes the new Vec's element
/// type (via `typeof(*arr)`)
/// @param count Number of elements to copy
/// @param alloc Allocator Optional, defaults to `alloc_ctx`
/// @return A non-NULL Vec bound to `alloc`, containing a copy of `arr`'s first `count` elements.
/// For `count == 0` it is empty with capacity 1, and `arr` is not accessed (it may be NULL).
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
  ((typeof(*(arr))*)RKI_OVERLOAD(RKI_VEC_FROM, arr, count, ##__VA_ARGS__))

/// @brief `void vec_release(Vec(T)& self)` - Frees the underlying allocation and sets the Vec to
/// NULL.
/// @note The allocator binding is released too: if the Vec is used again, it grows from the current
/// `alloc_ctx`. This is the only operation that returns a constructed Vec to NULL. Safe to call on
/// a NULL Vec.
#define vec_release(self) ((void)RKI_VEC_RELEASE(self))

/// @brief Returns the number of elements in the vec, 0 if `self` is NULL.
rklib_fun rk_pure size_t vec_count(const Vec(void) self);
#define vec_COUNT(self) rk_to_rvalue(RKI_VEC_COUNT(self))

/// @brief Alias for `vec_count()`
rklib_fun rk_pure size_t vec_len(const Vec(void) self);
#define vec_LEN(self) vec_COUNT(self)

/// @brief Returns the current capacity of the Vec, 0 iff `self` is NULL.
rklib_fun rk_pure size_t vec_cap(const Vec(void) self);
#define vec_CAP(self) rk_to_rvalue(RKI_VEC_CAP(self))

/// @brief Returns the Allocator the Vec was constructed with, or `alloc_ctx` if `self` is `NULL`
/// or custom allocators are disabled.
rklib_fun rk_pure Allocator vec_allocator(const Vec(void) self);
#define vec_ALLOCATOR(self) rk_to_rvalue(RKI_VEC_ALLOCATOR(self))

/// @brief Returns whether the count of a Vec is zero.
rklib_fun rk_pure bool vec_is_empty(const Vec(void) self);

/// @brief Clears the contents of `self` by setting its count to 0.
rklib_fun void         vec_clear(Vec(void) self);

/// @brief `size_t vec_allocation_size(Vec(T) self)` - Returns the total size of memory allocated
/// for the Vec in bytes, including its header, 0 iff `self` is NULL.
#define vec_allocation_size(self) RKI_VEC_ALLOCATION_SIZE(self)

/// @brief Returns the remaining count of elements that can be pushed to a vec without reallocation.
rklib_fun rk_pure size_t vec_remaining(const Vec(void) self);

/// @brief Returns whether an index is within the range of a Vec.
rklib_fun rk_pure bool   vec_index_in_range(const Vec(void) self, size_t idx);

/// @brief `void vec_reserve(Vec(T)& self, size_t new_cap)` - Grows the Vec to be able to hold at
/// least `new_cap` elements.
/// @attention **Arguments with side effects are not safe in `vec_` macros**
/// @note Reassigns `self`, if necessary
#define vec_reserve(self, new_cap)    ((void)RKI_VEC_RESERVE(self, new_cap))

/// @brief `void vec_resize(Vec(T)& self, size_t len)` - Resizes the Vec to `len`, expanding the
/// capacity by reallocating and creating uninitialised objects if necessary.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Reassigns `self`, if necessary
#define vec_resize(self, len)         ((void)RKI_VEC_RESIZE(self, len))

/// @brief `void vec_shrink_to_fit(Vec(T)& self)` - Shrinks the Vec's capacity to the next power of
/// two greater than or equal to its length (matching `str_shrink_to_fit()`'s convention), leaving
/// some slack to reduce reallocation on subsequent growth.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Reassigns `self`, if necessary. An empty Vec shrinks to capacity 1 rather than being
/// deallocated, so it keeps its allocator; use `vec_release()` to free it entirely.
/// @note Use `vec_shrink_to_fit_exact()` for an exact-capacity shrink.
#define vec_shrink_to_fit(self)       ((void)RKI_VEC_SHRINK_TO_FIT(self))

/// @brief `void vec_shrink_to_fit_exact(Vec(T)& self)` - Shrinks the Vec's capacity to be exactly
/// equal to its length, or 1 if it is empty.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Reassigns `self`, if necessary. An empty Vec keeps one slot rather than being
/// deallocated, so it keeps its allocator; use `vec_release()` to free it entirely.
#define vec_shrink_to_fit_exact(self) ((void)RKI_VEC_SHRINK_TO_FIT_EXACT(self))

/// @brief `void vec_assign(Vec(T)& self, T* arr, size_t count)` - Assigns `count` objects of `arr`
/// to the Vec, overriding its contents and expanding `self`, if necessary.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @attention `arr[0..count)` must not overlap the Vec's own backing allocation: if growth is
/// triggered, the old buffer is freed before the copy from `arr` happens, turning an `arr` that
/// points into it into a use-after-free; even without growth, the underlying copy is a plain
/// `memcpy`, which is undefined for overlapping source and destination.
/// @note Reassigns `self`, if necessary.
#define vec_assign(self, arr, count)  ((void)RKI_VEC_ASSIGN(self, arr, count))

// todo docs
#define vec_begin(self)               ((typeof(self))(self))

/// @brief `T* vec_end(Vec(T) self)` - Returns a pointer one past the end of a the elements of
/// `self` or `NULL` if `self` is `NULL`.
#define vec_end(self)                 ((self) ? ((self) + RKI_VEC_COUNT(self)) : (self))

/// @brief Returns the first element as an lvalue, preserving the element type's constness.
/// @pre The Vec is nonempty.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note The precondition is asserted in debug builds; invalid access is undefined in release.
#define vec_front(self)               (((typeof(self))rki_vec_check_front(self))[0])

/// @brief Returns the last element as an lvalue, preserving the element type's constness.
/// @pre The Vec is nonempty.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note The precondition is asserted in debug builds; invalid access is undefined in release.
#define vec_back(self)                (*((typeof(self))rki_vec_check_back(sizeof(*(self)), self)))

/// @brief Returns the element at zero-based index `idx` as an lvalue, preserving the element
/// type's constness.
/// @param self The Vec, by value.
/// @param idx Zero-based element index.
/// @pre `idx < vec_count(self)`.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note The precondition is asserted in debug builds; invalid access is undefined in release.
/// @see vec_peek_at
#define vec_at(self, idx)            (*((typeof(self))rki_vec_check_at(sizeof(*(self)), self, idx)))

/// @brief Returns a pointer to the first element, or `NULL` if the Vec is empty or uninitialised.
/// @return An element pointer preserving the element type's constness.
/// @attention **Arguments with side effects are not safe in vec_ macros**
#define vec_peek_front(self)         ((typeof(self))rki_vec_peek_at(sizeof(*(self)), self, 0))

/// @brief Returns a pointer to the last element, or `NULL` if the Vec is empty or uninitialised.
/// @return An element pointer preserving the element type's constness.
/// @attention **Arguments with side effects are not safe in vec_ macros**
#define vec_peek_back(self)          ((typeof(self))rki_vec_peek_back(sizeof(*(self)), self))

/// @brief Returns a pointer to the element at zero-based index `idx`, or `NULL` if the index is
/// out of bounds (including an empty or uninitialised Vec).
/// @param self The Vec, by value.
/// @param idx Zero-based element index.
/// @return An element pointer preserving the element type's constness.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Bounds are checked in both debug and release builds.
#define vec_peek_at(self, idx)       ((typeof(self))rki_vec_peek_at(sizeof(*(self)), self, idx))

/// @brief `void vec_push(Vec(T)& self, T obj)` - Pushes a value onto the Vec, resising the
/// allocation, if necessary.
/// @attention **`obj` must not modify the vec due to sequencing issues**
/// @note Reassigns `self`, if necessary
#define vec_push(self, obj)          ((void)RKI_VEC_PUSH(self, obj)) // NOLINT

/// @brief `void vec_push_n(Vec(T)& self, T* arr, size_t count)` - Copies `count` values of `arr`
/// onto `self`. `arr` must be a pointer variable of type `T*`.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @attention `arr[0..count)` must not overlap the Vec's own backing allocation, for the same
/// reasons documented on `vec_assign()`.
/// @note Reassigns `self`, if necessary
#define vec_push_n(self, arr, count) ((void)RKI_VEC_PUSH_N(self, arr, count))

/// @brief `void vec_push_unchecked(Vec(T)& self, T obj)` - Pushes a value onto the Vec, not
/// checking for capacity.
/// @attention **`obj` must not modify the vec due to sequencing issues**
#define vec_push_unchecked(self, obj) ((void)(RKI_VEC_PUSH_U(self, obj)))

/// @brief `T vec_pop(Vec(T)& self)` - Pops the last value off the Vec and decreases its length.
/// @return The popped value
/// @pre The Vec is nonempty; use `vec_try_pop()` to check safely.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note The precondition is asserted in debug builds; invalid access is undefined in release.
#define vec_pop(self)                 ((self)[--RKI_VEC_COUNT(rki_check_vec_pop(self))])

/// @brief `bool vec_try_pop(Vec(T) self, T* out)` - Removes the last value and writes it to `*out`,
/// if the Vec is nonempty.
/// @param out Destination for the removed value. Evaluated at most once, and only when a value is
/// removed; left untouched if the Vec is empty or uninitialised.
/// @return `true` if a value was removed, `false` if the Vec was empty or uninitialised.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Never reallocates, so `self` is not reassigned.
#define vec_try_pop(self, out)        RKI_VEC_TRY_POP(self, out)

/// @brief `T* vec_pop_n(Vec(T)& self, size_t count)` - Pops 'count' values off the Vec, decreasing
/// its length.
/// @return A pointer to the popped memory region, to copy away from
/// @pre `count <= vec_count(self)`.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note The precondition is asserted in debug builds; invalid access is undefined in release.
#define vec_pop_n(self, count)        (typeof(self))rki_vec_pop_n(sizeof(*(self)), self, count)

/// @brief `void vec_insert_at(Vec(T)& self, size_t idx, T obj)` - Inserts an object at index `idx`,
/// shifting subsequent elements back and expanding `self`, if necessary.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Reassigns `self`, if necessary. Behavior is undefined if `idx` is out of bounds.
#define vec_insert_at(self, idx, obj) ((void)(RKI_VEC_INSERT_AT(self, idx, obj)))

/// @brief `void vec_insert_at_unordered(Vec(T)& self, size_t idx, T obj)` - Inserts `obj` at index
/// `idx` without preserving element order. The element currently at `idx` is moved to the back
/// before `obj` is placed at `idx`. O(1) (ignoring possible reallocation), unlike `vec_insert_at`
/// which is O(n).
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Reassigns `self`, if necessary. Behavior is undefined if `idx` > vec_count(self).
#define vec_insert_at_unordered(self, idx, obj) ((void)RKI_VEC_INSERT_AT_UNORDERED(self, idx, obj))

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
  ((void)(RKI_VEC_INSERT_ARR_AT(self, idx, ptr, count)))

/// @brief `void vec_erase_at(Vec(T) self, size_t idx)` - Erases an element from `self` at index
/// `idx`, shifting subsequent elements forward.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Behavior is undefined if `idx` is out of bounds.
#define vec_erase_at(self, idx) ((void)(RKI_VEC_ERASE_AT(self, idx)))

/// @brief `void vec_erase_at_unordered(Vec(T) self, size_t idx)` - Erases an element at index `idx`
/// without preserving element order. The last element is moved into the erased slot. O(1), unlike
/// `vec_erase_at` which is O(n).
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Behavior is undefined if `idx` is out of bounds.
#define vec_erase_at_unordered(self, idx)                                                          \
  ((void)(((self)[idx] = (self)[rki_decrease_index_check(self)])))

/// @brief `void vec_erase_at_n(Vec(T) self, size_t idx, size_t count)` - Erases `count` elements at
/// index `idx` from `self`, shifting subsequent elements forward.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Behavior is undefined if `idx` + count > vec_count()
#define vec_erase_at_n(self, idx, count) ((void)(RKI_VEC_ERASE_AT_N(self, idx, count)))

/// @brief Reverse the elements of a Vec in place.
#define vec_reverse(vec)                                                                           \
  do {                                                                                             \
    typeof(vec) RKI__BEG = (vec);                                                                  \
    if (!vec_count(RKI__BEG)) { break; }                                                           \
    typeof(RKI__BEG) RKI__END = RKI__BEG + RKI_VEC_COUNT(RKI__BEG) - 1;                            \
    for (; RKI__BEG < RKI__END; ++RKI__BEG, --RKI__END) { rk_SWAP(*RKI__BEG, *RKI__END); }         \
  } while (0)

/// @brief Visits every element in index order.
/// @param vec The Vec to iterate. Evaluated once.
/// @param it  Iterator name (a pointer into the Vec; access via `*it`).
/// @note break stops traversal; continue advances to the next element.
///
/// Usage:
/// ```c
/// vec_foreach(vec, it) { printf("%d\n", *it); }
/// ```
#define vec_foreach(vec, it)          RKI_VEC_FOREACH(vec, it)

/// @brief Like `vec_foreach()`, but iterates in reverse index order. Same parameters and contract.
#define vec_foreach_reversed(vec, it) RKI_VEC_FOREACH_REVERSED(vec, it)

/// @brief Erases every element satisfying `pred`.
/// @param vec  The Vec to erase from. Evaluated once.
/// @param it   Iterator name (access via `*it`).
/// @param pred Predicate expression, evaluated once per original element.
/// @note The predicate must not structurally modify the Vec.
///
/// Usage:
/// ```c
/// Vec(int) vec = vec_init(int, 10);
/// vec_push(vec, 1), vec_push(vec, 2);
/// vec_erase_if(vec, it, *it % 2); // remove odd numbers
/// ```
#define vec_erase_if(vec, it, pred)   RKI_VEC_ERASE_IF(vec, it, pred)

#define RKI_AMALG_IMPL_253EB59549665B16 1
RKI_HEADER_END
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
RKI_HEADER_BEGIN

/// @brief Define a heap type and functions for a given element type.
///
/// This macro generates a complete, type-specific binary min-heap API for the given element type.
///
/// @param T       Name of the element type. Must be an identifier; use a typedef for a pointer or
///                struct type.
/// @param CMP_FUN Comparison function (`int CMP_FUN(T a, T b)`), returning negative, zero, or
///                positive, following the same convention as `strcmp` and the tree comparators.
/// @attention Must be invoked at file scope, once per `T`.
#define HEAP_DEFINE(T, CMP_FUN)          RKI_HEAP_DEFINE(T, CMP_FUN)

/// @brief Macro to indicate that an object is a Heap.
/// @param T The type of elements stored in the Heap
#define Heap(T)                          Heap_##T

/// @brief `Heap(T) heap_init(T, size_t cap, Allocator alloc = alloc_ctx)` - Initialises and
/// returns an empty Heap.
/// @param T        The desired type of the Heap's elements
/// @param cap      The initial capacity of the backing Vec (in elements)
/// @param alloc    Optional allocator; defaults to `alloc_ctx`. See `vec_init()`.
/// @return An initialised, empty `Heap(T)`
/// @note Always allocates the backing Vec, so the allocator stays bound even for `cap == 0`
/// (rounded up to 1). A zero-initialized `Heap(T)` is also a valid, empty heap; it performs no
/// allocation and grows from `alloc_ctx`.
#define heap_init(T, cap, ...)           ((Heap(T)){.data = vec_init(T, cap, ##__VA_ARGS__)})

/// @brief `Heap(T) heap_from(T, const T* arr, size_t n, Allocator alloc = alloc_ctx)` - Constructs
/// a new Heap by copying `n` values from `arr` and heapifying them, in O(n) overall.
/// @param T     Element type
/// @param arr   Source array of `n` values
/// @param n     Number of values to copy
/// @param alloc Optional allocator; defaults to `alloc_ctx`
/// @return A new `Heap(T)` containing a heap-ordered copy of `arr`'s first `n` values
#define heap_from(T, arr, n, ...)        RKI_OVERLOAD(RKI_HEAP_FROM, T, arr, n, ##__VA_ARGS__)

/// @brief `Heap(T) heap_adopt(T, Vec(T) vec)` - Constructs a new Heap by taking ownership of `vec`
/// and heapifying it in place, with no copy.
/// @param T   Element type
/// @param vec An existing `Vec(T)`, passed by value
/// @return A `Heap(T)` wrapping `vec`'s own storage, now in heap order
/// @attention `vec` is consumed: its storage now belongs to the returned Heap. Do not read, mutate,
/// or `vec_release()` the original `vec` variable afterwards; release the Heap instead.
#define heap_adopt(T, vec)               RKI_HEAP_PUB(T, adopt)(vec)

/// @brief `void heap_release(Heap(T)* self)` - Frees the backing Vec and resets the Heap to an
/// empty state.
/// @note Releases the allocator binding too, like `vec_release()`: if used again, the Heap grows
/// from the current `alloc_ctx`.
#define heap_release(self)               vec_release((self)->data)

/// @brief `size_t heap_count(const Heap(T)* self)` - Returns the number of elements in the Heap.
#define heap_count(self)                 vec_count((self)->data)

/// @brief `size_t heap_cap(const Heap(T)* self)` - Returns the current capacity of the backing Vec.
#define heap_cap(self)                   vec_cap((self)->data)

/// @brief `Allocator heap_allocator(const Heap(T)* self)` - Returns the Allocator the Heap's
/// backing Vec was constructed with, or `alloc_ctx` if the Heap was never initialized or custom
/// allocators are disabled.
#define heap_allocator(self)             vec_allocator((self)->data)

/// @brief `bool heap_is_empty(const Heap(T)* self)` - Returns `true` iff the Heap contains no
/// elements.
#define heap_is_empty(self)              ((bool)(heap_count(self) == 0))

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
#define heap_assign(T, self, arr, n)     RKI_HEAP_PUB(T, assign)(self, arr, n)

/// @brief `const T* heap_peek(T, const Heap(T)* self)` - Returns a pointer to the minimum element
/// without removing it.
/// @param T Element type
/// @return Pointer to the minimum element, or `NULL` if the Heap is empty
/// @note Invalidated by any later mutation of the Heap.
#define heap_peek(T, self)               RKI_HEAP_PUB(T, peek)(self)

/// @brief Returns the top element according to the heap's ordering as a const lvalue.
/// @param T Element type.
/// @pre The Heap is nonempty; use `heap_peek()` to check safely.
/// @note Always const, even for a mutable Heap: writing the top in place would break the heap
/// order. Use `heap_replace_top()` to change it.
/// @note The precondition is asserted in debug builds; invalid access is undefined in release.
/// @note Invalidated by any later mutation of the Heap.
/// @see heap_peek
#define heap_top(T, self)                (*RKI_HEAP_PUB(T, top_ptr)(self))

/// @brief `void heap_push(T, Heap(T)* self, T value)` - Inserts `value` into the Heap.
/// @param T     Element type
/// @param value Value to insert. Evaluated once.
/// @note A push may reallocate the backing Vec, invalidating prior pointers into it.
#define heap_push(T, self, value)        RKI_HEAP_PUB(T, push)(self, value)

/// @brief `T heap_pop(T, Heap(T)* self)` - Removes and returns the minimum element.
/// @param T Element type
/// @return The (former) minimum element
/// @pre The Heap is nonempty; use `heap_try_pop()` to check safely.
/// @note The precondition is asserted in debug builds; invalid access is undefined in release.
#define heap_pop(T, self)                RKI_HEAP_PUB(T, pop)(self)

/// @brief `bool heap_try_pop(T, Heap(T)* self, T* out)` - Removes the minimum element and writes
/// it to `*out`, if the Heap is nonempty.
/// @param T   Element type
/// @param out Destination for the removed value. Left untouched if the Heap is empty.
/// @return `true` if an element was removed, `false` if the Heap was empty
#define heap_try_pop(T, self, out)       RKI_HEAP_PUB(T, try_pop)(self, out)

/// @brief `T heap_replace_top(T, Heap(T)* self, T value)` - Removes the minimum element and
/// inserts `value`, in a single sift-down.
/// @param T     Element type
/// @param value Value to insert in place of the removed minimum. Evaluated once.
/// @return The (former) minimum element
/// @pre The Heap is nonempty.
/// @note The precondition is asserted in debug builds; invalid access is undefined in release.
/// @note Equivalent to, but cheaper than, `heap_pop()` followed by `heap_push()`: it never shrinks
/// or reallocates the backing Vec.
#define heap_replace_top(T, self, value) RKI_HEAP_PUB(T, replace_top)(self, value)

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
#define heap_extend(T, self, arr, n)     RKI_HEAP_PUB(T, extend)(self, arr, n)

/// @brief Visits every element in backing-array order.
///
/// A Heap's only real contract is the heap invariant (min/max at the root, O(log n)
/// push/pop/replace_top) -- the backing array's exact layout beyond that is an implementation
/// detail of how elements landed there via sift-up/sift-down, not a property the caller can rely
/// on. There is deliberately no `heap_foreach_reversed`: reversing an order that was never part of
/// the contract (same reasoning as `pool_foreach` having no `_reversed`, and Dict/Set's
/// "unspecified slot order" never getting one either) wouldn't add anything over calling this.
/// @param self Pointer to the Heap. Evaluated once.
/// @param it   Iterator name. Pointer to a const element.
/// @note break stops traversal; continue advances to the next element.
/// @note Do not modify the Heap during traversal.
///
/// Usage:
/// ```c
/// heap_foreach(&h, it) { printf("%d\n", *it); }
/// ```
#define heap_foreach(self, it)           vec_foreach((const typeof(*(self)->data)*)(self)->data, it)

/// @brief Erases every element satisfying `pred`, then restores the heap invariant.
/// @param T    Element type.
/// @param self Pointer to the mutable Heap. Evaluated once.
/// @param it   Iterator name (access via `*it`).
/// @param pred Predicate expression, evaluated once per original element.
/// @note The predicate must not structurally modify the Heap.
/// @note Takes O(n) time and does not allocate.
///
/// Usage:
/// ```c
/// heap_erase_if(int, &h, it, *it % 2 == 0); // drop even values
/// ```
#define heap_erase_if(T, self, it, pred) RKI_HEAP_ERASE_IF(T, self, it, pred)

#define RKI_AMALG_IMPL_C643CC5EB236C471 1
RKI_HEADER_END
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
RKI_HEADER_BEGIN

/// @brief Generates a deque type and its operations for T. Invoke once per T at file scope.
/// @param T Name of the element type. Must be a plain type identifier; use a typedef for a pointer
/// or struct type.
/// @attention Must be invoked at file scope, once per `T`.
/// @note Allocator functions handle allocation failures according to `rk_alloc.h`.
#define DEQUE_DEFINE(T)               RKI_DEQUE_DEFINE(T)

/// @brief Macro to indicate that an object is a Deque.
/// @param T The type of elements stored in the Deque
#define Deque(T)                      Deque_##T

/// @brief `Deque(T) deque_init(T, size_t capacity, Allocator alloc = alloc_ctx)` - Initialises and
/// returns an empty Deque.
/// @param T        The desired type of the Deque's elements
/// @param capacity The initial capacity of the backing buffer (in elements)
/// @param alloc    Optional allocator; defaults to `alloc_ctx`
/// @return An initialised, empty `Deque(T)`
/// @note A zero-initialized `Deque(T)` is also a valid, empty deque; it allocates using `alloc_ctx`
/// on first insertion.
#define deque_init(T, cap, ...)       RKI_OVERLOAD(RKI_DEQUE_INIT, T, cap, ##__VA_ARGS__)

/// @brief `Deque(T) deque_from(T, const T* arr, size_t n, Allocator alloc = alloc_ctx)` -
/// Constructs a new Deque by copying `n` values from `arr`, in front-to-back order.
/// @param T     Element type
/// @param arr   Source array of `n` values
/// @param n     Number of values to copy
/// @param alloc Optional allocator; defaults to `alloc_ctx`
/// @return A new `Deque(T)` containing a copy of `arr`'s first `n` values
#define deque_from(T, arr, n, ...)    RKI_OVERLOAD(RKI_DEQUE_FROM, T, arr, n, ##__VA_ARGS__)

/// @brief `void deque_release(T, Deque(T)* self)` - Frees the backing buffer and resets the Deque
/// to an empty state.
/// @param T Element type
/// @note Safe to call on a zero-initialized Deque.
#define deque_release(T, self)        RKI_DEQUE_PUB(T, release)(self)

/// @brief `size_t deque_count(Deque(T)* self)` - Returns the number of elements stored in the
/// Deque.
/// @note A same-named, real, typed function also exists per instantiation (e.g.
/// `deque_int_count`) purely for discoverability/direct use (IDE completion, taking its address,
/// cross-container generic dispatch); this macro is the one meant for everyday use and does not
/// need `T`, since it's a plain field read with no type-dependent logic.
#define deque_count(self)             ((size_t)(self)->count)

/// @brief `size_t deque_cap(Deque(T)* self)` - Returns the current capacity of the backing buffer.
/// Always a power of two (or zero).
#define deque_cap(self)               ((size_t)(self)->cap)

/// @brief `Allocator deque_allocator(Deque(T)* self)` - Returns the Allocator the Deque was
/// constructed with, or `alloc_ctx` if the Deque was never initialized or custom allocators are
/// disabled.
#define deque_allocator(self)         RKI_allocatorof(self)

/// @brief `bool deque_is_empty(Deque(T)* self)` - Returns `true` iff the Deque contains no
/// elements.
#define deque_is_empty(self)          ((bool)((self)->count == 0))

/// @brief `void deque_clear(T, Deque(T)* self)` - Removes all elements without freeing the backing
/// buffer.
/// @param T Element type
#define deque_clear(T, self)          RKI_DEQUE_PUB(T, clear)(self)

/// @brief `void deque_reserve(T, Deque(T)* self, size_t cap)` - Ensures the backing buffer holds at
/// least `cap` elements without reallocating.
/// @param T   Element type
/// @param cap Minimum capacity to reserve (in elements)
/// @note Existing elements retain their logical order.
#define deque_reserve(T, self, cap)   RKI_DEQUE_PUB(T, reserve)(self, cap)

/// @brief `void deque_shrink_to_fit(T, Deque(T)* self)` - Shrinks the Deque's capacity to the next
/// power of two greater than or equal to its length, with a floor of 8 for a nonempty Deque
/// (matching the same minimum `deque_init()`/`deque_reserve()` enforce), leaving contents
/// unchanged.
/// @param T Element type
/// @note Frees the backing buffer entirely if the Deque is empty.
#define deque_shrink_to_fit(T, self)  RKI_DEQUE_PUB(T, shrink_to_fit)(self)

/// @brief `void deque_assign(T, Deque(T)* self, const T* arr, size_t n)` - Replaces the Deque's
/// contents with a copy of `arr`'s first `n` values, reusing the existing backing buffer (growing
/// it if necessary) rather than allocating a new one.
/// @param T   Element type
/// @param arr Source array of `n` values
/// @param n   Number of values to copy
/// @attention `arr[0..n)` must not overlap the Deque's own backing allocation, for the same reasons
/// documented on `deque_push_back_n()`.
#define deque_assign(T, self, arr, n) RKI_DEQUE_PUB(T, assign)(self, arr, n)

/// @brief Returns the first element as an lvalue, mutable for `Deque(T)* self` and const for
/// `const Deque(T)* self`. Like `vec_front()`, requires a nonempty Deque.
/// @param T Element type
/// @return Lvalue for the first element
/// @pre The Deque is nonempty; use `deque_peek_front()` to check safely.
/// @note The precondition is asserted in debug builds; invalid access is undefined in release.
/// @note Invalidated by any later mutation of the Deque.
#define deque_front(T, self)                                                                       \
  (*_Generic((self),                                                                               \
       const Deque(T)*: RKI_DEQUE_PUB(T, front_const),                                             \
       default: RKI_DEQUE_PUB(T, front))(self))

/// @brief Returns the last element as an lvalue, mutable for `Deque(T)* self` and const for
/// `const Deque(T)* self`. Like `vec_back()`, requires a nonempty Deque.
/// @param T Element type
/// @return Lvalue for the last element
/// @pre The Deque is nonempty; use `deque_peek_back()` to check safely.
/// @note The precondition is asserted in debug builds; invalid access is undefined in release.
/// @note Invalidated by any later mutation of the Deque.
#define deque_back(T, self)                                                                        \
  (*_Generic((self),                                                                               \
       const Deque(T)*: RKI_DEQUE_PUB(T, back_const),                                              \
       default: RKI_DEQUE_PUB(T, back))(self))

/// @brief Returns the element at the zero-based logical index (counting from the front) as an
/// lvalue, mutable for `Deque(T)* self` and const for `const Deque(T)* self`.
/// @param T     Element type
/// @param index Zero-based logical index
/// @pre `index < deque_count(self)`.
/// @note The precondition is asserted in debug builds; invalid access is undefined in release.
/// @note Invalidated by any later mutation of the Deque.
/// @see deque_peek_at
#define deque_at(T, self, index)                                                                   \
  (*_Generic((self), const Deque(T)*: RKI_DEQUE_PUB(T, at_const), default: RKI_DEQUE_PUB(T, at))(  \
      (self), (index)))

/// @brief Returns a pointer to the element at the zero-based logical index (counting from the
/// front): `T*` for `Deque(T)* self`, `const T*` for `const Deque(T)* self`.
/// @param T     Element type
/// @param index Zero-based logical index
/// @return Pointer to the element, or `NULL` if `index` is out of bounds (including an empty
/// Deque).
/// @note Bounds are checked in both debug and release builds.
/// @note Invalidated by any later mutation of the Deque.
#define deque_peek_at(T, self, index)                                                              \
  _Generic((self),                                                                                 \
      const Deque(T)*: RKI_DEQUE_PUB(T, peek_at_const),                                            \
      default: RKI_DEQUE_PUB(T, peek_at))((self), (index))

/// @brief Returns a pointer to the first element: `T*` for `Deque(T)* self`, `const T*` for
/// `const Deque(T)* self`.
/// @param T Element type
/// @return Pointer to the first element, or `NULL` if the Deque is empty
/// @note Invalidated by any later mutation of the Deque.
#define deque_peek_front(T, self)                                                                  \
  _Generic((self),                                                                                 \
      const Deque(T)*: RKI_DEQUE_PUB(T, peek_front_const),                                         \
      default: RKI_DEQUE_PUB(T, peek_front))(self)

/// @brief Returns a pointer to the last element: `T*` for `Deque(T)* self`, `const T*` for
/// `const Deque(T)* self`.
/// @param T Element type
/// @return Pointer to the last element, or `NULL` if the Deque is empty
/// @note Invalidated by any later mutation of the Deque.
#define deque_peek_back(T, self)                                                                   \
  _Generic((self),                                                                                 \
      const Deque(T)*: RKI_DEQUE_PUB(T, peek_back_const),                                          \
      default: RKI_DEQUE_PUB(T, peek_back))(self)

/// @brief `void deque_push_front(T, Deque(T)* self, T value)` - Inserts `value` at the front of the
/// Deque.
/// @param T     Element type
/// @param value Value to insert. Evaluated once.
/// @note May reallocate the backing buffer, invalidating prior pointers into it.
#define deque_push_front(T, self, value)        RKI_DEQUE_PUB(T, push_front)(self, value)

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
#define deque_push_front_n(T, self, arr, count) RKI_DEQUE_PUB(T, push_front_n)(self, arr, count)

/// @brief `void deque_push_back(T, Deque(T)* self, T value)` - Inserts `value` at the back of the
/// Deque.
/// @param T     Element type
/// @param value Value to insert. Evaluated once.
/// @note May reallocate the backing buffer, invalidating prior pointers into it.
#define deque_push_back(T, self, value)         RKI_DEQUE_PUB(T, push_back)(self, value)

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
#define deque_push_back_n(T, self, arr, count)  RKI_DEQUE_PUB(T, push_back_n)(self, arr, count)

/// @brief `T deque_pop_front(T, Deque(T)* self)` - Removes and returns the first element.
/// @param T Element type
/// @return The (former) first element
/// @attention Requires a nonempty Deque.
#define deque_pop_front(T, self)                RKI_DEQUE_PUB(T, pop_front)(self)

/// @brief `T deque_pop_back(T, Deque(T)* self)` - Removes and returns the last element.
/// @param T Element type
/// @return The (former) last element
/// @attention Requires a nonempty Deque.
#define deque_pop_back(T, self)                 RKI_DEQUE_PUB(T, pop_back)(self)

/// @brief `bool deque_try_pop_front(T, Deque(T)* self, T* out)` - Removes the first element and
/// writes it to `*out`, if the Deque is nonempty.
/// @param T   Element type
/// @param out Destination for the removed value. Left untouched if the Deque is empty.
/// @return `true` if an element was removed, `false` if the Deque was empty
#define deque_try_pop_front(T, self, out)       RKI_DEQUE_PUB(T, try_pop_front)(self, out)

/// @brief `bool deque_try_pop_back(T, Deque(T)* self, T* out)` - Removes the last element and
/// writes it to `*out`, if the Deque is nonempty.
/// @param T   Element type
/// @param out Destination for the removed value. Left untouched if the Deque is empty.
/// @return `true` if an element was removed, `false` if the Deque was empty
#define deque_try_pop_back(T, self, out)        RKI_DEQUE_PUB(T, try_pop_back)(self, out)

/// @brief Visits every element in front-to-back order.
/// @param self Pointer to the Deque. Evaluated once.
/// @param it   Iterator name. Element constness follows self.
/// @note break stops traversal; continue advances to the next element.
/// @note Do not structurally modify the Deque during traversal.
///
/// Usage:
/// ```c
/// deque_foreach(&q, it) { printf("%d\n", *it); }
/// ```
#define deque_foreach(self, it)                 RKI_DEQUE_FOREACH(self, it)

/// @brief Like `deque_foreach()`, but iterates in back-to-front order. Same parameters and
/// contract.
#define deque_foreach_reversed(self, it)        RKI_DEQUE_FOREACH_REVERSED(self, it)

/// @brief Erases every element satisfying `pred`, preserving the retained elements' relative
/// order.
/// @param self Pointer to a mutable Deque. Evaluated once.
/// @param it   Iterator name. Access the current element through `*it`.
/// @param pred Predicate expression, evaluated once per original element.
/// @note The predicate must not structurally modify the Deque.
/// @note Does not allocate or change capacity.
///
/// Usage:
/// ```c
/// deque_erase_if(&q, it, *it % 2 == 0); // remove even numbers
/// ```
#define deque_erase_if(self, it, pred)          RKI_DEQUE_ERASE_IF(self, it, pred)

#define RKI_AMALG_IMPL_E739244C6328D655 1
RKI_HEADER_END
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
/// @brief Growable collection of arenas for temporary allocations and bulk reuse.
/// @details Stores a Vec of Arena objects and the index of the active arena. Growth allocates or
/// reuses backing buffers without moving existing allocations; reallocation may move an allocation.
/// Initialize with arenastack_init(), allocate with arenastack_allocate() or the typed macros,
/// reclaim storage with arenastack_clear() or arenastack_rewind_to(), and release it with
/// arenastack_release().
/// @note Concurrent access involving allocation, resizing, clearing, rewinding, or release requires
/// external synchronization. Independent stacks may be used independently, subject to their
/// allocator's thread-safety requirements.
/// @see rk_alloc.h
/// @see rk_arena.h
/// @see rk_vec.h
/// @{
#ifndef RK_ARENASTACK_H
#define RK_ARENASTACK_H
/* inlined from include/rk_arenastack.h:20: #include "rk_arena.h" */
/* skipped already-included: "include/rk_arena.h" */
/* inlined from include/rk_arenastack.h:21: #include "rk_vec.h" */
/* skipped already-included: "include/rk_vec.h" */
RKI_HEADER_BEGIN

/// @brief Owns arena backing buffers and a vector of arena descriptors, with an active arena index.
/// @note ArenaStack pointer arguments must be non-null and refer to initialized objects. A
/// zero-initialized stack supports lazy allocation and has no backing buffers initially.
/// Copying this structure does not duplicate its owned resources; do not treat copies as
/// independent owners. Allocator handles borrowing the stack require it to remain at the same
/// address.
typedef struct ArenaStack {
  size_t     arena_size;
  Vec(Arena) arenas;
  size_t     cur;
} ArenaStack;

/// @brief `ArenaStack arenastack_init(size_t arena_size, Allocator alloc = alloc_ctx)` -
/// Initializes an owning stack and allocates its first arena.
/// @param arena_size Minimum backing-buffer size for new arenas, rounded up to a power of two; zero
/// selects one byte. The rounded size must be representable in size_t.
/// @param alloc Backing allocator; defaults to alloc_ctx. Explicit selection requires custom
/// allocators.
/// @return An initialized ArenaStack owning its first buffer and descriptor vector.
/// @note Larger requests can create larger arenas; this is not a fixed size for every arena.
/// Allocation failures follow the backing allocator's failure policy.
#define arenastack_init(arena_size, ...)                                                           \
  RKI_OVERLOAD(RKI_ARENASTACK_INIT, arena_size, ##__VA_ARGS__)

/// @brief Releases every arena backing buffer and the descriptor vector, resetting the stack.
/// @note Invalidates all allocations and marks. A zero-initialized stack is supported. A subsequent
/// allocation lazily initializes the stack using alloc_ctx; the previous allocator is not retained.
rklib_fun void              arenastack_release(ArenaStack* self);

/// @brief Returns the backing allocator associated with the arena descriptor vector.
/// @note Returns alloc_ctx for a zero-initialized stack or when custom allocators are disabled.
/// This is the allocator used to obtain buffers, not an Allocator adapter allocating within this
/// stack.
/// @see arenastack_to_alloc
rklib_fun rk_pure Allocator arenastack_allocator(const ArenaStack* self);

/// @brief Reclaims all allocations for reuse without releasing backing storage.
/// @return self, for chaining.
/// @note Clears arenas through the active index and resets that index to zero. Later arenas are
/// retained and cleared when reused. Invalidates previous allocations and marks; does not clear
/// their bytes. A zero-initialized stack is supported.
rklib_fun ArenaStack*       arenastack_clear(ArenaStack* self);

/// @brief Saves the active arena's cursor for arenastack_rewind_to().
/// @return A marker for the current position, or a null-position marker for a zero-initialized
/// stack.
/// @note Does not allocate or keep allocations alive. A null-position mark denotes the stack's
/// starting position: rewinding to it clears the stack, even if it has since been lazily
/// initialized.
rklib_fun rk_pure ArenaMark arenastack_mark(const ArenaStack* self);

/// @brief Reclaims allocations made after a saved position, retaining backing buffers for reuse.
/// @param self Stack from which the mark was obtained.
/// @param mark A mark from arenastack_mark() on this stack, still valid for its current state and
/// not beyond its current allocation position. A null-position mark (from a zero-initialized
/// stack) rewinds to the start, equivalent to arenastack_clear().
/// @return self, for chaining.
/// @note Invalidates allocations and marks in the discarded region. Rewinding to an empty later
/// arena may make the preceding arena active. Foreign or invalidated non-null marks are
/// unsupported.
rklib_fun ArenaStack*       arenastack_rewind_to(ArenaStack* restrict self, ArenaMark mark);

/// @brief `void* arenastack_allocate(size_t nbytes, size_t align, ArenaStack* self)` - Allocates
/// aligned storage, reusing or adding an arena if necessary.
/// @param nbytes Payload size in bytes; may be zero. The size plus align - 1, and its required
/// power-of-two backing-buffer size, must be representable in size_t.
/// @param align Required alignment; must be a nonzero power of two.
/// @param self Initialized or zero-initialized stack to allocate from.
/// @return A non-null aligned pointer on success; failures follow the applicable allocator/arena
/// failure policy rather than returning a normal null failure result.
/// @note A zero-initialized stack is lazily initialized using alloc_ctx. A zero-size request
/// returns an aligned cursor position and consumes only alignment padding; it may initialize or
/// grow the stack. The result may be an end pointer and must not be dereferenced for a zero-size
/// request.
/// @note Existing allocation addresses remain stable when backing buffers or the descriptor vector
/// grow. Unlike generic alloc_allocate(), this direct API does not normalize zero-size requests to
/// NULL; such a zero-size position must not be passed to alloc_deallocate()/alloc_reallocate().
/// @see arenastack_new
/// @see arenastack_to_alloc
rklib_fun void*             arenastack_allocate(size_t nbytes, size_t align, ArenaStack* self);

/// @brief `T* arenastack_new(T, size_t count, ArenaStack* arena_stack)` - Allocates raw storage for
/// count elements with the type's required alignment.
/// @param T Element type.
/// @param count Element count; may be zero. The byte-size calculation and backing-size requirements
/// of arenastack_allocate() must be representable.
/// @param arena_stack Stack to allocate from.
/// @return A typed pointer to the allocated storage.
/// @note Does not invoke C++ constructors. Zero count follows arenastack_allocate() semantics.
/// @see arenastack_allocate
#define arenastack_new(T, count, arena_stack) RKI_ARENASTACK_NEW(T, count, arena_stack)

/// @brief `T* arenastack_new_aligned(T, size_t count, size_t align, ArenaStack* arena_stack)` -
/// Like arenastack_new(), with explicitly requested alignment.
/// @param T Element type.
/// @param count Element count; may be zero. Byte-size and backing-size calculations must be
/// representable.
/// @param align A nonzero power of two, at least the alignment required by T.
/// @param arena_stack Stack to allocate from.
/// @return A typed pointer to the allocated raw storage.
/// @see arenastack_new
/// @see arenastack_allocate
#define arenastack_new_aligned(T, count, align, arena_stack)                                       \
  RKI_ARENASTACK_ALIGNED_NEW(T, count, align, arena_stack)

/// @brief `void* arenastack_try_resize_top(size_t old_size, size_t new_size, ArenaStack* self)` -
/// Resizes the active arena's top allocation without moving its start, like
/// arena_try_resize_top().
/// @param old_size Current payload size of the active arena's top allocation in bytes; may be zero
/// for a valid zero-size position at the active cursor.
/// @param new_size Desired payload size in bytes; may be zero.
/// @param self Stack owning the top allocation or zero-size position. For a zero-initialized or
/// released stack, `old_size` must be zero and the request fails, including when `new_size` is
/// zero.
/// @return The unchanged start pointer on success, including when `new_size` is zero; `NULL` if
/// the requested size does not fit in the active arena.
/// @note Failure leaves the stack unchanged. Never moves data, adds an arena, or invokes failure
/// handlers. A zero new size reclaims the payload but not preceding alignment padding; the
/// returned pointer then denotes only a zero-size position and must not be dereferenced.
/// @note Only the active arena's top can be resized. An allocation that ends an earlier arena is
/// not the stack's top, even if it was the most recent allocation before the stack grew.
/// @note Operates on the implicit top allocation, so `old_size` correctly describing it is a
/// precondition, not a checked failure. To resize a specific allocation that may not be on top,
/// use arenastack_try_resize() or arenastack_try_extend().
/// @see arenastack_try_resize
rklib_fun void* arenastack_try_resize_top(size_t old_size, size_t new_size, ArenaStack* self);

/// @brief `void* arenastack_try_resize(void* ptr, size_t old_size, size_t new_size,
/// ArenaStack* self)` - Attempts to resize a specific allocation in place without moving it, like
/// arena_try_resize(); it can grow or shrink.
/// @param ptr Non-null pointer to an allocation or zero-size position owned by this stack; it need
/// not be the top allocation.
/// @param old_size Current payload size in bytes; must match the allocation.
/// @param new_size Desired payload size in bytes; may be zero.
/// @param self Stack owning the allocation.
/// @return `ptr` on success, including when `new_size` is zero. `NULL` if `ptr` is not the active
/// arena's top allocation (including the top of an earlier arena), the requested size does not
/// fit in the active arena, or the stack is zero-initialized or released.
/// @note Failure leaves the stack unchanged; a non-top allocation is an ordinary failure, not a
/// contract violation. Ownership and a correct `old_size` remain preconditions. Never moves data
/// or adds an arena. Preserves the retained bytes. A zero new size reclaims the payload but not
/// preceding alignment padding; the returned pointer then denotes only a zero-size position and
/// must not be dereferenced.
/// @see arenastack_try_extend
/// @see arenastack_try_resize_top
rklib_fun void* arenastack_try_resize(void* ptr, size_t old_size, size_t new_size,
                                      ArenaStack* self);

/// @brief `T* arenastack_try_extend(T* ptr, size_t old_count, size_t new_count,
/// ArenaStack* arena_stack)` - Typed arenastack_try_resize() using element counts; despite the
/// name, it can also shrink.
/// @param ptr Non-null pointer to an allocation or zero-size position owned by this stack; it need
/// not be the top allocation.
/// @param old_count Current payload element count; must match the allocation.
/// @param new_count Desired element count; may be zero. Byte-size calculations must be
/// representable.
/// @param arena_stack Stack owning the allocation.
/// @return `ptr` on success, cast to the same pointer type, including when `new_count` is zero.
/// `NULL` under the same conditions as arenastack_try_resize(), including a non-top `ptr`.
/// @note Inherits arenastack_try_resize() semantics. Does not construct or destroy C++ objects.
/// @see arenastack_try_resize
#define arenastack_try_extend(ptr, old_count, new_count, arena_stack)                              \
  ((typeof(ptr))arenastack_try_resize((ptr), sizeof_n(*(ptr), old_count),                          \
                                      sizeof_n(*(ptr), new_count), (arena_stack)))

/// @brief Internal allocation callback for the ArenaStack allocator adapter.
/// @note Generic alloc_* wrappers normalize zero-size and null-pointer cases before dispatch:
/// allocation callbacks receive positive sizes; reallocation/deallocation callbacks receive valid
/// non-null existing allocations with matching sizes and alignment, and reallocation receives a
/// positive new size. Allocation and reallocation callbacks must return valid storage or handle
/// failure locally. The allocation callback also serves arenastack_allocate(), whose direct
/// zero-size contract is broader.
rklib_fun alloc_allocation_f   rki_arenastack_allocate;
rklib_fun alloc_reallocation_f rki_arenastack_reallocate;
rklib_fun alloc_deallocation_f rki_arenastack_deallocate;
static const AllocatorVTable arenastack_allocator_vtable = {.alloc_f   = rki_arenastack_allocate,
                                                            .realloc_f = rki_arenastack_reallocate,
                                                            .dealloc_f = rki_arenastack_deallocate};

/// @brief Returns an Allocator handle borrowing the ArenaStack as its context.
/// @param self Initialized or zero-initialized stack; must remain at the same address while used.
/// @return An adapter allocating within the stack, not its backing allocator.
/// @note Only useful with custom allocators enabled: otherwise alloc_* wrappers accept no explicit
/// allocator and alloc_ctx cannot be replaced, so the handle cannot be passed anywhere.
/// @note Generic alloc_* wrappers return NULL for zero-size allocation, deallocate and return NULL
/// for zero-size reallocation, route null-input reallocation to allocation, and ignore null
/// deallocation. These cases are normalized before callback dispatch.
/// @note Only the active arena's top allocation can reclaim payload storage on deallocation or
/// resize. Other deallocations and shrinks reclaim no storage. Growth may allocate and copy,
/// retaining the old storage until bulk reclamation. Preceding alignment padding is not reclaimed
/// by deallocation.
/// @note Clearing, rewinding, or releasing must not discard storage still used through this handle.
/// @see arenastack_allocator
static_fun rk_const Allocator arenastack_to_alloc(ArenaStack* self);

#define RKI_AMALG_IMPL_F79CED2231B0C73E 1
RKI_HEADER_END
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
RKI_HEADER_BEGIN

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
#define str_dat(strlike)      RKI_STR_DAT(strlike)

/// @brief `size_t str_len(strlike)` - Returns the length of any Stringlike.
#define str_len(strlike)      RKI_STR_LEN(strlike)

/// @brief `bool str_is_empty(strlike)` - Returns if stringlike is empty.
#define str_is_empty(strlike) ((bool)(str_len(strlike) == 0))

/// @brief Returns a pointer to the first character of any Stringlike, for iterating together with
/// str_end().
/// @param strlike A Stringlike, by value.
/// @return `char*` for Str or char*, `const char*` for Strv or const char*. `NULL` for an
/// unallocated Str or a `(Strv){NULL, 0}`, in which case str_end() is also `NULL`.
/// @note Same as str_dat(). Unlike str_front(), valid for an empty string: `[str_begin, str_end)`
/// is then an empty range.
/// @note Invalidated by any operation that reallocates a Str.
/// @see str_end
#define str_begin(strlike)    str_dat(strlike)

/// @brief Returns a pointer one past the last character of any Stringlike, so that
/// `[str_begin(s), str_end(s))` covers exactly `str_len(s)` characters.
/// @param strlike A Stringlike, by value.
/// @return `char*` for Str or char*, `const char*` for Strv or const char*. For a non-null
/// null-terminated string this is the terminating null; it must not be dereferenced as an element.
/// `NULL` for an unallocated Str or a `(Strv){NULL, 0}`, matching str_begin().
/// @note For `char*` and `const char*` arguments the length is computed with `strlen()`, so this is
/// O(n); Str and Strv store their length.
/// @note Invalidated by any operation that reallocates a Str.
/// @see str_begin
#define str_end(strlike)      RKI_STR_QCHARPTR(strlike, rki_str_end(strv_from(strlike)))

/// @brief Returns the first character as an lvalue, mutable for Str or char*, const for Strv or
/// const char*.
/// @param strlike A Stringlike, by value.
/// @pre The string is nonempty.
/// @note The precondition is asserted in debug builds; invalid access is undefined in release.
#define str_front(strlike)    (*RKI_STR_QCHARPTR(strlike, rki_str_front_ptr(strv_from(strlike))))

/// @brief Returns the last character as an lvalue, mutable for Str or char*, const for Strv or
/// const char*.
/// @param strlike A Stringlike, by value.
/// @pre The string is nonempty.
/// @note The precondition is asserted in debug builds; invalid access is undefined in release.
#define str_back(strlike)     (*RKI_STR_QCHARPTR(strlike, rki_str_back_ptr(strv_from(strlike))))

/// @brief Returns the character at zero-based index `idx` as an lvalue, mutable for Str or char*,
/// const for Strv or const char*.
/// @param strlike A Stringlike, by value.
/// @param idx Zero-based byte index; the terminating null is not an element.
/// @pre `idx < str_len(strlike)`.
/// @note The precondition is asserted in debug builds; invalid access is undefined in release.
/// @see str_peek_at
#define str_at(strlike, idx)  (*RKI_STR_QCHARPTR(strlike, rki_str_at_ptr(strv_from(strlike), idx)))

/// @brief Returns a pointer to the first character, or `NULL` if the string is empty.
/// @param strlike A Stringlike, by value.
/// @return `char*` for Str or char*, `const char*` for Strv or const char*.
#define str_peek_front(strlike) RKI_STR_QCHARPTR(strlike, rki_str_peek_front(strv_from(strlike)))

/// @brief Returns a pointer to the last character, or `NULL` if the string is empty.
/// @param strlike A Stringlike, by value.
/// @return `char*` for Str or char*, `const char*` for Strv or const char*.
#define str_peek_back(strlike)  RKI_STR_QCHARPTR(strlike, rki_str_peek_back(strv_from(strlike)))

/// @brief Returns a pointer to the character at zero-based index `idx`, or `NULL` if out of bounds.
/// @param strlike A Stringlike, by value.
/// @param idx Zero-based byte index; the terminating null is not an element.
/// @return `char*` for Str or char*, `const char*` for Strv or const char*.
/// @note Bounds are checked in both debug and release builds.
#define str_peek_at(strlike, idx)                                                                  \
  RKI_STR_QCHARPTR(strlike, rki_str_peek_at(strv_from(strlike), idx))

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @name String Lifetime/Ownership
////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief `Str str_init(size_t init_cap, Allocator alloc = alloc_ctx)` - Constructs a Str with a
/// given initial capacity.
/// @param init_cap size_t The initial capacity of the string (in elements)
/// @param alloc Allocator Optional parameter - The allocator; defaults to `alloc_ctx`
/// @return A `Str` object with the given capacity
#define str_init(init_cap, ...)       RKI_OVERLOAD(RKI_STR_INIT, init_cap, ##__VA_ARGS__)

/// @brief `Str str_from(Strlike strlike, Allocator alloc = alloc_ctx)` - Constructs a Str from a
/// Stringlike object, copying the data.
/// @param strlike     A Stringlike, by value
/// @param alloc Allocator Optional parameter - The allocator backing the new Str. If not provided,
/// this defaults to the Allocator of the cloned Strlike if it is a Str object, or alloc_ctx
/// otherwise.
/// @return A `Str` object with the copied string data
#define str_from(strlike, ...)        RKI_OVERLOAD(RKI_STR_FROM, strlike, ##__VA_ARGS__)

/// @brief `Str str_from_literal(STRING_LITERAL, Allocator alloc = alloc_ctx)`
/// - Construct a Str from a string literal.
/// @param strlit  A string literal
/// @param alloc Allocator Optional parameter - The allocator; defaults to `alloc_ctx` if not
/// provided
/// @return A `Str` object initialised with the literal's contents
#define str_from_literal(strlit, ...) RKI_OVERLOAD(RKI_STR_FROMLIT, strlit, ##__VA_ARGS__)

/// @brief Frees the underlying memory of `self`.
rklib_fun void str_release(Str* restrict self);

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
  RKI_OVERLOAD(RKI_STR_JOIN_STRV_N, svs, count, sep, ##__VA_ARGS__)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @name String Accessors
////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Returns whether the String is null-terminated.
rklib_fun rk_pure bool        str_is_null_terminated(const Str* self);

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
rklib_fun rk_pure const char* str_cstr(const Str* self);

/// @brief Returns the current capacity of `self`, in bytes.
rklib_fun rk_pure size_t      str_cap(const Str* self);

/// @brief Returns the Allocator `self` was constructed with, or `alloc_ctx` if `self` was never
/// initialized, or custom allocators are disabled.
rklib_fun rk_pure Allocator   str_allocator(const Str* self);

/// @brief Clears the contents of `self`, setting its length to zero and null-terminating it, if it
/// owns an allocation.
rklib_fun Str*                str_clear(Str* restrict self);

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @name String Capacity
////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Ensures at least `new_cap` bytes of capacity are allocated for `self`, reallocating, if
/// necessary.
rklib_fun Str*                str_reserve(Str* restrict self, size_t new_cap);

/// @brief Resizes the length of `self` to `new_len`, reallocating the memory if necessary and
/// null-terminating it.
rklib_fun Str*                str_resize(Str* restrict self, size_t new_len);

/// @brief Resizes a Str's capacity to the next power of two larger than its length,
/// null-terminating it (matching `vec_shrink_to_fit()`'s convention). Leaves some slack to reduce
/// reallocation on subsequent growth.
/// @note Use `str_shrink_to_fit_exact()` for an exact-capacity shrink.
rklib_fun Str*                str_shrink_to_fit(Str* restrict self);

/// @brief Resizes a Str's capacity to exactly its length + 1 (matching
/// `vec_shrink_to_fit_exact()`'s convention).
rklib_fun Str*                str_shrink_to_fit_exact(Str* restrict self);

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
rklib_fun Str* str_null_terminate(Str* restrict self);

/// @brief Like `str_null_terminate()` without checking for capacity.
rklib_fun Str* str_null_terminate_unchecked(Str* restrict self);

/// @brief Terminates a `Str` with `suffix` if it does not already end with it and then
/// null-terminates it if it isn't.
#define str_terminate(self, suffix) RKI_STR_TERMINATE(self, suffix)

/// @brief Adds a character to a Str object, resizing if necessary.
/// @note Appends int as a character of its value, doing `str_push(s, 1)` is likely a bug.
rklib_fun Str* str_push(Str* restrict self, char c);

/// @brief Like `str_push()` without checking for capacity.
rklib_fun Str* str_push_unchecked(Str* restrict self, char c);

/// @brief Like `str_push()` without null-terminating the Str.
rklib_fun Str* str_push_raw(Str* restrict self, char c);

/// @brief Like `str_push_unchecked()` but without null-terminating the Str.
rklib_fun Str* str_push_unchecked_raw(Str* restrict self, char c);

/// @brief Generic Convenience Macro for appending Strlikes or characters to a string, automatically
/// converting `strlike` to Strv and null-terminating `self`. May reallocate `self`.
/// @param self    The String to append to
/// @param strlike The Stringlike to append
/// @return The updated String.
/// @note Appends int as a character of its value, `str_cat(s, 1)` is likely wrong.
/// @attention Appending a substring of `self` (i.e. overlapping memory) results in undefined
/// behavior. Use `str_cat_mayalias()` in that case.
#define str_cat(self, strlike)          RKI_STR_CAT(self, strlike)

/// @brief Like `str_cat()` but checks for overlap between `self` and `strlike`.
#define str_cat_mayalias(self, strlike) RKI_STR_CAT_MAYALIAS(self, strlike)

/// @brief Like `str_cat()`, specialised on string literals. Avoids a runtime `strlen()` call via
/// compile-time `sizeof()` and ensures a compile-time error if `strlit` is not a string literal.
#define str_cat_literal(self, strlit)   str_cat_strv(self, (Strv)strv_from_literal(strlit))

/// @brief Appends formatted output (printf-style) to the string. The format string and arguments
/// follow the rules of the `printf` family.
/// @param self The string to append to
/// @param fmt  A printf-style format string
/// @param ...  Format arguments
/// @return The updated string on success, or `NULL` on formatting error
rklib_fun Str* str_cat_fmt(Str* self, const char* fmt, ...);

/// @brief Inserts a Stringlike into a Str, resizing the str if necessary.
/// @return `self`, for chaining
/// @attention Appending a substring of `self` to itself via this is undefined
#define str_insert_at(self, idx, strlike)          RKI_STR_INSERT_AT(self, idx, strlike)

/// @brief Like `str_insert_at()` but checks for pointer aliasing between self and strlike.
#define str_insert_at_mayalias(self, idx, strlike) RKI_STR_INSERT_AT_MAYALIAS(self, idx, strlike)

/// @brief Pops the last character off the Str, decreasing its length and null-terminating it.
/// @return The popped character.
/// @pre The Str is nonempty; use `str_try_pop()` to check safely.
/// @note The precondition is asserted in debug builds; invalid access is undefined in release.
rklib_fun char str_pop(Str* restrict self);

/// @brief Removes the last character and writes it to `*out`, if the Str is nonempty, keeping the
/// Str null-terminated.
/// @param out Destination for the removed character. Left untouched if the Str is empty or
/// unallocated.
/// @return `true` if a character was removed, `false` if the Str was empty or unallocated.
/// @note Unlike a sentinel return value, this distinguishes popping an embedded `'\0'` from an
/// empty Str.
rklib_fun bool str_try_pop(Str* restrict self, char* restrict out);

/// @brief Pops the last n characters off the Str, decreasing its length and null-terminating it.
/// @pre `n <= str_len(*self)`. Popping zero characters is a no-op, even for an unallocated Str.
/// @note The precondition is asserted in debug builds; invalid access is undefined in release.
rklib_fun void str_pop_n(Str* restrict self, size_t n);

/// @brief Removes a single character at the given index from the string.
/// @param self The string to modify
/// @param idx  The position of the character to remove. Must be < str->len
/// @attention Passing an out-of-bounds index results in undefined behavior.
rklib_fun Str* str_erase_at(Str* restrict self, size_t idx);

/// @brief Removes `count` characters starting at `idx` from the string.
/// @param self  The string to modify
/// @param idx   The starting position of characters to erase
/// @param count The number of characters to remove
/// @attention Supplying an out-of-range index or count is undefined
rklib_fun Str* str_erase_at_n(Str* restrict self, size_t idx, size_t count);

/// @brief Visits every character in index order.
/// @param self Pointer to the Str. Evaluated once.
/// @param it   Iterator name (a pointer to a character; access via `*it`).
/// @note break stops traversal; continue advances to the next character.
///
/// Usage:
/// ```c
/// str_foreach(&str, c) { putchar(*c); }
/// ```
#define str_foreach(self, it)          RKI_STR_FOREACH(self, it)

/// @brief Like `str_foreach()`, but iterates in reverse index order. Same parameters and contract.
#define str_foreach_reversed(self, it) RKI_STR_FOREACH_REVERSED(self, it)

/// @brief Erases every character satisfying `pred`.
/// @param self Pointer to a mutable Str. Evaluated once.
/// @param it   Iterator name (access via `*it`).
/// @param pred Predicate expression, evaluated once per original character.
/// @note The predicate must not structurally modify the Str.
///
/// Usage:
/// ```c
/// Str str = str_from_literal("hello");
/// str_erase_if(&str, c, (*c == 'o'));
/// printf("%s\n", str.str); // prints "hell"
/// ```
#define str_erase_if(self, it, pred)   RKI_STR_ERASE_IF(self, it, pred)

/// @brief Replaces all instances of `oldc` in `self` with `newc`.
rklib_fun Str* str_replace(Str* restrict self, char oldc, char newc);

/// @brief Replaces the character at position `pos` in `self` with `c`. Does not modify the `len`
/// field, even if `c == '\0'`. This allows temporarily inserting a null terminator within the
/// string to treat a substring as a C-string, and later restoring the original character without
/// needing to modify the `len` field.
/// @return The previously stored character at position `pos`
/// @note Behaviour is undefined if `pos >= self->len`.
rklib_fun char str_replace_at(Str* restrict self, size_t pos, char c);

/// @brief Converts all lowercase letters in `self` to uppercase.
rklib_fun Str* str_to_upper(Str* restrict self);

/// @brief Converts all uppercase letters in `self` to lowercase.
rklib_fun Str* str_to_lower(Str* restrict self);

/// @brief Reverses `self` in place.
rklib_fun Str* str_reverse(Str* restrict self);

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @name Strv Construction and View
////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Constructs a Strv from a string literal.
#define strv_from_literal(strlit) {.str = strlit, .len = lenof(strlit)}

rklib_fun rk_const Strv strv_from_cstrn(const char* str, size_t len);

/// @brief Constructs a Strv from a Stringlike.
#define strv_from(strlike)              RKI_STRV_FROM(strlike)

/// @brief Returns a view over [start, min(end, len)). If start > len or end < start, returns an
/// empty view.
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
#define str_split(strvptr, delims) RKI_STR_SPLIT(strvptr, delims)

/// @brief Tokenises the Stringlike without modifying the underlying data, allocating an array of
/// Strv Objects.
/// @param strlike   The Stringlike to tokenise (by value)
/// @param delims    The delimiters of the tokenisation, a Stringlike by value
/// @param out_count size_t* an out-parameter in which the count of tokens will be stored.
/// @param alloc Optional; The Allocator that allocates the array of Strv objects; defaults to
/// `alloc_ctx` (or malloc_allocator, if `RK_CUSTOM_ALLOCATORS` == `0`)
/// @return A Strv* to the array of tokens.
#define str_split_alloc(strlike, delims, out_count, ...)                                           \
  RKI_OVERLOAD(RKI_STR_SPLIT_ALLOC, strv_from(strlike), strv_from(delims), out_count, ##__VA_ARGS__)

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
#define str_find(strlike, subs)         RKI_STR_QCHARPTR(strlike, RKI_STR_FIND(strlike, subs))

/// @brief Finds the last occurrence of `subs` in `strlike`.
/// @param strlike  A Stringlike, the haystack
/// @param subs     A Stringlike, the needle
/// @return A `(const) char*` to the last occurrence or a nullpointer if the substring is not found.
#define str_findr(strlike, subs)        RKI_STR_QCHARPTR(strlike, RKI_STR_FINDR(strlike, subs))

/// @brief Checks whether a Stringlike contains another Stringlike.
/// @param strlike  A Stringlike, the haystack
/// @param subs     A Stringlike, the needle
/// @return `true`, if strlike contains subs, `false` otherwise
#define str_contains(strlike, subs)     RKI_STR_CONTAINS(strlike, subs)

/// @brief Tests if a `pre` is a prefix of `strlike`.
/// @param strlike A Stringlike, by value
/// @param pre    The potential prefix of strlike
/// @return `true` if pre is prefix of strlike, `false` otherwise
#define str_starts_with(strlike, pre)   RKI_STR_STARTS_WITH(strlike, pre)

/// @brief Tests if a `suf` is a suffix of `strlike`.
/// @param strlike A Stringlike, by value
/// @param suf  The potential suffix of strlike
/// @return `true` if suf is suffix of strlike, `false` otherwise
#define str_ends_with(strlike, suf)     RKI_STR_ENDS_WITH(strlike, suf)

#define RKI_AMALG_IMPL_A44D854F7D622D4E 1
RKI_HEADER_END
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
RKI_HEADER_BEGIN

/// @brief Defines a pool type and associated functions for type `T` Depending on the arguments,
/// this macro defines either a dynamic pool (`Pool(T)`) or a static/fixed pool (`Pool(T, C)`).
/// @param T Type of elements stored in the pool
/// @param C Capacity of the pool if static
#define POOL_DEFINE(T, ...)           RKI_STATOVERLOAD(RKI_POOL_DEFINE, T, ##__VA_ARGS__)

/// @brief Alias for the pool type (dynamic or static).
/// @param T Type of elements stored in the pool
/// @param C Capacity of the pool if static
/// @note Static Pools take a second capacity parameter
#define Pool(T, ...)                  RKI_STATOVERLOAD__(RKI_POOL, T, ##__VA_ARGS__)
#define StaticPool(T, CAP)            Pool_##CAP##_##T
#define DynPool(T)                    Pool_##T

/// @brief `Pool(T) pool_init(T, size_t cap, Allocator alloc = alloc_ctx)` - Initializes a dynamic
/// pool with given capacity.
/// @param T Element type
/// @param cap Desired capacity
/// @param alloc Optional allocator
/// @return Initialized pool struct
#define pool_init(T, _cap, ...)       RKI_OVERLOAD(RKI_DPOOL_INIT, T, _cap, ##__VA_ARGS__)

/// @brief Compile-time zero-initializer for a `Pool(T, C)` (`StaticPool`). Suitable for global and
/// static variables. No memory is allocated.
/// @note Named after `StaticPool`, the type it initializes — not to be confused with the (removed)
/// `_init_static` convention other containers used for static-storage-duration-safe initializers.
#define staticpool_init               {RKI_ZINIT}

/// @brief `void pool_release(Pool(T, ...)* self)` - Releases the associated resources of the pool
/// (if the pool is dynamic) and resets its members. For static pools, this resets the allocation
/// bitset but does not modify the underlying element storage.
#define pool_release(self)            ((void)RKI_POOL_RELEASE(self))

/// @brief `size_t pool_cap(Pool(T, ...)* self)` - Returns the total capacity of the pool.
#define pool_cap(self)                ((size_t)RKI_POOL_CAP(self))

/// @brief `Allocator pool_allocator(Pool(T)* self)` - Returns the Allocator the (dynamic) pool was
/// constructed with, or `alloc_ctx` if the pool was never initialized or custom allocators are
/// disabled.
#define pool_allocator(self)          RKI_allocatorof(self)

/// @brief `size_t pool_used(Pool(T)* self)` - Returns the number of active (allocated) elements in
/// the pool.
#define pool_used(self)               ((size_t)RKI_POOL_USED(self))
#define pool_count(self)              pool_used(self) /// @brief Alias for `pool_used`

/// @brief `size_t pool_remaining(Pool(T)* self)` - Returns the number of free slots remaining in
/// the pool.
#define pool_remaining(self)          ((size_t)RKI_POOL_REMAINING(self))

/// @brief Returns `true` iff the pool is empty.
#define pool_is_empty(self)           ((bool)(pool_used(self) == 0))

/// @brief Returns `true` iff the pool is full.
#define pool_is_full(self)            ((bool)(pool_remaining(self) == 0))

/// @brief `Pool(T)* pool_clear(Pool(T)* self)` - Marks all elements in the pool as reusable.
/// @return `self`, for chaining
#define pool_clear(self)              ((typeof(self))RKI_POOL_CLEAR(self))

/// @brief `T* pool_new(Pool(T)* self)` - Allocates a new element in the pool.
/// @return Pointer to the newly allocated element
#define pool_new(self)                ((RKI_POOL_T(self)*)RKI_POOL_NEW(self))

/// @brief `T* pool_try_new(Pool(T)* self)` - Like `pool_new()`, but returns `NULL` if full instead
/// of running `RK_POOL_FAIL()`.
#define pool_try_new(self)            ((RKI_POOL_T(self)*)RKI_POOL_TRY_NEW(self))

/// @brief `T* pool_put(Pool(T)* self, T el)` - Allocates a new element and stores a copy of the
/// value.
/// @return Pointer to the inserted element
#define pool_put(self, el)            ((RKI_POOL_T(self)*)RKI_POOL_PUT(self, el))

/// @brief `T* pool_try_put(Pool(T)* self, T el)` - Like `pool_put()`, but returns `NULL` if full
/// instead of running `RK_POOL_FAIL()`.
#define pool_try_put(self, el)        ((RKI_POOL_T(self)*)RKI_POOL_TRY_PUT(self, el))

/// @brief `void pool_delete(Pool(T)* self, T* ptr)` - Frees an element in the pool.
#define pool_delete(self, ptr)        ((void)RKI_POOL_DELETE(self, ptr))

/// @brief Visits every allocated element in the pool.
/// @param self Pointer to the Pool. Evaluated once.
/// @param it   Iterator name (a pointer to an element; access via `*it`).
/// @note Elements are const when accessed through a pointer to a const Pool.
/// @note break stops traversal; continue advances to the next allocated element.
/// @note There is deliberately no `pool_foreach_reversed`: unlike Vec/Deque/Str (positional
/// sequences) or the trees (sorted by key), a Pool's iteration order is just the ascending bitset
/// index of whichever slots the first-fit allocator happened to occupy -- an implementation
/// artifact, not a property of the data a caller can rely on (the same reason Dict/Set's
/// "unspecified slot order" never got one either).
///
/// Usage:
/// ```c
/// pool_foreach(&my_pool, elem) {
///     printf("%d\n", *elem);
/// }
/// ```
#define pool_foreach(self, it)        RKI_POOL_FOREACH(self, it)

/// @brief Erases every allocated element satisfying `pred`.
/// @param self Pointer to a mutable Pool. Evaluated once.
/// @param it   Iterator name (access via `*it`).
/// @param pred Predicate expression, evaluated once per original allocated element.
/// @note The predicate must not structurally modify the Pool.
///
/// Usage:
/// ```c
/// pool_erase_if(&my_pool, it, *it % 2 == 0); // delete even values
/// ```
#define pool_erase_if(self, it, pred) RKI_POOL_ERASE_IF(self, it, pred)

#define RKI_AMALG_IMPL_66FA845F969DDC89 1
RKI_HEADER_END
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
/* inlined from include/rk_dict.h:126: #include "rk_alloc.h" */
/* skipped already-included: "include/rk_alloc.h" */
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
#define dict_get(K, V, self, key)                                                                  \
  _Generic((self),                                                                                 \
      const Dict(K, V)*: RKI_DICT_PUB(K, V, get_const),                                            \
      Dict(K, V)*: RKI_DICT_PUB(K, V, get))((self), (key))

/// @brief Returns the value for `key` as an lvalue, mutable for a mutable Dict and const for a
/// const Dict.
/// @pre `key` is present; use `dict_get()` to check safely.
/// @note The precondition is asserted in debug builds; invalid access is undefined in release.
/// @note Never inserts, unlike `dict_get_or_add()`. Invalidated by any insertion that rehashes,
/// and by removal of `key`.
/// @see dict_get
#define dict_at(K, V, self, key)                                                                   \
  (*_Generic((self),                                                                               \
       const Dict(K, V)*: RKI_DICT_PUB(K, V, at_const),                                            \
       Dict(K, V)*: RKI_DICT_PUB(K, V, at))((self), (key)))

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
#define RKI_AMALG_IMPL_1E829680B591C33B 1
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

/* Deferred implementation sections (dependency order). */
#ifdef RKI_AMALG_IMPL_74750096303C1B2D
#undef RKI_AMALG_IMPL_74750096303C1B2D
/* BEGIN DEFERRED IMPLEMENTATION: include/rk_defs.h */
RKI_HEADER_BEGIN
#pragma region implementation
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// Implementation Details //////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

#define RKI_CONC(a, b)         a##b

#define rk_EXP(x)              x
#define rk_CONC(a, b)          RKI_CONC(a, b)
#define rk_UNIQUE_NAME(prefix) rk_CONC(prefix, __LINE__)

#define RKI_ARGCOUNT(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16,    \
                     _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31,    \
                     _32, N, ...)                                                                  \
  N
#if __STDC_VERSION__ >= 202000L || (defined(__cplusplus) && __cplusplus >= 202002L)
# define rk_ARGCOUNT(...)                                                                          \
   RKI_ARGCOUNT(dummy __VA_OPT__(, ) __VA_ARGS__, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21,  \
                20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#else
# define rk_ARGCOUNT(...)                                                                          \
   RKI_ARGCOUNT(dummy, ##__VA_ARGS__, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18,  \
                17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#endif

#ifndef __cplusplus
# define RKI_ZINIT 0
#else
# define RKI_ZINIT
#endif

#define RKI_GENCASE(T, N, fun_name) , T : fun_name##N
#define RKI_contrav(T, x)           _Generic((x), T: x, default: (T){RKI_ZINIT})
#define RKI_contrav_p(T, x)         _Generic((x), T: x, default: (T)1)

#define rk_dummyofp(v)              ((typeof(v)*)0)

#ifndef __cplusplus
# define typeof_decayed(v) typeof((void)0, (v))
# define rk_to_rvalue(obj) ((void)0, (obj))
#else
# define typeof_decayed(v) typename std::decay<typeof(v)>::type
# define rk_to_rvalue(obj) ((typeof(obj))(obj))
#endif

#define rk_is_const(v)    _Generic(rk_dummyofp(v), const typeof(v)*: 1, default: 0)
#define rk_is_volatile(v) _Generic(rk_dummyofp(v), volatile typeof(v)*: 1, default: 0)
#if defined(_MSC_VER) && !defined(__clang__)
# define rk_is_atomic(v) ((void)rk_dummyofp(v), 0)
#else
# define rk_is_atomic(v) _Generic(rk_dummyofp(v), _Atomic typeof(v)*: 1, default: 0)
#endif

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
  static_assert_expr(alignof(T) <= RKI_MALLOC_ALIGN, "Type alignment too "                         \
                                                     "large.")

/// ensures the backing array is legitimate storage, evaluates to 0
#define rk_ensure_valid_storage_type(arr)                                                          \
  static_assert_expr(rk_is_same_type(&(arr), unsigned char (*)[sizeof(arr)]),                      \
                     "Backing storage must be an unsigned char array")

/// Function overloading by argument count
#define RKI_OVERLOAD(m, ...)   rk_CONC(m, rk_ARGCOUNT(__VA_ARGS__))(__VA_ARGS__)
#define RKI_OVERLOAD_(m, ...)  rk_CONC(m, rk_ARGCOUNT(__VA_ARGS__))(__VA_ARGS__)
#define RKI_OVERLOAD__(m, ...) rk_CONC(m, rk_ARGCOUNT(__VA_ARGS__))(__VA_ARGS__)

rklib_fun rk_const bool rk_ptrs_overlap(const void* beg1, const void* end1, const void* beg2,
                                        const void* end2) {
  rk_assert(((uptr)beg1 <= (uptr)end1 && (uptr)beg2 <= (uptr)end2) && "Invalid Memory Region");
  return (uptr)beg1 < (uptr)end2 && (uptr)end1 > (uptr)beg2;
}
rklib_fun rk_const bool rk_ptr_in_range(const void* ptr, const void* beg, const void* end) {
  rk_assert((uptr)beg <= (uptr)end && "Invalid Memory Region");
  return (uptr)ptr >= (uptr)beg && (uptr)ptr < (uptr)end;
}

#if defined(_MSC_VER)
# if defined(_WIN64)
#  define RKI_MALLOC_ALIGN 16u
# endif
#elif defined(__GLIBC__)
# if defined(__LP64__) || defined(_LP64)
#  define RKI_MALLOC_ALIGN 16u
# endif
#elif defined(__APPLE__) && defined(__MACH__)
# if defined(__LP64__) || defined(_LP64)
#  define RKI_MALLOC_ALIGN 16u
# endif
#endif
#ifndef RKI_MALLOC_ALIGN
# define RKI_MALLOC_ALIGN align_max
#endif

#if defined(_MSC_VER) || defined(__TINYC__)
typedef struct {
  long double ld;
  long long   ll;
  void*       vp;
} rki_max_align_t;
#else
typedef max_align_t rki_max_align_t;
#endif

/// bug prior to 17.44 that treated char == (un)signed char for _Generic
#if defined(_MSC_VER) && _MSC_VER < 1944
# define RKI_IFNMSVC_CHARBUG(...)
#else
# define RKI_IFNMSVC_CHARBUG(...) __VA_ARGS__
#endif

enum {                  // NOLINT
  RKI_NUMCLASS_b = 0xF, ///< Boolean types  (0b1111)
  RKI_NUMCLASS_o = 0x0, ///< Other types    (0b0000)
  RKI_NUMCLASS_u = 0x1, ///< Unsigned types (0b0001)
  RKI_NUMCLASS_s = 0x2, ///< Signed types   (0b0010)
  RKI_NUMCLASS_f = 0x4, ///< Float types    (0b0100)
  RKI_NUMCLASS_c = 0x8, ///< Char type      (0b1000)
};

#define RKI_numclassof_(T, N, class)                                                               \
T:                                                                                                 \
  class,

#define RK_numclassof(x)                                                                           \
  _Generic(rk_ensure_type_is_num(typeof(x)),                                                       \
      RKI_F_TYPES(RKI_numclassof_, RKI_NUMCLASS_f) bool: RKI_NUMCLASS_b,                           \
      RKI_IFNMSVC_CHARBUG(char : RKI_NUMCLASS_c, ) default: (1 + !(((typeof(x))-1) > 0)))

#define RKI_TOSIGNED(x)                                                                            \
  _Generic((x),                                                                                    \
      unsigned char: (signed char)(x),                                                             \
      unsigned short: (short)(x),                                                                  \
      unsigned: (int)(x),                                                                          \
      unsigned long: (long)(x),                                                                    \
      unsigned long long: (long long)(x)RKI_IFHAS_INT128(, u128 : (s128)(x)))

#define RKI_U_TYPES(X, ...)                                                                        \
  X(unsigned char, uc, ##__VA_ARGS__)                                                              \
  X(unsigned short, us, ##__VA_ARGS__)                                                             \
  X(unsigned, ui, ##__VA_ARGS__)                                                                   \
  X(unsigned long, ul, ##__VA_ARGS__)                                                              \
  X(unsigned long long, ull, ##__VA_ARGS__)                                                        \
  RKI_IFHAS_INT128(X(u128, ullx, ##__VA_ARGS__))

#define RKI_S_TYPES(X, ...)                                                                        \
  X(signed char, sc, ##__VA_ARGS__)                                                                \
  X(short, ss, ##__VA_ARGS__)                                                                      \
  X(int, si, ##__VA_ARGS__)                                                                        \
  X(long, sl, ##__VA_ARGS__)                                                                       \
  X(long long, sll, ##__VA_ARGS__)                                                                 \
  RKI_IFHAS_INT128(X(s128, sllx, ##__VA_ARGS__))

#define RKI_SU_TYPES(X, ...)                                                                       \
  RKI_U_TYPES(X, ##__VA_ARGS__)                                                                    \
  RKI_S_TYPES(X, ##__VA_ARGS__)

#define RKI_INT_TYPES(X, ...)                                                                      \
  RKI_IFNMSVC_CHARBUG(X(char, c, ##__VA_ARGS__))                                                   \
  X(bool, b, ##__VA_ARGS__)                                                                        \
  RKI_SU_TYPES(X, ##__VA_ARGS__)

#define RKI_F_TYPES(X, ...)                                                                        \
  X(float, f, ##__VA_ARGS__)                                                                       \
  X(double, d, ##__VA_ARGS__)                                                                      \
  X(long double, ld, ##__VA_ARGS__)

#define RKI_NUM_TYPES(X, ...)                                                                      \
  RKI_INT_TYPES(X, ##__VA_ARGS__)                                                                  \
  RKI_F_TYPES(X, ##__VA_ARGS__)

#define RKI_WIDER_T(x, y)                                                                          \
  rk_static_if(rk_ensure_numclass_compatible(x, y) + sizeof(typeof(x)) >= sizeof(typeof(y)),       \
               (typeof(x))0, (typeof(y))0)

#define RKI_WIDER_T3(a1, a2, a3)                                                                   \
  rk_static_if(rk_ensure_numclass_compatible(RKI_WIDER_T(a1, a2), a3)                              \
                       + sizeof(RKI_WIDER_T(a1, a2))                                               \
                   >= sizeof(typeof(a3)),                                                          \
               RKI_WIDER_T(a1, a2), (typeof(a3))0)

#define RKI_TWONUMS(pref, classes, x, y)                                                           \
  _Generic(RKI_WIDER_T(x, y) classes(RKI_GENCASE, pref))(x, y)

#define RKI_THREENUMS(pref, classes, x, y, z)                                                      \
  _Generic(RKI_WIDER_T3(x, y, z) classes(RKI_GENCASE, pref))(x, y, z)

RKI_IFHAS_INT128(rklib_fun rk_const rk_forceinline s128 abs_llx(s128 x) {
  return x < 0 ? -x : x; // UB if v == I128_MIN
})

#define RKI_ABS(x)                                                                                 \
  _Generic(rk_ensure_type_is_num(typeof(x)),                                                       \
      signed char: (signed char)abs((signed char)(x)),                                             \
      short: (short)abs((short)(x)),                                                               \
      int: abs((int)(x)),                                                                          \
      long: labs((long)(x)),                                                                       \
      long long: llabs((long long)(x)),                                                            \
      float: fabsf((float)(x)),                                                                    \
      double: fabs((double)(x)),                                                                   \
      long double: fabsl((long double)(x)),                                                        \
      RKI_IFHAS_INT128(s128 : abs_llx(x), ) default: (x))

#define RKI_MINOF(T)                                                                               \
  ((T) _Generic(rk_ensure_type_is_num(T),                                                          \
       signed char: SCHAR_MIN,                                                                     \
       short: SHRT_MIN,                                                                            \
       int: INT_MIN,                                                                               \
       long: LONG_MIN,                                                                             \
       long long: LLONG_MIN,                                                                       \
       RKI_IFNMSVC_CHARBUG(char : CHAR_MIN, )                                                      \
           RKI_IFHAS_INT128(s128 : -((s128)(((u128) - 1) >> 1)) - 1, ) default: 0))

#define RKI_MAXOF(T)                                                                               \
  ((T) _Generic(rk_ensure_type_is_num(T),                                                          \
       signed char: SCHAR_MAX,                                                                     \
       short: SHRT_MAX,                                                                            \
       int: INT_MAX,                                                                               \
       long: LONG_MAX,                                                                             \
       long long: LLONG_MAX,                                                                       \
       RKI_IFNMSVC_CHARBUG(char : CHAR_MAX, )                                                      \
           RKI_IFHAS_INT128(s128 : ((u128)(~(u128)0)) >> 1, ) default: ((T)(~(T)0))))

#define RKI_CHELPER         rklib_fun rk_const rk_forceinline
#define RKI_UNSEQUENCED_NOW rk_unsequenced
#define RKI_DEFINE_STUFF(T, N)                                                                     \
  RKI_CHELPER T min_##N(T x, T y) RKI_UNSEQUENCED_NOW { return rk_MIN(x, y); }                     \
  RKI_CHELPER T max_##N(T x, T y) RKI_UNSEQUENCED_NOW { return rk_MAX(x, y); }                     \
  RKI_CHELPER T clamp_##N(T arg, T low, T high) RKI_UNSEQUENCED_NOW {                              \
    return rk_CLAMP(arg, low, high);                                                               \
  }

RKI_INT_TYPES(RKI_DEFINE_STUFF)
#undef RKI_UNSEQUENCED_NOW
#define RKI_UNSEQUENCED_NOW
#undef RKI_CHELPER
#define RKI_CHELPER rklib_fun rk_forceinline
RKI_F_TYPES(RKI_DEFINE_STUFF)
#undef RKI_DEFINE_STUFF
#undef RKI_CHELPER
#define RKI_CHELPER rklib_fun rk_const rk_forceinline

#define RKI_DEF_SAT_U(T, N)                                                                        \
  RKI_CHELPER T rk_sat_add_##N(T glob_a, T b) rk_unsequenced {                                     \
    T sum = (T)(glob_a + b);                                                                       \
    return sum >= glob_a ? sum : (T) - 1;                                                          \
  }                                                                                                \
  RKI_CHELPER T rk_sat_sub_##N(T glob_a, T b) rk_unsequenced {                                     \
    return (T)(glob_a < b ? 0 : glob_a - b);                                                       \
  }                                                                                                \
  RKI_CHELPER T rk_sat_mul_##N(T glob_a, T b) rk_unsequenced {                                     \
    return (b != 0 && glob_a > (T)(maxof(T) / b)) ? maxof(T) : (T)(glob_a * b);                    \
  }
RKI_U_TYPES(RKI_DEF_SAT_U)
#undef RKI_DEF_SAT_U

#if rk_has_builtin(__builtin_add_overflow)
# define RKI_DEF_SA_S_(T)                                                                          \
   T s;                                                                                            \
   if (__builtin_add_overflow(glob_a, b, &s)) { return (b < 0) ? minof(T) : maxof(T); }            \
   return s;
#else
# define RKI_DEF_SA_S_(T)                                                                          \
   T min = minof(T), max = maxof(T);                                                               \
   if (b > 0 && glob_a > max - b) { return max; }                                                  \
   if (b < 0 && glob_a < min - b) { return min; }                                                  \
   return (T)(glob_a + b);
#endif

#if rk_has_builtin(__builtin_sub_overflow)
# define RKI_DEF_SS_S_(T)                                                                          \
   T s;                                                                                            \
   if (__builtin_sub_overflow(glob_a, b, &s)) { return (b < 0) ? maxof(T) : minof(T); }            \
   return s;
#else
# define RKI_DEF_SS_S_(T)                                                                          \
   const T min = minof(T), max = maxof(T);                                                         \
   if (b > 0 && glob_a < min + b) { return min; }                                                  \
   if (b < 0 && glob_a > max + b) { return max; }                                                  \
   return (T)(glob_a - b);
#endif
#if rk_has_builtin(__builtin_mul_overflow)
# define RKI_DEF_SM_S_(T)                                                                          \
   if (glob_a == 0 || b == 0) return (T)0;                                                         \
   T glob_point;                                                                                   \
   if (__builtin_mul_overflow(glob_a, b, &glob_point)) {                                           \
     return ((glob_a < 0) ^ (b < 0)) ? minof(T) : maxof(T);                                        \
   }                                                                                               \
   return glob_point;
#else
# define RKI_DEF_SM_S_(T)                                                                          \
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

#define RKI_DEF_SAT_S(T, N)                                                                        \
  RKI_CHELPER T                                          rk_sat_add_##N(T glob_a, T b)             \
      rk_unsequenced{RKI_DEF_SA_S_(T)} RKI_CHELPER T     rk_sat_sub_##N(T glob_a, T b)             \
          rk_unsequenced{RKI_DEF_SS_S_(T)} RKI_CHELPER T rk_sat_mul_##N(T glob_a, T b)             \
              rk_unsequenced {                                                                     \
    RKI_DEF_SM_S_(T)                                                                               \
  }

RKI_S_TYPES(RKI_DEF_SAT_S)
#undef RKI_CHELPER
#undef RKI_UNSEQUENCED_NOW
#undef RKI_DEF_SS_S_
#undef RKI_DEF_SA_S_
#undef RKI_DEF_SM_S_
#undef RKI_DEF_SAT_S

#if RKI_STDBIT_FALLBACK
# ifdef __GNUC__
#  if rk_has_builtin(__builtin_clzg)
#   define RKI_DEF_LZ__(V) __builtin_clzg(V)
#  else
#   define RKI_DEF_LZ__(V)                                                                         \
     _Generic(V,                                                                                   \
         default: (unsigned)__builtin_clz(V) - (bitsof(unsigned) - bitsof(V)),                     \
         unsigned long: __builtin_clzl(V),                                                         \
         unsigned long long: __builtin_clzll(V) RKI_IFHAS_INT128(                                  \
                  , u128 : (u64)((u128)V >> 64) ? __builtin_clzll((u64)((u128)V >> 64))            \
                                                : 64 + __builtin_clzll((u64)V)))
#  endif
#  define RKI_DEF_LZ_(T, V) return V ? (unsigned)RKI_DEF_LZ__(V) : bitsof(V);

# elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#  ifdef _M_X64
#   define RKI_DEF_LZ__(V) _BitScanReverse64(&idx, (unsigned long long)V), 63u - (unsigned)idx
#  elif defined(_M_IX86)
#   define RKI_DEF_LZ__(V)                                                                         \
     (hi = (unsigned)((unsigned long long)V >> 32))                                                \
         ? (_BitScanReverse(&idx, (unsigned long)hi), 31u - (unsigned)idx)                         \
         : (_BitScanReverse(&idx, (unsigned long)(unsigned)V), 63u - (unsigned)idx)
#  endif
#  define RKI_DEF_LZ_(T, V)                                                                        \
    if (!V) return bitsof(V);                                                                      \
    unsigned long idx;                                                                             \
    unsigned      hi;                                                                              \
    (void)hi;                                                                                      \
    return rk_static_if(sizeof(T) <= 4,                                                            \
                        (_BitScanReverse(&idx, (unsigned long)V),                                  \
                         31u - (unsigned)idx - (bitsof(unsigned) - bitsof(T))),                    \
                        (RKI_DEF_LZ__(V)));
# else
#  define RKI_DEF_LZ_(T, V)                                                                        \
    if (!V) { return bitsof(T); }                                                                  \
    unsigned count = 0;                                                                            \
    T        mask  = (T)1 << (bitsof(T) - 1);                                                      \
    while (!(V & mask)) { ++count, V <<= 1; }                                                      \
    return count;
# endif

# if rk_has_builtin(__builtin_ctzg)
#  define RKI_DEF_TZ_(V) return V ? (unsigned)__builtin_ctzg(V) : bitsof(V);
# elif defined(__GNUC__)
#  define RKI_DEF_TZ_(V)                                                                           \
    return V ? (unsigned)_Generic(V,                                                               \
                   default: __builtin_ctz(V),                                                      \
                   unsigned long: __builtin_ctzl(V),                                               \
                   unsigned long long: __builtin_ctzll(V)                                          \
                       RKI_IFHAS_INT128(, u128 : (u64)(V)                                          \
                                              ? __builtin_ctzll((u64)V)                            \
                                              : 64 + __builtin_ctzll((u64)((u128)V >> 64))))       \
             : bitsof(V);

# elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#  ifdef _M_X64
#   define RKI_DEF_TZ__(V) _BitScanForward64(&idx, (unsigned long long)V), (unsigned)idx

#  elif defined(_M_IX86)
#   define RKI_DEF_TZ__(V)                                                                         \
     (lo = (unsigned)(V))                                                                          \
         ? (_BitScanForward(&idx, (unsigned long)lo), (unsigned)idx)                               \
         : (_BitScanForward(&idx, (unsigned long)(unsigned)((unsigned long long)V >> 32)),         \
            32u + (unsigned)idx)
#  endif
#  define RKI_DEF_TZ_(V)                                                                           \
    if (!V) { return bitsof(V); }                                                                  \
    unsigned long idx;                                                                             \
    unsigned      lo;                                                                              \
    (void)lo;                                                                                      \
    return rk_static_if(sizeof(V) <= 4,                                                            \
                        (_BitScanForward(&idx, (unsigned long)(unsigned)V), (unsigned)idx),        \
                        (RKI_DEF_TZ__(V)));
# else
#  define RKI_DEF_TZ_(V)                                                                           \
    if (!V) { return bitsof(V); }                                                                  \
    unsigned count = 0;                                                                            \
    while (!(V & 1)) { ++count, V >>= 1; }                                                         \
    return count;
# endif

# if rk_has_builtin(__builtin_popcountg)
#  define RKI_DEF_CO_(V) return (unsigned)__builtin_popcountg(V);
# elif defined(__GNUC__)
#  define RKI_DEF_CO_(V)                                                                           \
    return (unsigned)_Generic(V,                                                                   \
        default: __builtin_popcount(V),                                                            \
        unsigned long: __builtin_popcountl(V),                                                     \
        unsigned long long: __builtin_popcountll(V)                                                \
            RKI_IFHAS_INT128(, u128 : __builtin_popcountll((u64)V)                                 \
                                   + __builtin_popcountll((u64)((u128)V >> 64))));
# else
#  define RKI_DEF_CO_(V)                                                                           \
    unsigned count = 0;                                                                            \
    while (V) { count++, V &= (V - 1); }                                                           \
    return count;
# endif

# define RKI_DEF_STDCBIT_FUNS(T, N)                                                                \
   rklib_fun rk_const unsigned stdc_leading_zeros_##N(T value)                                     \
       rk_unsequenced{RKI_DEF_LZ_(T, value)} /**/                                                  \
   rklib_fun rk_const unsigned stdc_trailing_zeros_##N(T value) rk_unsequenced{RKI_DEF_TZ_(value)} \
   /**/                                                                                            \
   rklib_fun rk_const unsigned stdc_count_ones_##N(T value) rk_unsequenced{RKI_DEF_CO_(value)}     \
   /**/                                                                                            \
   rklib_fun rk_const unsigned stdc_count_zeros_##N(T value) rk_unsequenced {                      \
     return bitsof(T) - stdc_count_ones_##N(value);                                                \
   } /**/                                                                                          \
   rklib_fun rk_const unsigned stdc_first_trailing_one_##N(T value) rk_unsequenced {               \
     return value ? stdc_trailing_zeros_##N(value) + 1u : 0u;                                      \
   }                                                                                               \
   rklib_fun rk_const unsigned stdc_leading_ones_##N(T value) rk_unsequenced {                     \
     return stdc_leading_zeros_##N((T)~value);                                                     \
   }                                                                                               \
   rklib_fun rk_const unsigned stdc_trailing_ones_##N(T value) rk_unsequenced {                    \
     return stdc_trailing_zeros_##N((T)~value);                                                    \
   }                                                                                               \
   rklib_fun rk_const unsigned stdc_first_leading_zero_##N(T value) rk_unsequenced {               \
     return value == (T) ~(T)0u ? 0u : stdc_leading_zeros_##N((T)~value) + 1u;                     \
   }                                                                                               \
   rklib_fun rk_const unsigned stdc_first_leading_one_##N(T value) rk_unsequenced {                \
     return value ? stdc_leading_zeros_##N(value) + 1u : 0u;                                       \
   }                                                                                               \
   rklib_fun rk_const unsigned stdc_first_trailing_zero_##N(T value) rk_unsequenced {              \
     return value == (T) ~(T)0u ? 0u : stdc_trailing_zeros_##N((T)~value) + 1u;                    \
   }                                                                                               \
   rklib_fun rk_const bool stdc_has_single_bit_##N(T value) rk_unsequenced {                       \
     return stdc_count_ones_##N(value) == 1u;                                                      \
   }                                                                                               \
   rklib_fun rk_const unsigned stdc_bit_width_##N(T value) rk_unsequenced {                        \
     return bitsof(T) - stdc_leading_zeros_##N(value);                                             \
   }                                                                                               \
   rklib_fun rk_const T stdc_bit_floor_##N(T value) rk_unsequenced {                               \
     return (T)(value ? ((T)1u << (stdc_bit_width_##N(value) - 1u)) : (T)0u);                      \
   }                                                                                               \
   rklib_fun rk_const T stdc_bit_ceil_##N(T value) rk_unsequenced {                                \
     if (!value) { return (T)1u; }                                                                 \
     size_t shift = bitsof(T) - stdc_leading_zeros_##N((T)(value - 1u));                           \
     return shift < bitsof(T) ? (T)((T)1u << shift) : (T)0u;                                       \
   }

RKI_U_TYPES(RKI_DEF_STDCBIT_FUNS)
# undef RKI_DEF_LZ_
# undef RKI_DEF_LZ__
# undef RKI_DEF_TZ_
# undef RKI_DEF_TZ__
# undef RKI_DEF_CO_
# undef RKI_DEF_STDCBIT_FUNS

#endif /* RKI_STDBIT_FALLBACK */

#define rk_assert_ptr_nonnull(ptr) rk_assert(((ptr) != rk_null) && #ptr " must not be rk_null.")

#define rk_assert_align_pow2(align)                                                                \
  rk_assert(stdc_has_single_bit(align) && #align " must be a power of two.")

#define rk_assert_valid_align(T, align)                                                            \
  rk_assert(alignof(T) <= (align) && #align " must be >= alignof(" #T ").")

rklib_fun rk_const size_t rk_align_up(size_t size, size_t align) {
  rk_assert_align_pow2(align);
#if rk_has_builtin(__builtin_align_up)
  return __builtin_align_up(size, align);
#else
  size_t mask = align - 1;
  rk_assert(size <= SIZE_MAX - mask && "Size overflow");
  return (size + mask) & ~mask;
#endif
}

rklib_fun rk_const size_t rk_align_pad(const void* ptr, size_t align) {
  rk_assert_align_pow2(align);
  return (-(uintptr_t)ptr) & (size_t)(align - 1);
}

RKI_HEADER_END
#ifdef __cplusplus
template <class T, size_t N>
constexpr inline size_t rki_countof(T (&)[N]) noexcept {
  return N;
}
#endif

#ifndef __cplusplus
# define RKI_STATIC_ASSERT_EXPR(...)                                                               \
   (0 * sizeof(union {                                                                             \
     static_assert(__VA_ARGS__);                                                                   \
     char _;                                                                                       \
    }))

#elif __cplusplus >= 202002L
# define RKI_STATIC_ASSERT_EXPR(...) (0 * sizeof([]() { static_assert(__VA_ARGS__); }))
#else
template <bool Condition>
struct rki_static_assert_expr_check {
  static_assert(Condition, "static_assert_expr: condition is false");
};
# define RKI_STATIC_ASSERT_EXPR_(condition, message, ...)                                          \
   (0 * sizeof(rki_static_assert_expr_check<!!(condition)>) + 0 * sizeof("" message))
# define RKI_STATIC_ASSERT_EXPR(...) RKI_STATIC_ASSERT_EXPR_(__VA_ARGS__, "", unused)
#endif

#ifdef __cplusplus
template <class T>
using rki_remove_reference_t = typename std::remove_reference<T>::type;
#endif

/// @endcond
#pragma endregion implementation
/* END DEFERRED IMPLEMENTATION: include/rk_defs.h */
#endif
#ifdef RKI_AMALG_IMPL_16C73346C0E1F501
#undef RKI_AMALG_IMPL_16C73346C0E1F501
/* BEGIN DEFERRED IMPLEMENTATION: include/rk_alloc.h */
RKI_HEADER_BEGIN
#pragma region implementation
////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

/// @cond INTERNAL
#ifndef _MSC_VER
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

/// @brief Returns a container's effective allocator.
/// If the stored allocator is unset (its `vtab` is NULL), returns `alloc_ctx`.
/// `self` must point to an object containing an `Allocator alloc` member.
/// The result is an rvalue and cannot be used to modify the stored allocator.
#if RK_CUSTOM_ALLOCATORS
# define RKI_REQUIRE_CUSTOM_ALLOCATORS(...) __VA_ARGS__

rklib_fun rk_pure rk_forceinline Allocator rki_allocator_of(Allocator alloc) {
  return alloc.vtab ? alloc : alloc_ctx;
}
# define RKI_allocatorof(self)              rki_allocator_of((self)->alloc)
# define RKI_assert_allocator_valid(_alloc) rk_assert((_alloc).vtab && "Invalid Allocator")
# define RKI_set_alloc_fallback(_alloc)                                                            \
   ((void)(rk_likely((_alloc).vtab)                                                                \
               ? alloc_ctx                                                                         \
               : (RKI_assert_allocator_valid(alloc_ctx), (_alloc) = alloc_ctx)))
#else
# define RKI_allocator_disabled_assert()    static_assert_expr(0, "Allocators Disabled")
# define RKI_REQUIRE_CUSTOM_ALLOCATORS(...) ((void*)RKI_allocator_disabled_assert())
# define RKI_allocatorof(self)              ((void)(self), alloc_ctx)
# define RKI_assert_allocator_valid(_alloc) ((void)0)
# define RKI_set_alloc_fallback(_alloc) ((void)0)
#endif

// Allocation pointer/size pairing shared by every layer: a null pointer has size zero, and a
// non-null allocation has a positive size.
#define RKI_ASSERT_ALLOC_PAIR(ptr, size)                                                           \
  ((ptr) ? rk_assert((size) && "Non-NULL allocation has zero size")                                \
         : rk_assert(!(size) && "NULL allocation has nonzero size"))

///////////////////////// Page Allocator /////////////////////////////////
#if defined(_MSC_VER) && !defined(_WINDOWS_)
__declspec(dllimport) void* __stdcall VirtualAlloc(void* lpAddress, size_t dwSize,
                                                   unsigned long flAllocationType,
                                                   unsigned long flProtect);
__declspec(dllimport) int __stdcall   VirtualFree(void* lpAddress, size_t dwSize,
                                                  unsigned long dwFreeType);
#endif

rklib_fun size_t rki_mmap_page_size(void) {
#ifndef _MSC_VER
  long ps = sysconf(_SC_PAGESIZE);
  RK_MMAP_FAIL(ps != -1, ps, rk_null, 0, 0);
  return (size_t)ps;
#else
  return 4096;
#endif
}

rklib_fun rk_malloc_fun rk_alloc_size(1) void* page_alloc(size_t size) {
  if rk_unlikely (!size) { return rk_null; }
  size_t ps = rki_mmap_page_size();
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

rklib_fun void page_free(void* ptr, size_t size) {
  if rk_unlikely (!ptr || !size) { return; }
  size_t ps = rki_mmap_page_size();
  size      = rk_align_up(size, ps);
#ifndef _MSC_VER
  int r = munmap(ptr, size);
  RK_MMAP_FAIL(r == 0, ps, ptr, 0, size);
#else
  int r = VirtualFree(ptr, 0, 0x00008000);
  RK_MMAP_FAIL(r != 0, ps, ptr, 0, size);
#endif
}

rklib_fun rk_alloc_size(3) void* page_realloc(void* ptr, size_t old_size, size_t new_size) {
  RKI_ASSERT_ALLOC_PAIR(ptr, old_size);
  if (!new_size) { return page_free(ptr, old_size), rk_null; }
  if (!ptr) { return page_alloc(new_size); }
  size_t ps      = rki_mmap_page_size();
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

rklib_fun rk_malloc_fun rk_alloc_alignsize(2, 1) void* rki_page_allocate(size_t       size,
                                                                         size_t align rk_unused,
                                                                         void* ctx    rk_unused) {
  rk_assert(align <= rki_mmap_page_size() && "Wrong alignment");
  return page_alloc(size);
}

rklib_fun rk_alloc_alignsize(4, 3) void* rki_page_reallocate(void* ptr, size_t old_size,
                                                             size_t       new_size,
                                                             size_t align rk_unused,
                                                             void* ctx    rk_unused) {
  rk_assert(align <= rki_mmap_page_size() && "Wrong alignment");
  return page_realloc(ptr, old_size, new_size);
}

rklib_fun void rki_page_deallocate(void* ptr, size_t old_size, size_t align rk_unused,
                                   void* ctx rk_unused) {
  rk_assert(align <= rki_mmap_page_size() && "Wrong alignment");
  page_free(ptr, old_size);
}
///////////////////////////////////    Malloc wrappers   ///////////////////////////////////////////
// Zero/null normalization is retained here for direct public malloc_* entry points.

rklib_fun rk_forceinline rk_malloc_fun rk_alloc_size(1) void* rki_malloc_f(size_t size) {
  if rk_unlikely (!size) { return rk_null; }
  void* res = malloc(size);
  RK_MALLOC_FAIL(res, rk_null, rk_null, RKI_MALLOC_ALIGN, size);
  return res;
}

rklib_fun rk_forceinline void rki_free_f(void* ptr) { free(ptr); }

rklib_fun rk_forceinline      rk_alloc_size(2) void* rki_realloc_f(void* ptr, size_t size) {
  if (!size) { return rki_free_f(ptr), rk_null; }
  if (ptr == rk_null) { return rki_malloc_f(size); }
  void* res = realloc(ptr, size);
  RK_MALLOC_FAIL(res, rk_null, ptr, RKI_MALLOC_ALIGN, size);
  return res;
}

rklib_fun rk_malloc_fun rk_alloc_alignsize(2, 1) void* rki_aligned_alloc_f(size_t size,
                                                                           size_t align) {
  rk_assert_align_pow2(align);
  if rk_unlikely (!size) { return rk_null; }
  align = rk_max(align, RKI_MALLOC_ALIGN);
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
# define rki_aligned_free_f rki_free_f
#else

rklib_fun rk_forceinline void rki_aligned_free_f(void* ptr) {
  if (ptr != rk_null) { _aligned_free(ptr); }
}
#endif

rklib_fun rk_forceinline rk_alloc_alignsize(4, 3) void* rki_aligned_realloc_f(void*  ptr,
                                                                              size_t old_size,
                                                                              size_t new_size,
                                                                              size_t align) {
  // Same dispatch as rki_realloc_f: keyed on the pointer, not on old_size.
  RKI_ASSERT_ALLOC_PAIR(ptr, old_size);
  if (!new_size) { return rki_aligned_free_f(ptr), rk_null; }
  if (!ptr) { return rki_aligned_alloc_f(new_size, align); }
  rk_assert_align_pow2(align);
  align    = rk_max(align, RKI_MALLOC_ALIGN);
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

rklib_fun rk_malloc_fun rk_alloc_alignsize(2, 1) void* rki_malloc_allocate(size_t    size,
                                                                           size_t    align,
                                                                           void* ctx rk_unused) {
  return align <= RKI_MALLOC_ALIGN ? rki_malloc_f(size) : rki_aligned_alloc_f(size, align);
}

rklib_fun rk_alloc_alignsize(4, 3) void* rki_malloc_reallocate(void* ptr, size_t old_size,
                                                               size_t new_size, size_t align,
                                                               void* ctx rk_unused) {
  RKI_ASSERT_ALLOC_PAIR(ptr, old_size); // the ordinary realloc path ignores old_size
  return align <= RKI_MALLOC_ALIGN ? rki_realloc_f(ptr, new_size)
                                   : rki_aligned_realloc_f(ptr, old_size, new_size, align);
}

rklib_fun void rki_malloc_deallocate(void* ptr, size_t old_size rk_unused, size_t align rk_unused,
                                     void* ctx rk_unused) {
  align <= RKI_MALLOC_ALIGN ? rki_free_f(ptr) : rki_aligned_free_f(ptr);
}
// dynamically chose whether malloc or aligned_alloc
#define RKI_MALLOC_ALLOCATE(bytes, align)                                                          \
  (alloc_log_new(), rki_malloc_allocate(bytes, align, rk_null))
#define RKI_MALLOC_REALLOCATE(ptr, obytes, nbytes, align)                                          \
  (alloc_log_renew(), rki_malloc_reallocate(ptr, obytes, nbytes, align, rk_null))
#define RKI_MALLOC_DEALLOCATE(ptr, align)                                                          \
  (alloc_log_delete(), rki_malloc_deallocate(ptr, 0, align, rk_null))
// always call malloc, compiler error if over-aligned
#define RKI_MALLOC_NEW(T, count)                                                                   \
  (alloc_log_new(), rk_ensure_malloc_align(T), rki_malloc_f(sizeof_n(T, count)))
#define RKI_MALLOC_RENEW(ptr, count)                                                               \
  (alloc_log_renew(), rk_ensure_malloc_align(typeof(*(ptr))),                                      \
   rki_realloc_f(ptr, sizeof_n(*(ptr), count)))
#define RKI_MALLOC_DELETE(ptr)                                                                     \
  (alloc_log_delete(), rk_ensure_malloc_align(typeof(*(ptr))), rki_free_f(ptr))
// always call aligned_alloc, check if alignment is enough for type
#define RKI_MALLOC_ALIGNED_NEW(T, count, align)                                                    \
  (alloc_log_new(), rk_assert_valid_align(T, align), rki_aligned_alloc_f(sizeof_n(T, count), align))
#define RKI_MALLOC_ALIGNED_RENEW(ptr, old_count, new_count, align)                                 \
  (alloc_log_renew(), rk_assert_valid_align(typeof(*(ptr)), align),                                \
   rki_aligned_realloc_f(ptr, sizeof_n(*(ptr), old_count), sizeof_n(*(ptr), new_count), align))
#define RKI_MALLOC_ALIGNED_DELETE(ptr) (alloc_log_delete(), rki_aligned_free_f(ptr))
///////////////////////////////////  Alloc Wrappers ////////////////////////////////////////////////
#if RK_CUSTOM_ALLOCATORS

rklib_fun rk_forceinline rk_alloc_alignsize(2, 1) void* rki_call_alloc(size_t nbytes, size_t align,
                                                                       Allocator alloc) {
  rk_assert(alloc.vtab && "Invalid Allocator");
  if (!nbytes) { return rk_null; }
  return alloc.vtab->alloc_f(nbytes, align, alloc.ctx);
}

rklib_fun rk_forceinline rk_alloc_alignsize(4, 3) void* rki_call_realloc(void* ptr, size_t obytes,
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

rklib_fun rk_forceinline void rki_call_dealloc(void* ptr, size_t obytes, size_t align,
                                               Allocator alloc) {
  rk_assert(alloc.vtab && "Invalid Allocator");
  if (!ptr) {
    rk_assert(!obytes && "NULL allocation has nonzero size");
    return;
  }
  rk_assert(obytes && "Non-NULL allocation has zero size");
  alloc.vtab->dealloc_f(ptr, obytes, align, alloc.ctx);
}
# define RKI_ALLOC_ALLOCATE(bytes, align, all) (alloc_log_new(), rki_call_alloc(bytes, align, all))
# define RKI_ALLOC_REALLOCATE(ptr, obytes, nbytes, align, all)                                     \
   (alloc_log_renew(), rki_call_realloc(ptr, obytes, nbytes, align, all))
# define RKI_ALLOC_DEALLOCATE(ptr, obytes, align, all)                                             \
   (alloc_log_delete(), rki_call_dealloc(ptr, obytes, align, all))
#else

rklib_fun rk_forceinline rk_alloc_alignsize(2, 1) void* rki_call_alloc(size_t nbytes,
                                                                       size_t align) {
  if (!nbytes) { return rk_null; }
  return alloc_ctx.vtab->alloc_f(nbytes, align, alloc_ctx.ctx);
}

rklib_fun rk_forceinline rk_alloc_alignsize(4, 3) void* rki_call_realloc(void* ptr, size_t obytes,
                                                                         size_t nbytes,
                                                                         size_t align) {
  if (!nbytes) {
    if (ptr) {
      rk_assert(obytes && "Non-NULL allocation has zero size");
      alloc_ctx.vtab->dealloc_f(ptr, obytes, align, alloc_ctx.ctx);
    } else {
      rk_assert(!obytes && "NULL allocation has nonzero size");
    }
    return rk_null;
  }
  if (!ptr) {
    rk_assert(!obytes && "NULL allocation has nonzero size");
    return alloc_ctx.vtab->alloc_f(nbytes, align, alloc_ctx.ctx);
  }
  rk_assert(obytes && "Non-NULL allocation has zero size");
  return alloc_ctx.vtab->realloc_f(ptr, obytes, nbytes, align, alloc_ctx.ctx);
}

rklib_fun rk_forceinline void rki_call_dealloc(void* ptr, size_t obytes, size_t align) {
  if (!ptr) {
    rk_assert(!obytes && "NULL allocation has nonzero size");
    return;
  }
  rk_assert(obytes && "Non-NULL allocation has zero size");
  alloc_ctx.vtab->dealloc_f(ptr, obytes, align, alloc_ctx.ctx);
}
# define RKI_ALLOC_ALLOCATE(bytes, align, all) (alloc_log_new(), rki_call_alloc(bytes, align))
# define RKI_ALLOC_REALLOCATE(ptr, obytes, nbytes, align, all)                                     \
   (alloc_log_renew(), rki_call_realloc(ptr, obytes, nbytes, align))
# define RKI_ALLOC_DEALLOCATE(ptr, obytes, align, all)                                             \
   (alloc_log_delete(), rki_call_dealloc(ptr, obytes, align))
#endif
#define RKI_ALLOC_NEW(T, count, all) RKI_ALLOC_ALLOCATE(sizeof_n(T, count), alignof(T), all)
#define RKI_ALLOC_ALIGNED_NEW(T, count, align, all)                                                \
  (rk_assert_valid_align(T, align), RKI_ALLOC_ALLOCATE(sizeof_n(T, count), align, all))
#define RKI_ALLOC_RENEW(ptr, ocount, ncount, all)                                                  \
  RKI_ALLOC_REALLOCATE(ptr, sizeof_n(*(ptr), ocount), sizeof_n(*(ptr), ncount),                    \
                       alignof(typeof(*(ptr))), all)
#define RKI_ALLOC_ALIGNED_RENEW(ptr, ocount, ncount, align, all)                                   \
  (rk_assert_valid_align(typeof(*(ptr)), align),                                                   \
   RKI_ALLOC_REALLOCATE(ptr, sizeof_n(*(ptr), ocount), sizeof_n(*(ptr), ncount), align, all))
#define RKI_ALLOC_DELETE(ptr, ocount, all)                                                         \
  RKI_ALLOC_DEALLOCATE(ptr, sizeof_n(*(ptr), ocount), alignof(typeof(*(ptr))), all)
#define RKI_ALLOC_ALIGNED_DELETE(ptr, ocount, align, all)                                          \
  (rk_assert_valid_align(typeof(*(ptr)), align),                                                   \
   RKI_ALLOC_DEALLOCATE(ptr, sizeof_n(*(ptr), ocount), align, all))
// macros with allocator parameter; disabled if no local allocators enabled
#define RKI_ALLOC_ALLOCATE3(bytes, align, all)                                                     \
  RKI_REQUIRE_CUSTOM_ALLOCATORS(RKI_ALLOC_ALLOCATE(bytes, align, all))
#define RKI_ALLOC_REALLOCATE5(ptr, obytes, nbytes, align, all)                                     \
  RKI_REQUIRE_CUSTOM_ALLOCATORS(RKI_ALLOC_REALLOCATE(ptr, obytes, nbytes, align, all))
#define RKI_ALLOC_DEALLOCATE4(ptr, obytes, align, all)                                             \
  RKI_REQUIRE_CUSTOM_ALLOCATORS(RKI_ALLOC_DEALLOCATE(ptr, obytes, align, all))
#define RKI_ALLOC_NEW3(T, count, all) RKI_REQUIRE_CUSTOM_ALLOCATORS(RKI_ALLOC_NEW(T, count, all))
#define RKI_ALLOC_ALIGNED_NEW4(T, count, align, all)                                               \
  RKI_REQUIRE_CUSTOM_ALLOCATORS(RKI_ALLOC_ALIGNED_NEW(T, count, align, all))
#define RKI_ALLOC_RENEW4(ptr, ocount, ncount, all)                                                 \
  RKI_REQUIRE_CUSTOM_ALLOCATORS(RKI_ALLOC_RENEW(ptr, ocount, ncount, all))
#define RKI_ALLOC_ALIGNED_RENEW5(ptr, ocount, ncount, align, all)                                  \
  RKI_REQUIRE_CUSTOM_ALLOCATORS(RKI_ALLOC_ALIGNED_RENEW(ptr, ocount, ncount, align, all))
#define RKI_ALLOC_DELETE3(ptr, ocount, all)                                                        \
  RKI_REQUIRE_CUSTOM_ALLOCATORS(RKI_ALLOC_DELETE(ptr, ocount, all))
#define RKI_ALLOC_ALIGNED_DELETE4(ptr, ocount, align, all)                                         \
  RKI_REQUIRE_CUSTOM_ALLOCATORS(RKI_ALLOC_ALIGNED_DELETE(ptr, ocount, align, all))
//  macros with fewer parameters (might default to alloc_ctx)
#define RKI_ALLOC_ALLOCATE2(bytes, align)       RKI_ALLOC_ALLOCATE(bytes, align, alloc_ctx)
#define RKI_ALLOC_ALIGNED_NEW3(T, count, align) RKI_ALLOC_ALIGNED_NEW(T, count, align, alloc_ctx)
#define RKI_ALLOC_NEW2(T, count)                RKI_ALLOC_NEW(T, count, alloc_ctx)
#define RKI_ALLOC_REALLOCATE4(ptr, obytes, nbytes, align)                                          \
  RKI_ALLOC_REALLOCATE(ptr, obytes, nbytes, align, alloc_ctx)
#define RKI_ALLOC_ALIGNED_RENEW4(ptr, ocount, ncount, align)                                       \
  RKI_ALLOC_ALIGNED_RENEW(ptr, ocount, ncount, align, alloc_ctx)
#define RKI_ALLOC_RENEW3(ptr, ocount, ncount) RKI_ALLOC_RENEW(ptr, ocount, ncount, alloc_ctx)
#define RKI_ALLOC_DEALLOCATE3(ptr, obytes, align)                                                  \
  RKI_ALLOC_DEALLOCATE(ptr, obytes, align, alloc_ctx)
#define RKI_ALLOC_ALIGNED_DELETE3(ptr, ocount, align)                                              \
  RKI_ALLOC_ALIGNED_DELETE(ptr, ocount, align, alloc_ctx)
#define RKI_ALLOC_DELETE2(ptr, ocount) RKI_ALLOC_DELETE(ptr, ocount, alloc_ctx)
rklib_fun
    rk_alloc_alignsize(3, 2) void* rki_memdup_aligned(const void* src, size_t size,
                                                      size_t align RK_IFALLOC(, Allocator alloc)) {
  return rk_memcpy(alloc_allocate(size, align RK_IFALLOC(, alloc)), src, size);
}
#define RKI_MEMDUP4(src, nbytes, align, alloc)                                                     \
  RKI_REQUIRE_CUSTOM_ALLOCATORS(rki_memdup_aligned(src, nbytes, align, alloc))
#define RKI_MEMDUP3(src, nbytes, align)                                                            \
  rki_memdup_aligned(src, nbytes, align RK_IFALLOC(, alloc_ctx))
#undef RKI_ALLOCCTX_STORAGE
#undef RKI_ALLOCCTX_INIT
/// @endcond
#pragma endregion implementation
RKI_HEADER_END
/* END DEFERRED IMPLEMENTATION: include/rk_alloc.h */
#endif
#ifdef RKI_AMALG_IMPL_28FAD6E6119BCEC1
#undef RKI_AMALG_IMPL_28FAD6E6119BCEC1
/* BEGIN DEFERRED IMPLEMENTATION: include/rk_trees.h */
RKI_HEADER_BEGIN
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
/* END DEFERRED IMPLEMENTATION: include/rk_trees.h */
#endif
#ifdef RKI_AMALG_IMPL_EAF61C7F654781E3
#undef RKI_AMALG_IMPL_EAF61C7F654781E3
/* BEGIN DEFERRED IMPLEMENTATION: include/rk_bitset.h */
RKI_HEADER_BEGIN
#pragma region implementation
////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

#define rki_assert_bitset_in_bounds(idx, nbits)                                                    \
  rk_assert((idx) < (nbits) && "Index out of Bitset bounds.")

// Each word is loaded once. Mutations to other bits in a cached word are not observed.
typedef struct RKI_BitsetIter {
  cbitset     bits;
  size_t      count, word;
  bitset_word remaining;
} RKI_BitsetIter;

rklib_fun rk_forceinline size_t rki_bitset_iter_next(RKI_BitsetIter* state) {
  enum { W = bitsof(bitset_word) }; // NOLINT
  while (!state->remaining) {
    if (state->word >= state->count / W + (state->count % W != 0)) { return BITSET_NPOS; }
    state->remaining  = state->bits[state->word++];
    const size_t rest = state->count % W;
    if (rest && state->word - 1 == state->count / W) {
      state->remaining &= ((bitset_word)1 << rest) - 1;
    }
  }
  const size_t bit  = stdc_first_trailing_one(state->remaining) - 1;
  state->remaining &= state->remaining - 1;
  return (state->word - 1) * W + bit;
}

rklib_fun rk_forceinline size_t rki_bitset_iter_prev(RKI_BitsetIter* state) {
  enum { W = bitsof(bitset_word) }; // NOLINT
  while (!state->remaining) {
    if (!state->word) { return BITSET_NPOS; }
    state->remaining  = state->bits[--state->word];
    const size_t rest = state->count % W;
    if (rest && state->word == state->count / W) {
      state->remaining &= ((bitset_word)1 << rest) - 1;
    }
  }
  const size_t bit  = W - stdc_first_leading_one(state->remaining);
  state->remaining &= ~((bitset_word)1 << bit);
  return state->word * W + bit;
}

#define RKI_BITSET_FOREACH(self, nbits, idx)                                                       \
  for (RKI_BitsetIter rki_var_state = {(self), (nbits), 0, 0}; rki_var_state.bits;                 \
       rki_var_state.bits           = rk_null)                                                     \
    for (size_t idx = 0;                                                                           \
         (idx = rki_bitset_iter_next(&rki_var_state)) != BITSET_NPOS && ((void)idx, 1);)

#define RKI_BITSET_FOREACH_REVERSED(self, nbits, idx)                                              \
  for (RKI_BitsetIter rki_var_state = {(self), (nbits), 0, 0};                                     \
       rki_var_state.bits                                                                          \
       && (rki_var_state.word = rki_var_state.count / bitsof(bitset_word)                          \
                              + (rki_var_state.count % bitsof(bitset_word) != 0),                  \
          1);                                                                                      \
       rki_var_state.bits = rk_null)                                                               \
    for (size_t idx = 0;                                                                           \
         (idx = rki_bitset_iter_prev(&rki_var_state)) != BITSET_NPOS && ((void)idx, 1);)

#define RKI_BITSET_CLEAR_IF(self, nbits, idx, pred)                                                \
  do {                                                                                             \
    bitset const rki_var_bits  = (self);                                                           \
    const size_t rki_var_count = (nbits);                                                          \
    RKI_BITSET_FOREACH(rki_var_bits, rki_var_count, rki_var_cursor) {                              \
      const size_t idx = rki_var_cursor;                                                           \
      (void)idx;                                                                                   \
      if (pred) { bitset_clear(rki_var_bits, rki_var_count, idx); }                                \
    }                                                                                              \
  } while (0)

/// @brief Internal helper implementing range operations.
/// @param bs Bitset to modify
/// @param nbits Logical size of the bitset
/// @param start First bit index (inclusive)
/// @param end Last bit index (exclusive)
/// @param op Operation selector: -1 toggle, 0 clear, 1 set
/// @return `bs`.
/// @pre `start <= end && end <= nbits`.
/// @warning Internal API.
rklib_fun rk_forceinline bitset rki_bitset_range_op(bitset bs, size_t nbits, size_t start,
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

// Public function definitions.

rklib_fun bitset bitset_copy(bitset restrict dst, size_t nbits, cbitset restrict src) {
  return rk_copy(dst, src, bitset_words(nbits));
}

rklib_fun rk_pure bool bitset_test(cbitset bs, size_t nbits, size_t idx) {
  rki_assert_bitset_in_bounds(idx, nbits), (void)nbits;
  return (bs[bitset_word_index(idx)] & bitset_word_mask(idx)) != 0;
}

rklib_fun bitset bitset_clear(bitset bs, size_t nbits, size_t idx) {
  rki_assert_bitset_in_bounds(idx, nbits), (void)nbits;
  bs[bitset_word_index(idx)] &= ~bitset_word_mask(idx);
  return bs;
}

rklib_fun bitset bitset_set(bitset bs, size_t nbits, size_t idx) {
  rki_assert_bitset_in_bounds(idx, nbits), (void)nbits;
  bs[bitset_word_index(idx)] |= bitset_word_mask(idx);
  return bs;
}

rklib_fun bitset bitset_write(bitset bs, size_t nbits, size_t idx, bool value) {
  rki_assert_bitset_in_bounds(idx, nbits), (void)nbits;
  bitset_word mask = bitset_word_mask(idx);
  size_t      w    = bitset_word_index(idx);
  bs[w]            = (bs[w] & ~mask) | (-((bitset_word)value) & mask);
  return bs;
}

rklib_fun bitset bitset_flip(bitset bs, size_t nbits, size_t idx) {
  rki_assert_bitset_in_bounds(idx, nbits), (void)nbits;
  bs[bitset_word_index(idx)] ^= bitset_word_mask(idx);
  return bs;
}

rklib_fun bitset bitset_clear_range(bitset bs, size_t nbits, size_t start, size_t end) {
  return rki_bitset_range_op(bs, nbits, start, end, 0);
}

rklib_fun bitset bitset_set_range(bitset bs, size_t nbits, size_t start, size_t end) {
  return rki_bitset_range_op(bs, nbits, start, end, 1);
}

rklib_fun bitset bitset_write_range(bitset bs, size_t nbits, size_t start, size_t end, bool value) {
  return value ? bitset_set_range(bs, nbits, start, end)
               : bitset_clear_range(bs, nbits, start, end);
}

rklib_fun bitset bitset_flip_range(bitset bs, size_t nbits, size_t start, size_t end) {
  return rki_bitset_range_op(bs, nbits, start, end, -1);
}

rklib_fun bitset bitset_clear_padding(bitset bs, size_t nbits) {
  size_t words = bitset_words(nbits), rest = nbits % bitset_word_bits;
  if (rest) { bs[words - 1] &= (((bitset_word)1 << rest) - 1); }
  return bs;
}

rklib_fun bitset bitset_clear_all(bitset bs, size_t nbits) {
  return (bitset)rk_memset(bs, 0, sizeof_n(*bs, bitset_words(nbits)));
}

rklib_fun bitset bitset_set_all(bitset bs, size_t nbits) {
  rk_memset(bs, 0xFF, sizeof_n(*bs, bitset_words(nbits)));
  return bitset_clear_padding(bs, nbits);
}

rklib_fun bitset bitset_write_all(bitset bs, size_t nbits, bool value) {
  return value ? bitset_set_all(bs, nbits) : bitset_clear_all(bs, nbits);
}

rklib_fun bitset bitset_flip_all(bitset bs, size_t nbits) {
  size_t words = bitset_words(nbits);
  for (size_t w = 0; w < words; ++w) { bs[w] = ~bs[w]; }
  return bitset_clear_padding(bs, nbits);
}

rklib_fun rk_pure size_t bitset_first_leading_one(cbitset bs, size_t nbits) {
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

rklib_fun rk_pure size_t bitset_first_leading_zero(cbitset bs, size_t nbits) {
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

rklib_fun rk_pure size_t bitset_first_trailing_one(cbitset bs, size_t nbits) {
  rk_assert(nbits > 0);
  for (size_t w = 0, words = bitset_words(nbits); w < words; ++w) {
    size_t pos = stdc_first_trailing_one(bs[w]);
    if (pos) { return pos + bitset_word_bits * w; }
  }
  return 0;
}

rklib_fun rk_pure size_t bitset_first_trailing_zero(cbitset bs, size_t nbits) {
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

rklib_fun rk_pure size_t bitset_leading_zeros(cbitset bs, size_t nbits) {
  size_t pos = bitset_first_leading_one(bs, nbits);
  return pos ? pos - 1 : nbits;
}

rklib_fun rk_pure size_t bitset_leading_ones(cbitset bs, size_t nbits) {
  size_t pos = bitset_first_leading_zero(bs, nbits);
  return pos ? pos - 1 : nbits;
}

rklib_fun rk_pure size_t bitset_trailing_zeros(cbitset bs, size_t nbits) {
  size_t pos = bitset_first_trailing_one(bs, nbits);
  return pos ? pos - 1 : nbits;
}

rklib_fun rk_pure size_t bitset_trailing_ones(cbitset bs, size_t nbits) {
  size_t pos = bitset_first_trailing_zero(bs, nbits);
  return pos ? pos - 1 : nbits;
}

rklib_fun rk_pure size_t bitset_count_ones(cbitset bs, size_t nbits) {
  size_t words = bitset_words(nbits), count = 0;
  for (size_t w = 0; w < words; ++w) { count += stdc_count_ones(bs[w]); }
  return count;
}

rklib_fun rk_pure size_t bitset_count_zeros(cbitset bs, size_t nbits) {
  return nbits - bitset_count_ones(bs, nbits);
}

rklib_fun rk_pure bool bitset_any(cbitset bs, size_t nbits) {
  for (size_t w = 0, words = bitset_words(nbits); w < words; ++w) {
    if (bs[w]) { return true; }
  }
  return false;
}

rklib_fun rk_pure bool bitset_none(cbitset bs, size_t nbits) { return !bitset_any(bs, nbits); }

rklib_fun rk_pure bool bitset_all(cbitset bs, size_t nbits) {
  size_t rest = nbits % bitset_word_bits, w = 0;
  for (size_t words = bitset_words(nbits); w < words - (rest != 0); ++w) {
    if (bs[w] != ((bitset_word)~0)) { return false; }
  }
  if (rest) { return bs[w] == (((bitset_word)1 << rest) - 1); }
  return true;
}

rklib_fun rk_pure bool bitset_has_single_bit(cbitset bs, size_t nbits) {
  size_t count = 0;
  for (size_t w = 0, words = bitset_words(nbits); w < words; ++w) {
    if ((count += stdc_count_ones(bs[w])) > 1) { return false; }
  }
  return count == 1;
}

rklib_fun rk_pure bool bitset_equals(cbitset a, size_t nbits, cbitset b) {
  return rk_memcmp(a, b, sizeof_n(*a, bitset_words(nbits))) == 0;
}

rklib_fun bitset bitset_or(bitset dst, size_t nbits, cbitset src) {
  for (size_t w = 0, words = bitset_words(nbits); w < words; ++w) { dst[w] |= src[w]; }
  return dst;
}

rklib_fun bitset bitset_and(bitset dst, size_t nbits, cbitset src) {
  for (size_t w = 0, words = bitset_words(nbits); w < words; ++w) { dst[w] &= src[w]; }
  return dst;
}

rklib_fun bitset bitset_xor(bitset dst, size_t nbits, cbitset src) {
  for (size_t w = 0, words = bitset_words(nbits); w < words; ++w) { dst[w] ^= src[w]; }
  return dst;
}

rklib_fun bitset bitset_sub(bitset dst, size_t nbits, cbitset src) {
  for (size_t w = 0, words = bitset_words(nbits); w < words; ++w) { dst[w] &= ~src[w]; }
  return bitset_clear_padding(dst, nbits);
}

rklib_fun bitset bitset_not(bitset dst, size_t nbits, cbitset src) {
  for (size_t w = 0, words = bitset_words(nbits); w < words; ++w) { dst[w] = ~src[w]; }
  return bitset_clear_padding(dst, nbits);
}

rklib_fun bitset bitset_shift_left_into(bitset dst, size_t nbits, cbitset src, size_t sh) {
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

rklib_fun bitset bitset_shift_left(bitset bs, size_t nbits, size_t sh) {
  return bitset_shift_left_into(bs, nbits, bs, sh);
}

rklib_fun bitset bitset_shift_right_into(bitset dst, size_t nbits, cbitset src, size_t sh) {
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

rklib_fun bitset bitset_shift_right(bitset bs, size_t nbits, size_t sh) {
  return bitset_shift_right_into(bs, nbits, bs, sh);
}

rklib_fun rk_pure size_t bitset_find_next_set(cbitset bs, size_t nbits, size_t cur) {
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

rklib_fun rk_pure size_t bitset_find_next_clear(cbitset bs, size_t nbits, size_t cur) {
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

rklib_fun rk_pure size_t bitset_find_prev_set(cbitset bs, size_t nbits rk_unused, size_t cur) {
  enum { W = bitsof(*bs) }; // NOLINT
  rk_assert(cur <= nbits);
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

rklib_fun rk_pure size_t bitset_find_prev_clear(cbitset bs, size_t nbits, size_t cur) {
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

rklib_fun rk_pure size_t bitset_find_first_set(cbitset bs, size_t nbits) {
  return bitset_find_next_set(bs, nbits, BITSET_NPOS);
}

rklib_fun rk_pure size_t bitset_find_first_clear(cbitset bs, size_t nbits) {
  return bitset_find_next_clear(bs, nbits, BITSET_NPOS);
}

rklib_fun rk_pure size_t bitset_find_last_set(cbitset bs, size_t nbits) {
  return bitset_find_prev_set(bs, nbits, nbits);
}

rklib_fun rk_pure size_t bitset_find_last_clear(cbitset bs, size_t nbits) {
  return bitset_find_prev_clear(bs, nbits, nbits);
}

rklib_fun char* bitset_tostr(char* restrict dst, cbitset restrict src, size_t nbits) {
  for (size_t w = 0; w < nbits; ++w) { dst[w] = '0' + bitset_test(src, nbits, nbits - 1 - w); }
  dst[nbits] = '\0';
  return dst;
}

rklib_fun bitset bitset_fromstr(bitset dst, const char* restrict src, size_t len) {
  bitset_clear_all(dst, len);
  for (size_t w = 0; w < len; ++w) {
    rk_assert((src[w] == '0' || src[w] == '1') && "Invalid character in bitset string");
    bitset_write(dst, len, len - 1 - w, src[w] != '0');
  }
  return bitset_clear_padding(dst, len);
}

/// @endcond
#pragma endregion implementation
RKI_HEADER_END
/* END DEFERRED IMPLEMENTATION: include/rk_bitset.h */
#endif
#ifdef RKI_AMALG_IMPL_7A588F7E95C26AAF
#undef RKI_AMALG_IMPL_7A588F7E95C26AAF
/* BEGIN DEFERRED IMPLEMENTATION: include/rk_arena.h */
RKI_HEADER_BEGIN
RKI_IGNWARN_CLANG_BEG("-Wreturn-type-c-linkage")
#pragma region implementation
////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

rklib_fun rk_const Arena arena_init(unsigned char* arr, size_t len) {
  return (Arena){.beg = arr, .cur = arr, .end = arr ? arr + len : 0};
}
rklib_fun Arena*         arena_clear(Arena* self) { return self->cur = self->beg, self; }
rklib_fun rk_pure size_t arena_cap(const Arena* self) {
  return rk_likely(self->beg) ? (size_t)(self->end - self->beg) : 0;
}
rklib_fun rk_pure size_t arena_used(const Arena* self) {
  return rk_likely(self->beg) ? (size_t)(self->cur - self->beg) : 0;
}
rklib_fun rk_pure size_t arena_remaining(const Arena* self) {
  return rk_likely(self->beg) ? (size_t)(self->end - self->cur) : 0;
}

rklib_fun rk_pure bool arena_is_empty(const Arena* self) {
  return rk_likely(self->beg) ? self->cur == self->beg : true;
}
rklib_fun rk_pure ArenaMark arena_mark(const Arena* self) { return (ArenaMark){.pos = self->cur}; }

rklib_fun Arena*            arena_rewind_to(Arena* self, ArenaMark mark) {
  if (mark.pos == self->cur) { return self; }
  rk_assert(rk_ptr_in_range(mark.pos, self->beg, self->cur) && "Pointer is outside this Arena");
  self->cur = mark.pos;
  return self;
}
rklib_fun rk_pure bool arena_is_top_allocation(const Arena* self, const void* ptr, size_t size) {
  return (const unsigned char*)ptr + size == self->cur;
}

rklib_fun rk_alloc_alignsize(2, 1) void* arena_try_allocate(size_t nbytes, size_t align,
                                                            Arena* self) {
  if rk_unlikely (!self->cur) { return rk_null; }
  size_t pad = rk_align_pad(self->cur, align), avail = arena_remaining(self);
  if (avail < pad || avail - pad < nbytes) { return rk_null; }
  unsigned char* ptr = self->cur + pad;
  self->cur          = ptr + nbytes;
  return ptr;
}

rklib_fun rk_alloc_size(2) void* arena_try_resize_top(size_t old_size, size_t new_size,
                                                      Arena* self) {
  if (arena_remaining(self) + old_size < new_size) { return rk_null; }
  if rk_unlikely (!self->cur) { return rk_null; }
  unsigned char* ptr = self->cur - old_size;
  self->cur          = ptr + new_size;
  return ptr;
}

rklib_fun rk_alloc_size(2) void* arena_resize_top(size_t old_size, size_t new_size, Arena* self) {
  void* res = arena_try_resize_top(old_size, new_size, self);
  RK_ARENA_FAIL(res, self, (self->cur ? self->cur - old_size : rk_null), align_max, new_size);
  return res;
}

rklib_fun rk_alloc_alignsize(2, 1) void* rki_arena_allocate(size_t nbytes, size_t align,
                                                            void* ctx) {
  void* ptr = arena_try_allocate(nbytes, align, (Arena*)ctx);
  RK_ARENA_FAIL(ptr, (Arena*)ctx, rk_null, align, nbytes);
  return ptr;
}

rklib_fun void rki_arena_deallocate(void* ptr, size_t old_size, size_t align rk_unused, void* ctx) {
  if (arena_is_top_allocation((Arena*)ctx, ptr, old_size)) {
    (void)arena_try_resize_top(old_size, 0, (Arena*)ctx);
  }
}

rklib_fun rk_alloc_alignsize(4, 3) void* rki_arena_reallocate(void* ptr, size_t old_size,
                                                              size_t new_size, size_t align,
                                                              void* ctx) {
  rk_assert_align_pow2(align);
  Arena* self = (Arena*)ctx;

  if ((arena_is_top_allocation(self, ptr, old_size)
       && arena_try_resize_top(old_size, new_size, self))
      || new_size <= old_size) {
    return ptr;
  }

  void* res = rki_arena_allocate(new_size, align, self);
  rk_memcpy(res, ptr, old_size);
  return res;
}

rklib_fun rk_alloc_alignsize(2, 1) void* arena_allocate(size_t nbytes, size_t align, Arena* self) {
  return rki_arena_allocate(nbytes, align, self);
}
rklib_fun rk_alloc_size(3) void* arena_try_resize(void* ptr, size_t old_size, size_t new_size,
                                                  Arena* self) {
  if (!arena_is_top_allocation(self, ptr, old_size)) { return rk_null; }
  return arena_try_resize_top(old_size, new_size, self);
}

rklib_fun rk_alloc_size(3) void* rki_arena_extend(void* ptr, size_t old_size, size_t new_size,
                                                  Arena* self) {
  void* r = arena_try_resize(ptr, old_size, new_size, self);
  RK_ARENA_FAIL(r, self, ptr, align_max, new_size);
  return r;
}

#define RKI_ARENA_INIT_STATIC(arr)                                                                 \
  {.beg = (arr) + rk_ensure_valid_storage_type(arr), .cur = (arr), .end = (arr) + sizeof(arr)}

#define RKI_ARENA_ALIGNED_NEW(T, count, align, arena)                                              \
  ((typeof(T)*)(alloc_log_new(), rk_assert_valid_align(T, align),                                  \
                arena_allocate(sizeof_n(T, count), align, arena)))
#define RKI_ARENA_NEW(T, count, arena)                                                             \
  ((typeof(T)*)(alloc_log_new(), arena_allocate(sizeof_n(T, count), alignof(T), arena)))

static_fun rk_const Allocator arena_to_alloc(Arena* arena) {
  return (Allocator)arena_to_alloc_static(arena);
}

RKI_IGNWARN_CLANG_END()

/// @endcond
#pragma endregion implementation
RKI_HEADER_END
/* END DEFERRED IMPLEMENTATION: include/rk_arena.h */
#endif
#ifdef RKI_AMALG_IMPL_253EB59549665B16
#undef RKI_AMALG_IMPL_253EB59549665B16
/* BEGIN DEFERRED IMPLEMENTATION: include/rk_vec.h */
RKI_HEADER_BEGIN
#pragma region implementation
////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

/// @brief Header of a dynamically allocated Vec. The Vec is implemented as a contiguous block of
/// memory with a header (`RKI_VecHdr`) that stores metadata about the vec, such as its capacity,
/// length, and allocator.
typedef struct RKI_VecHdr {
#if RK_CUSTOM_ALLOCATORS
  Allocator alloc; ///< Allocator (can be disabled)
#endif
  size_t                    cap;    ///< Capacity of the Vec (in terms of elements)
  size_t                    count;  ///< Length of the Vec (in terms of elements)
  alignas_max unsigned char data[]; ///< Vec Data
} RKI_VecHdr;

#define RKI_VEC_HDR(self)                                                                          \
  ((RKI_VecHdr*)(void*)((char*)(self)                                                              \
                        - offsetof(RKI_VecHdr,                                                     \
                                   data)))             // NOLINT(clang-analyzer-security.ArrayBound)
#define RKI_VEC_CAP(self)   (RKI_VEC_HDR(self)->cap)   // NOLINT(clang-analyzer-security.ArrayBound)
#define RKI_VEC_COUNT(self) (RKI_VEC_HDR(self)->count) // NOLINT(clang-analyzer-security.ArrayBound)
#define RKI_VEC_ALLOCATOR(self)                                                                    \
  RKI_allocatorof(RKI_VEC_HDR(self)) // NOLINT(clang-analyzer-security.ArrayBound)

rklib_fun rk_pure size_t vec_count(const Vec(void) self) { return self ? RKI_VEC_COUNT(self) : 0; }
rklib_fun rk_pure size_t vec_len(const Vec(void) self) { return vec_count(self); }
rklib_fun rk_pure size_t vec_cap(const Vec(void) self) { return self ? RKI_VEC_CAP(self) : 0; }
rklib_fun rk_pure bool   vec_is_empty(const Vec(void) self) { return vec_count(self) == 0; }
rklib_fun rk_pure Allocator vec_allocator(const Vec(void) self) {
  return self ? vec_ALLOCATOR(self) : alloc_ctx;
}
rklib_fun void vec_clear(Vec(void) self) {
  if (self) { RKI_VEC_COUNT(self) = 0; }
}
rklib_fun rk_pure size_t vec_remaining(const Vec(void) self) {
  return self ? RKI_VEC_CAP(self) - RKI_VEC_COUNT(self) : 0;
}

rklib_fun rk_pure bool vec_index_in_range(const Vec(void) self, size_t idx) {
  return idx < vec_count(self);
}

rklib_fun rk_forceinline size_t rki_decrease_index_check(void* self) {
  rk_assert(vec_count(self) && "Cannot decrease count of empty vec");
  return --RKI_VEC_COUNT(self);
}

rklib_fun rk_forceinline void* rki_check_vec_push_u(void* self) {
  rk_assert(vec_remaining(self) && "Not enough capacity for unchecked push");
  return self;
}

#define RKI_VEC_TRY_POP(self, out)                                                                 \
  ((bool)(vec_count(self) ? (*(out) = (self)[--RKI_VEC_COUNT(self)], true) : false))

rklib_fun rk_forceinline void* rki_check_vec_pop(void* self) {
  rk_assert(vec_count(self) && "Attempting to pop from zero-length vec");
  return self;
}

rklib_fun rk_forceinline void* rki_vec_pop_n(size_t elsize, void* self, size_t count) {
  rk_assert(vec_count(self) >= count && "Attempting to pop more than vec_count() elements");
  RKI_VEC_COUNT(self) -= count;
  return (char*)self + rk_mult(elsize, RKI_VEC_COUNT(self));
}

// Like the peek helpers, these take and return const void* so const element types pass through;
// the accessor macros cast the result back to typeof(self), restoring the caller's constness.
rklib_fun rk_forceinline const void* rki_vec_check_front(const void* self) {
  rk_assert(vec_count(self) && "Attempting to access front of zero-sized vec");
  return self;
}

rklib_fun rk_forceinline const void* rki_vec_check_back(size_t elsize, const void* self) {
  rk_assert(vec_count(self) && "Attempting to access back of zero-sized vec");
  return (const char*)self + rk_mult(elsize, vec_count(self) - 1);
}

rklib_fun rk_forceinline const void* rki_vec_check_at(size_t elsize, const void* self, size_t idx) {
  rk_assert(idx < vec_count(self) && "Access out of bounds of Vec.");
  return (const char*)self + rk_mult(elsize, idx);
}

rklib_fun rk_forceinline const void* rki_vec_peek_at(size_t elsize, const void* self, size_t idx) {
  return idx < vec_count(self) ? (const char*)self + rk_mult(elsize, idx) : rk_null;
}

rklib_fun rk_forceinline const void* rki_vec_peek_back(size_t elsize, const void* self) {
  size_t count = vec_count(self);
  return count ? (const char*)self + rk_mult(elsize, count - 1) : rk_null;
}

rklib_fun rk_forceinline size_t rki_vec_assert_insertbounds(void* self, size_t i) {
  rk_assert(i <= vec_count(self) && "Attempting to insert into Vec at out of bounds index");
  return i;
}

rklib_fun rk_forceinline size_t rki_vec_assert_erasebounds_n(void* self, size_t i, size_t n) {
  rk_assert((i <= vec_count(self) && n <= vec_count(self) - i)
            && "Attempting to erase from Vec at out of bounds index");
  return i;
}

//  logical size of a vec if type information not available
#define RKI_VECSIZE_UT(elsize, elcount) (offsetof(RKI_VecHdr, data) + rk_mult(elsize, elcount))
#define RKI_VEC_COMPUTE_SIZE(V, C)      RKI_VECSIZE_UT(sizeof(*(V)), C)
#define RKI_VEC_ALLOCSIZE(V)            RKI_VEC_COMPUTE_SIZE(V, RKI_VEC_CAP(V))

#define RKI_VEC_ALLOCATION_SIZE(self)   rki_vec_allocation_size(self, sizeof(*(self)))
rklib_fun rk_forceinline rk_pure size_t rki_vec_allocation_size(const Vec(void) self,
                                                                size_t          elsize) {
  return self ? RKI_VECSIZE_UT(elsize, RKI_VEC_CAP(self)) : 0;
}

rklib_fun rk_forceinline RKI_VecHdr* rk_alloc_size(2)
    rki_vec_init(size_t init_cap, size_t total_size,
                 size_t init_count RK_IFALLOC(, Allocator alloc)) {
  RKI_assert_allocator_valid(alloc);
  RKI_VecHdr* v = (RKI_VecHdr*)alloc_allocate(total_size, align_max RK_IFALLOC(, alloc));
  v->cap = init_cap, v->count = init_count;
  RK_IFALLOC(v->alloc = alloc;)
  return v;
}
#define RKI_VEC_NEW_NONZERO(T, cap, count, alloc)                                                  \
  ((typeof(T)*)(void*)(rki_vec_init(cap, offsetof(RKI_VecHdr, data) + sizeof_n(T, cap),            \
                                    count RK_IFALLOC(, alloc))                                     \
                           ->data))
// Invariant: a non-NULL Vec always has capacity >= 1, so growth by doubling never needs a zero
// check. Constructors round a requested capacity of 0 up to 1 (keeping the allocator bound to the
// Vec), and shrinking an empty Vec keeps one slot; only vec_release() returns a Vec to NULL.
rklib_fun rk_const rk_forceinline size_t rki_vec_cap_min1(size_t cap) { return cap ? cap : 1; }
#define RKI_VEC_NEW(T, cap, count, alloc)                                                          \
  RKI_VEC_NEW_NONZERO(T, rki_vec_cap_min1(cap), count, alloc)

// initialises a Vec with positive cap (no cap 0 check) and assigns it to V
#define RKI_VEC_INIT_ASSIGN_NONZERO(V, C, A) ((V) = RKI_VEC_NEW_NONZERO(*(V), (C), 0, (A)))

#define RKI_VEC_INIT(T, C, A)                RKI_VEC_NEW(T, C, 0, A)
#define RKI_VEC_INIT3(T, C, A)               RKI_REQUIRE_CUSTOM_ALLOCATORS(RKI_VEC_INIT(T, C, A))
#define RKI_VEC_INIT2(T, C)                  RKI_VEC_INIT(T, C, alloc_ctx)

#define RKI_VEC_FROM(arr, count, alloc)                                                            \
  rk_copy(RKI_VEC_NEW(*(arr), (count), (count), (alloc)), (arr), (count))
#define RKI_VEC_FROM3(arr, count, alloc)                                                           \
  RKI_REQUIRE_CUSTOM_ALLOCATORS(RKI_VEC_FROM(arr, count, alloc))
#define RKI_VEC_FROM2(arr, count) RKI_VEC_FROM(arr, count, alloc_ctx)

#define RKI_VEC_RELEASE(V)                                                                         \
  ((V)                                                                                             \
   && (alloc_deallocate(RKI_VEC_HDR(V), RKI_VEC_ALLOCSIZE(V),                                      \
                        align_max RK_IFALLOC(, RKI_VEC_ALLOCATOR(V))),                             \
       (V) = rk_null))

/// always reallocates to a positive cap, sets cap accordingly
#define RKI_VEC_CHANGE_CAP(V, C)                                                                   \
  ((V) = (typeof(V))(void*)(((RKI_VecHdr*)alloc_reallocate(                                        \
                                 RKI_VEC_HDR(V), RKI_VEC_ALLOCSIZE(V), RKI_VEC_COMPUTE_SIZE(V, C), \
                                 align_max RK_IFALLOC(, RKI_VEC_ALLOCATOR(V))))                    \
                                ->data),                                                           \
   RKI_VEC_CAP(V) = (C), (V))

/*doubles capacity if at limit*/
#define RKI_VEC_RESERVE_1(V)                                                                       \
  ((V) ? (RKI_VEC_COUNT(V) == RKI_VEC_CAP(V) ? RKI_VEC_CHANGE_CAP(V, rk_mult(RKI_VEC_CAP(V), 2))   \
                                             : (V))                                                \
       : RKI_VEC_INIT_ASSIGN_NONZERO(V, 1, alloc_ctx))

#define RKI_VEC_RESERVE(V, C)                                                                      \
  ((V) ? ((C) > RKI_VEC_CAP(V) ? RKI_VEC_CHANGE_CAP(V, C) : (V))                                   \
       : ((C) ? RKI_VEC_INIT_ASSIGN_NONZERO(V, C, alloc_ctx) : rk_null))

#define RKI_VEC_RESIZE(V, C) (RKI_VEC_RESERVE(V, C), (V) && (RKI_VEC_COUNT(V) = (C)))

// Shrinking never releases: an empty Vec keeps one slot, preserving its allocator and the
// capacity >= 1 invariant. stdc_bit_ceil(0) is 1, so SHRINK_TO_FIT needs no special case.
#define RKI_VEC_SHRINK_TO_FIT_EXACT(V)                                                             \
  ((V) && rki_vec_cap_min1(RKI_VEC_COUNT(V)) < RKI_VEC_CAP(V)                                      \
   && RKI_VEC_CHANGE_CAP(V, rki_vec_cap_min1(RKI_VEC_COUNT(V))))

#define RKI_VEC_SHRINK_TO_FIT(V)                                                                   \
  ((V) && stdc_bit_ceil(RKI_VEC_COUNT(V)) < RKI_VEC_CAP(V)                                         \
   && RKI_VEC_CHANGE_CAP(V, stdc_bit_ceil(RKI_VEC_COUNT(V))))

#define RKI_VEC_PUSH_U(V, O)      ((V)[RKI_VEC_COUNT(rki_check_vec_push_u(V))++] = (O))
#define RKI_VEC_PUSH(V, O)        (RKI_VEC_RESERVE_1(V), RKI_VEC_PUSH_U(V, O))

#define RKI_VEC_PUSH_N_U(V, O, N) (rk_copy((V) + RKI_VEC_COUNT(V), O, N), RKI_VEC_COUNT(V) += (N))
#define RKI_VEC_PUSH_N(V, O, N)                                                                    \
  ((void)((N) && (RKI_VEC_RESERVE(V, vec_count(V) + (N)), RKI_VEC_PUSH_N_U(V, O, N), 1)))

rklib_fun rk_forceinline void rki_vec_insert_arr_at(size_t elsize, void* restrict v, size_t i,
                                                    const void* restrict arr, size_t n) {
  size_t old_count = RKI_VEC_COUNT(v); // NOLINT(clang-analyzer-security.ArrayBound)
  char * src = (char*)v + rk_mult(i, elsize), *dst = src + rk_mult(n, elsize);
  memmove(dst, src, rk_mult(old_count - i, elsize));
  memcpy(src, arr, rk_mult(elsize, n));
  RKI_VEC_COUNT(v) = old_count + n; // NOLINT(clang-analyzer-security.ArrayBound)
}

#define RKI_VEC_INSERT_ARR_AT_U(V, I, O, N)                                                        \
  rki_vec_insert_arr_at(sizeof(*(V)), (V), rki_vec_assert_insertbounds(V, I), (O), (N))
#define RKI_VEC_INSERT_AT_U(V, I, O) RKI_VEC_INSERT_ARR_AT_U(V, I, ((typeof (*(V))[1]){(O)}), 1)

#define RKI_VEC_INSERT_AT(V, I, O)   (RKI_VEC_RESERVE_1(V), RKI_VEC_INSERT_AT_U(V, I, O))

#define RKI_VEC_INSERT_ARR_AT(V, I, O, N)                                                          \
  ((void)((N) && (RKI_VEC_RESERVE(V, vec_count(V) + (N)), RKI_VEC_INSERT_ARR_AT_U(V, I, O, N), 1)))

rklib_fun rk_forceinline void rki_vec_insert_at_unordered(size_t elsize, void* restrict vec,
                                                          size_t i, const void* restrict o) {
  size_t count = RKI_VEC_COUNT(vec); // NOLINT(clang-analyzer-security.ArrayBound)
  char*  dst   = (char*)vec + rk_mult(elsize, count);
  if (i < count) {
    char* src = (char*)vec + rk_mult(elsize, i);
    memcpy(dst, src, elsize);
    memcpy(src, o, elsize);
  } else {
    memcpy(dst, o, elsize);
  }
  ++RKI_VEC_COUNT(vec); // NOLINT(clang-analyzer-security.ArrayBound)
}

#define RKI_VEC_INSERT_AT_UNORDERED(V, I, O)                                                       \
  (RKI_VEC_RESERVE_1(V),                                                                           \
   rki_vec_insert_at_unordered(sizeof(*(V)), V, rki_vec_assert_insertbounds(V, I),                 \
                               ((typeof (*(V))[1]){(O)})))

rklib_fun rk_forceinline void rki_vec_erase_at_n(size_t elsize, void* v, size_t i, size_t n) {
  if (!n) { return; }
  char *dst = (char*)v + rk_mult(i, elsize), *src = dst + rk_mult(n, elsize);
  memmove(dst, src,
          rk_mult(((RKI_VEC_COUNT(v) -= n) - i),
                  elsize)); // NOLINT(clang-analyzer-security.ArrayBound)
}
#define RKI_VEC_ERASE_AT_N(V, I, N)                                                                \
  rki_vec_erase_at_n(sizeof(*(V)), (V), rki_vec_assert_erasebounds_n(V, I, N), (N))
#define RKI_VEC_ERASE_AT(V, I) RKI_VEC_ERASE_AT_N(V, I, 1)

#define RKI_VEC_ASSIGN(V, O, N)                                                                    \
  (RKI_VEC_RESERVE(V, N), (V) && (RKI_VEC_COUNT(V) = (N), rk_copy(V, O, N)))

#define RKI_VEC_FOREACH(vec, it)                                                                   \
  for (typeof(*(vec))*it = (vec), *const rki_var_end = vec_end(it); it != rki_var_end; ++it)

#define RKI_VEC_FOREACH_REVERSED(vec, it)                                                          \
  for (typeof(*(vec))*const rki_var_begin = (vec), *it = vec_end(rki_var_begin);                   \
       it != rki_var_begin && (--it, 1);)

#define RKI_VEC_ERASE_IF(vec, it, pred)                                                            \
  do {                                                                                             \
    RKI_IGNWARN_MSC_BEG(4114)                                                                      \
    typeof(vec) rki_var_vec = (vec);                                                               \
    if (!vec_count(rki_var_vec)) { break; }                                                        \
    typeof(*rki_var_vec)*rki_var_write = rki_var_vec, *const rki_var_end                           \
                                                      = rki_var_write                              \
                                                      + RKI_VEC_COUNT(rki_var_write);              \
    for (typeof(*rki_var_vec)* rki_var_it = rki_var_vec; rki_var_it != rki_var_end;                \
         ++rki_var_it) {                                                                           \
      typeof(*rki_var_vec)* const it = rki_var_it;                                                 \
      if (!(pred)) { *rki_var_write++ = *rki_var_it; }                                             \
    }                                                                                              \
    RKI_VEC_COUNT(rki_var_vec) = (size_t)(rki_var_write - rki_var_vec);                            \
    RKI_IGNWARN_MSC_END()                                                                          \
  } while (0)

// msvc sizeof returns 0
#define RKI_VEC_INIT_LIST_(T, arr, alloc)                                                          \
  memcpy(RKI_VEC_NEW_NONZERO(T, rk_COUNTOF(arr), rk_COUNTOF(arr), alloc), arr, sizeof(arr))
#define RKI_VEC_CONTRAV(T, x) _Generic((x), T: x, Allocator: (T){RKI_ZINIT})
#if RK_CUSTOM_ALLOCATORS
# define RKI_VEC_INIT_LIST(T, ...)                                                                 \
   _Generic((VA_FIRST(__VA_ARGS__)),                                                               \
       Allocator: RKI_VEC_INIT_LIST_(T, ((const T[]){VA_REST(__VA_ARGS__)}),                       \
                                     RKI_contrav(Allocator, VA_FIRST(__VA_ARGS__))),               \
       default: RKI_VEC_INIT_LIST_(                                                                \
                T, ((const T[]){RKI_VEC_CONTRAV(T, VA_FIRST(__VA_ARGS__)), VA_REST(__VA_ARGS__)}), \
                alloc_ctx))
#else
# define RKI_VEC_INIT_LIST(T, ...)                                                                 \
   RKI_VEC_INIT_LIST_(                                                                             \
       T, ((const T[]){RKI_VEC_CONTRAV(T, VA_FIRST(__VA_ARGS__)), VA_REST(__VA_ARGS__)}), 0)
#endif

/// @endcond
#pragma endregion implementation
RKI_HEADER_END
/* END DEFERRED IMPLEMENTATION: include/rk_vec.h */
#endif
#ifdef RKI_AMALG_IMPL_C643CC5EB236C471
#undef RKI_AMALG_IMPL_C643CC5EB236C471
/* BEGIN DEFERRED IMPLEMENTATION: include/rk_heap.h */
RKI_HEADER_BEGIN
#pragma region implementation
////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

#define RKI_HEAP_PUB(T, FNAME) heap_##T##_##FNAME
#define RKI_HEAP_PRI(T, FNAME) rki_heap_##T##_##FNAME

#define RKI_HEAP_DEFINE(T, CMP_FUN)                                                                \
  RK_EXTERNC_BEG                                                                                   \
  typedef struct Heap(T) { Vec(T) data; } Heap(T);                                                 \
  rklib_fun rk_pure size_t RKI_HEAP_PUB(T, count)(const Heap(T) * self) {                          \
    return vec_count(self->data);                                                                  \
  }                                                                                                \
  rklib_fun rk_pure size_t RKI_HEAP_PUB(T, cap)(const Heap(T) * self) {                            \
    return vec_cap(self->data);                                                                    \
  }                                                                                                \
  rklib_fun rk_pure bool RKI_HEAP_PUB(T, is_empty)(const Heap(T) * self) {                         \
    return vec_count(self->data) == 0;                                                             \
  }                                                                                                \
  rklib_fun rk_pure Allocator RKI_HEAP_PUB(T, allocator)(const Heap(T) * self) {                   \
    return vec_allocator(self->data);                                                              \
  }                                                                                                \
  rklib_fun const T* RKI_HEAP_PUB(T, top_ptr)(const Heap(T) * self) {                              \
    rk_assert(heap_count(self) && "Cannot access top of empty heap");                              \
    return self->data;                                                                             \
  }                                                                                                \
  rklib_fun rk_pure const T* RKI_HEAP_PUB(T, peek)(const Heap(T) * self) {                         \
    return vec_count(self->data) ? &self->data[0] : rk_null;                                       \
  }                                                                                                \
  rklib_fun void RKI_HEAP_PUB(T, release)(Heap(T) * self) { vec_release(self->data); }             \
  rklib_fun void RKI_HEAP_PUB(T, clear)(Heap(T) * self) { vec_clear(self->data); }                 \
  rklib_fun void RKI_HEAP_PUB(T, reserve)(Heap(T) * self, size_t cap) {                            \
    vec_reserve(self->data, cap);                                                                  \
  }                                                                                                \
  rklib_fun void RKI_HEAP_PUB(T, shrink_to_fit)(Heap(T) * self) { vec_shrink_to_fit(self->data); } \
  rklib_fun void RKI_HEAP_PRI(T, sift_down)(Heap(T) * self, size_t i, T value, size_t n) {         \
    while (i < n / 2) {                                                                            \
      size_t child = 2 * i + 1;                                                                    \
      if (child + 1 < n && CMP_FUN(self->data[child + 1], self->data[child]) < 0) { ++child; }     \
      if (CMP_FUN(value, self->data[child]) <= 0) { break; }                                       \
      self->data[i] = self->data[child];                                                           \
      i             = child;                                                                       \
    }                                                                                              \
    self->data[i] = value;                                                                         \
  }                                                                                                \
  rklib_fun void RKI_HEAP_PRI(T, heapify)(Heap(T) * self) {                                        \
    const size_t n = vec_count(self->data);                                                        \
    for (size_t i = n / 2; i > 0;) {                                                               \
      --i;                                                                                         \
      RKI_HEAP_PRI(T, sift_down)(self, i, self->data[i], n);                                       \
    }                                                                                              \
  }                                                                                                \
  rklib_fun Heap(T) RKI_HEAP_PUB(T, from)(const T* arr, size_t n RK_IFALLOC(, Allocator alloc)) {  \
    Heap(T) h = heap_init(T, n RK_IFALLOC(, alloc));                                               \
    vec_push_n(h.data, arr, n);                                                                    \
    RKI_HEAP_PRI(T, heapify)(&h);                                                                  \
    return h;                                                                                      \
  }                                                                                                \
  rklib_fun Heap(T) RKI_HEAP_PUB(T, adopt)(Vec(T) vec) {                                           \
    Heap(T) h = {vec};                                                                             \
    RKI_HEAP_PRI(T, heapify)(&h);                                                                  \
    return h;                                                                                      \
  }                                                                                                \
  rklib_fun void RKI_HEAP_PUB(T, assign)(Heap(T) * self, const T* arr, size_t n) {                 \
    vec_assign(self->data, arr, n);                                                                \
    RKI_HEAP_PRI(T, heapify)(self);                                                                \
  }                                                                                                \
  rklib_fun void RKI_HEAP_PUB(T, extend)(Heap(T) * self, const T* arr, size_t n) {                 \
    vec_push_n(self->data, arr, n);                                                                \
    RKI_HEAP_PRI(T, heapify)(self);                                                                \
  }                                                                                                \
  rklib_fun void RKI_HEAP_PUB(T, push)(Heap(T) * self, T value) {                                  \
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
  rklib_fun T RKI_HEAP_PUB(T, pop)(Heap(T) * self) {                                               \
    rk_assert(vec_count(self->data) && "Cannot pop an empty heap");                                \
    const T      result = self->data[0], last = vec_pop(self->data);                               \
    const size_t n = vec_count(self->data);                                                        \
    if (n) { RKI_HEAP_PRI(T, sift_down)(self, 0, last, n); }                                       \
    return result;                                                                                 \
  }                                                                                                \
  rklib_fun bool RKI_HEAP_PUB(T, try_pop)(Heap(T) * self, T * out) {                               \
    if (!vec_count(self->data)) { return false; }                                                  \
    return *out = RKI_HEAP_PUB(T, pop)(self), true;                                                \
  }                                                                                                \
  rklib_fun T RKI_HEAP_PUB(T, replace_top)(Heap(T) * self, T value) {                              \
    rk_assert(vec_count(self->data) && "Cannot replace_top an empty heap");                        \
    const T result = self->data[0];                                                                \
    RKI_HEAP_PRI(T, sift_down)(self, 0, value, vec_count(self->data));                             \
    return result;                                                                                 \
  }                                                                                                \
  RK_EXTERNC_END

#define RKI_HEAP_FROM(T, arr, n, alloc) RKI_HEAP_PUB(T, from)((arr), (n)RK_IFALLOC(, (alloc)))
#define RKI_HEAP_FROM4(T, arr, n, alloc)                                                           \
  RKI_REQUIRE_CUSTOM_ALLOCATORS(RKI_HEAP_FROM(T, arr, n, alloc))
#define RKI_HEAP_FROM3(T, arr, n) RKI_HEAP_FROM(T, arr, n, alloc_ctx)

#define RKI_HEAP_ERASE_IF(T, self, it, pred)                                                       \
  do {                                                                                             \
    Heap(T)* const rki_var_heap = (self);                                                          \
    if (!rki_var_heap) { break; }                                                                  \
    const size_t rki_var_count = heap_count(rki_var_heap);                                         \
    vec_erase_if(rki_var_heap->data, it, pred);                                                    \
    if (heap_count(rki_var_heap) != rki_var_count) { RKI_HEAP_PRI(T, heapify)(rki_var_heap); }     \
  } while (0)

/// @endcond
#pragma endregion implementation
RKI_HEADER_END
/* END DEFERRED IMPLEMENTATION: include/rk_heap.h */
#endif
#ifdef RKI_AMALG_IMPL_E739244C6328D655
#undef RKI_AMALG_IMPL_E739244C6328D655
/* BEGIN DEFERRED IMPLEMENTATION: include/rk_deque.h */
RKI_HEADER_BEGIN
#pragma region implementation
////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

#define RKI_DEQUE_PUB(K, FNAME) deque_##K##_##FNAME
#define RKI_DEQUE_PRI(K, FNAME) rki_deque_##K##_##FNAME

// The member `data` is a mutable pointer even when the Deque is const; propagate the
// container's constness explicitly when choosing an iterator pointer type. Constness is detected
// through `count`, a member of a named type: matching on `const typeof(*(self))*` would spell
// `const const Deque` for a const Deque (MSVC C4114).
#define RKI_DEQUE_ITER_PTR(self)                                                                   \
  _Generic(&(self)->count,                                                                         \
      const size_t*: (const typeof((self)->data[0])*)0,                                            \
      default: (typeof((self)->data))0)
#define RKI_DEQUE_DEFINE(T)                                                                        \
  RK_EXTERNC_BEG                                                                                   \
  typedef struct Deque(T) {                                                                        \
    T*     data;                                                                                   \
    size_t head, count, cap;                                                                       \
    RK_IFALLOC(Allocator alloc;)                                                                   \
  } Deque(T);                                                                                      \
  rklib_fun size_t rk_pure RKI_DEQUE_PUB(T, count)(const Deque(T) * self) { return self->count; }  \
  rklib_fun size_t rk_pure RKI_DEQUE_PUB(T, cap)(const Deque(T) * self) { return self->cap; }      \
  rklib_fun bool rk_pure   RKI_DEQUE_PUB(T, is_empty)(const Deque(T) * self) {                     \
    return !self->count;                                                                           \
  }                                                                                                \
  rklib_fun Allocator rk_pure RKI_DEQUE_PUB(T, allocator)(const Deque(T) * self) {                 \
    return RKI_allocatorof(self);                                                                  \
  }                                                                                                \
                                                                                                   \
  rklib_fun Deque(T) RKI_DEQUE_PUB(T, init)(size_t cap RK_IFALLOC(, Allocator alloc)) {            \
    RK_IFALLOC(RKI_assert_allocator_valid(alloc);)                                                 \
    Deque(T) result = {rk_null, 0, 0, 0 RK_IFALLOC(, alloc)};                                      \
    if (cap) {                                                                                     \
      cap = stdc_bit_ceil(rk_MAX((size_t)8, cap));                                                 \
      rk_assert(cap && "Deque capacity overflow");                                                 \
      result.data = alloc_new(T, cap RK_IFALLOC(, alloc));                                         \
      result.cap  = cap;                                                                           \
    }                                                                                              \
    return result;                                                                                 \
  }                                                                                                \
  rklib_fun void RKI_DEQUE_PUB(T, release)(Deque(T) * self) {                                      \
    if (self->data) { alloc_delete(self->data, self->cap RK_IFALLOC(, self->alloc)); }             \
    self->data = rk_null, self->head = self->count = self->cap = 0;                                \
  }                                                                                                \
  rklib_fun void RKI_DEQUE_PUB(T, clear)(Deque(T) * self) { self->head = self->count = 0; }        \
  rklib_fun void RKI_DEQUE_PRI(T, realloc_to)(Deque(T) * self, size_t new_cap) {                   \
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
  rklib_fun void RKI_DEQUE_PUB(T, reserve)(Deque(T) * self, size_t requested) {                    \
    if (requested <= self->cap) { return; }                                                        \
    size_t cap = stdc_bit_ceil(rk_MAX((size_t)8, requested));                                      \
    rk_assert(cap && "Deque capacity overflow");                                                   \
    RK_IFALLOC(RKI_set_alloc_fallback(self->alloc);)                                               \
    RKI_DEQUE_PRI(T, realloc_to)(self, cap);                                                       \
  }                                                                                                \
  rklib_fun void RKI_DEQUE_PUB(T, shrink_to_fit)(Deque(T) * self) {                                \
    if (!self->count) {                                                                            \
      RKI_DEQUE_PUB(T, release)(self);                                                             \
      return;                                                                                      \
    }                                                                                              \
    size_t cap = stdc_bit_ceil(rk_MAX((size_t)8, self->count));                                    \
    if (cap == self->cap) { return; }                                                              \
    RK_IFALLOC(RKI_set_alloc_fallback(self->alloc);)                                               \
    RKI_DEQUE_PRI(T, realloc_to)(self, cap);                                                       \
  }                                                                                                \
  rklib_fun T* RKI_DEQUE_PUB(T, at)(Deque(T) * self, size_t i) {                                   \
    rk_assert(i < self->count && "Access out of bounds of Deque.");                                \
    return &self->data[(self->head + i) & (self->cap - 1)];                                        \
  }                                                                                                \
  rklib_fun const T* RKI_DEQUE_PUB(T, at_const)(const Deque(T) * self, size_t i) {                 \
    rk_assert(i < self->count && "Access out of bounds of Deque.");                                \
    return &self->data[(self->head + i) & (self->cap - 1)];                                        \
  }                                                                                                \
  rklib_fun T* RKI_DEQUE_PUB(T, peek_at)(Deque(T) * self, size_t i) {                              \
    return i < self->count ? &self->data[(self->head + i) & (self->cap - 1)] : rk_null;            \
  }                                                                                                \
  rklib_fun const T* RKI_DEQUE_PUB(T, peek_at_const)(const Deque(T) * self, size_t i) {            \
    return i < self->count ? &self->data[(self->head + i) & (self->cap - 1)] : rk_null;            \
  }                                                                                                \
  rklib_fun T* RKI_DEQUE_PUB(T, peek_front)(Deque(T) * self) {                                     \
    return self->count ? &self->data[self->head] : rk_null;                                        \
  }                                                                                                \
  rklib_fun const T* RKI_DEQUE_PUB(T, peek_front_const)(const Deque(T) * self) {                   \
    return self->count ? &self->data[self->head] : rk_null;                                        \
  }                                                                                                \
  rklib_fun T* RKI_DEQUE_PUB(T, peek_back)(Deque(T) * self) {                                      \
    return self->count ? &self->data[(self->head + self->count - 1) & (self->cap - 1)] : rk_null;  \
  }                                                                                                \
  rklib_fun const T* RKI_DEQUE_PUB(T, peek_back_const)(const Deque(T) * self) {                    \
    return self->count ? &self->data[(self->head + self->count - 1) & (self->cap - 1)] : rk_null;  \
  }                                                                                                \
  rklib_fun T* RKI_DEQUE_PUB(T, front)(Deque(T) * self) {                                          \
    rk_assert(self->count && "Cannot access front of empty deque");                                \
    return &self->data[self->head];                                                                \
  }                                                                                                \
  rklib_fun const T* RKI_DEQUE_PUB(T, front_const)(const Deque(T) * self) {                        \
    rk_assert(self->count && "Cannot access front of empty deque");                                \
    return &self->data[self->head];                                                                \
  }                                                                                                \
  rklib_fun T* RKI_DEQUE_PUB(T, back)(Deque(T) * self) {                                           \
    rk_assert(self->count && "Cannot access back of empty deque");                                 \
    return &self->data[(self->head + self->count - 1) & (self->cap - 1)];                          \
  }                                                                                                \
  rklib_fun const T* RKI_DEQUE_PUB(T, back_const)(const Deque(T) * self) {                         \
    rk_assert(self->count && "Cannot access back of empty deque");                                 \
    return &self->data[(self->head + self->count - 1) & (self->cap - 1)];                          \
  }                                                                                                \
  rklib_fun void RKI_DEQUE_PUB(T, push_front)(Deque(T) * self, T value) {                          \
    if (self->count == self->cap) {                                                                \
      rk_assert(self->cap <= SIZE_MAX / 2 && "Deque capacity overflow");                           \
      RKI_DEQUE_PUB(T, reserve)(self, self->cap ? self->cap * 2 : 8);                              \
    }                                                                                              \
    self->head             = (self->head - 1) & (self->cap - 1);                                   \
    self->data[self->head] = value;                                                                \
    ++self->count;                                                                                 \
  }                                                                                                \
  rklib_fun void RKI_DEQUE_PUB(T, push_back)(Deque(T) * self, T value) {                           \
    if (self->count == self->cap) {                                                                \
      rk_assert(self->cap <= SIZE_MAX / 2 && "Deque capacity overflow");                           \
      RKI_DEQUE_PUB(T, reserve)(self, self->cap ? self->cap * 2 : 8);                              \
    }                                                                                              \
    self->data[(self->head + self->count) & (self->cap - 1)] = value;                              \
    ++self->count;                                                                                 \
  }                                                                                                \
  rklib_fun T RKI_DEQUE_PUB(T, pop_front)(Deque(T) * self) {                                       \
    rk_assert(self->count && "Cannot pop an empty deque");                                         \
    T result   = self->data[self->head];                                                           \
    self->head = (self->head + 1) & (self->cap - 1);                                               \
    if (!--self->count) { self->head = 0; }                                                        \
    return result;                                                                                 \
  }                                                                                                \
  rklib_fun T RKI_DEQUE_PUB(T, pop_back)(Deque(T) * self) {                                        \
    rk_assert(self->count && "Cannot pop an empty deque");                                         \
    T result = self->data[(self->head + self->count - 1) & (self->cap - 1)];                       \
    if (!--self->count) { self->head = 0; }                                                        \
    return result;                                                                                 \
  }                                                                                                \
  rklib_fun bool RKI_DEQUE_PUB(T, try_pop_front)(Deque(T) * self, T * out) {                       \
    return self->count ? (*out = RKI_DEQUE_PUB(T, pop_front)(self), true) : false;                 \
  }                                                                                                \
  rklib_fun bool RKI_DEQUE_PUB(T, try_pop_back)(Deque(T) * self, T * out) {                        \
    return self->count ? (*out = RKI_DEQUE_PUB(T, pop_back)(self), true) : false;                  \
  }                                                                                                \
  rklib_fun void RKI_DEQUE_PUB(T, push_front_n)(Deque(T) * self, const T* arr, size_t n) {         \
    if (!n) { return; }                                                                            \
    rk_assert(n <= SIZE_MAX - self->count && "Deque capacity overflow");                           \
    RKI_DEQUE_PUB(T, reserve)(self, self->count + n);                                              \
    self->head   = (self->head - n) & (self->cap - 1);                                             \
    size_t first = self->cap - self->head < n ? self->cap - self->head : n;                        \
    rk_memcpy(self->data + self->head, arr, sizeof_n(T, first));                                   \
    if (first < n) { rk_memcpy(self->data, arr + first, sizeof_n(T, n - first)); }                 \
    self->count += n;                                                                              \
  }                                                                                                \
  rklib_fun void RKI_DEQUE_PUB(T, push_back_n)(Deque(T) * self, const T* arr, size_t n) {          \
    if (!n) { return; }                                                                            \
    rk_assert(n <= SIZE_MAX - self->count && "Deque capacity overflow");                           \
    RKI_DEQUE_PUB(T, reserve)(self, self->count + n);                                              \
    size_t start = (self->head + self->count) & (self->cap - 1);                                   \
    size_t first = self->cap - start < n ? self->cap - start : n;                                  \
    rk_memcpy(self->data + start, arr, sizeof_n(T, first));                                        \
    if (first < n) { rk_memcpy(self->data, arr + first, sizeof_n(T, n - first)); }                 \
    self->count += n;                                                                              \
  }                                                                                                \
  rklib_fun void RKI_DEQUE_PUB(T, assign)(Deque(T) * self, const T* arr, size_t n) {               \
    RKI_DEQUE_PUB(T, clear)(self);                                                                 \
    RKI_DEQUE_PUB(T, push_back_n)(self, arr, n);                                                   \
  }                                                                                                \
  rklib_fun Deque(T)                                                                               \
      RKI_DEQUE_PUB(T, from)(const T* arr, size_t n RK_IFALLOC(, Allocator alloc)) {               \
    Deque(T) d = RKI_DEQUE_PUB(T, init)(n RK_IFALLOC(, alloc));                                    \
    RKI_DEQUE_PUB(T, push_back_n)(&d, arr, n);                                                     \
    return d;                                                                                      \
  }                                                                                                \
  RK_EXTERNC_END

#define RKI_DEQUE_INIT3(T, cap, alloc)                                                             \
  RKI_REQUIRE_CUSTOM_ALLOCATORS(RKI_DEQUE_PUB(T, init)(cap, alloc))
#define RKI_DEQUE_INIT2(T, cap) RKI_DEQUE_PUB(T, init)(cap RK_IFALLOC(, alloc_ctx))

#define RKI_DEQUE_FROM4(T, arr, n, alloc)                                                          \
  RKI_REQUIRE_CUSTOM_ALLOCATORS(RKI_DEQUE_PUB(T, from)(arr, n, alloc))
#define RKI_DEQUE_FROM3(T, arr, n) RKI_DEQUE_PUB(T, from)(arr, n RK_IFALLOC(, alloc_ctx))

#define RKI_DEQUE_FOREACH(self, it)                                                                \
  for (struct {                                                                                    \
         typeof(*(self))* deque;                                                                   \
         size_t           idx, count;                                                              \
       } rki_var_state = {(self), 0, 0};                                                           \
       rki_var_state.deque && (rki_var_state.count = rki_var_state.deque->count, 1);               \
       rki_var_state.deque = rk_null)                                                              \
    for (typeof(*RKI_DEQUE_ITER_PTR(rki_var_state.deque))* it = rk_null;                           \
         rki_var_state.idx < rki_var_state.count                                                   \
         && (it = &rki_var_state.deque->data[(rki_var_state.deque->head + (rki_var_state.idx))     \
                                             & (rki_var_state.deque->cap - 1)],                    \
            1);                                                                                    \
         ++rki_var_state.idx)

#define RKI_DEQUE_FOREACH_REVERSED(self, it)                                                       \
  for (struct {                                                                                    \
         typeof(*(self))* deque;                                                                   \
         size_t           idx, count;                                                              \
       } rki_var_state = {(self), 0, 0};                                                           \
       rki_var_state.deque && (rki_var_state.count = rki_var_state.deque->count, 1);               \
       rki_var_state.deque = rk_null)                                                              \
    for (typeof(*RKI_DEQUE_ITER_PTR(rki_var_state.deque))* it = rk_null;                           \
         rki_var_state.idx < rki_var_state.count                                                   \
         && (it = &rki_var_state.deque->data[(rki_var_state.deque->head                            \
                                              + (rki_var_state.count - 1 - rki_var_state.idx))     \
                                             & (rki_var_state.deque->cap - 1)],                    \
            1);                                                                                    \
         ++rki_var_state.idx)

#define RKI_DEQUE_ERASE_IF(self, it, pred)                                                         \
  do {                                                                                             \
    typeof(self) const rki_var_deque = (self);                                                     \
    if (!rki_var_deque || !rki_var_deque->count) { break; }                                        \
    const size_t rki_var_count = rki_var_deque->count, rki_var_mask = rki_var_deque->cap - 1;      \
    size_t       rki_var_write = 0;                                                                \
    for (size_t rki_var_idx = 0; rki_var_idx < rki_var_count; ++rki_var_idx) {                     \
      typeof(*rki_var_deque->data)* const it                                                       \
          = &rki_var_deque->data[(rki_var_deque->head + rki_var_idx) & rki_var_mask];              \
      if (!(pred)) {                                                                               \
        if (rki_var_write != rki_var_idx) {                                                        \
          rki_var_deque->data[(rki_var_deque->head + rki_var_write) & rki_var_mask] = *it;         \
        }                                                                                          \
        ++rki_var_write;                                                                           \
      }                                                                                            \
    }                                                                                              \
    rki_var_deque->count = rki_var_write;                                                          \
    if (!rki_var_write) { rki_var_deque->head = 0; }                                               \
  } while (0)

/// @endcond
#pragma endregion implementation
RKI_HEADER_END
/* END DEFERRED IMPLEMENTATION: include/rk_deque.h */
#endif
#ifdef RKI_AMALG_IMPL_F79CED2231B0C73E
#undef RKI_AMALG_IMPL_F79CED2231B0C73E
/* BEGIN DEFERRED IMPLEMENTATION: include/rk_arenastack.h */
RKI_HEADER_BEGIN
#pragma region implementation
////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL
rklib_fun rk_pure Allocator arenastack_allocator(const ArenaStack* self) {
  return vec_allocator(self->arenas);
}
rklib_fun rk_pure ArenaMark arenastack_mark(const ArenaStack* self) {
  return self->arena_size ? arena_mark(&self->arenas[self->cur]) : (ArenaMark){rk_null};
}

#define RKI_ARENASTACK_ALIGNED_NEW(T, count, align, arena_stack)                                   \
  ((typeof(T)*)(alloc_log_new(), rk_assert_valid_align(T, align),                                  \
                arenastack_allocate(sizeof_n(T, count), align, arena_stack)))

#define RKI_ARENASTACK_NEW(T, count, arena_stack)                                                  \
  ((typeof(T)*)(alloc_log_new(), arenastack_allocate(sizeof_n(T, count), alignof(T), arena_stack)))

rklib_fun void arenastack_release(ArenaStack* self) {
  RK_IFALLOC(Allocator alloc = vec_allocator(self->arenas);)
  vec_foreach(self->arenas, arena) {
    alloc_deallocate(arena->beg, (size_t)(arena->end - arena->beg), align_max RK_IFALLOC(, alloc));
  }
  vec_release(self->arenas);
  self->arena_size = 0, self->cur = 0;
}

rklib_fun ArenaStack* arenastack_clear(ArenaStack* self) {
  if rk_unlikely (!self->arena_size) { return self; }
  for (size_t cur = self->cur, i = 0; i <= cur; ++i) { arena_clear(&self->arenas[i]); }
  return self->cur = 0, self;
}

rklib_fun ArenaStack* arenastack_rewind_to(ArenaStack* restrict self, ArenaMark mark) {
  const unsigned char* ptr = mark.pos;
  // A null mark was taken before lazy initialization, i.e. at the starting position.
  if rk_unlikely (!ptr) { return arenastack_clear(self); }
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
// The end-position check alone is not enough here: when the active arena is empty and its buffer
// directly follows the previous arena's buffer, a top allocation of the previous arena also ends at
// the active cursor. Require the allocation to start inside the active arena as well; a zero-size
// position may equal the cursor.
rklib_fun rk_pure bool rki_arenastack_is_active_top(const Arena* arena, const void* ptr,
                                                    size_t size) {
  return (uptr)ptr >= (uptr)arena->beg && arena_is_top_allocation(arena, ptr, size);
}

rklib_fun rk_alloc_size(2) void* arenastack_try_resize_top(size_t old_size, size_t new_size,
                                                           ArenaStack* self) {
  if rk_unlikely (!self->arena_size) { return rk_null; }
  return arena_try_resize_top(old_size, new_size, &self->arenas[self->cur]);
}

rklib_fun rk_alloc_size(3) void* arenastack_try_resize(void* ptr, size_t old_size, size_t new_size,
                                                       ArenaStack* self) {
  if (!self->arena_size || !rki_arenastack_is_active_top(&self->arenas[self->cur], ptr, old_size)) {
    return rk_null;
  }
  return arenastack_try_resize_top(old_size, new_size, self);
}

rklib_fun ArenaStack rki_arenastack_init(size_t cap RK_IFALLOC(, Allocator alloc)) {
  RKI_assert_allocator_valid(alloc);
  cap = stdc_bit_ceil(cap); /*1 if cap==0*/
  return (ArenaStack){
      .arena_size = cap,
      .arenas     = vec_init_list(
          Arena, RK_IFALLOC(alloc, ) arena_init(
                     (unsigned char*)alloc_allocate(cap, align_max RK_IFALLOC(, alloc)), cap)),
      .cur = 0};
}
#define RKI_ARENASTACK_INIT2(cap, alloc)                                                           \
  RKI_REQUIRE_CUSTOM_ALLOCATORS(rki_arenastack_init(cap, alloc))
#define RKI_ARENASTACK_INIT1(cap) rki_arenastack_init(cap RK_IFALLOC(, alloc_ctx))

#define RKI_ARENA_ALLOC_INIT(_SIZE, _ALIGN, _ALLOC)                                                \
  arena_init((unsigned char*)alloc_allocate(_SIZE, _ALIGN RK_IFALLOC(, _ALLOC)), _SIZE)

rklib_fun rk_alloc_alignsize(2, 1) void* rki_arenastack_allocate(size_t nbytes, size_t align,
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
                          RKI_ARENA_ALLOC_INIT(stdc_bit_ceil(rk_MAX(needed, self->arena_size)),
                                               align_max, vec_allocator(self->arenas)));
  return arena_allocate(nbytes, align, &self->arenas[self->cur]);
}

rklib_fun void rki_arenastack_deallocate(void* ptr, size_t old_size, size_t align rk_unused,
                                         void* ctx) {
  ArenaStack* self  = (ArenaStack*)ctx;
  Arena*      arena = &self->arenas[self->cur];
  if (rki_arenastack_is_active_top(arena, ptr, old_size)) {
    (void)arena_try_resize_top(old_size, 0, arena);
  }
}

rklib_fun rk_alloc_alignsize(4, 3) void* rki_arenastack_reallocate(void* ptr, size_t old_size,
                                                                   size_t new_size, size_t align,
                                                                   void* ctx) {
  rk_assert_align_pow2(align);
  ArenaStack* self  = (ArenaStack*)ctx;
  Arena*      arena = &self->arenas[self->cur];

  if ((rki_arenastack_is_active_top(arena, ptr, old_size)
       && arena_try_resize_top(old_size, new_size, arena))
      || new_size <= old_size) {
    return ptr;
  }

  void* res = arenastack_allocate(new_size, align, self);
  rk_memcpy(res, ptr, old_size);
  return res;
}
rklib_fun rk_alloc_alignsize(2, 1) void* arenastack_allocate(size_t nbytes, size_t align,
                                                             ArenaStack* self) {
  return rki_arenastack_allocate(nbytes, align, self);
}

static_fun rk_const Allocator arenastack_to_alloc(ArenaStack* self) {
  return (Allocator){.vtab = &arenastack_allocator_vtable, .ctx = self};
}
#undef RKI_ARENA_ALLOC_INIT

/// @endcond
#pragma endregion implementation
RKI_HEADER_END
/* END DEFERRED IMPLEMENTATION: include/rk_arenastack.h */
#endif
#ifdef RKI_AMALG_IMPL_A44D854F7D622D4E
#undef RKI_AMALG_IMPL_A44D854F7D622D4E
/* BEGIN DEFERRED IMPLEMENTATION: include/rk_string.h */
RKI_HEADER_BEGIN
#pragma region implementation
////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

rklib_fun rk_pure Allocator str_allocator(const Str* self) { return RKI_allocatorof(self); }

#define RKI_STR_DAT(_S)                                                                            \
  _Generic((_S),                                                                                   \
      Str: (char*)RKI_contrav(Str, _S).str,                                                        \
      Strv: (const char*)RKI_contrav(Strv, _S).str,                                                \
      char*: (char*)RKI_contrav_p(char*, _S),                                                      \
      const char*: (const char*)RKI_contrav_p(const char*, _S))

#define RKI_STR_LEN(_S)                                                                            \
  ((size_t)_Generic((_S),                                                                          \
       Str: RKI_contrav(Str, _S).len,                                                              \
       Strv: RKI_contrav(Strv, _S).len,                                                            \
       char*: strlen(RKI_contrav_p(char*, _S)),                                                    \
       const char*: strlen(RKI_contrav_p(const char*, _S)),                                        \
       int: 1,                                                                                     \
       char: 1))

// conditionally cast away const (const is default)
#define RKI_STR_QCHARPTR(_S, expr)                                                                 \
  _Generic((_S), Str: (char*)(expr), Strv: expr, char*: (char*)(expr), const char*: expr)

#define RKI_STR_ITER_PTR(self)                                                                     \
  _Generic((self),                                                                                 \
      Str*: (char*)0,                                                                              \
      const Str*: (const char*)0,                                                                  \
      Strv*: (const char*)0,                                                                       \
      const Strv*: (const char*)0)

#define RKI_STR_FOREACH(self, it)                                                                  \
  for (typeof(*(self))* rki_var_str = (self); rki_var_str; rki_var_str = rk_null)                  \
    for (typeof(*RKI_STR_ITER_PTR(rki_var_str))*it = str_begin(*rki_var_str), *const rki_var_end   \
                                                                              = str_end(           \
                                                                                  *rki_var_str);   \
         it != rki_var_end; ++it)

#define RKI_STR_FOREACH_REVERSED(self, it)                                                         \
  for (typeof(*(self))* rki_var_str = (self); rki_var_str; rki_var_str = rk_null)                  \
    for (typeof(*RKI_STR_ITER_PTR(rki_var_str))*it = str_end(*rki_var_str), *const rki_var_begin   \
                                                                            = str_begin(           \
                                                                                *rki_var_str);     \
         it != rki_var_begin && (--it, 1);)

#define RKI_STR_ERASE_IF(self, it, pred)                                                           \
  do {                                                                                             \
    Str* const rki_var_str = (self);                                                               \
    if (!rki_var_str->str) { break; }                                                              \
    const char* const rki_var_read  = rki_var_str->str;                                            \
    char*             rki_var_write = rki_var_str->str;                                            \
    for (const char *rki_var_it = rki_var_read, *const rki_var_end                                 \
                                                = rki_var_read + rki_var_str->len;                 \
         rki_var_it < rki_var_end; ++rki_var_it) {                                                 \
      const char* const it = rki_var_it;                                                           \
      if (!(pred)) { *rki_var_write++ = *rki_var_it; }                                             \
    }                                                                                              \
    rki_var_str->len = (size_t)(rki_var_write - rki_var_read);                                     \
    *rki_var_write   = '\0';                                                                       \
  } while (0)

#define RKI_STRV_FROM(_S)                                                                          \
  _Generic((_S),                                                                                   \
      Str: RKI_contrav(Str, _S).v,                                                                 \
      Strv: RKI_contrav(Strv, _S),                                                                 \
      char*: strv_from_cstr(RKI_contrav_p(char*, _S)),                                             \
      const char*: strv_from_cstr(RKI_contrav_p(const char*, _S)))

// only to satisfy _Generic when the type cannot be int or char
#define RKI_STRV_FROM_FALLBACK(_S)                                                                 \
  _Generic((_S),                                                                                   \
      Str: RKI_contrav(Str, _S).v,                                                                 \
      Strv: RKI_contrav(Strv, _S),                                                                 \
      char*: strv_from_cstr(RKI_contrav_p(char*, _S)),                                             \
      const char*: strv_from_cstr(RKI_contrav_p(const char*, _S)),                                 \
      int: (Strv){rk_null, 0},                                                                     \
      char: (Strv){rk_null, 0})

#define RKI_ISCHARLITLIKE(C, _if, _else)                                                           \
  rk_static_if(_Generic((C), char: 1, int: 1, default: 0), _if, _else)
#define RKI_GETCHARLITLIKE(_S) _Generic((_S), char: _S, int: _S, default: 0)

#define RKI_STR_INSERT_AT(self, idx, strlike)                                                      \
  RKI_ISCHARLITLIKE(strlike, str_insert_at_char(self, idx, RKI_GETCHARLITLIKE(strlike)),           \
                    str_insert_at_strv(self, idx, RKI_STRV_FROM_FALLBACK(strlike)))

#define RKI_STR_INSERT_AT_MAYALIAS(self, idx, strlike)                                             \
  RKI_ISCHARLITLIKE(strlike, str_insert_at_char(self, idx, RKI_GETCHARLITLIKE(strlike)),           \
                    str_insert_at_strv_mayalias(self, idx, RKI_STRV_FROM_FALLBACK(strlike)))

#define RKI_STR_SPLIT(strvptr, delims)                                                             \
  RKI_ISCHARLITLIKE(delims, str_split_char(strvptr, RKI_GETCHARLITLIKE(delims)),                   \
                    str_split_strv(strvptr, RKI_STRV_FROM_FALLBACK(delims)))

#define RKI_STR_TERMINATE(self, suffix)                                                            \
  RKI_ISCHARLITLIKE(suffix, str_terminate_char(self, RKI_GETCHARLITLIKE(suffix)),                  \
                    str_terminate_strv(self, RKI_STRV_FROM_FALLBACK(suffix)))

#define RKI_STR_CAT(self, strlike)                                                                 \
  RKI_ISCHARLITLIKE(strlike, str_push(self, RKI_GETCHARLITLIKE(strlike)),                          \
                    str_cat_strv(self, strv_from(strlike)))

#define RKI_STR_CAT_MAYALIAS(self, strlike)                                                        \
  RKI_ISCHARLITLIKE(strlike, str_push(self, RKI_GETCHARLITLIKE(strlike)),                          \
                    str_cat_strv_mayalias(self, strv_from(strlike)))

#define RKI_STR_FIND(strlike, subs)                                                                \
  RKI_ISCHARLITLIKE(subs, str_find_char(strv_from(strlike), RKI_GETCHARLITLIKE(subs)),             \
                    str_find_strv(strv_from(strlike), RKI_STRV_FROM_FALLBACK(subs)))

#define RKI_STR_FINDR(strlike, subs)                                                               \
  RKI_ISCHARLITLIKE(subs, str_findr_char(strv_from(strlike), RKI_GETCHARLITLIKE(subs)),            \
                    str_findr_strv(strv_from(strlike), RKI_STRV_FROM_FALLBACK(subs)))

#define RKI_STR_CONTAINS(strlike, subs)                                                            \
  RKI_ISCHARLITLIKE(subs, str_contains_char(strv_from(strlike), RKI_GETCHARLITLIKE(subs)),         \
                    str_contains_strv(strv_from(strlike), RKI_STRV_FROM_FALLBACK(subs)))

#define RKI_STR_STARTS_WITH(strlike, prefix)                                                       \
  RKI_ISCHARLITLIKE(prefix, str_starts_with_char(strv_from(strlike), RKI_GETCHARLITLIKE(prefix)),  \
                    str_starts_with_strv(strv_from(strlike), RKI_STRV_FROM_FALLBACK(prefix)))

#define RKI_STR_ENDS_WITH(strlike, prefix)                                                         \
  RKI_ISCHARLITLIKE(prefix, str_ends_with_char(strv_from(strlike), RKI_GETCHARLITLIKE(prefix)),    \
                    str_ends_with_strv(strv_from(strlike), RKI_STRV_FROM_FALLBACK(prefix)))

rklib_fun rk_pure Strv strv_from_cstr(const char* s) {
  return (Strv){.str = s, .len = s ? strlen(s) : 0};
}

rklib_fun void str_release(Str* restrict self) {
  if (self->str) { alloc_delete(self->str, self->cap RK_IFALLOC(, self->alloc)); }
  self->len = self->cap = 0, self->str = rk_null;
}

rklib_fun rk_pure bool str_is_null_terminated(const Str* self) {
  return self->cap > self->len && self->str[self->len] == '\0';
}
rklib_fun rk_pure const char* str_cstr(const Str* self) {
  return str_is_null_terminated(self) ? self->str : "";
}
rklib_fun rk_pure size_t str_cap(const Str* self) { return self->cap; }
rklib_fun Str*           str_clear(Str* restrict self) {
  if (self->str) { self->str[self->len = 0] = '\0'; }
  return self;
}
rklib_fun char str_pop(Str* restrict self) {
  rk_assert(self->len && "Attempted to pop from an empty Str");
  char tmp                    = self->str[--self->len];
  return self->str[self->len] = '\0', tmp;
}
rklib_fun bool str_try_pop(Str* restrict self, char* restrict out) {
  if (!self->len) { return false; }
  return *out = str_pop(self), true;
}
rklib_fun void str_pop_n(Str* restrict self, size_t n) {
  if (!n) { return; }
  rk_assert(n <= self->len && "Attempted to pop more than Str length");
  self->len -= n, self->str[self->len] = '\0';
}
rklib_fun rk_const Strv strv_from_cstrn(const char* str, size_t len) {
  return (Strv){.str = str, .len = len};
}

rklib_fun rk_const Strv strv_from_strv(Strv str) { return str; }

#define RKI_STR_FROMLIT2(a, alloc)                                                                 \
  RKI_REQUIRE_CUSTOM_ALLOCATORS(str_from_strv((Strv)strv_from_literal(a), alloc))
#define RKI_STR_FROMLIT1(a) str_from_strv((Strv)strv_from_literal(a) RK_IFALLOC(, alloc_ctx))

rklib_fun rk_const Strv strv_from_str(Str str) { return str.v; }
rklib_fun rk_pure bool  str_equals_strv(Strv s1, Strv s2) {
  size_t mlen = rk_MIN(s1.len, s2.len);
  return s1.len == s2.len && !rk_memcmp(s1.str, s2.str, mlen);
}
rklib_fun rk_pure int str_compare_strv(Strv s1, Strv s2) {
  size_t mlen = rk_MIN(s1.len, s2.len);
  int    res  = rk_memcmp(s1.str, s2.str, mlen);
  return res ? res : (s1.len < s2.len ? -1 : (s1.len > s2.len ? 1 : 0));
}

rklib_fun rk_pure bool str_starts_with_char(Strv sv, char c) { return sv.len && sv.str[0] == c; }
rklib_fun rk_pure bool str_starts_with_strv(Strv sv, Strv pref) {
  return pref.len <= sv.len && !rk_memcmp(sv.str, pref.str, pref.len);
}
rklib_fun rk_pure bool str_ends_with_char(Strv sv, char c) {
  return sv.len && sv.str[sv.len - 1] == c;
}

rklib_fun rk_pure bool str_ends_with_strv(Strv sv, Strv suf) {
  if (!suf.len) { return true; }
  return suf.len <= sv.len && !rk_memcmp(sv.str + sv.len - suf.len, suf.str, suf.len);
}

rklib_fun rk_pure const char* str_find_char(Strv sv, char c) {
  return sv.str ? (const char*)memchr(sv.str, (unsigned char)c, sv.len) : sv.str;
}

rklib_fun rk_pure const char* str_find_strv(Strv hs, Strv ne) {
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
rklib_fun rk_pure const char* str_findr_char(Strv sv, char c) {
  while (sv.len--) {
    if (sv.str[sv.len] == c) { return sv.str + sv.len; }
  }
  return rk_null;
}
rklib_fun rk_pure const char* str_findr_strv(Strv hs, Strv ne) {
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

rklib_fun rk_pure bool str_contains_char(Strv sv, char c) {
  for (size_t i = 0; i < sv.len; ++i) {
    if (sv.str[i] == c) { return true; }
  }
  return false;
}

rklib_fun rk_pure bool str_contains_strv(Strv s1, Strv s2) {
  if (!s2.len) { return true; }
  for (size_t i = 0, j; i < s1.len && s1.len - i >= s2.len; ++i) {
    for (j = 0; j < s2.len; ++j) {
      if (s2.str[j] != s1.str[i + j]) { break; }
    }
    if (j == s2.len) { return true; }
  }
  return false;
}

rklib_fun Str rki_str_init(size_t cap RK_IFALLOC(, Allocator alloc)) {
  RKI_assert_allocator_valid(alloc);
  cap       = stdc_bit_ceil(cap); // 1 if cap == 0 (-> space for terminator)
  char* str = alloc_new(char, cap RK_IFALLOC(, alloc));
  str[0]    = '\0';
  return (Str){.str = str, .len = 0, .cap = cap RK_IFALLOC(, .alloc = alloc)};
}
#define RKI_STR_INIT2(cap, alloc) RKI_REQUIRE_CUSTOM_ALLOCATORS(rki_str_init(cap, alloc))
#define RKI_STR_INIT1(cap)        rki_str_init(cap RK_IFALLOC(, alloc_ctx))

rklib_fun void rki_str_change_cap(Str* restrict self, size_t new_cap) {
  RKI_set_alloc_fallback(self->alloc);
  self->str = alloc_renew(self->str, self->cap, new_cap RK_IFALLOC(, self->alloc));
  self->cap = new_cap;
}
rklib_fun void rki_str_ensure_cap(Str* restrict self, size_t new_cap) {
  if (new_cap > self->cap) { rki_str_change_cap(self, stdc_bit_ceil(new_cap)); }
}
rklib_fun rk_const const char* rki_str_end(Strv sv) { return sv.str ? sv.str + sv.len : rk_null; }

rklib_fun rk_const const char* rki_str_peek_front(Strv sv) { return sv.len ? sv.str : rk_null; }
rklib_fun rk_const const char* rki_str_peek_back(Strv sv) {
  return sv.len ? sv.str + sv.len - 1 : rk_null;
}
rklib_fun rk_const const char* rki_str_peek_at(Strv sv, size_t idx) {
  return idx < sv.len ? sv.str + idx : rk_null;
}

// Not rk_const: the asserts are side effects, and a const call whose result is discarded (e.g.
// `(void)str_at(s, i)`) may be removed entirely, silently skipping the bounds check.
rklib_fun const char* rki_str_front_ptr(Strv sv) {
  rk_assert(sv.len > 0 && "Cannot access first element of empty string");
  return sv.str;
}

rklib_fun const char* rki_str_back_ptr(Strv sv) {
  rk_assert(sv.len > 0 && "Cannot access last element of empty string");
  return sv.str + sv.len - 1;
}

rklib_fun const char* rki_str_at_ptr(Strv sv, size_t idx) {
  rk_assert(idx < sv.len && "Access out of bounds of string");
  return sv.str + idx;
}

rklib_fun Str str_from_strv(Strv sv RK_IFALLOC(, Allocator alloc)) {
  RKI_assert_allocator_valid(alloc);
  size_t cap = stdc_bit_ceil(sv.len + 1);
  char*  str = alloc_new(char, cap RK_IFALLOC(, alloc));
  if (sv.len) { memcpy(str, sv.str, sv.len); }
  str[sv.len] = '\0';
  return (Str){.str = str, .len = sv.len, .cap = cap RK_IFALLOC(, .alloc = alloc)};
}
#define RKI_STR_FROM2(_S, alloc) RKI_REQUIRE_CUSTOM_ALLOCATORS(str_from_strv(strv_from(_S), alloc))
#define RKI_STR_FROM1(_S)                                                                          \
  str_from_strv(strv_from(_S) RK_IFALLOC(                                                          \
      , _Generic((_S), Str: rki_allocator_of(RKI_contrav(Str, _S).alloc), default: alloc_ctx)))

rklib_fun Str rki_str_join_strv_n(Strv* svs, size_t count, Strv sep RK_IFALLOC(, Allocator alloc)) {
  RKI_assert_allocator_valid(alloc);
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
#define RKI_STR_JOIN_STRV_N4(svs, count, sep, alloc)                                               \
  RKI_REQUIRE_CUSTOM_ALLOCATORS(rki_str_join_strv_n(svs, count, sep, alloc))
#define RKI_STR_JOIN_STRV_N3(svs, count, sep)                                                      \
  rki_str_join_strv_n(svs, count, sep RK_IFALLOC(, alloc_ctx))

rklib_fun Str* str_assign_strv(Str* restrict self, Strv sv) {
  rki_str_ensure_cap(self, sv.len + 1);
  if (sv.len) { memmove(self->str, sv.str, sv.len); }
  self->str[self->len = sv.len] = '\0';
  return self;
}

rklib_fun rk_const Strv strv_slice_strv(Strv sv, size_t start, size_t end) {
  if (start > sv.len || end < start) {
    sv.len = 0;
  } else {
    if (sv.str) { sv.str += start; }
    sv.len = rk_MIN(end, sv.len) - start;
  }
  return sv;
}

rklib_fun Str* str_reserve(Str* restrict self, size_t new_cap) {
  rki_str_ensure_cap(self, new_cap);
  return self;
}

rklib_fun Str* str_resize(Str* restrict self, size_t new_len) {
  rki_str_ensure_cap(self, new_len + 1);
  return self->str[self->len = new_len] = '\0', self;
}

rklib_fun Str* str_shrink_to_fit(Str* restrict self) {
  size_t new_cap = stdc_bit_ceil(self->len + 1);
  if (self->cap > new_cap) { rki_str_change_cap(self, new_cap); }
  return self;
}
rklib_fun Str* str_shrink_to_fit_exact(Str* restrict self) {
  if (self->cap > self->len + 1) { rki_str_change_cap(self, self->len + 1); }
  return self;
}

rklib_fun Str* str_cat_strv(Str* restrict self, Strv sv) {
  if (!sv.len) { return self; }
  size_t new_len = sv.len + self->len;
  rki_str_ensure_cap(self, new_len + 1);
  memcpy(self->str + self->len, sv.str, sv.len);
  self->str[self->len = new_len] = '\0';
  return self;
}

rklib_fun Str* str_cat_strv_mayalias(Str* restrict self, Strv sv) {
  if (!sv.len) { return self; }
  size_t new_len = sv.len + self->len;
  if (new_len + 1 > self->cap) {
    uptr sbeg = (uptr)self->str, send = sbeg + self->cap, cbeg = (uptr)sv.str;
    rki_str_ensure_cap(self, new_len + 1);
    if (cbeg >= sbeg && cbeg < send) {
      // offset of the char* into the str mem since a Str is not a
      // substring, char is always at higher address
      sv.str = self->str + (cbeg - sbeg);
    }
  }
  memmove(self->str + self->len, sv.str, sv.len);
  self->str[self->len = new_len] = '\0';
  return self;
}

rklib_fun Str* str_insert_at_strv(Str* restrict self, size_t at, Strv sv) {
  rk_assert(at <= self->len && "Attempted to insert of out Str bounds");
  if (!sv.len) { return self; }
  if (at == self->len) { return str_cat_strv(self, sv); }
  size_t new_len = sv.len + self->len;
  rki_str_ensure_cap(self, new_len + 1);
  memmove(self->str + at + sv.len, self->str + at, self->len + 1 - at);
  memcpy(self->str + at, sv.str, sv.len);
  self->len = new_len;
  return self;
}

rklib_fun Str* str_insert_at_strv_mayalias(Str* restrict self, size_t idx, Strv sv) {
  rk_assert(idx <= self->len && "Attempted to insert out of Str bounds");
  if (!sv.len) { return self; }
  if (idx == self->len) { return str_cat_strv_mayalias(self, sv); }
  uptr   sbeg = (uptr)self->str, send = sbeg + self->cap, cbeg = (uptr)sv.str;
  bool   alias = cbeg >= sbeg && cbeg < send;
  size_t new_len;
  if (alias) {
    RKI_set_alloc_fallback(self->alloc);
    char* from = alloc_new(char, sv.len RK_IFALLOC(, self->alloc));
    memcpy(from, sv.str, sv.len);
    new_len = sv.len + self->len;
    rki_str_ensure_cap(self, new_len + 1);
    memmove(self->str + idx + sv.len, self->str + idx, self->len + 1 - idx);
    memcpy(self->str + idx, from, sv.len);
    alloc_delete(from, sv.len RK_IFALLOC(, self->alloc));
  } else {
    const char* from = sv.str;
    new_len          = sv.len + self->len;
    rki_str_ensure_cap(self, new_len + 1);
    memmove(self->str + idx + sv.len, self->str + idx, self->len + 1 - idx);
    memcpy(self->str + idx, from, sv.len);
  }
  self->len = new_len;
  return self;
}

rklib_fun Str* str_insert_at_char(Str* restrict self, size_t at, char chr) {
  rk_assert(at <= self->len && "Attempted to insert out of Str bounds");
  if (at == self->len) { return str_push(self, chr); }
  rki_str_ensure_cap(self, self->len + 2);
  memmove(self->str + at + 1, self->str + at, self->len + 1 - at);
  self->str[at] = chr;
  ++self->len;
  return self;
}

rklib_fun Str* str_null_terminate(Str* restrict self) {
  rki_str_ensure_cap(self, self->len + 1);
  return str_null_terminate_unchecked(self);
}

rklib_fun Str* str_null_terminate_unchecked(Str* restrict self) {
  rk_assert(self->cap > self->len && "Attempted to push beyond the capacity of the Str");
  return self->str[self->len] = '\0', self;
}

rklib_fun Str* str_terminate_char(Str* restrict self, char suf) {
  if (!str_ends_with_char(self->v, suf)) { str_push(self, suf); }
  return self;
}

rklib_fun Str* str_terminate_strv(Str* restrict self, Strv suf) {
  if (!str_ends_with_strv(self->v, suf)) { str_cat_strv(self, suf); }
  return self;
}

rklib_fun Str* str_push(Str* restrict self, char c) {
  rki_str_ensure_cap(self, self->len + 2);
  return str_push_unchecked(self, c);
}
rklib_fun Str* str_push_unchecked(Str* restrict self, char c) {
  rk_assert(self->cap > self->len + 1 && "Attempted to push beyond the capacity of the Str");
  return self->str[self->len++] = c, str_null_terminate_unchecked(self);
}
rklib_fun Str* str_push_raw(Str* restrict self, char c) {
  rki_str_ensure_cap(self, self->len + 1);
  return str_push_unchecked_raw(self, c);
}
rklib_fun Str* str_push_unchecked_raw(Str* restrict self, char c) {
  rk_assert(self->cap > self->len && "Attempted to push beyond the capacity of the Str");
  return self->str[self->len++] = c, self;
}

rklib_fun Str* str_erase_at(Str* restrict self, size_t idx) {
  rk_assert(idx < self->len && "Erase out of bounds");
  memmove(self->str + idx, self->str + idx + 1, self->len - idx);
  return --self->len, self;
}

rklib_fun Str* str_erase_at_n(Str* restrict self, size_t idx, size_t count) {
  rk_assert(idx <= self->len && count <= self->len - idx && "Erase out of bounds");
  if (count) {
    memmove(self->str + idx, self->str + idx + count, self->len - idx - count + 1);
    self->len -= count;
  }
  return self;
}

rklib_fun Str* str_replace(Str* restrict self, char oldc, char newc) {
  for (size_t i = 0, len = self->len; i < len; ++i) {
    if (self->str[i] == oldc) { self->str[i] = newc; }
  }
  return self;
}

rklib_fun char str_replace_at(Str* restrict self, size_t pos, char c) {
  rk_assert(pos < self->len);
  rk_SWAP(c, self->str[pos]);
  return c;
}

rklib_fun Str* str_to_upper(Str* restrict self) {
  char* s = self->str;
  for (size_t i = 0, len = self->len; i < len; ++i) {
    // The adjustment is always exactly 0 or 'a'-'A', so the result always
    // stays within a valid char; the cast just makes that narrowing explicit.
    s[i] = (char)(s[i] - (s[i] >= 'a' && s[i] <= 'z') * ('a' - 'A'));
  }
  return self;
}
rklib_fun Str* str_to_lower(Str* restrict self) {
  char* s = self->str;
  for (size_t i = 0, len = self->len; i < len; ++i) {
    s[i] = (char)(s[i] + (s[i] >= 'A' && s[i] <= 'Z') * ('a' - 'A'));
  }
  return self;
}
rklib_fun Str* str_reverse(Str* restrict self) {
  for (size_t i = 0, len = self->len; i < len / 2; ++i) {
    rk_SWAP(self->str[i], self->str[len - 1 - i]);
  }
  return self;
}

#define RKI_STR_CHAR_ISSPACE(c) ((c) == ' ' || ((c) >= '\t' && (c) <= '\r'))
rklib_fun rk_pure Strv str_trimmed_left_strv(Strv sv) {
  size_t i = 0;
  for (; i < sv.len && RKI_STR_CHAR_ISSPACE(sv.str[i]); ++i);
  if (sv.str) { sv.str += i, sv.len -= i; }
  return sv;
}
rklib_fun rk_pure Strv str_trimmed_right_strv(Strv sv) {
  for (; sv.len && RKI_STR_CHAR_ISSPACE(sv.str[sv.len - 1]); --sv.len);
  return sv;
}
rklib_fun rk_pure Strv str_trimmed_strv(Strv sv) {
  return str_trimmed_right_strv(str_trimmed_left_strv(sv));
}
#undef RKI_STR_CHAR_ISSPACE

rklib_fun Strv*(str_split_alloc)(Strv str, Strv dels,
                                 size_t* restrict out_count RK_IFALLOC(, Allocator alloc)) {
  RKI_assert_allocator_valid(alloc);
  typedef unsigned char uchar;
  uchar                 dbits[256] = {RKI_ZINIT};
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
#define RKI_STR_SPLIT_ALLOC4(str, delims, count, alloc)                                            \
  RKI_REQUIRE_CUSTOM_ALLOCATORS(str_split_alloc(str, delims, count, alloc))
#define RKI_STR_SPLIT_ALLOC3(str, delims, count)                                                   \
  str_split_alloc(str, delims, count RK_IFALLOC(, alloc_ctx))

rklib_fun Str* rk_attr_printf(2, 3) str_cat_fmt(Str* self, const char* fmt, ...) {
  size_t  len = self->len, rem = self->cap - len;
  va_list ap, ap_probe;
  va_start(ap, fmt), va_copy(ap_probe, ap);
  int n = vsnprintf(self->str ? self->str + len : rk_null, rem, fmt, ap_probe);
  va_end(ap_probe);
  if (n < 0) { return va_end(ap), rk_null; }
  if (rem < (size_t)n + 1) {
    rki_str_ensure_cap(self, len + (size_t)n + 1);
    n = vsnprintf(self->str + len, self->cap - len, fmt, ap);
    if (n < 0) { return va_end(ap), rk_null; }
  }
  self->len = len + (size_t)n;
  return va_end(ap), self;
}

rklib_fun Strv str_split_char(Strv* self, char delim) {
  Strv tok = {.str = self->str, .len = 0};
  if (!self->len) { return self->len = STR_SPLIT_END, tok; } // uses unsigned wraparound
  const char* p  = str_find_char(*self, delim);
  tok.len        = p ? (size_t)(p - tok.str) : self->len;
  self->str     += tok.len + (p != rk_null);
  self->len     -= tok.len + 1;
  return tok;
}
rklib_fun Strv str_split_strv(Strv* self, Strv dels) {
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
#pragma endregion implementation
RKI_HEADER_END
/* END DEFERRED IMPLEMENTATION: include/rk_string.h */
#endif
#ifdef RKI_AMALG_IMPL_66FA845F969DDC89
#undef RKI_AMALG_IMPL_66FA845F969DDC89
/* BEGIN DEFERRED IMPLEMENTATION: include/rk_pool.h */
RKI_HEADER_BEGIN
#pragma region implementation
////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

#define RKI_POOL2 StaticPool
#define RKI_POOL_DEFINE2(T, C)                                                                     \
  typedef struct StaticPool(T, C) {                                                                \
    bitset(C) data;                                                                                \
    union {                                                                                        \
      T els[C], rki_els_dummy[C];                 /* only for _Generic, never read */              \
      union { unsigned char cap, els[1]; } _pool; /* only for _Generic, never read */              \
    };                                                                                             \
  } StaticPool(T, C)

/// @brief to allow for type-generic access of dynamic pools todo c++ UB union
typedef struct RKI_DynPool {
  RK_IFALLOC(Allocator alloc;)
  bitset data;
  size_t cap;
  union {
    void* els;
    char  rki_els_dummy[1];
  };
} RKI_DynPool;

#define RKI_POOL_DEFINE1(T)                                                                        \
  typedef struct DynPool(T) {                                                                      \
    union {                                                                                        \
      RKI_DynPool _pool;                                                                           \
      struct {                                                                                     \
        RK_IFALLOC(Allocator alloc;)                                                               \
        bitset data;                                                                               \
        size_t cap;                                                                                \
        union {                                                                                    \
          T*   els;              /*type marker, never accessed directly during runtime */          \
          char rki_els_dummy[1]; /*unselected _Generic branch countof */                           \
        };                                                                                         \
      };                                                                                           \
    };                                                                                             \
  } DynPool(T)

#define RKI_POOL1        DynPool

#define RKI_POOL_T(self) typeof(*(self)->els)

#define RKI_POOL_DISPATCH(self, if_stat, if_dyn)                                                   \
  rk_static_if(sizeof((self)->_pool) == 1, if_stat, if_dyn)

rklib_fun rk_forceinline void* rki_dpool_init(size_t elsize, size_t elalign, RKI_DynPool* self,
                                              size_t cap RK_IFALLOC(, Allocator alloc)) {
  RKI_assert_allocator_valid(alloc);
  self->cap = cap;
  RK_IFALLOC(self->alloc = alloc;)
  self->data
      = bitset_clear_all(alloc_new(bitset_word, bitset_words(cap) RK_IFALLOC(, self->alloc)), cap);
  self->els = alloc_allocate(rk_mult(elsize, cap), elalign RK_IFALLOC(, alloc));
  return self;
}
#define RKI_DPOOL_INIT(T, _cap, _alloc)                                                            \
  (*((Pool(T)*)rki_dpool_init(sizeof(T), alignof(T), (RKI_DynPool*)((Pool(T)[1]){RKI_ZINIT}),      \
                              _cap RK_IFALLOC(, _alloc))))
#define RKI_DPOOL_INIT3(T, _cap, _alloc)                                                           \
  RKI_REQUIRE_CUSTOM_ALLOCATORS(RKI_DPOOL_INIT(T, _cap, _alloc))
#define RKI_DPOOL_INIT2(T, _cap) RKI_DPOOL_INIT(T, _cap, alloc_ctx)

rklib_fun rk_forceinline void rki_dpool_release(size_t elsize, size_t elalign, RKI_DynPool* self) {
  if (!self->cap) { return; }
  alloc_deallocate(self->els, rk_mult(elsize, self->cap), elalign RK_IFALLOC(, self->alloc));
  alloc_delete(self->data, bitset_words(self->cap) RK_IFALLOC(, self->alloc));
  self->cap = 0, self->data = rk_null, self->els = rk_null;
}

#define RKI_POOL_RELEASE(self)                                                                     \
  RKI_POOL_DISPATCH(self, (void)bitset_clear_all((self)->data, RKI_SPOOL_CAP(self)),               \
                    rki_dpool_release(RKI_POOL_SIZE(self), RKI_POOL_ALIGN(self),                   \
                                      (RKI_DynPool*)&((self)->_pool)))

#define RKI_SPOOL_CAP(self) rk_COUNTOF((self)->rki_els_dummy)
#define RKI_POOL_CAP(self)  RKI_POOL_DISPATCH(self, RKI_SPOOL_CAP(self), (size_t)(self)->_pool.cap)

rklib_fun rk_pure rk_forceinline size_t rki_dpool_used(const RKI_DynPool* self) {
  return bitset_count_ones(self->data, self->cap);
}
#define RKI_POOL_USED(self)                                                                        \
  RKI_POOL_DISPATCH(self, bitset_count_ones((self)->data, RKI_SPOOL_CAP(self)),                    \
                    rki_dpool_used((RKI_DynPool*)&((self)->_pool)))

rklib_fun rk_pure rk_forceinline size_t rki_dpool_remaining(const RKI_DynPool* self) {
  return bitset_count_zeros(self->data, self->cap);
}
#define RKI_POOL_REMAINING(self)                                                                   \
  RKI_POOL_DISPATCH(self, bitset_count_zeros((self)->data, RKI_SPOOL_CAP(self)),                   \
                    rki_dpool_remaining((RKI_DynPool*)&((self)->_pool)))

rklib_fun rk_forceinline void* rki_dpool_clear(RKI_DynPool* self) {
  bitset_clear_all(self->data, self->cap);
  return self;
}
#define RKI_POOL_CLEAR(self)                                                                       \
  RKI_POOL_DISPATCH(self, bitset_clear_all((self)->data, RKI_SPOOL_CAP(self)),                     \
                    rki_dpool_clear((RKI_DynPool*)&((self)->_pool)))

rklib_fun rk_forceinline void rki_spool_delete(size_t elsize, size_t cap, size_t offset, void* self,
                                               void* ptr) {
  bitset_clear((bitset)self, cap, (size_t)((size_t)((char*)ptr - ((char*)self + offset)) / elsize));
}
rklib_fun rk_forceinline void rki_dpool_delete(size_t elsize, RKI_DynPool* self, void* ptr) {
  bitset_clear(self->data, self->cap, (size_t)((size_t)((char*)ptr - (char*)self->els) / elsize));
}
#define RKI_POOL_DELETE(self, ptr)                                                                 \
  RKI_POOL_DISPATCH(self,                                                                          \
                    rki_spool_delete(RKI_POOL_SIZE(self), RKI_SPOOL_CAP(self),                     \
                                     offsetof(typeof(*(self)), els), self, ptr),                   \
                    rki_dpool_delete(RKI_POOL_SIZE(self), (RKI_DynPool*)&((self)->_pool), ptr))

rklib_fun rk_forceinline rk_alloc_alignsize(2, 1) void* rki_spool_try_new(size_t         elsize,
                                                                          size_t elalign rk_unused,
                                                                          size_t cap, size_t offset,
                                                                          void* self) {
  size_t free_slot = bitset_first_trailing_zero((bitset)self, cap);
  if (!free_slot) { return rk_null; }
  bitset_set((bitset)self, cap, free_slot - 1);
  return (char*)self + offset + elsize * (free_slot - 1);
}

rklib_fun rk_forceinline rk_alloc_alignsize(2, 1) void* rki_dpool_try_new(size_t         elsize,
                                                                          size_t elalign rk_unused,
                                                                          RKI_DynPool*   self) {
  if (!self->cap) { return rk_null; }
  size_t free_slot = bitset_first_trailing_zero(self->data, self->cap);
  if (!free_slot) { return rk_null; }
  bitset_set(self->data, self->cap, free_slot - 1);
  return (char*)self->els + elsize * (free_slot - 1);
}

#define RKI_POOL_TRY_NEW(self)                                                                     \
  RKI_POOL_DISPATCH(self,                                                                          \
                    rki_spool_try_new(RKI_POOL_SIZE(self), RKI_POOL_ALIGN(self),                   \
                                      RKI_SPOOL_CAP(self), offsetof(typeof(*(self)), els), self),  \
                    rki_dpool_try_new(RKI_POOL_SIZE(self), RKI_POOL_ALIGN(self),                   \
                                      (RKI_DynPool*)&((self)->_pool)))

rklib_fun rk_forceinline rk_alloc_alignsize(2, 1) void* rki_spool_new(size_t elsize, size_t elalign,
                                                                      size_t cap, size_t offset,
                                                                      void* self) {
  void* res = rki_spool_try_new(elsize, elalign, cap, offset, self);
  RK_POOL_FAIL(res, self, rk_null, elalign, elsize);
  return res;
}
rklib_fun rk_forceinline rk_alloc_alignsize(2, 1) void* rki_dpool_new(size_t elsize, size_t elalign,
                                                                      RKI_DynPool* self) {
  void* res = rki_dpool_try_new(elsize, elalign, self);
  RK_POOL_FAIL(res, self, rk_null, elalign, elsize);
  return res;
}

#define RKI_POOL_NEW(self)                                                                         \
  RKI_POOL_DISPATCH(                                                                               \
      self,                                                                                        \
      rki_spool_new(RKI_POOL_SIZE(self), RKI_POOL_ALIGN(self), RKI_SPOOL_CAP(self),                \
                    offsetof(typeof(*(self)), els), self),                                         \
      rki_dpool_new(RKI_POOL_SIZE(self), RKI_POOL_ALIGN(self), (RKI_DynPool*)&((self)->_pool)))

rklib_fun rk_forceinline rk_alloc_alignsize(2, 1) void* rki_spool_try_put(size_t         elsize,
                                                                          size_t elalign rk_unused,
                                                                          size_t cap, size_t offset,
                                                                          void* restrict self,
                                                                          void* restrict obj) {
  bitset data      = (bitset)self;
  size_t free_slot = bitset_first_trailing_zero(data, cap);
  if (!free_slot) { return rk_null; }
  bitset_set(data, cap, free_slot - 1);
  void* ptr = (char*)self + offset + elsize * (free_slot - 1);
  rk_memcpy(ptr, obj, elsize);
  return ptr;
}
rklib_fun rk_forceinline rk_alloc_alignsize(2, 1) void* rki_spool_put(size_t elsize, size_t elalign,
                                                                      size_t cap, size_t offset,
                                                                      void* restrict self,
                                                                      void* restrict obj) {
  void* res = rki_spool_try_put(elsize, elalign, cap, offset, self, obj);
  RK_POOL_FAIL(res, self, rk_null, elalign, elsize);
  return res;
}
rklib_fun rk_forceinline rk_alloc_alignsize(2, 1) void* rki_dpool_try_put(
    size_t elsize, size_t elalign rk_unused, RKI_DynPool* restrict self, void* restrict obj) {
  if (!self->cap) { return rk_null; }
  size_t free_slot = bitset_first_trailing_zero(self->data, self->cap);
  if (!free_slot) { return rk_null; }
  bitset_set(self->data, self->cap, free_slot - 1);
  return rk_memcpy((char*)self->els + elsize * (free_slot - 1), obj, elsize);
}
rklib_fun rk_forceinline rk_alloc_alignsize(2, 1) void* rki_dpool_put(size_t elsize, size_t elalign,
                                                                      RKI_DynPool* restrict self,
                                                                      void* restrict obj) {
  void* res = rki_dpool_try_put(elsize, elalign, self, obj);
  RK_POOL_FAIL(res, self, rk_null, elalign, elsize);
  return res;
}
#define RKI_POOL_TRY_PUT(self, el)                                                                 \
  RKI_POOL_DISPATCH(self,                                                                          \
                    rki_spool_try_put(RKI_POOL_SIZE(self), RKI_POOL_ALIGN(self),                   \
                                      RKI_SPOOL_CAP(self), offsetof(typeof(*(self)), els), self,   \
                                      (RKI_POOL_T(self)[1]){el}),                                  \
                    rki_dpool_try_put(RKI_POOL_SIZE(self), RKI_POOL_ALIGN(self),                   \
                                      (RKI_DynPool*)&((self)->_pool), (RKI_POOL_T(self)[1]){el}))

#define RKI_POOL_PUT(self, el)                                                                     \
  RKI_POOL_DISPATCH(self,                                                                          \
                    rki_spool_put(RKI_POOL_SIZE(self), RKI_POOL_ALIGN(self), RKI_SPOOL_CAP(self),  \
                                  offsetof(typeof(*(self)), els), self,                            \
                                  (RKI_POOL_T(self)[1]){el}),                                      \
                    rki_dpool_put(RKI_POOL_SIZE(self), RKI_POOL_ALIGN(self),                       \
                                  (RKI_DynPool*)&((self)->_pool), (RKI_POOL_T(self)[1]){el}))

#define RKI_POOL_SIZE(self)  sizeof(*(self)->els)
#define RKI_POOL_ALIGN(self) alignof(RKI_POOL_T(self))
// Constness is detected through `_pool.cap`, a member of a named type (unsigned char for static
// pools, size_t for dynamic ones): matching on `const typeof(*(self))*` would spell
// `const const Pool` for a const Pool (MSVC C4114), as for the Dict/Deque/tree helpers.
#define RKI_POOL_ITER_PTR(self)                                                                    \
  RKI_IGNWARN_MSC(4114, _Generic(&(self)->_pool.cap,                                               \
                      const unsigned char*: (const RKI_POOL_T(self)*)0,                            \
                      const size_t*: (const RKI_POOL_T(self)*)0,                                   \
                      default: (RKI_POOL_T(self)*)0))
/// to prevent inactive union member access in c++
#define RKI_POOL_ELS(self)                                                                         \
  RKI_POOL_DISPATCH(self, (self)->els, (typeof(RKI_POOL_ITER_PTR(self)))(self)->_pool.els)

#define RKI_STATOVERLOAD__(m, ...) rk_CONC(m, rk_ARGCOUNT(__VA_ARGS__))(__VA_ARGS__)
#define RKI_STATOVERLOAD(m, ...)   rk_CONC(m, rk_ARGCOUNT(__VA_ARGS__))(__VA_ARGS__)

// A single real loop: `it` is the loop variable itself, and each re-check of the condition
// (whether reached normally or via `continue`) advances to the next set bit. `break` therefore
// exits this loop directly, same as it would for a plain array loop -- unlike the old
// three-nested-loop version this replaced, where the innermost loop existed only to declare `it`
// once per index and always terminated after a single pass regardless of the body, so `break` only
// ever exited that already-terminating inner loop and silently behaved like `continue`.
#define RKI_POOL_FOREACH(self, it)                                                                 \
  for (struct {                                                                                    \
         typeof(*(self))* pool;                                                                    \
         RKI_BitsetIter   bits;                                                                    \
         size_t           idx;                                                                     \
       } rki_var_state = {(self), {rk_null, 0, 0, 0}, 0};                                          \
       rki_var_state.pool                                                                          \
       && (rki_var_state.bits.bits = rki_var_state.pool->data,                                     \
          rki_var_state.bits.count = pool_cap(rki_var_state.pool), 1);                             \
       rki_var_state.pool = rk_null)                                                               \
    for (typeof(RKI_POOL_ITER_PTR(rki_var_state.pool)) it = rk_null;                               \
         (rki_var_state.idx = rki_bitset_iter_next(&rki_var_state.bits)) != BITSET_NPOS            \
         && (it = RKI_POOL_ELS(rki_var_state.pool) + rki_var_state.idx, (void)it, 1);)

#define RKI_POOL_ERASE_IF(self, it, pred)                                                          \
  do {                                                                                             \
    typeof(self) const rki_var_pool = (self);                                                      \
    RKI_POOL_FOREACH(rki_var_pool, rki_var_cursor) {                                               \
      RKI_POOL_T(rki_var_pool)* const it = rki_var_cursor;                                         \
      (void)it;                                                                                    \
      if (pred) { pool_delete(rki_var_pool, it); }                                                 \
    }                                                                                              \
  } while (0)

/// @endcond
#pragma endregion implementation
RKI_HEADER_END
/* END DEFERRED IMPLEMENTATION: include/rk_pool.h */
#endif
#ifdef RKI_AMALG_IMPL_1E829680B591C33B
#undef RKI_AMALG_IMPL_1E829680B591C33B
/* BEGIN DEFERRED IMPLEMENTATION: include/rk_dict.h */
RKI_HEADER_BEGIN
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
/* END DEFERRED IMPLEMENTATION: include/rk_dict.h */
#endif
