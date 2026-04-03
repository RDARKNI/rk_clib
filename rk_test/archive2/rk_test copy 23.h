/// @file rk_test.h
/// @version 1.0
/// @brief Single-Header Test Framework for C
///
/// Criterion-Like Header-Only Test Framework.
/// Runs each test in its own process with customisable verbosity levels,
/// stdout/stderr and meta information file redirection.
///
/// Todo:
///     - Refactoring (macro undefs)
///     - Testing Windows Compatibility
///     - Better Customisation Interface (perhaps automatic main creation,
///     command line passing and test recognition)
///     - ...
///

#ifndef RK_TEST_H
#define RK_TEST_H
#ifdef _WIN32
# pragma section(".CRT$XCU", read)
# include <windows.h>
#else
# include <fcntl.h>
# include <poll.h>
# include <signal.h>
# include <sys/stat.h>
# include <sys/time.h>
# include <sys/wait.h>
# include <unistd.h>
#endif
#include <assert.h>
#include <errno.h>
#ifdef __cplusplus
# include <cmath>
# include <sstream>
# include <type_traits>
#endif
#include <inttypes.h>
#include <math.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/// Configuration resolution model:
///
/// Settings are resolved using a cascading hierarchy:
///
///     Test → Suite → Global → Built-in default
///
/// Each level may specify a concrete value or use INHERIT
/// to defer the decision to the next level.
///
/// If Global is INHERIT, the setting resolves to a
/// framework-defined built-in default, which is the first enum value after
/// INHERIT
///
/// Resolution always produces a concrete value.

/// @brief Verbosity level of the output
typedef enum RK_Verbosity {
  RK_VERBOSITY_INHERIT,
  RK_VERBOSITY_AUTO,   ///< Output depending on type of output/result
  RK_VERBOSITY_ALWAYS, ///< Print everything
  RK_VERBOSITY_NEVER,
} RK_Verbosity;
#define RK_VERBOSITY_DEFAULT RK_VERBOSITY_AUTO

/// @brief Whether a test function runs in its own process (more robust) or in
/// the main process (faster)
typedef enum RK_IsolationMode {
  RK_ISOLATION_INHERIT,
  RK_ISOLATION_ON,
  RK_ISOLATION_OFF
} RK_IsolationMode;
#define RK_ISOLATION_DEFAULT RK_ISOLATION_ON

/// @brief Output Streams from Test Functions todo
typedef enum RK_OutPutStreams {
  RK_OUTPUTSTREAMS_STDOUT, // captured stdout of the child process
  RK_OUTPUTSTREAMS_STDERR, // captured stderr of the child process
  RK_OUTPUTSTREAMS_LOG,    // assertion information
} RK_OutPutStreams;

/// @brief Output todo
typedef enum RK_OutputFormats {
  RK_OUTPUTFORMAT_NORMAL,
  RK_OUTPUTFORMAT_JSON,
  RK_OUTPUTFORMAT_TAP,
} RK_OutputFormats;

/// @brief Fixture functions take no arguments may run multiple times per test,
/// in different processes
typedef void (*RK_FixtureFunc)(void);

/// @brief Customisable attributes (global, suite or test-level)
typedef struct RK_CustomTestAttributes {
  const char*      tags;                /// Tags todo docs
  RK_FixtureFunc   init;                /// Setup fixture
  RK_FixtureFunc   fini;                /// Teardown fixture
  RK_Verbosity     verbosity_levels[3]; /// verbosity level, per RK_OutPutStream
  RK_IsolationMode isolation;
  long             timeout_ms; /// Timeout in ms (default is no timeout)
} RK_CustomTestAttributes;

/// @brief todo documentation
#define RK_REGISTER_SUITE(SUITENAME, ...)                                      \
  RKT_REGISTER_SUITE_IMPL(SUITENAME, __VA_ARGS__)

/// @brief todo documentation
#define RK_REGISTER_TEST(SUITENAME, fn, ...)                                   \
  RKT_REGISTER_TEST_IMPL(SUITENAME, fn, __VA_ARGS__)

/// @brief todo documentation
#define RK_RUN_TESTS(suites, nsuites, ...)                                     \
  RKT_RUN_TESTS_IMPL(suites, nsuites, __VA_ARGS__)

// clang-format off
#define rk_expect_true(expr)                  RKT_AE1(expect, true, expr, #expr)
#define rk_expect_false(expr)                 RKT_AE1(expect, false, expr, #expr)
#define rk_expect_null(ptr)                   RKT_AE1(expect, null, ptr, #ptr)
#define rk_expect_nonnull(ptr)                RKT_AE1(expect, nonnull, ptr, #ptr)
#define rk_expect_memeq(ptr1, ptr2, siz)      RKT_AE3(expect, memeq, ptr1, ptr2, siz, #ptr1, #ptr2, #siz)
#define rk_expect_memneq(ptr1, ptr2, siz)     RKT_AE3(expect, memneq, ptr1, ptr2, siz, #ptr1, #ptr2, #siz)
#define rk_expect_memzero(ptr, siz)           RKT_AE2(expect, memzero, ptr, siz, #ptr, #siz)
#define rk_expect_memnzero(ptr, siz)          RKT_AE2(expect, memnzero, ptr, siz, #ptr, #siz)
#define rk_expect_eq(exp, act)                RKT_AE2(expect, eq, exp, act, #exp, #act)
#define rk_expect_neq(exp, act)               RKT_AE2(expect, neq, exp, act, #exp, #act)
#define rk_expect_lt(exp, act)                RKT_AE2(expect, lt, exp, act, #exp, #act)
#define rk_expect_leq(exp, act)               RKT_AE2(expect, leq, exp, act, #exp, #act)
#define rk_expect_gt(exp, act)                RKT_AE2(expect, gt, exp, act, #exp, #act)
#define rk_expect_geq(exp, act)               RKT_AE2(expect, geq, exp, act, #exp, #act)
#define rk_expect_inrange(val, low, high)     RKT_AE3(expect, inrange, val, low, high, #val, #low, #high)
#define rk_expect_floateq_tol(exp, act, tol)  RKT_AE3(expect, floateq_tol, exp, act, tol, #exp, #act, #tol)
#define rk_expect_floatneq_tol(exp, act, tol) RKT_AE3(expect, floatneq_tol, exp, act, tol, #exp, #act, #tol)
#define rk_expect_streq(str1, str2)           RKT_AE2(expect, streq, str1, str2, #str1, #str2)
#define rk_expect_strneq(str1, str2)          RKT_AE2(expect, strneq, str1, str2, #str1, #str2)
#define rk_expect_streq_n(str1, str2, len)    RKT_AE3(expect, streq_n, str1, str2, len, #str1, #str2, #len)
#define rk_expect_strneq_n(str1, str2, len)   RKT_AE3(expect, strneq_n, str1, str2, len, #str1, #str2, #len)
#define rk_expect_stdouteq(errstr, len)       RKT_AE2(expect, stdouteq, errstr, len, #errstr, #len)
#define rk_expect_stdoutneq(errstr, len)      RKT_AE2(expect, stdoutneq, errstr, len, #errstr, #len)
#define rk_expect_stderreq(errstr, len)       RKT_AE2(expect, stderreq, errstr, len, #errstr, #len)
#define rk_expect_stderrneq(errstr, len)      RKT_AE2(expect, stderrneq, errstr, len, #errstr, #len)

#define rk_assert_true(expr)                  RKT_AE1(assert, true, expr, #expr)
#define rk_assert_false(expr)                 RKT_AE1(assert, false, expr, #expr)
#define rk_assert_null(ptr)                   RKT_AE1(assert, null, ptr, #ptr)
#define rk_assert_nonnull(ptr)                RKT_AE1(assert, nonnull, ptr, #ptr)
#define rk_assert_memeq(ptr1, ptr2, siz)      RKT_AE3(assert, memeq, ptr1, ptr2, siz, #ptr1, #ptr2, #siz)
#define rk_assert_memneq(ptr1, ptr2, siz)     RKT_AE3(assert, memneq, ptr1, ptr2, siz, #ptr1, #ptr2, #siz)
#define rk_assert_memzero(ptr, siz)           RKT_AE2(assert, memzero, ptr, siz, #ptr, #siz)
#define rk_assert_memnzero(ptr, siz)          RKT_AE2(assert, memnzero, ptr, siz, #ptr, #siz)
#define rk_assert_eq(exp, act)                RKT_AE2(assert, eq, exp, act, #exp, #act)
#define rk_assert_neq(exp, act)               RKT_AE2(assert, neq, exp, act, #exp, #act)
#define rk_assert_lt(exp, act)                RKT_AE2(assert, lt, exp, act, #exp, #act)
#define rk_assert_leq(exp, act)               RKT_AE2(assert, leq, exp, act, #exp, #act)
#define rk_assert_gt(exp, act)                RKT_AE2(assert, gt, exp, act, #exp, #act)
#define rk_assert_geq(exp, act)               RKT_AE2(assert, geq, exp, act, #exp, #act)
#define rk_assert_inrange(val, low, high)     RKT_AE3(assert, inrange, val, low, high, #val, #low, #high)
#define rk_assert_floateq_tol(exp, act, tol)  RKT_AE3(assert, floateq_tol, exp, act, tol, #exp, #act, #tol)
#define rk_assert_floatneq_tol(exp, act, tol) RKT_AE3(assert, floatneq_tol, exp, act, tol, #exp, #act, #tol)
#define rk_assert_streq(str1, str2)           RKT_AE2(assert, streq, str1, str2, #str1, #str2)
#define rk_assert_strneq(str1, str2)          RKT_AE2(assert, strneq, str1, str2, #str1, #str2)
#define rk_assert_streq_n(str1, str2, len)    RKT_AE3(assert, streq_n, str1, str2, len, #str1, #str2, #len)
#define rk_assert_strneq_n(str1, str2, len)   RKT_AE3(assert, strneq_n, str1, str2, len, #str1, #str2, #len)
#define rk_assert_stdouteq(errstr, len)       RKT_AE2(assert, stdouteq, errstr, len, #errstr, #len)
#define rk_assert_stdoutneq(errstr, len)      RKT_AE2(assert, stdoutneq, errstr, len, #errstr, #len)
#define rk_assert_stderreq(errstr, len)       RKT_AE2(assert, stderreq, errstr, len, #errstr, #len)
#define rk_assert_stderrneq(errstr, len)      RKT_AE2(assert, stderrneq, errstr, len, #errstr, #len)

// clang-format on
#define rk_assert_crash(signal, ...)                                           \
  do {                                                                         \
    RKT_SEND_HDR("assert_crash(" #signal ", " #__VA_ARGS__ ")", RKTF_crash,    \
                 signal);                                                      \
    __VA_ARGS__;                                                               \
    {                                                                          \
      RKT_AssertResPkg RK_PKG = {2, 0};                                        \
      RKT_send_res(&RK_PKG);                                                   \
    }                                                                          \
    exit(0);                                                                   \
  } while (0)

#define rk_assert_exit(code, ...)                                              \
  do {                                                                         \
    RKT_SEND_HDR("assert_exit(" #code ", " #__VA_ARGS__ ")", RKTF_exit, code); \
    __VA_ARGS__;                                                               \
    {                                                                          \
      RKT_AssertResPkg RK_PKG = {2, 0};                                        \
      RKT_send_res(&RK_PKG);                                                   \
    }                                                                          \
    exit(0);                                                                   \
  } while (0)

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////  Implementation   /////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define RKT_AE1(AE, FUN, A1, S1)                                               \
  RKT_CONCAT(RKT_, AE)(#AE "_" #FUN "(" S1 ")", RKTF_##FUN, RKTf_##FUN(A1))
#define RKT_AE2(AE, FUN, A1, A2, S1, S2)                                       \
  RKT_CONCAT(RKT_, AE)(#AE "_" #FUN "(" S1 ", " S2 ")", RKTF_##FUN,            \
                       RKTf_##FUN(A1, A2))
#define RKT_AE3(AE, FUN, A1, A2, A3, S1, S2, S3)                               \
  RKT_CONCAT(RKT_, AE)(#AE "_" #FUN "(" S1 ", " S2 ", " S3 ")", RKTF_##FUN,    \
                       RKTf_##FUN(A1, A2, A3))

#define RKT_expect(EXPRSTR, FUNENUM, FUNCALL)                                  \
  (RKT_SEND_HDR(EXPRSTR, FUNENUM, 0), (FUNCALL))

#define RKT_assert(EXPRSTR, FUNENUM, FUNCALL)                                  \
  (RKT_SEND_HDR(EXPRSTR, FUNENUM, 0),                                          \
   ((FUNCALL) ? (longjmp(RKT_glob.fret, 1)) : ((void)0)))

typedef enum RKT_TestEndType {
  RKT_PASSED,
  RKT_FAILED,   // Regular failure
  RKT_CRASHED,  // Unexpected crash
  RKT_EXITED,   // Unexpected exit
  RKT_TIMEDOUT, // Test timed out
  RKT_TESTERROR // Error in the testing function
} RKT_TestEndType;

#ifdef _WIN32
typedef HANDLE              rkt_fd;
typedef PROCESS_INFORMATION rkt_pid;
#else
typedef int   rkt_fd;
typedef pid_t rkt_pid;
#endif

typedef enum RKT_TestAssertFunction {
  RKTF_true,
  RKTF_false,
  RKTF_null,
  RKTF_nonnull,
  RKTF_eq,
  RKTF_floateq_tol,
  RKTF_neq,
  RKTF_floatneq_tol,
  RKTF_lt,
  RKTF_leq,
  RKTF_gt,
  RKTF_geq,
  RKTF_inrange,
  RKTF_memeq,
  RKTF_memneq,
  RKTF_memzero,
  RKTF_memnzero,
  RKTF_streq,
  RKTF_strneq,
  RKTF_streq_n,
  RKTF_strneq_n,
  RKTF_crash,
  RKTF_exit,
  RKTF_stdouteq,
  RKTF_stdoutneq,
  RKTF_stderreq,
  RKTF_stderrneq,
} RKT_TestAssertFunction;

typedef struct RKT_Strv {
  char*  str;
  size_t len;
} RKT_Strv;

typedef struct RKT_Cstrv {
  const char* str;
  size_t      len;
} RKT_Cstrv;

typedef enum RKT_CrashReason {
  RKT_SIGNAL_ANY = -1,
  RKT_SIGNAL_NONE,
  RKT_SIGNAL_SEGFAULT,            // access violation / SIGSEGV
  RKT_SIGNAL_ILLEGAL_INSTRUCTION, // SIGILL, EXCEPTION_ILLEGAL_INSTRUCTION
  RKT_SIGNAL_ABORT,               // SIGABRT, fatal app exit
  RKT_SIGNAL_ARITHMETIC,          // SIGFPE, FP/INT divide-by-zero
  RKT_SIGNAL_BREAKPOINT,          // SIGTRAP or EXCEPTION_BREAKPOINT
  RKT_SIGNAL_UNKNOWN,
} RKT_CrashReason;

typedef enum RKT_ExitType {
  RKT_EXIT_EXITED,
  RKT_EXIT_CRASHED,
  RKT_EXIT_DISCARD
} RKT_ExitType;

typedef struct RKT_Exit {
  RKT_ExitType type;
  union {
    RKT_CrashReason reason;
    int             exit_code;
  };
} RKT_Exit;

/// @brief Header to be sent to the test runner before assert/expect
typedef struct RKT_AssertHdr {
  int                    pos; ///< The line of the function
  RKT_TestAssertFunction F;   ///< The assert function (assert_eq, assert_true)
  union {
    int             exit_code;
    RKT_CrashReason reason;
  };
  struct {
    char          str[243]; ///< The stringified assertion
    unsigned char len;
  } expr;
} RKT_AssertHdr;

#define RKT_ASSERTRES_PASS    (((size_t)(0)))
#define RKT_ASSERTRES_TIMEOUT (((size_t)(-1)) - 2)
#define RKT_ASSERTRES_CRASH   (((size_t)(-1)) - 1)
#define RKT_ASSERTRES_EXIT    (((size_t)(-1)))

/// @brief the arguments and result (true/false or other state) is stored here
typedef struct RKT_AssertResPkg {
  size_t res;
  size_t len;
  char   args[];
} RKT_AssertResPkg;

typedef union RKT_AssertRes {
  RKT_AssertResPkg pkg;
  struct {
    size_t res;
    size_t len;
    char*  args[6];
  };
} RKT_AssertRes;

/// @brief Data packet for each assertion
typedef struct RKT_AssertDat {
  RKT_AssertHdr hdr; ///< Received before each assert runs
  RKT_AssertRes res; ///< Received after each assert
} RKT_AssertDat;

typedef struct RKT_Suite {
  struct RKT_Suite*       next;
  struct RKT_TestEntry*   tests;
  const RKT_Cstrv         name;
  RK_CustomTestAttributes attrs;
} RKT_Suite;

/// @brief Summarised results of each Test function; owns its pointers
typedef struct RKT_TestResult {
  size_t          count;   ///< Number of assertions in the test
  size_t          fails;   ///< Failed assertions in the test
  RKT_AssertDat*  results; ///< Result of each assertion in the test
  RKT_Strv        capt[2]; ///< Captured stdout/stderr
  RKT_TestEndType ended;   ///< If the crash/exit was expected, a timeout etc.
  RKT_Exit        exit_status; ///< Actual exit type and
} RKT_TestResult;

/// @brief Data and results of each test function
typedef struct RKT_TestEntry {
  struct RKT_TestEntry* next;
  RKT_Suite*            suite;
  const RKT_Cstrv       name;
  const RKT_Cstrv       file;
  void (*const func)(void);
  RK_CustomTestAttributes attrs;
  union {
    RKT_TestResult res;
    RKT_Cstrv      tmp_suitename;
  };
} RKT_TestEntry;

/// @brief Global settings/variables for the testing framework
static struct {
  RK_CustomTestAttributes attrs;
  const char*             custom_paths[3]; // todo this system is a mess
  FILE*                   output_types[3]; // todo rename to default files?
  RKT_Suite*              suites;
  size_t                  tests_run;
  struct /* do not modify after initialisation */ {
    rkt_fd tmpfds[3]; // fds of tmp files, open for duration of program
    rkt_fd saved[2];
  };
  rkt_fd  log;
  jmp_buf fret;
  rkt_pid pid;
#ifndef _WIN32
  sigjmp_buf fret_sig;
#else
  rkt_fd job;
#endif
  union {
    RKT_AssertResPkg resbuf;
    char             backing_storage[2048]; // buffer for optimisation
  };
} RKT_glob;

#ifdef __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wpragmas"
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
# pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
# pragma GCC diagnostic ignored "-Wc2x-extensions"
# pragma GCC diagnostic ignored "-Wunused-function"
#endif

#ifdef __cplusplus
# define rkt_fun inline
# define RKT_ZINIT
#else
# define rkt_fun   static inline
# define RKT_ZINIT 0
#endif

#if defined(__cplusplus) && __cplusplus >= 201103L
# define RKT_noreturn [[noreturn]]
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
# define RKT_noreturn _Noreturn
#elif defined(__GNUC__)
# define RKT_noreturn __attribute__((noreturn))
#elif defined(_MSC_VER)
# define RKT_noreturn __declspec(noreturn)
#else
# define RKT_noreturn
#endif

#if !defined(restrict) && (defined(_MSC_VER) || defined(__cplusplus))
# define restrict __restrict
#endif

#define RKT_CONCAT(a, b)           RKT_CONCAT2(a, b)
#define RKT_CONCAT2(a, b)          a##b

#define RKT_COUNTOF(...)           (sizeof(__VA_ARGS__) / sizeof((__VA_ARGS__)[0]))
#define RKT_lenof(STRLIT)          (sizeof("" STRLIT "") - 1)
#define RK_MIN(X, Y)               ((X) <= (Y) ? (X) : (Y))
#define RK_MAX(X, Y)               ((X) >= (Y) ? (X) : (Y))
#define RKT_malloc(T, COUNT)       ((T*)malloc(sizeof(T) * (COUNT)))
#define RKT_realloc(T, PTR, COUNT) ((T*)realloc(PTR, sizeof(T) * (COUNT)))

rkt_fun const char* RKT_strcrash(RKT_CrashReason reason) {
  switch (reason) {
  case RKT_SIGNAL_ANY                : return "Crashed (any)";
  case RKT_SIGNAL_NONE               : return "No crash";
  case RKT_SIGNAL_SEGFAULT           : return "Segmentation fault / access violation";
  case RKT_SIGNAL_ILLEGAL_INSTRUCTION: return "Illegal instruction";
  case RKT_SIGNAL_ABORT              : return "Abort (SIGABRT / fatal app exit)";
  case RKT_SIGNAL_ARITHMETIC         : return "Divide by zero";
  case RKT_SIGNAL_BREAKPOINT         : return "Breakpoint trap";
  case RKT_SIGNAL_UNKNOWN            : return "Unknown crash";
  default                            : return "Invalid crash reason";
  }
}

RKT_noreturn rkt_fun void RKT_fatal(const char* str);
rkt_fun void              RKT_init_group(void);
rkt_fun int             RKT_write_full(rkt_fd fd, const void* buf, size_t len);
rkt_fun int             RKT_read_full_tm(rkt_fd fd, void* buf, size_t nbytes,
                                         long timeout_ms);
rkt_fun void            RKT_init_test(const RKT_TestEntry* restrict e);
rkt_fun RKT_TestEndType RKT_parent_loop(RKT_TestEntry* restrict e,
                                        bool timed_out);
rkt_fun RKT_TestEndType RKT_run_test(RKT_TestEntry* e);
rkt_fun void            RKT_run_suite(RKT_Suite* s);
rkt_fun void            RKT_init_global(RK_CustomTestAttributes attrs);
rkt_fun void     RKT_run_all(const char** suites_strs, size_t nsuites_strs,
                             RK_CustomTestAttributes attrs);

rkt_fun RKT_Exit RKT_inspect_exit(int code) {
  RKT_Exit res;
#ifndef _WIN32
  if (WIFSIGNALED(code)) {
    res.type = RKT_EXIT_CRASHED;
    switch (WTERMSIG(code)) {
    case SIGSEGV:
    case SIGBUS : res.reason = RKT_SIGNAL_SEGFAULT; break;
    case SIGILL : res.reason = RKT_SIGNAL_ILLEGAL_INSTRUCTION; break;
    case SIGABRT: res.reason = RKT_SIGNAL_ABORT; break;
    case SIGFPE : res.reason = RKT_SIGNAL_ARITHMETIC; break;
    case SIGTRAP: res.reason = RKT_SIGNAL_BREAKPOINT; break;
    default     : res.reason = RKT_SIGNAL_UNKNOWN; break;
    }
  } else {
    res.type = RKT_EXIT_EXITED, res.exit_code = WEXITSTATUS(code);
  }
#else
  if (code >= 0x80000000) {
    res.type = RKT_EXIT_CRASHED;
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:
    case EXCEPTION_STACK_OVERFLOW  : res.reason = RKT_SIGNAL_SEGFAULT; break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
      res.reason = RKT_SIGNAL_ILLEGAL_INSTRUCTION;
      break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    case EXCEPTION_FLT_INVALID_OPERATION:
      res.reason = RKT_SIGNAL_ARITHMETIC;
      break;
    case EXCEPTION_BREAKPOINT : res.reason = RKT_SIGNAL_BREAKPOINT; break;
    case STATUS_FATAL_APP_EXIT: res.reason = RKT_SIGNAL_ABORT; break;
    default                   : res.reason = RKT_SIGNAL_UNKNOWN; break;
    }
  } else {
    res.type = RKT_EXIT_EXITED, res.exit_code = code;
  }
#endif
  return res;
}

rkt_fun long RKT_resolve_timeout(const RKT_TestEntry* restrict e) {
  if (e->attrs.timeout_ms) { return e->attrs.timeout_ms; }
  if (e->suite->attrs.timeout_ms) { return e->suite->attrs.timeout_ms; }
  if (RKT_glob.attrs.timeout_ms) { return RKT_glob.attrs.timeout_ms; }
  return -1;
}
rkt_fun bool RKT_resolve_isolation(const RKT_TestEntry* restrict e) {
  if (e->attrs.isolation) { return e->attrs.isolation != RK_ISOLATION_OFF; }
  if (e->suite->attrs.isolation) {
    return e->suite->attrs.isolation != RK_ISOLATION_OFF;
  }
  return RKT_glob.attrs.isolation != RK_ISOLATION_OFF;
}

rkt_fun RK_Verbosity RKT_resolve_verbosity(const RKT_TestEntry* restrict e,
                                           RK_OutPutStreams type) {
  if (e->attrs.verbosity_levels[type]) {
    return e->attrs.verbosity_levels[type];
  } else if (e->suite->attrs.verbosity_levels[type]) {
    return e->suite->attrs.verbosity_levels[type];
  }
  return RKT_glob.attrs.verbosity_levels[type];
}

rkt_fun bool RKT_shall_print(const RKT_TestEntry* e, RK_OutPutStreams s) {
  RK_Verbosity v = RKT_resolve_verbosity(e, s);
  if (v == RK_VERBOSITY_NEVER) { return false; }
  switch (s) {
  case RK_OUTPUTSTREAMS_STDOUT:
    return e->res.capt[s].len
        && (e->res.ended != RKT_PASSED || v == RK_VERBOSITY_ALWAYS);
  case RK_OUTPUTSTREAMS_STDERR: return e->res.capt[s].len > 0;
  case RK_OUTPUTSTREAMS_LOG   : return true;
  }
  assert(0);
}

rkt_fun void RKT_send_hdr(RKT_AssertHdr hdr) {
  if (RKT_write_full(RKT_glob.log, &hdr, sizeof(hdr)) < 1) {
    RKT_fatal("CHILD SEND HDR");
  }
}
#define RKT_SEND_HDR(ESTR, FUNENUM, SIG)                                       \
  RKT_send_hdr((RKT_AssertHdr){__LINE__, FUNENUM, SIG, {ESTR, RKT_lenof(ESTR)}})

rkt_fun size_t RKT_send_res(const RKT_AssertResPkg* buf) {
  if (RKT_write_full(RKT_glob.log, buf, sizeof(*buf) + buf->len) < 1) {
    RKT_fatal("CHILD SEND RES");
  }
  return buf->res;
}

rkt_fun RKT_Strv RKT_readfile(rkt_fd fd);

/// @brief returns first position where strings differ +1 (0 if same)
rkt_fun size_t   RKT_strdiff(const char* e1, size_t l1, const char* e2,
                             size_t l2) {
  size_t min = RK_MIN(l1, l2);
  for (size_t i = 0; i < min; ++i) {
    if (e1[i] != e2[i]) { return i + 1; }
  }
  return l1 == l2 ? 0 : (min + 1);
}

#define RKT_SNDBUF0(RES)                                                       \
  RKT_glob.resbuf.res = (RES), RKT_glob.resbuf.len = 0;                        \
  return RKT_send_res(&RKT_glob.resbuf);
#define RKT_SNDBUF1(RES, e1, FMT1)                                             \
  RKT_glob.resbuf.res = (RES);                                                 \
  RKT_glob.resbuf.len = sprintf(RKT_glob.resbuf.args, FMT1, e1) + 1;           \
  return RKT_send_res(&RKT_glob.resbuf);
#define RKT_SNDBUF2(RES, e1, FMT1, e2, FMT2)                                   \
  RKT_glob.resbuf.res = (RES);                                                 \
  RKT_glob.resbuf.len                                                          \
      = sprintf(RKT_glob.resbuf.args, FMT1 "%c" FMT2, e1, '\0', e2) + 1;       \
  return RKT_send_res(&RKT_glob.resbuf);
#define RKT_SNDBUF3(RES, e1, FMT1, e2, FMT2, e3, FMT3)                         \
  RKT_glob.resbuf.res = (RES);                                                 \
  RKT_glob.resbuf.len                                                          \
      = sprintf(RKT_glob.resbuf.args, FMT1 "%c" FMT2 "%c" FMT3, e1, '\0', e2,  \
                '\0', e3)                                                      \
      + 1;                                                                     \
  return RKT_send_res(&RKT_glob.resbuf);

#ifndef __cplusplus
# define RKT_FUNCASE(T, N, F, FTYPE)     , T : RKTf_##FTYPE##_##N
# define RKT_FUNCASE_TOL(T, N, F, FTYPE) , T : RKTf_##FTYPE##_##N

# define RKT_SELFUN(FTYPE, VAL)                                                \
   _Generic((VAL)RKT_TYPELIST(RKT_FUNCASE, FTYPE),                             \
       char*: RKTf_streq,                                                      \
       const char*: RKTf_streq,                                                \
       default: RKTf_##FTYPE##_vp)
# define RKT_SELFUN_TOL(FTYPE, EXP)                                            \
   _Generic((EXP)RKT_TYPELIST_FLOAT(RKT_FUNCASE_TOL, FTYPE))
#else
# define RKT_SELFUN(FTYPE, EXP)     RKTf_##FTYPE
# define RKT_SELFUN_TOL(FTYPE, EXP) RKTf_##FTYPE
#endif

#ifndef _WIN32
# define RKT_SCHAR_CASE(Y, ...) Y(__VA_ARGS__)
#else
# define RKT_SCHAR_CASE(Y, ...)
#endif
#define RKT_TYPELIST_NOFLOAT(Y, ...)                                           \
  Y(unsigned char, uc, "%hhu", ##__VA_ARGS__)                                  \
  Y(unsigned short, us, "%hu", ##__VA_ARGS__)                                  \
  Y(unsigned, ui, "%u", ##__VA_ARGS__)                                         \
  Y(unsigned long, ul, "%lu", ##__VA_ARGS__)                                   \
  Y(unsigned long long, ull, "%llu", ##__VA_ARGS__)                            \
  Y(char, c, "%c", ##__VA_ARGS__)                                              \
  Y(short, s, "%hd", ##__VA_ARGS__)                                            \
  Y(int, i, "%d", ##__VA_ARGS__)                                               \
  Y(long, sl, "%ld", ##__VA_ARGS__)                                            \
  Y(long long, sll, "%lld", ##__VA_ARGS__)                                     \
  Y(bool, b, "%d", ##__VA_ARGS__)                                              \
  Y(const void*, vp, "%p", ##__VA_ARGS__)                                      \
  RKT_SCHAR_CASE(Y, signed char, sc, "%hhd", ##__VA_ARGS__)
#define RKT_TYPELIST_FLOAT(Y, ...)                                             \
  Y(float, f, "%f", ##__VA_ARGS__)                                             \
  Y(double, d, "%f", ##__VA_ARGS__)                                            \
  Y(long double, ld, "%Lf", ##__VA_ARGS__)
#define RKT_TYPELIST(Y, ...)                                                   \
  RKT_TYPELIST_FLOAT(Y, ##__VA_ARGS__)                                         \
  RKT_TYPELIST_NOFLOAT(Y, ##__VA_ARGS__)

#ifdef __cplusplus
class RK_TestStream : public std::ostream {
public:
  RK_TestStream() : std::ostream(&streambuf) {}
  template <class T>
  void add_arg(const T& val) {
    add_arg_impl<T>(val, streamable<T>{});
  }
  size_t send(size_t res_val) {
    RKT_AssertResPkg* res = (RKT_AssertResPkg*)streambuf.buf;
    res->res = res_val, res->len = streambuf.len - sizeof(RKT_AssertResPkg);
    return RKT_send_res(res);
  }

private:
# if __cpp_lib_format >= 201907L // todo
#  include <format>
  template <typename T, typename CharT = char>
  concept RK_formattable = requires (const T& value) {
    std::formatter<T, CharT>{};
    std::format(std::basic_string<CharT>{"{}"}, value);
  };
# endif
  template <typename...>
  using void_t = void;
  template <typename T, typename U = void>
  struct streamable : std::false_type {};
  template <typename T>
  struct streamable<T, void_t<decltype(std::declval<std::ostream&>()
                                       << std::declval<T>())> /**/>
      : std::true_type {};

  template <class T>
  void add_arg_impl(const T& val, std::true_type) {
    *this << val, this->put('\0');
  }
  template <class T>
  void add_arg_impl(const T& val, std::false_type) { // backup, print address
    *this << static_cast<const void*>(&val), this->put('\0');
  }
  class RKT_Sbuf : public std::streambuf {
    friend RK_TestStream;

  private:
    char*  buf{RKT_glob.backing_storage};
    size_t len{hdr_size}, cap{mem_size};
    ~RKT_Sbuf() {
      if (cap > mem_size) { free(buf); }
    }
    static const size_t hdr_size = sizeof(RKT_AssertResPkg),
                        mem_size = sizeof(RKT_glob.backing_storage);
    void grow_to(size_t ncap) {
      if (cap == mem_size) {
        char* nbuf = RKT_malloc(char, ncap);
        if (!nbuf) { RKT_fatal("child out of memory"); } // todo out of memory
        memcpy(nbuf, buf, mem_size), buf = nbuf;
      } else {
        if (!(buf = RKT_realloc(char, buf, ncap))) {
          RKT_fatal("child out of memory");
        }
      }
      cap = ncap;
    }
    virtual std::streamsize xsputn(const char* s, std::streamsize n) override {
      if (n + len > cap) { grow_to(n + cap * 2); }
      return memcpy(buf + len, s, n), len += n, n;
    }
    virtual int_type overflow(int_type ch) override {
      if (ch != traits_type::eof()) {
        if (cap == len) { grow_to(cap * 2); }
        buf[len++] = ch;
      }
      return ch;
    }
  } streambuf;
};

# define RKT_GEN_fargs_tmplate(F)                                              \
   F(T, U, "", eq, !(e1 == e2))                                                \
   F(T, U, "", neq, !(e1 != e2))                                               \
   F(T, U, "", gt, (e1 > e2 ? 0 : (e1 == e2 ? 1 : 2)))                         \
   F(T, U, "", geq, !(e1 >= e2))                                               \
   F(T, U, "", lt, (e1 < e2 ? 0 : (e1 == e2 ? 1 : 2)))                         \
   F(T, U, "", leq, !(e1 <= e2))

rkt_fun size_t RKTf_streq(const char* e1, const char* e2);
rkt_fun size_t RKTf_strneq(const char* e1, const char* e2);
# define RKT_char_mkfun(C1, C2)                                                \
   rkt_fun size_t RKTf_eq(C1 char* e1, C2 char* e2) {                          \
     return RKTf_streq(e1, e2);                                                \
   }
# define RKT_tmpchartypes(Y) Y(const, const) Y(const, ) Y(, const) Y(, )
RKT_tmpchartypes(RKT_char_mkfun)
# undef RKT_char_mkfun
# undef RKT_tmpchartypes
#endif

#ifndef __cplusplus
# define RKT_GEN_f2(T, N, FMT, name, expr)                                     \
   rkt_fun size_t RKTf_##name##_##N(T e1, T e2) {                              \
     RKT_SNDBUF2((expr), e1, FMT, e2, FMT);                                    \
   }
# define RKT_GEN_f3(T, N, FMT, name, expr)                                     \
   rkt_fun size_t RKTf_##name##_##N(T e1, T e2, T e3) {                        \
     RKT_SNDBUF3((expr), e1, FMT, e2, FMT, e3, FMT);                           \
   }
#else
# define RKT_GEN_f2(T, N, FMT, name, expr)                                     \
   rkt_fun size_t RKTf_##name(T e1, T e2) {                                    \
     RKT_SNDBUF2((expr), e1, FMT, e2, FMT);                                    \
   }
# define RKT_GEN_f3(T, N, FMT, name, expr)                                     \
   rkt_fun size_t RKTf_##name(T e1, T e2, T e3) {                              \
     RKT_SNDBUF3((expr), e1, FMT, e2, FMT, e3, FMT);                           \
   }

# define RKT_GEN_f2TMPLATE(T, U, FMT, name, expr)                              \
   template <class T, class U>                                                 \
   rkt_fun size_t RKTf_##name(T e1, U e2) {                                    \
     RK_TestStream stream;                                                     \
     stream.add_arg(e1), stream.add_arg(e2);                                   \
     return stream.send((size_t)(expr));                                       \
   }

RKT_GEN_fargs_tmplate(RKT_GEN_f2TMPLATE)
#endif

#define RKTf_eq(exp, act)            RKT_SELFUN(eq, exp)(exp, act)
#define RKTf_neq(exp, act)           RKT_SELFUN(neq, exp)(exp, act)
#define RKTf_lt(exp, act)            RKT_SELFUN(lt, exp)(exp, act)
#define RKTf_leq(exp, act)           RKT_SELFUN(leq, exp)(exp, act)
#define RKTf_gt(exp, act)            RKT_SELFUN(gt, exp)(exp, act)
#define RKTf_geq(exp, act)           RKT_SELFUN(geq, exp)(exp, act)
#define RKTf_inrange(val, low, high) RKT_SELFUN(inrange, val)(val, low, high)
#define RKTf_floateq_tol(exp, act, tol)                                        \
  RKT_SELFUN_TOL(floateq_tol, exp)(exp, act, tol)
#define RKTf_floatneq_tol(exp, act, tol)                                       \
  RKT_SELFUN_TOL(floatneq_tol, exp)(exp, act, tol)

// clang-format off
#define RKT_GEN_fargs_all(T, N, FMT, ...)                                 \
    RKT_GEN_f2(T, N, FMT, lt, (e1 < e2 ? 0 : (e1 == e2 ? 1 : 2)))         \
    RKT_GEN_f2(T, N, FMT, leq, !(e1 <= e2))                               \
    RKT_GEN_f2(T, N, FMT, gt, (e1 > e2 ? 0 : (e1 == e2 ? 1 : 2)))         \
    RKT_GEN_f2(T, N, FMT, geq, !(e1 >= e2))                               \
    RKT_GEN_f3(T, N, FMT, inrange, (e1 < e2 ? 1 : (e1 > e3 ? 2 : 0)))
#define RKT_GEN_fargs_nofloat(T, N, FMT, ...)                             \
    RKT_GEN_f2(T, N, FMT, eq, !(e1 == e2))                                \
    RKT_GEN_f2(T, N, FMT, neq, !(e1 != e2))                       
#define RKT_GEN_fargs_float(T, N, FMT, eps_default, fabs_fn)              \
    RKT_GEN_f2(T, N, FMT, eq, !(fabs_fn(e1 - e2) <= eps_default))         \
    RKT_GEN_f2(T, N, FMT, neq, !(fabs_fn(e1 - e2) > eps_default))         \
    RKT_GEN_f3(T, N, FMT, floateq_tol, !(fabs_fn(e1 - e2) <= e3))               \
    RKT_GEN_f3(T, N, FMT, floatneq_tol, !(fabs_fn(e1 - e2) > e3))

RKT_TYPELIST_NOFLOAT(RKT_GEN_fargs_nofloat)                
RKT_TYPELIST(RKT_GEN_fargs_all)                       
RKT_GEN_fargs_float(float, f, "%g", 1e-6f, fabsf)              
RKT_GEN_fargs_float(double, d, "%g", 1e-12, fabs)          
RKT_GEN_fargs_float(long double, ld, "%Lg", 1e-12L, fabsl)


rkt_fun size_t RKTf_true(bool e1) { RKT_SNDBUF0(!e1); }
rkt_fun size_t RKTf_false(bool e1) { RKT_SNDBUF0(!!e1); } // clang-format on

rkt_fun size_t RKTf_null(const void* e1) { RKT_SNDBUF1(!(!e1), e1, "%p"); }
rkt_fun size_t RKTf_nonnull(const void* e1) { RKT_SNDBUF1(!(e1), e1, "%p"); }

rkt_fun size_t RKTf_memzero(const void* e1, size_t e2) {
  size_t dif = 0;
  for (size_t i = 0; i < e2; ++i) {
    if (((const char*)e1)[i]) {
      dif = i + 1;
      break;
    }
  }
  RKT_SNDBUF2(dif, e1, "%p", e2, "%zu");
}
rkt_fun size_t RKTf_memnzero(const void* e1, size_t e2) {
  const void* l;
  size_t r = (!e2 || !(l = memchr(e1, 0, e2))) ? 0 : ((char*)l - (char*)e1 + 1);
  RKT_SNDBUF2(r, e1, "%p", e2, "%zu");
}

rkt_fun size_t RKTf_streq(const char* e1, const char* e2) {
  RKT_AssertResPkg* res;
  size_t l1 = strlen(e1), l2 = strlen(e2), dif = RKT_strdiff(e1, l1, e2, l2);
  size_t package_size = sizeof(*res) + l1 + l2 + 2;
  if (package_size <= sizeof(RKT_glob.backing_storage)) {
    res = &RKT_glob.resbuf;
  } else {
    if (!(res = (RKT_AssertResPkg*)malloc(package_size))) { exit(12); }
  }
  res->res = dif, res->len = l1 + l2 + 2;
  memcpy(res->args, e1, l1 + 1), memcpy(res->args + l1 + 1, e2, l2 + 1);
  RKT_send_res(res);
  if (package_size > sizeof(RKT_glob.backing_storage)) { free(res); }
  return dif;
}
rkt_fun size_t RKTf_strneq(const char* e1, const char* e2) {
  RKT_AssertResPkg* res;
  size_t l1 = strlen(e1), l2 = strlen(e2), dif = RKT_strdiff(e1, l1, e2, l2);
  size_t package_size = sizeof(*res) + l1 + l2 + 2;
  if (package_size <= sizeof(RKT_glob.backing_storage)) {
    res = &RKT_glob.resbuf;
  } else {
    if (!(res = (RKT_AssertResPkg*)malloc(package_size))) { exit(12); }
  }
  res->res = !(dif != 0), res->len = l1 + l2 + 2;
  memcpy(res->args, e1, l1 + 1), memcpy(res->args + l1 + 1, e2, l2 + 1);
  RKT_send_res(res);
  if (package_size > sizeof(RKT_glob.backing_storage)) { free(res); }
  return !(dif != 0);
}

rkt_fun size_t RKTf_memeq(const void* e1, const void* e2, size_t e3) {
  RKT_SNDBUF3(!(!e3 || !memcmp(e1, e2, e3)), e1, "%p", e2, "%p", e3, "%zu");
}
rkt_fun size_t RKTf_memneq(const void* e1, const void* e2, size_t e3) {
  RKT_SNDBUF3(!!(!e3 || !memcmp(e1, e2, e3)), e1, "%p", e2, "%p", e3, "%zu");
}

rkt_fun size_t RKTf_streq_n(const char* e1, const char* e2, size_t n) {
  RKT_AssertResPkg* res;
  if (!n) {
    res = &RKT_glob.resbuf, res->res = 0, res->len = 0;
    return RKT_send_res(res);
  }
  size_t dif          = RKT_strdiff(e1, n, e2, n);
  size_t package_size = sizeof(*res) + n + n + 2;
  if (package_size <= sizeof(RKT_glob.backing_storage)) {
    res = &RKT_glob.resbuf;
  } else {
    if (!(res = (RKT_AssertResPkg*)malloc(package_size))) { exit(12); }
  }
  res->res = dif, res->len                               = n + n + 2;
  memcpy(res->args, e1, n), res->args[n]                 = '\0';
  memcpy(res->args + n + 1, e2, n), res->args[n + 1 + n] = '\0';
  RKT_send_res(res);
  if (package_size > sizeof(RKT_glob.backing_storage)) { free(res); }
  return dif;
}
rkt_fun size_t RKTf_strneq_n(const char* e1, const char* e2, size_t n) {
  RKT_AssertResPkg* res;
  if (!n) {
    res = &RKT_glob.resbuf, res->res = 1, res->len = 0;
    return RKT_send_res(res);
  }
  size_t dif          = RKT_strdiff(e1, n, e2, n);
  size_t package_size = sizeof(*res) + n + n + 2;
  if (package_size <= sizeof(RKT_glob.backing_storage)) {
    res = &RKT_glob.resbuf;
  } else {
    if (!(res = (RKT_AssertResPkg*)malloc(package_size))) { exit(12); }
  }
  res->res = !(dif != 0), res->len                       = n + n + 2;
  memcpy(res->args, e1, n), res->args[n]                 = '\0';
  memcpy(res->args + n + 1, e2, n), res->args[n + 1 + n] = '\0';
  RKT_send_res(res);
  if (package_size > sizeof(RKT_glob.backing_storage)) { free(res); }
  return !(dif != 0);
}
// todo
rkt_fun size_t RKTf_streameq(RK_OutPutStreams stream, bool no, const char* str,
                             size_t len) {
  fflush(NULL);
  RKT_Strv          o = RKT_readfile(RKT_glob.tmpfds[stream]);
  RKT_AssertResPkg* res;
  if (!len && !o.len) {
    res = &RKT_glob.resbuf, res->res = 0, res->len = 0;
    return RKT_send_res(res);
  }
  size_t dif          = RKT_strdiff(str, len, o.str, o.len);
  size_t package_size = sizeof(*res) + len + o.len + 2;
  if (package_size <= sizeof(RKT_glob.backing_storage)) {
    res = &RKT_glob.resbuf;
  } else {
    if (!(res = (RKT_AssertResPkg*)malloc(package_size))) { exit(12); }
  }
  res->res = no ? dif : !(dif != 0), res->len = len + o.len + 2;
  if (len) { memcpy(res->args, str, len); }
  res->args[len] = '\0';
  if (o.len) { memcpy(res->args + len + 1, o.str, o.len); }
  res->args[len + 1 + o.len] = '\0';
  RKT_send_res(res);
  if (package_size > sizeof(RKT_glob.backing_storage)) { free(res); }
  return dif;
}
rkt_fun size_t RKTf_stdouteq(const char* str, size_t len) {
  return RKTf_streameq(RK_OUTPUTSTREAMS_STDOUT, 0, str, len);
}
rkt_fun size_t RKTf_stdoutneq(const char* str, size_t len) {
  return RKTf_streameq(RK_OUTPUTSTREAMS_STDOUT, 1, str, len);
}
rkt_fun size_t RKTf_stderreq(const char* str, size_t len) {
  return RKTf_streameq(RK_OUTPUTSTREAMS_STDERR, 0, str, len);
}
rkt_fun size_t RKTf_stderrneq(const char* str, size_t len) {
  return RKTf_streameq(RK_OUTPUTSTREAMS_STDERR, 1, str, len);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////Printing     ///////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#define RK_RESET               "\x1b[0m"
#define RK_RED                 "\x1b[31m"
#define RK_GREEN               "\x1b[32m"
#define RK_YELLOW              "\x1b[33m"
#define RK_BLUE                "\x1b[34m"
#define RK_INRED(str)          RK_RED str RK_RESET
#define RK_INGREEN(str)        RK_GREEN str RK_RESET
#define RK_INBLUE(str)         RK_BLUE str RK_RESET
#define RK_INYELLOW(str)       RK_YELLOW str RK_RESET
#define RKT_FMT_P              RK_INGREEN("[P] ")
#define RKT_FMT_F              RK_INRED("[F] ")
#define RKT_FMT_C              RK_INRED("[C] ")
#define RKT_FMT_E              RK_INRED("[E] ")
#define RKT_FMT_T              RK_INRED("[T] ")
#define RK_catlit(s, l)        (memcpy(s, l, RKT_lenof(l)), RKT_lenof(l))
#define RK_catstr(S, STR, LEN) (memcpy(S, STR, LEN), LEN)
#define RKT_fputs(STR, STREAM) fwrite(STR, 1, RKT_lenof(STR), STREAM)

rkt_fun size_t RKT_fmt_assertres(const RKT_TestEntry* restrict e,
                                 RKT_AssertDat cur, char** restrict fmtbuf,
                                 size_t* restrict fmtcap) {
#define RKT_fmtstub(_S, FMT, _E, _HDR)                                         \
  sprintf(_S, "%s %s:%d %s", FMT, _E->file.str, _HDR->pos, _HDR->expr.str)

  const RKT_AssertHdr* hdr  = &cur.hdr;
  RKT_AssertRes        res  = cur.res;
  char* const*         args = res.args;
  char*                s    = *fmtbuf;
  switch (res.res) {
  case RKT_ASSERTRES_PASS:
    if (RKT_resolve_verbosity(e, RK_OUTPUTSTREAMS_LOG) == RK_VERBOSITY_ALWAYS) {
      s += RKT_fmtstub(s, RKT_FMT_P, e, hdr); // todo done twice right now
    } else {
      return 0;
    }
    break;
  case RKT_ASSERTRES_EXIT : s += RKT_fmtstub(s, RKT_FMT_E, e, hdr); break;
  case RKT_ASSERTRES_CRASH: s += RKT_fmtstub(s, RKT_FMT_C, e, hdr); break;
  case RKT_ASSERTRES_TIMEOUT:
    if (!hdr->pos) {
      s += RK_catlit(s, RK_INRED("[T] TEST TIMEOUT\n"));
    } else {
      s += RKT_fmtstub(s, RKT_FMT_T, e, hdr);
    }
    break;
  default:
    if (res.len + 256 > *fmtcap) {
      *fmtcap = res.len + 256, *fmtbuf = RKT_realloc(char, *fmtbuf, *fmtcap);
      if (!*fmtbuf) { RKT_fatal("Realloc fmt"); }
      s = *fmtbuf;
    }
    s += RKT_fmtstub(s, RKT_FMT_F, e, hdr), *s++ = ':', *s++ = ' ';
    switch (hdr->F) {
    case RKTF_true   : s += RK_catlit(s, RK_INRED(" == 'false'")); break;
    case RKTF_false  : s += RK_catlit(s, RK_INRED(" == 'true'")); break;
    case RKTF_null   : s += sprintf(s, RK_INRED("%s != 'NULL'"), args[0]); break;
    case RKTF_nonnull: s += RK_catlit(s, RK_INRED(" == 'NULL'")); break;
    case RKTF_eq:
    case RKTF_floateq_tol:
      s += sprintf(s, RK_INRED("%s != %s"), args[0], args[1]);
      break;
    case RKTF_neq:
    case RKTF_floatneq_tol:
      s += sprintf(s, RK_INRED("%s == %s"), args[0], args[1]);
      break;
    case RKTF_lt:
      s += sprintf(s, RK_INRED("%s%s%s"), args[0],
                   res.res == 1 ? " == " : " > ", args[1]);
      break;
    case RKTF_leq:
      s += sprintf(s, RK_INRED("%s > %s"), args[0], args[1]);
      break;
    case RKTF_gt:
      s += sprintf(s, RK_INRED("%s%s%s"), args[0],
                   res.res == 1 ? " == " : " < ", args[1]);
      break;
    case RKTF_geq:
      s += sprintf(s, RK_INRED("%s < %s"), args[0], args[1]);
      break;
    case RKTF_inrange:
      s += sprintf(s, RK_INRED("%s%s%s"), args[0], res.res == 1 ? " < " : " > ",
                   res.res == 1 ? args[1] : args[2]);
      break;
    case RKTF_memeq:
      s += sprintf(s, "%s and %s (%s bytes long) " RK_INRED("not equal"),
                   args[0], args[1], args[2]);
      break;
    case RKTF_memneq:
      s += sprintf(s, "%s and %s (%s bytes long) " RK_INRED("equal"), args[0],
                   args[1], args[2]);
      break;
    case RKTF_memzero:
      s += sprintf(s, "%s (%s bytes long) " RK_INRED("nonzero at byte %zu"),
                   args[0], args[1], res.res - 1);
      break;
    case RKTF_memnzero:
      s += sprintf(s, "%s (%s bytes long) " RK_INRED("zero at byte %zu"),
                   args[0], args[1], res.res - 1);
      break;
    case RKTF_streq:
    case RKTF_streq_n:
      s += sprintf(s, "%s != %s " RK_INRED("position %zu"), args[0], args[1],
                   res.res - 1);
      break;
    case RKTF_strneq:
    case RKTF_strneq_n:
      s += sprintf(s, "%s " RK_INRED("==") " %s", args[0], args[0]);
      break;
    case RKTF_crash:
      if (res.res == 2) {
        s += RK_catlit(s, "Did not crash");
      } else {
        s += sprintf(s, "Expected: %s, got: %s", RKT_strcrash(hdr->reason),
                     RKT_strcrash(e->res.exit_status.reason));
      }
      break;
    case RKTF_exit:
      if (res.res == 2) {
        s += RK_catlit(s, "Did not exit");
      } else {
        s += sprintf(s, "Expected: exit via code %d, got: exit with code %d\n",
                     hdr->exit_code, e->res.exit_status.exit_code);
      }
      break;
    case RKTF_stdouteq:
      s += sprintf(s, "%s " RK_INRED("!=") " %s", args[0], args[1]);
      break;
    case RKTF_stdoutneq:
      s += sprintf(s, "%s " RK_INRED("==") " %s", args[0], args[1]);
      break;
    case RKTF_stderreq:
      s += sprintf(s, "%s " RK_INRED("!=") " %s", args[0], args[1]);
      break;
    case RKTF_stderrneq:
      s += sprintf(s, "%s " RK_INRED("==") " %s", args[0], args[1]);
      break;
    }
  }
  *s++ = '\n';
  return s - *fmtbuf;
#undef RKT_fmtstub
}
rkt_fun void RKT_print_test_reg(const RKT_TestEntry* restrict e) {
  RKT_TestResult r   = e->res;
  FILE*          out = RKT_glob.output_types[RK_OUTPUTFORMAT_NORMAL];
  if (!out) { return; }
  fprintf(out,
          RK_INYELLOW("Running %s\n") "---------------------------------------"
                                      "-----------\n",
          e->name.str);
  {
    size_t fmtcap = 1024;
    char*  fmtbuf = RKT_malloc(char, fmtcap);
    if (!fmtbuf) { RKT_fputs("Error: Realloc fmt failed\n", stderr), exit(1); }
    RK_Verbosity v = RKT_resolve_verbosity(e, RK_OUTPUTSTREAMS_LOG);

    for (size_t i = 0; i < r.count; ++i) {
      if (v == RK_VERBOSITY_NEVER
          || !r.results[i].res.res && v != RK_VERBOSITY_ALWAYS) {
        continue;
      }
      size_t bytes = RKT_fmt_assertres(e, r.results[i], &fmtbuf, &fmtcap);
      fwrite(fmtbuf, 1, bytes, out);
    }
    free(fmtbuf);
  }
  if (RKT_shall_print(e, RK_OUTPUTSTREAMS_STDOUT)) {
    RKT_fputs("\nStdout:\n", out);
    fwrite(r.capt[RK_OUTPUTSTREAMS_STDOUT].str, 1,
           r.capt[RK_OUTPUTSTREAMS_STDOUT].len, out);
    fputc('\n', out);
  }
  if (RKT_shall_print(e, RK_OUTPUTSTREAMS_STDERR)) {
    RKT_fputs("\nStderr:\n", out);
    fwrite(r.capt[RK_OUTPUTSTREAMS_STDERR].str, 1,
           r.capt[RK_OUTPUTSTREAMS_STDERR].len, out);
    fputc('\n', out);
  }
  switch (r.ended) {
  case RKT_PASSED:
    fprintf(out, "-> " RK_INGREEN("PASS") ": All %zu assertions succeeded\n\n",
            r.count);
    break;
  case RKT_FAILED:
    fprintf(out,
            "-> " RK_INRED("FAIL") ": %zu fails, %zu passed (total %zu)\n\n",
            r.fails, r.count - r.fails, r.count);
    break;
  case RKT_CRASHED:
    fprintf(out,
            "-> " RK_INRED("FAIL") ": %zu fails, %zu passed (total %zu), "
                                   "crashed unexpectedly with: %s\n\n",
            r.fails, r.count - r.fails, r.count,
            RKT_strcrash(r.exit_status.reason));
    break;
  case RKT_EXITED:
    fprintf(out,
            "-> " RK_INRED("FAIL") ": %zu fails, %zu passed (total %zu), "
                                   "exited unexpectedly with code %d\n\n",
            r.fails, r.count - r.fails, r.count, r.exit_status.exit_code);
    break;
  case RKT_TIMEDOUT:
    fprintf(out,
            "-> " RK_INRED("TIMED OUT") ": %zu fails, %zu passed (total %zu), "
                                        "exceeded time of %ldms\n\n",
            r.fails, r.count - r.fails, r.count, RKT_resolve_timeout(e));
    break;
  case RKT_TESTERROR:
    if (r.exit_status.type == RKT_EXIT_CRASHED) {
      fprintf(out, RK_INRED("Error in Test Function: %s\n\n"),
              RKT_strcrash(r.exit_status.reason));
    } else {
      fprintf(out, RK_INRED("Error in Test Function; Exited with code %d\n\n"),
              r.exit_status.exit_code);
    }
  }
}
rkt_fun void RKT_print_suite_reg(const RKT_Suite* s, size_t total,
                                 size_t failed, size_t crashed,
                                 size_t timed_out, size_t error) {
  FILE* out = RKT_glob.output_types[RK_OUTPUTFORMAT_NORMAL];
  if (!out) { return; }
  fprintf(out, RK_INBLUE("Suite %s\n"), s->name.str);
  for (const RKT_TestEntry* e = s->tests; e; e = e->next) {
    RKT_print_test_reg(e);
  }
  fprintf(out,
          "--- Passed %zu, failed %zu, crashed %zu, timed-out %zu out of %zu "
          "tests ---\n\n",
          total - failed - crashed, failed, crashed, timed_out, total);
}
rkt_fun void RKT_print_total_beg_json() {
  if (RKT_glob.output_types[RK_OUTPUTFORMAT_JSON]) {
    RKT_fputs("{\n  \"suites\": [\n",
              RKT_glob.output_types[RK_OUTPUTFORMAT_JSON]);
  }
}
rkt_fun void RKT_print_total_end_json() {
  if (RKT_glob.output_types[RK_OUTPUTFORMAT_JSON]) {
    RKT_fputs("  ]\n}\n", RKT_glob.output_types[RK_OUTPUTFORMAT_JSON]);
  }
}

rkt_fun void RKT_json_escape_n(const char* str, size_t len) {
  FILE* out = RKT_glob.output_types[RK_OUTPUTFORMAT_JSON];
  if (!str) { return; }
  const char *start = str, *end = str + len;
  while (str < end) {
    unsigned char c = (unsigned char)*str;
    if (c >= 0x20 && c != '"' && c != '\\') {
      str++;
    } else {
      if (str > start) { fwrite(start, 1, (size_t)(str - start), out); }
      const char* w;
      size_t      l = 2;
      switch (c) {
      case '"' : w = "\\\""; break;
      case '\\': w = "\\\\"; break;
      case '\b': w = "\\b"; break;
      case '\f': w = "\\f"; break;
      case '\n': w = "\\n"; break;
      case '\r': w = "\\r"; break;
      case '\t': w = "\\t"; break;
      default  : {
        static const char hex[] = "0123456789abcdef";
        char buf[6] = {'\\', 'u', '0', '0', hex[c >> 4], hex[c & 15]};
        w = buf, l = 6;
        break;
      }
      }
      fwrite(w, 1, l, out);
      str++, start = str;
    }
  }
  /* trailing safe run */
  if (str > start) { fwrite(start, 1, (size_t)(str - start), out); }
}

rkt_fun void RKT_print_test_json(const RKT_TestEntry* e) {
  FILE* out = RKT_glob.output_types[RK_OUTPUTFORMAT_JSON];
  if (!out) { return; }
  const RKT_TestResult* r = &e->res;
  /* Header */
  RKT_fputs("    {\n"
            "      \"test_name\": \"",
            out);
  RKT_json_escape_n(e->name.str, e->name.len);
  RKT_fputs("\",\n"
            "      \"file\": \"",
            out);
  RKT_json_escape_n(e->file.str, e->file.len);
  RKT_fputs("\",\n"
            "      \"tags\": \"",
            out);
  const char*  tags     = e->attrs.tags;
  const size_t tags_len = tags ? strlen(tags) : 0;
  RKT_json_escape_n(tags, tags_len);
  RKT_fputs("\",\n"
            "      \"result\": {\n",
            out);
  fprintf(out, "        \"ended\": %d,\n", r->ended);
  if (r->exit_status.type == RKT_EXIT_CRASHED) {
    fprintf(out, "        \"reason\": %s,\n",
            RKT_strcrash(r->exit_status.reason));
  } else {
    fprintf(out, "        \"term_code\": %d,\n", r->exit_status.exit_code);
  }
  fprintf(out,
          "        \"assert_count\": %zu,\n"
          "        \"assert_failures\": %zu,\n",
          r->count, r->fails);
  /* Assertions */
  if (!r->count) {
    RKT_fputs("        \"assertions\": []", out);
  } else {
    RKT_fputs("        \"assertions\": [\n", out);
    for (size_t i = 0; i < r->count; ++i) {
      RKT_AssertDat* ad = &r->results[i];
      RKT_fputs("          {\n"
                "            \"expr\": \"",
                out);
      RKT_json_escape_n(ad->hdr.expr.str, ad->hdr.expr.len);
      fprintf(out,
              "\",\n"
              "            \"pos\": %d,\n",
              ad->hdr.pos);
      const char* resstr;
      switch (ad->res.res) {
      case RKT_ASSERTRES_PASS   : resstr = "pass"; break;
      case RKT_ASSERTRES_EXIT   : resstr = "exit"; break;
      case RKT_ASSERTRES_CRASH  : resstr = "crash"; break;
      case RKT_ASSERTRES_TIMEOUT: resstr = "timeout"; break;
      default                   : resstr = "fail"; break;
      }
      fprintf(out,
              "            \"res\": \"%s\",\n"
              "            \"args\": [",
              resstr);

      /* args (max 3) */
      for (int j = 0; j < 3; ++j) {
        const char* a0 = ad->res.args[j];
        const char* a1 = ad->res.args[j + 1];
        if (!a1) { break; }
        if (j) { RKT_fputs(", ", out); }
        fputc('"', out);
        RKT_json_escape_n(a0, (size_t)(a1 - a0 - 1));
        fputc('"', out);
      }
      RKT_fputs("]\n"
                "          }",
                out);
      if (i + 1 < r->count) { fputc(',', out); }
      fputc('\n', out);
    }
    RKT_fputs("        ]", out);
  }
  /* Captured output */
  if (RKT_shall_print(e, RK_OUTPUTSTREAMS_STDOUT)) {
    RKT_fputs(",\n        \"stdout\": \"", out);
    RKT_json_escape_n(r->capt[RK_OUTPUTSTREAMS_STDOUT].str,
                      r->capt[RK_OUTPUTSTREAMS_STDOUT].len);
    fputc('"', out);
  }
  if (RKT_shall_print(e, RK_OUTPUTSTREAMS_STDERR)) {
    RKT_fputs(",\n        \"stderr\": \"", out);
    RKT_json_escape_n(r->capt[RK_OUTPUTSTREAMS_STDERR].str,
                      r->capt[RK_OUTPUTSTREAMS_STDERR].len);
    fputc('"', out);
  }
  /* Footer */
  RKT_fputs("\n"
            "      }\n"
            "    }",
            out);
  if (e->next) { fputc(',', out); }
  fputc('\n', out);
}

rkt_fun void RKT_print_suite_json(const RKT_Suite* s, size_t total,
                                  size_t failed, size_t crashed,
                                  size_t timed_out, size_t error) {
  FILE* out = RKT_glob.output_types[RK_OUTPUTFORMAT_JSON];
  if (!out) { return; }
  RKT_fputs("  {\n"
            "    \"suite_name\": \"",
            out);

  RKT_json_escape_n(s->name.str, s->name.len);
  RKT_fputs("\",\n"
            "    \"tests\": [\n",
            out);
  for (const RKT_TestEntry* e = s->tests; e; e = e->next) {
    RKT_print_test_json(e);
  }
  RKT_fputs("    ]\n"
            "  }",
            out);
  if (s->next) { fputc(',', out); }
  fputc('\n', out);
}
rkt_fun void RKT_print_test_tap(const RKT_TestEntry* e) {
  FILE* out = RKT_glob.output_types[RK_OUTPUTFORMAT_TAP];
  if (!out) { return; }
  RKT_TestResult r = e->res;
  fputs(e->res.ended == RKT_PASSED ? "ok " : "not ok ", out);
  fprintf(out, "%zu ", RKT_glob.tests_run);
  fwrite(e->suite->name.str, 1, e->suite->name.len, out);
  RKT_fputs(" > ", out);
  fputs(e->name.str, out);
  // fputs("\n --- \n", out);
  RKT_fputs(" # -> ", out);
  switch (e->res.ended) {
  case RKT_PASSED:
    fprintf(out, "PASS: Passed all %zu/%zu Assertions.", r.count, r.count);
    break;
  case RKT_FAILED:
    fprintf(out, "FAIL: Passed %zu/%zu Assertions", r.count - r.fails, r.count);
    break;
  case RKT_CRASHED:
    fprintf(out,
            "UCrash: Passed %zu/%zu Assertions, crashed unexpectedly with: %s",
            r.count - r.fails, r.count, RKT_strcrash(r.exit_status.reason));
    break;
  case RKT_EXITED:
    fprintf(
        out,
        "UExit: Passed %zu/%zu Assertions, exited unexpectedly with code %d",
        r.count - r.fails, r.count, r.exit_status.exit_code);
    break;
  case RKT_TIMEDOUT:
    fprintf(out, "TIMED OUT: Passed %zu/%zu Assertions, exceeded time of %ldms",
            r.count - r.fails, r.count, RKT_resolve_timeout(e));
    break;
  case RKT_TESTERROR:
    RKT_fputs(RK_INRED("Error in Test Function: "), out);
    if (r.exit_status.type == RKT_EXIT_CRASHED) {
      fprintf(out, RK_INRED("%s\n\n"), RKT_strcrash(r.exit_status.reason));
    } else {
      fprintf(out, RK_INRED("Exited with code %d\n\n"),
              r.exit_status.exit_code);
    }
  }
  /* todo yaml tap */
  fputc('\n', out);
}
rkt_fun void RKT_print_end_tap(void) {
  FILE* out = RKT_glob.output_types[RK_OUTPUTFORMAT_TAP];
  if (!out) { return; }
  fprintf(out, "1..%zu", RKT_glob.tests_run);
}

rkt_fun void RKT_cleanup_test_res(RKT_TestEntry* e) {
  while (e->res.count--) { free(e->res.results[e->res.count].res.args[0]); }
  free(e->res.capt[0].str), free(e->res.capt[1].str);
  free(e->res.results);
  e->res.count = 0;
}
rkt_fun void RKT_cleanup_suite_res(RKT_Suite* s) {
  for (RKT_TestEntry* e = s->tests; e; e = e->next) { RKT_cleanup_test_res(e); }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////PROGRAM LOGIC///////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

rkt_fun int RKT_init_test_noisolation(const RKT_TestEntry* restrict e);

#ifndef _WIN32
rkt_fun void RKT_handle_sigterm(int sigterm) {
  // Sigterm does not automatically propagate to child processes in a process
  // group, so we manually forward it to all processes in the group.
  kill(-getpid(), sigterm);
  _exit(1); // TODO exit code
}
#endif

RKT_noreturn rkt_fun void RKT_fatal(const char* str) {
#ifndef _WIN32
  perror(str);
  fflush(NULL);
  if (RKT_glob.pid) {
    sigset_t mask;
    sigemptyset(&mask), sigaddset(&mask, SIGTERM);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    kill(0, SIGTERM); // Kill all children (not parent?)
  }
  abort(); // Abort for core dump
#else
  char* buf = NULL;
  DWORD len = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER
                                 | FORMAT_MESSAGE_FROM_SYSTEM
                                 | FORMAT_MESSAGE_IGNORE_INSERTS,
                             NULL, GetLastError(), 0, (LPSTR)&buf, 0, NULL);
  if (!len || !buf) {
    fprintf(stderr, "%s: %s\n", str, "Unknown error");
  } else {
    fprintf(stderr, "%s: %s%c", str, buf, buf[len - 1] == '\n' ? '\0' : '\n');
  }
  if (buf) { LocalFree(buf); }
  TerminateJobObject(RKT_glob.job, 1);
  abort();
#endif
}
rkt_fun RKT_Exit RKT_reapchild() {
#ifndef _WIN32
  int status;
  while (waitpid(RKT_glob.pid, &status, 0) == -1) {
    if (errno != EINTR) { RKT_fatal("waitpid"); }
  }
  RKT_glob.pid = 0;
#else
  DWORD status;
  WaitForSingleObject(RKT_glob.pid.hProcess, INFINITE);
  GetExitCodeProcess(RKT_glob.pid.hProcess, &status);
  CloseHandle(RKT_glob.pid.hProcess), CloseHandle(RKT_glob.pid.hThread);
  RKT_glob.pid.hProcess = NULL;
#endif
  return RKT_inspect_exit(status);
}

rkt_fun int64_t RKT_now_ms(void) {
#ifndef _WIN32
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#else
  static LARGE_INTEGER freq = {RKT_ZINIT};
  if (!freq.QuadPart) { QueryPerformanceFrequency(&freq); }
  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);
  return (counter.QuadPart * 1000LL) / freq.QuadPart;
#endif
}

rkt_fun int RKT_write_full(rkt_fd fd, const void* buf, size_t len) {
  const char* ptr = (const char*)buf;
#ifndef _WIN32
  for (ssize_t w; len;) {
    if ((w = write(fd, ptr, len)) == -1) {
      if (errno != EINTR) { return -1; }
    } else {
      len -= (size_t)w, ptr += w;
    }
  }
#else
  for (DWORD w; len > 0; ptr += w, len -= w) {
    if (!WriteFile(fd, ptr, (DWORD)len, &w, NULL)) { return -1; }
  }
#endif
  return 1;
}
/*
-3: timeout
-2: EOF while reading (incomplete read)
-1: Other error
0: EOF
1: all read, success
*/
rkt_fun int RKT_read_full_tm(rkt_fd fd, void* buf, size_t nbytes,
                             long timeout_ms) {
  char* ptr      = (char*)buf;
  long  deadline = (timeout_ms == -1) ? -1 : RKT_now_ms() + timeout_ms;
#ifndef _WIN32
  struct pollfd pfd = {.fd = fd, .events = POLLIN};
  for (ssize_t r; nbytes;) {
    for (long remaining = -1;;) {
      if (deadline != -1) {
        remaining = deadline - RKT_now_ms();
        if (remaining <= 0) { return -3; }
      }
      switch (poll(&pfd, 1, (int)remaining)) {
      case -1:
        if (errno != EINTR) { return -1; }
        continue;
      case 0: return -3;
      default:
        if (pfd.revents & (POLLERR | POLLNVAL)) { return -1; }
      }
      break;
    }
    switch ((r = read(fd, ptr, nbytes))) {
    case -1:
      if (errno != EINTR) { return -1; }
      break;
    case 0 : return (void*)ptr == buf ? 0 : -2; // eof
    default: ptr += r, nbytes -= r;
    }
  }
#else
  for (DWORD r; nbytes;) {
    if (deadline != -1) {
      long remaining = deadline - RKT_now_ms();
      if (remaining <= 0) { return -3; }
      switch (WaitForSingleObject(
          fd, (remaining > INFINITE - 1) ? INFINITE : (DWORD)remaining)) {
      case WAIT_OBJECT_0: break; // readable
      case WAIT_TIMEOUT : return -3;
      default           : return -1;
      }
    }
    DWORD chunk = (DWORD)((nbytes > 0xFFFFFFFFUL) ? 0xFFFFFFFFUL : nbytes);
    if (!ReadFile(fd, ptr, chunk, &r, NULL)) {
      switch (GetLastError()) {
      case ERROR_BROKEN_PIPE      : return (ptr == (char*)buf) ? 0 : -2;
      case ERROR_OPERATION_ABORTED: continue;
      default                     : return -1;
      }
    }
    if (!r) { return ((void*)ptr == buf) ? 0 : -2; }
    ptr += r, nbytes -= r;
  }
#endif
  return 1; // read all, no eof
}

rkt_fun int RKT_read_full(rkt_fd fd, void* buf, size_t nbytes) {
  int res = RKT_read_full_tm(fd, buf, nbytes, -1);
  if (res == -3) { RKT_fatal("IMPOSSIBLE TIMEOUT"); }
  return res;
}

rkt_fun void RKT_rewindfile(rkt_fd fd) {
#ifndef _WIN32
  if (lseek(fd, 0, SEEK_SET) == (off_t)-1) { RKT_fatal("lseek"); }
#else
  LARGE_INTEGER liDistance = {RKT_ZINIT};
  if (!SetFilePointerEx(fd, liDistance, NULL, FILE_BEGIN)) {
    RKT_fatal("SetFilePointerEx");
  }
#endif
}

rkt_fun RKT_Strv RKT_readfile(rkt_fd fd) {
  RKT_Strv res;
  RKT_rewindfile(fd);
  {
#ifndef _WIN32
    struct stat st;
    if (fstat(fd, &st) != 0) { RKT_fatal("fstat"); }
    if (st.st_size < 0 || (uintmax_t)st.st_size > SIZE_MAX) {
      RKT_fatal("file too large");
    }
    res.len = (size_t)st.st_size;
#else
    LARGE_INTEGER size64;
    if (!GetFileSizeEx(fd, &size64)) { RKT_fatal("GetFileSizeEx"); }
    if (size64.QuadPart < 0 || (uintmax_t)size64.QuadPart > SIZE_MAX) {
      RKT_fatal("file too large");
    }
    res.len = (size_t)size64.QuadPart;
#endif
  }
  if (res.len == 0) { return res.str = NULL, res; }
  if (!(res.str = RKT_malloc(char, res.len))) { RKT_fatal("malloc read fd"); }
  switch (RKT_read_full(fd, res.str, res.len)) {
  case -2: RKT_fatal("readfile unexpected EOF");
  case -1: RKT_fatal("readfile error");
  case 0 : RKT_fatal("readfile unexpected EOF");
  case 1 : break;
  }
  return res;
}

rkt_fun void RKT_init_fds() {
  if (!RKT_glob.tmpfds[0]) {
#ifndef _WIN32
    if ((RKT_glob.saved[0] = dup(STDOUT_FILENO)) < 0) { RKT_fatal("dup sout"); }
    if ((RKT_glob.saved[1] = dup(STDERR_FILENO)) < 0) { RKT_fatal("dup serr"); }
    char temps[3][32]
        = {"/tmp/rkt_out_XXXXXX", "/tmp/rkt_err_XXXXXX", "/tmp/rkt_log_XXXXXX"};
    for (int i = 0; i < 3; ++i) {
      if ((RKT_glob.tmpfds[i] = mkstemp(temps[i])) < 0) {
        RKT_fatal("mkstemp");
      }
      unlink(temps[i]);
    }
#else
    HANDLE proc = GetCurrentProcess();
    if (!DuplicateHandle(proc, GetStdHandle(STD_OUTPUT_HANDLE), proc,
                         &RKT_glob.saved[0], 0, TRUE, DUPLICATE_SAME_ACCESS)) {
      RKT_fatal("dup sout");
    }
    if (!DuplicateHandle(proc, GetStdHandle(STD_ERROR_HANDLE), proc,
                         &RKT_glob.saved[1], 0, TRUE, DUPLICATE_SAME_ACCESS)) {
      RKT_fatal("dup serr");
    }
    char temp_path[MAX_PATH];
    if (!GetTempPathA(sizeof(temp_path), temp_path)) {
      RKT_fatal("GetTempPath");
    }
    for (size_t i = 0; i < 3; ++i) {
      char temp_name[MAX_PATH];
      if (!GetTempFileNameA(temp_path, "rkt", 0, temp_name)) {
        RKT_fatal("GetTempFileName");
      }
      SECURITY_ATTRIBUTES sa = {RKT_ZINIT};
      sa.nLength = sizeof(sa), sa.bInheritHandle = TRUE;
      RKT_glob.tmpfds[i] = CreateFileA(
          temp_name, GENERIC_READ | GENERIC_WRITE,
          FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, CREATE_ALWAYS,
          FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
      if (RKT_glob.tmpfds[i] == INVALID_HANDLE_VALUE) {
        RKT_fatal("CreateFile");
      }
    }
#endif
  }
}
rkt_fun void RKT_init_global(RK_CustomTestAttributes attrs) {
  RKT_init_fds();
  for (size_t i = 0; i < 3; ++i) {
    FILE* const default_files[3] = {stdout, NULL, NULL};
    if (RKT_glob.custom_paths[i]) {
      if (!strcmp(RKT_glob.custom_paths[i], "stdout")) {
        RKT_glob.output_types[i] = stdout;
      } else if (!strcmp(RKT_glob.custom_paths[i], "stderr")) {
        RKT_glob.output_types[i] = stderr;
      } else {
        if (strcmp(RKT_glob.custom_paths[i], "none")) { // todo
          RKT_glob.output_types[i] = fopen(RKT_glob.custom_paths[i], "w");
          if (!RKT_glob.output_types[i]) { perror("fopen"), exit(1); }
        }
      }
    } else {
      RKT_glob.output_types[i] = default_files[i];
    }
  }
  RKT_glob.attrs = attrs;
  {
#ifndef _WIN32
    struct sigaction sa = {RKT_ZINIT};
    setpgid(0, 0);
    sa.sa_handler = RKT_handle_sigterm, sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGINT);
    sigaddset(&sa.sa_mask, SIGQUIT);
    sigaddset(&sa.sa_mask, SIGTSTP);
    sigaction(SIGTERM, &sa, NULL);
#else
    RKT_glob.job = CreateJobObject(NULL, NULL);
    if (!AssignProcessToJobObject(RKT_glob.job, GetCurrentProcess())) {
      abort();
    }
#endif
  }
}

rkt_fun void RKT_run_all(const char** suites_strs, size_t nsuites_strs,
                         RK_CustomTestAttributes attrs) {
  RKT_init_global(attrs);
  RKT_print_total_beg_json();
#ifndef _WIN32
  if (RKT_glob.attrs.init) { RKT_glob.attrs.init(); }
#endif
  if (!nsuites_strs) {
    for (RKT_Suite* s = RKT_glob.suites; s; s = s->next) { RKT_run_suite(s); }
  } else {
    for (RKT_Suite* s = RKT_glob.suites; s; s = s->next) {
      for (size_t i = 0; i < nsuites_strs; ++i) {
        if (!strcmp(suites_strs[i], s->name.str)) {
          RKT_run_suite(s);
          break;
        }
      }
    }
  }
  if (RKT_glob.attrs.fini) { RKT_glob.attrs.fini(); }
  RKT_print_end_tap();
  RKT_print_total_end_json();
  for (size_t i = 0; i < 3; ++i) { // todo cleanup other resources
    FILE* out = RKT_glob.output_types[i];
    if (out && out != stdout && out != stderr) { fclose(out); }
  }
  // printf("--- Passed %zu, fails %zu, crashed %zu out of %zu tests ---\n",
  //        total - failed - crashed, failed, crashed, total);
}

rkt_fun void RKT_run_suite(RKT_Suite* s) {
  //  printf("RUNNING SUITE %s\n", s->name); // for debugging
  if (s->attrs.init) { s->attrs.init(); }
  size_t total = 0, failed = 0, crashed = 0, timed_out = 0, error = 0;
  for (RKT_TestEntry* e = s->tests; e; e = e->next) {
    if (!RKT_glob.attrs.tags) { goto run_test; } // todo local tags
    if (!e->attrs.tags) { continue; }
    const char *s1 = RKT_glob.attrs.tags, *e1;
    if (*s1 == '"') { ++s1; }
    for (s1 = RKT_glob.attrs.tags; *s1; s1 = *e1 ? e1 + 1 : e1) {
      for (e1 = s1; *e1 && *e1 != ',' && *e1 != '"'; ++e1);
      const char *s2 = e->attrs.tags, *e2;
      if (*s2 == '"') { ++s2; }
      for (; *s2; s2 = *e2 ? e2 + 1 : e2) {
        for (e2 = s2; *e2 && *e2 != ',' && *e2 != '"'; ++e2);
        if (e1 - s1 == e2 - s2 && !strncmp(s2, s1, e1 - s1)) { goto run_test; }
      }
    }
    continue;
  run_test:
    switch (RKT_run_test(e)) {
    case RKT_PASSED   : break;
    case RKT_FAILED   : ++failed; break;
    case RKT_CRASHED  : ++crashed; break;
    case RKT_EXITED   : ++crashed; break;
    case RKT_TIMEDOUT : ++timed_out; break;
    case RKT_TESTERROR: ++error; break;
    }
    ++total;
  }
  if (s->attrs.fini) { s->attrs.fini(); }
  RKT_print_suite_reg(s, total, failed, crashed, timed_out, error);
  RKT_print_suite_json(s, total, failed, crashed, timed_out, error);
  RKT_cleanup_suite_res(s);
}

rkt_fun RKT_TestEndType RKT_run_test(RKT_TestEntry* e) {
  // printf("RUNNING %s\n", e->name.str); // for debugging
  if (RKT_resolve_isolation(e) == false) {
    int timed_out = RKT_init_test_noisolation(e);
    RKT_parent_loop(e, timed_out);
  } else {
    RKT_init_test(e);
    RKT_parent_loop(e, 0);
#ifndef _WIN32
    close(RKT_glob.log);
#else
    CloseHandle(RKT_glob.log);
#endif
  }
  ++RKT_glob.tests_run;
  RKT_print_test_tap(e);
  return e->res.ended;
}
rkt_fun int RKT_read_hdr(RKT_AssertHdr* restrict hdr, long ms_left) {
  return RKT_read_full_tm(RKT_glob.log, hdr, sizeof(*hdr), ms_left);
}
rkt_fun int RKT_read_res(RKT_AssertRes* restrict res, long ms_left) {
  switch (RKT_read_full_tm(RKT_glob.log, res, sizeof(res->pkg), ms_left)) {
  case -3: return -3;
  case -2: return -2;
  case -1: return -1;
  case 0 : return 0;
  case 1:
    memset(res->args, 0, sizeof(res->args));
    if (!res->len) { break; }
    res->args[0] = RKT_malloc(char, res->len);
    if (!res->args[0]) { RKT_fatal("malloc read res"); }
    if (RKT_read_full_tm(RKT_glob.log, res->args[0], res->len, ms_left) < 1) {
      return -2;
    }
    size_t i = 1;
    for (size_t len = 0; len + 1 < res->len; ++len) {
      if (!res->args[0][len]) {
        assert(i + 1 < RKT_COUNTOF(res->args));
        res->args[i++] = res->args[0] + len + 1;
      }
    }
    res->args[i] = res->args[0] + res->len;
  }
  return 1;
}
#ifndef _WIN32
# define RKT_HASCHILD()  (!!RKT_glob.pid)
# define RKT_KILLCHILD() kill(RKT_glob.pid, SIGKILL)
#else
# define RKT_HASCHILD()  (!!RKT_glob.pid.hProcess)
# define RKT_KILLCHILD() TerminateProcess(RKT_glob.pid.hProcess, 1)
#endif

rkt_fun RKT_TestEndType RKT_finalise_test(RKT_TestResult* r,
                                          int             exit_condition) {
  r->ended = RKT_PASSED;
  if (exit_condition == 2) {
    r->exit_status.type = RKT_EXIT_DISCARD; // timeout, exit status irrelevant
    if (RKT_HASCHILD()) { RKT_KILLCHILD(), (void)RKT_reapchild(); }
  } else {
    if (RKT_HASCHILD()) {
      r->exit_status = RKT_reapchild();
    } else {
      r->exit_status.type = RKT_EXIT_DISCARD; // todo perhaps assertion failed
    }
  }

  if (exit_condition == 2) {
    memset(&r->results[r->count - 1].res, 0, sizeof(RKT_AssertRes));
    r->results[r->count - 1].res.res = RKT_ASSERTRES_TIMEOUT;
    r->ended                         = RKT_TIMEDOUT;
  } else if (exit_condition == 1) {
    const RKT_AssertHdr* hdr = &r->results[r->count - 1].hdr;
    RKT_AssertRes*       res = &r->results[r->count - 1].res;
    memset(res, 0, sizeof(*res));
    if (r->exit_status.type == RKT_EXIT_CRASHED) {
      if (hdr->F != RKTF_crash) {
        res->res = RKT_ASSERTRES_CRASH;
        r->ended = RKT_CRASHED;
      } else {
        res->res = (hdr->reason != RKT_SIGNAL_ANY
                    && hdr->reason != r->exit_status.reason);
      }
    } else {
      if (hdr->F != RKTF_exit) {
        res->res = RKT_ASSERTRES_EXIT;
        r->ended = RKT_EXITED;
      } else {
        res->res = (hdr->exit_code != -1
                    && hdr->exit_code != r->exit_status.exit_code);
      }
    }
    if (res->res) { ++r->fails; }
  } else if (r->exit_status.type == RKT_EXIT_CRASHED
             || r->exit_status.exit_code) {
    r->ended = RKT_TESTERROR;
  }
  if (r->ended == RKT_PASSED && r->fails) { r->ended = RKT_FAILED; }
  r->capt[RK_OUTPUTSTREAMS_STDOUT] = RKT_readfile(RKT_glob.tmpfds[0]);
  r->capt[RK_OUTPUTSTREAMS_STDERR] = RKT_readfile(RKT_glob.tmpfds[1]);
  return r->ended;
}
rkt_fun RKT_TestEndType RKT_parent_loop(RKT_TestEntry* restrict e,
                                        bool timed_out) {
  size_t          rescap = 512;
  RKT_TestResult* r      = &e->res;
  r->results             = RKT_malloc(RKT_AssertDat, rescap);
  if (!r->results) { RKT_fatal("malloc buf"); }
  long    timeout_ms = !timed_out ? RKT_resolve_timeout(e) : -1;
  int64_t deadline   = (timeout_ms == -1) ? -1 : RKT_now_ms() + timeout_ms;
  for (bool reading_res = 0;; reading_res = !reading_res) {
    int64_t remaining = -1;
    if (deadline != -1) {
      remaining = deadline - RKT_now_ms();
      if (remaining <= 0) { remaining = 0; }
    }
    if (!reading_res) {
      if (r->count + 2 == rescap) { // +2 to simplify timeout code
        rescap     *= 2;
        r->results  = RKT_realloc(RKT_AssertDat, r->results, rescap);
        if (!r->results) { RKT_fatal("realloc 0"); }
      } // todo partial read timeout
      switch (RKT_read_hdr(&r->results[r->count].hdr, remaining)) {
      case -3:
      case -2:
        return memset(&r->results[r->count++].hdr, 0, sizeof(RKT_AssertHdr)),
               RKT_finalise_test(r, 2);
      case -1: RKT_fatal("read hdr");
      case 0 : return RKT_finalise_test(r, timed_out ? 2 : 0);
      case 1 : ++r->count;
      }
    } else {
      switch (RKT_read_res(&r->results[r->count - 1].res, remaining)) {
      case -3:
      case -2: return RKT_finalise_test(r, 2);
      case -1: RKT_fatal("read res");
      case 0 : return RKT_finalise_test(r, timed_out ? 2 : 1);
      case 1 : r->fails += r->results[r->count - 1].res.res > 0;
      }
    }
  }
}

/// PLATFORM DIFFERENCES
#ifndef _WIN32
# define RKT_CONSTRUCTOR(fn)      __attribute__((constructor)) static void fn(void)
# define RKT_Windows_Childentry() ((void)0)

rkt_fun void RKT_handle_sigalarm(int signum) {
  siglongjmp(RKT_glob.fret_sig, signum);
}
#else
# define RKT_CONSTRUCTOR(fn)                                                   \
   static void fn(void);                                                       \
   __declspec(allocate(".CRT$XCU")) static void (*RKT_CONCAT(fn, _ptr))(void)  \
       = fn;                                                                   \
   static void fn(void)

rkt_fun void RKT_Windows_Childentry(void) {
  char buf[256];
  if (!GetEnvironmentVariableA("RK_CHILD_FN", buf, sizeof(buf))) { return; }
  for (RKT_Suite* s = RKT_glob.suites; s; s = s->next) {
    for (RKT_TestEntry* e = s->tests; e; e = e->next) {
      if (strcmp(buf, e->name.str)) { continue; }
      {
        if (!GetEnvironmentVariableA("RK_CHILD_OUT=", buf, sizeof(buf))) {
          exit(1);
        }
        char* ep;
        RKT_glob.log = (rkt_fd)(uintptr_t)strtoumax(buf, &ep, 10);
        if (*ep != '\0') { exit(1); }
      }
      for (int i = 0; i < 2; ++i) {
        const char* names[2] = {"RK_CHILD_ERR=", "RK_CHILD_META"};
        if (!GetEnvironmentVariableA(names[i], buf, sizeof(buf))) { exit(1); }
        char* ep;
        RKT_glob.tmpfds[i] = (rkt_fd)(uintptr_t)strtoumax(buf, &ep, 10);
        if (*ep != '\0') { exit(1); }
      }

      if (RKT_glob.attrs.init) { RKT_glob.attrs.init(); }
      if (s->attrs.init) { s->attrs.init(); }
      if (e->attrs.init) { e->attrs.init(); }
      int val = setjmp(RKT_glob.fret);
      if (val == 0) {
        e->func();
      } else {
        /*todo jump*/
      }
      fflush(NULL);
      if (e->attrs.fini) { e->attrs.fini(); }
      if (s->attrs.fini) { s->attrs.fini(); }
      if (RKT_glob.attrs.fini) { RKT_glob.attrs.fini(); }
      exit(0);
    }
  }
  exit(1); /*function not found?*/
}
#endif

rkt_fun void RKT_redirect_streams(rkt_fd new_logfd) {
  RKT_rewindfile(RKT_glob.tmpfds[0]);
  RKT_rewindfile(RKT_glob.tmpfds[1]);
  RKT_rewindfile(RKT_glob.tmpfds[2]);
#ifndef _WIN32
  if (ftruncate(RKT_glob.tmpfds[0], 0) < 0) { RKT_fatal("ftruncate"); }
  if (ftruncate(RKT_glob.tmpfds[1], 0) < 0) { RKT_fatal("ftruncate"); }
  if (ftruncate(RKT_glob.tmpfds[2], 0) < 0) { RKT_fatal("ftruncate"); }
  if (dup2(RKT_glob.tmpfds[0], STDOUT_FILENO) < 0) { RKT_fatal("dup2 red1"); }
  if (dup2(RKT_glob.tmpfds[1], STDERR_FILENO) < 0) { RKT_fatal("dup2 red2"); }
#else
  if (!SetEndOfFile(RKT_glob.tmpfds[0])) { RKT_fatal("SetEndOfFile"); }
  if (!SetEndOfFile(RKT_glob.tmpfds[1])) { RKT_fatal("SetEndOfFile"); }
  if (!SetEndOfFile(RKT_glob.tmpfds[2])) { RKT_fatal("SetEndOfFile"); }
  if (!SetStdHandle(STD_OUTPUT_HANDLE, RKT_glob.tmpfds[0])) {
    RKT_fatal("SetStdHandle STDOUT");
  }
  if (!SetStdHandle(STD_ERROR_HANDLE, RKT_glob.tmpfds[1])) {
    RKT_fatal("SetStdHandle STDERR");
  }
#endif
  RKT_glob.log = new_logfd;
}

rkt_fun void RKT_restore_streams() {
  RKT_rewindfile(RKT_glob.tmpfds[0]);
  RKT_rewindfile(RKT_glob.tmpfds[1]);
  RKT_rewindfile(RKT_glob.tmpfds[2]);
#ifndef _WIN32
  if (dup2(RKT_glob.saved[0], STDOUT_FILENO) < 0) { RKT_fatal("dup2 res out"); }
  if (dup2(RKT_glob.saved[1], STDERR_FILENO) < 0) { RKT_fatal("dup2 res err"); }
#else
  if (!SetStdHandle(STD_OUTPUT_HANDLE, RKT_glob.saved[0])) {
    RKT_fatal("SetStdHandle STDOUT");
  }
  if (!SetStdHandle(STD_ERROR_HANDLE, RKT_glob.saved[1])) {
    RKT_fatal("SetStdHandle STDERR");
  }
#endif
}

rkt_fun void RKT_init_test(const RKT_TestEntry* restrict e) {
#ifndef _WIN32
  fflush(NULL);
  int pp[2];
  if (pipe(pp) < 0) { RKT_fatal("pipe"); }
  switch ((RKT_glob.pid = fork())) {
  case -1: RKT_fatal("fork");
  case 0:
    close(pp[0]), RKT_redirect_streams(pp[1]);
    if (e->attrs.init) { e->attrs.init(); }
    if (!setjmp(RKT_glob.fret)) {
      e->func();
    } else {
    }
    if (e->attrs.fini) { e->attrs.fini(); }
    fflush(NULL), _exit(0);
  default: close(pp[1]); RKT_glob.log = pp[0];
  }

#else
  SECURITY_ATTRIBUTES sa = {sizeof(sa), NULL, TRUE};
  rkt_fd              w_end;
  if (!CreatePipe(&RKT_glob.log, &w_end, &sa, 0)
      || !SetHandleInformation(RKT_glob.log, HANDLE_FLAG_INHERIT, 0)) {
    exit(1);
  }
  char path[MAX_PATH];
  if (!GetModuleFileNameA(NULL, path, MAX_PATH)) { exit(1); }
  LPCH   penv = GetEnvironmentStringsA();
  size_t plen = 0;
  if (penv) {
    LPCH p = penv;
    while (*p) { p += strlen(p) + 1; }
    plen = p - penv; // double null not included here
  }
  size_t extra = RKT_lenof("RK_CHILD_FN=") + e->name.len + 1
               + RKT_lenof("RK_CHILD_META=") + 32 + 1
               + RKT_lenof("RK_CHILD_OUT=") + 32 + 1
               + RKT_lenof("RK_CHILD_ERR=") + 32 + 1 + 1;
  char *nenv = RKT_malloc(char, extra + plen), *s = nenv;
  if (!nenv) { exit(1); }
  s    += RK_catlit(s, "RK_CHILD_FN=");
  s    += RK_catstr(s, e->name.str, e->name.len);
  *s++  = '\0';
  s    += sprintf(s, "RK_CHILD_META=%" PRIuPTR, (uintptr_t)w_end) + 1;
  // todo
  s += sprintf(s, "RK_CHILD_OUT=%" PRIuPTR,
               (uintptr_t)RKT_glob.tmpfds[RK_OUTPUTSTREAMS_STDERR])
     + 1;
  s += sprintf(s, "RK_CHILD_ERR=%" PRIuPTR,
               (uintptr_t)RKT_glob.tmpfds[RK_OUTPUTSTREAMS_STDOUT])
     + 1;

  if (plen) { s += (memcpy(s, penv, plen), plen); }
  *s = '\0'; /* ensure double-null terminator */
  STARTUPINFO si;
  ZeroMemory(&si, sizeof(si));
  si.cb         = sizeof(si);
  si.dwFlags    = STARTF_USESTDHANDLES;
  si.hStdOutput = RKT_glob.tmpfds[RK_OUTPUTSTREAMS_STDOUT];
  si.hStdError  = RKT_glob.tmpfds[RK_OUTPUTSTREAMS_STDERR];
  BOOL worked = CreateProcessA(path, path, NULL, NULL, TRUE, 0, nenv, NULL, &si,
                               &RKT_glob.pid);
  if (penv) { FreeEnvironmentStringsA(penv); }
  free(nenv);
  CloseHandle(w_end);
  if (!worked) { exit(1); }
#endif
}

#ifdef _WIN32
rkt_fun DWORD RKT_runfunc(void* arg) {
  if (!setjmp(RKT_glob.fret)) { // for failed assertions (no signals)
    ((const RKT_TestEntry*)arg)->func();
  } else {
    return 1;
  }
  return 0;
}
#endif

rkt_fun int RKT_init_test_noisolation(const RKT_TestEntry* restrict e) {
#ifndef _WIN32
  RKT_redirect_streams(RKT_glob.tmpfds[2]);
  if (e->attrs.init) { e->attrs.init(); }
  int  timed_out = 0;
  long ts        = RKT_resolve_timeout(e);
  if (ts != -1) {
    timed_out = sigsetjmp(RKT_glob.fret_sig, 1);
    if (!timed_out) {
      struct sigaction sa = {RKT_ZINIT};
      sa.sa_handler       = RKT_handle_sigalarm, sigemptyset(&sa.sa_mask);
      sigaction(SIGALRM, &sa, NULL);
      struct itimerval timer
          = {{0, 0},
             {(suseconds_t)(ts / 1000), (suseconds_t)((ts % 1000) * 1000)}};
      setitimer(ITIMER_REAL, &timer, NULL);
    }
  }
  if (!timed_out) {
    if (!setjmp(RKT_glob.fret)) {
      e->func();
    } else {
      // failed assertion
    }
  }
  struct itimerval timer = {RKT_ZINIT};
  setitimer(ITIMER_REAL, &timer, NULL);
  struct sigaction sa = {RKT_ZINIT};
  sa.sa_handler       = SIG_DFL, sigemptyset(&sa.sa_mask);
  sigaction(SIGALRM, &sa, NULL);
  if (e->attrs.fini) { e->attrs.fini(); }
  fflush(NULL), RKT_restore_streams();
  return timed_out;
#else
  RKT_redirect_streams(RKT_glob.tmpfds[2]);
  if (e->attrs.init) { e->attrs.init(); }
  long  ts        = RKT_resolve_timeout(e);
  int   timed_out = 0;
  DWORD exit_code;
  if (ts != -1) {
    HANDLE t = CreateThread(NULL, 0, RKT_runfunc, (void*)e, 0, NULL);
    if (!t) { RKT_fatal("CreateThread"); }
    timed_out = WaitForSingleObject(t, (DWORD)ts) == WAIT_TIMEOUT;
    if (timed_out) {
      TerminateThread(t, 1);
    } else {
      GetExitCodeThread(t, &exit_code);
    }
    CloseHandle(t);
  } else {
    exit_code = RKT_runfunc((void*)e);
  }
  if (e->attrs.fini) { e->attrs.fini(); }
  fflush(NULL), RKT_restore_streams();
  return timed_out;
#endif
}

// to debug the testing framework - compare test result function to expected
#define RKT_VALIDATE(FUNNAME, RESULT)                                          \
  RK_REGISTER_TEST("testmeta", test_##FUNNAME) {                               \
    RKT_TestEntry* e = &RKT_CONCAT(RKT_entry_, FUNNAME);                       \
    if (e->suite->attrs.init) { e->suite->attrs.init(); }                      \
    if (!e->res.count) {                                                       \
      rk_assert_eq(RKT_run_test(e), RESULT);                                   \
      RKT_cleanup_test_res(e);                                                 \
    } else {                                                                   \
      rk_assert_eq(e->res.ended, RESULT);                                      \
    }                                                                          \
    if (e->suite->attrs.fini) { e->suite->attrs.fini(); }                      \
  }

rkt_fun RKT_Suite* RKT_find_suite(const char* suitename) {
  for (RKT_Suite* s = RKT_glob.suites; s; s = s->next) {
    if (!strcmp(s->name.str, suitename)) { return s; }
  }
  return NULL;
}
rkt_fun RKT_TestEntry* RKT_add_test_to_suite(RKT_Suite*     s,
                                             RKT_TestEntry* entry) {
  RKT_TestEntry* e;
  entry->suite = s;
  if (!s->tests) { return s->tests = entry; }
  for (e = s->tests; e->next; e = e->next);
  return e->next = entry;
}

rkt_fun RKT_Suite* RKT_add_suite(RKT_Suite* suite) {
  RKT_Suite* s;
  if (!RKT_glob.suites) { return RKT_glob.suites = suite; }
  for (s = RKT_glob.suites; s->next; s = s->next);
  return s->next = suite;
}

#define RKT_REGISTER_SUITE_IMPL(SUITENAME, ...)                                \
  RKT_CONSTRUCTOR(RKT_CONCAT(RK_test_suite_register_, __COUNTER__)) {          \
    RK_CustomTestAttributes attrs = {__VA_ARGS__};                             \
    RKT_Suite*              s     = RKT_find_suite(SUITENAME);                 \
    if (!s) {                                                                  \
      static RKT_Suite suite                                                   \
          = {NULL, NULL, {SUITENAME, RKT_lenof(SUITENAME)}};                   \
      s = RKT_add_suite(&suite);                                               \
    }                                                                          \
    s->attrs = attrs;                                                          \
  }
/*
  const char*      tags;                /// Tags todo docs
  RK_FixtureFunc   init;                /// Setup fixture
  RK_FixtureFunc   fini;                /// Teardown fixture
  RK_Verbosity     verbosity_levels[3]; /// verbosity level, per RK_OutPutStream
  RK_IsolationMode isolation;
  long             timeout_ms;
*/
#define RKT_REGISTER_TEST_IMPL(SUITENAME, fn, ...)                             \
  static void          fn();                                                   \
  static RKT_TestEntry RKT_CONCAT(RKT_entry_, fn)                              \
      = {NULL,                                                                 \
         NULL,                                                                 \
         {#fn, RKT_lenof(#fn)},                                                \
         {__FILE__, RKT_lenof(__FILE__)},                                      \
         fn,                                                                   \
         {__VA_ARGS__},                                                        \
         {RKT_ZINIT}};                                                         \
  RKT_CONSTRUCTOR(RK_test_register_##fn) {                                     \
    RKT_Suite* s = RKT_find_suite(SUITENAME);                                  \
    if (!s) {                                                                  \
      static RKT_Suite suite                                                   \
          = {NULL, NULL, {SUITENAME, RKT_lenof(SUITENAME)}};                   \
      s = RKT_add_suite(&suite);                                               \
    }                                                                          \
    RKT_add_test_to_suite(s, &(RKT_CONCAT(RKT_entry_, fn)));                   \
  }                                                                            \
  static void fn()

#define RKT_RUN_TESTS_IMPL(suites, nsuites, ...)                               \
  do {                                                                         \
    RKT_Windows_Childentry();                                                  \
    RK_CustomTestAttributes attrs = {__VA_ARGS__};                             \
    RKT_run_all(suites, nsuites, attrs);                                       \
  } while (0)

#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif

#undef RK_catlit
#undef RKT_GEN_fargs_nofloat
#undef RKT_GEN_fargs_float
#undef RK_INRED
#undef RK_INGREEN
#undef RK_INBLUE
#undef RK_INYELLOW
/*
  const char*      tags;                /// Tags todo docs
  RK_FixtureFunc   init;                /// Setup fixture
  RK_FixtureFunc   fini;                /// Teardown fixture
  RK_Verbosity     verbosity_levels[3]; /// verbosity level, per RK_OutPutStream
  RK_IsolationMode isolation;
  long             timeout_ms; /// Timeout in ms (default is no timeout)
*/
// #define RKT_prefix(str, lit) (!strncmp(str, lit, RKT_lenof(lit)))
// rkt_fun int main_default(int argc, char* argv[]) {
//   RK_CustomTestAttributes attrs;
//   char**                  suites; // todo
//   size_t                  nsuites = 0;
//   for (char **args = argv + 1, *arg = *args; arg != NULL; arg = *++args) {
//     if (*arg == '-') {
//       ++arg;
//       if (*arg == '-') {
//         ++arg;
//         if (RKT_prefix(arg, "tags=")) {
//           arg        += RKT_lenof("tags=");
//           attrs.tags  = arg + 1;
//         } else if (RKT_prefix(arg, "verbosity_levels")) {
//           arg += RKT_lenof("verbosity_levels");
//           if (*arg == '=') { // todo
//             ++arg;
//             //
//             /*
//               RK_OUTPUTSTREAMS_STDOUT, // captured stdout of the child
//               process RK_OUTPUTSTREAMS_STDERR, // captured stderr of the
//               child process RK_OUTPUTSTREAMS_LOG,    // assertion information
//             */
//           } else if (RKT_prefix(arg, "[RK_OUTPUTSTREAMS_")) {
//             arg += RKT_lenof("[RK_OUTPUTSTREAMS_");
//             RK_Verbosity* v;
//             if (RKT_prefix(arg, "STDOUT]=")) {
//               arg += RKT_lenof("STDOUT]=");
//               v    = &attrs.verbosity_levels[RK_OUTPUTSTREAMS_STDOUT];
//             } else if (RKT_prefix(arg, "STDERR]=")) {
//               arg += RKT_lenof("STDERR]=");
//               v    = &attrs.verbosity_levels[RK_OUTPUTSTREAMS_STDERR];
//             } else if (RKT_prefix(arg, "LOG]=")) {
//               arg += RKT_lenof("LOG]=");
//               v    = &attrs.verbosity_levels[RK_OUTPUTSTREAMS_LOG];
//             } else {
//               // invalid
//             }
//             if (!strcmp(arg, "RK_VERBOSITY_")) {
//               arg += RKT_lenof("RK_VERBOSITY_");
//               if (!strcmp(arg, "INHERIT")) {
//                 *v = RK_VERBOSITY_INHERIT;
//               } else if (!strcmp(arg, "AUTO")) {
//                 *v = RK_VERBOSITY_AUTO;
//               } else if (!strcmp(arg, "ALWAYS")) {
//                 *v = RK_VERBOSITY_ALWAYS;
//               } else if (!strcmp(arg, "NEVER")) {
//                 *v = RK_VERBOSITY_NEVER;
//               }
//             } else {
//               // invalid
//             }
//           } else {
//             // invalid
//           }
//         } else if (RKT_prefix(arg, "isolation=")) {
//           arg += RKT_lenof("isolation=");
//           if (RKT_prefix(arg, "RK_ISOLATION_")) {
//             arg += RKT_lenof("RK_ISOLATION_");
//             if (!strcmp(arg, "INHERIT")) {
//               attrs.isolation = RK_ISOLATION_INHERIT;
//             } else if (!strcmp(arg, "ON")) {
//               attrs.isolation = RK_ISOLATION_ON;
//             } else if (!strcmp(arg, "OFF")) {
//               attrs.isolation = RK_ISOLATION_OFF;
//             } else {
//               // invalid
//             }
//           } else {
//             // invalid
//           }
//         } else if (RKT_prefix(arg, "timeout_ms=")) {
//           arg += RKT_lenof("timeout_ms=");
//           char* endptr;
//           errno            = 0;
//           attrs.timeout_ms = strtol(arg, &endptr, 10);
//           if (errno == ERANGE) {
//             printf("Overflow or underflow occurred\n"), exit(1);
//           } else if (*endptr != '\0') {
//             printf("Invalid characters after number: %s\n", endptr), exit(1);
//           }
//         } else if (RKT_prefix(arg, "suites=\"")) {
//           arg               += RKT_lenof("suites=");
//           size_t suites_cap  = 100;
//           suites             = RKT_malloc(char*, suites_cap);
//           // todo suite parsing
//         }
//       }
//     }
//   }
//   return 0;
// }
/*
  RK_VERBOSITY_INHERIT,
  RK_VERBOSITY_AUTO,   ///< Output depending on type of output/result
  RK_VERBOSITY_ALWAYS, ///< Print everything
  RK_VERBOSITY_NEVER,
  RK_ISOLATION_INHERIT,
  RK_ISOLATION_ON,
  RK_ISOLATION_OFF
*/
#endif
