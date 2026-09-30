// Included only by mtu_impl.c / mtu_impl.cpp, which define RK_MULTI_TU and RK_IMPL before
// including this file -- this is the one TU per executable that provides the real, external
// definitions for every extern_fun/extern_var declared through mtu_shared.h.
#define RK_MULTI_TU
#define RK_IMPL
#include "mtu_shared.h"

int mtu_impl_populate(Dict(int, int) * d, Vec(int) * v) {
  *d = dict_init(int, int, 8);
  for (int i = 0; i < 16; ++i) {
    if (!dict_set(int, int, d, i, i * 10)) { return 0; }
  }
  if (dict_count(d) != 16u) { return 0; }

  *v = vec_init(int, 4);
  for (int i = 0; i < 10; ++i) { vec_push(*v, i); }
  if (vec_count(*v) != 10u) { return 0; }

  mtu_shared_counter = 42;
  return 1;
}
