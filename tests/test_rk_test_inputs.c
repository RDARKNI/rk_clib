#ifndef TEST_TEST_INPUTS_H
#define TEST_TEST_INPUTS_H
#include "conf.h"

RK__IGNWARN_CLANG_BEG("-Wunused-variable")

triax_test(test_crash, test0, .isolation = TRIAX_ISOLATION_OFF) {
  triax_assert_fault(TRIAX_FAULT_ANY, assert(0););
  triax_expect_true(0);
}
// triax_test(test_crash, test1, .isolation = TRIAX_ISOLATION_OFF) {
//   triax_assert_exit(1, exit(1););
//   triax_expect_true(0);
// }
#ifndef TEST_COMPERRORS
# define TEST_COMPERRORS 1
#endif
static int         _int  = 10;
static void*       _ptr  = (void*)20;
static const void* _cptr = (const void*)20;
static char*       _str  = (char*)"hello";
static const char* _cstr = "hello";
triax_test(test_invalid_inputs, test_ptr_ptr_eq_pass) {
  triax_expect_eq(_ptr, _ptr);
  triax_expect_eq(_ptr, _cptr);
  triax_expect_eq(_cptr, _ptr);
  triax_expect_eq(_cptr, _cptr);
}
triax_test(test_invalid_inputs, test_ptr_ptr_neq_fail) {
  triax_expect_neq(_ptr, _ptr);
  triax_expect_neq(_ptr, _cptr);
  triax_expect_neq(_cptr, _ptr);
  triax_expect_neq(_cptr, _cptr);
}
triax_test(test_invalid_inputs, test_ptr_ptr_gt_fail) {
  triax_expect_gt(_ptr, _ptr);
  triax_expect_gt(_ptr, _cptr);
  triax_expect_gt(_cptr, _ptr);
  triax_expect_gt(_cptr, _cptr);
}
triax_test(test_invalid_inputs, test_ptr_ptr_geq_pass) {
  triax_expect_geq(_ptr, _ptr);
  triax_expect_geq(_ptr, _cptr);
  triax_expect_geq(_cptr, _ptr);
  triax_expect_geq(_cptr, _cptr);
}
triax_test(test_invalid_inputs, test_ptr_ptr_lt_fail) {
  triax_expect_lt(_ptr, _ptr);
  triax_expect_lt(_ptr, _cptr);
  triax_expect_lt(_cptr, _ptr);
  triax_expect_lt(_cptr, _cptr);
}
triax_test(test_invalid_inputs, test_ptr_ptr_leq_pass) {
  triax_expect_leq(_ptr, _ptr);
  triax_expect_leq(_ptr, _cptr);
  triax_expect_leq(_cptr, _ptr);
  triax_expect_leq(_cptr, _cptr);
}
#ifndef __cplusplus
# define ZTMP(T) (T){0}
#else
# define ZTMP(T) T{0}
#endif
typedef void*              vptr;
typedef const void*        cvptr;
typedef char*              str;
typedef const char*        cstr;
typedef long double        ldouble;
typedef unsigned char      uchar;
typedef unsigned short     ushort;
typedef unsigned int       uint;
typedef unsigned long      ulong;
typedef unsigned long long ullong;
typedef long long          llong;

#define UNSIGNED_INTEGRALS(X, ...)                                                                 \
  X(uchar, __VA_ARGS__)                                                                            \
  X(ushort, __VA_ARGS__)                                                                           \
  X(uint, __VA_ARGS__)                                                                             \
  X(ulong, __VA_ARGS__)                                                                            \
  X(ulong, __VA_ARGS__)

#define SIGNED_INTEGRALS(X, ...)                                                                   \
  X(char, __VA_ARGS__)                                                                             \
  X(short, __VA_ARGS__)                                                                            \
  X(int, __VA_ARGS__)                                                                              \
  X(long, __VA_ARGS__)                                                                             \
  X(llong, __VA_ARGS__)

#define FLOATS(X, ...)                                                                             \
  X(float, __VA_ARGS__)                                                                            \
  X(double, __VA_ARGS__)                                                                           \
  X(ldouble, __VA_ARGS__)

#define POINTERS(X, ...)                                                                           \
  X(vptr, __VA_ARGS__)                                                                             \
  X(cvptr, __VA_ARGS__)

#define CSTRINGS(X, ...)                                                                           \
  X(str, __VA_ARGS__)                                                                              \
  X(cstr, __VA_ARGS__)

// todo line string
#define ATYPES(X, T, U)                                                                            \
  X(eq, T, U)                                                                                      \
  X(neq, T, U)                                                                                     \
  X(gt, T, U)                                                                                      \
  X(geq, T, U)                                                                                     \
  X(lt, T, U)                                                                                      \
  X(leq, T, U)

#define ENSURE_NONCOMPATIBLE(ATYPE, T, U)                                                          \
  triax_test(test_invalid_inputs, test_ptr_int_##ATYPE##_##T##_##U##_comperr) {                    \
    triax_expect_##ATYPE(ZTMP(T), ZTMP(U));                                                        \
    triax_expect_##ATYPE(ZTMP(U), ZTMP(T));                                                        \
  }

#if 0
ATYPES(ENSURE_NONCOMPATIBLE, int, vptr)
ATYPES(ENSURE_NONCOMPATIBLE, int, cvptr)
ATYPES(ENSURE_NONCOMPATIBLE, int, str)
ATYPES(ENSURE_NONCOMPATIBLE, int, cstr) // 48 errors each

ATYPES(ENSURE_NONCOMPATIBLE, char, vptr)
ATYPES(ENSURE_NONCOMPATIBLE, char, cvptr)
ATYPES(ENSURE_NONCOMPATIBLE, char, str)
ATYPES(ENSURE_NONCOMPATIBLE, char, cstr)

ATYPES(ENSURE_NONCOMPATIBLE, float, vptr)
ATYPES(ENSURE_NONCOMPATIBLE, float, cvptr)
ATYPES(ENSURE_NONCOMPATIBLE, float, str)
ATYPES(ENSURE_NONCOMPATIBLE, float, cstr)

ATYPES(ENSURE_NONCOMPATIBLE, double, vptr)
ATYPES(ENSURE_NONCOMPATIBLE, double, cvptr)
ATYPES(ENSURE_NONCOMPATIBLE, double, str)
ATYPES(ENSURE_NONCOMPATIBLE, double, cstr)

ATYPES(ENSURE_NONCOMPATIBLE, ldouble, vptr)
ATYPES(ENSURE_NONCOMPATIBLE, ldouble, cvptr)
ATYPES(ENSURE_NONCOMPATIBLE, ldouble, str)
ATYPES(ENSURE_NONCOMPATIBLE, ldouble, cstr)

// expecting 240 errors
#endif
triax_test(test_invalid_inputs, test_wrong_string_comparison) {
  const char* s = "Hello, this is fine!";
  void*       p = (void*)"This is garbage";
  triax_expect_eq(p, s); // pointer comparison - correct
  triax_expect_eq(s, p); // string comparison - dangerous?

  triax_expect_eq(s, s); // string comparison - dangerous
}
triax_test(test_invalid_inputs, test_ptr_str) {
  void* _ptr = (void*)"hello";
  triax_expect_eq(_ptr, _ptr);
  char*       _str  = (char*)_ptr;
  const char* _cstr = _str;

  // expect pointer comparison
  triax_expect_eq(_ptr, _str);
  triax_expect_eq(_str, _ptr);

  // expect string comparison
  triax_expect_eq(_str, _str);
  triax_expect_eq(_str, _cstr);
  triax_expect_eq(_cstr, _str);
  triax_expect_eq(_cstr, _cstr);

#ifdef __cplusplus
  std::string _cppstr = "hello";
  triax_expect_eq(_cppstr, _cppstr);
  triax_expect_eq(_cppstr, _str);
  triax_expect_eq(_str, _cppstr);
  triax_expect_eq(_cppstr, _cstr);
  triax_expect_eq(_cstr, _cppstr);
# if __cplusplus >= 201703L
  std::string_view _cppstrv = "hello";
  triax_expect_eq(_cppstrv, _str);
  triax_expect_eq(_str, _cppstrv);
  triax_expect_eq(_cppstrv, _cstr);
  triax_expect_eq(_cstr, _cppstrv);
  triax_expect_eq(_cppstrv, _cppstrv);
  triax_expect_eq(_cppstr, _cppstrv);
  triax_expect_eq(_cppstrv, _cppstr);
# endif
# if TEST_COMPERRORS

  // triax_expect_eq(_cppstr, _ptr);
  // triax_expect_eq(_ptr, _cppstr);
# endif
#endif
}
triax_test(test_invalid_inputs, test_signed_unsigned) { triax_expect_gt(10ull, -1); }
triax_test(test_invalid_inputs, test_float_int) { // triax_expect_gt(4., 5);
}
#endif
