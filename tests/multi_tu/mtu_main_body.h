// Included only by mtu_main.c / mtu_main.cpp. Defines RK_MULTI_TU (not RK_IMPL) since it only needs
// the Dict(int,int)/Vec(int) type definitions to declare local variables here -- it never calls a
// generated container function directly, only the two hand-written functions that do.
#define RK_MULTI_TU
#include "mtu_shared.h"

int main(void) {
  Dict(int, int) d;
  Vec(int)       v = rk_null;

  if (!mtu_impl_populate(&d, &v)) { return 1; }
  if (!mtu_use_verify(&d, &v)) { return 1; }

  dict_release(int, int, &d);
  vec_release(v);
  return 0;
}
