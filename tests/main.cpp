#define RK_IMPL
#include "../rk_test/rk_test.h"
// clang-format off
#define RKT_FULL_TEST 1 
#if RKT_FULL_TEST
#include "test_rk_string.c"
#include "test_rk_arena.c"
#include "test_rk_bitset.c"
#include "test_rk_dict.c"
#include "test_rk_vec.c"
#include "test_rk_test.c"
#endif
// clang-format on 

RK_REGISTER_TEST("testname now", tt, .isolation = RK_ISOLATION_OFF) {
  rk_expect_eq(0, 1);
  rk_expect_eq(0, 1);
  rk_expect_eq(0, 1);
  rk_expect_eq(0, 1);
}
RK_REGISTER_TEST("testname now", tt2, .isolation = RK_ISOLATION_OFF) {
  rk_expect_eq(0, 1);
}
RK_REGISTER_TEST("testname now", ttf, .isolation = RK_ISOLATION_OFF) {
  rk_assert_eq(0, 1);
}
RK_REGISTER_TEST("testname now", lots) {
  for (int i = 0; i < 10; ++i) {
    rk_expect_eq(i, 1);
    rk_expect_eq(i, i);
  }
}
int do_stuff() { return 0; }
RK_REGISTER_TEST("testname now", ttff) { rk_assert_eq(do_stuff(), 1); }

RK_REGISTER_TEST("test timeout noisolation", tim, .isolation = RK_ISOLATION_OFF,
                 .timeout_ms = 1) {
  for (int i = 0; i < 10000; ++i) { rk_expect_eq(do_stuff(), i); }
}
RK_REGISTER_TEST("test outeq", oe) {
  rk_expect_stderreq("42", 2);
  printf("e");
  rk_expect_stdouteq("dD", 2);
  rk_expect_stderreq("d", 1);
  rk_expect_stdoutneq("d", 1);
  rk_expect_stderrneq("d", 1);
  fprintf(stderr, "e");
  rk_expect_stdouteq("d", 1);
  rk_expect_stderreq("d", 1);
  rk_expect_stdoutneq("d", 1);
  rk_expect_stderrneq("d", 1);
}

RK_REGISTER_TEST("stringtest", test_str_lentest) {
  //rk_expect_eq(str_len("hello"), lenof("f"));
}

struct MyType {
    int id;
    std::string name;
    bool operator==(const MyType& other) const {
        return id == other.id && name == other.name;
    }
};
std::ostream& operator<<(std::ostream& os, const MyType& obj) {
    static const std::string long_string(3500, 'x'); // 3500 'x' characters
    return os << "MyType{id=" << long_string << ", name=\"" << obj.name << "\"}";
}
RK_REGISTER_TEST("cpp_only",test_cppstuff){
  MyType a={0},b={1};
  rk_expect_eq(a,b);
  MyType c={0},d={1};
  rk_expect_eq(c,d);
  MyType e={0},f={0};
  rk_expect_eq(e,f);
}

RK_REGISTER_TEST("cpp_only",test_cppstuff2){
  MyType a={0},b={1};
  rk_expect_eq(a,b);
}

RK_REGISTER_TEST("cpp_only2",test_cppstuff22){
  const char* a = "hello", *b = "world";
  rk_assert_eq(a,b);
}


RK_REGISTER_TEST("cpp_only3",test_cppstuff23){
  char a[] = "hello", b[] = "world";
  rk_assert_eq(a,b);
}

int main(int argc, char** argv) {
  (void)argc, (void)(argv);
  const char* suites3[]                       = {"stringtest"};
  //RKT_glob.custom_paths[RK_OUTPUTFORMAT_JSON] = "RKT_jsonpathc.json";
  //RKT_glob.custom_paths[RK_OUTPUTFORMAT_TAP]  = "RKT_tappathc.tap";
  //RKT_glob.custom_paths[RK_OUTPUTFORMAT_NORMAL] = "RKT_regcpp.txt";
  long start = RKT_now_ms();
  for (int i = 0; i < 1; ++i) { RK_RUN_TESTS(NULL, 0); }
  printf("Total time: %lld ms\n", RKT_now_ms() - start);
}
