
#ifndef TEST_TEST_H
#define TEST_TEST_H
#include "conf.h"

RK__IGNWARN_CLANG_BEG("-Wunused-variable")

#if defined(__has_feature)
# if __has_feature(address_sanitizer)
#  define TRIAXI_IFASAN 1
# endif
#endif
#if defined(__SANITIZE_ADDRESS__)
# define TRIAXI_IFASAN 1
#endif
#ifndef TRIAXI_IFASAN
# define TRIAXI_IFASAN 0
#endif

#define donothing(a, b)
/*
**
*** TEST_EXITS — isolation, signals, exit codes, crashes, timeouts
**
*/
triax_suite(test_exits, .isolation = TRIAX_ISOLATION_ON);

triax_test(test_exits, skip) { triax_skip(); }
triax_test(test_exits, pass) { triax_assert_true(1); }
triax_test(test_exits, fail) { triax_assert_true(0); }
triax_test(test_exits, timeout, .isolation = TRIAX_ISOLATION_ON, .timeout_ms = 1) { sleep(100); }
triax_test(test_exits, uexit) { triax_assert_true((exit(1), 1)); }
triax_test(test_exits, ufault) { triax_assert_true(raise(SIGABRT)); }
triax_test(test_exits, fault_testerr) { raise(SIGABRT); }
triax_test(test_exits, exit_testerr) { exit(0); }
triax_test(test_exits, exit_pass) { triax_assert_exit(5, exit(5)); }
triax_test(test_exits, exit_noexit_fail) { triax_assert_exit(0, (void)0); }
triax_test(test_exits, exit_wrongcode_fail) { triax_assert_exit(5, exit(0)); }
triax_test(test_exits, exit_ucrash) { triax_assert_exit(5, raise(SIGABRT)); }
triax_test(test_exits, crash_pass) { triax_assert_fault(TRIAX_FAULT_ABORT, raise(SIGABRT)); }
triax_test(test_exits, crash_any_pass) { triax_assert_fault(TRIAX_FAULT_ANY, raise(SIGABRT)); }
triax_test(test_exits, crash_nocrash_fail) { triax_assert_fault(TRIAX_FAULT_ABORT, (void)0); }
triax_test(test_exits, crash_wrongcode_fail) {
  triax_assert_fault(TRIAX_FAULT_MEMORY, raise(SIGABRT));
}
triax_test(test_exits, crash_uexit) { triax_assert_fault(TRIAX_FAULT_ABORT, exit(SIGABRT)); }
triax_test(test_exits, timeout_pass, .isolation = TRIAX_ISOLATION_ON, .timeout_ms = 10) {}
triax_test(test_exits, timeout_fail, .isolation = TRIAX_ISOLATION_ON, .timeout_ms = 100) {
  triax_assert_true(0);
}
triax_test(test_exits, timeout_timeout, .isolation = TRIAX_ISOLATION_ON, .timeout_ms = 1) {
  sleep(10);
}

/*
**
*** TEST_ASSERTIONS — eq/neq, true/false/null, assert-vs-expect behaviour
**
*/
triax_suite(test_assertions, .verbosity = TRIAX_VERBOSITY_ALWAYS);

triax_test(test_assertions, eq_neq_pass) {
  triax_expect_eq((char)'a', (char)'a');
  triax_expect_neq((char)'a', (char)'b');
  triax_expect_eq((void*)NULL, (void*)NULL);
  triax_expect_neq((void*)NULL, (void*)1);

  triax_expect_eq((signed char)1, (signed char)1);
  triax_expect_eq((short)1, (short)1);
  triax_expect_eq((int)1, (int)1);
  triax_expect_eq((long)1, (long)1);
  triax_expect_eq((long long)1, (long long)1);
  triax_expect_neq((signed char)1, (signed char)0);
  triax_expect_neq((short)1, (short)0);
  triax_expect_neq((int)1, (int)0);
  triax_expect_neq((long)1, (long)0);
  triax_expect_neq((long long)1, (long long)0);

  triax_expect_eq((unsigned char)1, (unsigned char)1);
  triax_expect_eq((unsigned short)1, (unsigned short)1);
  triax_expect_eq((unsigned int)1, (unsigned int)1);
  triax_expect_eq((unsigned long)1, (unsigned long)1);
  triax_expect_eq((unsigned long long)1, (unsigned long long)1);
  triax_expect_neq((unsigned char)1, (unsigned char)0);
  triax_expect_neq((unsigned short)1, (unsigned short)0);
  triax_expect_neq((unsigned int)1, (unsigned int)0);
  triax_expect_neq((unsigned long)1, (unsigned long)0);
  triax_expect_neq((unsigned long long)1, (unsigned long long)0);

  // floats — all dispatch to long double via _Generic
  triax_expect_eq(0.1f + 0.2f, 0.3f); // float, within epsilon
  triax_expect_eq(0.1 + 0.2, 0.3);    // double, within epsilon
  triax_expect_eq(0.1L + 0.2L, 0.3L); // long double
  triax_expect_neq(1.0f, 2.0f);
  triax_expect_neq(1.0, 2.0);
  triax_expect_neq(1.0L, 2.0L);

  // mixed: second arg widens to long double; float 0.1 != double 0.1 (differ by
  // ~1.5e-9)
  triax_expect_eq(1.0f, (double)1.0f); // same real value, passes
  triax_expect_neq(0.1f, 0.1);         // genuinely different representations

  const char *hello1 = "hello", *hello2 = "hello";
  char *      world1 = "world", *world2 = "world";
  triax_expect_eq(hello1, hello2);
  triax_expect_eq(hello2, hello1);
  triax_expect_eq(world1, world2);
  triax_expect_eq(world2, world1);
  triax_expect_neq(hello1, world1);
  triax_expect_neq(world1, hello1);
  triax_expect_neq(world1, hello1);
  triax_expect_neq(hello1, world1);
}

triax_test(test_assertions, eq_neq_fail) {
  triax_expect_neq((char)'a', (char)'a');
  triax_expect_eq((char)'a', (char)'b');
  triax_expect_neq((void*)NULL, (void*)NULL);
  triax_expect_eq((void*)NULL, (void*)1);

  triax_expect_neq((signed char)1, (signed char)1);
  triax_expect_neq((short)1, (short)1);
  triax_expect_neq((int)1, (int)1);
  triax_expect_neq((long)1, (long)1);
  triax_expect_neq((long long)1, (long long)1);
  triax_expect_eq((signed char)1, (signed char)0);
  triax_expect_eq((short)1, (short)0);
  triax_expect_eq((int)1, (int)0);
  triax_expect_eq((long)1, (long)0);
  triax_expect_eq((long long)1, (long long)0);

  triax_expect_neq((unsigned char)1, (unsigned char)1);
  triax_expect_neq((unsigned short)1, (unsigned short)1);
  triax_expect_neq((unsigned int)1, (unsigned int)1);
  triax_expect_neq((unsigned long)1, (unsigned long)1);
  triax_expect_neq((unsigned long long)1, (unsigned long long)1);
  triax_expect_eq((unsigned char)1, (unsigned char)0);
  triax_expect_eq((unsigned short)1, (unsigned short)0);
  triax_expect_eq((unsigned int)1, (unsigned int)0);
  triax_expect_eq((unsigned long)1, (unsigned long)0);
  triax_expect_eq((unsigned long long)1, (unsigned long long)0);

  // floats — all dispatch to long double via _Generic
  triax_expect_neq(0.1f + 0.2f, 0.3f);
  triax_expect_neq(0.1 + 0.2, 0.3);
  triax_expect_neq(0.1L + 0.2L, 0.3L);
  triax_expect_eq(1.0f, 2.0f);
  triax_expect_eq(1.0, 2.0);
  triax_expect_eq(1.0L, 2.0L);

  // mixed: second arg widens to long double; float 0.1 != double 0.1 (differ by
  // ~1.5e-9)
  triax_expect_neq(1.0f, (double)1.0f);
  triax_expect_eq(0.1f, 0.1);
}

// expect continues past failures; assert stops at first
triax_test(test_assertions, expect_continues) {
  triax_expect_eq(1, 2); // fail 1
  triax_expect_eq(3, 4); // fail 2 — still reached
  triax_expect_eq(5, 6); // fail 3 — still reached
}
triax_test(test_assertions, assert_stops_early, .timeout_ms = 10, .isolation = TRIAX_ISOLATION_ON) {
  triax_assert_eq(1, 2); // stops here; only one failure recorded
  triax_expect_eq(3, 4); // never reached
}

// true / false / null / nonnull
triax_test(test_assertions, true_false_null_pass) {
  triax_expect_true(1);
  triax_expect_true(42);
  triax_expect_true(-1);
  triax_expect_false(0);
  triax_expect_null(NULL);
  triax_expect_nonnull((void*)1);
  triax_expect_nonnull("hello");
}
triax_test(test_assertions, true_false_null_fail) {
  triax_expect_true(0);
  triax_expect_false(1);
  triax_expect_false(-1);
  triax_expect_null((void*)1);
  triax_expect_nonnull(NULL);
}

/*
**
*** TEST_ORDERING — lt, leq, gt, geq, inrange for ints, floats, strings
**
*/
triax_suite(test_ordering, .verbosity = TRIAX_VERBOSITY_ALWAYS);

triax_test(test_ordering, int_pass) {
  triax_expect_lt(1, 2);
  triax_expect_leq(1, 2);
  triax_expect_leq(2, 2); // boundary: equal counts as leq
  triax_expect_gt(2, 1);
  triax_expect_geq(2, 1);
  triax_expect_geq(2, 2); // boundary: equal counts as geq
}
triax_test(test_ordering, int_fail) {
  triax_expect_lt(2, 1);
  triax_expect_lt(2, 2); // equal, not strictly less
  triax_expect_leq(3, 2);
  triax_expect_gt(1, 2);
  triax_expect_gt(2, 2); // equal, not strictly greater
  triax_expect_geq(1, 2);
}
triax_test(test_ordering, unsigned_pass) {
  triax_expect_lt((unsigned)0, (unsigned)1);
  triax_expect_leq((unsigned)0, (unsigned)0);
  triax_expect_gt((unsigned)1, (unsigned)0);
  triax_expect_geq((unsigned)1, (unsigned)1);
}
triax_test(test_ordering, float_pass) {
  triax_expect_lt(1.0, 2.0);
  triax_expect_leq(2.0, 2.0);
  triax_expect_gt(2.0, 1.0);
  triax_expect_geq(2.0, 2.0);
}
triax_test(test_ordering, float_fail) {
  triax_expect_lt(2.0, 1.0);
  triax_expect_gt(1.0, 2.0);
}
triax_test(test_ordering, str_pass) {
  triax_expect_lt("apple", "banana");
  triax_expect_lt("", "a");
  triax_expect_lt(NULL, "");    // NULL < empty string
  triax_expect_lt("ab", "abc"); // prefix: shorter < longer
  triax_expect_leq("apple", "apple");
  triax_expect_leq("apple", "banana");
  triax_expect_gt("banana", "apple");
  triax_expect_geq("banana", "banana");
}
triax_test(test_ordering, str_fail) {
  triax_expect_lt("banana", "apple");
  triax_expect_lt("apple", "apple"); // equal, not strictly less
  triax_expect_gt("apple", "banana");
}

/*
**
*** TEST_FLOAT_TOLERANCE — floateq_abstol / floatneq_abstol
**
*/
triax_suite(test_float_tol, .verbosity = TRIAX_VERBOSITY_ALWAYS);

triax_test(test_float_tol, floateq_pass) {
  triax_expect_floateq_abstol(1.0, 1.0 + 9e-5, 1e-4); // within tolerance
  triax_expect_floateq_abstol(1.0, 1.0 + 1e-4, 1e-4); // exactly at boundary (<=)
  triax_expect_floateq_abstol(0.0, 0.0, 0.0);         // exact zero, zero tolerance
}
triax_test(test_float_tol, floateq_fail) {
  triax_expect_floateq_abstol(1.0, 2.0, 0.5);           // well outside
  triax_expect_floateq_abstol(1.0, 1.0 + 1.1e-4, 1e-4); // just over boundary
}
triax_test(test_float_tol, floatneq_pass) {
  triax_expect_floatneq_abstol(1.0, 2.0, 0.5);
  triax_expect_floatneq_abstol(1.0, 1.0 + 1.1e-4, 1e-4);
}
triax_test(test_float_tol, floatneq_fail) {
  triax_expect_floatneq_abstol(1.0, 1.0 + 9e-5, 1e-4); // within tolerance
  triax_expect_floatneq_abstol(1.0, 1.0, 1e-4);        // exactly equal
}

/*
**
*** TEST_TIMEOUT — timeout fires at different positions relative to assertions
**
*/
static inline int RKTITEST_loop_forever(void) {
  for (;;) { printf(""); }
}
triax_suite(test_timeout, .isolation = TRIAX_ISOLATION_ON);
triax_test(test_timeout, timeout_in_assert, .timeout_ms = 10) {
  triax_expect_true(RKTITEST_loop_forever());
}
triax_test(test_timeout, timeout_before_assert, .timeout_ms = 10) { RKTITEST_loop_forever(); }

triax_test(test_timeout, timeout_after_assert, .timeout_ms = 10) {
  RKTITEST_loop_forever();
  triax_expect_true(1);
}
triax_test(test_timeout, timeout_before_after_assert, .timeout_ms = 10) {
  triax_expect_true(1);
  RKTITEST_loop_forever();
  triax_expect_true(1);
}

/*
**
*** TEST_FIXTURES — test-level, suite-level, global
**
*/

static int*        test_level_mem;
static size_t      test_level_len;
static inline void fixture_test_level_init(void) {
  test_level_len = 1000;
  test_level_mem = (int*)malloc(sizeof(int) * test_level_len);
  for (size_t i = 0; i < test_level_len; ++i) { test_level_mem[i] = (int)i; }
}
static inline void fixture_test_level_fini(void) {
  test_level_len = 0, free(test_level_mem), test_level_mem = 0;
}

triax_test(test_fixtures_test2, test_test_fixture_before_init) {
  triax_assert_eq(test_level_len, 0);
}
triax_test(test_fixtures_test, test_test_fixture, .init = fixture_test_level_init,
           .fini = fixture_test_level_fini) {
  triax_assert_eq(test_level_len, 1000);
  for (size_t i = 0; i < test_level_len; ++i) { triax_assert_eq(test_level_mem[i], (int)i); }
}
triax_test(test_fixtures_test2, test_test_fixture_between_init_fini) {
  triax_assert_eq(test_level_len, 0);
}
triax_test(test_fixtures_test, test_test_fixture_independence1, .init = fixture_test_level_init,
           .fini = fixture_test_level_fini) {
  triax_assert_eq(test_level_len, 1000);
  for (size_t i = 0; i < test_level_len; ++i) {
    triax_assert_eq((size_t)test_level_mem[i], i);
    test_level_mem[i] = 3;
  }
}
triax_test(test_fixtures_test, test_test_fixture_independence2, .init = fixture_test_level_init,
           .fini = fixture_test_level_fini) {
  triax_assert_eq(test_level_len, 1000);
  for (size_t i = 0; i < test_level_len; ++i) { triax_assert_eq((size_t)test_level_mem[i], i); }
}
triax_test(test_fixtures_test2, test_test_fixture_after_fini) {
  triax_assert_eq(test_level_len, 0);
}

static int*        suite_level_mem;
static size_t      suite_level_len;
static inline void fixture_suite_level_init(void) {
  suite_level_len = 1000;
  suite_level_mem = (int*)malloc(sizeof(int) * suite_level_len);
  for (size_t i = 0; i < suite_level_len; ++i) { suite_level_mem[i] = (int)i; }
}
static inline void fixture_suite_level_fini(void) {
  suite_level_len = 0, free(suite_level_mem), suite_level_mem = 0;
}
static inline void fixture_suite_level_reset(void) {
  for (size_t i = 0; i < suite_level_len; ++i) { suite_level_mem[i] = (int)i; }
}

triax_test(test_fixtures_suite_before, test_suite_fixture_before_init) {
  triax_assert_eq(suite_level_len, 0);
}
triax_suite(test_fixtures_suite, .init = fixture_suite_level_init,
            .fini = fixture_suite_level_fini);
triax_test(test_fixtures_suite, test_suite_fixture_1) {
  triax_assert_eq(suite_level_len, 1000);
  for (size_t i = 0; i < suite_level_len; ++i) { triax_assert_eq((size_t)suite_level_mem[i], (size_t)i); }
}
triax_test(test_fixtures_suite, test_suite_fixture_independence1) {
  triax_assert_eq(suite_level_len, 1000);
  for (size_t i = 0; i < suite_level_len; ++i) {
    triax_assert_eq((size_t)suite_level_mem[i], (size_t)i);
    suite_level_mem[i] = 3; // mutate for next test
  }
}
triax_test(test_fixtures_suite, test_suite_fixture_independence2,
           .init = fixture_suite_level_reset) {
  triax_assert_eq(suite_level_len, 1000);
  for (size_t i = 0; i < suite_level_len; ++i) { triax_assert_eq((size_t)suite_level_mem[i], (size_t)i); }
}
triax_test(test_fixtures_suite_after, test_suite_fixture_after_fini) {
  triax_assert_eq(suite_level_len, 0);
}

static int*        global_level_mem;
static size_t      global_level_len;
static inline void fixture_global_level_init(void) {
  global_level_len = 1000;
  global_level_mem = (int*)malloc(sizeof(int) * global_level_len);
  for (size_t i = 0; i < global_level_len; ++i) { global_level_mem[i] = (int)i; }
}
static inline void fixture_global_level_fini(void) {
  global_level_len = 0, free(global_level_mem), global_level_mem = 0;
}
#ifndef TRIAX_MULTI_TU
// todo see if I can fix this
// triax_test(test_fixtures_glob, test_global_fixture) {
//  if (TRIAXI_run.attrs.init == fixture_global_level_init) {
//    triax_assert_eq(global_level_len, 1000);
//    for (size_t i = 0; i < global_level_len; ++i) {
//      triax_assert_eq(global_level_mem[i], (int)i);
//    }
//  }
//}
//// global fixtures are shared state — they do NOT reset between tests
// triax_test(test_fixtures_glob, test_global_fixture_independence) {
//   if (TRIAXI_run.attrs.init == fixture_global_level_init) {
//     triax_assert_eq(global_level_len, 1000);
//     for (size_t i = 0; i < global_level_len; ++i) {
//       global_level_mem[i] = 3;
//       triax_assert_eq(global_level_mem[i], 3);
//     }
//   }
// }
// triax_test(test_fixtures_glob, test_global_fixture_independence2) {
//   // mutation from test_global_fixture_independence persists — no reset
//   if (TRIAXI_run.attrs.init == fixture_global_level_init) {
//     triax_assert_eq(global_level_len, 1000);
//     for (size_t i = 0; i < global_level_len; ++i) {
//       triax_assert_eq(global_level_mem[i], 3);
//     }
//   }
// }
#endif

/*
**
*** TEST_STRINGS — streq/strneq, strv, startswith/endswith
**
*/
triax_suite(test_strings, .verbosity = TRIAX_VERBOSITY_ALWAYS, .isolation = TRIAX_ISOLATION_ON);

triax_test(test_strings, streq_pass) {
  triax_expect_streq("hello", "hello");
  triax_expect_streq("", "");
  triax_expect_streq((const char*)NULL, (const char*)NULL);
}
triax_test(test_strings, streq_fail) {
  // triax_expect_streq("hello", "world");
  // triax_expect_streq("hello", "");

  triax_expect_streq((const char*)NULL, "");
  // triax_expect_streq("", (const char*)NULL);
}
triax_test(test_strings, strneq_pass) {
  triax_expect_strneq("hello", "world");
  triax_expect_strneq("hello", "");
  triax_expect_strneq((const char*)NULL, "");
  triax_expect_strneq("", (const char*)NULL);
}
triax_test(test_strings, strneq_fail) {
  triax_expect_strneq("hello", "hello");
  triax_expect_strneq("", "");
  triax_expect_strneq((const char*)NULL, (const char*)NULL);
}
triax_test(test_strings, strv_eq_pass) {
  triax_expect_streq(triax_str("hello", 5), triax_str("hello", 5));
  triax_expect_streq(triax_str("hello", 3), triax_str("helloworld", 3)); // same first 3 bytes
  triax_expect_streq(triax_str("", 0), triax_str("", 0));
  triax_expect_streq(triax_str((const char*)NULL, 0), triax_str((const char*)NULL, 0));
}
triax_test(test_strings, strv_eq_fail) {
  triax_expect_streq(triax_str("hello", 5), triax_str("world", 5));
  triax_expect_streq(triax_str("hello", 5),
                     triax_str("hello", 3)); // same content, different length
}
triax_test(test_strings, strv_neq_pass) {
  triax_expect_strneq(triax_str("hello", 5), triax_str("world", 5));
  triax_expect_strneq(triax_str("hello", 5), triax_str("hello", 3));
}
triax_test(test_strings, strv_neq_fail) {
  triax_expect_strneq(triax_str("hello", 5), triax_str("hello", 5));
  triax_expect_strneq(triax_str("", 0), triax_str("", 0));
}
triax_test(test_strings, startswith_pass) {
  triax_expect_str_startswith(triax_str("hello world", 11), triax_str("hello", 5));
  triax_expect_str_startswith(triax_str("hello", 5), triax_str("hello", 5)); // exact match
  triax_expect_str_startswith(triax_str("hello", 5), triax_str("", 0));      // empty prefix
}
triax_test(test_strings, startswith_fail) {
  triax_expect_str_startswith(triax_str("hello", 5), triax_str("world", 5));
  triax_expect_str_startswith(triax_str("hello", 5), triax_str("hello!",
                                                               6)); // prefix longer than str
}
triax_test(test_strings, endswith_pass) {
  triax_expect_str_endswith(triax_str("hello world", 11), triax_str("world", 5));
  triax_expect_str_endswith(triax_str("hello", 5), triax_str("hello", 5));
  triax_expect_str_endswith(triax_str("hello", 5), triax_str("", 0));
}
triax_test(test_strings, endswith_fail) {
  triax_expect_str_endswith(triax_str("hello world", 11), triax_str("hello", 5));
  triax_expect_str_endswith(triax_str("hello", 5), triax_str("world!", 6));
}

/*
**
*** TEST_MEMORY — memeq, memneq, memzero, memnzero
**
*/
triax_suite(test_memory, .verbosity = TRIAX_VERBOSITY_ALWAYS);

triax_test(test_memory, memeq_pass) {
  char a[4] = {1, 2, 3, 4}, b[4] = {1, 2, 3, 4};
  triax_expect_memeq(a, b, 4);
  triax_expect_memeq(a, b, 0); // zero length always equal
}
triax_test(test_memory, memeq_fail) {
  char a[4] = {1, 2, 3, 4}, b[4] = {1, 2, 3, 5};
  triax_expect_memeq(a, b, 4);
}
triax_test(test_memory, memneq_pass) {
  char a[4] = {1, 2, 3, 4}, b[4] = {1, 2, 3, 5};
  triax_expect_memneq(a, b, 4);
}
triax_test(test_memory, memneq_fail) {
  char a[4] = {1, 2, 3, 4}, b[4] = {1, 2, 3, 4};
  triax_expect_memneq(a, b, 4);
}
triax_test(test_memory, memzero_pass) {
  char a[4] = {0};
  triax_expect_memzero(a, 4);
  triax_expect_memzero(a, 0);
}
triax_test(test_memory, memzero_fail) {
  char a[4] = {0, 0, 0, 1};
  triax_expect_memzero(a, 4);
}
triax_test(test_memory, memnzero_pass) {
  char a[4] = {0, 0, 0, 1};
  triax_expect_memnzero(a, 4);
}
triax_test(test_memory, memnzero_fail) {
  char a[4] = {0};
  triax_expect_memnzero(a, 4);
}

/*
**
*** TEST_PRINT — stdout/stderr visibility by verbosity and outcome
**
*/
triax_suite(test_print, .verbosity = TRIAX_VERBOSITY_DEFAULT, .isolation = TRIAX_ISOLATION_ON);
triax_test(test_print, test_pass_stdout_noprint) {
  printf("SHANT PRINT"); // nolint
  triax_expect_true(1);
}
triax_test(test_print, test_fail_stdout_print) {
  printf("SHALL PRINT");
  triax_expect_true(0);
}
triax_test(test_print, test_fault_stdout_print) {
  printf("SHALL PRINT"); // nolint
  triax_expect_true((exit(0), 1));
}
triax_test(test_print, test_timeout_stdout_print, .timeout_ms = 10) {
  printf("SHALL PRINT"); // nolint
  sleep(100000);
}
triax_test(test_print, test_testerr_stdout_print) {
  printf("SHALL PRINT"); // nolint
  raise(SIGABRT);
}
triax_test(test_print, test_pass_stderr_noprint) {
  fprintf(stderr, "SHANT PRINT"); // nolint
  triax_expect_true(1);
}
triax_test(test_print, test_fail_stderr_print) {
  fprintf(stderr, "SHALL PRINT");
  triax_expect_true(0);
}
triax_test(test_print, test_fault_stderr_print) {
  fprintf(stderr, "SHALL PRINT"); // nolint
  triax_expect_true((exit(0), 1));
}
triax_test(test_print, test_timeout_stderr_print, .timeout_ms = 1) {
  fprintf(stderr, "SHALL PRINT"); // nolint
  sleep(10);
}
triax_test(test_print, test_testerr_stderr_print) {
  fprintf(stderr, "SHALL PRINT"); // nolint
  raise(SIGABRT);
}

/*
Test Test-Level Verbosity never
*/
triax_suite(test_print_verbosity_testlevel_never, .verbosity = TRIAX_VERBOSITY_ALWAYS,
            .isolation = TRIAX_ISOLATION_ON);
triax_test(test_print_verbosity_testlevel_never, test_fail_stdout_print_never,
           .verbosity = TRIAX_VERBOSITY_NEVER) {
  printf("SHANT PRINT\n"); // nolint
  triax_assert_true(0);
}
triax_test(test_print_verbosity_testlevel_never, test_fault_stdout_print_never,
           .verbosity = TRIAX_VERBOSITY_NEVER) {
  printf("SHANT PRINT\n"); // nolint
  triax_expect_true((exit(0), 1));
}
triax_test(test_print_verbosity_testlevel_never, test_timeout_stdout_print_never,
           .verbosity = TRIAX_VERBOSITY_NEVER, .timeout_ms = 1) {
  printf("SHANT PRINT\n"); // nolint
  sleep(10);
}
triax_test(test_print_verbosity_testlevel_never, test_print_to_out_e_never,
           .verbosity = TRIAX_VERBOSITY_NEVER) {
  printf("SHANT PRINT\n"); // nolint
  raise(SIGABRT);
}
triax_test(test_print_verbosity_testlevel_never, test_print_to_err_p_never,
           .verbosity = TRIAX_VERBOSITY_NEVER) {
  fprintf(stderr, "SHANT PRINT\n"); // nolint
  triax_assert_true(1);
}
triax_test(test_print_verbosity_testlevel_never, test_print_to_err_f_never,
           .verbosity = TRIAX_VERBOSITY_NEVER) {
  fprintf(stderr, "SHANT PRINT\n"); // nolint
  triax_assert_true(0);
}
triax_test(test_print_verbosity_testlevel_never, test_print_to_err_c_never,
           .verbosity = TRIAX_VERBOSITY_NEVER) {
  fprintf(stderr, "SHANT PRINT\n"); // nolint
  triax_expect_true((exit(0), 1));
}
triax_test(test_print_verbosity_testlevel_never, test_print_to_err_t_never,
           .verbosity = TRIAX_VERBOSITY_NEVER, .timeout_ms = 10) {
  fprintf(stderr, "SHANT PRINT\n"); // nolint
  sleep(10000);
}
triax_test(test_print_verbosity_testlevel_never, test_print_to_err_e_never,
           .verbosity = TRIAX_VERBOSITY_NEVER) {
  fprintf(stderr, "SHANT PRINT\n"); // nolint
  raise(SIGABRT);
}

/*
TEST PRINT ALWAYS
*/
triax_test(test_print_always, test_pass_stdout_print_always, .verbosity = TRIAX_VERBOSITY_ALWAYS) {
  printf("SHALL PRINT\n"); // nolint
  triax_assert_true(1);
}
triax_test(test_print_always, test_fail_stdout_print_always, .verbosity = TRIAX_VERBOSITY_ALWAYS) {
  printf("SHALL PRINT\n"); // nolint
  triax_assert_true(0);
}
triax_test(test_print_always, test_fault_stdout_print_always, .verbosity = TRIAX_VERBOSITY_ALWAYS,
           .isolation = TRIAX_ISOLATION_ON) {
  printf("SHALL PRINT\n"); // nolint
  triax_expect_true((exit(0), 1));
}
triax_test(test_print_always, test_timeout_stdout_print_always, .verbosity = TRIAX_VERBOSITY_ALWAYS,
           .isolation = TRIAX_ISOLATION_ON, .timeout_ms = 10) {
  printf("SHALL PRINT\n"); // nolint
  sleep(1000);
}
triax_test(test_print_always, test_print_to_out_e_always, .verbosity = TRIAX_VERBOSITY_ALWAYS,
           .isolation = TRIAX_ISOLATION_ON) {
  printf("SHALL PRINT\n"); // nolint
  raise(SIGABRT);
}
triax_test(test_print_always, test_print_to_err_p_always, .verbosity = TRIAX_VERBOSITY_ALWAYS) {
  fprintf(stderr, "SHALL PRINT\n"); // nolint
  triax_assert_true(1);
}
triax_test(test_print_always, test_print_to_err_f_always, .verbosity = TRIAX_VERBOSITY_ALWAYS) {
  fprintf(stderr, "SHALL PRINT\n"); // nolint
  triax_assert_true(0);
}
triax_test(test_print_always, test_print_to_err_c_always, .verbosity = TRIAX_VERBOSITY_ALWAYS,
           .isolation = TRIAX_ISOLATION_ON) {
  fprintf(stderr, "SHALL PRINT\n"); // nolint
  triax_expect_true((exit(0), 1));
}
triax_test(test_print_always, test_print_to_err_t_always, .verbosity = TRIAX_VERBOSITY_ALWAYS,
           .isolation = TRIAX_ISOLATION_ON, .timeout_ms = 10) {
  fprintf(stderr, "SHALL PRINT\n"); // nolint
  sleep(10);
}
triax_test(test_print_always, test_print_to_err_e_always, .verbosity = TRIAX_VERBOSITY_ALWAYS,
           .isolation = TRIAX_ISOLATION_ON) {
  fprintf(stderr, "SHALL PRINT\n"); // nolint
  raise(SIGABRT);
}

/*
always suite-level
*/
triax_suite(test_print_always_suitelevel, .verbosity = TRIAX_VERBOSITY_ALWAYS);
triax_test(test_print_always_suitelevel, test_pass_stdout_print_always_suitelevel) {
  printf("SHALL PRINT\n"); // nolint
  triax_assert_true(1);
}
triax_test(test_print_always_suitelevel, test_fail_stdout_print_always_suitelevel) {
  printf("SHALL PRINT\n"); // nolint
  triax_assert_true(0);
}
triax_test(test_print_always_suitelevel, test_fault_stdout_print_always_suitelevel,
           .isolation = TRIAX_ISOLATION_ON) {
  printf("SHALL PRINT\n"); // nolint
  triax_expect_true((exit(0), 1));
}
triax_test(test_print_always_suitelevel, test_timeout_stdout_print_always_suitelevel,
           .isolation = TRIAX_ISOLATION_ON, .timeout_ms = 10) {
  printf("SHALL PRINT\n"); // nolint
  sleep(1000);
}
triax_test(test_print_always_suitelevel, test_print_to_out_e_always_suitelevel,
           .isolation = TRIAX_ISOLATION_ON) {
  printf("SHALL PRINT\n"); // nolint
  raise(SIGABRT);
}
triax_test(test_print_always_suitelevel, test_print_to_err_p_always_suitelevel) {
  fprintf(stderr, "SHALL PRINT\n"); // nolint
  triax_assert_true(1);
}
triax_test(test_print_always_suitelevel, test_print_to_err_f_always_suitelevel) {
  fprintf(stderr, "SHALL PRINT\n"); // nolint
  triax_assert_true(0);
}
triax_test(test_print_always_suitelevel, test_print_to_err_c_always_suitelevel,
           .isolation = TRIAX_ISOLATION_ON) {
  fprintf(stderr, "SHALL PRINT\n"); // nolint
  triax_expect_true((exit(0), 1));
}
triax_test(test_print_always_suitelevel, test_print_to_err_t_always_suitelevel,
           .isolation = TRIAX_ISOLATION_ON, .timeout_ms = 10) {
  fprintf(stderr, "SHALL PRINT\n"); // nolint
  sleep(1000);
}
triax_test(test_print_always_suitelevel, test_print_to_err_e_always_suitelevel,
           .isolation = TRIAX_ISOLATION_ON) {
  fprintf(stderr, "SHALL PRINT\n"); // nolint
  raise(SIGABRT);
}

/*
never suite-level
*/
triax_suite(test_print_never_suitelevel, .verbosity = TRIAX_VERBOSITY_NEVER);
triax_test(test_print_never_suitelevel, test_pass_stdout_noprint2_suitelevel) {
  printf("SHANT PRINT\n"); // nolint
  triax_assert_true(1);
}
triax_test(test_print_never_suitelevel, test_fail_stdout_print_never_suitelevel) {
  printf("SHANT PRINT\n"); // nolint
  triax_assert_true(0);
}
triax_test(test_print_never_suitelevel, test_fault_stdout_print_never_suitelevel,
           .isolation = TRIAX_ISOLATION_ON) {
  printf("SHANT PRINT\n");         // nolint
  triax_expect_true((exit(0), 1)); // todo this is broken somehow
}
triax_test(test_print_never_suitelevel, test_timeout_stdout_print_never_suitelevel,
           .isolation = TRIAX_ISOLATION_ON, .timeout_ms = 10) {
  printf("SHANT PRINT\n"); // nolint
  sleep(1000);
}
triax_test(test_print_never_suitelevel, test_print_to_out_e_never_suitelevel,
           .isolation = TRIAX_ISOLATION_ON) {
  printf("SHANT PRINT\n"); // nolint
  raise(SIGABRT);
}
triax_test(test_print_never_suitelevel, test_print_to_err_p_never_suitelevel) {
  fprintf(stderr, "SHANT PRINT\n"); // nolint
  triax_assert_true(1);
}
triax_test(test_print_never_suitelevel, test_print_to_err_f_never_suitelevel) {
  fprintf(stderr, "SHANT PRINT\n"); // nolint
  triax_assert_true(0);
}
triax_test(test_print_never_suitelevel, test_print_to_err_c_never_suitelevel,
           .isolation = TRIAX_ISOLATION_ON) {
  fprintf(stderr, "SHANT PRINT\n"); // nolint
  triax_expect_true((exit(0), 1));
}
triax_test(test_print_never_suitelevel, test_print_to_err_t_never_suitelevel,
           .isolation = TRIAX_ISOLATION_ON, .timeout_ms = 10) {
  fprintf(stderr, "SHANT PRINT\n"); // nolint
  sleep(10);
}
triax_test(test_print_never_suitelevel, test_print_to_err_e_never_suitelevel,
           .isolation = TRIAX_ISOLATION_ON) {
  fprintf(stderr, "SHANT PRINT\n"); // nolint
  raise(SIGABRT);
}

/*
**
*** TEST_MISC
**
*/

triax_test(test_misc, test_print_only_if_verbosity_not_never) { triax_expect_true(0); }

triax_test(test_misc, test_long_assert) {
  int asdfdsalkfjdsFFFGHFHGFGHFGHFGHFGHFHGFGHFHGFafdsakjfhdsakjfhdsafkjdashfdsakljhdajkfdhasjklfhdsajkfhdsakhfkdsahfas
      = 5;
  triax_expect_eq(
      asdfdsalkfjdsFFFGHFHGFGHFGHFGHFGHFHGFGHFHGFafdsakjfhdsakjfhdsafkjdashfdsakljhdajkfdhasjklfhdsajkfhdsakhfkdsahfas,
      0);
}
triax_test(test_misc, test_noasserts) {}

triax_test(test_misc, test_manyasserts, .skip = 1) {
  for (size_t i = 0; i < 100000; ++i) { triax_expect_eq(i, i + 1); }
}

#if 0

triax_test(test, test_removethis,
                 .verbosity_levels.out = TRIAX_VERBOSITY_ALWAYS) {
  /* todo issue:*/
  triax_expect_floatneq_abstol(1.0, .1, .3);
  triax_expect_floateq_abstol(1.0, 1.0, 1.0);
  char arr[10] = {0}, arr2[10] = {0};
  triax_expect_eq(arr, arr2);
  triax_expect_memeq(arr, arr2, 10);
  const char* a1 = "hello";
  const char* a2 = "world";
  const char* a3 = "hello";
  printf("TESTOMATICO\n");
  // triax_expect_death(1, 0);
  triax_expect_streq(a1, a2);
  triax_expect_streq(a1, a2);
  triax_expect_strneq(a1, a2);
  triax_expect_strneq(a1, a3);
  triax_expect_inrange(0, 1, 10);
  triax_expect_inrange(2, 1, 10);
  triax_expect_inrange(0, 2, 134);
  triax_expect_inrange(699, 2, 134);
  triax_expect_true(0);
  triax_assert_true(0);
  triax_assert_exit(0, exit(0));
}
TRIAX_VALIDATE_OUTCOME(test_removethis, TRIAXI_FAILED)
#endif
#endif
