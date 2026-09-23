#define RK_IMPL
#define TRIAX_IMPL

#include "conf.h"

// clang-format off
/*
gcc cli.c test_rk_arena.c test_rk_arenastack.c test_rk_bitset.c test_rk_bst.c test_rk_dict.c test_rk_pool.c test_rk_string.c test_rk_test.c test_rk_vec.c -O3 -g3 && ./a.out 
*/
// clang-format on

triax_suite(noisolation_recovery, .isolation = TRIAX_ISOLATION_OFF);
triax_test(noisolation_recovery, recover_abort_testerr) { abort(); }
triax_test(noisolation_recovery, recover_segfault_testerr) { raise(SIGSEGV); }
triax_test(noisolation_recovery, recover_abort_ucrash) { triax_expect_true((abort(), 1)); }
triax_test(noisolation_recovery, recover_segfault_ucrash) {
  triax_expect_true((raise(SIGSEGV), 1));
}
static const int static_params_int_range[5] = {1, 2, 3, 4, 5};
triax_test(test_parameterised, test0, .params = triax_as_params(static_params_int_range)) {
  const int* p = triax_param(int);
  triax_expect_eq(*p, 1);
}

triax_test(test_custom_msg, test0) {
  void*  ptr = (void*)0;
  size_t n   = 4;
  triax_assert(ptr != NULL,
               "alloc failed for %zu "
               "bytesfdaskfjdsaklfjsdlkafjsadlkjflksdajflkasdjflkdsjfkljdsaklfjds"
               "kajfkldsajflkjsdklfjdsl;jfklsadjfsend",
               n);
}
triax_test(test_unparameterised, t1, .isolation = TRIAX_ISOLATION_ON) {
  (void)triax_param(int); // exercises the "accessed with no parameter" user-error path
}
triax_test(print_quick, eg) {
  triax_expect_eq(0, 1);
  triax_expect_neq(0, 0);
}

triax_test(mem_output, memeq) {
  const char x[] = "hello world", y[] = "hello, my love";
  triax_expect_memeq(x, y, sizeof(x));
}

triax_test(edge_case, manyasserts) {
  for (unsigned i = 0; i < 100; ++i) {
    triax_expect(0, "at %u", i);
    triax_expect(1, "at %u", i);
  }
}

triax_test(edge_case, long_string) {
  char buf[16384 * 2 + 1];
  memset(buf, 'a', 16384);
  memset(buf + 16384, 'b', 16384);
  buf[sizeof(buf) - 1] = '\0';
  triax_assert_strneq(buf, buf);
}
triax_test(fatal_cases, noisolation_triaxi_fatal, .isolation = TRIAX_ISOLATION_OFF) {
  errno = 0;
  triaxi_fatal();
}
triax_test(fatal_cases, isolation_triaxi_fatal) {
  errno = 0;
  triaxi_fatal();
  triax_assert_eq(1, 2);
}
triax_test(fatal_cases, param_nexists_noisolation, .isolation = TRIAX_ISOLATION_OFF) {
  const int* p = triax_param(int);
  (void)p;
}
triax_test(fatal_cases, param_nexists_isolation) {
  const int* p = triax_param(int);
  (void)p;
}

triax_test(fatal_cases, segfault_noisolation, .isolation = TRIAX_ISOLATION_OFF) { raise(SIGSEGV); }
triax_test(fatal_cases, segfault_isolation) { raise(SIGSEGV); }
triax_test(fatal_cases, exit_from_noisolation, .isolation = TRIAX_ISOLATION_OFF) {
  triax_expect_exit(0, (void)0;);
}
triax_test(fatal_cases, continuation) {}

#define litcpy(dst, lit) memcpy(dst, lit, sizeof(lit))

triax_test(execs, sleeper, .isolation = TRIAX_ISOLATION_ON, .skip = true) {
  Triax_Str s = {0};
  triax_assert_eq(s, triax_str("hello", 4));
  triax_assert_lt(s, triax_str("hello", 4));
  triax_assert_lt(s, triax_str("hello", 4));

  switch (fork()) {
  case 0:
    while (1) {
      printf("Still alive\n");
      sleep(1);
    }
    break;
  default: triax_expect_true(0);
  }
  while (1) {
    printf("Still alive\n");
    sleep(1);
  }
}
triax_suite(empty_suite);
triax_test(print_both, t0) {
  fprintf(stderr, "I am out");
  fprintf(stdout, "I am error");
  triax_expect_false(1);
}

triax_test(test_triax_expect, t1) {
  triax_expect(0);
  triax_expect(0, "stuff failed");
}
triax_test(control_flow, t0) {}

triax_test(string_data, s1) {
  char      arr[] = {'h', 0, 'l', 0, 'o', 0, 'w', 0, 'r', 0, 'd', 0};
  Triax_Str a     = {arr, sizeof(arr) - 1};
  Triax_Str b     = {"hello suzan", sizeof("hello suzan") - 1};
  triax_expect_streq(a, b);
}

triax_test(open_failure, s1) {
  Triax_Str s = triax_read_file("I don't exist");
  triax_assert_streq(s, "nope");
}

triax_test(open_failure, s2) {
  Triax_Str s = triax_read_file(NULL);
  triax_assert_streq(s, "nope");
}

triax_test(new_suite, fail1) { triax_assert_eq(1, 2); }
triax_test(new_suite, fail2) { triax_assert_eq(1, 1), triax_assert_eq(1, 2); }
triax_test(new_suite, crash1) { triax_assert_eq(1, raise(SIGSEGV)); }
triax_test(new_suite, crash2) { triax_assert_eq(1, 1), triax_assert_eq(1, raise(SIGSEGV)); }
triax_test(new_suite, exit1, .isolation = TRIAX_ISOLATION_ON) { triax_assert_eq(1, (exit(0), 1)); }
triax_test(new_suite, exit2, .isolation = TRIAX_ISOLATION_ON) {
  triax_assert_eq(1, 1), triax_assert_eq(1, (exit(0), 1));
}
triax_test(new_suite, err_crash1) { raise(SIGSEGV); }
triax_test(new_suite, err_crash2) { triax_assert_eq(1, 1), raise(SIGSEGV); }
triax_test(new_suite, err_exit1, .isolation = TRIAX_ISOLATION_ON) { exit(0); }
triax_test(new_suite, err_exit2, .isolation = TRIAX_ISOLATION_ON) {
  triax_assert_eq(1, 1), exit(0);
}
triax_test(new_suite, err_after_fail_crash) { triax_expect_eq(1, 2), raise(SIGSEGV); }
triax_test(new_suite, err_after_fail_exit, .isolation = TRIAX_ISOLATION_ON) {
  triax_expect_eq(1, 2), exit(0);
}
triax_test(new_suite, expected_fault) { triax_assert_fault(SIGSEGV, raise(SIGSEGV)); }
triax_test(new_suite, wrong_fault) { triax_assert_fault(SIGABRT, raise(SIGSEGV)); }
triax_test(new_suite, expected_exit, .isolation = TRIAX_ISOLATION_ON) {
  triax_assert_exit(7, exit(7));
}
triax_test(new_suite, wrong_exit, .isolation = TRIAX_ISOLATION_ON) {
  triax_assert_exit(7, exit(8));
}
triax_test(new_suite, neg_fault) { triax_assert_nfault(raise(SIGSEGV)); }
triax_test(new_suite, neg_exit, .isolation = TRIAX_ISOLATION_ON) { triax_assert_nexit(exit(7)); }
#include <math.h>

triax_test(new_suite, nan_handling, .verbosity = TRIAX_VERBOSITY_ALWAYS) {
  float a = NAN, b = NAN;
  triax_expect_eq(a, b);
  triax_expect_neq(a, b); // should pass
  triax_expect_gt(a, b);
  triax_expect_lt(a, b);
  triax_expect_geq(a, b);
  triax_expect_leq(a, b);
  float nan = NAN;
  triax_expect_gt(nan, 1.0f); // fail
  triax_expect_gt(1.0f, nan); // fail

  triax_expect_lt(nan, 1.0f); // fail
  triax_expect_lt(1.0f, nan); // fail

  triax_expect_geq(nan, 1.0f); // fail
  triax_expect_leq(nan, 1.0f); // fail
}
triax_test(array_eq, t1) {
  int arr1[10] = {0, 1, 2}, arr2[10] = {0, 2, 3};
  triax_expect_arreq(arr1, arr2);
}
triax_test(array_eq, compounds) {
  int arr1[10] = {0, 1, 2};
  triax_expect_arreq(arr1, ((int[10]){0, 2, 3}));
}

// triax_test(new_suite, wrong_setup, .timeout_ms = 1,
//          .isolation = TRIAX_ISOLATION_OFF) {}

triax_suite(new_suite, .isolation = TRIAX_ISOLATION_OFF);
triax_test(new_suite2, eq_str) { triax_assert_eq("hello", "world"); }
triax_test(new_suite2, gt_lt_str) {
  triax_assert_gt("hello", "world");
  triax_assert_lt("hello", "world");
}
triax_test(new_suite2, sw) { triax_assert_str_startswith("hello", "world"); }

triax_test(string_nulls, sw) { triax_assert_str_startswith("hello", (const char*)0); }
triax_test(invalid_conf, t0, .timeout_ms = 10, .isolation = TRIAX_ISOLATION_OFF) {}
triax_test(invalid_conf, t1, .isolation = TRIAX_ISOLATION_OFF) { triax_assert_eq(1, 1); }
triax_test(invalid_conf, t2, .params = {1, 1, 0}) { triax_assert_eq(1, 1); }

#ifdef RK_RUN_PARALLEL
# define RK_OUTDIR "par"
#else
# define RK_OUTDIR "seq"
#endif
int main(int argc, char* argv[]) {
  int64_t         t0       = triaxi_now_ms();
  Triax_RunConfig defaults = {TRIAXI_ZINIT};
  Triax_RunConfig conf     = triax_parse_argv(argc, argv, defaults);
  const char *    text, *json, *junit, *tap;
  if (conf.attrs.isolation != TRIAX_ISOLATION_OFF) {
    text  = "./outputs/" RK_OUTDIR "/iso/out.txt";
    json  = "./outputs/" RK_OUTDIR "/iso/out.json";
    junit = "./outputs/" RK_OUTDIR "/iso/out.xml";
    tap   = "./outputs/" RK_OUTDIR "/iso/out.tap";
  } else {
    text  = "./outputs/" RK_OUTDIR "/nis/out.txt";
    json  = "./outputs/" RK_OUTDIR "/nis/out.json";
    junit = "./outputs/" RK_OUTDIR "/nis/out.xml";
    tap   = "./outputs/" RK_OUTDIR "/nis/out.tap";
  }
  if (!conf.outpaths.text) { conf.outpaths.text = text; }
  if (!conf.outpaths.json) { conf.outpaths.json = json; }
  if (!conf.outpaths.junit) { conf.outpaths.junit = junit; }
  if (!conf.outpaths.tap) { conf.outpaths.tap = tap; }
  // printf("Test");

  triax_run(conf);
  uint32_t duration = (uint32_t)(triaxi_now_ms() - t0);
  printf("total time: %" PRIu32 "ms\n", duration);
}
