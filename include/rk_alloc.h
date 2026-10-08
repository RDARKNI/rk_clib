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
#include "rk_defs.h"
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
# define RK_IFALLOC(...)                    __VA_ARGS__
# define RKI_set_alloc_fallback(_alloc)                                                            \
   ((void)(rk_likely((_alloc).vtab)                                                                \
               ? alloc_ctx                                                                         \
               : (RKI_assert_allocator_valid(alloc_ctx), (_alloc) = alloc_ctx)))
#else
# define RKI_allocator_disabled_assert()    static_assert_expr(0, "Allocators Disabled")
# define RKI_REQUIRE_CUSTOM_ALLOCATORS(...) ((void*)RKI_allocator_disabled_assert())
# define RKI_allocatorof(self)              ((void)(self), alloc_ctx)
# define RKI_assert_allocator_valid(_alloc) ((void)0)
# define RK_IFALLOC(...)
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
