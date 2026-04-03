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
# include <fcntl.h> // _O_WRONLY, _O_BINARY
# include <io.h>    // _open_osfhandle, _dup2, _dup, _close
# include <windows.h>
#else
# define _GNU_SOURCE /* todo*/
# include <fcntl.h>
# include <poll.h>
# include <sys/stat.h>
# include <sys/time.h>
# include <sys/wait.h>
# include <unistd.h>
#endif
#ifdef __cplusplus
# include <cmath>
# include <sstream>
# include <type_traits>
# if __cpp_lib_format >= 201907L
#  include <format>
# endif
#endif
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <setjmp.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
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

#define RKT_MAKE_ENUM_VAL(name, val) name = val,
#if defined(__cplusplus)                                                       \
    || (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L               \
        && !defined(_MSC_VER))
# define RKT_MAKE_ENUM_TYPEDEF(T, ITEMS)                                       \
   typedef enum T : char { ITEMS(RKT_MAKE_ENUM_VAL) } T;
#else
# define RKT_MAKE_ENUM_TYPEDEF(T, ITEMS)                                       \
   typedef signed char T;                                                      \
   enum { ITEMS(RKT_MAKE_ENUM_VAL) };
#endif

#define RKT_VERBOSITY_ITEMS(X)                                                 \
  X(RK_VERBOSITY_INHERIT, 0)                                                   \
  X(RK_VERBOSITY_AUTO, 1)                                                      \
  X(RK_VERBOSITY_ALWAYS, 2)                                                    \
  X(RK_VERBOSITY_NEVER, 3)
#define RK_VERBOSITY_DEFAULT RK_VERBOSITY_AUTO
RKT_MAKE_ENUM_TYPEDEF(RK_Verbosity, RKT_VERBOSITY_ITEMS)

/// @brief Whether a test function runs in its own process (more robust) or in
/// the main process (faster)
#define RKT_ISOLATIONMODE_ITEMS(X)                                             \
  X(RK_ISOLATION_INHERIT, 0)                                                   \
  X(RK_ISOLATION_ON, 1)                                                        \
  X(RK_ISOLATION_OFF, 2)
#define RK_ISOLATION_DEFAULT RK_ISOLATION_ON

RKT_MAKE_ENUM_TYPEDEF(RK_IsolationMode, RKT_ISOLATIONMODE_ITEMS)

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
  RK_FixtureFunc   init, fini;          /// Setup and Teardown fixtures
  RK_Verbosity     verbosity_levels[3]; /// verbosity level, per RK_OutPutStream
  RK_IsolationMode isolation;
  unsigned         timeout_ms; /// Timeout in ms (default is no timeout)
} RK_CustomTestAttributes;

/// @brief todo documentation
#define RK_REGISTER_SUITE(SUITENAME, ...)                                      \
  RKT_REGISTER_SUITE_IMPL(SUITENAME, __VA_ARGS__)

/// @brief todo documentation
#define RK_REGISTER_TEST(SUITENAME, FN, ...)                                   \
  RKT_REGISTER_TEST_IMPL(SUITENAME, FN, __VA_ARGS__)

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

// two C-strings 
#define rk_assert_streq(str1, str2)           RKT_AE2(assert, streq, str1, str2, #str1, #str2)
#define rk_assert_strneq(str1, str2)          RKT_AE2(assert, strneq, str1, str2, #str1, #str2)

// todo document/extend 
// Two C-strings, with a length limit (e.g. for testing if a string is correctly truncated)
#define rk_assert_streq_n(str1, str2, len)    RKT_AE3(assert, streq_n, str1, str2, len, #str1, #str2, #len)
#define rk_assert_strneq_n(str1, str2, len)   RKT_AE3(assert, strneq_n, str1, str2, len, #str1, #str2, #len)

// Two C-strings, with a length limit (e.g. for testing if a string is correctly truncated)
#define rk_assert_streq_exact(str1, len1, str2, len2)    RKT_AE4(assert, streq_n, str1, len1, str2, len2, #str1, #len1, #str2, #len2)
#define rk_assert_strneq_exact(str1, len1, str2, len2)   RKT_AE4(assert, strneq_n, str1, len1, str2, len2, #str1, #len1, #str2, #len2)


#define rk_assert_stdouteq(errstr, len)       RKT_AE2(assert, stdouteq, errstr, len, #errstr, #len)
#define rk_assert_stdoutneq(errstr, len)      RKT_AE2(assert, stdoutneq, errstr, len, #errstr, #len)
#define rk_assert_stderreq(errstr, len)       RKT_AE2(assert, stderreq, errstr, len, #errstr, #len)
#define rk_assert_stderrneq(errstr, len)      RKT_AE2(assert, stderrneq, errstr, len, #errstr, #len)

//todo
#define rk_assert_stdout_starts_with(errstr, len)       RKT_AE2(assert, stdouteq, errstr, len, #errstr, #len)
#define rk_assert_stdout_nstarts_with(errstr, len)      RKT_AE2(assert, stdoutneq, errstr, len, #errstr, #len)
#define rk_assert_stderr_starts_with(errstr, len)       RKT_AE2(assert, stderreq, errstr, len, #errstr, #len)
#define rk_assert_stderr_nstarts_with(errstr, len)      RKT_AE2(assert, stderrneq, errstr, len, #errstr, #len)

// clang-format on

// TODO IMPORTANT STATEMENTS LENGTH
#define rk_assert_crash(signal, ...)                                           \
  do {                                                                         \
    RKT_SEND_HDR("assert_crash(" #signal ", " #__VA_ARGS__ ")", RKTF_crash,    \
                 signal);                                                      \
    RK__IGNWARN_CLANG_BEG("-Wunused-value")                                    \
    __VA_ARGS__; /*todo unused*/                                               \
    RK__IGNWARN_CLANG_END() {                                                  \
      RKT_AssertRes RK_PKG = {2, 0};                                           \
      RKT_send_res(&RK_PKG);                                                   \
    }                                                                          \
    exit(0);                                                                   \
  } while (0)

#define rk_assert_exit(code, ...)                                              \
  do {                                                                         \
    RKT_SEND_HDR("assert_exit(" #code ", " #__VA_ARGS__ ")", RKTF_exit, code); \
    RK__IGNWARN_CLANG_BEG("-Wunused-value")                                    \
    __VA_ARGS__; /*todo unused*/                                               \
    RK__IGNWARN_CLANG_END() {                                                  \
      RKT_AssertRes RK_PKG = {2, 0};                                           \
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

#if defined(__cplusplus) || (!defined(_MSC_VER) && __STDC_VERSION__ >= 202000L)
# define rk_null nullptr
#else
# define rk_null ((void*)0)
#endif

#define RKT_AE1(AE, FUN, A1, S1)                                               \
  RKT_CONCAT(RKT_, AE)(#AE "_" #FUN "(" S1 ")", RKTF_##FUN, RKTf_##FUN(A1))
#define RKT_AE2(AE, FUN, A1, A2, S1, S2)                                       \
  RKT_CONCAT(RKT_, AE)(#AE "_" #FUN "(" S1 ", " S2 ")", RKTF_##FUN,            \
                       RKTf_##FUN(A1, A2))
#define RKT_AE3(AE, FUN, A1, A2, A3, S1, S2, S3)                               \
  RKT_CONCAT(RKT_, AE)(#AE "_" #FUN "(" S1 ", " S2 ", " S3 ")", RKTF_##FUN,    \
                       RKTf_##FUN(A1, A2, A3))
#define RKT_AE4(AE, FUN, A1, A2, A3, A4, S1, S2, S3, S4)                       \
  RKT_CONCAT(RKT_, AE)(#AE "_" #FUN "(" S1 ", " S2 ", " S3 ", " S4 ")",        \
                       RKTF_##FUN, RKTf_##FUN(A1, A2, A3, A4))

#define RKT_ASSERT_FAILED_CODE 124

#define RKT_expect(EXPRSTR, FUNENUM, FUNCALL)                                  \
  (RKT_SEND_HDR(EXPRSTR, FUNENUM, 0), (FUNCALL))

#define RKT_assert(EXPRSTR, FUNENUM, FUNCALL)                                  \
  (RKT_SEND_HDR(EXPRSTR, FUNENUM, 0),                                          \
   ((FUNCALL) ? (longjmp(RKT_glob.fret, RKT_ASSERT_FAILED_CODE)) : ((void)0)))

#define RKT_TESTENDTYPE_ITEMS(X)                                               \
  X(RKT_PASSED, 0)                                                             \
  X(RKT_FAILED, 1)                                                             \
  X(RKT_CRASHED, 2)                                                            \
  X(RKT_EXITED, 3)                                                             \
  X(RKT_TIMEDOUT, 4)                                                           \
  X(RKT_TESTERROR, 5)
RKT_MAKE_ENUM_TYPEDEF(RKT_TestEndType, RKT_TESTENDTYPE_ITEMS)
#undef RKT_TESTENDTYPE_ITEMS

#ifndef _WIN32
typedef int      rkt_fd;
typedef pid_t    rkt_pid;
typedef uint32_t RKT_dword;
#else
typedef HANDLE              rkt_fd;
typedef PROCESS_INFORMATION rkt_pid;
typedef DWORD               RKT_dword;
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
  RKTF_stdouteq,
  RKTF_stdoutneq,
  RKTF_stderreq,
  RKTF_stderrneq,
  RKTF_crash,
  RKTF_exit,
} RKT_TestAssertFunction;

typedef enum RKT_CrashReason {
  RKT_FAULT_ANY = -1,
  RKT_FAULT_NONE,
  RKT_FAULT_MEMORY     = SIGSEGV, // access violation / SIGSEGV
  RKT_FAULT_ILLEGAL_OP = SIGILL,  // SIGILL, EXCEPTION_ILLEGAL_INSTRUCTION
  RKT_FAULT_ABORT      = SIGABRT, // SIGABRT, fatal app exit
  RKT_FAULT_ARITHMETIC = SIGFPE,  // SIGFPE, FP/INT divide-by-zero
  RKT_FAULT_BREAKPOINT,           // SIGTRAP or EXCEPTION_BREAKPOINT
  RKT_FAULT_UNKNOWN,
} RKT_CrashReason;

typedef enum RKT_ExitType {
  RKT_EXIT_EXITED,
  RKT_EXIT_FAULT,
  RKT_EXIT_DISCARD
} RKT_ExitType;

typedef struct RKT_Exit {
  RKT_ExitType type;
  union {
    RKT_CrashReason reason;
    int             exit_code;
  };
} RKT_Exit;

typedef struct RKT_Strv {
  char*  str;
  size_t len;
} RKT_Strv;

typedef struct RKT_Cstrv {
  const char* str;
  size_t      len;
} RKT_Cstrv;

/// @brief Header to be sent to the test runner before assert/expect
typedef struct RKT_AssertHdr {
  int                    pos; ///< The line of the function
  RKT_TestAssertFunction F;   ///< The assert function (assert_eq, assert_true)
  union {
    int             exit_code;
    RKT_CrashReason reason;
  };
  struct {
    unsigned char len;
    char          str[243]; ///< The stringified assertion
  } expr;
} RKT_AssertHdr;

#define RKT_ASSERTRES_PASS    (((size_t)(0)))
#define RKT_ASSERTRES_TIMEOUT (((size_t)(-1)) - 2)
#define RKT_ASSERTRES_CRASH   (((size_t)(-1)) - 1)
#define RKT_ASSERTRES_EXIT    (((size_t)(-1)))

/// @brief the arguments and result (true/false or other state) is stored here
typedef struct RKT_AssertRes {
  size_t res;
  size_t len;
  char   args[];
} RKT_AssertRes;

/// @brief Data packet for each assertion
typedef struct RKT_AssertDat {
  RKT_AssertHdr  hdr; ///< Received before each assert runs
  RKT_AssertRes* res; ///< Received after each assert
} RKT_AssertDat;

typedef struct RKT_Suite {
  struct RKT_Suite*       next;
  struct RKT_TestEntry*   tests;
  const RKT_Cstrv         name;
  RK_CustomTestAttributes attrs;
} RKT_Suite;

/// @brief Summarised results of each Test function; owns its pointers
typedef struct RKT_TestResult {
  unsigned        count;   ///< Number of assertions in the test
  unsigned        fails;   ///< Failed assertions in the test
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
  void (*const func)(void);
  size_t                  id;
  const RKT_Cstrv         file;
  RK_CustomTestAttributes attrs;
  RKT_TestResult          res;
} RKT_TestEntry;

/// @brief Global settings/variables for the testing framework
static struct {
  RK_CustomTestAttributes attrs;
  const char*             custom_paths[3]; // todo this system is a mess
  FILE*                   output_types[3]; // todo rename to default files?
  RKT_Suite*              suites;
  size_t                  tests_run;
  rkt_fd tmpfds[3]; // fds of tmp files, open for duration of program
  rkt_fd saved[3];  // saved stdout/stderr (0 is reserved for future use)
#ifdef _WIN32
  // TODO remove
  int saved_crt_out;
  int saved_crt_err;
#endif
  rkt_fd  log;
  jmp_buf fret;
  rkt_pid pid;
#ifndef _WIN32
  sigjmp_buf fret_sig;
#else
  rkt_fd job;
#endif
} RKT_glob;

union {
  RKT_AssertRes pkg;
  char          storage[2048]; // buffer for optimisation
} RKT_glob_buf;

static const bool RKT_print_passes = 0; // todo figure out what to do with this

/* ------------------------------------------------------------------------- */
/* Function prototypes (in the order functions appear in the file)           */
/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
# define rkt_fun inline
# define RKT_ZINIT
# define RKT_TMP(T, EXPR)                                                      \
   T { EXPR }
#else
# define rkt_fun   static inline
# define RKT_ZINIT 0
# define RKT_TMP(T, EXPR)                                                      \
   (T) { EXPR }
#endif
#define RKT_ZTMP(T) RKT_TMP(T, RKT_ZINIT)

#if (defined(_MSC_VER) || defined(__cplusplus))
# define RKT_restrict __restrict
#else
# define RKT_restrict restrict
#endif

#ifdef unreachable
# define RKT_unreachable unreachable
#elif defined(__cplusplus) && __cplusplus >= 202302L
# include <utility>
# define RKT_unreachable() std::unreachable()
#elif defined(__GNUC__)
# define RKT_unreachable() __builtin_unreachable()
#elif defined(_MSC_VER)
# define RKT_unreachable() __assume(0)
#else
# define RKT_unreachable() (assert(0 && "unreachable code reached"), abort())
#endif

#if (defined(__cplusplus) && __cplusplus >= 201103L)
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

#ifndef _WIN32
# define RKT_closeHandle(file) close(file);
# define RKT_HASCHILD()        (!!RKT_glob.pid)
# define RKT_KILLCHILD()       kill(RKT_glob.pid, SIGKILL)
#else
# define RKT_closeHandle(file) CloseHandle(file);
# define RKT_HASCHILD()        (!!RKT_glob.pid.hProcess)
# define RKT_KILLCHILD()       TerminateProcess(RKT_glob.pid.hProcess, 1)
#endif

/* NOTE: Some functions are already forward-declared earlier in the header.
   This block includes *all* functions defined in the provided snippet, in
   definition order, including ones missing from the earlier forward-decls. */

/* --- Program logic ------------------------------------------------------- */
RKT_noreturn rkt_fun void RKT_fatal(const char* str);
rkt_fun RKT_Exit          RKT_reapchild(void);
rkt_fun int64_t           RKT_now_ms(void);

rkt_fun int  RKT_init_test_noisolation(const RKT_TestEntry* RKT_restrict e);
rkt_fun void RKT_parent_loop_noisolation(RKT_TestEntry* RKT_restrict e,
                                         bool                        timed_out);
rkt_fun void RKT_finalise_test_noisolation(RKT_TestResult* r,
                                           int             exit_condition);

rkt_fun void RKT_init_test_isolation(const RKT_TestEntry* RKT_restrict e);
rkt_fun void RKT_parent_loop_isolation(RKT_TestEntry* RKT_restrict e);
rkt_fun void RKT_finalise_test_isolation(RKT_TestResult* r, int exit_condition);

#ifndef _WIN32
rkt_fun void RKT_handle_sigterm(int sigterm);
rkt_fun void RKT_handle_sigalarm(int sigalarm);
#endif

rkt_fun int       RKT_read_full(rkt_fd fd, void* buf, size_t nbytes);
rkt_fun int       RKT_read_full_tm(rkt_fd fd, void* buf, size_t nbytes,
                                   long timeout_ms);
rkt_fun int       RKT_write_full(rkt_fd fd, const void* buf, size_t len);

rkt_fun void      RKT_rewindfile(rkt_fd fd);
rkt_fun RKT_Strv  RKT_readfile(rkt_fd fd);

rkt_fun void      RKT_init_fds(void);
rkt_fun void      RKT_init_global(RK_CustomTestAttributes attrs);

rkt_fun void      RKT_run_all(const char** suites_strs, size_t nsuites_strs,
                              RK_CustomTestAttributes attrs);
rkt_fun void      RKT_run_suite(RKT_Suite* RKT_restrict s);
rkt_fun void      RKT_run_test(RKT_TestEntry* e);

rkt_fun RKT_dword RKT_runfunc(void* arg);

rkt_fun int       RKT_read_hdr(RKT_AssertHdr* RKT_restrict hdr);
rkt_fun int  RKT_read_hdr_tm(RKT_AssertHdr* RKT_restrict hdr, long ms_left);
rkt_fun int  RKT_read_res(RKT_AssertRes** RKT_restrict r);
rkt_fun int  RKT_read_res_tm(RKT_AssertRes** RKT_restrict r, long ms);

rkt_fun void RKT_redirect_streams(rkt_fd new_logfd);
rkt_fun void RKT_restore_streams(void);

#ifdef _WIN32
rkt_fun void RKT_Windows_Childentry(void);
#endif

/* --- Resolution / child IPC helpers ------------------------------------- */
rkt_fun const char*  RKT_strcrash(RKT_CrashReason reason);
rkt_fun long         RKT_resolve_timeout(const RKT_TestEntry* RKT_restrict e);
rkt_fun bool         RKT_resolve_isolation(const RKT_TestEntry* RKT_restrict e);
rkt_fun RK_Verbosity RKT_resolve_verbosity(const RKT_TestEntry* RKT_restrict e,
                                           RK_OutPutStreams type);
rkt_fun bool   RKT_shall_print(const RKT_TestEntry* e, RK_OutPutStreams s);
rkt_fun void   RKT_send_hdr(int line, RKT_TestAssertFunction f, int sig,
                            const char* str, size_t len);
rkt_fun size_t RKT_send_res(const RKT_AssertRes* buf);

/* --- Generated assertion helpers (by macros) ----------------------------- */
/* These functions are *defined* by macro expansion later (RKT_GEN_*).
   For refactoring, it’s convenient to declare them via the same typelists. */

/* --- Non-generated assertion helpers ------------------------------------ */
rkt_fun size_t RKTf_true(bool e1);
rkt_fun size_t RKTf_false(bool e1);
rkt_fun size_t RKTf_null(const void* e1);
rkt_fun size_t RKTf_nonnull(const void* e1);

rkt_fun size_t RKTf_memzero(const void* e1, size_t e2);
rkt_fun size_t RKTf_memnzero(const void* e1, size_t e2);

rkt_fun size_t RKT_memdiff(const void* e1, size_t l1, const void* e2,
                           size_t l2);
rkt_fun size_t RKT_strn_compare_impl_exact(bool want_equal, const char* e1,
                                           size_t l1, const char* e2,
                                           size_t l2);

rkt_fun size_t RKTf_streq_impl(bool want_equal, const char* e1, const char* e2);
rkt_fun size_t RKTf_streq_n_impl(bool want_equal, const char* e1,
                                 const char* e2, size_t n);

rkt_fun size_t RKTf_streq(const char* e1, const char* e2);
rkt_fun size_t RKTf_strneq(const char* e1, const char* e2);
rkt_fun size_t RKTf_streq_n(const char* e1, const char* e2, size_t e3);
rkt_fun size_t RKTf_strneq_n(const char* e1, const char* e2, size_t e3);

rkt_fun size_t RKTf_streameq(RK_OutPutStreams stream, bool want_equal,
                             const char* str, size_t len);
rkt_fun size_t RKTf_stdouteq(const char* str, size_t len);
rkt_fun size_t RKTf_stdoutneq(const char* str, size_t len);
rkt_fun size_t RKTf_stderreq(const char* str, size_t len);
rkt_fun size_t RKTf_stderrneq(const char* str, size_t len);

rkt_fun size_t RKTf_memeq_impl(bool want_equal, const void* e1, const void* e2,
                               size_t e3);
rkt_fun size_t RKTf_memeq(const void* e1, const void* e2, size_t e3);
rkt_fun size_t RKTf_memneq(const void* e1, const void* e2, size_t e3);

/* --- Registration helpers ------------------------------------------------ */
rkt_fun RKT_Suite*     RKT_find_suite(const char* suitename);
rkt_fun RKT_TestEntry* RKT_add_test_to_suite(RKT_Suite*     s,
                                             RKT_TestEntry* entry);
rkt_fun RKT_Suite*     RKT_add_suite(RKT_Suite* suite);

/* --- Printing ------------------------------------------------------------ */
rkt_fun size_t RKT_unpack_args(const char* RKT_restrict args_in, size_t len,
                               const char** RKT_restrict args);

rkt_fun size_t RKT_print_assertres(const RKT_TestEntry* RKT_restrict e,
                                   RKT_AssertDat cur, FILE* RKT_restrict out);
rkt_fun void   RKT_print_test_reg(const RKT_TestEntry* RKT_restrict e);

rkt_fun void   RKT_print_suite_reg(const RKT_Suite* s, size_t total,
                                   size_t failed, size_t crashed,
                                   size_t timed_out, size_t error);

rkt_fun void   RKT_print_total_beg_json(void);
rkt_fun void   RKT_print_total_end_json(void);
rkt_fun void   RKT_json_escape_n(const char* RKT_restrict str, size_t len);

rkt_fun void   RKT_print_test_json(const RKT_TestEntry* RKT_restrict e);
rkt_fun void   RKT_print_suite_json(const RKT_Suite* s, size_t total,
                                    size_t failed, size_t crashed,
                                    size_t timed_out, size_t error);

rkt_fun void   RKT_print_test_tap(const RKT_TestEntry* e);
rkt_fun void   RKT_print_end_tap(void);

rkt_fun void   RKT_cleanup_test_res(RKT_TestEntry* e);
rkt_fun void   RKT_cleanup_suite_res(RKT_Suite* s);

#define RK_DO_PRAGMA(a) _Pragma(#a)
#ifdef __clang__
/// Disables warnings about:
/// - Zero-argument variadic macro arguments that work on all targeted platforms
/// - Overriding keywords as macros (typeof with certain flags)
/// - Unknown warning options (for the next warning on older compiler versions)
/// - Warnings about c2x, c2y and c23 extensions (namely countof, for which a
///   fallback exists and attribute syntax), since they're used conditionally
///   and portably.
/// - Warnings about unknown attributes (for the previous two)
# define RK__SILENCE_WARNINGS_BEG                                              \
   RK_DO_PRAGMA(clang diagnostic push)                                         \
   RK_DO_PRAGMA(clang diagnostic ignored                                       \
                "-Wgnu-zero-variadic-macro-arguments")                         \
   RK_DO_PRAGMA(clang diagnostic ignored "-Wkeyword-macro")                    \
   RK_DO_PRAGMA(clang diagnostic ignored "-Wunknown-warning-option")           \
   RK_DO_PRAGMA(clang diagnostic ignored "-Wc2x-extensions")                   \
   RK_DO_PRAGMA(clang diagnostic ignored "-Wc23-extensions")                   \
   RK_DO_PRAGMA(clang diagnostic ignored "-Wc2y-extensions")                   \
   RK_DO_PRAGMA(clang diagnostic ignored "-Wunknown-attributes")               \
   RK_DO_PRAGMA(clang diagnostic ignored "-Wc++17-extensions")

# define RK__SILENCE_WARNINGS_END RK_DO_PRAGMA(clang diagnostic pop)

# define RK__IGNWARN_CLANG_BEG(warn)                                           \
   RK_DO_PRAGMA(clang diagnostic push)                                         \
   RK_DO_PRAGMA(clang diagnostic ignored warn)
# define RK__IGNWARN_CLANG_END() RK_DO_PRAGMA(clang diagnostic pop)
# define RK__IGNWARN_CLANG(warn, ...)                                          \
   RK__IGNWARN_CLANG_BEG(warn)                                                 \
   (__VA_ARGS__) RK__IGNWARN_CLANG_END()
#else
# define RK__IGNWARN_CLANG_BEG(warn)
# define RK__IGNWARN_CLANG_END()
# define RK__IGNWARN_CLANG(warn, ...)
#endif

#ifdef __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wpragmas"
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
# pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
# pragma GCC diagnostic ignored "-Wc2x-extensions"
# pragma GCC diagnostic ignored "-Wunused-function"
#endif

#define RKT_CONCAT(a, b)           RKT_CONCAT2(a, b)
#define RKT_CONCAT2(a, b)          a##b
#define RKT_COUNTOF(...)           (sizeof(__VA_ARGS__) / sizeof((__VA_ARGS__)[0]))
#define RKT_lenof(STRLIT)          (sizeof("" STRLIT "") - 1)
#define RKT_MIN(X, Y)              ((X) <= (Y) ? (X) : (Y))
#define RKT_MAX(X, Y)              ((X) >= (Y) ? (X) : (Y))
#define RKT_malloc(T, COUNT)       ((T*)malloc(sizeof(T) * (COUNT)))
#define RKT_realloc(T, PTR, COUNT) ((T*)realloc(PTR, sizeof(T) * (COUNT)))

rkt_fun const char* RKT_strcrash(RKT_CrashReason reason) {
  switch (reason) {
  case RKT_FAULT_ANY       : return "Crashed (any)";
  case RKT_FAULT_NONE      : return "No crash";
  case RKT_FAULT_MEMORY    : return "Segmentation fault / access violation";
  case RKT_FAULT_ILLEGAL_OP: return "Illegal instruction";
  case RKT_FAULT_ABORT     : return "Abort (SIGABRT / fatal app exit)";
  case RKT_FAULT_ARITHMETIC: return "Divide by zero";
  case RKT_FAULT_BREAKPOINT: return "Breakpoint trap";
  case RKT_FAULT_UNKNOWN   : return "Unknown crash";
  default                  : return "Invalid crash reason";
  }
}

rkt_fun long RKT_resolve_timeout(const RKT_TestEntry* RKT_restrict e) {
  if (e->attrs.timeout_ms) { return e->attrs.timeout_ms; }
  if (e->suite->attrs.timeout_ms) { return e->suite->attrs.timeout_ms; }
  if (RKT_glob.attrs.timeout_ms) { return RKT_glob.attrs.timeout_ms; }
  return -1;
}
rkt_fun bool RKT_resolve_isolation(const RKT_TestEntry* RKT_restrict e) {
  if (e->attrs.isolation) { return e->attrs.isolation != RK_ISOLATION_OFF; }
  if (e->suite->attrs.isolation) {
    return e->suite->attrs.isolation != RK_ISOLATION_OFF;
  }
  return RKT_glob.attrs.isolation != RK_ISOLATION_OFF;
}

rkt_fun RK_Verbosity RKT_resolve_verbosity(const RKT_TestEntry* RKT_restrict e,
                                           RK_OutPutStreams type) {
  if (e->attrs.verbosity_levels[type] != RK_VERBOSITY_INHERIT) {
    return e->attrs.verbosity_levels[type];
  } else if (e->suite->attrs.verbosity_levels[type] != RK_VERBOSITY_INHERIT) {
    return e->suite->attrs.verbosity_levels[type];
  }

  return RKT_glob.attrs.verbosity_levels[type];
}

rkt_fun bool RKT_shall_print(const RKT_TestEntry* e, RK_OutPutStreams s) {
  RK_Verbosity v = RKT_resolve_verbosity(e, s);
  if (v == RK_VERBOSITY_NEVER) { return false; }
  switch (s) {
  case RK_OUTPUTSTREAMS_STDOUT:
    return e->res.capt[s].len > 0
        && (e->res.ended != RKT_PASSED || v == RK_VERBOSITY_ALWAYS);
  case RK_OUTPUTSTREAMS_STDERR: return e->res.capt[s].len > 0;
  case RK_OUTPUTSTREAMS_LOG   : return true;
  }
  RKT_unreachable();
}

RKT_noreturn rkt_fun void RKT_fatal(const char* str) {
#ifndef _WIN32
  perror(str);
  fflush(rk_null);
  if (RKT_glob.pid) {
    sigset_t mask;
    sigemptyset(&mask), sigaddset(&mask, SIGTERM);
    sigprocmask(SIG_BLOCK, &mask, rk_null);
    kill(0, SIGTERM); // Kill all children (not parent?)
  }
  abort(); // Abort for core dump
#else
  char* buf = rk_null;
  DWORD len = FormatMessageA(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
          | FORMAT_MESSAGE_IGNORE_INSERTS,
      rk_null, GetLastError(), 0, (LPSTR)&buf, 0, rk_null);
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

rkt_fun RKT_Exit RKT_reapchild(void) {
  RKT_Exit r;
#ifndef _WIN32
  int status;
  while (waitpid(RKT_glob.pid, &status, 0) == -1) {
    if (errno != EINTR) { RKT_fatal("waitpid"); }
  }
  RKT_glob.pid = 0;
  if (WIFSIGNALED(status)) {
    r.type = RKT_EXIT_FAULT;
    switch (WTERMSIG(status)) {
    case SIGSEGV:
    case SIGBUS : r.reason = RKT_FAULT_MEMORY; break;
    case SIGILL : r.reason = RKT_FAULT_ILLEGAL_OP; break;
    case SIGABRT: r.reason = RKT_FAULT_ABORT; break;
    case SIGFPE : r.reason = RKT_FAULT_ARITHMETIC; break;
    case SIGTRAP: r.reason = RKT_FAULT_BREAKPOINT; break;
    default     : r.reason = RKT_FAULT_UNKNOWN; break;
    }
  } else {
    r.type = RKT_EXIT_EXITED, r.exit_code = WEXITSTATUS(status);
  }
#else
  DWORD status;
  WaitForSingleObject(RKT_glob.pid.hProcess, INFINITE);
  GetExitCodeProcess(RKT_glob.pid.hProcess, &status);
  CloseHandle(RKT_glob.pid.hProcess), CloseHandle(RKT_glob.pid.hThread);
  RKT_glob.pid.hProcess = rk_null;
  if (status >= 0x80000000) {
    r.type = RKT_EXIT_FAULT;
    switch (status) {
    case EXCEPTION_ACCESS_VIOLATION:
    case EXCEPTION_STACK_OVERFLOW       : r.reason = RKT_FAULT_MEMORY; break;
    case EXCEPTION_ILLEGAL_INSTRUCTION  : r.reason = RKT_FAULT_ILLEGAL_OP; break;
    case EXCEPTION_FLT_INVALID_OPERATION:
    case EXCEPTION_INT_DIVIDE_BY_ZERO   :
    case EXCEPTION_FLT_DIVIDE_BY_ZERO   : r.reason = RKT_FAULT_ARITHMETIC; break;
    case EXCEPTION_BREAKPOINT           : r.reason = RKT_FAULT_BREAKPOINT; break;
    case STATUS_FATAL_APP_EXIT          : r.reason = RKT_FAULT_ABORT; break;
    default                             : r.reason = RKT_FAULT_UNKNOWN; break;
    }
  } else {
    r.type = RKT_EXIT_EXITED, r.exit_code = status;
  }
#endif
  return r;
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

rkt_fun void RKT_send_hdr(int line, RKT_TestAssertFunction f, int sig,
                          const char* str, size_t len) {
  RKT_AssertHdr hdr;
  hdr.pos = line, hdr.exit_code = sig, hdr.F = f;
  hdr.expr.len = (unsigned char)RKT_MIN(len, sizeof(hdr.expr.str) - 1);
  memcpy(hdr.expr.str, str, hdr.expr.len), hdr.expr.str[hdr.expr.len] = '\0';
  if (RKT_write_full(RKT_glob.log, &hdr, sizeof(hdr)) < 1) {
    RKT_fatal("CHILD SEND HDR");
  }
}
#define RKT_SEND_HDR(ESTR, FUNENUM, SIG)                                       \
  RKT_send_hdr(__LINE__, FUNENUM, SIG, ESTR, RKT_lenof(ESTR))

rkt_fun size_t RKT_send_res(const RKT_AssertRes* buf) {
  if (RKT_write_full(RKT_glob.log, buf, sizeof(*buf) + buf->len) < 1) {
    RKT_fatal("CHILD SEND RES");
  }
  return buf->res;
}
#define RKT_SEND_RES0(RES)                                                     \
  (RKT_glob_buf.pkg.res = (RES), RKT_glob_buf.pkg.len = 0,                     \
   RKT_send_res(&RKT_glob_buf.pkg))
#define RKT_SEND_RES1(RES, E1, FMT1)                                           \
  (RKT_glob_buf.pkg.res = (RES),                                               \
   RKT_glob_buf.pkg.len = ((RKT_print_passes || RKT_glob_buf.pkg.res)          \
                               ? sprintf(RKT_glob_buf.pkg.args, FMT1, E1) + 1  \
                               : 0),                                           \
   RKT_send_res(&RKT_glob_buf.pkg))
#define RKT_SEND_RES2(RES, E1, FMT1, E2, FMT2)                                 \
  (RKT_glob_buf.pkg.res = (RES),                                               \
   RKT_glob_buf.pkg.len                                                        \
   = ((RKT_print_passes || RKT_glob_buf.pkg.res)                               \
          ? sprintf(RKT_glob_buf.pkg.args, FMT1 "%c" FMT2, E1, '\0', E2) + 1   \
          : 0),                                                                \
   RKT_send_res(&RKT_glob_buf.pkg))
#define RKT_SEND_RES3(RES, E1, FMT1, E2, FMT2, E3, FMT3)                       \
  (RKT_glob_buf.pkg.res = (RES),                                               \
   RKT_glob_buf.pkg.len                                                        \
   = ((RKT_print_passes || RKT_glob_buf.pkg.res)                               \
          ? sprintf(RKT_glob_buf.pkg.args, FMT1 "%c" FMT2 "%c" FMT3, E1, '\0', \
                    E2, '\0', E3)                                              \
                + 1                                                            \
          : 0),                                                                \
   RKT_send_res(&RKT_glob_buf.pkg))

// todo better win32 bug detection
#ifndef _WIN32
# define RKT_SCHAR_CASE(X, ...) X(__VA_ARGS__)
#else
# define RKT_SCHAR_CASE(X, ...)
#endif
#define RKT_TYPELIST_NOFLOAT(X, ...)                                           \
  X(unsigned char, uc, "%hhu", ##__VA_ARGS__)                                  \
  X(unsigned short, us, "%hu", ##__VA_ARGS__)                                  \
  X(unsigned, ui, "%u", ##__VA_ARGS__)                                         \
  X(unsigned long, ul, "%lu", ##__VA_ARGS__)                                   \
  X(unsigned long long, ull, "%llu", ##__VA_ARGS__)                            \
  X(char, c, "%c", ##__VA_ARGS__)                                              \
  X(short, s, "%hd", ##__VA_ARGS__)                                            \
  X(int, i, "%d", ##__VA_ARGS__)                                               \
  X(long, sl, "%ld", ##__VA_ARGS__)                                            \
  X(long long, sll, "%lld", ##__VA_ARGS__)                                     \
  X(bool, b, "%d", ##__VA_ARGS__)                                              \
  X(const void*, vp, "%p", ##__VA_ARGS__)                                      \
  RKT_SCHAR_CASE(X, signed char, sc, "%hhd", ##__VA_ARGS__)
#define RKT_TYPELIST_FLOAT(X, ...)                                             \
  X(float, f, "%f", ##__VA_ARGS__)                                             \
  X(double, d, "%f", ##__VA_ARGS__)                                            \
  X(long double, ld, "%Lf", ##__VA_ARGS__)
#define RKT_TYPELIST(X, ...)                                                   \
  RKT_TYPELIST_FLOAT(X, ##__VA_ARGS__)                                         \
  RKT_TYPELIST_NOFLOAT(X, ##__VA_ARGS__)

#ifdef __cplusplus
# if __cpp_lib_format >= 201907L /* todo */
#  define RKT_HAS_FORMAT 1
template <typename T, typename CharT = char>
concept RKT_formattable = requires (const T& value) {
  std::formatter<std::remove_cvref_t<T>, char>{};
};
# else
#  define RKT_HAS_FORMAT 0
# endif

class RK_TestStream : public std::ostream {
public:
  RK_TestStream() : std::ostream(&streambuf) {}
  template <class T>
  void add_arg(const T& val) {
    add_arg_impl<T>(val, streamable<T>{});
  }
  size_t send(size_t res_val) {
    RKT_AssertRes* res = (RKT_AssertRes*)streambuf.buf;
    res->res = res_val, res->len = streambuf.len - sizeof(RKT_AssertRes);
    (void)RKT_send_res(res);
    if (streambuf.cap > streambuf.mem_size) { free(streambuf.buf); }
    streambuf.buf = RKT_glob_buf.storage;
    streambuf.len = streambuf.hdr_size;
    streambuf.cap = streambuf.mem_size;
    return res_val;
  }

private:
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

  protected:
    static const size_t hdr_size{sizeof(RKT_glob_buf.pkg)};
    static const size_t mem_size{sizeof(RKT_glob_buf.storage)};
    char*               buf{RKT_glob_buf.storage};
    size_t              len{hdr_size}, cap{mem_size};
    void                grow_to(size_t ncap) {
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
#endif

#ifndef __cplusplus
/// @brief name of the type-specific function (in c++ via overloads)
# define RKT_namefunc(FNAME, N)        RKTf_##FNAME##_##N
# define RKT_SELFUN_(T, N, FMT, FNAME) , T : RKT_namefunc(FNAME, N)
# define RKT_SELFUN(FNAME, EXPR)                                               \
   _Generic((EXPR)RKT_TYPELIST(RKT_SELFUN_, FNAME),                            \
       char*: RKTf_streq,                                                      \
       const char*: RKTf_streq,                                                \
       default: RKTf_##FNAME##_vp)
# define RKT_SELFUN_FLOAT(FNAME, EXPR)                                         \
   _Generic((EXPR)RKT_TYPELIST_FLOAT(RKT_SELFUN_, FNAME))
#else
/// @brief name of the type-specific function (in c++ via overloads)
# define RKT_namefunc(FNAME, N)        RKTf_##FNAME
# define RKT_SELFUN(FNAME, EXPR)       RKT_namefunc(FNAME, )
# define RKT_SELFUN_FLOAT(FNAME, EXPR) RKT_SELFUN(FNAME, EXPR)
// todo check float overloads
#endif

#define RKT_GEN_f2(T, N, FMT, FNAME, EXPR)                                     \
  rkt_fun size_t RKT_namefunc(FNAME, N)(T e1, T e2) {                          \
    return RKT_SEND_RES2((EXPR), e1, FMT, e2, FMT);                            \
  }
#define RKT_GEN_f3(T, N, FMT, FNAME, EXPR)                                     \
  rkt_fun size_t RKT_namefunc(FNAME, N)(T e1, T e2, T e3) {                    \
    return RKT_SEND_RES3((EXPR), e1, FMT, e2, FMT, e3, FMT);                   \
  }
#ifdef __cplusplus
# define RKT_GEN_f2TMPLATE(FNAME, EXPR)                                        \
   template <class T, class U>                                                 \
   rkt_fun size_t RKT_namefunc(FNAME, )(T e1, U e2) {                          \
     size_t res = (size_t)(EXPR);                                              \
     if (RKT_print_passes || res) {                                            \
       RK_TestStream stream;                                                   \
       stream.add_arg(e1), stream.add_arg(e2);                                 \
       return stream.send(res);                                                \
     }                                                                         \
     return RKT_SEND_RES0(res);                                                \
   }
# define RKT_GEN_fargs_tmplate(X)                                              \
   X(eq, !(e1 == e2))                                                          \
   X(neq, !(e1 != e2))                                                         \
   X(gt, (e1 > e2 ? 0 : (e1 == e2 ? 1 : 2)))                                   \
   X(geq, !(e1 >= e2))                                                         \
   X(lt, (e1 < e2 ? 0 : (e1 == e2 ? 1 : 2)))                                   \
   X(leq, !(e1 <= e2))
RKT_GEN_fargs_tmplate(RKT_GEN_f2TMPLATE) // clang-format off
#undef RKT_GEN_f2TMPLATE
#undef RKT_GEN_fargs_tmplate
// In order to prioritise streq over the generic template in C++
rkt_fun size_t RKTf_streq(const char*, const char*);
rkt_fun size_t RKTf_strneq(const char*, const char*);
# define RKT_TYPELIST_STRINGS_TMP(X)                                           \
   X(const char*, const char*)                                                 \
   X(const char*, char*) X(char*, const char*) X(char*, char*)
# define RKT_GEN_str(C1, C2)                                                   \
   rkt_fun size_t RKTf_eq(C1 e1, C2 e2) { return RKTf_streq(e1, e2); }         \
   rkt_fun size_t RKTf_neq(C1 e1, C2 e2) { return RKTf_strneq(e1, e2); }
RKT_TYPELIST_STRINGS_TMP(RKT_GEN_str)
# undef RKT_TYPELIST_STRINGS_TMP
# undef RKT_GEN_str // clang-format on 
#endif

#define RKTf_eq(exp, act)            RKT_SELFUN(eq, exp)(exp, act)
#define RKTf_neq(exp, act)           RKT_SELFUN(neq, exp)(exp, act)
#define RKTf_lt(exp, act)            RKT_SELFUN(lt, exp)(exp, act)
#define RKTf_leq(exp, act)           RKT_SELFUN(leq, exp)(exp, act)
#define RKTf_gt(exp, act)            RKT_SELFUN(gt, exp)(exp, act)
#define RKTf_geq(exp, act)           RKT_SELFUN(geq, exp)(exp, act)
#define RKTf_inrange(val, low, high) RKT_SELFUN(inrange, val)(val, low, high)
#define RKTf_floateq_tol(exp, act, tol)                                        \
  RKT_SELFUN_FLOAT(floateq_tol, (exp + act + tol))(exp, act, tol)
#define RKTf_floatneq_tol(exp, act, tol)                                       \
  RKT_SELFUN_FLOAT(floatneq_tol, (exp + act + tol))(exp, act, tol)

// clang-format off
// all functions that apply to floats and nonfloats
#define RKT_GEN_fargs_all(T, N, FMT, ...)                                       \
  RKT_GEN_f2(T, N, FMT, lt, (e1 < e2 ? 0 : (e1 == e2 ? 1 : 2)))                 \
  RKT_GEN_f2(T, N, FMT, leq, !(e1 <= e2))                                       \
  RKT_GEN_f2(T, N, FMT, gt, (e1 > e2 ? 0 : (e1 == e2 ? 1 : 2)))                 \
  RKT_GEN_f2(T, N, FMT, geq, !(e1 >= e2))                                       \
  RKT_GEN_f3(T, N, FMT, inrange, (e1 < e2 ? 1 : (e1 > e3 ? 2 : 0)))
RKT_TYPELIST(RKT_GEN_fargs_all)
#undef RKT_GEN_fargs_all

// all functions that apply to all but non-float types
#define RKT_GEN_fargs_nofloat(T, N, FMT, ...)                                   \
  RKT_GEN_f2(T, N, FMT, eq, !(e1 == e2))                                        \
  RKT_GEN_f2(T, N, FMT, neq, !(e1 != e2))
RKT_TYPELIST_NOFLOAT(RKT_GEN_fargs_nofloat)
#undef RKT_GEN_fargs_nofloat

#define RKT_TYPELIST_FLOAT_EXTENDED(X, ...)                                     \
  X(float, f, "%f", 1e-6f, fabsf, ##__VA_ARGS__)                                \
  X(double, d, "%f", 1e-12, fabs, ##__VA_ARGS__)                                \
  X(long double, ld, "%Lf", 1e-12L, fabsl, ##__VA_ARGS__)
// float-only implementations/functions
#define RKT_GEN_fargs_float(T, N, FMT, eps_default, fabs_fn)                    \
  RKT_GEN_f2(T, N, FMT, eq, !(fabs_fn(e1 - e2) <= eps_default))                 \
  RKT_GEN_f2(T, N, FMT, neq, !(fabs_fn(e1 - e2) > eps_default))                 \
  RKT_GEN_f3(T, N, FMT, floateq_tol, !(fabs_fn(e1 - e2) <= e3))                 \
  RKT_GEN_f3(T, N, FMT, floatneq_tol, !(fabs_fn(e1 - e2) > e3))
RKT_TYPELIST_FLOAT_EXTENDED(RKT_GEN_fargs_float)
#undef RKT_GEN_fargs_float

// clang-format on
rkt_fun size_t RKTf_true(bool e1) { return RKT_SEND_RES0(!e1); }
rkt_fun size_t RKTf_false(bool e1) { return RKT_SEND_RES0(!!e1); }

rkt_fun size_t RKTf_null(const void* e1) {
  return RKT_SEND_RES1(!(!e1), e1, "%p");
}
rkt_fun size_t RKTf_nonnull(const void* e1) {
  return RKT_SEND_RES1(!(e1), e1, "%p");
}

rkt_fun size_t RKTf_memzero(const void* e1, size_t e2) {
  size_t dif = 0;
  for (size_t i = 0; i < e2; ++i) {
    if (((const char*)e1)[i]) {
      dif = i + 1;
      break;
    }
  }
  return RKT_SEND_RES2(dif, e1, "%p", e2, "%zu");
}

rkt_fun size_t RKTf_memnzero(const void* e1, size_t e2) {
  const void* l;
  size_t r = (!e2 || !(l = memchr(e1, 0, e2))) ? 0 : ((char*)l - (char*)e1 + 1);
  return RKT_SEND_RES2(r, e1, "%p", e2, "%zu");
}

#define RKT_catstr(S, STR, LEN) (memcpy(S, STR, LEN), LEN)

/// @brief returns first position where strings differ +1 (0 if same)
/// if both strings are length 0, they're equal if both rk_null or both
/// non-rk_null, otherwise differ at pos 1
rkt_fun size_t RKT_memdiff(const void* e1, size_t l1, const void* e2,
                           size_t l2) {
  size_t min = RKT_MIN(l1, l2);
  for (size_t i = 0; i < min; ++i) {
    if (((const char*)e1)[i] != ((const char*)e2)[i]) { return i + 1; }
  }
  return l1 == l2 ? (min == 0 ? !((bool)e1 == (bool)e2) : 0) : (min + 1);
}

rkt_fun size_t RKT_strn_compare_impl_send(RKT_AssertRes             tmp,
                                          const char** RKT_restrict es,
                                          size_t* RKT_restrict      ls) {
  if (RKT_print_passes || tmp.res) {
    for (size_t i = 0; i < 2; ++i) {
      es[i] = es[i] ? es[i] : (ls[i] = RKT_lenof("(null)"), "(null)");
    }
    tmp.len = ls[0] + ls[1] + 2;
    RKT_AssertRes* res;
    if (sizeof(*res) + tmp.len > sizeof(RKT_glob_buf.storage)) {
      if (!(res = (RKT_AssertRes*)malloc(sizeof(*res) + tmp.len))) { exit(12); }
    } else {
      res = &RKT_glob_buf.pkg;
    }
    *res    = tmp;
    char* s = res->args;
    for (size_t i = 0; i < 2; ++i) {
      if (ls[i]) { s += RKT_catstr(s, es[i], ls[i]); }
      *s++ = '\0';
    }
    RKT_send_res(res);
    if (res != &RKT_glob_buf.pkg) { free(res); }
    return tmp.res;
  }
  tmp.len = 0;
  return RKT_send_res(&tmp);
}

/*
If want equal:
- Success: 0
- Failure: Return first position of difference
           In case of one being rk_null 1
else
- Success: 0
- Failure: 1
If failure/send_success, we send string "(null)" for rk_null pointers
*/
rkt_fun size_t RKTf_streq_impl(bool want_equal, const char* e1,
                               const char* e2) {
  RKT_AssertRes tmp;
  size_t        ls[2];
  const char*   es[2] = {e1, e2};
  for (size_t i = 0; i < 2; ++i) { ls[i] = es[i] ? strlen(es[i]) : 0; }
  size_t dif = RKT_memdiff(e1, ls[0], e2, ls[1]);
  tmp.res    = want_equal ? dif : !(dif != 0);
  return RKT_strn_compare_impl_send(tmp, es, ls);
}

rkt_fun size_t RKTf_streq(const char* e1, const char* e2) {
  return RKTf_streq_impl(1, e1, e2);
}
rkt_fun size_t RKTf_strneq(const char* e1, const char* e2) {
  return RKTf_streq_impl(0, e1, e2);
}

/// @brief To check if two strings are equal up to a length n. If both are
/// rk_null, they're equal, if one is rk_null and length is 0, they're not equal
/// todo create macro for upto? what api could I use for that
rkt_fun size_t RKTf_streq_n_impl(bool want_equal, const char* e1,
                                 const char* e2, size_t n) {
  size_t      ls[2]; // find out actual lengths here
  const char* es[2] = {e1, e2};
  for (size_t i = 0; i < 2; ++i) {
    size_t _l = es[i] ? strlen(es[i]) : 0;
    ls[i]     = RKT_MIN(_l, n);
  }
  RKT_AssertRes tmp;
  size_t dif = RKT_memdiff(es[0], ls[0], es[1], ls[1]); // todo check semantics
  tmp.res    = want_equal ? dif : !(dif != 0);
  return RKT_strn_compare_impl_send(tmp, es, ls);
}
rkt_fun size_t RKTf_streq_n(const char* e1, const char* e2, size_t e3) {
  return RKTf_streq_n_impl(1, e1, e2, e3);
}
rkt_fun size_t RKTf_strneq_n(const char* e1, const char* e2, size_t e3) {
  return RKTf_streq_n_impl(0, e1, e2, e3);
}

rkt_fun size_t RKT_strn_compare_impl_exact(bool want_equal, const char* e1,
                                           size_t l1, const char* e2,
                                           size_t l2) {
  RKT_AssertRes tmp;
  size_t        ls[2] = {l1, l2};
  const char*   es[2] = {e1, e2};
  size_t        dif   = RKT_memdiff(e1, ls[0], e2, ls[1]);
  tmp.res             = want_equal ? dif : !(dif != 0);
  return RKT_strn_compare_impl_send(tmp, es, ls);
}
// todo macro for this
rkt_fun size_t RKTf_stream_startswith_impl(RK_OutPutStreams stream,
                                           bool want_equal, const char* str,
                                           size_t len) {
  fflush(rk_null);
  // RKT_assert(str || !len); //todo
  RKT_Strv o   = RKT_readfile(RKT_glob.tmpfds[stream]);
  size_t   res = RKT_strn_compare_impl_exact(want_equal, str, len, o.str,
                                             RKT_MIN(len, o.len));
  free(o.str);
  return res;
}
rkt_fun size_t RKTf_stdout_starts_with(const char* str, size_t len) {
  return RKTf_stream_startswith_impl(RK_OUTPUTSTREAMS_STDOUT, 1, str, len);
}
rkt_fun size_t RKTf_stdout_nstarts_with(const char* str, size_t len) {
  return RKTf_stream_startswith_impl(RK_OUTPUTSTREAMS_STDOUT, 0, str, len);
}
rkt_fun size_t RKTf_stderr_starts_with(const char* str, size_t len) {
  return RKTf_stream_startswith_impl(RK_OUTPUTSTREAMS_STDERR, 1, str, len);
}
rkt_fun size_t RKTf_stderr_nstarts_with(const char* str, size_t len) {
  return RKTf_stream_startswith_impl(RK_OUTPUTSTREAMS_STDERR, 0, str, len);
}

rkt_fun size_t RKTf_stream_endswith_impl(RK_OutPutStreams stream,
                                         bool want_equal, const char* str,
                                         size_t len) {
  fflush(rk_null);
  // RKT_assert(str || !len); //todo
  RKT_Strv o = RKT_readfile(RKT_glob.tmpfds[stream]);
  RKT_Strv endstr;
  if (len > o.len) {
    endstr.str = o.str, endstr.len = o.len;
  } else {
    endstr.str = o.str + o.len - len, endstr.len = len;
  }
  size_t res = RKT_strn_compare_impl_exact(want_equal, str, len, endstr.str,
                                           endstr.len);
  free(o.str);
  return res;
}
rkt_fun size_t RKTf_stdout_ends_with(const char* str, size_t len) {
  return RKTf_stream_endswith_impl(RK_OUTPUTSTREAMS_STDOUT, 1, str, len);
}
rkt_fun size_t RKTf_stdout_nends_with(const char* str, size_t len) {
  return RKTf_stream_endswith_impl(RK_OUTPUTSTREAMS_STDOUT, 0, str, len);
}
rkt_fun size_t RKTf_stderr_ends_with(const char* str, size_t len) {
  return RKTf_stream_endswith_impl(RK_OUTPUTSTREAMS_STDERR, 1, str, len);
}
rkt_fun size_t RKTf_stderr_nends_with(const char* str, size_t len) {
  return RKTf_stream_endswith_impl(RK_OUTPUTSTREAMS_STDERR, 0, str, len);
}

rkt_fun size_t RKTf_streameq(RK_OutPutStreams stream, bool want_equal,
                             const char* str, size_t len) {
  fflush(rk_null);
  // TODO assert
  // RKT_assert(str || !len);
  RKT_Strv o = RKT_readfile(RKT_glob.tmpfds[stream]);
  size_t res = RKT_strn_compare_impl_exact(want_equal, str, len, o.str, o.len);
  free(o.str);
  return res;
}

rkt_fun size_t RKTf_stdouteq(const char* str, size_t len) {
  return RKTf_streameq(RK_OUTPUTSTREAMS_STDOUT, 1, str, len);
}
rkt_fun size_t RKTf_stdoutneq(const char* str, size_t len) {
  return RKTf_streameq(RK_OUTPUTSTREAMS_STDOUT, 0, str, len);
}
rkt_fun size_t RKTf_stderreq(const char* str, size_t len) {
  return RKTf_streameq(RK_OUTPUTSTREAMS_STDERR, 1, str, len);
}
rkt_fun size_t RKTf_stderrneq(const char* str, size_t len) {
  return RKTf_streameq(RK_OUTPUTSTREAMS_STDERR, 0, str, len);
}

rkt_fun size_t RKTf_memeq_impl(bool want_equal, const void* e1, const void* e2,
                               size_t e3) {
  size_t res = !(!e3 || !memcmp(e1, e2, e3));
  if (!want_equal) { res = 1 - res; }
  return RKT_SEND_RES3(res, e1, "%p", e2, "%p", e3, "%zu");
}
rkt_fun size_t RKTf_memeq(const void* e1, const void* e2, size_t e3) {
  return RKTf_memeq_impl(1, e1, e2, e3);
}
rkt_fun size_t RKTf_memneq(const void* e1, const void* e2, size_t e3) {
  return RKTf_memeq_impl(0, e1, e2, e3);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////PROGRAM LOGIC///////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#ifndef _WIN32
rkt_fun void RKT_handle_sigterm(int sigterm) {
  // Sigterm does not automatically propagate to child processes in a process
  // group, so we manually forward it to all processes in the group.
  kill(0, sigterm);
  _exit(1); // TODO exit code
}
rkt_fun void RKT_handle_sigalarm(int sigalarm) {
  siglongjmp(RKT_glob.fret_sig, sigalarm);
}
#endif

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
  for (; nbytes;) {
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
    ssize_t r = read(fd, ptr, nbytes);
    switch (r) {
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
      long remaining = deadline - (long)RKT_now_ms();
      if (remaining <= 0) { return -3; }
      DWORD avail = 0;
      if (!PeekNamedPipe(fd, rk_null, 0, rk_null, &avail, rk_null)) {
        DWORD err = GetLastError();
        if (err == ERROR_BROKEN_PIPE) { return (ptr == (char*)buf) ? 0 : -2; }
        return -1;
      }
      if (avail == 0) {
        // Nothing readable yet: sleep a bit but respect deadline.
        DWORD snooze = (remaining > 5) ? 1 : (DWORD)remaining; // 0..1ms-ish
        if (snooze) {
          Sleep(snooze);
        } else {
          Sleep(0);
        } // yield
        continue;
      }
      // avail > 0 => fall through to ReadFile below
    }
    DWORD chunk = (DWORD)((nbytes > 0xFFFFFFFFUL) ? 0xFFFFFFFFUL : nbytes);
    if (!ReadFile(fd, ptr, chunk, &r, rk_null)) {
      switch (GetLastError()) {
      case ERROR_BROKEN_PIPE: return (ptr == (char*)buf) ? 0 : -2;
      default               : return -1;
      }
    }
    if (!r) { return ((void*)ptr == buf) ? 0 : -2; }
    ptr += r, nbytes -= r;
  }
#endif
  return 1; // read all, no eof
}

rkt_fun int RKT_read_full(rkt_fd fd, void* buf, size_t nbytes) {
  char* ptr = (char*)buf;
#ifndef _WIN32
  for (; nbytes;) {
    ssize_t r = read(fd, ptr, nbytes);
    switch (r) {
    case -1:
      if (errno != EINTR) { return -1; }
      break;
    case 0 : return (void*)ptr == buf ? 0 : -2; // eof
    default: ptr += r, nbytes -= r;
    }
  }
#else
  for (DWORD r; nbytes;) {
    DWORD chunk = (DWORD)((nbytes > 0xFFFFFFFFUL) ? 0xFFFFFFFFUL : nbytes);
    if (!ReadFile(fd, ptr, chunk, &r, rk_null)) {
      switch (GetLastError()) {
      case ERROR_BROKEN_PIPE: return (ptr == (char*)buf) ? 0 : -2;
      default               : return -1;
      }
    }
    if (!r) { return ((void*)ptr == buf) ? 0 : -2; }
    ptr += r, nbytes -= r;
  }
#endif
  return 1; // read all, no eof
}
rkt_fun int RKT_write_full(rkt_fd fd, const void* buf, size_t len) {
  const char* ptr = (const char*)buf;
#ifndef _WIN32
  for (; len;) {
    ssize_t w = write(fd, ptr, len);
    if (w == -1) {
      if (errno != EINTR) { return -1; }
    } else {
      len -= (size_t)w, ptr += w;
    }
  }
#else
  for (DWORD w = 0; len > 0; ptr += (size_t)w, len -= (size_t)w) {
    DWORD chunk = (DWORD)((len > 0xFFFFFFFFULL) ? 0xFFFFFFFFUL : (DWORD)len);
    if (!WriteFile(fd, ptr, chunk, &w, rk_null)) { return -1; }
    if (w == 0) { return -1; }
  }
#endif
  return 1;
}

rkt_fun void RKT_rewindfile(rkt_fd fd) {
#ifndef _WIN32
  if (lseek(fd, 0, SEEK_SET) == (off_t)-1) { RKT_fatal("lseek"); }
#else
  LARGE_INTEGER liDistance = {RKT_ZINIT};
  if (!SetFilePointerEx(fd, liDistance, rk_null, FILE_BEGIN)) {
    RKT_fatal("SetFilePointerEx");
  }
#endif
}

rkt_fun RKT_Strv RKT_readfile(rkt_fd fd) {
  RKT_Strv res = {rk_null, 0};
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
  if (res.len == 0) { return res; }
  if (!(res.str = RKT_malloc(char, res.len))) { RKT_fatal("malloc read fd"); }
  switch (RKT_read_full(fd, res.str, res.len)) {
  case -2: RKT_fatal("readfile unexpected EOF");
  case -1: RKT_fatal("readfile error");
  case 0 : RKT_fatal("readfile unexpected EOF");
  case 1 : break;
  }
  return res;
}

rkt_fun void RKT_init_fds(void) {
  if (!RKT_glob.tmpfds[RK_OUTPUTSTREAMS_LOG]) {
#ifndef _WIN32
    if ((RKT_glob.saved[RK_OUTPUTSTREAMS_STDOUT] = dup(STDOUT_FILENO)) < 0) {
      RKT_fatal("dup sout");
    }
    if ((RKT_glob.saved[RK_OUTPUTSTREAMS_STDERR] = dup(STDERR_FILENO)) < 0) {
      RKT_fatal("dup serr");
    }
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
                         &RKT_glob.saved[RK_OUTPUTSTREAMS_STDOUT], 0, TRUE,
                         DUPLICATE_SAME_ACCESS)) {
      RKT_fatal("dup sout");
    }
    if (!DuplicateHandle(proc, GetStdHandle(STD_ERROR_HANDLE), proc,
                         &RKT_glob.saved[RK_OUTPUTSTREAMS_STDERR], 0, TRUE,
                         DUPLICATE_SAME_ACCESS)) {
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
          FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, rk_null);
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
    FILE* const default_files[3] = {stdout, rk_null, rk_null};
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
#ifndef _WIN32
  struct sigaction sa = {RKT_ZINIT};
  setpgid(0, 0);
  sa.sa_handler = RKT_handle_sigterm, sigemptyset(&sa.sa_mask);
  sigaddset(&sa.sa_mask, SIGINT);
  sigaddset(&sa.sa_mask, SIGQUIT);
  sigaddset(&sa.sa_mask, SIGTSTP);
  sigaction(SIGTERM, &sa, rk_null);
#else
  RKT_glob.job = CreateJobObject(rk_null, rk_null);
  if (!AssignProcessToJobObject(RKT_glob.job, GetCurrentProcess())) { abort(); }
#endif
}

rkt_fun void RKT_run_all(const char** suites_strs, size_t nsuites_strs,
                         RK_CustomTestAttributes attrs) {
  if (!suites_strs && nsuites_strs) {
    RKT_fatal("rk_null suites_strs requires zero nsuites_strs");
  }
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

rkt_fun void RKT_run_suite(RKT_Suite* RKT_restrict s) {
  // printf("RUNNING SUITE %s\n", s->name.str); // for debugging
  if (s->attrs.init) { s->attrs.init(); }
  size_t total = 0, failed = 0, crashed = 0, timed_out = 0, error = 0;
  for (RKT_TestEntry* e = s->tests; e; e = e->next) {
    if (!RKT_glob.attrs.tags) { goto run_test; } // todo local tags
    if (!e->attrs.tags) { continue; }
    const char *s1, *e1;
    for (s1 = RKT_glob.attrs.tags, s1 += (*s1 == '"'); *s1;
         s1 = *e1 ? e1 + 1 : e1) {
      for (e1 = s1; *e1 && *e1 != ',' && *e1 != '"'; ++e1);
      const char *s2, *e2;
      for (s2 = e->attrs.tags, s2 += (*s2 == '"'); *s2;
           s2 = *e2 ? e2 + 1 : e2) {
        for (e2 = s2; *e2 && *e2 != ',' && *e2 != '"'; ++e2);
        if (e1 - s1 == e2 - s2 && !strncmp(s2, s1, e1 - s1)) { goto run_test; }
      }
    }
    continue;
  run_test:
    RKT_run_test(e);
    switch (e->res.ended) {
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

rkt_fun void RKT_run_test(RKT_TestEntry* e) {
  if (RKT_resolve_isolation(e) == false) {
    int timed_out = RKT_init_test_noisolation(e);
    RKT_parent_loop_noisolation(e, timed_out);
  } else {
    RKT_init_test_isolation(e);
    RKT_parent_loop_isolation(e);
    RKT_closeHandle(RKT_glob.log);
  }
  e->res.capt[RK_OUTPUTSTREAMS_STDOUT]
      = RKT_readfile(RKT_glob.tmpfds[RK_OUTPUTSTREAMS_STDOUT]);
  e->res.capt[RK_OUTPUTSTREAMS_STDERR]
      = RKT_readfile(RKT_glob.tmpfds[RK_OUTPUTSTREAMS_STDERR]);
  ++RKT_glob.tests_run;
  RKT_print_test_tap(e);
}

rkt_fun RKT_dword RKT_runfunc(void* arg) {
  const RKT_TestEntry* e = (const RKT_TestEntry*)arg;
#if 0 
  RKT_print_passes
      = RKT_resolve_verbosity(e, RK_OUTPUTSTREAMS_LOG) == RK_VERBOSITY_ALWAYS;
#endif
  int res;
  if (!(res = setjmp(RKT_glob.fret))) { // for failed assertions (no signals)
    e->func();
  } else {
    if (res == RKT_ASSERT_FAILED_CODE) { return 0; }
    return res;
  }
  return 0;
}

rkt_fun int RKT_read_hdr(RKT_AssertHdr* RKT_restrict hdr) {
  return RKT_read_full(RKT_glob.log, hdr, sizeof(*hdr));
}
rkt_fun int RKT_read_hdr_tm(RKT_AssertHdr* RKT_restrict hdr, long ms_left) {
  return RKT_read_full_tm(RKT_glob.log, hdr, sizeof(*hdr), ms_left);
}
rkt_fun int RKT_read_res(RKT_AssertRes** RKT_restrict r) {
  RKT_AssertRes tmp;
  int           res = RKT_read_full(RKT_glob.log, &tmp, sizeof(RKT_AssertRes));
  if (res == 1) {
    *r = (RKT_AssertRes*)malloc(sizeof(tmp) + tmp.len);
    if (!*r) { RKT_fatal("malloc read res"); }
    **r = tmp;
    if (RKT_read_full(RKT_glob.log, (*r)->args, (*r)->len) < 1) { return -2; }
  }
  return res;
}
rkt_fun int RKT_read_res_tm(RKT_AssertRes** RKT_restrict r, long ms) {
  RKT_AssertRes tmp;
  int res = RKT_read_full_tm(RKT_glob.log, &tmp, sizeof(RKT_AssertRes), ms);
  if (res == 1) {
    *r = (RKT_AssertRes*)malloc(sizeof(tmp) + tmp.len);
    if (!*r) { RKT_fatal("malloc read res"); }
    **r = tmp;
    if (RKT_read_full_tm(RKT_glob.log, (*r)->args, (*r)->len, ms) < 1) {
      return -2;
    }
  }
  return res;
}

rkt_fun int RKT_init_test_noisolation(const RKT_TestEntry* RKT_restrict e) {
#ifndef _WIN32
  RKT_redirect_streams(RKT_glob.tmpfds[RK_OUTPUTSTREAMS_LOG]);
  if (e->attrs.init) { e->attrs.init(); }
  int  timed_out = 0;
  long ts        = RKT_resolve_timeout(e);
  if (ts != -1) {
    timed_out = sigsetjmp(RKT_glob.fret_sig, 1);
    if (!timed_out) {
      struct sigaction sa = {RKT_ZINIT};
      sa.sa_handler       = RKT_handle_sigalarm, sigemptyset(&sa.sa_mask);
      sigaction(SIGALRM, &sa, rk_null);
      struct itimerval timer
          = {{0, 0},
             {(suseconds_t)(ts / 1000), (suseconds_t)((ts % 1000) * 1000)}};
      setitimer(ITIMER_REAL, &timer, rk_null);
    }
  }
  if (!timed_out) {
    RKT_dword res = RKT_runfunc((void*)e);
    (void)res; /*todo*/
  }
  struct itimerval timer = {RKT_ZINIT};
  setitimer(ITIMER_REAL, &timer, rk_null);
  struct sigaction sa = {RKT_ZINIT};
  sa.sa_handler       = SIG_DFL, sigemptyset(&sa.sa_mask);
  sigaction(SIGALRM, &sa, rk_null);
  if (e->attrs.fini) { e->attrs.fini(); }
  fflush(rk_null), RKT_restore_streams();
  return timed_out;
#else
  RKT_redirect_streams(RKT_glob.tmpfds[RK_OUTPUTSTREAMS_LOG]);
  if (e->attrs.init) { e->attrs.init(); }
  long  ts        = RKT_resolve_timeout(e);
  int   timed_out = 0;
  DWORD exit_code;
  if (ts != -1) {
    HANDLE t = CreateThread(rk_null, 0, RKT_runfunc, (void*)e, 0, rk_null);
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
  fflush(rk_null), RKT_restore_streams();
  return timed_out;
#endif
}

rkt_fun void RKT_parent_loop_noisolation(RKT_TestEntry* RKT_restrict e,
                                         bool timed_out) {
  size_t          rescap = 512;
  RKT_TestResult* r      = &e->res;
  r->ended               = RKT_PASSED;
  r->results             = RKT_malloc(RKT_AssertDat, rescap);
  if (r->results == rk_null) { RKT_fatal("malloc buf"); }
  int cond = 0;
  for (bool reading_res = 0; cond == 0; reading_res = !reading_res) {
    if (!reading_res) {
      if (r->count + 2 == rescap) { // +2 to simplify timeout code
        rescap     *= 2;
        r->results  = RKT_realloc(RKT_AssertDat, r->results, rescap);
        if (!r->results) { RKT_fatal("realloc 0"); }
      } // todo partial read timeout
      switch (RKT_read_hdr(&r->results[r->count].hdr)) {
      default: RKT_unreachable();
      case -2: ++r->count, r->ended = RKT_TIMEDOUT, cond = 2; break;
      case -1: RKT_fatal("read hdr");
      case 0:
        cond = !timed_out ? 1 : (r->ended = RKT_TIMEDOUT, ++r->count, 3);
        break;
      case 1: ++r->count; break;
      }
    } else {
      switch (RKT_read_res(&r->results[r->count - 1].res)) {
      default: RKT_unreachable();
      case -2: r->ended = RKT_TIMEDOUT, cond = 4; break;
      case -1: RKT_fatal("read res");
      case 0 : r->ended = RKT_TIMEDOUT, cond = 5; break;
      case 1:
        if (r->results[r->count - 1].res->res > 0) {
          ++r->fails, r->ended = RKT_FAILED;
        };
      }
    }
  }
  RKT_finalise_test_noisolation(r, cond);
}

/*
1: partial header read (don't show assert)
2: timeout before next reader could be read (don't show assert)
3: full header read (show assert), no res read
4: full header read (show assert)
5: full header read (show assert)
*/
rkt_fun void RKT_finalise_test_noisolation(RKT_TestResult* r,
                                           int             exit_condition) {
  RKT_AssertDat* dat  = &r->results[r->count - 1];
  r->exit_status.type = RKT_EXIT_DISCARD, r->exit_status.exit_code = 0;
  if (exit_condition >= 2) { // timeout, exit status irrelevant
    if (exit_condition < 4) { dat->hdr = RKT_ZTMP(RKT_AssertHdr); }
    dat->res = (RKT_AssertRes*)malloc(sizeof(*dat->res));
    if (!dat->res) { RKT_fatal("malloc"); }
    dat->res->len = 0, dat->res->res = RKT_ASSERTRES_TIMEOUT;
  }
}

rkt_fun void RKT_init_test_isolation(const RKT_TestEntry* RKT_restrict e) {
#ifndef _WIN32
  fflush(rk_null);
  int pp[2];
  if (pipe(pp) < 0) { RKT_fatal("pipe"); }
  switch ((RKT_glob.pid = fork())) {
  case -1: RKT_fatal("fork");
  case 0 : {
    close(pp[0]), RKT_redirect_streams(pp[1]);
    if (e->attrs.init) { e->attrs.init(); }
    RKT_dword res = RKT_runfunc((void*)e);
    if (e->attrs.fini) { e->attrs.fini(); }
    fflush(rk_null), _exit(res);
  }
  default: close(pp[1]); RKT_glob.log = pp[0];
  }

#else
  SECURITY_ATTRIBUTES sa = {sizeof(sa), rk_null, TRUE};
  rkt_fd              w_end;
  {
    HANDLE h;
    if (!CreatePipe(&h, &w_end, &sa, 0)
        || !SetHandleInformation(h, HANDLE_FLAG_INHERIT, 0)) {
      exit(1);
    }
    RKT_glob.log = h;
  }
  char path[MAX_PATH], cmdline[MAX_PATH * 2 + 4];
  if (!GetModuleFileNameA(rk_null, path, MAX_PATH)) { exit(1); }
  int n = snprintf(cmdline, sizeof(cmdline), "\"%s\"", path);
  if (n < 0 || (size_t)n >= sizeof(cmdline)) {
    // Fallback: extremely long path; treat as fatal for now
    RKT_fatal("CreateProcessA cmdline overflow");
  }
  LPCH   penv = GetEnvironmentStringsA();
  size_t plen = 0;
  if (penv) {
    LPCH p = penv;
    while (*p) { p += strlen(p) + 1; }
    plen = (size_t)((p - penv) + 1); // include the final '\0' (double-null)
  }
  size_t extra = RKT_lenof("RK_CHILD_FN=") + e->name.len + 1
               + RKT_lenof("RKT_WPIPE_LOG=") + 32 + 1
               + RKT_lenof("RKT_TMPFD_OUT=") + 32 + 1
               + RKT_lenof("RKT_TMPFD_ERR=") + 32 + 1 + 1;
  char *nenv = RKT_malloc(char, extra + plen), *s = nenv;
  if (!nenv) { exit(1); }

  // name of the test function to run
  s += RKT_catlit(s, "RK_CHILD_FN=");
  s += RKT_catstr(s, e->name.str, e->name.len), *s++ = '\0';

  // handle for writing (set to global log by child)
  s += sprintf(s, "RKT_WPIPE_LOG=%" PRIuPTR, (uintptr_t)w_end) + 1;

  // handles for reading from own output/error
  s += sprintf(s, "RKT_TMPFD_OUT=%" PRIuPTR,
               (uintptr_t)RKT_glob.tmpfds[RK_OUTPUTSTREAMS_STDOUT])
     + 1;
  s += sprintf(s, "RKT_TMPFD_ERR=%" PRIuPTR,
               (uintptr_t)RKT_glob.tmpfds[RK_OUTPUTSTREAMS_STDERR])
     + 1;

  if (plen) {
    s += (memcpy(s, penv, plen), plen);
  } else {
    *s++ = '\0';
  }
  *s = '\0'; /* ensure double-null terminator */
  STARTUPINFO si;
  ZeroMemory(&si, sizeof(si));
  si.cb         = sizeof(si);
  si.dwFlags    = STARTF_USESTDHANDLES;
  si.hStdOutput = RKT_glob.tmpfds[RK_OUTPUTSTREAMS_STDOUT];
  si.hStdError  = RKT_glob.tmpfds[RK_OUTPUTSTREAMS_STDERR];
  si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
  BOOL worked   = CreateProcessA(path, cmdline, rk_null, rk_null, TRUE, 0, nenv,
                                 rk_null, &si, &RKT_glob.pid);
  if (penv) { FreeEnvironmentStringsA(penv); }
  free(nenv);
  CloseHandle(w_end);
  if (!worked) { exit(1); }
#endif
}

/*
    In isolated mode:                               non-isolated mode:
-3: timeout                                         -
-2: partial read is hard error                      partial read is timeout
-1: error                                           error
0 : EOF                                             may be timeout
1 : all read, success                               all read, success
*/
rkt_fun void RKT_parent_loop_isolation(RKT_TestEntry* RKT_restrict e) {
  size_t          rescap = 512;
  RKT_TestResult* r      = &e->res;
  r->ended               = RKT_PASSED;
  r->results             = RKT_malloc(RKT_AssertDat, rescap);
  if (r->results == rk_null) { RKT_fatal("malloc buf"); }
  long    timeout_ms = RKT_resolve_timeout(e);
  int64_t deadline   = (timeout_ms == -1) ? -1 : RKT_now_ms() + timeout_ms;
  int64_t remaining  = -1;
  int     cond       = -1;
  for (bool reading_res = 0; cond == -1; reading_res = !reading_res) {
    if (deadline != -1) {
      if ((remaining = deadline - RKT_now_ms()) <= 0) { remaining = 0; }
    }
    if (!reading_res) {
      if (r->count + 2 == rescap) { // +2 to simplify timeout code
        rescap     *= 2;
        r->results  = RKT_realloc(RKT_AssertDat, r->results, rescap);
        if (!r->results) { RKT_fatal("realloc 0"); }
      } // todo partial read timeout
      switch (RKT_read_hdr_tm(&r->results[r->count].hdr, remaining)) {
      case -3: r->ended = RKT_TIMEDOUT, ++r->count, cond = 1; break; // to nhdr
      case -2: RKT_fatal("read hdr partial read"); /*partial read is error*/
      case -1: RKT_fatal("read hdr io");           /*other io error*/
      case 0 : cond = 0; break;
      case 1 : ++r->count;
      }
    } else {
      switch (RKT_read_res_tm(&r->results[r->count - 1].res, remaining)) {
      case -3: r->ended = RKT_TIMEDOUT, cond = 2; break; /*timeout, header*/
      case -2: RKT_fatal("read hdr partial read"); /*partial read is error*/
      case -1: RKT_fatal("read res io");           /*other io error*/
      case 0 : cond = 3; break;                     /*crash during run*/
      case 1:
        if (r->results[r->count - 1].res->res) {
          ++r->fails, r->ended = RKT_FAILED;
        }
      }
    }
  }
  RKT_finalise_test_isolation(r, cond);
}

rkt_fun void RKT_finalise_test_isolation(RKT_TestResult* r,
                                         int             exit_condition) {
  /*
  1: timeout before header got read
  2: timeout after header got read
  3: crash after header
  */
  RKT_AssertDat*   dat  = &r->results[r->count - 1];
  RKT_Exit*        exit = &r->exit_status;
  RKT_TestEndType* end  = &r->ended;
  *end                  = RKT_PASSED;
  if (exit_condition == 1 || exit_condition == 2) { // timeout, no exit status
    exit->type = RKT_EXIT_DISCARD, exit->exit_code = 0;
    RKT_KILLCHILD(), (void)RKT_reapchild();
    if (exit_condition == 1) { dat->hdr = RKT_ZTMP(RKT_AssertHdr); }
    if (!(dat->res = (RKT_AssertRes*)malloc(sizeof(*(dat->res))))) {
      RKT_fatal("malloc");
    }
    dat->res->len = 0;
    dat->res->res = RKT_ASSERTRES_TIMEOUT, *end = RKT_TIMEDOUT;
  } else {
    *exit = RKT_reapchild();
    if (exit_condition == 3) { // during assert
      if (!(dat->res = (RKT_AssertRes*)malloc(sizeof(*(dat->res))))) {
        RKT_fatal("malloc");
      }
      dat->res->len = 0;
      if (exit->type == RKT_EXIT_FAULT) { // child crashed
        if (dat->hdr.F != RKTF_crash) {   // unexpectedly
          dat->res->res = RKT_ASSERTRES_CRASH, *end = RKT_CRASHED;
        } else {
          if (dat->hdr.reason != RKT_FAULT_ANY
              && dat->hdr.reason != exit->reason) {
            dat->res->res = 1;
          } else {
            dat->res->res = 0;
          }
        }
      } else {                         // child exited
        if (dat->hdr.F != RKTF_exit) { // unexpectedly
          dat->res->res = RKT_ASSERTRES_EXIT, *end = RKT_EXITED;
        } else {
          if (dat->hdr.exit_code != -1
              && dat->hdr.exit_code != exit->exit_code) {
            dat->res->res = 1;
          } else {
            dat->res->res = 0;
          }
        }
      }
    } else if (exit->type == RKT_EXIT_FAULT
               || (exit->type == RKT_EXIT_EXITED && exit->exit_code)) {
      *end = RKT_TESTERROR; // child exited/crashed during test function
    }
    if (*end == RKT_PASSED && r->fails) { *end = RKT_FAILED; }
  }
}

#ifndef _WIN32
# define RKT_CONSTRUCTOR(fn) __attribute__((constructor)) static void fn(void)
#else
# define RKT_CONSTRUCTOR(fn)                                                   \
   static void fn(void);                                                       \
   __declspec(allocate(".CRT$XCU")) static void (*RKT_CONCAT(fn, _ptr))(void)  \
       = fn;                                                                   \
   static void fn(void)
#endif

#ifndef _WIN32
# define RKT_Windows_Childentry() ((void)0)
#else
rkt_fun void RKT_Windows_Childentry(void) {
  char buf[256];
  if (!GetEnvironmentVariableA("RK_CHILD_FN", buf, sizeof(buf))) { exit(1); }
  // todo this is really slow
  for (RKT_Suite* s = RKT_glob.suites; s; s = s->next) {
    for (RKT_TestEntry* e = s->tests; e; e = e->next) {
      if (strcmp(buf, e->name.str)) { continue; }
      char* ep;
      if (!GetEnvironmentVariableA("RKT_WPIPE_LOG", buf, sizeof(buf))) {
        exit(1);
      }
      RKT_glob.log = (rkt_fd)(uintptr_t)strtoumax(buf, &ep, 10);
      if (*ep != '\0') { exit(1); }

      if (!GetEnvironmentVariableA("RKT_TMPFD_OUT", buf, sizeof(buf))) {
        exit(1);
      }
      RKT_glob.tmpfds[RK_OUTPUTSTREAMS_STDOUT]
          = (rkt_fd)(uintptr_t)strtoumax(buf, &ep, 10);
      if (*ep != '\0') { exit(1); }

      if (!GetEnvironmentVariableA("RKT_TMPFD_ERR", buf, sizeof(buf))) {
        exit(1);
      }
      RKT_glob.tmpfds[RK_OUTPUTSTREAMS_STDERR]
          = (rkt_fd)(uintptr_t)strtoumax(buf, &ep, 10);
      if (*ep != '\0') { exit(1); }

      if (RKT_glob.attrs.init) { RKT_glob.attrs.init(); }
      if (s->attrs.init) { s->attrs.init(); }
      if (e->attrs.init) { e->attrs.init(); }

      RKT_dword res = RKT_runfunc((void*)e);
      fflush(rk_null);
      if (e->attrs.fini) { e->attrs.fini(); }
      if (s->attrs.fini) { s->attrs.fini(); }
      if (RKT_glob.attrs.fini) { RKT_glob.attrs.fini(); }
      exit(res);
    }
  }
  exit(1); /*function not found?*/
}
#endif

rkt_fun void RKT_redirect_streams(rkt_fd new_logfd) {
  RKT_rewindfile(RKT_glob.tmpfds[RK_OUTPUTSTREAMS_STDOUT]);
  RKT_rewindfile(RKT_glob.tmpfds[RK_OUTPUTSTREAMS_STDERR]);
  RKT_rewindfile(RKT_glob.tmpfds[RK_OUTPUTSTREAMS_LOG]);
#ifndef _WIN32
  if (ftruncate(RKT_glob.tmpfds[RK_OUTPUTSTREAMS_STDOUT], 0) < 0) {
    RKT_fatal("ftruncate");
  }
  if (dup2(RKT_glob.tmpfds[RK_OUTPUTSTREAMS_STDOUT], STDOUT_FILENO) < 0) {
    RKT_fatal("dup2 red1");
  }
  if (ftruncate(RKT_glob.tmpfds[RK_OUTPUTSTREAMS_STDERR], 0) < 0) {
    RKT_fatal("ftruncate");
  }
  if (dup2(RKT_glob.tmpfds[RK_OUTPUTSTREAMS_STDERR], STDERR_FILENO) < 0) {
    RKT_fatal("dup2 red2");
  }
  if (ftruncate(RKT_glob.tmpfds[RK_OUTPUTSTREAMS_LOG], 0) < 0) {
    RKT_fatal("ftruncate");
  }
#else
  if (!SetEndOfFile(RKT_glob.tmpfds[RK_OUTPUTSTREAMS_STDOUT])) {
    RKT_fatal("SetEndOfFile");
  }
  if (!SetStdHandle(STD_OUTPUT_HANDLE,
                    RKT_glob.tmpfds[RK_OUTPUTSTREAMS_STDOUT])) {
    RKT_fatal("SetStdHandle STDOUT");
  }
  if (!SetEndOfFile(RKT_glob.tmpfds[RK_OUTPUTSTREAMS_STDERR])) {
    RKT_fatal("SetEndOfFile");
  }
  if (!SetStdHandle(STD_ERROR_HANDLE,
                    RKT_glob.tmpfds[RK_OUTPUTSTREAMS_STDERR])) {
    RKT_fatal("SetStdHandle STDERR");
  }
  if (!SetEndOfFile(RKT_glob.tmpfds[RK_OUTPUTSTREAMS_LOG])) {
    RKT_fatal("SetEndOfFile");
  }
#endif
  RKT_glob.log = new_logfd;
}

rkt_fun void RKT_restore_streams(void) {
  RKT_rewindfile(RKT_glob.tmpfds[RK_OUTPUTSTREAMS_STDOUT]);
  RKT_rewindfile(RKT_glob.tmpfds[RK_OUTPUTSTREAMS_STDERR]);
  RKT_rewindfile(RKT_glob.tmpfds[RK_OUTPUTSTREAMS_LOG]);
#ifndef _WIN32
  if (dup2(RKT_glob.saved[RK_OUTPUTSTREAMS_STDOUT], STDOUT_FILENO) < 0) {
    RKT_fatal("dup2 res out");
  }
  if (dup2(RKT_glob.saved[RK_OUTPUTSTREAMS_STDERR], STDERR_FILENO) < 0) {
    RKT_fatal("dup2 res err");
  }
#else
  if (!SetStdHandle(STD_OUTPUT_HANDLE,
                    RKT_glob.saved[RK_OUTPUTSTREAMS_STDOUT])) {
    RKT_fatal("SetStdHandle STDOUT");
  }
  if (!SetStdHandle(STD_ERROR_HANDLE,
                    RKT_glob.saved[RK_OUTPUTSTREAMS_STDERR])) {
    RKT_fatal("SetStdHandle STDERR");
  }
#endif
}
rkt_fun void RKT_cleanup_test_res(RKT_TestEntry* e) {
  RKT_TestResult* r = &e->res;
  for (size_t i = 0; i < r->count; ++i) { free(r->results[i].res); }
  free(r->capt[RK_OUTPUTSTREAMS_STDOUT].str);
  free(r->capt[RK_OUTPUTSTREAMS_STDERR].str);
  free(r->results);
  *r = RKT_ZTMP(RKT_TestResult);
}
rkt_fun void RKT_cleanup_suite_res(RKT_Suite* s) {
  for (RKT_TestEntry* e = s->tests; e; e = e->next) { RKT_cleanup_test_res(e); }
}

// to debug the testing framework - compare test result function to expected
#define RKT_VALIDATE(FUNNAME, RESULT)                                          \
  //  RK_REGISTER_TEST("testmeta", test_##FUNNAME) {                               \
//    RKT_TestEntry* e = &RKT_CONCAT(RKT_entry_, FUNNAME);                       \
//    if (e->suite->attrs.init) { e->suite->attrs.init(); }                      \
//    if (!e->res.count) {                                                       \
//      rk_assert_eq(RKT_run_test(e), RESULT);                                   \
//      RKT_cleanup_test_res(e);                                                 \
//    } else {                                                                   \
//      rk_assert_eq(e->res.ended, RESULT);                                      \
//    }                                                                          \
//    if (e->suite->attrs.fini) { e->suite->attrs.fini(); }                      \
//  }

rkt_fun RKT_Suite* RKT_find_suite(const char* suitename) {
  for (RKT_Suite* s = RKT_glob.suites; s; s = s->next) {
    if (!strcmp(s->name.str, suitename)) { return s; }
  }
  return rk_null;
}
rkt_fun RKT_TestEntry* RKT_add_test_to_suite(RKT_Suite*     s,
                                             RKT_TestEntry* entry) {
  entry->suite = s;
  if (!s->tests) { return s->tests = entry; }
  RKT_TestEntry* e;
  for (e = s->tests; e->next; e = e->next);
  return e->next = entry;
}

rkt_fun RKT_Suite* RKT_add_suite(RKT_Suite* suite) {
  if (!RKT_glob.suites) { return RKT_glob.suites = suite; }
  RKT_Suite* s;
  for (s = RKT_glob.suites; s->next; s = s->next);
  return s->next = suite;
}

#define RKT_REGISTER_SUITE_IMPL(SUITENAME, ...)                                \
  RKT_CONSTRUCTOR(RKT_CONCAT(RK_test_suite_register_, __COUNTER__)) {          \
    RK_CustomTestAttributes attrs = {__VA_ARGS__};                             \
    RKT_Suite*              s     = RKT_find_suite(SUITENAME);                 \
    if (!s) {                                                                  \
      static RKT_Suite suite                                                   \
          = {rk_null, rk_null, {SUITENAME, RKT_lenof(SUITENAME)}};             \
      s = RKT_add_suite(&suite);                                               \
    }                                                                          \
    s->attrs = attrs;                                                          \
  }

#define RKT_REGISTER_TEST_IMPL(SUITENAME, FN, ...)                             \
  static void FN(void);                                                        \
  RK__IGNWARN_CLANG_BEG("-Wmissing-field-initializers")                        \
  static RKT_TestEntry RKT_CONCAT(RKT_entry_, FN)                              \
      = {rk_null,                                                              \
         rk_null,                                                              \
         {#FN, RKT_lenof(#FN)},                                                \
         FN,                                                                   \
         __COUNTER__,                                                          \
         {__FILE__, RKT_lenof(__FILE__)},                                      \
         {__VA_ARGS__}, /*todo empty initialiser*/                             \
         {RKT_ZINIT}};                                                         \
  RKT_CONSTRUCTOR(RK_test_register_##FN) {                                     \
    RKT_Suite* s = RKT_find_suite(SUITENAME);                                  \
    if (!s) {                                                                  \
      static RKT_Suite suite                                                   \
          = {rk_null, rk_null, {SUITENAME, RKT_lenof(SUITENAME)}};             \
      s = RKT_add_suite(&suite);                                               \
    }                                                                          \
    RKT_add_test_to_suite(s, &(RKT_CONCAT(RKT_entry_, FN)));                   \
  }                                                                            \
  RK__IGNWARN_CLANG_END()                                                      \
  static void FN(void)

#define RKT_RUN_TESTS_IMPL(suites, nsuites, ...)                               \
  do {                                                                         \
    RKT_Windows_Childentry();                                                  \
    RK_CustomTestAttributes attrs = {__VA_ARGS__};                             \
    RKT_run_all(suites, nsuites, attrs);                                       \
  } while (0)

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////Printing     ///////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#define RKT_RESET               "\x1b[0m"
#define RKT_RED                 "\x1b[31m"
#define RKT_GREEN               "\x1b[32m"
#define RKT_YELLOW              "\x1b[33m"
#define RKT_BLUE                "\x1b[34m"
#define RKT_INRED(str)          RKT_RED str RKT_RESET
#define RKT_INGREEN(str)        RKT_GREEN str RKT_RESET
#define RKT_INBLUE(str)         RKT_BLUE str RKT_RESET
#define RKT_INYELLOW(str)       RKT_YELLOW str RKT_RESET
#define RKT_FMT_P               RKT_INGREEN("[P] ")
#define RKT_FMT_F               RKT_INRED("[F] ")
#define RKT_FMT_C               RKT_INRED("[C] ")
#define RKT_FMT_E               RKT_INRED("[E] ")
#define RKT_FMT_T               RKT_INRED("[T] ")
#define RKT_catlit(s, l)        (memcpy(s, l, RKT_lenof(l)), RKT_lenof(l))
#define RKT_catstr(S, STR, LEN) (memcpy(S, STR, LEN), LEN)
#define RKT_fputs(STR, STREAM)  fwrite(STR, 1, RKT_lenof(STR), STREAM)

/*
Turns a length-prefixed array of up to 5 null-terminated strings into an array
by copying the pointers.
*/
rkt_fun size_t RKT_unpack_args(const char* RKT_restrict args_in, size_t len,
                               const char** RKT_restrict args) {
  if (!args_in) { return 0; }
  size_t      n = 0;
  const char *s = args_in, *e = args_in + len;
  while (s < e && n < 5) { args[n++] = s, s += strlen(s) + 1; }
  if (s < e) { RKT_fatal("more arguments than allowed"); }
  args[n] = e; // sentinel
  return n;
}

/*
TODO IMPORTANT:
ARGS FOR JSON/TAP - SHOULD I SEND EVERY TIME?
*/

rkt_fun size_t RKT_print_assertres(const RKT_TestEntry* RKT_restrict e,
                                   RKT_AssertDat cur, FILE* RKT_restrict out) {
#define RKT_printstub(_S, _FMT, _E, _HDR, _SUFF)                               \
  fprintf(_S, "%s %s:%d %s" _SUFF, _FMT, _E->file.str, _HDR->pos,              \
          _HDR->expr.str)
  char                 numbuf[2][32];
  const RKT_AssertHdr* hdr = &cur.hdr;
  const RKT_AssertRes* res = cur.res;
  switch (res->res) {
  case RKT_ASSERTRES_PASS:
    return RKT_resolve_verbosity(e, RK_OUTPUTSTREAMS_LOG) == RK_VERBOSITY_ALWAYS
        && RKT_printstub(out, RKT_FMT_P, e, hdr, "\n");
  case RKT_ASSERTRES_EXIT : return RKT_printstub(out, RKT_FMT_E, e, hdr, "\n");
  case RKT_ASSERTRES_CRASH: return RKT_printstub(out, RKT_FMT_C, e, hdr, "\n");
  case RKT_ASSERTRES_TIMEOUT:
    return !hdr->pos ? RKT_fputs(RKT_INRED("[T] TEST TIMEOUT\n"), out)
                     : RKT_printstub(out, RKT_FMT_T, e, hdr, "\n");
  default: RKT_printstub(out, RKT_FMT_F, e, hdr, ": ");
  }

  const char* args[6] = {};
  RKT_unpack_args(res->args, res->len, args);
  const char* f;
  switch (hdr->F) {
  case RKTF_true       : f = RKT_INRED(" == 'false'\n"); break;
  case RKTF_false      : f = RKT_INRED(" == 'true'\n"); break;
  case RKTF_null       : f = RKT_INRED("%s != 'rk_null'\n"); break;
  case RKTF_nonnull    : f = RKT_INRED(" == 'rk_null'\n"); break;
  case RKTF_eq         :
  case RKTF_floateq_tol: f = "%s " RKT_INRED("!=") " %s\n"; break;
  case RKTF_neq:
  case RKTF_floatneq_tol:
    f       = "%s " RKT_INRED("==") " %s\n";
    args[1] = args[0];
    break;
  case RKTF_stdouteq:
  case RKTF_stderreq: f = "\"%s\" " RKT_INRED("!=") " \"%s\"\n"; break;
  case RKTF_streq:
  case RKTF_streq_n:
    f = "\"%s\" " RKT_INRED("!=") " \"%s\" (position %s)\n";
    sprintf(numbuf[0], "%zu", res->res - 1), args[2] = numbuf[0];
    break;
  case RKTF_strneq:
  case RKTF_stdoutneq:
  case RKTF_stderrneq:
  case RKTF_strneq_n:
    f       = "\"%s\" " RKT_INRED("==") " \"%s\"\n";
    args[1] = args[0];
    break;
  case RKTF_lt:
    f = res->res == 1 ? "%s " RKT_INRED("==") " %s\n"
                      : "%s " RKT_INRED(">") " %s\n";
    break;
  case RKTF_gt:
    f = res->res == 1 ? "%s " RKT_INRED("==") " %s\n"
                      : "%s " RKT_INRED("<") " %s\n";
    break;
  case RKTF_leq: f = "%s " RKT_INRED(">") " %s\n"; break;
  case RKTF_geq: f = "%s " RKT_INRED("<") " %s\n"; break;
  case RKTF_inrange:
    f       = res->res == 1 ? "%s " RKT_INRED("<") " %s\n"
                            : "%s " RKT_INRED(">") " %s\n";
    args[1] = res->res == 1 ? args[1] : args[2];
    break;
  case RKTF_memeq:
    f = "%s and %s (%s bytes long) " RKT_INRED("not equal\n");
    break;
  case RKTF_memneq:
    f = "%s and %s (%s bytes long) " RKT_INRED("equal\n");
    break;
  case RKTF_memzero:
    f = "%s (%s bytes long) " RKT_INRED("nonzero at byte %s\n");
    sprintf(numbuf[0], "%zu", res->res - 1), args[2] = numbuf[0];
    break;
  case RKTF_memnzero:
    f = "%s (%s bytes long) " RKT_INRED("zero at byte %s\n");
    sprintf(numbuf[0], "%zu", res->res - 1), args[2] = numbuf[0];
    break;
  case RKTF_crash:
    f       = "Expected: %s, got: %s\n";
    args[0] = RKT_strcrash(hdr->reason);
    args[1] = RKT_strcrash(e->res.exit_status.reason);
    break;
  case RKTF_exit:
    f = "Expected: exit via code %s, got: exit with code %s\n";
    sprintf(numbuf[0], "%d", hdr->exit_code), args[0] = numbuf[0];
    if (res->res == 2) {
      args[1] = "none/no exit";
    } else {
      sprintf(numbuf[1], "%d", e->res.exit_status.exit_code),
          args[1] = numbuf[1];
    }
    break;
  default: RKT_fatal("Invalid Assertion Type");
  }
  return fprintf(out, f, args[0], args[1], args[2]);
#undef RKT_printstub
}
rkt_fun void RKT_print_test_reg(const RKT_TestEntry* RKT_restrict e) {
  RKT_TestResult r   = e->res;
  FILE*          out = RKT_glob.output_types[RK_OUTPUTFORMAT_NORMAL];
  if (!out) { return; }
  fprintf(out,
          RKT_INYELLOW("Running %s\n") "---------------------------------------"
                                       "-----------\n",
          e->name.str);
  {
    // RK_Verbosity v = RKT_resolve_verbosity(e, RK_OUTPUTSTREAMS_LOG);
    for (size_t i = 0; i < r.count; ++i) {
      // if (v == RK_VERBOSITY_NEVER
      //     || (!r.results[i].res->res && v != RK_VERBOSITY_ALWAYS)) {
      //   continue;
      // } todo figure this out
      RKT_print_assertres(e, r.results[i], out);
    }
  }
  if (RKT_shall_print(e, RK_OUTPUTSTREAMS_STDOUT)) {
    RKT_fputs("Stdout:\n", out);
    fwrite(r.capt[RK_OUTPUTSTREAMS_STDOUT].str, 1,
           r.capt[RK_OUTPUTSTREAMS_STDOUT].len, out);
    fputc('\n', out);
  }
  if (RKT_shall_print(e, RK_OUTPUTSTREAMS_STDERR)) {
    RKT_fputs("Stderr:\n", out);
    fwrite(r.capt[RK_OUTPUTSTREAMS_STDERR].str, 1,
           r.capt[RK_OUTPUTSTREAMS_STDERR].len, out);
    fputc('\n', out);
  }
  switch (r.ended) {
  case RKT_PASSED:
    fprintf(out, "-> " RKT_INGREEN("PASS") ": All %u assertions succeeded\n\n",
            r.count);
    break;
  case RKT_FAILED:
    fprintf(out, "-> " RKT_INRED("FAIL") ": %u fails, %u passed (total %u)\n\n",
            r.fails, r.count - r.fails, r.count); // todo bug
    break;
  case RKT_CRASHED:
    fprintf(out,
            "-> " RKT_INRED("FAIL") ": %u fails, %u passed (total %u), "
                                    "crashed unexpectedly with: %s\n\n",
            r.fails, r.count - r.fails, r.count,
            RKT_strcrash(r.exit_status.reason));
    break;
  case RKT_EXITED:
    fprintf(out,
            "-> " RKT_INRED("FAIL") ": %u fails, %u passed (total %u), "
                                    "exited unexpectedly with code %d\n\n",
            r.fails, r.count - r.fails, r.count, r.exit_status.exit_code);
    break;
  case RKT_TIMEDOUT:
    fprintf(out,
            "-> " RKT_INRED("TIMED OUT") ": %u fails, %u passed (total %u), "
                                         "exceeded time of %ldms\n\n",
            r.fails, r.count - r.fails, r.count, RKT_resolve_timeout(e));
    break;
  case RKT_TESTERROR:
    if (r.exit_status.type == RKT_EXIT_FAULT) {
      fprintf(out, RKT_INRED("Error in Test Function: %s\n\n"),
              RKT_strcrash(r.exit_status.reason));
    } else {
      fprintf(out, RKT_INRED("Error in Test Function; Exited with code %d\n\n"),
              r.exit_status.exit_code);
    }
  }
}

rkt_fun void RKT_print_suite_reg(const RKT_Suite* s, size_t total,
                                 size_t failed, size_t crashed,
                                 size_t timed_out, size_t error) {
  FILE* out = RKT_glob.output_types[RK_OUTPUTFORMAT_NORMAL];
  if (!out) { return; }
  fprintf(out, RKT_INBLUE("Suite %s\n"), s->name.str);
  for (const RKT_TestEntry* e = s->tests; e; e = e->next) {
    RKT_print_test_reg(e);
  }
  fprintf(out,
          "--- Passed %zu, failed %zu, crashed %zu, timed-out %zu out of %zu "
          "tests ---\n\n",
          total - failed - crashed, failed, crashed, timed_out, total);
}
rkt_fun void RKT_print_total_beg_json(void) {
  if (RKT_glob.output_types[RK_OUTPUTFORMAT_JSON]) {
    RKT_fputs("{\n  \"suites\": [\n",
              RKT_glob.output_types[RK_OUTPUTFORMAT_JSON]);
  }
}

rkt_fun void RKT_print_total_end_json(void) {
  if (RKT_glob.output_types[RK_OUTPUTFORMAT_JSON]) {
    RKT_fputs("  ]\n}\n", RKT_glob.output_types[RK_OUTPUTFORMAT_JSON]);
  }
}

rkt_fun void RKT_json_escape_n(const char* RKT_restrict str, size_t len) {
  FILE* out = RKT_glob.output_types[RK_OUTPUTFORMAT_JSON];
  if (!str) { return; } // not !len to allow for wraparound
  char        buf[6] = {'\\', 'u', '0', '0'};
  const char *start = str, *end = str + len;
  while (str < end) {
    unsigned char c = (unsigned char)*str;
    if (c >= 0x20 && c != '"' && c != '\\') {
      str++;
      continue;
    }
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
      buf[4] = hex[c >> 4], buf[5] = hex[c & 15];
      w = buf, l = 6;
    }
    }
    fwrite(w, 1, l, out);
    start = ++str;
  }
  if (str > start) { fwrite(start, 1, (size_t)(str - start), out); }
}

rkt_fun void RKT_print_test_json(const RKT_TestEntry* RKT_restrict e) {
  FILE* out = RKT_glob.output_types[RK_OUTPUTFORMAT_JSON];
  if (!out) { return; }
  RKT_TestResult  r    = e->res;
  const RKT_Cstrv name = e->name, file = e->file;
  char            numbuf[2][32];

  /* Header */
  RKT_fputs("    {\n"
            "      \"test_name\": \"",
            out);
  RKT_json_escape_n(name.str, name.len);
  RKT_fputs("\",\n"
            "      \"file\": \"",
            out);
  RKT_json_escape_n(file.str, file.len);
  RKT_fputs("\",\n"
            "      \"tags\": \"",
            out);
  const char* tags     = e->attrs.tags;
  size_t      tags_len = tags ? strlen(tags) : 0;
  RKT_json_escape_n(tags, tags_len);
  RKT_fputs("\",\n"
            "      \"result\": {\n",
            out);
  fprintf(out, "        \"ended\": %d,\n", r.ended);
  if (r.exit_status.type == RKT_EXIT_FAULT) {
    fprintf(out, "        \"reason\": \"%s\",\n",
            RKT_strcrash(r.exit_status.reason));
  } else {
    fprintf(out, "        \"term_code\": %d,\n", r.exit_status.exit_code);
  }
  fprintf(out,
          "        \"assert_count\": %u,\n"
          "        \"assert_failures\": %u,\n",
          r.count, r.fails);
  /* Assertions */
  if (!r.count) {
    RKT_fputs("        \"assertions\": []", out);
  } else {
    RKT_fputs("        \"assertions\": [\n", out);
    for (size_t i = 0; i < r.count; ++i) {
      RKT_AssertDat ad = r.results[i];
      RKT_fputs("          {\n"
                "            \"expr\": \"",
                out);
      RKT_json_escape_n(ad.hdr.expr.str, ad.hdr.expr.len);
      fprintf(out,
              "\",\n"
              "            \"pos\": %d,\n",
              ad.hdr.pos);
      const char* resstr;
      switch (ad.res->res) {
      case RKT_ASSERTRES_PASS   : resstr = "pass"; break;
      case RKT_ASSERTRES_EXIT   : resstr = "exit"; break;
      case RKT_ASSERTRES_CRASH  : resstr = "crash"; break;
      case RKT_ASSERTRES_TIMEOUT: resstr = "timeout"; break;
      default                   : resstr = "fail"; break;
      }
      fprintf(out, "            \"res\": \"%s\"", resstr);
      if (RKT_resolve_verbosity(e, RK_OUTPUTSTREAMS_LOG) == RK_VERBOSITY_ALWAYS
          || ad.res->res) {
        RKT_fputs(",\n            \"args\": [", out);
        if (ad.hdr.F == RKTF_crash || ad.hdr.F == RKTF_exit) {
          const char* args[2];
          if (ad.hdr.F == RKTF_crash) {
            args[0] = RKT_strcrash(ad.hdr.reason);
            args[1] = RKT_strcrash(e->res.exit_status.reason);
          } else if (ad.hdr.F == RKTF_exit) {
            sprintf(numbuf[0], "%d", ad.hdr.exit_code), args[0] = numbuf[0];
            sprintf(numbuf[1], "%d", e->res.exit_status.exit_code),
                args[1] = numbuf[1];
          }
          fprintf(out, "\"%s\", \"%s\"", args[0], args[1]);
        } else {
          const char* args[6];
          size_t      nargs = RKT_unpack_args(ad.res->args, ad.res->len, args);
          /*todo special args like exit codes etc. - need proper logic */
          for (size_t i = 0; i < nargs; ++i) {
            size_t len = args[i + 1] - args[i] - 1;
            if (i) { RKT_fputs(", ", out); }
            fputc('"', out), RKT_json_escape_n(args[i], len), fputc('"', out);
          }
        }
        RKT_fputs("]", out);
      }
      RKT_fputs("\n          }", out);
      if (i + 1 < r.count) { fputc(',', out); }
      fputc('\n', out);
    }
    RKT_fputs("        ]", out);
  }
  /* Captured output */
  if (RKT_shall_print(e, RK_OUTPUTSTREAMS_STDOUT)) {
    RKT_fputs(",\n        \"stdout\": \"", out);
    RKT_json_escape_n(r.capt[RK_OUTPUTSTREAMS_STDOUT].str,
                      r.capt[RK_OUTPUTSTREAMS_STDOUT].len);
    fputc('"', out);
  }
  if (RKT_shall_print(e, RK_OUTPUTSTREAMS_STDERR)) {
    RKT_fputs(",\n        \"stderr\": \"", out);
    RKT_json_escape_n(r.capt[RK_OUTPUTSTREAMS_STDERR].str,
                      r.capt[RK_OUTPUTSTREAMS_STDERR].len);
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
/*
TODO IMPORTANT JSON SUITE PRINTING, WHEN TO PRINT PASSES, WHAT TO DO WITH
ZERO-SIZED ARGS*/
rkt_fun void RKT_print_suite_json(const RKT_Suite* s, size_t total,
                                  size_t failed, size_t crashed,
                                  size_t timed_out, size_t error) {

  // todo unused variables
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
    fprintf(out, "PASS: Passed all %u/%u Assertions.", r.count, r.count);
    break;
  case RKT_FAILED:
    fprintf(out, "FAIL: Passed %u/%u Assertions", r.count - r.fails, r.count);
    break;
  case RKT_CRASHED:
    fprintf(out,
            "UCrash: Passed %u/%u Assertions, crashed unexpectedly with: %s",
            r.count - r.fails, r.count, RKT_strcrash(r.exit_status.reason));
    break;
  case RKT_EXITED:
    fprintf(out,
            "UExit: Passed %u/%u Assertions, exited unexpectedly with code %d",
            r.count - r.fails, r.count, r.exit_status.exit_code);
    break;
  case RKT_TIMEDOUT:
    fprintf(out, "TIMED OUT: Passed %u/%u Assertions, exceeded time of %ldms",
            r.count - r.fails, r.count, RKT_resolve_timeout(e));
    break;
  case RKT_TESTERROR:
    RKT_fputs(RKT_INRED("Error in Test Function: "), out);
    if (r.exit_status.type == RKT_EXIT_FAULT) {
      fprintf(out, RKT_INRED("%s\n\n"), RKT_strcrash(r.exit_status.reason));
    } else {
      fprintf(out, RKT_INRED("Exited with code %d\n\n"),
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
//   for (char **args = argv + 1, *arg = *args; arg != rk_null; arg = *++args) {
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

#undef RKT_ASSERTRES_PASS
#undef RKT_ASSERTRES_TIMEOUT
#undef RKT_ASSERTRES_CRASH
#undef RKT_ASSERTRES_EXIT
#undef RKT_unreachable
#undef RKT_noreturn
#undef RKT_restrict
#undef RKT_COUNTOF
#undef RKT_MIN
#undef RKT_MAX
#undef RKT_malloc
#undef RKT_realloc
#undef RKT_RESET
#undef RKT_RED
#undef RKT_GREEN
#undef RKT_YELLOW
#undef RKT_BLUE
#undef RKT_INRED
#undef RKT_INGREEN
#undef RKT_INBLUE
#undef RKT_INYELLOW
#undef RKT_FMT_P
#undef RKT_FMT_F
#undef RKT_FMT_C
#undef RKT_FMT_E
#undef RKT_FMT_T
#undef RKT_catlit
#undef RKT_catstr
#undef RKT_fputs
#undef RKT_TMP
#undef RKT_closeHandle
#undef RKT_HASCHILD
#undef RKT_KILLCHILD

#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif

#endif
