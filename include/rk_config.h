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
     rk_assert((cond) && "malloc allocation failure");                                             \
     (void)(ctx), (void)(old_ptr), (void)(align), (void)(new_size);                                \
     (cond) ? (void)0 : abort();                                                                   \
   } while (0)

#endif

#ifndef RK_MMAP_FAIL

/// @brief Failure handler for OS page allocation (`mmap` / `VirtualAlloc`).
# define RK_MMAP_FAIL(cond, ctx, old_ptr, align, new_size)                                         \
   do {                                                                                            \
     rk_assert((cond) && "map allocation failure");                                                \
     (void)(ctx), (void)(old_ptr), (void)(align), (void)(new_size);                                \
     (cond) ? (void)0 : abort();                                                                   \
   } while (0)

#endif

#ifndef RK_ARENA_FAIL

/// @brief Failure handler for arena allocation. `ctx` is the corresponding `Arena *`.
# define RK_ARENA_FAIL(cond, ctx, old_ptr, align, new_size)                                        \
   do {                                                                                            \
     rk_assert((cond) && "arena allocation failure");                                              \
     (void)(ctx), (void)(old_ptr), (void)(align), (void)(new_size);                                \
     (cond) ? (void)0 : abort();                                                                   \
   } while (0)

#endif

#ifndef RK_POOL_FAIL

/// @brief Failure handler for pool allocation. `ctx` is the corresponding pool pointer.
# define RK_POOL_FAIL(cond, ctx, old_ptr, align, new_size)                                         \
   do {                                                                                            \
     rk_assert((cond) && "pool allocation failure");                                               \
     (void)(ctx), (void)(old_ptr), (void)(align), (void)(new_size);                                \
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
