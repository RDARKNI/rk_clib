#ifndef TRIAX_CONF
#define TRIAX_CONF

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"

// #define RK_MULTI_TU
// #define TRIAX_MULTI_TU
#include "../triax/triax.h"

#define RK_ALLOCMODE RK_ALLOCMODE_FULL
// #define RK_ALLOCMODE RK_ALLOCMODE_NO_LOCAL
// #define RK_ALLOCMODE RK_ALLOCMODE_MALLOC_ONLY
#define RK_DEBUG     2
#include "../include/rklib_includeall.h"

#if RK_ALLOCMODE != RK_ALLOCMODE_MALLOC_ONLY
static Allocator alloc_cpy;
# define RK_IFNMALLOC(...) __VA_ARGS__
#else
# define RK_IFNMALLOC(...)
#endif
#define SWAP_ALLOC(alloc)                                                                          \
  do { alloc_cpy = alloc_ctx, alloc_ctx = (alloc); } while (0)

#define DO_SKIP 1

#ifndef TRIAX_MULTI_TU
// clang-format off
# include "test_rk_arena.c"
# include "test_rk_arenastack.c"
# include "test_rk_bitset.c"
# include "test_rk_bst.c"
# include "test_rk_dict.c"
# include "test_rk_pool.c"
# include "test_rk_string.c"
# include "test_rk_vec.c"
# include "test_rk_test.c"
#else
#endif
#endif
