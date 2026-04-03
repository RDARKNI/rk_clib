#define RK_IMPL

#include "../rk_test/rk_test.h"
// clang-format off
#define RKT_FULL_TEST 1 
#if RKT_FULL_TEST
//#include "test_rk_string.c"
//#include "test_rk_arena.c"
//#include "test_rk_bitset.c"
//#include "test_rk_dict.c"
//#include "test_rk_vec.c"
//#include "test_rk_test.c"
//// clang-format on
//
//RK_REGISTER_TEST("maintestfuns", noiso_test_expect,
//                 .isolation = RK_ISOLATION_OFF) {
//  rk_expect_eq(0, 0);
//  rk_expect_eq(1, 2);
//}
//RK_REGISTER_TEST("maintestfuns", noiso_test_assert,
//                 .isolation = RK_ISOLATION_OFF) {
//  rk_assert_eq(0, 0);
//  rk_assert_eq(0, 1);
//  rk_assert_eq(0, 1); // should not run
//}
//
//RK_REGISTER_TEST("maintestfuns", noiso_test_timeout,
//                 .isolation = RK_ISOLATION_OFF, .timeout_ms = 1) {
//  rk_expect_eq((sleep(1), 1), 1);
//}
//
//RK_REGISTER_TEST("maintestfuns", noiso_test_timeout2,
//                 .isolation = RK_ISOLATION_OFF, .timeout_ms = 10) {
//  rk_assert_eq(1, 0);
//  rk_expect_eq((sleep(1), 1), 1);
//}
//
//RK_REGISTER_TEST("maintestfuns", noiso_test_notimeout,
//                 .isolation = RK_ISOLATION_OFF, .timeout_ms = 10) {
//  rk_expect_eq(0, 0);
//}
//RK_REGISTER_TEST("test outeq", oe) {
//  rk_expect_stderreq("42", 2);
//  printf("e");
//  rk_expect_stdouteq("dD", 2);
//  rk_expect_stderreq("d", 1);
//  rk_expect_stdoutneq("d", 1);
//  rk_expect_stderrneq("d", 1);
//  fprintf(stderr, "e");
//  rk_expect_stdouteq("d", 1);
//  rk_expect_stderreq("d", 1);
//  rk_expect_stdoutneq("d", 1);
//  rk_expect_stderrneq("d", 1);
//}
//
//RK_REGISTER_TEST("stringtest", test_str_lentest) {
//  rk_expect_eq(str_len("hello"), lenof("f"));
//}
//
//RK_REGISTER_TEST("maintestfuns", lots) {
//  for (int i = 0; i < 10; ++i) {
//    rk_expect_eq(i, 1);
//    rk_expect_eq(i, i);
//  }
//}
//
//RK_REGISTER_TEST("maintestfuns", err_testfun) { rk_expect_eq(1, (exit(0), 0)); }

// RK_REGISTER_TEST("external tests", etest1);

RK_REGISTER_TEST("streqtests", streqtest1) {
  rk_expect_streq("hello", "oo"); // should fail 
  rk_expect_strneq("hello", "oo"); // should pass 

}
// clang-format on
RK_REGISTER_TEST("streqtests", streqtest_empty_edgecases) {
  rk_expect_streq("", NULL);       // should fail
  rk_expect_strneq("", NULL);      // should pass
  rk_expect_streq("", "");         // should pass
  rk_expect_strneq("", "");        // should fail
  rk_expect_streq("hello", NULL);  // should fail
  rk_expect_strneq("hello", NULL); // should pass
  rk_expect_streq(NULL, NULL);     // should pass
  rk_expect_strneq(NULL, NULL);    // should fail
}

RK_REGISTER_TEST("streqtests", streqtest_nullstring) {
  rk_expect_streq("(null)", NULL);      // should fail
  rk_expect_strneq("(null)", NULL);     // should pass
  rk_expect_streq("(null)", "(null)");  // should pass
  rk_expect_strneq("(null)", "(null)"); // should fail
}
RK_REGISTER_TEST("streqtests", streqtest_prefixes) {
  rk_expect_streq("", "a");      // should fail
  rk_expect_strneq("", "a");     // should pass
  rk_expect_streq("aa", "aaa");  // should fail
  rk_expect_strneq("aa", "aaa"); // should pass
}
int main(int argc, char** argv) {
  (void)argc, (void)(argv);
  const char* suites3[]                       = {"stringtest"};
  RKT_glob.custom_paths[RK_OUTPUTFORMAT_JSON] = "RKT_jsonpathc.json";
  // RKT_glob.custom_paths[RK_OUTPUTFORMAT_TAP]    = "RKT_tappathc.tap";
  // RKT_glob.custom_paths[RK_OUTPUTFORMAT_NORMAL] = "RKT_regc.txt";
  long start                         = RKT_now_ms();
  RKT_glob.attrs.verbosity_levels[0] = RK_VERBOSITY_ALWAYS;
  RKT_glob.attrs.verbosity_levels[1] = RK_VERBOSITY_ALWAYS;
  RKT_glob.attrs.verbosity_levels[2] = RK_VERBOSITY_ALWAYS;

  RK_RUN_TESTS(NULL, 0);
  printf("Total time: %lld ms\n", RKT_now_ms() - start);
  printf("END\n");
}
#else
RK_REGISTER_TEST("tfunc1", tfunc1, .isolation = RK_ISOLATION_OFF) {
  rk_assert_eq(0, 1);
  rk_assert_eq(0, 1);
}

RK_REGISTER_TEST("tfunc2", tfunc2) {
  rk_assert_eq(0, 1);
  rk_assert_eq(2, 5);
}

int main(int argc, char** argv) {
  (void)argc, (void)(argv);
  const char* suites3[]                       = {"stringtest"};
  RKT_glob.custom_paths[RK_OUTPUTFORMAT_JSON] = "RKT_jsonpathc.json";
  RKT_glob.custom_paths[RK_OUTPUTFORMAT_TAP]  = "RKT_tappathc.tap";

  // RKT_glob.custom_paths[RK_OUTPUTFORMAT_NORMAL] = "none";
  long start = RKT_now_ms();
  // RK_RUN_TESTS(NULL, 0, .init = fixture_global_level_init,
  //              .fini = fixture_global_level_fini);

  for (int i = 0; i < 1; ++i) {
    RK_RUN_TESTS(NULL, 0, .verbosity_levels[0] = RK_VERBOSITY_NEVER,
                 .verbosity_levels[1] = RK_VERBOSITY_NEVER,
                 .verbosity_levels[2] = RK_VERBOSITY_NEVER);
  }
  printf("Total time: %lld ms\n", RKT_now_ms() - start);
}

#endif
