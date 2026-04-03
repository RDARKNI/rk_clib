/// @file rk_config.h
/// @version 1.0
/// @defgroup rk_config rklib Configuration Macros
/// @brief Configuration macros for rklib. All options relating to a
/// non-included header are ignored.
///
/// @warning All translation units using this library **must** share the same
/// configuration. Mismatches cause **ill-formed, no diagnostic required**
/// (IFNDR) behaviour due to differing struct layouts and function definitions.
///
/// ---
///
/// - **RK_IMPL**
///   Must be `#define`d in exactly **one** translation unit before including
///   any rklib header. This provides the definitions for `extern inline`
///   functions. All other translation units must omit it.
///
/// - **RK_ALLOCMODE**           default: `RK_ALLOCMODE_FULL`
///   Controls how allocators are threaded through library objects. Three modes
///   are available (defined as integer constants below):
///
///   - `RK_ALLOCMODE_FULL` *(default)*
///     Every library object stores its own `Allocator` as a struct member and
///     accepts one as an init argument. Allocations on that object always use
///     its local allocator. This is the most flexible mode and supports mixing
///     different allocators across objects simultaneously, at the cost of one
///     extra `Allocator`-sized field per object.
///
///   - `RK_ALLOCMODE_NO_LOCAL`
///     No per-object allocator field or init argument. All allocations go
///     through the global (or thread-local) `alloc_ctx` pointer. Saves the
///     per-object overhead of `FULL`, but requires care when swapping
///     `alloc_ctx` at runtime, since all live objects are affected.
///
///   - `RK_ALLOCMODE_MALLOC_ONLY`
///     All allocations unconditionally use thin wrappers over `malloc`/`free`.
///     `alloc_ctx` exists but is `static const` and cannot be changed at
///     runtime. Allocator arguments to init macros are accepted syntactically
///     but ignored. Use this mode when custom allocators are not needed and
///     you want zero allocator overhead.
///
/// - **RKLIB_DEBUG**               default: not defined
///   Define to enable debug logging. `rk_log()` will print diagnostics to
///   `stderr` and `rk_assert()` will print the failed expression before
///   calling `abort()`. Has no runtime cost when `0`.
///
/// - **RK_ALLOC_MULTITHREADED** default: `0`
///   Set to `1` to give the global `alloc_ctx` pointer `thread_local` storage
///   duration, enabling a distinct default allocator per thread. Has no effect
///   when `RK_ALLOCMODE == RK_ALLOCMODE_MALLOC_ONLY`, because that mode has
///   no `alloc_ctx`.
///
/// - **RK_MALLOC_FAIL**(`cond, ctx, old_ptr, align, new_size`)
///                              default: `rk_assert(cond)` + `abort()`
///   Called inside the malloc-based allocator when an allocation returns
///   `NULL`. `cond` is the (null) pointer returned by the allocator. The
///   remaining arguments provide context for custom handlers (e.g. logging or
///   fallback allocation). The handler must not return normally; it must
///   either abort or longjmp.
///
/// - **RK_MMAP_FAIL**(`cond, ctx, old_ptr, align, new_size`)
///                              default: `rk_assert(cond)` + `abort()`
///   Same as `RK_MALLOC_FAIL` but called by the page allocator (`mmap` /
///   `VirtualAlloc`) on failure.
///
/// - **RK_ARENA_FAIL**(`cond, ctx, old_ptr, align, new_size`)
///                              default: `rk_assert(cond)` + `abort()`
///   Called inside `rk_arena.h` allocators when the arena has no space left.
///   `ctx` is the `Arena*`. The handler must not return normally.
///
/// - **RK_POOL_FAIL**(`cond, ctx, old_ptr, align, new_size`)
///                              default: `rk_assert(cond)` + `abort()`
///   Called inside `rk_pool.h` allocators when all pool slots are occupied.
///   `ctx` is the pool pointer. The handler must not return normally.
///
/// - **RK_DICT_MAX_LOAD_FACTOR** default: `0.75f`
///   Floating-point value in `(0, 1)` controlling when `rk_dict` rehashes.
///   Lower values reduce collisions at the cost of more memory; higher values
///   do the opposite. Applies globally to all `Dict` instances.
/// @{

#ifndef RK_CONFIG_H
#define RK_CONFIG_H
#if defined(__cplusplus) && !defined(__clang__)
# error "C++ support only for Clang"
#elif defined(_MSC_VER) && _MSC_VER < 1939
# error "Unsupported MSVC version"
#endif

////////////////////////////////////////////////////////////////////////////////
///////////////////////// Configuration Macros /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// clang-format off
// Quick-reference: define any of these before including rklib headers.
// Full documentation is in the Doxygen block above.
//
// #define RK_IMPL                                           // one TU only
// #define RKLIB_DEBUG                                      // enable logging
// #define RK_ALLOCMODE                  RK_ALLOCMODE_FULL  // FULL / NO_LOCAL / MALLOC_ONLY
// #define RK_ALLOC_MULTITHREADED        1                  // thread_local alloc_ctx
// #define RK_MALLOC_FAIL(cond, ctx, old_ptr, align, new_size)  ...
// #define RK_MMAP_FAIL( cond, ctx, old_ptr, align, new_size)   ...
// #define RK_ARENA_FAIL(cond, ctx, old_ptr, align, new_size)   ...
// #define RK_POOL_FAIL( cond, ctx, old_ptr, align, new_size)   ...
// #define RK_DICT_MAX_LOAD_FACTOR       0.75f
// clang-format on

/// @brief Each library object stores its own `Allocator` member and accepts
/// one as an init argument. Supports mixing allocators across objects at the
/// cost of one extra `Allocator`-sized field per object.
#define RK_ALLOCMODE_FULL        0

/// @brief No per-object allocator field. All allocations go through the global
/// (or thread-local) `alloc_ctx` pointer. Saves per-object overhead, but
/// swapping `alloc_ctx` at runtime affects all live objects.
#define RK_ALLOCMODE_NO_LOCAL    1

/// @brief All allocations unconditionally use thin wrappers over
/// `malloc`/`free`. `alloc_ctx` is `static const` and cannot be changed at
/// runtime. Allocator arguments to init macros are accepted syntactically but
/// ignored. Zero allocator overhead.
#define RK_ALLOCMODE_MALLOC_ONLY 2

#ifndef RK_ALLOCMODE
# define RK_ALLOCMODE RK_ALLOCMODE_FULL
#endif

#ifndef RK_ALLOC_MULTITHREADED
/// @brief Set to 1 to give `alloc_ctx` `thread_local` storage, enabling a
/// per-thread default allocator. Has no effect when
/// `RK_ALLOCMODE == RK_ALLOCMODE_MALLOC_ONLY` (no `alloc_ctx` exists).
# define RK_ALLOC_MULTITHREADED 0
#endif
#if RK_ALLOC_MULTITHREADED && RK_ALLOCMODE != RK_ALLOCMODE_MALLOC_ONLY
# define RK_alloc_tl thread_local
#else
# define RK_alloc_tl /* no thread local storage */
#endif

/// @defgroup alloc_failure_handlers Allocator Failure Handlers
/// @brief Customizable failure handlers for each allocator type.
/// All handlers receive `cond` (the null pointer returned on failure) plus
/// context arguments for custom logging or fallback logic. The handler
/// must not return normally — it must `abort()` or `longjmp()`.
/// @{
#ifndef RK_MALLOC_FAIL
/// @brief Malloc-based allocator failure handler.
# define RK_MALLOC_FAIL(cond, ctx, old_ptr, align, new_size)                   \
   do {                                                                        \
     (void)(ctx), (void)(old_ptr), (void)(align), (void)(new_size);            \
     rk_assert((cond) && "malloc allocation failure");                         \
     (cond) ? (void)0 : abort();                                               \
   } while (0)
#endif

#ifndef RK_MMAP_FAIL
/// @brief Page allocation failure handler (mmap / VirtualAlloc).
# define RK_MMAP_FAIL(cond, ctx, old_ptr, align, new_size)                     \
   do {                                                                        \
     (void)(ctx), (void)(old_ptr), (void)(align), (void)(new_size);            \
     rk_assert((cond) && "map allocation failure");                            \
     (cond) ? (void)0 : abort();                                               \
   } while (0)
#endif

#ifndef RK_ARENA_FAIL
/// @brief Arena allocator failure handler (`ctx` is the `Arena*`).
# define RK_ARENA_FAIL(cond, ctx, old_ptr, align, new_size)                    \
   do {                                                                        \
     (void)(ctx), (void)(old_ptr), (void)(align), (void)(new_size);            \
     rk_assert((cond) && "arena allocation failed");                           \
     (cond) ? (void)0 : abort();                                               \
   } while (0)
#endif

#ifndef RK_POOL_FAIL
/// @brief Pool allocator failure handler (`ctx` is the pool pointer).
# define RK_POOL_FAIL(cond, ctx, old_ptr, align, new_size)                     \
   do {                                                                        \
     (void)(ctx), (void)(old_ptr), (void)(align), (void)(new_size);            \
     rk_assert((cond) && "pool allocation failed");                            \
     (cond) ? (void)0 : abort();                                               \
   } while (0)
#endif
/// @}

/// @brief Body of `rk_mult(size_t x, size_t y)` in rk_defs.h.
/// Override to replace the overflow-checked multiplication used internally
/// when computing allocation sizes (e.g. `sizeof_n`). Must contain a
/// `return` statement yielding `x * y` as a `size_t`.
/// The default aborts on overflow (`x * y > SIZE_MAX`).
#ifndef RK_MULTIPLICATION_FUN
# define RK_MULTIPLICATION_FUN(x, y)                                           \
   if (x != 0 && y > SIZE_MAX / x) { abort(); }                                \
   return x * y
#endif

/// @brief Maximum load factor for `rk_dict` hash tables, in `(0, 1)`.
/// The table rehashes when `count / cap` exceeds this value.
#ifndef RK_DICT_MAX_LOAD_FACTOR
# define RK_DICT_MAX_LOAD_FACTOR 0.75f
#endif

/// @}
#endif /* RK_CONFIG_H */
