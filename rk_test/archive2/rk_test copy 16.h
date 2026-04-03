/// @file rk_test.h
/// @version 0.1
/// @brief Single-Header Test Framework for C
///
/// Criterion-Like Header-Only Test Framework.
/// Runs each test in its own process with customisable verbosity levels,
/// stdout/stderr and meta information file redirection.
///
/// Todo:
///     - Stdout/erreq functions
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
# include <inttypes.h>
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
# include <sstream> // needed for std::ostringstream
# include <type_traits>
#else
# include <math.h>
#endif
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum RK_OutPutStreams {
  RK_OUTPUTSTREAMS_STDOUT, // captured stdout of the child process
  RK_OUTPUTSTREAMS_STDERR, // captured stderr of the child process
  RK_OUTPUTSTREAMS_LOG,    // assertion information
} RK_OutPutStreams;

typedef enum RK_OutputFormats {
  RK_OUTPUTFORMAT_NORMAL,
  RK_OUTPUTFORMAT_JSON,
  RK_OUTPUTFORMAT_TAP,
} RK_OutputFormats;

typedef enum RK_Verbosity {
  RK_VERBOSITY_GLOB_DEFAULT,
  RK_VERBOSITY_DEFAULT,
  RK_VERBOSITY_ALWAYS,
  RK_VERBOSITY_NEVER,
} RK_Verbosity;

typedef enum RK_IsolationMode {
  RKT_ISOLATION_DEFAULT,
  RKT_ISOLATION_ON,
  RKT_ISOLATION_OFF
} RK_IsolationMode;

typedef void (*RK_FixtureFunc)(void);

/// @brief customisable attributes (global, suite or test-level)
typedef struct RKT_CustomAttrs {
  const char*      tags;                /// Tags todo docs
  RK_FixtureFunc   init;                /// Setup fixture
  RK_FixtureFunc   fini;                /// Teardown fixture
  RK_Verbosity     verbosity_levels[3]; /// verbosity level
  /// per output stream
  RK_IsolationMode isolation;
  long             timeout_ms; /// Timeout in ms (default is no timeout)
} RKT_CustomAttrs;

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
#define rk_expect_true(EXPR_)                 RK_EXPECT(RKTf_true, RKTF_TRUE, "rk_expect_true("#EXPR_")", EXPR_)
#define rk_expect_false(EXPR_)                RK_EXPECT(RKTf_false, RKTF_FALSE, "rk_expect_false("#EXPR_")", EXPR_)
#define rk_expect_null(PTR_)                  RK_EXPECT(RKTf_null, RKTF_NULL, "rk_expect_null("#PTR_")", PTR_)
#define rk_expect_nonnull(PTR_)               RK_EXPECT(RKTf_nnull, RKTF_NNULL, "rk_expect_nonnull("#PTR_")", PTR_)
#define rk_expect_memeq(ptr1, ptr2, siz)      RK_EXPECT(RKTf_memeq, RKTF_MEMEQ, "rk_expect_memeq(" #ptr1 ", " #ptr2 ", " #siz ")", ptr1, ptr2, siz)
#define rk_expect_memneq(ptr1, ptr2, siz)     RK_EXPECT(RKTf_memneq, RKTF_MEMNEQ, "rk_expect_memneq(" #ptr1 ", " #ptr2 ", " #siz ")", ptr1, ptr2, siz)
#define rk_expect_memzero(ptr, siz)           RK_EXPECT(RKTf_memzero, RKTF_MEMZERO, "rk_expect_memzero(" #ptr ",  " #siz ")", ptr, siz)
#define rk_expect_memnzero(ptr, siz)          RK_EXPECT(RKTf_memnzero, RKTF_MEMNZERO, "rk_expect_memnzero(" #ptr ",  " #siz ")", ptr, siz)

#define rk_expect_eq(exp, act)                RK_EXPECT(RKT_SELFUN(eq, exp), RKTF_EQ, "rk_expect_eq("#exp", " #act")", exp, act)
#define rk_expect_neq(exp, act)               RK_EXPECT(RKT_SELFUN(neq, exp), RKTF_NEQ, "rk_expect_neq("#exp", " #act")", exp, act)
#define rk_expect_lt(exp, act)                RK_EXPECT(RKT_SELFUN(eq, exp), RKTF_LT, "rk_expect_lt("#exp", " #act")", exp, act)
#define rk_expect_leq(exp, act)               RK_EXPECT(RKT_SELFUN(neq, exp), RKTF_LEQ, "rk_expect_leq("#exp", " #act")", exp, act)
#define rk_expect_gt(exp, act)                RK_EXPECT(RKT_SELFUN(eq, exp), RKTF_GT, "rk_expect_gt("#exp", " #act")", exp, act)
#define rk_expect_geq(exp, act)               RK_EXPECT(RKT_SELFUN(neq, exp), RKTF_GEQ, "rk_expect_geq("#exp", " #act")", exp, act)
#define rk_expect_inrange(val, low, high)     RK_EXPECT(RKT_SELFUN(inrange, val), RKTF_INRANGE, "rk_expect_inrange(" #val ", " #low", " #high ")", val, low, high)
#define rk_expect_floateq_tol(exp, act, tol)  RK_EXPECT(RKT_SELFUN_TOL(toleq, exp), RKTF_TOLEQ, "rk_expect_floateq_tol("#exp", " #act ", " #tol ")", exp, act, tol)
#define rk_expect_floatneq_tol(exp, act, tol) RK_EXPECT(RKT_SELFUN_TOL(tolneq, exp), RKTF_TOLNEQ,  "rk_expect_floatneq_tol("#exp", " #act ", " #tol ")", exp, act, tol)
#define rk_expect_streq(str1, str2)           RK_EXPECT(RKTf_streq, RKTF_STREQ, "streq(" #str1 ", " #str2 ")",str1,str2)
#define rk_expect_strneq(str1, str2)          RK_EXPECT(RKTf_strneq, RKTF_STRNEQ,"strneq(" #str1 ", " #str2 ")",str1,str2)                                    
#define rk_expect_streq_n(str1, str2, len)    RK_EXPECT(RKTf_streq_n, RKTF_STREQ, "streq(" #str1 ", " #str2 ", " #len")",str1,str2,len)
#define rk_expect_strneq_n(str1, str2,len)    RK_EXPECT(RKTf_strneq_n, RKTF_STRNEQ,"strneq(" #str1 ", " #str2 ", " #len")",str1,str2,len)  

// todo not implemented 
//#define rk_expect_stdouteq(errstr, ...)       RKT_SEND_HDR("stderror prints " #errstr, RK_TEST_STDOUTEQ, RK_TEST_T_vp)
//#define rk_expect_stdoutneq(errstr, ...)      RKT_SEND_HDR("stderror prints " #errstr, RK_TEST_STDOUTNEQ, RK_TEST_T_vp)
//#define rk_expect_stderreq(errstr, ...)       RKT_SEND_HDR("stderror prints " #errstr, RK_TEST_STDERREQ, RK_TEST_T_vp)
//#define rk_expect_stderrneq(errstr, ...)      RKT_SEND_HDR("stderror prints " #errstr, RK_TEST_STDERRNEQ, RK_TEST_T_vp)

#define rk_assert_true(expr)                  RK_ASSERT(RKTf_true, RKTF_TRUE, "rk_assert_true("#expr")", expr)
#define rk_assert_false(expr)                 RK_ASSERT(RKTf_false, RKTF_FALSE, "rk_assert_false("#expr")", expr)
#define rk_assert_null(ptr)                   RK_ASSERT(RKTf_null, RKTF_NULL, "rk_assert_null("#ptr")", ptr)
#define rk_assert_nonnull(ptr)                RK_ASSERT(RKTf_nnull, RKTF_NNULL, "rk_assert_nonnull("#ptr")", ptr)
#define rk_assert_memeq(ptr1, ptr2, siz)      RK_ASSERT(RKTf_memeq, RKTF_MEMEQ, "rk_assert_memeq(" #ptr1 ", " #ptr2 ", " #siz ")", ptr1, ptr2, siz)
#define rk_assert_memneq(ptr1, ptr2, siz)     RK_ASSERT(RKTf_memneq, RKTF_MEMNEQ, "rk_assert_memneq(" #ptr1 ", " #ptr2 ", " #siz ")", ptr1, ptr2, siz)
#define rk_assert_memzero(ptr, siz)           RK_ASSERT(RKTf_memzero, RKTF_MEMZERO, "rk_assert_memzero(" #ptr ",  " #siz ")", ptr, siz)
#define rk_assert_memnzero(ptr, siz)          RK_ASSERT(RKTf_memnzero, RKTF_MEMNZERO, "rk_assert_memnzero(" #ptr ",  " #siz ")", ptr, siz)

#define rk_assert_eq(exp, act)                RK_ASSERT(RKT_SELFUN(eq, exp), RKTF_EQ, "rk_assert_eq("#exp", " #act")", exp, act)
#define rk_assert_neq(exp, act)               RK_ASSERT(RKT_SELFUN(neq, exp), RKTF_NEQ, "rk_assert_neq("#exp", " #act")", exp, act)
#define rk_assert_lt(exp, act)                RK_ASSERT(RKT_SELFUN(eq, exp), RKTF_LT, "rk_assert_lt("#exp", " #act")", exp, act)
#define rk_assert_leq(exp, act)               RK_ASSERT(RKT_SELFUN(neq, exp), RKTF_LEQ, "rk_assert_leq("#exp", " #act")", exp, act)
#define rk_assert_gt(exp, act)                RK_ASSERT(RKT_SELFUN(eq, exp), RKTF_GT, "rk_assert_gt("#exp", " #act")", exp, act)
#define rk_assert_geq(exp, act)               RK_ASSERT(RKT_SELFUN(neq, exp), RKTF_GEQ, "rk_assert_geq("#exp", " #act")", exp, act)
#define rk_assert_inrange(val, low, high)     RK_ASSERT(RKT_SELFUN(inrange, val), RKTF_INRANGE, "rk_assert_inrange(" #val ", " #low", " #high ")", val, low, high)
#define rk_assert_floateq_tol(exp, act, tol)  RK_ASSERT(RKT_SELFUN_TOL(toleq, exp), RKTF_TOLEQ, "rk_assert_floateq_tol("#exp", " #act ", " #tol ")", exp, act, tol)
#define rk_assert_floatneq_tol(exp, act, tol) RK_ASSERT(RKT_SELFUN_TOL(tolneq, exp), RKTF_TOLNEQ,  "rk_assert_floatneq_tol("#exp", " #act ", " #tol ")", exp, act, tol)
#define rk_assert_streq(str1, str2)           RK_ASSERT(RKTf_streq, RKTF_STREQ, "streq(" #str1 ", " #str2 ")",str1,str2)
#define rk_assert_strneq(str1, str2)          RK_ASSERT(RKTf_strneq, RKTF_STRNEQ,"strneq(" #str1 ", " #str2 ")",str1,str2)  
#define rk_assert_streq_n(str1, str2, len)    RK_ASSERT(RKTf_streq_n, RKTF_STREQ, "streq(" #str1 ", " #str2 ", " #len")",str1,str2,len)
#define rk_assert_strneq_n(str1, str2,len)    RK_ASSERT(RKTf_strneq_n, RKTF_STRNEQ,"strneq(" #str1 ", " #str2 ", " #len")",str1,str2,len)  

// todo not implemented
//#define rk_assert_stdouteq(errstr, ...)  RKT_SEND_HDR("stderror prints " #errstr, RK_TEST_STDOUTEQ, RK_TEST_T_vp)
//#define rk_assert_stdoutneq(errstr, ...) RKT_SEND_HDR("stderror prints " #errstr, RK_TEST_STDOUTNEQ, RK_TEST_T_vp)
//#define rk_assert_stderreq(errstr, ...)  RKT_SEND_HDR("stderror prints " #errstr, RK_TEST_STDERREQ, RK_TEST_T_vp)
//#define rk_assert_stderrneq(errstr, ...) RKT_SEND_HDR("stderror prints " #errstr, RK_TEST_STDERRNEQ, RK_TEST_T_vp)

// clang-format on
#define rk_assert_crash(signal, ...)                                           \
  do {                                                                         \
    RKT_SEND_HDR("assert_crash(" #signal ", " #__VA_ARGS__ ")", RKTF_CRASH,    \
                 signal);                                                      \
    __VA_ARGS__;                                                               \
    {                                                                          \
      RKT_AssertResPkg RK_PKG = {1, 0};                                        \
      RKT_send_res(&RK_PKG);                                                   \
    }                                                                          \
    exit(0);                                                                   \
  } while (0)

#define rk_assert_exit(code, ...)                                              \
  do {                                                                         \
    RKT_SEND_HDR("rk_assert_exit(" #code ", " #__VA_ARGS__ ")", RKTF_EXIT,     \
                 code);                                                        \
    __VA_ARGS__;                                                               \
    {                                                                          \
      RKT_AssertResPkg RK_PKG = {1, 0};                                        \
      RKT_send_res(&RK_PKG);                                                   \
    }                                                                          \
    exit(0);                                                                   \
  } while (0)

/// implementation section
typedef enum RKT_TestEndType {
  RKT_PASSED,
  RKT_FAILED,   // Regular failure
  RKT_CRASHED,  // Unexpected crash
  RKT_EXITED,   // Unexpected exit
  RKT_TIMEDOUT, // Test timed out
  RKT_TESTERROR // Error in the testing function
} RKT_TestEndType;

#ifdef _WIN32
typedef HANDLE              rk_fd;
typedef PROCESS_INFORMATION rk_pid;
#else
typedef int   rk_fd;
typedef pid_t rk_pid;
#endif

typedef enum RKT_TestAssertFunction {
  RKTF_TRUE,
  RKTF_FALSE,
  RKTF_NULL,
  RKTF_NNULL,
  RKTF_EQ,
  RKTF_TOLEQ,
  RKTF_NEQ,
  RKTF_TOLNEQ,
  RKTF_LT,
  RKTF_LEQ,
  RKTF_GT,
  RKTF_GEQ,
  RKTF_INRANGE,
  RKTF_MEMEQ,
  RKTF_MEMNEQ,
  RKTF_MEMZERO,
  RKTF_MEMNZERO,
  RKTF_STREQ,
  RKTF_STRNEQ,
  RKTF_CRASH,
  RKTF_EXIT,
  RK_TEST_STDOUTEQ,
  RK_TEST_STDOUTNEQ,
  RK_TEST_STDERREQ,
  RK_TEST_STDERRNEQ,
} RKT_TestAssertFunction;

typedef struct RKT_Strv {
  char*  str;
  size_t len;
} RKT_Strv;
typedef const struct RKT_Cstrv {
  const char* str;
  size_t      len;
} RKT_Cstrv;

// TODO
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

typedef struct RKT_Exit {
  bool exited_abnormally;
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
    RKT_CrashReason reason;
    int             exit_code;
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
  struct RKT_Suite*     next;
  struct RKT_TestEntry* tests;
  const RKT_Cstrv       name;
  RKT_CustomAttrs       attrs;
} RKT_Suite;

/// @brief Summarised results of each Test function; owns its pointers
typedef struct RKT_TestResult {
  size_t          count;   ///< Number of assertions in the test
  size_t          fails;   ///< Failed assertions in the test
  RKT_AssertDat*  results; ///< Result of each assertion in the test
  RKT_Strv        capt[2]; ///< Captured stdout/stderr
  RKT_TestEndType ended;
  RKT_Exit        exit_status;
} RKT_TestResult;

/// @brief Data and results of each test function
typedef struct RKT_TestEntry {
  struct RKT_TestEntry* next;
  RKT_Suite*            suite;
  const RKT_Cstrv       name;
  const RKT_Cstrv       file;
  void (*const func)(void);
  RKT_CustomAttrs attrs;
  union {
    RKT_TestResult res;
    RKT_Cstrv      tmp_suitename;
  };
} RKT_TestEntry;

/// @brief Global settings/variables for the testing framework
static struct {
  RKT_CustomAttrs attrs;
  const char*     custom_paths[3]; // todo this system is a mess
  FILE*           output_types[3]; // todo rename to default files?
  RKT_Suite*      suites;
  size_t          tests_run;
  rk_fd           log_writer;
  jmp_buf         fret;
#ifndef _WIN32
  sigjmp_buf fret_sig;
#endif
  union {
    RKT_AssertResPkg resbuf;
    char             backing_storage[2048]; // buffer for optimisation
  };
} RKT_glob;

#ifdef __GNUC__
# pragma GCC diagnostic push
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

#define RK_COUNTOF(...)            (sizeof(__VA_ARGS__) / sizeof((__VA_ARGS__)[0]))
#define RKT_lenof(strlit)          (sizeof("" strlit "") - 1)
#define RK_MIN(X, Y)               ((X) <= (Y) ? (X) : (Y))
#define RK_MAX(X, Y)               ((X) >= (Y) ? (X) : (Y))
#define RKT_malloc(T, COUNT)       ((T*)malloc(sizeof(T) * (COUNT)))
#define RKT_realloc(T, PTR, COUNT) ((T*)realloc(PTR, sizeof(T) * (COUNT)))

rkt_fun const char* RKT_stringify_crash(RKT_CrashReason reason) {
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
rkt_fun int               RKT_write_full(rk_fd fd, const void* buf, size_t len);
rkt_fun int               RKT_read_full_nb(rk_fd fd, void* buf, size_t nbytes);
rkt_fun rk_pid            RKT_init_test(const RKT_TestEntry* restrict e,
                                        rk_fd* restrict pipes);
rkt_fun RKT_TestEndType   RKT_parent_loop(RKT_TestEntry* restrict e, rk_pid pid,
                                          rk_fd* restrict pipes);
rkt_fun RKT_TestEndType   RKT_run_test(RKT_TestEntry* e);

rkt_fun RKT_Exit          RKT_inspect_exit(int code) {
  RKT_Exit res;
#ifdef __GNUC__
  if (WIFSIGNALED(code)) {
    res.exited_abnormally = true;
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
    res.exited_abnormally = false, res.exit_code = WEXITSTATUS(code);
  }
#else
  if (code >= 0x80000000) {
    res.exited_abnormally = true;
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
    res.exited_abnormally = false, res.exit_code = code;
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
  if (e->attrs.isolation) { return e->attrs.isolation != RKT_ISOLATION_OFF; }
  if (e->suite->attrs.isolation) {
    return e->suite->attrs.isolation != RKT_ISOLATION_OFF;
  }
  return RKT_glob.attrs.isolation != RKT_ISOLATION_OFF;
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
  switch (s) {
  case RK_OUTPUTSTREAMS_STDOUT:
    return e->res.capt[s].len
        && (e->res.ended != RKT_PASSED || v == RK_VERBOSITY_ALWAYS);
  case RK_OUTPUTSTREAMS_STDERR:
    return e->res.capt[s].len && v != RK_VERBOSITY_NEVER;
  case RK_OUTPUTSTREAMS_LOG: return v != RK_VERBOSITY_NEVER;
  }
  assert(0);
}

rkt_fun void RKT_send_hdr(RKT_AssertHdr hdr) {
  if (RKT_write_full(RKT_glob.log_writer, &hdr, sizeof(hdr)) < 1) {
    exit(1); // todo important
  }
}
#define RKT_SEND_HDR(ESTR, FUNENUM, SIG)                                       \
  RKT_send_hdr((RKT_AssertHdr){__LINE__, FUNENUM, SIG, {ESTR, RKT_lenof(ESTR)}})

rkt_fun size_t RKT_send_res(const RKT_AssertResPkg* buf) {
  if (RKT_write_full(RKT_glob.log_writer, buf, sizeof(*buf) + buf->len) < 1) {
    exit(1); // todo error handling
  }
  return buf->res;
}

#define RK_EXPECT(FUN, FUNENUM, EXPRSTR, ...)                                  \
  (RKT_SEND_HDR(EXPRSTR, FUNENUM, 0), FUN(__VA_ARGS__))

#define RK_ASSERT(FUN, FUNENUM, EXPRSTR, ...)                                  \
  (RKT_SEND_HDR(EXPRSTR, FUNENUM, 0),                                          \
   (FUN(__VA_ARGS__) ? (longjmp(RKT_glob.fret, 1)) : ((void)0)))

/// @brief returns first position where strings differ +1 (0 if same)
rkt_fun size_t RKT_strdiff(const char* e1, size_t l1, const char* e2,
                           size_t l2) {
  size_t min = RK_MIN(l1, l2);
  for (size_t i = 0; i < min; ++i) {
    if (e1[i] != e2[i]) { return i + 1; }
  }
  return l1 == l2 ? 0 : (min + 1);
}

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
  Y(const void*, vp, "%p", ##__VA_ARGS__)

#define RKT_TYPELIST_FLOAT(Y, ...)                                             \
  Y(float, f, "%f", ##__VA_ARGS__)                                             \
  Y(double, d, "%f", ##__VA_ARGS__)                                            \
  Y(long double, ld, "%Lf", ##__VA_ARGS__)

#define RKT_TYPELIST(Y, ...)                                                   \
  RKT_TYPELIST_FLOAT(Y, ##__VA_ARGS__)                                         \
  RKT_TYPELIST_NOFLOAT(Y, ##__VA_ARGS__)

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
# define RKT_SELFUN_(T, N, F, FTYPE) , T : RKTf_##FTYPE##_##N
# define RKT_SELFUN(FTYPE, VAL)                                                \
   _Generic((VAL)RKT_TYPELIST(RKT_SELFUN_, FTYPE),                             \
       char*: RKTf_streq,                                                      \
       const char*: RKTf_streq,                                                \
       default: RKTf_##FTYPE##_vp)
# define RKT_SELFUN_TOL_(T, N, F, FTYPE) , T : RKTf_##FTYPE##_##N
# define RKT_SELFUN_TOL(FTYPE, EXP)                                            \
   _Generic((EXP)RKT_TYPELIST_FLOAT(RKT_SELFUN_TOL_, FTYPE))

# define RKT_GEN_f2(T, N, FMT, name, expr)                                     \
   rkt_fun size_t RKTf_##name##_##N(T e1, T e2) {                              \
     RKT_SNDBUF2((expr), e1, FMT, e2, FMT);                                    \
   }
# define RKT_GEN_f3(T, N, FMT, name, expr)                                     \
   rkt_fun size_t RKTf_##name##_##N(T e1, T e2, T e3) {                        \
     RKT_SNDBUF3((expr), e1, FMT, e2, FMT, e3, FMT);                           \
   }

#else

# define RKT_SELFUN(FTYPE, VAL)     RKTf_##FTYPE
# define RKT_SELFUN_TOL(FTYPE, ...) RKTf_##FTYPE

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
  void add_arg_impl(const T& val, std::false_type) {
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
                        mem_size = sizeof(RKT_glob.backing_storage); // todo
    void grow_to(size_t ncap) {
      if (cap == mem_size) {
        char* nbuf = RKT_malloc(char, ncap);
        if (!nbuf) { exit(1); } // todo out of memory
        memcpy(nbuf, buf, mem_size), buf = nbuf;
      } else {
        if (!(buf = RKT_realloc(char, buf, ncap))) { exit(1); }
        // todo out of memory
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

// clang-format off
rkt_fun size_t RKTf_streq(const char* e1, const char* e2);
rkt_fun size_t RKTf_strneq(const char* e1, const char* e2);
template <> rkt_fun size_t RKTf_eq<char*, char*>(char* e1, char* e2) { return RKTf_streq(e1, e2); }
template <> rkt_fun size_t RKTf_eq<char*, const char*>(char* e1, const char* e2) { return RKTf_streq(e1, e2); }
template <> rkt_fun size_t RKTf_eq<const char*, char*>(const char* e1, char* e2) { return RKTf_streq(e1, e2); }
template <> rkt_fun size_t RKTf_eq<const char*, const char*>(const char* e1, const char* e2) { return RKTf_streq(e1, e2); }
template <> rkt_fun size_t RKTf_neq<char*, char*>(char* e1, char* e2) { return RKTf_strneq(e1, e2); }
template <> rkt_fun size_t RKTf_neq<char*, const char*>(char* e1, const char* e2) { return RKTf_strneq(e1, e2); }
template <> rkt_fun size_t RKTf_neq<const char*, char*>(const char* e1, char* e2) { return RKTf_strneq(e1, e2); }
template <> rkt_fun size_t RKTf_neq<const char*, const char*>(const char* e1, const char* e2) { return RKTf_strneq(e1, e2); }

// clang-format on
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

# define RKT_GEN_fargs_tmplate(F)                                              \
   F(T, U, "", eq, !(e1 == e2))                                                \
   F(T, U, "", neq, !(e1 != e2))                                               \
   F(T, U, "", gt, (e1 > e2 ? 0 : (e1 == e2 ? 1 : 2)))                         \
   F(T, U, "", geq, !(e1 >= e2))                                               \
   F(T, U, "", lt, (e1 < e2 ? 0 : (e1 == e2 ? 1 : 2)))                         \
   F(T, U, "", leq, !(e1 <= e2))

RKT_GEN_fargs_tmplate(RKT_GEN_f2TMPLATE)

#endif

// clang-format off
#define RKT_GEN_fargs_all(T, N, FMT, ...)                          \
    RKT_GEN_f2(T, N, FMT, lt, (e1 < e2 ? 0 : (e1 == e2 ? 1 : 2)))         \
    RKT_GEN_f2(T, N, FMT, leq, !(e1 <= e2))                               \
    RKT_GEN_f2(T, N, FMT, gt, (e1 > e2 ? 0 : (e1 == e2 ? 1 : 2)))         \
    RKT_GEN_f2(T, N, FMT, geq, !(e1 >= e2))                               \
    RKT_GEN_f3(T, N, FMT, inrange, (e1 < e2 ? 1 : (e1 > e3 ? 2 : 0)))
#define RKT_GEN_fargs_nofloat(T, N, FMT, ...)                           \
    RKT_GEN_f2(T, N, FMT, eq, !(e1 == e2))                                \
    RKT_GEN_f2(T, N, FMT, neq, !(e1 != e2))                       
#define RKT_GEN_fargs_float(T, N, FMT, eps_default, fabs_fn)            \
    RKT_GEN_f2(T, N, FMT, eq, !(fabs_fn(e1 - e2) <= eps_default))         \
    RKT_GEN_f2(T, N, FMT, neq, !(fabs_fn(e1 - e2) > eps_default))         \
    RKT_GEN_f3(T, N, FMT, toleq, !(fabs_fn(e1 - e2) <= e3))               \
    RKT_GEN_f3(T, N, FMT, tolneq, !(fabs_fn(e1 - e2) > e3))

RKT_TYPELIST_NOFLOAT(RKT_GEN_fargs_nofloat)                
RKT_TYPELIST(RKT_GEN_fargs_all)                       
RKT_GEN_fargs_float(float, f, "%g", 1e-6f, fabsf)              
RKT_GEN_fargs_float(double, d, "%g", 1e-12, fabs)          
RKT_GEN_fargs_float(long double, ld, "%Lg", 1e-12L, fabsl)
    // clang-format on

    // clang-format off

rkt_fun size_t RKTf_true(bool e1) { RKT_SNDBUF0(!e1); }
rkt_fun size_t RKTf_false(bool e1) { RKT_SNDBUF0(!!e1); } // clang-format on

rkt_fun size_t RKTf_null(const void* e1) { RKT_SNDBUF1(!(!e1), e1, "%p"); }
rkt_fun size_t RKTf_nnull(const void* e1) { RKT_SNDBUF1(!(e1), e1, "%p"); }

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
  res->res = RKT_strdiff(e1, n, e2, n), res->len         = n + n + 2;
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
      s += RKT_fmtstub(s, RKT_FMT_P, e, hdr);
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
    case RKTF_TRUE : s += RK_catlit(s, RK_INRED(" == 'false'")); break;
    case RKTF_FALSE: s += RK_catlit(s, RK_INRED(" == 'true'")); break;
    case RKTF_NULL : s += sprintf(s, RK_INRED("%s != 'NULL'"), args[0]); break;
    case RKTF_NNULL: s += RK_catlit(s, RK_INRED(" == 'NULL'")); break;
    case RKTF_EQ:
    case RKTF_TOLEQ:
      s += sprintf(s, RK_INRED("%s != %s"), args[0], args[1]);
      break;
    case RKTF_NEQ:
    case RKTF_TOLNEQ:
      s += sprintf(s, RK_INRED("%s == %s"), args[0], args[1]);
      break;
    case RKTF_LT:
      s += sprintf(s, RK_INRED("%s%s%s"), args[0],
                   res.res == 1 ? " == " : " > ", args[1]);
      break;
    case RKTF_LEQ:
      s += sprintf(s, RK_INRED("%s > %s"), args[0], args[1]);
      break;
    case RKTF_GT:
      s += sprintf(s, RK_INRED("%s%s%s"), args[0],
                   res.res == 1 ? " == " : " < ", args[1]);
      break;
    case RKTF_GEQ:
      s += sprintf(s, RK_INRED("%s < %s"), args[0], args[1]);
      break;
    case RKTF_INRANGE:
      s += sprintf(s, RK_INRED("%s%s%s"), args[0], res.res == 1 ? " < " : " > ",
                   res.res == 1 ? args[1] : args[2]);
      break;
    case RKTF_MEMEQ:
      s += sprintf(s, "%s and %s (%s bytes long) " RK_INRED("not equal"),
                   args[0], args[1], args[2]);
      break;
    case RKTF_MEMNEQ:
      s += sprintf(s, "%s and %s (%s bytes long) " RK_INRED("equal"), args[0],
                   args[1], args[2]);
      break;
    case RKTF_MEMZERO:
      s += sprintf(s, "%s (%s bytes long) " RK_INRED("nonzero at byte %zu"),
                   args[0], args[1], res.res - 1);
      break;
    case RKTF_MEMNZERO:
      s += sprintf(s, "%s (%s bytes long) " RK_INRED("zero at byte %zu"),
                   args[0], args[1], res.res - 1);
      break;
    case RKTF_STREQ:
      s += sprintf(s, "%s != %s " RK_INRED("position %zu"), args[0], args[1],
                   res.res - 1);
      break;
    case RKTF_STRNEQ:
      s += sprintf(s, "%s " RK_INRED("==") " %s", args[0], args[0]);
      break;
    case RKTF_CRASH:
      if (res.res == 1) {                   // TODO WINDOWS
        s += RK_catlit(s, "Did not crash"); // todo
      } else {
        s += sprintf(s, "Expected: %s, got: %s",
                     RKT_stringify_crash(hdr->reason),
                     RKT_stringify_crash(e->res.exit_status.reason));
      }
      break;
    case RKTF_EXIT:
      if (res.res == 1) {
        s += RK_catlit(s, "Did not exit");
      } else {
        s += sprintf(s, "Expected: exit via code %d, got: exit with code %d\n",
                     hdr->exit_code, e->res.exit_status.exit_code);
      }
      break;
    case RK_TEST_STDOUTEQ:
    case RK_TEST_STDOUTNEQ:
    case RK_TEST_STDERREQ:
    case RK_TEST_STDERRNEQ: exit(1); break; // todo not implemented
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
    if (!fmtbuf) { fputs("Error: Realloc fmt failed\n", stderr), exit(1); }
    for (size_t i = 0; i < r.count; ++i) {
      size_t bytes = RKT_fmt_assertres(e, r.results[i], &fmtbuf, &fmtcap);
      fwrite(fmtbuf, 1, bytes, out);
    }
    free(fmtbuf);
  }
  if (RKT_shall_print(e, RK_OUTPUTSTREAMS_STDOUT)) {
    fputs("\nStdout:\n", out);
    fwrite(r.capt[RK_OUTPUTSTREAMS_STDOUT].str, 1,
           r.capt[RK_OUTPUTSTREAMS_STDOUT].len, out);
    fputc('\n', out);
  }
  if (RKT_shall_print(e, RK_OUTPUTSTREAMS_STDERR)) {
    fputs("\nStderr:\n", out);
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
            RKT_stringify_crash(r.exit_status.reason));
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
    if (r.exit_status.exited_abnormally) {
      fprintf(out, RK_INRED("Error in Test Function: %s\n\n"),
              RKT_stringify_crash(r.exit_status.reason));
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
    fputs("{\n  \"suites\": [\n", RKT_glob.output_types[RK_OUTPUTFORMAT_JSON]);
  }
}

rkt_fun void RKT_print_total_end_json() {
  if (RKT_glob.output_types[RK_OUTPUTFORMAT_JSON]) {
    fputs("  ]\n}\n", RKT_glob.output_types[RK_OUTPUTFORMAT_JSON]);
  }
}
rkt_fun void RKT_json_escape_n(const char* str, size_t len) {
  FILE* out = RKT_glob.output_types[RK_OUTPUTFORMAT_JSON];
  for (size_t i = 0; i < len; ++i) {
    switch (str[i]) {
    case '\"': fputs("\\\"", out); break;
    case '\\': fputs("\\\\", out); break;
    case '\b': fputs("\\b", out); break;
    case '\f': fputs("\\f", out); break;
    case '\n': fputs("\\n", out); break;
    case '\r': fputs("\\r", out); break;
    case '\t': fputs("\\t", out); break;
    default:
      if ((unsigned char)str[i] < 0x20) {
        fprintf(out, "\\u%04x", (unsigned char)str[i]);
      } else {
        fputc(str[i], out);
      }
    }
  }
}

rkt_fun void RKT_print_test_json(const RKT_TestEntry* e) {
  const RKT_TestResult* r   = &e->res;
  FILE*                 out = RKT_glob.output_types[RK_OUTPUTFORMAT_JSON];
  if (out) {
    fputs("    {\n", out);
    fputs("      \"test_name\": \"", out);
    RKT_json_escape_n(e->name.str, e->name.len), fputs("\",\n", out);
    fputs("      \"file\": \"", out);
    RKT_json_escape_n(e->file.str, e->file.len), fputs("\",\n", out);
    fputs("      \"tags\": \"", out);
    size_t tags_len = e->attrs.tags ? strlen(e->attrs.tags) : 0;
    RKT_json_escape_n(e->attrs.tags, tags_len), fputs("\",\n", out);
    fputs("      \"result\": {\n", out);
    fprintf(out, "        \"ended\": %d,\n", r->ended);
    if (!r->exit_status.exited_abnormally) {
      fprintf(out, "        \"term_code\": %d,\n",
              r->exit_status.exit_code); // todo maybe wrong
    } else {
      fprintf(out, "        \"reason\": %s,\n",
              RKT_stringify_crash(r->exit_status.reason));
    }
    fprintf(out, "        \"assert_count\": %zu,\n", r->count);
    fprintf(out, "        \"assert_failures\": %zu,\n", r->fails);
    if (!r->count) {
      fputs("        \"assertions\": []", out);
    } else {
      fputs("        \"assertions\": [\n", out);
      for (size_t i = 0; i < r->count; i++) {
        RKT_AssertDat* ad = &r->results[i];
        fputs("          {\n", out);
        fputs("            \"expr\": \"", out);
        RKT_json_escape_n(ad->hdr.expr.str, ad->hdr.expr.len);
        fputs("\",\n", out);
        fprintf(out, "            \"pos\": %d,\n", ad->hdr.pos);
        const char* resstr;
        switch (ad->res.res) {
        case RKT_ASSERTRES_PASS   : resstr = "pass"; break;
        default                   : resstr = "fail"; break;
        case RKT_ASSERTRES_EXIT   : resstr = "exit"; break;
        case RKT_ASSERTRES_CRASH  : resstr = "crash"; break;
        case RKT_ASSERTRES_TIMEOUT: resstr = "timeout"; break;
        }
        fprintf(out, "            \"res\": \"%s\",\n", resstr);
        fputs("            \"args\": [", out);
        for (int j = 0; j < 3; ++j) { // shit
          if (!ad->res.args[j + 1]) { break; }
          if (j > 0) { fputs(", ", out); }
          fputs("\"", out);
          RKT_json_escape_n(ad->res.args[j], (size_t)(ad->res.args[j + 1]
                                                      - ad->res.args[j] - 1));
          fputs("\"", out);
        }
        fputs("]\n", out);
        fputs("          }", out);
        if (i != r->count - 1) { fputs(",", out); }
        fputc('\n', out);
      }
      fputs("        ]", out);
    }
    if (RKT_shall_print(e, RK_OUTPUTSTREAMS_STDOUT)) {
      fputs(",\n        \"stdout\": \"", out);
      RKT_json_escape_n(e->res.capt[RK_OUTPUTSTREAMS_STDOUT].str,
                        e->res.capt[RK_OUTPUTSTREAMS_STDOUT].len);
      fputs("\"", out);
    }
    if (RKT_shall_print(e, RK_OUTPUTSTREAMS_STDERR)) {
      fputs(",\n        \"stderr\": \"", out);
      RKT_json_escape_n(e->res.capt[RK_OUTPUTSTREAMS_STDERR].str,
                        e->res.capt[RK_OUTPUTSTREAMS_STDERR].len);
      fputs("\"", out);
    }
    fputs("\n", out);
    fputs("      }\n", out);
    fputs("    }", out);
    if (e->next) { fputs(",", out); }
    fputc('\n', out);
  }
}

rkt_fun void RKT_print_suite_json(const RKT_Suite* s, size_t total,
                                  size_t failed, size_t crashed,
                                  size_t timed_out, size_t error) {
  FILE* out = RKT_glob.output_types[RK_OUTPUTFORMAT_JSON];
  if (!out) { return; }
  fputs("  {\n", out);
  fputs("    \"suite_name\": \"", out);
  RKT_json_escape_n(s->name.str, s->name.len);
  fputs("\",\n", out);
  fputs("    \"tests\": [\n", out);
  for (const RKT_TestEntry* e = s->tests; e; e = e->next) {
    RKT_print_test_json(e);
  }
  fputs("    ]\n", out);
  fputs("  }", out);
  if (s->next) { fputs(",", out); }
  fputc('\n', out);
}

rkt_fun void RKT_print_test_tap(const RKT_TestEntry* e) {
  FILE* out = RKT_glob.output_types[RK_OUTPUTFORMAT_TAP];
  if (!out) { return; }
  RKT_TestResult r = e->res;
  fputs(e->res.ended == RKT_PASSED ? "ok " : "not ok ", out);
  fprintf(out, "%zu ", RKT_glob.tests_run);
  fputs(e->suite->name.str, out);
  fputs(" > ", out);
  fputs(e->name.str, out);
  // fputs("\n --- \n", out);
  fputs(" # -> ", out);
  switch (e->res.ended) {
  case RKT_PASSED:
    fprintf(out, "PASS: Passed all %zu Assertions.", r.count);
    break;
  case RKT_FAILED:
    fprintf(out, "FAIL: %zu fails, %zu passed (total %zu)", r.fails,
            r.count - r.fails, r.count);
    break;
  case RKT_CRASHED:
    fprintf(out,
            "CRASH: %zu fails, %zu passed (total %zu), "
            "crashed unexpectedly with: %s",
            r.fails, r.count - r.fails, r.count,
            RKT_stringify_crash(r.exit_status.reason));
    break;
  case RKT_EXITED:
    fprintf(out,
            "UExit: %zu fails, %zu passed (total %zu), "
            "exited unexpectedly with code %d",
            r.fails, r.count - r.fails, r.count, r.exit_status.exit_code);
    break;
  case RKT_TIMEDOUT:
    fprintf(out,
            "TIMED OUT: %zu fails, %zu passed (total %zu), exceeded time of "
            "%ldms",
            r.fails, r.count - r.fails, r.count, RKT_resolve_timeout(e));
    break;
  case RKT_TESTERROR: // todo windows
    if (r.exit_status.exited_abnormally) {
      fprintf(out, RK_INRED("Error in Test Function: %s\n\n"),
              RKT_stringify_crash(r.exit_status.reason));
    } else {
      fprintf(out, RK_INRED("Error in Test Function: Exited with code %d\n\n"),
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

rkt_fun int RKT_init_test_noisolation(const RKT_TestEntry* restrict e,
                                      rk_fd* restrict pipes);
rkt_fun RKT_TestEndType RKT_parent_loop_noisolation(RKT_TestEntry* restrict e,
                                                    bool timed_out,
                                                    rk_fd* restrict fds);
rkt_fun RKT_TestEndType RKT_run_test(RKT_TestEntry* e) {
  // printf("RUNNING %s\n", e->name); // for debugging
  rk_fd pipes[3];
  if (RKT_resolve_isolation(e) == false) {
    int timed_out = RKT_init_test_noisolation(e, pipes);
    RKT_parent_loop_noisolation(e, timed_out, pipes);
  } else {
    rk_pid pid = RKT_init_test(e, pipes);
    RKT_parent_loop(e, pid, pipes);
  }
  close(pipes[0]), close(pipes[1]), close(pipes[2]);
  ++RKT_glob.tests_run;
  RKT_print_test_tap(e);
  return e->res.ended;
}

rkt_fun void RKT_run_suite(RKT_Suite* s) {
  //  printf("RUNNING SUITE %s\n", s->name); // for debugging
  if (s->attrs.init) { s->attrs.init(); }
  size_t total = 0, failed = 0, crashed = 0, timed_out = 0, error = 0;
  for (RKT_TestEntry* e = s->tests; e; e = e->next) {
    if (!RKT_glob.attrs.tags) { // no tags to look for
    run_test:
      switch (++total, RKT_run_test(e)) {
      case RKT_PASSED   : continue;
      case RKT_FAILED   : ++failed; continue;
      case RKT_CRASHED  : ++crashed; continue;
      case RKT_EXITED   : ++crashed; continue;
      case RKT_TIMEDOUT : ++timed_out; continue;
      case RKT_TESTERROR: ++error; continue;
      }
    }
    if (!e->attrs.tags) { continue; }
    for (const char *cur = RKT_glob.attrs.tags, *end = cur; *cur != '\0';
         cur = end + 1) {
      for (; *end && *end != ','; ++end);
      size_t len = end - cur;
      for (const char* p = e->attrs.tags; *p; ++p) {
        if (!strncmp(p, cur, len) && (p[len] == ',' || !p[len])) {
          goto run_test;
        }
      }
      if (*end == '\0') { break; }
    }
  }
  if (s->attrs.fini) { s->attrs.fini(); }
  RKT_print_suite_reg(s, total, failed, crashed, timed_out, error);
  RKT_print_suite_json(s, total, failed, crashed, timed_out, error);
  RKT_cleanup_suite_res(s);
}
rkt_fun void RKT_init(RKT_CustomAttrs attrs) {
  for (int i = 0; i < 3; ++i) {
    FILE* const default_files[3] = {stdout, NULL, NULL};
    if (RKT_glob.custom_paths[i]) {
      if (!strcmp(RKT_glob.custom_paths[i], "stdout")) {
        RKT_glob.output_types[i] = stdout;
      } else if (!strcmp(RKT_glob.custom_paths[i], "stderr")) {
        RKT_glob.output_types[i] = stderr;
      } else {
        if (strcmp(RKT_glob.custom_paths[i], "none")) { // todo
          if (!(RKT_glob.output_types[i]
                = fopen(RKT_glob.custom_paths[i], "w"))) {
            perror("fopen"), exit(1);
          }
        }
      }
    } else {
      RKT_glob.output_types[i] = default_files[i];
    }
  }
  RKT_glob.attrs = attrs;
  RKT_init_group();
}
rkt_fun void RKT_run_all_tests(const char** suites_strs, size_t nsuites_strs,
                               RKT_CustomAttrs attrs) {
  RKT_init(attrs);
  RKT_print_total_beg_json();
  if (RKT_glob.attrs.init) { RKT_glob.attrs.init(); }
  for (RKT_Suite* s = RKT_glob.suites; s; s = s->next) {
    if (nsuites_strs) {
      for (size_t i = 0; i < nsuites_strs; ++i) {
        if (!strcmp(suites_strs[i], s->name.str)) { goto runsuite; }
      }
    } else {
    runsuite:
      RKT_run_suite(s);
    }
  }
  if (RKT_glob.attrs.fini) { RKT_glob.attrs.fini(); }
  RKT_print_end_tap();
  RKT_print_total_end_json();
  for (int i = 0; i < 3; ++i) {
    if (RKT_glob.output_types[i] && RKT_glob.output_types[i] != stdout
        && RKT_glob.output_types[i] != stderr) {
      fclose(RKT_glob.output_types[i]);
    }
  }
  // printf("--- Passed %zu, fails %zu, crashed %zu out of %zu tests ---\n",
  //        total - failed - crashed, failed, crashed, total);
}

#ifndef _WIN32
rkt_fun void RKT_handle_sigterm(int sigterm) {
  // Sigterm does not automatically propagate to child processes in a process
  // group, so we manually forward it to all processes in the group.
  kill(-getpid(), sigterm);
  _exit(1); // TODO exit code
}

rkt_fun void RKT_init_group() {
  setpgid(0, 0);
  sigset_t m;
  sigemptyset(&m);
  // block these to ensure children get killed in handler
  sigaddset(&m, SIGINT), sigaddset(&m, SIGQUIT), sigaddset(&m, SIGTSTP);
  struct sigaction sa = {.sa_handler = RKT_handle_sigterm, .sa_mask = m};
  sigaction(SIGTERM, &sa, NULL);
}
RKT_noreturn rkt_fun void RKT_fatal(const char* str) {
  perror(str);
  fflush(NULL);
  sigset_t mask;
  sigemptyset(&mask), sigaddset(&mask, SIGTERM);
  sigprocmask(SIG_BLOCK, &mask, NULL);
  kill(0, SIGTERM); // Kill all children (not parent)
  abort();          // Abort for core dump
}
#else

static rk_fd RKT_glob_job;
rkt_fun void RKT_init_group() {
  RKT_glob_job = CreateJobObject(NULL, NULL);
  if (!AssignProcessToJobObject(RKT_glob_job, GetCurrentProcess())) { abort(); }
}
RKT_noreturn rkt_fun void RKT_fatal(const char* msg) {
  char* buf = NULL;
  FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
                     | FORMAT_MESSAGE_IGNORE_INSERTS,
                 NULL, GetLastError(),
                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&buf, 0,
                 NULL);
  fprintf(stderr, "%s: %s\n", msg, buf ? buf : "Unknown error");
  if (buf) { LocalFree(buf); }
  TerminateJobObject(RKT_glob_job, 1), abort();
}

#endif

/* todo long */
rkt_fun long RKT_now_ms(void) {
#ifdef __GNUC__
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#else
  static LARGE_INTEGER freq;
  if (!freq.QuadPart) { QueryPerformanceFrequency(&freq); }
  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);
  return (long)((counter.QuadPart * 1000) / freq.QuadPart);
#endif
}

rkt_fun int RKT_read_full_nb(rk_fd fd, void* buf, size_t nbytes) {
#ifdef __GNUC__
  struct pollfd pfd = {.fd = fd, .events = POLLIN};
  for (char* ptr = (char*)buf; nbytes;) {
    ssize_t r = read(fd, ptr, nbytes);
    switch (r) {
    case -1:
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        poll(&pfd, 1, -1); // todo maybe timeout
      } else if (errno != EINTR) {
        return -1;
      }
      break;
    case 0 : return (void*)ptr == buf ? 0 : -2; // eof
    default: ptr += r, nbytes -= r;
    }
  }
#else
  char* ptr = (char*)buf;
  for (DWORD r; nbytes;) {
    if (!ReadFile(h, ptr, (DWORD)nbytes, &r, NULL)) {
      switch (GetLastError()) {
      case ERROR_BROKEN_PIPE: return (void*)ptr == buf ? 0 : -2;
      case ERROR_NO_DATA    : continue;
      default               : return -1;
      }
    }
    if (!r) { return (void*)ptr == buf ? 0 : -2; }
    ptr += r, nbytes -= r;
  }
#endif
  return 1; // read all, no eof
}

// todo
rkt_fun int RKT_read_full(rk_fd fd, void* buf, size_t nbytes) {
#ifdef __GNUC__
  for (char* ptr = (char*)buf; nbytes;) {
    ssize_t r = read(fd, ptr, nbytes);
    switch (r) {
    case -1:
      if (errno != EINTR) { return -1; }
      break;
    case 0 : return 0; // eof
    default: ptr += r, nbytes -= r;
    }
  }
#else
  char* ptr = (char*)buf;
  for (DWORD r; nbytes;) {
    if (!ReadFile(h, ptr, (DWORD)nbytes, &r, NULL)) {
      switch (GetLastError()) {
      case ERROR_BROKEN_PIPE: return 0;
      default               : return -1;
      }
    }
    if (!r) { return 0; }
    ptr += r, nbytes -= r;
  }
#endif
  return 1; // read all, no eof
}

rkt_fun int RKT_write_full(rk_fd fd, const void* buf, size_t len) {
  const char* ptr = (const char*)buf;
#ifdef __GNUC__
  for (ssize_t w; len;) {
    if ((w = write(fd, ptr, len)) == -1) {
      if (errno != EINTR) { return -1; }
    } else {
      len -= (size_t)w, ptr += w;
    }
  }
#else
  for (DWORD w; len > 0; ptr += w, len -= w) {
    if (!WriteFile(h, ptr, (DWORD)len, &w, NULL)) { return -1; }
  }
#endif
  return 1;
}

rkt_fun int RKT_read_hdr(rk_fd fd, RKT_AssertHdr* restrict hdr) {
  return RKT_read_full_nb(fd, hdr, sizeof(RKT_AssertHdr));
}
rkt_fun int RKT_read_res(rk_fd fd, RKT_AssertRes* restrict res) {
  switch (RKT_read_full_nb(fd, res, sizeof(res->pkg))) {
  case -2: return -2;
  case -1: return -1;
  case 0 : return 0;
  case 1:
    if (!res->len) {
      memset(res->args, 0, sizeof(res->args));
    } else {
      if (!(res->args[0] = RKT_malloc(char, res->len))) { RKT_fatal("malloc"); }
      if (RKT_read_full_nb(fd, res->args[0], res->len) < 1) { return -2; }
      size_t i = 1;
      for (size_t len = 0; len + 1 < res->len; ++len) {
        if (res->args[0][len] == '\0') {
          assert(i + 1 < RK_COUNTOF(res->args));
          res->args[i++] = res->args[0] + len + 1;
        }
      }
      res->args[i] = res->args[0] + res->len; // points past last byte
      assert(i + 1 < RK_COUNTOF(res->args));
      res->args[i + 1] = NULL;
      //  second to last is sentinel, check next one == NULL to detect it
    }
  }
  return 1;
}
rkt_fun bool RKT_read_stream(rk_fd fd, RKT_Strv* str, size_t* restrict cap) {
  if (str->len == *cap - 1) {
    *cap *= 2, str->str = RKT_realloc(char, str->str, *cap);
    if (!str->str) { RKT_fatal("realloc"); }
  }
#ifndef _WIN32
  ssize_t rd;
  while ((rd = read(fd, str->str + str->len, *cap - str->len - 1)) == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) { return 1; }
    if (errno != EINTR) { RKT_fatal("read out/err res"); }
  }
#else
  DWORD rd;
  if (!ReadFile(fd, str->str + str->len, (DWORD)(*cap - str->len - 1), &rd,
                NULL)) {
    if (GetLastError() == ERROR_BROKEN_PIPE) { return 0; }
    RKT_fatal("ReadFile stdout/stderr");
  }
#endif
  str->str[str->len] = '\0';
  return rd != 0;
}

rkt_fun RKT_Exit RKT_reapchild(rk_pid pid) {
#ifndef _WIN32
  int status;
  while (waitpid(pid, &status, 0) == -1) {
    if (errno != EINTR) { RKT_fatal("waitpid"); }
  }
#else
  DWORD status;
  WaitForSingleObject(pid.hProcess, INFINITE);
  GetExitCodeProcess(pid.hProcess, &status);
  CloseHandle(pid.hProcess), CloseHandle(pid.hThread);
#endif
  return RKT_inspect_exit(status);
}

rkt_fun void RKT_finalise_test(RKT_TestResult* r, rk_pid pid,
                               bool reading_res) {
  r->exit_status = RKT_reapchild(pid);
  if (reading_res) {
    RKT_AssertRes* lastres = &r->results[r->count - 1].res;
    *lastres               = (RKT_AssertRes){RKT_ZINIT};
    RKT_AssertHdr* lasthdr = &r->results[r->count - 1].hdr;
    if (r->exit_status.exited_abnormally) {
      if (lasthdr->F == RKTF_CRASH) {
        if (lasthdr->reason != RKT_SIGNAL_ANY
            && r->exit_status.reason != lasthdr->reason) {
          lastres->res = 2;
        } else {
          lastres->res = 0;
        }
      } else {
        lastres->res = RKT_ASSERTRES_CRASH, r->ended = RKT_CRASHED;
      }
    } else {
      if (lasthdr->F == RKTF_EXIT) {
        if (lasthdr->exit_code != -1
            && r->exit_status.exit_code != lasthdr->exit_code) {
          lastres->res = 2;
        } else {
          lastres->res = 0;
        }
      } else {
        lastres->res = RKT_ASSERTRES_EXIT, r->ended = RKT_EXITED;
      }
    }
    if (lastres->res) { ++r->fails; }
  } else if (r->exit_status.exited_abnormally || r->exit_status.exit_code) {
    r->ended = RKT_TESTERROR;
  }
  if (r->ended == RKT_PASSED && r->fails) { r->ended = RKT_FAILED; }
}

/// PLATFORM DIFFERENCES
#ifndef _WIN32
# define RKT_CONSTRUCTOR(fn) __attribute__((constructor)) static void fn(void)
# define RKT_SPECIALCHILDENTRY()

rkt_fun void RKT_handle_sigalarm(int signum) {
  siglongjmp(RKT_glob.fret_sig, signum);
}
/*
todo important: unmix sigjmp and non sig
*/
rkt_fun int RKT_init_test_noisolation(const RKT_TestEntry* restrict e,
                                      rk_fd* restrict pipes) {
  int ret          = 0;
  int saved_stdout = dup(STDOUT_FILENO), saved_stderr = dup(STDERR_FILENO);
  if (saved_stdout < 0 || saved_stderr < 0) { RKT_fatal("dup"); }
  int  fds[3];
  char temps[3][32]
      = {"/tmp/rkt_out_XXXXXX", "/tmp/rkt_err_XXXXXX", "/tmp/rkt_log_XXXXXX"};
  for (int i = 0; i < 3; ++i) {
    if ((fds[i] = mkstemp(temps[i])) < 0) { RKT_fatal("mkstemp"); }
    if ((pipes[i] = open(temps[i], O_RDONLY)) < 0) {
      unlink(temps[i]), RKT_fatal("open read end");
    };
    unlink(temps[i]);
  }
  if (dup2(fds[0], STDOUT_FILENO) < 0 || dup2(fds[1], STDERR_FILENO) < 0) {
    RKT_fatal("dup2");
  }
  close(fds[0]), close(fds[1]);
  RKT_glob.log_writer = fds[2];
  if (e->attrs.init) { e->attrs.init(); }
  long ts = RKT_resolve_timeout(e);
  if (ts != -1) { // todo
    sigset_t m;
    sigemptyset(&m);
    struct sigaction sa = {.sa_handler = RKT_handle_sigalarm, .sa_mask = m};
    sigaction(SIGALRM, &sa, NULL);
    struct itimerval timer = {{0, 0}, {ts / 1000, (ts % 1000) * 1000}};
    setitimer(ITIMER_REAL, &timer, NULL);
  } // todo
  int val = sigsetjmp(RKT_glob.fret_sig, 1);
  if (val == 0) {
    int val2 = setjmp(RKT_glob.fret);
    if (val2 == 0) {
      e->func();
      if (ts != -1) {
        struct itimerval timer = {{0, 0}, {0, 0}};
        setitimer(ITIMER_REAL, &timer, NULL);
      }
    }
  } else {
    ret = 1;
    // todo distinguish between assert false and timeout
    // timed out?
  }
  if (e->attrs.fini) { e->attrs.fini(); }
  fflush(NULL);
  if (dup2(saved_stdout, STDOUT_FILENO) < 0
      || dup2(saved_stderr, STDERR_FILENO) < 0) {
    RKT_fatal("dup2 restore");
  }
  close(saved_stdout), close(saved_stderr);
  return ret;
}

rkt_fun rk_pid RKT_init_test(const RKT_TestEntry* restrict e,
                             rk_fd* restrict pipes) {
  int    pps[6];
  rk_pid pid;
  for (int i = 0; i < 3; ++i) {
    if (pipe(pps + i * 2) < 0) { RKT_fatal("pipe"); };
  }
  fflush(NULL);
  if ((pid = fork()) == -1) { RKT_fatal("fork"); }
  for (int i = 0, end = (pid > 0) ^ 1; i < 3; ++i) {
    close(pps[i * 2 + !end]), pipes[i] = pps[i * 2 + end];
  }
  if (pid == 0) {
    if (dup2(pipes[0], STDOUT_FILENO) < 0
        || dup2(pipes[1], STDERR_FILENO) < 0) {
      perror("dup2"), _exit(1); // todo
    }
    if (e->attrs.init) { e->attrs.init(); }
    RKT_glob.log_writer = pipes[RK_OUTPUTSTREAMS_LOG];
    int val             = setjmp(RKT_glob.fret);
    if (val == 0) {
      e->func();
    } else {
      // todo jump
    }
    if (e->attrs.fini) { e->attrs.fini(); }
    fflush(NULL); // todo apparently unsafe
    _exit(0);
  } else {
    for (int i = 0; i < 3; ++i) { // set pipes nonblocking
      int flags = fcntl(pipes[i], F_GETFL, 0);
      if (flags == -1 || fcntl(pipes[i], F_SETFL, flags | O_NONBLOCK) == -1) {
        RKT_fatal("fcntl");
      }
    }
  }
  return pid;
}
// TODO windows
rkt_fun char* RKT_read_fd(int fd, size_t* out_size) {
  struct stat st;
  if (fstat(fd, &st) != 0) { return NULL; }
  char* buf = malloc(st.st_size);
  if (!buf) { RKT_fatal("malloc"); }
  size_t total = 0;
  while (total < (size_t)st.st_size) {
    ssize_t n = read(fd, buf + total, st.st_size - total);
    if (n <= 0) { RKT_fatal("read"); }
    total += n;
  }
  *out_size = st.st_size;
  return buf;
}
/*
todo distinguish between timeout during assert and between asserts
*/
rkt_fun RKT_TestEndType RKT_parent_loop_noisolation(RKT_TestEntry* restrict e,
                                                    bool timed_out,
                                                    rk_fd* restrict fds) {
  size_t rescap                            = 512;
  e->res.capt[RK_OUTPUTSTREAMS_STDOUT].str = RKT_read_fd(
      fds[RK_OUTPUTSTREAMS_STDOUT], &e->res.capt[RK_OUTPUTSTREAMS_STDOUT].len);
  e->res.capt[RK_OUTPUTSTREAMS_STDERR].str = RKT_read_fd(
      fds[RK_OUTPUTSTREAMS_STDERR], &e->res.capt[RK_OUTPUTSTREAMS_STDERR].len);
  e->res.results = RKT_malloc(RKT_AssertDat, rescap);
  if (!e->res.results) { RKT_fatal("malloc buf"); }
  bool reading_res = 0;
  for (bool run = true; run;) {
    if (!reading_res) {
      if (e->res.count + 2 == rescap) { //+2 to simplify timeout code
        rescap         *= 2;
        e->res.results  = RKT_realloc(RKT_AssertDat, e->res.results, rescap);
        if (!e->res.results) { RKT_fatal("realloc 0"); }
      } // todo partial read timeout
      switch (RKT_read_hdr(fds[RK_OUTPUTSTREAMS_LOG],
                           &e->res.results[e->res.count].hdr)) {
      case -2: run = 0; break; // incomplete read
      case -1: RKT_fatal("read hdr");
      case 0 : run = 0; break;
      case 1 : ++e->res.count, reading_res = 1;
      }
    } else {
      switch (RKT_read_res(fds[RK_OUTPUTSTREAMS_LOG],
                           &e->res.results[e->res.count - 1].res)) {
      case -2:
        memset(&e->res.results[e->res.count - 1].res, 0, sizeof(RKT_AssertRes));
        e->res.results[e->res.count - 1].res.res = RKT_ASSERTRES_TIMEOUT;
        run                                      = 0;
        break; // incomplete read
      case -1: RKT_fatal("read res");
      case 0 : run = 0; break;
      case 1:
        e->res.fails += e->res.results[e->res.count - 1].res.res > 0;
        reading_res   = 0;
      }
    }
  }
  e->res.ended = e->res.fails ? RKT_FAILED : RKT_PASSED;
  if (timed_out) { // todo check this
    e->res.ended = RKT_TIMEDOUT;
  }
  return e->res.ended;
}

rkt_fun RKT_TestEndType RKT_parent_loop(RKT_TestEntry* restrict e, rk_pid pid,
                                        rk_fd* restrict pipes) {
  size_t rescap = 512, caps[2] = {512, 512};
  e->res.results     = RKT_malloc(RKT_AssertDat, rescap);
  e->res.capt[0].str = RKT_malloc(char, caps[0]);
  e->res.capt[1].str = RKT_malloc(char, caps[1]);
  if (!e->res.results || !e->res.capt[0].str || !e->res.capt[1].str) {
    RKT_fatal("malloc buf");
  }
  long          timeout_ms = RKT_resolve_timeout(e), time_start = RKT_now_ms();
  struct pollfd fds[3]
      = {{pipes[0], POLLIN, 0}, {pipes[1], POLLIN, 0}, {pipes[2], POLLIN, 0}};
  bool reading_res = 0;
  for (int nfds = 3; nfds > 0;) {
    for (;;) {
      if (timeout_ms != -1) {
        long elapsed = (RKT_now_ms() - time_start);
        timeout_ms   = RK_MAX(timeout_ms - elapsed, 0);
      }
      int pr = poll(fds, 3, (int)timeout_ms);
      if (pr == -1) {
        if (errno != EINTR) { RKT_fatal("poll"); }
      } else if (pr > 0) {
        break;
      } else {
        if (!reading_res) {
          memset(&e->res.results[e->res.count++].hdr, 0, sizeof(RKT_AssertHdr));
        }
        memset(&e->res.results[e->res.count - 1].res, 0, sizeof(RKT_AssertRes));
        e->res.results[e->res.count - 1].res.res = RKT_ASSERTRES_TIMEOUT;
        kill(pid, SIGKILL), RKT_reapchild(pid); // todo sigterm?
        e->res.ended = RKT_TIMEDOUT;
        return e->res.ended;
      }
    }
    if (fds[RK_OUTPUTSTREAMS_LOG].revents & POLLIN) {
      if (!reading_res) {
        if (e->res.count + 2 == rescap) { //+2 to simplify timeout code
          rescap         *= 2;
          e->res.results  = RKT_realloc(RKT_AssertDat, e->res.results, rescap);
          if (!e->res.results) { RKT_fatal("realloc 0"); }
        }
        switch (RKT_read_hdr(fds[RK_OUTPUTSTREAMS_LOG].fd,
                             &e->res.results[e->res.count].hdr)) {
        case -2:
        case -1: RKT_fatal("read hdr");
        case 0 : fds[RK_OUTPUTSTREAMS_LOG].fd = -1, --nfds; break;
        case 1 : ++e->res.count, reading_res = 1;
        }
      } else {
        switch (RKT_read_res(fds[RK_OUTPUTSTREAMS_LOG].fd,
                             &e->res.results[e->res.count - 1].res)) {
        case -2:
        case -1: RKT_fatal("read res");
        case 0 : fds[RK_OUTPUTSTREAMS_LOG].fd = -1, --nfds; break;
        case 1:
          e->res.fails += e->res.results[e->res.count - 1].res.res > 0;
          reading_res   = 0;
        }
      }
    }
    for (int i = 0; i < 2; ++i) {
      if (fds[i].revents & POLLIN) {
        if (!RKT_read_stream(fds[i].fd, &e->res.capt[i], &caps[i])) {
          fds[i].fd = -1, --nfds;
          break;
        }
      }
    }
  }
  RKT_finalise_test(&e->res, pid, reading_res);
  return e->res.ended;
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
      if (!strcmp(buf, e->name.str)) {
        if (!GetEnvironmentVariableA("RK_CHILD_META", buf, sizeof(buf))) {
          exit(1);
        }
        char* ep;
        rk_fd meta_fd = (rk_fd)(uintptr_t)strtoumax(buf, &ep, 10);
        if (*ep != '\0') { exit(1); }
        if (RKT_glob.attrs.init) { RKT_glob.attrs.init(); }
        if (s->attrs.init) { s->attrs.init(); }
        if (e->attrs.init) { e->attrs.init(); }
        RKT_glob.log_writer = meta_fd;
        int val             = setjmp(RKT_glob.fret);
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
  }
  exit(1); /*function not found?*/
}
# define RKT_SPECIALCHILDENTRY() RKT_Windows_Childentry()

rkt_fun rk_pid RKT_init_test(const RKT_TestEntry* restrict e,
                             rk_fd* restrict pipes) {
  SECURITY_ATTRIBUTES sa = {sizeof(sa), NULL, TRUE};
  rk_fd               w_ends[3];
  for (int i = 0; i < 3; ++i) {
    if (!CreatePipe(&pipes[i], &w_ends[i], &sa, 0)
        || !SetHandleInformation(pipes[i], HANDLE_FLAG_INHERIT, 0)) {
      exit(1);
    }
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
               + RKT_lenof("RK_CHILD_META=") + 32 + 1 + 1;
  char *nenv = RKT_malloc(char, extra + plen), *s = nenv;
  if (!nenv) { exit(1); }
  s    += RK_catlit(s, "RK_CHILD_FN=");
  s    += RK_catstr(s, e->name.str, e->name.len);
  *s++  = '\0';

  s    += sprintf(s, "RK_CHILD_META=%" PRIuPTR,
                  (uintptr_t)w_ends[RK_OUTPUTSTREAMS_LOG])
     + 1;
  if (plen) { s += (memcpy(s, penv, plen), plen); }
  *s = '\0'; /* ensure double-null terminator */

  STARTUPINFO si;
  ZeroMemory(&si, sizeof(si)); // Clear everything
  si.cb         = sizeof(si);
  si.dwFlags    = STARTF_USESTDHANDLES;
  si.hStdOutput = w_ends[RK_OUTPUTSTREAMS_STDOUT];
  si.hStdError  = w_ends[RK_OUTPUTSTREAMS_STDERR];
  rk_pid pid;
  BOOL   worked
      = CreateProcessA(path, path, NULL, NULL, TRUE, 0, nenv, NULL, &si, &pid);
  if (penv) { FreeEnvironmentStringsA(penv); }
  free(nenv);
  CloseHandle(w_ends[0]), CloseHandle(w_ends[1]), CloseHandle(w_ends[2]);
  if (!worked) { exit(1); }
  return pid;
}

rkt_fun RKT_TestEndType RKT_parent_loop(RKT_TestEntry* restrict e, rk_pid pid,
                                        rk_fd* restrict pipes) {
  long   timeout_ms = RKT_resolve_timeout(e);
  size_t rescap = 512, caps[2] = {512, 512};
  e->res.results     = RKT_malloc(RKT_AssertDat, rescap);
  e->res.capt[0].str = RKT_malloc(char, caps[0]);
  e->res.capt[1].str = RKT_malloc(char, caps[1]);
  if (!e->res.results || !e->res.capt[0].str || !e->res.capt[1].str) {
    RKT_fatal("malloc buf");
  }
  long  timeout_ms = RKT_resolve_timeout(e), time_start = RKT_now_ms();
  rk_fd handles[4]  = {pipes[0], pipes[1], pipes[2]};
  bool  reading_res = 0;
  for (int nfds = 3; nfds > 0;) {
    DWORD poll_timeout = INFINITE;
    if (timeout_ms != -1) {
      long elapsed = RKT_now_ms() - time_start;
      poll_timeout = RK_MAX(timeout_ms - elapsed, 0);
    }
    DWORD wait = WaitForMultipleObjects(nfds, handles, FALSE, poll_timeout);
    if (wait == WAIT_FAILED) {
      RKT_fatal("WaitForMultipleObjects");
    } else if (wait == WAIT_TIMEOUT) {
      RKT_AssertRes res = {RKT_ASSERTRES_TIMEOUT, 0, {RKT_ZINIT}};
      if (reading_res) {
        e->res.results[e->res.count - 1].res = res;
      } else {
        memset(&e->res.results[e->res.count].hdr, 0, sizeof(RKT_AssertHdr));
        e->res.results[e->res.count++].res = res;
      }
      TerminateProcess(pid.hProcess, 1), RKT_reapchild(pid);
      e->res.ended = RKT_TIMEDOUT;
      return e->res.ended;
    }
    int idx = wait - WAIT_OBJECT_0;
    if (handles[idx] == pipes[RK_OUTPUTSTREAMS_LOG]) {
      if (!reading_res) {
        if (e->res.count + 2 == rescap) { //+2 to simplify timeout code
          rescap         *= 2;
          e->res.results  = RKT_realloc(RKT_AssertDat, e->res.results, rescap);
          if (!e->res.results) { RKT_fatal("realloc 0"); }
        }
        ++e->res.count;
        switch (RKT_read_hdr(handles[idx], &e->res.results[e->res.count].hdr)) {
        case -2:
        case -1: RKT_fatal("read hdr");
        case 0 : handles[idx] = INVALID_HANDLE_VALUE; break;
        case 1 : ++e->res.count, reading_res = 1;
        }
      } else {
        switch (
            RKT_read_res(handles[idx], &e->res.results[e->res.count - 1].res)) {
        case -2:
        case -1: RKT_fatal("read res");
        case 0 : handles[idx] = INVALID_HANDLE_VALUE; break;
        case 1:
          e->res.fails += e->res.results[e->res.count - 1].res.res > 0;
          reading_res   = 0;
        }
      }
    } else {
      size_t i = handles[idx] != pipes[RK_OUTPUTSTREAMS_STDOUT];
      switch (RKT_read_stream(handles[idx], &e->res.capt[i], &caps[i])) {
      case true : break;
      case false: handles[idx] = INVALID_HANDLE_VALUE; break;
      }
    }
    for (int i = nfds = 0; i < 3; ++i) {
      if (handles[i] != INVALID_HANDLE_VALUE) { handles[nfds++] = handles[i]; }
    }
  }
  RKT_finalise_test(&e->res, pid, reading_res);
  return e->res.ended;
}

#endif

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

/*
todo fix suite issues
*/

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

/// @brief todo documentation
#define RKT_REGISTER_SUITE_IMPL(SUITENAME, ...)                                \
  RKT_CONSTRUCTOR(RKT_CONCAT(RK_test_suite_register_, __COUNTER__)) {          \
    RKT_CustomAttrs attrs = {__VA_ARGS__};                                     \
    RKT_Suite*      s     = RKT_find_suite(SUITENAME);                         \
    if (!s) {                                                                  \
      static RKT_Suite suite                                                   \
          = {NULL, NULL, {SUITENAME, RKT_lenof(SUITENAME)}};                   \
      s = RKT_add_suite(&suite);                                               \
    }                                                                          \
    s->attrs = attrs;                                                          \
  }

/// @brief todo documentation
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

/// @brief todo documentation
#define RKT_RUN_TESTS_IMPL(suites, nsuites, ...)                               \
  do {                                                                         \
    RKT_SPECIALCHILDENTRY()                                                    \
    RKT_CustomAttrs attrs = {__VA_ARGS__};                                     \
    RKT_run_all_tests(suites, nsuites, attrs);                                 \
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
#if 0
 int*               globmem;
 int                globmemlen;
 rkt_fun void test2_init(void) {
     globmemlen = 10000, globmem = (int*)malloc(sizeof(int) * 10000);
     for (int i = 0; i < globmemlen; ++i) { globmem[i] = i; }
 }
 rkt_fun void test2_fini(void) {
     globmemlen = 0, free(globmem), globmem = 0;
 }
 RK_REGISTER_SUITE("test2", .init = test2_init, .fini = test2_fini)

 RK_REGISTER_TEST("test2", test_test1) {
     rk_assert_eq(globmemlen, 10000);
     for (int i = 0; i < globmemlen; ++i) { globmem[i] = i; }
 }
 RK_REGISTER_TEST("test2", test_test2) {
     for (int i = 0; i < globmemlen; ++i) { printf("%d\n", globmem[i]); }
     rk_expect_eq(globmemlen, 10000);
 }

 RK_REGISTER_TEST("test2", test_test_lots_of_asserts) {
     for (size_t i = 0; i < 100000; ++i) {
         rk_expect_eq(i % 10000, i % 10000 - 1);
     }
 }

#endif

#endif
