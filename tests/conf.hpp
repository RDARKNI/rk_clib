#ifndef TRIAX_CONF
#define TRIAX_CONF
#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#ifdef __clang__
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

// #define RK_MULTI_TU
// #define TRIAX_MULTI_TU
#include "../triax/triax.h"

#define RK_DEBUG     2

// Unlike conf.h (the C suite's counterpart), nothing in the C++ suite uses
// RK_ALLOCMODE, Allocator, SWAP_ALLOC, or RK_IFNMALLOC — this file never
// actually includes any rk_clib allocator header, so there's nothing here
// for those to condition on. An RK_ALLOCMODE-gated block was previously
// copy-pasted in from conf.h anyway; it happened to reference
// RK_ALLOCMODE_FULL/RK_ALLOCMODE_MALLOC_ONLY before they were ever defined
// (only triax.h is included above), so its #if was always silently false
// regardless of the real mode. Removed rather than "fixed", since making it
// real would require pulling in rk_alloc.h here for no current benefit.

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

#endif
