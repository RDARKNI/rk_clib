// SPDX-License-Identifier: MIT
/// @file rk_config.h
/// @version 1.0
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
///   size/count computations (`sizeof_n`, `Vec`/`Pool` capacity sizing, etc.). The default is a raw,
///   unchecked multiply: an overflowing `count * sizeof(T)` silently wraps to a small value, so a
///   too-large `count` can lead to a successful but undersized allocation. Define as `rk_mult_safe`
///   (declared in `rk_defs.h`) to `abort()` on overflow instead, at the cost of a runtime check
///   (a branch and, on the overflowing path, a division) on every multiplication.
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
