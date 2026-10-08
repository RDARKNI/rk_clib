// Shared declarations for the multi-TU linkage tests. This header is included, unmodified, by
// every translation unit involved in a given multi_tu executable (the RK_IMPL-defining TU, the
// consuming TU(s), and main), each of which may be compiled as C or C++, in any combination. It
// exists to verify that RK_MULTI_TU's extern_fun/extern_var split actually produces a program
// that links and behaves correctly -- something no other test in this suite exercises, since every
// other test binary is a single translation unit built without RK_MULTI_TU at all.
//
// RK_MULTI_TU must be defined by the including .c/.cpp file before this header is included; RK_IMPL
// must additionally be defined by exactly one of them (the "impl" TU).
#ifndef MTU_SHARED_H
#define MTU_SHARED_H

#include "rklib.h"

// A DICT_DEFINE(...) instantiation's generated functions are themselves rklib_fun-based (see
// rk_defs.h's RK_MULTI_TU/RK_IMPL split), and RK__DICT_DEFINE wraps them in RK_EXTERNC_BEG/END
// internally -- so this single instantiation, included from every TU below, is enough to exercise
// cross-TU linkage for library-generated container code, not just the library's own hand-written
// foundational functions.
extern_fun unsigned mtu_hash(int key) { return (unsigned)key; }
extern_fun int      mtu_cmp(int a, int b) { return a != b; }
DICT_DEFINE(int, int, mtu_hash, mtu_cmp)

// A plain extern_var/extern_def global, exercised the same way rk_alloc.h's own alloc_ctx is: one
// real definition in the RK_IMPL TU, a linking declaration everywhere else. Mutating it in the impl
// TU and reading it back from the use TU is the only way to prove this is a single shared object
// and not an accidental per-TU copy.
extern_var int mtu_shared_counter extern_def(0);

RKI_HEADER_BEGIN

// Defined in mtu_impl.{c,cpp}. Populates *d and *v, using the generated dict_int_int_* functions
// and Vec's own rklib_fun accessors from within the RK_IMPL TU.
int mtu_impl_populate(Dict(int, int)* d, Vec(int)* v);

// Defined in mtu_use.{c,cpp}. Reads *d and *v back via the exact same generated/library functions,
// called from a TU that does NOT define RK_IMPL -- these calls only succeed if the linker correctly
// resolves them to mtu_impl_populate's TU.
int mtu_use_verify(Dict(int, int)* d, Vec(int)* v);

RKI_HEADER_END

#endif // MTU_SHARED_H
