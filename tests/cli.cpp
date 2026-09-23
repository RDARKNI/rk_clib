#define RK_IMPL
#define TRIAX_IMPL
#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif
#include <array>
#include <string>
#include <vector>
#ifdef __has_include
# if __has_include(<string_view>)
#  include <string_view>
# endif
#endif
#include "conf.hpp"
/*
gcc cli.c test_rk_arena.c test_rk_arenastack.c test_rk_bitset.c test_rk_bst.c test_rk_dict.c
test_rk_pool.c test_rk_string.c test_rk_test.c test_rk_vec.c -O3 -g3 && ./a.out
*/
triax_test(noisolation_recovery, recover_abort_testerr) { abort(); }
triax_test(noisolation_recovery, recover_segfault_testerr) { raise(SIGSEGV); }
triax_test(noisolation_recovery, recover_abort_ucrash) { triax_expect_true((abort(), 1)); }
triax_test(noisolation_recovery, recover_segfault_ucrash) {
  triax_expect_true((raise(SIGSEGV), 1));
}
/*
  char          TRIAXI_dummy; ///< prevents empty designated-initialiser list
  bool          skip;       ///< skip this test/suite; no-op at global level
  TRIAX_Verbosity verbosity;  ///< when to print captured output — cascades via
                            ///< INHERIT (see @ref TRIAX_Verbosity)
  TRIAX_Isolation isolation;  ///< whether to fork per test — cascades via INHERIT
                            ///< (see @ref TRIAX_Isolation)
  uint32_t timeout_ms; ///< timeout in ms; first non-zero value in cascade wins;
                       ///< 0 = no timeout
  const char*     tags; ///< comma-separated tags, e.g. `"fast,unit"`; at global
                        ///< level acts as a filter
  TRIAX_FixtureFunc init, fini;
*/
static const int static_params_int_range[5] = {1, 2, 3, 4, 5};
triax_test(test_parameterised, test0, .parameterize(static_params_int_range)) {
  const int* p = triax_param(int);
  triax_expect_eq(*p, 1);
}
struct Point { size_t x, y; };
#define PCOUNT 100
triax_test(test_parameterised, test1) {
  const struct Point* p = triax_param(struct Point);
  triax_assert_leq(p->x, p->y);
}
// clang-format on
triax_test(test_parameterised2, test1,
           .skip(0)
               .verbosity(TRIAX_VERBOSITY_INHERIT)
               .isolation(TRIAX_ISOLATION_INHERIT)
               .timeout_ms(0)
               .tags(NULL)
               .init(NULL)
               .fini(NULL)) {
  const struct Point* p = triax_param(struct Point);
  triax_assert_leq(p->x, p->y);
}
triax_suite(test_parameterised, .skip(0)
                                    .verbosity(TRIAX_VERBOSITY_INHERIT)
                                    .isolation(TRIAX_ISOLATION_INHERIT)
                                    .timeout_ms(0)
                                    .tags(NULL));
int xxx[2];
triax_test(asdfdsafdas, fff, .skip(false).parameterize(xxx)) {}
triax_test(test_floats, test1) {
  int  i = 0;
  char c = 0;
  triax_assert_leq((bool)i, c);
}
// triax_test(test_floats, test0) {
//   float  f;
//   double d;
//   triax_assert_leq(f, d);
// }
triax_test(stdstr, test0) {
  std::string a = "fff", b = "fff";
  triax_assert_eq(a, b);
#ifdef __cpp_lib_string_view
  std::string_view av = "fff", bv = "fffo";
  triax_assert_eq(av, bv);
#endif
  triax_assert_streq(a, b);
  triax_expect_streq(triax_read_stdout(), a);
  triax_assert_eq(a, b);
}
triax_test(test_invalid_inputs, test_ptr_int) {
  int         _int  = 10;
  void*       _ptr  = (void*)10;
  void*       _cptr = (void*)10;
  char*       _str  = (char*)"hello";
  const char* _cstr = "hello";
  triax_assert_eq(_ptr, _ptr);
  triax_assert_eq(_ptr, _cptr);
  triax_assert_eq(_cptr, _ptr);
  triax_assert_eq(_cptr, _cptr);
  triax_assert_neq(_ptr, _ptr);
  triax_assert_neq(_ptr, _cptr);
  triax_assert_neq(_cptr, _ptr);
  triax_assert_neq(_cptr, _cptr);
  triax_assert_gt(_ptr, _ptr);
  triax_assert_gt(_ptr, _cptr);
  triax_assert_gt(_cptr, _ptr);
  triax_assert_gt(_cptr, _cptr);
  triax_assert_geq(_ptr, _ptr);
  triax_assert_geq(_ptr, _cptr);
  triax_assert_geq(_cptr, _ptr);
  triax_assert_geq(_cptr, _cptr);
  triax_assert_lt(_ptr, _ptr);
  triax_assert_lt(_ptr, _cptr);
  triax_assert_lt(_cptr, _ptr);
  triax_assert_lt(_cptr, _cptr);
  triax_assert_leq(_ptr, _ptr);
  triax_assert_leq(_ptr, _cptr);
  triax_assert_leq(_cptr, _ptr);
  triax_assert_leq(_cptr, _cptr);
  // expect error
  // triax_assert_eq(_int, _ptr);
  // triax_assert_eq(_ptr, _int);
  // triax_assert_eq(_int, _cptr);
  // triax_assert_eq(_cptr, _int);
  // triax_assert_eq(_int, _str);
  // triax_assert_eq(_str, _int);
  // triax_assert_eq(_int, _cstr);
  // triax_assert_eq(_cstr, _int);
}
triax_test(test_invalid_inputs, test_ptr_str) {
  void* _ptr = (void*)"hello";
  triax_assert_eq(_ptr, _ptr);
  char*       _str  = (char*)_ptr;
  const char* _cstr = _str;
  // expect pointer comparison
  triax_assert_eq(_ptr, _str);
  triax_assert_eq(_str, _ptr);
  // expect string comparison
  triax_assert_eq(_str, _str);
  triax_assert_eq(_str, _cstr);
  triax_assert_eq(_cstr, _str);
  triax_assert_eq(_cstr, _cstr);
#ifdef __cplusplus
  std::string _cppstr = "hello";
  triax_assert_eq(_cppstr, _cppstr);
  triax_assert_eq(_cppstr, _str);
  triax_assert_eq(_str, _cppstr);
  triax_assert_eq(_cppstr, _cstr);
  triax_assert_eq(_cstr, _cppstr);
# ifdef __cpp_lib_string_view
  std::string_view _cppstrv = "hello";
  triax_assert_eq(_cppstrv, _str);
  triax_assert_eq(_str, _cppstrv);
  triax_assert_eq(_cppstrv, _cstr);
  triax_assert_eq(_cstr, _cppstrv);
  triax_assert_eq(_cppstrv, _cppstrv);
  triax_assert_eq(_cppstr, _cppstrv);
  triax_assert_eq(_cppstrv, _cppstr);
# endif
  // expect  error(?)
  // triax_assert_eq(_cppstr, _ptr);
  // triax_assert_eq(_ptr, _cppstr);
#endif
}
triax_test(test_invalid_inputs, test_cstr_stdstr_pass) {
  std::string stda = "okasdfdsafasdfhjadskfldsafdfsafadsfadsf", stdb = "nok";
  const char *cstra = "okasdfdsafasdfhjadskfldsafdfsafadsfadsf", *cstrb = "nok";
  triax_expect_eq(stda, cstra);
  triax_expect_eq(cstra, stda);
  triax_expect_neq(stda, cstrb);
  triax_expect_neq(cstrb, stda);
  triax_expect_strneq(triax_str(cstra, 5), cstra);
}
triax_test(test_invalid_inputs, test_float_int) {
  triax_expect_neq(nullptr, nullptr);
  triax_expect_eq(nullptr, nullptr);
  triax_expect_eq((char*)1, nullptr);
  triax_expect_eq((const char*)1, nullptr);
  triax_expect_eq((const char*)1, (const char*)2);
}
triax_test(edge_case, long_string) {
  char buf[16384 * 2 + 1];
  memset(buf, 'a', 16384);
  memset(buf + 16384, 'b', 16384);
  buf[sizeof(buf) - 1] = '\0';
  triax_assert_neq(buf, buf);
}
triax_test(edge_case, long_string_view) {
  char buf[16384 * 2 + 1];
  memset(buf, 'a', 16384);
  memset(buf + 16384, 'b', 16384);
  buf[sizeof(buf) - 1] = '\0';
  std::string S{buf};
  triax_assert_neq(S, S);
}
struct Unprintable {
  int  x;
  bool operator==(const Unprintable& other) const { return x == other.x; }
  bool operator==(const int& other) const { return x == other; }
};
triax_test(cppmisc, unprintable) {
  Unprintable a{0}, b{1}, c{0};
  int         d = 2;
  triax_expect_eq(a, b);
  triax_expect_eq(a, c);
  triax_expect_eq(a, d);
}
triax_test(cppmisc, nullptr) {
  void* a = (void*)1;
  triax_expect_eq(a, nullptr);
  triax_expect_eq(nullptr, nullptr);
  char* c = (char*)"asdf";
  triax_expect_eq(c, nullptr);
  triax_expect_eq(nullptr, c);
  Triax_Str s = "asdf";
  triax_expect_eq(s, c);
  triax_expect_lt(s, c);
  triax_expect_gt(s, c);
  triax_expect_neq(s, c);
}
triax_test(string_new, n) {
  char buf[10] = {};
  triax_expect_streq("ok", std::string("nok"));
  triax_expect_streq("ok", buf);
  Triax_Str s = "asdf";
  struct D {
    bool operator==(D) const { return false; }
  } d;
  // Exercises the generic C++ comparison/printing fallback in every C++ mode.
  triax_expect_eq(d, d);
#ifdef __cpp_lib_string_view
  std::string_view a = "hello";
  // Exercise Triax_Str/string_view interoperability directly. Do not introduce
  // a third type convertible to string_view here: pre-C++20 and C++20+
  // overload resolution intentionally differ, and the framework must not make
  // that an accidental portability requirement of this test.
  triax_expect_true(a == s);
#endif
  triax_expect_eq(s, s);
}
triax_test(newest, n) {
  triax_expect_eq(nullptr, nullptr);
  triax_expect_eq(nullptr, (char*)0);
  triax_expect_eq(nullptr, (void*)0);
  triax_expect_eq((char*)0, nullptr);
  triax_expect_eq((void*)0, nullptr);
  triax_expect_eq((char*)0, Triax_Str(nullptr, 0));
  triax_expect_eq((char*)0, Triax_Str(nullptr));
}
triax_test(array_comps, n) {
  std::array<int, 3> arr1   = {1, 2, 3};
  int                arr2[] = {1, 2, 4};
  triax_expect_arreq(arr1, arr2);
  std::array<std::string, 3> arr3   = {"hello", "world", "?!!"};
  std::string                arr4[] = {"nohello", "world", "..."};
  triax_expect_arreq(arr3, arr4);
  triax_expect_arrneq(arr3, arr4);
  // std::array<Point, 3> arr5   = {Point{1, 2}, Point{3, 4}, Point{5, 6}};
  // Point                arr6[] = {{1, 2}, {3, 4}, {0, 0}};
  // triax_expect_arreq(arr5, arr6);
}
triax_test(array_comps, n2) {
  int arr1[] = {1, 2, 3};
  int arr2[] = {1, 2, 4};
  triax_expect_arrneq(arr1, arr2);
}
triax_test(array_comps, n3) {
  int arr1[] = {1, 2, 3};
  int arr2[] = {1, 2, 3};
  triax_expect_arrneq(arr1, arr2);
}
triax_test(array_comps, n4) {
  int arr1[] = {1, 2, 3};
  triax_expect_arreq(arr1, (std::array<int, 3>{1, 2, 3}));
}
triax_test(array_compsdyn, c_array_vector_eq_pass) {
  int              a[] = {1, 2, 3};
  std::vector<int> b   = {1, 2, 3};
  triax_expect_arreq(a, b);
  triax_expect_arreq(b, a);
}
triax_test(array_compsdyn, vector_vector_eq_pass) {
  std::vector<int> a = {1, 2, 3};
  std::vector<int> b = {1, 2, 3};
  triax_expect_arreq(a, b);
}
triax_test(array_compsdyn, stdarray_vector_eq_pass) {
  std::array<int, 3> a = {{1, 2, 3}};
  std::vector<int>   b = {1, 2, 3};
  triax_expect_arreq(a, b);
  triax_expect_arreq(b, a);
}
triax_test(array_compsdyn, c_array_vector_neq_pass_elements) {
  int              a[] = {1, 2, 3};
  std::vector<int> b   = {1, 9, 3};
  triax_expect_arrneq(a, b);
  triax_expect_arrneq(b, a);
}
triax_test(array_compsdyn, vector_vector_neq_pass_elements) {
  std::vector<int> a = {1, 2, 3};
  std::vector<int> b = {1, 2, 4};
  triax_expect_arrneq(a, b);
}
triax_test(array_compsdyn, stdarray_vector_neq_pass_elements) {
  std::array<int, 3> a = {{1, 2, 3}};
  std::vector<int>   b = {1, 2, 4};
  triax_expect_arrneq(a, b);
  triax_expect_arrneq(b, a);
}
triax_test(array_compsdyn, c_array_vector_neq_pass_size) {
  int              a[] = {1, 2, 3};
  std::vector<int> b   = {1, 2, 3, 4};
  triax_expect_arrneq(a, b);
  triax_expect_arrneq(b, a);
}
triax_test(array_compsdyn, vector_vector_neq_pass_size) {
  std::vector<int> a = {1, 2, 3};
  std::vector<int> b = {1, 2};
  triax_expect_arrneq(a, b);
  triax_expect_arrneq(b, a);
}
triax_test(array_compsdyn, stdarray_vector_neq_pass_size) {
  std::array<int, 3> a = {{1, 2, 3}};
  std::vector<int>   b = {1, 2, 3, 4};
  triax_expect_arrneq(a, b);
  triax_expect_arrneq(b, a);
}
triax_test(array_compsdyn, vector_eq_fail_elements) {
  std::vector<int> a = {1, 2, 3};
  std::vector<int> b = {1, 9, 3};
  triax_expect_arreq(a, b); // fail: same size, element mismatch
}
triax_test(array_compsdyn, c_array_vector_eq_fail_elements) {
  int              a[] = {1, 2, 3};
  std::vector<int> b   = {1, 2, 4};
  triax_expect_arreq(a, b); // fail: same size, element mismatch
}
triax_test(array_compsdyn, stdarray_vector_eq_fail_elements) {
  std::array<int, 3> a = {{1, 2, 3}};
  std::vector<int>   b = {1, 9, 3};
  triax_expect_arreq(a, b); // fail: same size, element mismatch
}
triax_test(array_compsdyn, vector_eq_fail_size) {
  std::vector<int> a = {1, 2, 3};
  std::vector<int> b = {1, 2};
  triax_expect_arreq(a, b); // fail: runtime size mismatch
}
triax_test(array_compsdyn, c_array_vector_eq_fail_size) {
  int              a[] = {1, 2, 3};
  std::vector<int> b   = {1, 2, 3, 4};
  triax_expect_arreq(a, b); // fail: runtime size mismatch
}
triax_test(array_compsdyn, stdarray_vector_eq_fail_size) {
  std::array<int, 3> a = {{1, 2, 3}};
  std::vector<int>   b = {1, 2};
  triax_expect_arreq(a, b); // fail: runtime size mismatch
}
triax_test(array_compsdyn, vector_neq_fail_equal) {
  std::vector<int> a = {1, 2, 3};
  std::vector<int> b = {1, 2, 3};
  triax_expect_arrneq(a, b); // fail: equal
}
triax_test(array_compsdyn, c_array_vector_neq_fail_equal) {
  int              a[] = {1, 2, 3};
  std::vector<int> b   = {1, 2, 3};
  triax_expect_arrneq(a, b); // fail: equal
}
triax_test(array_compsdyn, stdarray_vector_neq_fail_equal) {
  std::array<int, 3> a = {{1, 2, 3}};
  std::vector<int>   b = {1, 2, 3};
  triax_expect_arrneq(a, b); // fail: equal
}
int main(int argc, char* argv[]) {
  Triax_RunConfig defaults = {};
  defaults.outpaths        = {TRIAX_OUTPATH_DEFAULT, "./outputs_isolation/jsonout.json",
                              "./outputs_isolation/tapout.tap", "./outputs_isolation/junitout.xml"};
  defaults.attrs.isolation = TRIAX_ISOLATION_ON;
  triax_run(triax_parse_argv(argc, argv, defaults));
}
