#ifndef TRIAX_CONF
#define TRIAX_CONF
#define _GNU_SOURCE

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"

// #define RK_MULTI_TU
// #define TRIAX_MULTI_TU
#include "../triax/triax.h"

#define RK_ALLOCMODE RK_ALLOCMODE_FULL
// #define RK_ALLOCMODE RK_ALLOCMODE_NO_LOCAL
// #define RK_ALLOCMODE RK_ALLOCMODE_MALLOC_ONLY
#define RK_DEBUG     2

#if RK_ALLOCMODE != RK_ALLOCMODE_MALLOC_ONLY
static Allocator alloc_cpy;
# define RK_IFNMALLOC(...) __VA_ARGS__
#else
# define RK_IFNMALLOC(...)
#endif
#define SWAP_ALLOC(alloc)                                                                          \
  do { alloc_cpy = alloc_ctx, alloc_ctx = (alloc); } while (0)

#define DO_SKIP         1

#define RK_DO_PRAGMA(a) _Pragma(#a)
#ifdef __clang__
/// Disables warnings about:
/// - Zero-argument variadic macro arguments that work on all targeted platforms
/// - Overriding keywords as macros (typeof with certain flags)
/// - Unknown warning options (for the next warning on older compiler versions)
/// - Warnings about c2x, c2y and c23 extensions (namely countof, for which a
///   fallback exists and attribute syntax), since they're used conditionally
///   and portably.
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
# define RK__IGNWARN_CLANG(warn, ...)
#endif

#ifndef TRIAX_MULTI_TU
// clang-format off
//# include "test_rk_test.c"
#else
#endif
#endif
