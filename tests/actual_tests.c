// #define RK_ALLOCMODE RK_ALLOCMODE_FULL
// #define RK_ALLOCMODE RK_ALLOCMODE_NO_LOCAL
// #define RK_ALLOCMODE RK_ALLOCMODE_MALLOC_ONLY
#define _GNU_SOURCE
#define RK_IMPL
// #define RK__TESTDUMMY
#include "../include/rklib_includeall.h"
// #include "../rk_test/rk_test_dummy.h"
// clang-format off
#include "test_rk_arenalist.c"
#include "test_rk_string.c"
#include "test_rk_arena.c"
#include "test_rk_bitset.c"
#include "test_rk_dict.c"
#include "test_rk_pool.c"
#include "test_rk_vec.c"
#include "test_rk_bst.c"

// clang-format on
// int* test2(int* arr, size_t len) { return rk_arrdup(arr, len); }
//
// int* test(int* arr, size_t len) { return rk_arrdup(arr, len); }
//
// typedef void*(alloc_test_f)(size_t size);
// void* f(size_t size) rk_alloc_size(1);
// rk_alloc_size(1) void* f(size_t size) { return malloc(10); }

int main() {
  // int  dstarr[10];
  // int* dst = dstarr;
  // int  srcarr[1];
  // int* src  = srcarr;
  // int* ndst = rk_memcpy(dst, src, 1);
  // printf("%zu\n", __builtin_object_size(dstarr, 0)); /* 40 */
  // printf("%zu\n", __builtin_object_size(dst, 0));    /* 40 */
  // printf("%zu\n", __builtin_object_size(srcarr, 0)); /* 4*/
  // printf("%zu\n", __builtin_object_size(src, 0));    /*4*/
  // printf("%zu\n", __builtin_object_size(ndst, 0));   /*?*/
  // printf("%zu\n", __builtin_object_align(ndst, 0));  /*?*/
  // int* v = vec_init(int, 11);
  // printf("%zu\n", __builtin_object_size(v, 0)); /*?*/
  // int* u = vec_copy(v);
  // printf("%zu\n", __builtin_object_size(u, 0)); /*?*/
  char* sss = alloc_new(char, 5);
  char* sfj = (char*)RK__call_alloc(10, 16, alloc_ctx.vtab, alloc_ctx.ctx);
  printf("%zu\n", __builtin_object_size(sfj, 0)); /*?*/

  printf("%zu\n", __builtin_object_size(sss, 0)); /*?*/
  Str s = str_from_literal("hello");
  printf("%zu\n", __builtin_dynamic_object_size(s.str, 0)); /*?*/

  // volatile int64_t start = RKT_now_ms();
  // RK_RUN_TESTS(NULL, 0);
  // printf("Total time: %lld ms\n", RKT_now_ms() - start);
}
