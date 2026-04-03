
#ifndef RK__TESTDUMMY
# include "../rk_test/rk_test.h"
#else
# include "../rk_test/rk_test_dummy.h"
#endif
RK__IGNWARN_CLANG_BEG("-Wunused-variable")
/*
**
*** TEST_EXITS
**
*/
RK_REGISTER_TEST("test_exits", test_exit_success) { //
  rk_assert_exit(5, exit(5));
}
RKT_VALIDATE(test_exit_success, RKT_PASSED)

RK_REGISTER_TEST("test_exits", test_crash_success) {
  rk_assert_crash(RKT_FAULT_MEMORY, raise(SIGSEGV));
}
RKT_VALIDATE(test_crash_success, RKT_PASSED)

RK_REGISTER_TEST("test_exits", test_exit_unexpected) { //
  rk_assert_true((exit(1), 1));
}
RKT_VALIDATE(test_exit_unexpected, RKT_EXITED)

RK_REGISTER_TEST("test_exits", test_crash_unexpected) { //
  rk_assert_true(raise(SIGSEGV));
}
RKT_VALIDATE(test_crash_unexpected, RKT_CRASHED)

RK_REGISTER_TEST("test_exits", test_exit_didnotexit) {
  rk_assert_exit(0, (void)0);
}
RKT_VALIDATE(test_exit_didnotexit, RKT_FAILED)

RK_REGISTER_TEST("test_exits", test_crash_didnotcrash) {
  rk_assert_crash(RKT_FAULT_MEMORY, (void)0);
}
RKT_VALIDATE(test_crash_didnotcrash, RKT_FAILED)

RK_REGISTER_TEST("test_exits", test_exit_wrongcode) {
  rk_assert_exit(5, exit(0));
}
RKT_VALIDATE(test_exit_wrongcode, RKT_FAILED)

RK_REGISTER_TEST("test_exits", test_crash_wrongcode) {
  rk_assert_crash(RKT_FAULT_ABORT, raise(SIGSEGV));
}
RKT_VALIDATE(test_crash_wrongcode, RKT_FAILED)

RK_REGISTER_TEST("test_exits", test_exit_crashedinstead) {
  rk_assert_exit(5, raise(SIGSEGV));
}
RKT_VALIDATE(test_exit_crashedinstead, RKT_CRASHED)

RK_REGISTER_TEST("test_exits", test_crash_exitedinstead) {
  rk_assert_crash(RKT_FAULT_ABORT, exit(SIGABRT));
}
RKT_VALIDATE(test_crash_exitedinstead, RKT_EXITED)

/*
**
*** TEST_ERRORS
**
*/
RK_REGISTER_TEST("test_errors", test_user_crash) { raise(SIGABRT); }
RKT_VALIDATE(test_user_crash, RKT_TESTERROR)
RK_REGISTER_TEST("test_errors", test_user_exit) { exit(1); }
RKT_VALIDATE(test_user_exit, RKT_TESTERROR)

/*
**
*** TEST_TIMEOUT
**
*/
RK_REGISTER_TEST("test_timeout", test_shant_timeout, .timeout_ms = 100) {}
RKT_VALIDATE(test_shant_timeout, RKT_PASSED)

RK_REGISTER_TEST("test_timeout", test_should_timeout, .timeout_ms = 1) {
  sleep(10);
}
RKT_VALIDATE(test_should_timeout, RKT_TIMEDOUT)

/*
**
*** TEST_FIXTURES
**
*/

int*               test_level_mem;
int                test_level_len;
static inline void fixture_test_level_init(void) {
  test_level_len = 1000;
  test_level_mem = (int*)malloc(sizeof(int) * test_level_len);
  for (int i = 0; i < test_level_len; ++i) { test_level_mem[i] = i; }
}
static inline void fixture_test_level_fini(void) {
  test_level_len = 0, free(test_level_mem), test_level_mem = 0;
}

RK_REGISTER_TEST("test_fixtures_test2", test_test_fixture_before_init) {
  rk_assert_eq(test_level_len, 0);
}
RK_REGISTER_TEST("test_fixtures_test", test_test_fixture,
                 .init = fixture_test_level_init) {
  rk_assert_eq(test_level_len, 1000);
  for (int i = 0; i < test_level_len; ++i) {
    rk_assert_eq(test_level_mem[i], i);
  }
}
RK_REGISTER_TEST("test_fixtures_test2", test_test_fixture_between_init_fini) {
  rk_assert_eq(test_level_len, 0);
}
RKT_VALIDATE(test_test_fixture_between_init_fini,
             RKT_PASSED) // todo should run this ?

RK_REGISTER_TEST("test_fixtures_test", test_test_fixture_independence1,
                 .init = fixture_test_level_init) {
  rk_assert_eq(test_level_len, 1000);
  for (int i = 0; i < test_level_len; ++i) {
    rk_assert_eq(test_level_mem[i], i);
    test_level_mem[i] = 3;
  }
}
RK_REGISTER_TEST("test_fixtures_test", test_test_fixture_independence2,
                 .init = fixture_test_level_init) {
  rk_assert_eq(test_level_len, 1000);
  for (int i = 0; i < test_level_len; ++i) {
    rk_assert_eq(test_level_mem[i], i);
  }
}
RK_REGISTER_TEST("test_fixtures_test2", test_test_fixture_after_fini) {
  rk_assert_eq(test_level_len, 0);
}

int*               suite_level_mem;
int                suite_level_len;
static inline void fixture_suite_level_init(void) {
  suite_level_len = 1000;
  suite_level_mem = (int*)malloc(sizeof(int) * suite_level_len);
  for (int i = 0; i < suite_level_len; ++i) { suite_level_mem[i] = i; }
}
static inline void fixture_suite_level_fini(void) {
  suite_level_len = 0, free(suite_level_mem), suite_level_mem = 0;
}

RK_REGISTER_TEST("test_fixtures_suite_before", test_suite_fixture_before_init) {
  rk_assert_eq(suite_level_len, 0);
}

RK_REGISTER_SUITE("test_fixtures_suite", .init = fixture_suite_level_init,
                  .fini = fixture_suite_level_fini)

RK_REGISTER_TEST("test_fixtures_suite", test_suite_fixture_1) {
  if (RKT_entry_test_suite_fixture_1.suite->attrs.init
      == fixture_suite_level_init) {
    rk_assert_eq(suite_level_len, 1000);
    for (int i = 0; i < suite_level_len; ++i) {
      rk_assert_eq(suite_level_mem[i], i);
    }
  }
}

RK_REGISTER_TEST("test_fixtures_suite", test_suite_fixture_independence1) {
  if (RKT_entry_test_suite_fixture_1.suite->attrs.init
      == fixture_suite_level_init) {
    rk_assert_eq(suite_level_len, 1000);
    for (int i = 0; i < suite_level_len; ++i) {
      rk_assert_eq(suite_level_mem[i], i);
      suite_level_mem[i] = 3; // mutate for next test
    }
  }
}

RK_REGISTER_TEST("test_fixtures_suite", test_suite_fixture_independence2) {
  if (RKT_entry_test_suite_fixture_1.suite->attrs.init
      == fixture_suite_level_init) {
    rk_assert_eq(suite_level_len, 1000);
    for (int i = 0; i < suite_level_len; ++i) {
      rk_assert_eq(suite_level_mem[i], i);
    }
  }
}

RK_REGISTER_TEST("test_fixtures_suite_after", test_suite_fixture_after_fini) {
  rk_assert_eq(suite_level_len, 0);
}

int*               global_level_mem;
int                global_level_len;
static inline void fixture_global_level_init(void) {
  global_level_len = 1000;
  global_level_mem = (int*)malloc(sizeof(int) * global_level_len);
  for (int i = 0; i < global_level_len; ++i) { global_level_mem[i] = i; }
}
static inline void fixture_global_level_fini(void) {
  global_level_len = 0, free(global_level_mem), global_level_mem = 0;
}
RK_REGISTER_TEST("test_fixtures_glob", test_global_fixture) {
  if (RKT_glob.attrs.init == fixture_global_level_init) {
    rk_assert_eq(global_level_len, 1000);
    for (int i = 0; i < global_level_len; ++i) {
      rk_assert_eq(global_level_mem[i], i);
    }
  }
}
RK_REGISTER_TEST("test_fixtures_glob", test_global_fixture_independence) {
  if (RKT_glob.attrs.init == fixture_global_level_init) {
    rk_assert_eq(global_level_len, 1000);
    for (int i = 0; i < global_level_len; ++i) {
      global_level_mem[i] = 3;
      rk_assert_eq(global_level_mem[i], 3);
    }
  }
}

RK_REGISTER_TEST("test_fixtures_glob", test_global_fixture_independence2) {
  if (RKT_glob.attrs.init == fixture_global_level_init) {
    rk_assert_eq(global_level_len, 1000);
    for (int i = 0; i < global_level_len; ++i) {
      rk_assert_eq(global_level_mem[i], i);
    }
  }
}

/*
**
*** TEST_PRINT
**
*/
RK_REGISTER_TEST("test_print", test_print_to_out_p) {
  printf("SHANT PRINT BY DEFAULT\n"); // nolint
  rk_expect_true(1);
}
RK_REGISTER_TEST("test_print", test_print_to_out_f) {
  printf("SHALL PRINT BY DEFAULT\n");
  rk_expect_true(0);
}
RK_REGISTER_TEST("test_print", test_print_to_out_c) {
  printf("SHALL PRINT BY DEFAULT\n"); // nolint
  rk_expect_true((exit(0), 1));
}
RK_REGISTER_TEST("test_print", test_print_to_out_t, .timeout_ms = 1) {
  printf("SHALL PRINT BY DEFAULT\n"); // nolint
  sleep(10);
}
RK_REGISTER_TEST("test_print", test_print_to_out_e) {
  printf("SHALL PRINT BY DEFAULT\n"); // nolint
  raise(SIGABRT);
}
RK_REGISTER_TEST("test_print", test_print_to_err_p) {
  fprintf(stderr, "SHALL PRINT BY DEFAULT\n"); // nolint
}

RK_REGISTER_TEST("test_print", test_print_to_err_f) {
  fprintf(stderr, "SHALL PRINT BY DEFAULT\n"); // nolint
  rk_expect_true(0);
}
RK_REGISTER_TEST("test_print", test_print_to_err_c) {
  fprintf(stderr, "SHALL PRINT BY DEFAULT\n"); // nolint
  rk_expect_true((exit(0), 1));
}
RK_REGISTER_TEST("test_print", test_print_to_err_t, .timeout_ms = 1) {
  fprintf(stderr, "SHALL PRINT BY DEFAULT\n"); // nolint
  sleep(10);
}
RK_REGISTER_TEST("test_print", test_print_to_err_e) {
  fprintf(stderr, "SHALL PRINT BY DEFAULT\n"); // nolint
  raise(SIGABRT);
}
/*
TEST PRINT NEVER
*/
RK_REGISTER_TEST("test_print_never", test_print_to_out_p_never,
                 .verbosity_levels[RK_OUTPUTSTREAMS_STDOUT]
                 = RK_VERBOSITY_NEVER) {
  printf("SHANT PRINT\n"); // nolint
  rk_assert_true(1);
}
RK_REGISTER_TEST("test_print_never", test_print_to_out_f_never,
                 .verbosity_levels[RK_OUTPUTSTREAMS_STDOUT]
                 = RK_VERBOSITY_NEVER) {
  printf("SHANT PRINT\n"); // nolint
  rk_assert_true(0);
}

RK_REGISTER_TEST("test_print_never", test_print_to_out_c_never,
                 .verbosity_levels[RK_OUTPUTSTREAMS_STDOUT]
                 = RK_VERBOSITY_NEVER) {
  printf("SHANT PRINT\n"); // nolint
  rk_expect_true((exit(0), 1));
}
RK_REGISTER_TEST("test_print_never", test_print_to_out_t_never,
                 .verbosity_levels[RK_OUTPUTSTREAMS_STDOUT]
                 = RK_VERBOSITY_NEVER,
                 .timeout_ms = 1) {
  printf("SHANT PRINT\n"); // nolint
  sleep(10);
}
RK_REGISTER_TEST("test_print_never", test_print_to_out_e_never,
                 .verbosity_levels[RK_OUTPUTSTREAMS_STDOUT]
                 = RK_VERBOSITY_NEVER) {
  printf("SHANT PRINT\n"); // nolint
  raise(SIGABRT);
}

RK_REGISTER_TEST("test_print_never", test_print_to_err_p_never,
                 .verbosity_levels[RK_OUTPUTSTREAMS_STDERR]
                 = RK_VERBOSITY_NEVER) {
  fprintf(stderr, "SHANT PRINT\n"); // nolint
  rk_assert_true(1);
}
RK_REGISTER_TEST("test_print_never", test_print_to_err_f_never,
                 .verbosity_levels[RK_OUTPUTSTREAMS_STDERR]
                 = RK_VERBOSITY_NEVER) {
  fprintf(stderr, "SHANT PRINT\n"); // nolint
  rk_assert_true(0);
}
RK_REGISTER_TEST("test_print_never", test_print_to_err_c_never,
                 .verbosity_levels[RK_OUTPUTSTREAMS_STDERR]
                 = RK_VERBOSITY_NEVER) {
  fprintf(stderr, "SHANT PRINT\n"); // nolint
  rk_expect_true((exit(0), 1));
}
RK_REGISTER_TEST("test_print_never", test_print_to_err_t_never,
                 .verbosity_levels[RK_OUTPUTSTREAMS_STDERR]
                 = RK_VERBOSITY_NEVER,
                 .timeout_ms = 1) {
  fprintf(stderr, "SHANT PRINT\n"); // nolint
  sleep(10);
}
RK_REGISTER_TEST("test_print_never", test_print_to_err_e_never,
                 .verbosity_levels[RK_OUTPUTSTREAMS_STDERR]
                 = RK_VERBOSITY_NEVER) {
  fprintf(stderr, "SHANT PRINT\n"); // nolint
  raise(SIGABRT);
}

/*
TEST PRINT ALWAYS
*/
RK_REGISTER_TEST("test_print_always", test_print_to_out_p_always,
                 .verbosity_levels[RK_OUTPUTSTREAMS_STDOUT]
                 = RK_VERBOSITY_ALWAYS) {
  printf("SHALL PRINT\n"); // nolint
  rk_assert_true(1);
}
RK_REGISTER_TEST("test_print_always", test_print_to_out_f_always,
                 .verbosity_levels[RK_OUTPUTSTREAMS_STDOUT]
                 = RK_VERBOSITY_ALWAYS) {
  printf("SHALL PRINT\n"); // nolint
  rk_assert_true(0);
}

RK_REGISTER_TEST("test_print_always", test_print_to_out_c_always,
                 .verbosity_levels[RK_OUTPUTSTREAMS_STDOUT]
                 = RK_VERBOSITY_ALWAYS) {
  printf("SHALL PRINT\n"); // nolint
  rk_expect_true((exit(0), 1));
}
RK_REGISTER_TEST("test_print_always", test_print_to_out_t_always,
                 .verbosity_levels[RK_OUTPUTSTREAMS_STDOUT]
                 = RK_VERBOSITY_ALWAYS,
                 .timeout_ms = 1) {
  printf("SHALL PRINT\n"); // nolint
  sleep(10);
}
RK_REGISTER_TEST("test_print_always", test_print_to_out_e_always,
                 .verbosity_levels[RK_OUTPUTSTREAMS_STDOUT]
                 = RK_VERBOSITY_ALWAYS) {
  printf("SHALL PRINT\n"); // nolint
  raise(SIGABRT);
}

RK_REGISTER_TEST("test_print_always", test_print_to_err_p_always,
                 .verbosity_levels[RK_OUTPUTSTREAMS_STDERR]
                 = RK_VERBOSITY_ALWAYS) {
  fprintf(stderr, "SHALL PRINT\n"); // nolint
  rk_assert_true(1);
}
RK_REGISTER_TEST("test_print_always", test_print_to_err_f_always,
                 .verbosity_levels[RK_OUTPUTSTREAMS_STDERR]
                 = RK_VERBOSITY_ALWAYS) {
  fprintf(stderr, "SHALL PRINT\n"); // nolint
  rk_assert_true(0);
}
RK_REGISTER_TEST("test_print_always", test_print_to_err_c_always,
                 .verbosity_levels[RK_OUTPUTSTREAMS_STDERR]
                 = RK_VERBOSITY_ALWAYS) {
  fprintf(stderr, "SHALL PRINT\n"); // nolint
  rk_expect_true((exit(0), 1));
}
RK_REGISTER_TEST("test_print_always", test_print_to_err_t_always,
                 .verbosity_levels[RK_OUTPUTSTREAMS_STDERR]
                 = RK_VERBOSITY_ALWAYS,
                 .timeout_ms = 1) {
  fprintf(stderr, "SHALL PRINT\n"); // nolint
  sleep(10);
}
RK_REGISTER_TEST("test_print_always", test_print_to_err_e_always,
                 .verbosity_levels[RK_OUTPUTSTREAMS_STDERR]
                 = RK_VERBOSITY_ALWAYS) {
  fprintf(stderr, "SHALL PRINT\n"); // nolint
  raise(SIGABRT);
}

/*
always suite-level
*/
RK_REGISTER_SUITE("test_print_always_suitelevel",
                  .verbosity_levels[RK_OUTPUTSTREAMS_STDOUT]
                  = RK_VERBOSITY_ALWAYS,
                  .verbosity_levels[RK_OUTPUTSTREAMS_STDERR]
                  = RK_VERBOSITY_ALWAYS)
RK_REGISTER_TEST("test_print_always_suitelevel",
                 test_print_to_out_p_always_suitelevel) {
  printf("SHALL PRINT\n"); // nolint
  rk_assert_true(1);
}
RK_REGISTER_TEST("test_print_always_suitelevel",
                 test_print_to_out_f_always_suitelevel) {
  printf("SHALL PRINT\n"); // nolint
  rk_assert_true(0);
}
RK_REGISTER_TEST("test_print_always_suitelevel",
                 test_print_to_out_c_always_suitelevel) {
  printf("SHALL PRINT\n"); // nolint
  rk_expect_true((exit(0), 1));
}
RK_REGISTER_TEST("test_print_always_suitelevel",
                 test_print_to_out_t_always_suitelevel, .timeout_ms = 1) {
  printf("SHALL PRINT\n"); // nolint
  sleep(10);
}
RK_REGISTER_TEST("test_print_always_suitelevel",
                 test_print_to_out_e_always_suitelevel) {
  printf("SHALL PRINT\n"); // nolint
  raise(SIGABRT);
}
// err
RK_REGISTER_TEST("test_print_always_suitelevel",
                 test_print_to_err_p_always_suitelevel) {
  fprintf(stderr, "SHALL PRINT\n"); // nolint
  rk_assert_true(1);
}
RK_REGISTER_TEST("test_print_always_suitelevel",
                 test_print_to_err_f_always_suitelevel) {
  fprintf(stderr, "SHALL PRINT\n"); // nolint
  rk_assert_true(0);
}
RK_REGISTER_TEST("test_print_always_suitelevel",
                 test_print_to_err_c_always_suitelevel) {
  fprintf(stderr, "SHALL PRINT\n"); // nolint
  rk_expect_true((exit(0), 1));
}
RK_REGISTER_TEST("test_print_always_suitelevel",
                 test_print_to_err_t_always_suitelevel, .timeout_ms = 1) {
  fprintf(stderr, "SHALL PRINT\n"); // nolint
  sleep(10);
}
RK_REGISTER_TEST("test_print_always_suitelevel",
                 test_print_to_err_e_always_suitelevel) {
  fprintf(stderr, "SHALL PRINT\n"); // nolint
  raise(SIGABRT);
}

/*
never suite-level
*/
RK_REGISTER_SUITE("test_print_never_suitelevel",
                  .verbosity_levels[RK_OUTPUTSTREAMS_STDOUT]
                  = RK_VERBOSITY_NEVER,
                  .verbosity_levels[RK_OUTPUTSTREAMS_STDERR]
                  = RK_VERBOSITY_NEVER)
RK_REGISTER_TEST("test_print_never_suitelevel",
                 test_print_to_out_p_never_suitelevel) {
  printf("SHANT PRINT\n"); // nolint
  rk_assert_true(1);
}
RK_REGISTER_TEST("test_print_never_suitelevel",
                 test_print_to_out_f_never_suitelevel) {
  printf("SHANT PRINT\n"); // nolint
  rk_assert_true(0);
}
RK_REGISTER_TEST("test_print_never_suitelevel",
                 test_print_to_out_c_never_suitelevel) {
  printf("SHANT PRINT\n"); // nolint
  rk_expect_true((exit(0), 1));
}
RK_REGISTER_TEST("test_print_never_suitelevel",
                 test_print_to_out_t_never_suitelevel, .timeout_ms = 1) {
  printf("SHANT PRINT\n"); // nolint
  sleep(10);
}
RK_REGISTER_TEST("test_print_never_suitelevel",
                 test_print_to_out_e_never_suitelevel) {
  printf("SHANT PRINT\n"); // nolint
  raise(SIGABRT);
}
// err
RK_REGISTER_TEST("test_print_never_suitelevel",
                 test_print_to_err_p_never_suitelevel) {
  fprintf(stderr, "SHANT PRINT\n"); // nolint
  rk_assert_true(1);
}
RK_REGISTER_TEST("test_print_never_suitelevel",
                 test_print_to_err_f_never_suitelevel) {
  fprintf(stderr, "SHANT PRINT\n"); // nolint
  rk_assert_true(0);
}
RK_REGISTER_TEST("test_print_never_suitelevel",
                 test_print_to_err_c_never_suitelevel) {
  fprintf(stderr, "SHANT PRINT\n"); // nolint
  rk_expect_true((exit(0), 1));
}
RK_REGISTER_TEST("test_print_never_suitelevel",
                 test_print_to_err_t_never_suitelevel, .timeout_ms = 1) {
  fprintf(stderr, "SHANT PRINT\n"); // nolint
  sleep(10);
}
RK_REGISTER_TEST("test_print_never_suitelevel",
                 test_print_to_err_e_never_suitelevel) {
  fprintf(stderr, "SHANT PRINT\n"); // nolint
  raise(SIGABRT);
}
/*
**
*** TEST_MISC
**
*/
RK_REGISTER_TEST("test_misc", test_noasserts) {}
RKT_VALIDATE(test_noasserts, RKT_PASSED)

RK_REGISTER_TEST("test_misc", test_manyasserts) {
  for (size_t i = 0; i < 10; ++i) { rk_expect_eq(i, i + 1); }
}

RK_REGISTER_TEST("test_misc", test_removethis,
                 .verbosity_levels[RK_OUTPUTSTREAMS_STDOUT]
                 = RK_VERBOSITY_ALWAYS) {
  rk_expect_floatneq_tol(1.0, .1, .3);
  rk_expect_floateq_tol(1.0, 1.0, 1.0);
  /* todo issue:*/
  rk_expect_floateq_tol(1.0, 2.0, 0.0);

  char arr[10] = {0}, arr2[10] = {0};
  rk_expect_eq(arr, arr2);
  int arr3[10] = {0}, arr4[10] = {0};
  rk_expect_eq(arr3, arr4);
  rk_expect_memeq(arr, arr2, 10);
  const char* a1 = "hello";
  const char* a2 = "world";
  const char* a3 = "hello";
  printf("TESTOMATICO\n");
  // rk_expect_death(1, 0);
  rk_expect_streq(a1, a2);
  rk_expect_streq(a1, a2);
  rk_expect_strneq(a1, a2);
  rk_expect_strneq(a1, a3);
  rk_expect_inrange(0, 1, 10);
  rk_expect_inrange(2, 1, 10);
  rk_expect_inrange(0, 2, 134);
  rk_expect_inrange(699, 2, 134);
  rk_expect_true(0);
  rk_assert_true(0);
  rk_assert_exit(0, exit(0));
}
#if 0

RK_REGISTER_TEST("test", test_removethis,
                 .verbosity_levels[RK_OUTPUTSTREAMS_STDOUT] = RK_VERBOSITY_ALWAYS) {
  /* todo issue:*/
  rk_expect_floatneq_tol(1.0, .1, .3);
  rk_expect_floateq_tol(1.0, 1.0, 1.0);
  char arr[10] = {0}, arr2[10] = {0};
  rk_expect_eq(arr, arr2);
  rk_expect_memeq(arr, arr2, 10);
  const char* a1 = "hello";
  const char* a2 = "world";
  const char* a3 = "hello";
  printf("TESTOMATICO\n");
  // rk_expect_death(1, 0);
  rk_expect_streq(a1, a2);
  rk_expect_streq(a1, a2);
  rk_expect_strneq(a1, a2);
  rk_expect_strneq(a1, a3);
  rk_expect_inrange(0, 1, 10);
  rk_expect_inrange(2, 1, 10);
  rk_expect_inrange(0, 2, 134);
  rk_expect_inrange(699, 2, 134);
  rk_expect_true(0);
  rk_assert_true(0);
  rk_assert_exit(0, exit(0));
}
RKT_VALIDATE(test_removethis, RKT_FAILED)
#endif
