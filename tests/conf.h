#ifndef TRIAX_CONF
#define TRIAX_CONF

#ifdef __clang__
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
// #define RK_MULTI_TU
// #define TRIAX_MULTI_TU
#include "../triax/triax.h"

#ifndef RK_CUSTOM_ALLOCATORS
# define RK_CUSTOM_ALLOCATORS 1
#endif

#define RKLIB_DEBUG 2
#include "../include/rklib.h"

#if RK_CUSTOM_ALLOCATORS
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
# include "test_rk_avl.c"
# include "test_rk_bitset.c"
# include "test_rk_bst.c"
# include "test_rk_deque.c"
# include "test_rk_dict.c"
# include "test_rk_heap.c"
# include "test_rk_pool.c"
# include "test_rk_rbt.c"
# include "test_rk_string.c"
# include "test_rk_vec.c"
#else
#endif
#endif
