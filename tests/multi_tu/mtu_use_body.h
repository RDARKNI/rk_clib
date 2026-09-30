// Included only by mtu_use.c / mtu_use.cpp, which define RK_MULTI_TU (but NOT RK_IMPL) before
// including this file -- every extern_fun call and extern_var read below resolves to the
// definition compiled into mtu_impl.{c,cpp}'s object file, not to anything in this TU.
#define RK_MULTI_TU
#include "mtu_shared.h"

int mtu_use_verify(Dict(int, int) * d, Vec(int) * v) {
  for (int i = 0; i < 16; ++i) {
    int* val = dict_get(int, int, d, i);
    if (!val || *val != i * 10) { return 0; }
  }
  if (dict_count(d) != 16u) { return 0; }

  if (vec_count(*v) != 10u) { return 0; }
  for (int i = 0; i < 10; ++i) {
    if ((*v)[i] != i) { return 0; }
  }

  // Proves mtu_shared_counter is one shared object across TUs, not a per-TU copy: mtu_impl_populate
  // set it to 42 in a different translation unit entirely.
  if (mtu_shared_counter != 42) { return 0; }

  return 1;
}
