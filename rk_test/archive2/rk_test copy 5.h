
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

typedef enum RK_OutPutStreams {
  RK_OUTPUTSTREAMS_STDOUT, // captured stdout of the child process
  RK_OUTPUTSTREAMS_STDERR, // captured stderr of the child process
  RK_OUTPUTSTREAMS_META,   // assertion information
  RK_OUTPUTSTREAMS_COUNT
} RK_OutPutStreams;

typedef enum RK_OutputFormats {
  RK_OUTPUTFORMAT_NORMAL,
  RK_OUTPUTFORMAT_JSON,
  RK_OUTPUTFORMAT_TAP, // todo
  RK_OUTPUTFORMAT_COUNT
} RK_OutputFormats;

typedef enum RK_Verbosity {
  RK_VERBOSITY_GLOB_DEFAULT, // todo perhaps remove glob_default?
  RK_VERBOSITY_DEFAULT,
  RK_VERBOSITY_ALWAYS,
  RK_VERBOSITY_NEVER,
  RK_VERBOSITY_COUNT,
} RK_Verbosity;

/// @brief customisable attributes (global, suite or test-level)
typedef struct RKT_CustomAttrs {
  const char* tags;                                      /// Tags todo docs
  void (*init)(void);                                    /// Setup fixture
  void (*fini)(void);                                    /// Teardown fixture
  RK_Verbosity verbosity_levels[RK_OUTPUTSTREAMS_COUNT]; /// verbosity level
                                                         /// per output stream
  long         timeout_ms; /// Timeout in ms (default is no timeout)
} RKT_CustomAttrs;

typedef enum RKT_TestEndType {
  RKT_PASSED = 0,
  RKT_FAILED,
  RKT_CRASHED,
  RKT_TIMEDOUT,
  RKT_TESTERROR
} RKT_TestEndType;

/// @brief todo documentation
#define RK_REGISTER_SUITE(SUITENAME, ...)                                      \
  RKT_REGISTER_SUITE_IMPL(SUITENAME, __VA_ARGS__)

/// @brief todo documentation
#define RK_REGISTER_TEST(SUITENAME, fn, ...)                                   \
  RKT_REGISTER_TEST_IMPL(SUITENAME, fn, __VA_ARGS__)

/// @brief todo documentation
#define RK_RUN_TESTS(suites, nsuites, ...)                                     \
  RKT_RUN_TESTS_IMPL(suites, nsuites, __VA_ARGS__)

/*
RK_VERBOSITY_GLOB_DEFAULT: If defined globally, same as RK_VERBOSITY_DEFAULT,
otherwise, follow global settings
RK_OUTPUTSTREAMS_STDOUT:
 - RK_VERBOSITY_DEFAULT: prints output only if test fails
 - RK_VERBOSITY_ALWAYS:  always prints output
 - RK_VERBOSITY_NEVER:   never prints output

RK_OUTPUTSTREAMS_STDERR:
 - RK_VERBOSITY_DEFAULT: always prints output
 - RK_VERBOSITY_ALWAYS:  always prints output
 - RK_VERBOSITY_NEVER:   never prints output

RK_OUTPUTSTREAMS_META:s
 - RK_VERBOSITY_DEFAULT: prints output only if test fails
 - RK_VERBOSITY_ALWAYS:  always prints output
 - RK_VERBOSITY_NEVER:   never prints output (not recommended)
*/

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
#define rk_expect_stdouteq(errstr, ...)       RK_test_sendhdr("stderror prints " #errstr, RK_TEST_STDOUTEQ, RK_TEST_T_vp)
#define rk_expect_stdoutneq(errstr, ...)      RK_test_sendhdr("stderror prints " #errstr, RK_TEST_STDOUTNEQ, RK_TEST_T_vp)
#define rk_expect_stderreq(errstr, ...)       RK_test_sendhdr("stderror prints " #errstr, RK_TEST_STDERREQ, RK_TEST_T_vp)
#define rk_expect_stderrneq(errstr, ...)      RK_test_sendhdr("stderror prints " #errstr, RK_TEST_STDERRNEQ, RK_TEST_T_vp)

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
#define rk_assert_stdouteq(errstr, ...)  RK_test_sendhdr("stderror prints " #errstr, RK_TEST_STDOUTEQ, RK_TEST_T_vp)
#define rk_assert_stdoutneq(errstr, ...) RK_test_sendhdr("stderror prints " #errstr, RK_TEST_STDOUTNEQ, RK_TEST_T_vp)
#define rk_assert_stderreq(errstr, ...)  RK_test_sendhdr("stderror prints " #errstr, RK_TEST_STDERREQ, RK_TEST_T_vp)
#define rk_assert_stderrneq(errstr, ...) RK_test_sendhdr("stderror prints " #errstr, RK_TEST_STDERRNEQ, RK_TEST_T_vp)

// clang-format on
#define rk_assert_crash(signal, ...)                                           \
  do {                                                                         \
    RK_test_sendhdr_sig("assert_crash(" #signal ", " #__VA_ARGS__ ")",         \
                        RKTF_CRASH, signal);                                   \
    __VA_ARGS__;                                                               \
    {                                                                          \
      RKT_AssertResPkg RK_PKG = {1, 0};                                        \
      RKT_send_AssertRes(&RK_PKG);                                             \
    }                                                                          \
    exit(0);                                                                   \
  } while (0)

#define rk_assert_exit(code, ...)                                              \
  do {                                                                         \
    RK_test_sendhdr_sig("rk_assert_exit(" #code ", " #__VA_ARGS__ ")",         \
                        RKTF_EXIT, code);                                      \
    __VA_ARGS__;                                                               \
    {                                                                          \
      RKT_AssertResPkg RK_PKG = {1, 0};                                        \
      RKT_send_AssertRes(&RK_PKG);                                             \
    }                                                                          \
    exit(0);                                                                   \
  } while (0)

// implementation section
#ifdef __GNUC__
# pragma GCC diagnostic push
// sprintf warnings when it's safe
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
// supported by virtually every compiler
# pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
// only used conditionally
# pragma GCC diagnostic ignored "-Wc2x-extensions"
# pragma GCC diagnostic ignored "-Wunused-function"

#endif
#ifdef _WIN32
# pragma section(".CRT$XCU", read)
# include <inttypes.h>
# include <windows.h>
#else
# include <fcntl.h>
# include <poll.h>
# include <signal.h>
# include <unistd.h>
#endif
#include <assert.h>
#include <errno.h>
#ifdef __cplusplus
# include <cmath>
# include <sstream> // needed for std::ostringstream
# include <type_traits>
# include <vector>
#else
# include <math.h>
#endif
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
# define rkt_fun inline
#else
# define rkt_fun static inline
#endif
#define RKT_CONCAT(a, b)  RKT_CONCAT2(a, b)
#define RKT_CONCAT2(a, b) a##b

#if defined(__cplusplus) && __cplusplus >= 201103L
# define RKT_noreturn [[noreturn]]
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
# include <stdnoreturn.h>
# define RKT_noreturn noreturn
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

#ifndef __cplusplus
# define RKT_ZINIT 0
#else
# define RKT_ZINIT
#endif

#ifdef _WIN32
typedef HANDLE              rk_fd;
typedef PROCESS_INFORMATION rk_pid;
#else
typedef int   rk_fd;
typedef pid_t rk_pid;
#endif

#define lenof(strlit)              (sizeof("" strlit "") - 1)
#define RK_MIN(x, y)               ((x) <= (y) ? (x) : (y))
#define RK_MAX(x, y)               ((x) >= (y) ? (x) : (y))
#define RKT_malloc(T, COUNT)       ((T*)malloc(sizeof(T) * (COUNT)))
#define RKT_realloc(T, ptr, COUNT) ((T*)realloc(ptr, sizeof(T) * (COUNT)))

#define RK_TEST_TYPELIST_NOFLOAT(Y, ...)                                       \
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
#define RK_TEST_TYPELIST_FLOAT(Y, ...)                                         \
  Y(float, f, "%f", ##__VA_ARGS__)                                             \
  Y(double, d, "%f", ##__VA_ARGS__)                                            \
  Y(long double, ld, "%Lf", ##__VA_ARGS__)
#define RK_TEST_TYPELIST(Y, ...)                                               \
  RK_TEST_TYPELIST_FLOAT(Y, ##__VA_ARGS__)                                     \
  RK_TEST_TYPELIST_NOFLOAT(Y, ##__VA_ARGS__)

typedef enum RK_test_FUN {
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
} RK_test_FUN;

typedef struct RKT_Strv {
  char*  str;
  size_t len;
} RKT_Strv;
typedef const struct RKT_Cstrv {
  const char*  str;
  const size_t len;
} RKT_Cstrv;

typedef struct RKT_Suite {
  const RKT_Cstrv       name;
  RKT_CustomAttrs       attrs;
  struct RKT_TestEntry* tests;
  struct RKT_Suite*     next;
} RKT_Suite;

/// @brief Header to be sent to the test runner before assert/expect
typedef struct RKT_AssertHdr {
  int         pos;  ///< The line of the function
  RK_test_FUN F;    ///< The assert function (assert_eq, assert_true)
  int         code; ///< The expected error code (if any)
  struct {
    char          str[243]; ///< The stringified assertion
    unsigned char len;
  } expr;
} RKT_AssertHdr;

/// @brief the arguments and result (true/false or other state) is stored here
typedef struct RKT_AssertResPkg {
  size_t res; ///< Result of the test (0 is success)
  size_t len; ///< length of the args[] section
  char   args[];
} RKT_AssertResPkg;

/// @brief the arguments and result (true/false or other state) is stored here
typedef struct RKT_AssertRes {
  size_t res;
  size_t len;
  char*  args[6];
} RKT_AssertRes;

/// @brief Global settings/variables for the testing framework
static struct {
  RKT_CustomAttrs   attrs;
  const char*       custom_paths[3]; // todo this system is a mess
  FILE*             output_types[3]; // todo rename to default files?
  char              resbufr[2048];   // buffer for optimisation
  struct RKT_Suite* suites;
  jmp_buf           fret;
  rk_fd             RK__OUT;
} RKT_glob;

#define RKT_ASSERTRES_PASS    (((size_t)(0)))
#define RKT_ASSERTRES_TIMEOUT (((size_t)(-1)) - 2)
#define RKT_ASSERTRES_CRASH   (((size_t)(-1)) - 1)
#define RKT_ASSERTRES_EXIT    (((size_t)(-1)))

/// @brief Data packet for each assertion
typedef struct RKT_AssertDat {
  RKT_AssertHdr hdr; ///< Received before each assert runs
  RKT_AssertRes res; ///< Received after each assert
} RKT_AssertDat;

/// @brief Summarised results of each Test function; all pointers are owned and
/// must be freed via RKT_cleanup()
typedef struct RKT_TestResult {
  size_t          count;   ///< Number of assertions in the test
  size_t          fails;   ///< Failed assertions in the test
  RKT_AssertDat*  results; ///< Result of each assertion in the test
  RKT_Strv        capt[2]; ///< Captured stdout/stderr
  RKT_TestEndType ended;
  int             term_code;
} RKT_TestResult;

/// @brief Each test function has one
typedef struct RKT_TestEntry {
  void (*const func)(void);
  const RKT_Cstrv       file;
  const RKT_Cstrv       name;
  RKT_CustomAttrs       attrs;
  struct RKT_Suite*     suite;
  struct RKT_TestEntry* next;
  RKT_TestResult        res;
} RKT_TestEntry;

RKT_noreturn rkt_fun void RKT_fatal(const char* str);
rkt_fun int               RKT_write_full(rk_fd fd, const void* buf, size_t len);
rkt_fun int               RKT_read_full_nb(rk_fd fd, void* buf, size_t nbytes);
rkt_fun RKT_TestEndType   RKT_run_test(RKT_TestEntry* e);
rkt_fun rk_pid            RKT_init_test(const RKT_TestEntry* restrict e,
                                        rk_fd* restrict pipes);
rkt_fun RKT_TestResult    RKT_parent_loop(rk_pid pid, rk_fd* pipes,
                                          long timeout_ms);
rkt_fun void              RKT_print_test_reg(const RKT_TestEntry* restrict e);
rkt_fun size_t            RKT_fmt_assertres(const RKT_TestEntry* restrict e,
                                            RKT_AssertDat cur, char** restrict fmtbuf,
                                            size_t* restrict fmtcap);

/* todo important */
rkt_fun int               RKT_interpret_code(int code) { // todo windows
#ifdef __GNUC__
  if (WIFEXITED(code)) {
    return WEXITSTATUS(code);
  } else if (WIFSIGNALED(code)) {
    return WTERMSIG(code);
  }
#else
#endif
  return code;
}

rkt_fun long RKT_resolve_timeout(const RKT_TestEntry* restrict e) {
  if (!e->attrs.timeout_ms) {
    if (!e->suite->attrs.timeout_ms) { return RKT_glob.attrs.timeout_ms; }
    return e->suite->attrs.timeout_ms;
  }
  return e->attrs.timeout_ms;
}

rkt_fun RK_Verbosity RKT_resolve_verbosity(const RKT_TestEntry* restrict e,
                                           RK_OutPutStreams type) {
  if (e->attrs.verbosity_levels[type] == RK_VERBOSITY_GLOB_DEFAULT) {
    if (e->suite->attrs.verbosity_levels[type] == RK_VERBOSITY_GLOB_DEFAULT) {
      return RKT_glob.attrs.verbosity_levels[type];
    }
    return e->suite->attrs.verbosity_levels[type];
  }
  return e->attrs.verbosity_levels[type];
}

rkt_fun void RKT_send_AssertHdr(RKT_AssertHdr hdr) {
  if (RKT_write_full(RKT_glob.RK__OUT, &hdr, sizeof(hdr)) < 1) {
    exit(1); // todo important
  }
}
#define RK_test_sendhdr_sig(EXPRSTR, FUNENUM, sig)                             \
  RKT_send_AssertHdr(                                                          \
      (RKT_AssertHdr){__LINE__, FUNENUM, sig, {EXPRSTR, lenof(EXPRSTR)}})

#define RK_test_sendhdr(EXPRSTR, FUNENUM)                                      \
  RK_test_sendhdr_sig(EXPRSTR, FUNENUM, 0)

rkt_fun size_t RKT_send_AssertRes(const RKT_AssertResPkg* buf) {
  /*
  todo deal with non-isolated mode
  */
  if (RKT_write_full(RKT_glob.RK__OUT, buf, sizeof(*buf) + buf->len) < 1) {
    exit(1);
  }
  return buf->res;
}

#define RK_EXPECT(FUN, FUNENUM, EXPRSTR, ...)                                  \
  (RK_test_sendhdr(EXPRSTR, FUNENUM), FUN(__VA_ARGS__))

#define RK_ASSERT(FUN, FUNENUM, EXPRSTR, ...)                                  \
  (RK_test_sendhdr(EXPRSTR, FUNENUM),                                          \
   (FUN(__VA_ARGS__) ? (fflush(NULL), longjmp(RKT_glob.fret, 1)) : ((void)0)))

/// @brief returns first position where strings differ +1 (0 if same)
rkt_fun size_t RK_test_strdiff(const char* e1, size_t l1, const char* e2,
                               size_t l2) {
  size_t min = RK_MIN(l1, l2);
  for (size_t i = 0; i < min; ++i) {
    if (e1[i] != e2[i]) { return i + 1; }
  }
  return l1 == l2 ? 0 : (min + 1);
}

#ifndef __cplusplus
# define RKT_SELFUN_(T, N, F, FTYPE) , T : RKTf_##FTYPE##_##N
# define RKT_SELFUN(FTYPE, VAL)                                                \
   _Generic((VAL)RK_TEST_TYPELIST(RKT_SELFUN_, FTYPE),                         \
       char*: RKTf_streq,                                                      \
       const char*: RKTf_streq,                                                \
       default: RKTf_##FTYPE##_vp)

# define RKT_SELFUN_TOL_(T, N, F, FTYPE) , T : RKTf_##FTYPE##_##N
# define RKT_SELFUN_TOL(FTYPE, EXP)                                            \
   _Generic((EXP)RK_TEST_TYPELIST_FLOAT(RKT_SELFUN_TOL_, FTYPE))

# define RK_test_GENFUN2_sig(T, N, FMT, name)                                  \
   rkt_fun size_t RKTf_##name##_##N(T e1, T e2)
# define RK_test_GENFUN3_sig(T, N, FMT, name)                                  \
   rkt_fun size_t RKTf_##name##_##N(T e1, T e2, T e3)

#else
/*
todo special char*
*/
# define RKT_SELFUN(FTYPE, VAL)     RKTf_##FTYPE
# define RKT_SELFUN_TOL(FTYPE, ...) RKTf_##FTYPE
# define RK_test_GENFUN2_sig(T, N, FMT, name)                                  \
   rkt_fun size_t RKTf_##name(T e1, T e2)
# define RK_test_GENFUN3_sig(T, N, FMT, name)                                  \
   rkt_fun size_t RKTf_##name(T e1, T e2, T e3)

# if __cpp_lib_format >= 201907L // C++20 std::format available
#  include <format>
template <typename T, typename CharT = char>
concept RK_formattable = requires (T const& value) {
  std::formatter<T, CharT>{};
  std::format(std::basic_string<CharT>{"{}"}, value);
};
# endif
# include <type_traits>
template <typename...>
using RK_void_t = void;
template <typename T, typename U = void>
struct RK_is_streamable : std::false_type {};
template <typename T>
struct RK_is_streamable<T, RK_void_t<decltype(std::declval<std::ostream&>()
                                              << std::declval<T>())> /**/>
    : std::true_type {};

class RK_TestStream : public std::ostream {
public:
  RK_TestStream() : std::ostream(&streambuf) {}
  // Helper overloads
  template <class T>
  void add_arg_impl(const T& val, std::true_type) {
    (*this << val), this->put('\0');
  }
  template <class T>
  void add_arg_impl(const T& val, std::false_type) {
    (*this << static_cast<const void*>(&val)), this->put('\0');
  }
  template <class T>
  void add_arg(const T& val) {
    add_arg_impl(val, RK_is_streamable<T>{});
  }
  size_t send(size_t res_val) {
    RKT_AssertResPkg* res;
    size_t            package_size = streambuf.buf.size() + hdr_size;
    if (package_size < sizeof(RKT_glob.resbufr)) {
      res = (RKT_AssertResPkg*)RKT_glob.resbufr;
    } else {
      if (!(res = (RKT_AssertResPkg*)malloc(package_size))) { exit(12); }
    }
    res->res = res_val, res->len = streambuf.buf.size();
    memcpy(res->args, streambuf.buf.data(), streambuf.buf.size());
    RKT_send_AssertRes(res);
    if (package_size >= sizeof(RKT_glob.resbufr)) { free(res); }
    return res_val;
  }

private:
  static const size_t hdr_size = sizeof(RKT_AssertResPkg);
  class : public std::streambuf {
  public:
    std::vector<char> buf;

  protected:
    virtual std::streamsize xsputn(const char* s, std::streamsize n) override {
      buf.insert(buf.end(), s, s + n);
      return n;
    }
    virtual int_type overflow(int_type ch) override {
      if (ch != traits_type::eof()) { buf.push_back(ch); }
      return ch;
    }
  } streambuf;
};

// todo undef
# define RK_CPP_GENFUNS2(F)                                                    \
   F(==, eq)                                                                   \
   F(!=, neq)                                                                  \
   F(>, gt)                                                                    \
   F(>=, geq)                                                                  \
   F(<, lt)                                                                    \
   F(<=, leq)
# define RK_CPP_GENFUN(OP, OPNAME, ...)                                        \
   template <class T, class U>                                                 \
   rkt_fun size_t RKTf_##OPNAME(T e1, U e2) {                                  \
     RK_TestStream stream;                                                     \
     stream.add_arg(e1);                                                       \
     stream.add_arg(e2);                                                       \
     return stream.send((size_t)(!(e1 OP e2)));                                \
   }

RK_CPP_GENFUNS2(RK_CPP_GENFUN)

// clang-format off
rkt_fun size_t RKTf_streq(const char* e1, const char* e2);
rkt_fun size_t RKTf_strneq(const char* e1, const char* e2);
template <> size_t RKTf_eq<char*, char*>(char* e1, char* e2) { return RKTf_streq(e1, e2); }
template <> size_t RKTf_eq<char*, const char*>(char* e1, const char* e2) { return RKTf_streq(e1, e2); }
template <> size_t RKTf_eq<const char*, char*>(const char* e1, char* e2) { return RKTf_streq(e1, e2); }
template <> size_t RKTf_eq<const char*, const char*>(const char* e1, const char* e2) { return RKTf_streq(e1, e2); }
template <> size_t RKTf_neq<char*, char*>(char* e1, char* e2) { return RKTf_strneq(e1, e2); }
template <> size_t RKTf_neq<char*, const char*>(char* e1, const char* e2) { return RKTf_strneq(e1, e2); }
template <> size_t RKTf_neq<const char*, char*>(const char* e1, char* e2) { return RKTf_strneq(e1, e2); }
template <> size_t RKTf_neq<const char*, const char*>(const char* e1, const char* e2) { return RKTf_strneq(e1, e2); }
// clang-format on

#endif

#define RKT_SNDBUF0(RES)                                                       \
  RKT_AssertResPkg* _res = (RKT_AssertResPkg*)RKT_glob.resbufr;                \
  _res->res = (RES), _res->len = 0;                                            \
  return RKT_send_AssertRes(_res);
#define RKT_SNDBUF1(RES, e1, FMT1)                                             \
  RKT_AssertResPkg* _res = (RKT_AssertResPkg*)RKT_glob.resbufr;                \
  _res->res = (RES), _res->len = sprintf(_res->args, FMT1, e1) + 1;            \
  return RKT_send_AssertRes(_res);

// todo important arg gc e bug
#define RKT_SNDBUF2(RES, e1, FMT1, e2, FMT2)                                   \
  RKT_AssertResPkg* _res = (RKT_AssertResPkg*)RKT_glob.resbufr;                \
  _res->res              = (RES);                                              \
  _res->len = sprintf(_res->args, FMT1 "%c" FMT2, e1, '\0', e2) + 1;           \
  return RKT_send_AssertRes(_res);

#define RKT_SNDBUF3(RES, e1, FMT1, e2, FMT2, e3, FMT3)                         \
  RKT_AssertResPkg* _res = (RKT_AssertResPkg*)RKT_glob.resbufr;                \
  _res->res              = (RES);                                              \
  _res->len                                                                    \
      = sprintf(_res->args, FMT1 "%c" FMT2 "%c" FMT3, e1, '\0', e2, '\0', e3)  \
      + 1;                                                                     \
  return RKT_send_AssertRes(_res);

#define RK_test_GENFUN2(T, N, FMT, name, expr)                                 \
  RK_test_GENFUN2_sig(T, N, FMT, name) {                                       \
    RKT_SNDBUF2((expr), e1, FMT, e2, FMT);                                     \
  }
#define RK_test_GENFUN3(T, N, FMT, name, expr)                                 \
  RK_test_GENFUN3_sig(T, N, FMT, name) {                                       \
    RKT_SNDBUF3((expr), e1, FMT, e2, FMT, e3, FMT);                            \
  }

// clang-format off
#define RK_test_GENFUNS_alltypes(T, N, FMT, ...)                               \
    RK_test_GENFUN2(T, N, FMT, lt, (e1 < e2 ? 0 : (e1 == e2 ? 1 : 2)))         \
    RK_test_GENFUN2(T, N, FMT, leq, !(e1 <= e2))                               \
    RK_test_GENFUN2(T, N, FMT, gt, (e1 > e2 ? 0 : (e1 == e2 ? 1 : 2)))         \
    RK_test_GENFUN2(T, N, FMT, geq, !(e1 >= e2))                               \
    RK_test_GENFUN3(T, N, FMT, inrange, (e1 < e2 ? 1 : (e1 > e3 ? 2 : 0)))
#define RK_test_GENFUNS_nofloat(T, N, FMT, ...)                                \
    RK_test_GENFUN2(T, N, FMT, eq, !(e1 == e2))                                \
    RK_test_GENFUN2(T, N, FMT, neq, !(e1 != e2))                       
#define RK_test_GENFUNS_float(T, N, FMT, eps_default, fabs_fn)                 \
    RK_test_GENFUN2(T, N, FMT, eq, !(fabs_fn(e1 - e2) <= eps_default))         \
    RK_test_GENFUN2(T, N, FMT, neq, !(fabs_fn(e1 - e2) > eps_default))         \
    RK_test_GENFUN3(T, N, FMT, toleq, !(fabs_fn(e1 - e2) <= e3))               \
    RK_test_GENFUN3(T, N, FMT, tolneq, !(fabs_fn(e1 - e2) > e3))

RK_TEST_TYPELIST_NOFLOAT(RK_test_GENFUNS_nofloat)                
RK_TEST_TYPELIST(RK_test_GENFUNS_alltypes)                       
RK_test_GENFUNS_float(float, f, "%g", 1e-6f, fabsf)              
RK_test_GENFUNS_float(double, d, "%g", 1e-12, fabs)          
RK_test_GENFUNS_float(long double, ld, "%Lg", 1e-12L, fabsl)


rkt_fun size_t RKTf_true(bool e1) { RKT_SNDBUF0(!e1); }
rkt_fun size_t RKTf_false(bool e1) { RKT_SNDBUF0(!!e1); }
// clang-format on
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
  size_t            l1 = strlen(e1), l2 = strlen(e2);
  size_t            dif          = RK_test_strdiff(e1, l1, e2, l2);
  size_t            package_size = sizeof(RKT_AssertResPkg) + l1 + l2 + 2;
  if (package_size <= sizeof(RKT_glob.resbufr)) {
    res = (RKT_AssertResPkg*)RKT_glob.resbufr;
  } else {
    if (!(res = (RKT_AssertResPkg*)malloc(package_size))) { exit(12); }
  }
  res->res = dif, res->len = l1 + l2 + 2;
  memcpy(res->args, e1, l1 + 1), memcpy(res->args + l1 + 1, e2, l2 + 1);
  RKT_send_AssertRes(res);
  if (package_size > sizeof(RKT_glob.resbufr)) { free(res); }
  return dif;
}

rkt_fun size_t RKTf_strneq(const char* e1, const char* e2) {
  RKT_AssertResPkg* res;
  size_t            l1 = strlen(e1), l2 = strlen(e2);
  size_t            dif          = RK_test_strdiff(e1, l1, e2, l2);
  size_t            package_size = sizeof(RKT_AssertResPkg) + l1 + l2 + 2;
  if (package_size <= sizeof(RKT_glob.resbufr)) {
    res = (RKT_AssertResPkg*)RKT_glob.resbufr;
  } else {
    if (!(res = (RKT_AssertResPkg*)malloc(package_size))) { exit(12); }
  }
  res->res = !(dif != 0), res->len = l1 + l2 + 2;
  memcpy(res->args, e1, l1 + 1), memcpy(res->args + l1 + 1, e2, l2 + 1);
  RKT_send_AssertRes(res);
  if (package_size > sizeof(RKT_glob.resbufr)) { free(res); }
  return !(dif != 0);
}
rkt_fun size_t RKTf_memeq(const void* e1, const void* e2, size_t e3) {
  size_t _result = !((e1 == e2) || (e1 && e2 && !memcmp(e1, e2, e3)));
  RKT_SNDBUF3(_result, e1, "%p", e2, "%p", e3, "%zu");
}
rkt_fun size_t RKTf_memneq(const void* e1, const void* e2, size_t e3) {
  size_t _result = !!((e1 == e2) || (e1 && e2 && !memcmp(e1, e2, e3)));
  RKT_SNDBUF3(_result, e1, "%p", e2, "%p", e3, "%zu");
}

rkt_fun size_t RKTf_streq_n(const char* e1, const char* e2, size_t n) {
  RKT_AssertResPkg* res;
  if (!n) {
    res = (RKT_AssertResPkg*)RKT_glob.resbufr, res->res = 0, res->len = 0;
    return RKT_send_AssertRes(res);
  }
  size_t dif          = RK_test_strdiff(e1, n, e2, n);
  size_t package_size = sizeof(RKT_AssertResPkg) + n + n + 2;
  if (package_size <= sizeof(RKT_glob.resbufr)) {
    res = (RKT_AssertResPkg*)RKT_glob.resbufr;
  } else {
    if (!(res = (RKT_AssertResPkg*)malloc(package_size))) { exit(12); }
  }
  res->res = dif, res->len                               = n + n + 2;
  memcpy(res->args, e1, n), res->args[n]                 = '\0';
  memcpy(res->args + n + 1, e2, n), res->args[n + 1 + n] = '\0';
  RKT_send_AssertRes(res);
  if (package_size > sizeof(RKT_glob.resbufr)) { free(res); }
  return dif;
}

rkt_fun size_t RKTf_strneq_n(const char* e1, const char* e2, size_t n) {
  RKT_AssertResPkg* res;
  if (!n) {
    res = (RKT_AssertResPkg*)RKT_glob.resbufr, res->res = 1, res->len = 0;
    return RKT_send_AssertRes(res);
  }
  size_t dif          = RK_test_strdiff(e1, n, e2, n);
  size_t package_size = sizeof(RKT_AssertResPkg) + n + n + 2;
  if (package_size <= sizeof(RKT_glob.resbufr)) {
    res = (RKT_AssertResPkg*)RKT_glob.resbufr;
  } else {
    if (!(res = (RKT_AssertResPkg*)malloc(package_size))) { exit(12); }
  }
  res->res = !(dif != 0), res->len                       = n + n + 2;
  memcpy(res->args, e1, n), res->args[n]                 = '\0';
  memcpy(res->args + n + 1, e2, n), res->args[n + 1 + n] = '\0';
  RKT_send_AssertRes(res);
  if (package_size > sizeof(RKT_glob.resbufr)) { free(res); }
  return !(dif != 0);
}

// todo assert_crash without specifying which one

#define RK_RESET               "\x1b[0m"
#define RK_RED                 "\x1b[31m"
#define RK_GREEN               "\x1b[32m"
#define RK_YELLOW              "\x1b[33m"
#define RK_BLUE                "\x1b[34m"
#define RK_INRED(str)          RK_RED str RK_RESET
#define RK_INGREEN(str)        RK_GREEN str RK_RESET
#define RK_INBLUE(str)         RK_BLUE str RK_RESET
#define RK_INYELLOW(str)       RK_YELLOW str RK_RESET
#define RK_TFMT_P              RK_INGREEN("[P] ")
#define RK_TFMT_F              RK_INRED("[F] ")
#define RK_TFMT_C              RK_INRED("[C] ")
#define RK_TFMT_E              RK_INRED("[E] ")
#define RK_TFMT_T              RK_INRED("[T] ")
#define RK_catlit(s, l)        (memcpy(s, l, lenof(l)), lenof(l))
#define RK_catstr(S, STR, LEN) (memcpy(S, STR, LEN), LEN)

rkt_fun size_t RK_fmt_stub(char* restrict s, const RKT_TestEntry* restrict e,
                           const RKT_AssertHdr hdr) {
  char* _s  = s;
  s        += RK_catstr(s, e->file.str, e->file.len);
  s        += sprintf(s, ":%d ", hdr.pos);
  s        += RK_catstr(s, hdr.expr.str, hdr.expr.len);
  return s - _s;
}

rkt_fun size_t RKT_fmt_assertres(const RKT_TestEntry* restrict e,
                                 RKT_AssertDat cur, char** restrict fmtbuf,
                                 size_t* restrict fmtcap) {

/*
todo: create no-colour option
*/
#define RK_catstub(S, PASSTYPE, E, HDR)                                        \
  (RK_catlit((S), PASSTYPE) + RK_fmt_stub(S + lenof(PASSTYPE), E, HDR))

  RKT_AssertRes res  = cur.res;
  char**        args = res.args;
  RKT_AssertHdr hdr  = cur.hdr;
  char*         s    = *fmtbuf;
  switch (res.res) {
  case RKT_ASSERTRES_PASS:
    if (RKT_resolve_verbosity(e, RK_OUTPUTSTREAMS_META)
        == RK_VERBOSITY_ALWAYS) {
      s += RK_catstub(s, RK_TFMT_P, e, hdr), *s++ = '\n';
    }
    break;
  case RKT_ASSERTRES_EXIT:
    s += RK_catstub(s, RK_TFMT_E, e, hdr), *s++ = '\n';
    break;
  case RKT_ASSERTRES_CRASH:
    s += RK_catstub(s, RK_TFMT_C, e, hdr), *s++ = '\n';
    break;
  case RKT_ASSERTRES_TIMEOUT:
    if (hdr.pos == 0) {
      s += RK_catlit(s, RK_INRED("[T] TEST TIMEOUT\n"));
    } else {
      s += RK_catstub(s, RK_TFMT_T, e, hdr), *s++ = '\n';
    }
    break;
  default:
    if (res.len + 256 > *fmtcap) {
      *fmtcap = res.len + 256;
      *fmtbuf = RKT_realloc(char, *fmtbuf, *fmtcap);
      if (!*fmtbuf) { fputs("Error: Realloc fmt failed\n", stderr), exit(1); }
      s = *fmtbuf;
    }
    s += RK_catstub(s, RK_TFMT_F, e, hdr), *s++ = ' ';
    switch (hdr.F) {
    case RKTF_TRUE:
      s += RK_catlit(s, "Evaluated to " RK_INRED("'false'") ".\n");
      break;
    case RKTF_FALSE:
      s += RK_catlit(s, "Evaluated to " RK_INRED("'true'") ".\n");
      break;
    case RKTF_NULL:
      s += RK_catlit(s, "Evaluated to " RK_RED);
      s += RK_catstr(s, args[0], args[1] - args[0]);
      s += RK_catlit(s, RK_RESET ".\n");
      break;
    case RKTF_NNULL:
      s += RK_catlit(s, "Evaluated to" RK_INRED("NULL") ".\n");
      break;
    case RKTF_EQ:
      s += RK_catlit(s, RK_RED);
      s += RK_catstr(s, args[0], args[1] - args[0]);
      s += RK_catlit(s, " != ");
      s += RK_catstr(s, args[1], args[2] - args[1]);
      s += RK_catlit(s, RK_RESET ".\n");
      break;
    case RKTF_TOLEQ:
      s += RK_catlit(s, RK_RED);
      s += RK_catstr(s, args[0], args[1] - args[0]);
      s += RK_catlit(s, " != ");
      s += RK_catstr(s, args[1], args[2] - args[1]);
      s += RK_catlit(s, RK_RESET " (tol: ");
      s += RK_catstr(s, args[2], args[3] - args[2]);
      s += RK_catlit(s, ").\n");
      break;
    case RKTF_NEQ:
      s += RK_catlit(s, RK_RED);
      s += RK_catstr(s, args[0], args[1] - args[0]);
      s += RK_catlit(s, " == ");
      s += RK_catstr(s, args[1], args[2] - args[1]);
      s += RK_catlit(s, RK_RESET ".\n");
      break;
    case RKTF_TOLNEQ:
      s += RK_catlit(s, RK_RED);
      s += RK_catstr(s, args[0], args[1] - args[0]);
      s += RK_catlit(s, " == ");
      s += RK_catstr(s, args[1], args[2] - args[1]);
      s += RK_catlit(s, RK_RESET " (tol: ");
      s += RK_catstr(s, args[2], args[3] - args[2]);
      s += RK_catlit(s, ").\n");
      break;
    case RKTF_LT:
      s += RK_catlit(s, RK_RED);
      s += RK_catstr(s, args[0], args[1] - args[0]);
      if (res.res == 1) {
        s += RK_catlit(s, " == ");
      } else {
        s += RK_catlit(s, " > ");
      }
      s += RK_catstr(s, args[1], args[2] - args[1]);
      s += RK_catlit(s, RK_RESET ".\n");
      break;
    case RKTF_LEQ:
      s += RK_catlit(s, RK_RED);
      s += RK_catstr(s, args[0], args[1] - args[0]);
      s += RK_catlit(s, " > ");
      s += RK_catstr(s, args[1], args[2] - args[1]);
      s += RK_catlit(s, RK_RESET ".\n");
      break;
    case RKTF_GT:
      s += RK_catlit(s, RK_RED);
      s += RK_catstr(s, args[0], args[1] - args[0]);
      if (res.res == 1) {
        s += RK_catlit(s, " == ");
      } else {
        s += RK_catlit(s, " < ");
      }
      s += RK_catstr(s, args[1], args[2] - args[1]);
      s += RK_catlit(s, RK_RESET ".\n");
      break;
    case RKTF_GEQ:
      s += RK_catlit(s, RK_RED);
      s += RK_catstr(s, args[0], args[1] - args[0]);
      s += RK_catlit(s, " > ");
      s += RK_catstr(s, args[1], args[2] - args[1]);
      s += RK_catlit(s, RK_RESET ".\n");
      break;
    case RKTF_INRANGE:
      s += RK_catlit(s, RK_RED);
      s += RK_catstr(s, args[0], args[1] - args[0]);
      if (res.res == 1) {
        s += RK_catlit(s, " < ");
        s += RK_catstr(s, args[1], args[2] - args[1]);
      } else {
        s += RK_catlit(s, " > ");
        s += RK_catstr(s, args[2], args[3] - args[2]);
      }
      s += RK_catlit(s, RK_RESET ".\n");
      break;
    case RKTF_MEMEQ:
      s += RK_catlit(s, "Memory Regions ");
      s += RK_catstr(s, args[0], args[1] - args[0]);
      s += RK_catlit(s, " and ");
      s += RK_catstr(s, args[1], args[2] - args[1]);
      s += RK_catlit(s, "(");
      s += RK_catstr(s, args[2], args[3] - args[2]);
      s += RK_catlit(s, " bytes long)" RK_INRED(" not equal") ".\n");
      break;
    case RKTF_MEMNEQ:
      s += RK_catlit(s, "Memory Regions ");
      s += RK_catstr(s, args[0], args[1] - args[0]);
      s += RK_catlit(s, " and ");
      s += RK_catstr(s, args[1], args[2] - args[1]);
      s += RK_catlit(s, "(");
      s += RK_catstr(s, args[2], args[3] - args[2]);
      s += RK_catlit(s, " bytes long)" RK_INRED(" equal") ".\n");
      break;
    case RKTF_MEMZERO:
      s += RK_catstr(s, args[0], args[1] - args[0]);
      s += RK_catlit(s, " (");
      s += RK_catstr(s, args[1], args[2] - args[1]);
      s += RK_catlit(s, " bytes long) nonzero at byte " RK_RED);
      s += sprintf(s, "%zu .\n", res.res - 1);
      break;
    case RKTF_MEMNZERO:
      s += RK_catstr(s, args[0], args[1] - args[0]);
      s += RK_catlit(s, " (");
      s += RK_catstr(s, args[1], args[2] - args[1]);
      s += RK_catlit(s, " bytes long) zero at byte " RK_RED);
      s += sprintf(s, "%zu .\n", res.res - 1);
      break;
    case RKTF_STREQ:
      s += RK_catlit(s, "strings ");
      s += RK_catstr(s, args[0], args[1] - args[0]);
      s += RK_catlit(s, ", ");
      s += RK_catstr(s, args[1], args[2] - args[1]);
      s += RK_catlit(s, RK_INRED(" not equal") " at position ");
      s += sprintf(s, "%zu.\n", res.res - 1);
      break;
    case RKTF_STRNEQ:
      s += RK_catlit(s, "strings " RK_INRED("equal") ".\n");
      break;
    case RKTF_CRASH:
      if (res.res == 1) {
        s += RK_catlit(s, "Did not crash\n");
      } else {
        s += sprintf(s,
                     "Expected: termination via signal %d, got: "
                     "termination with signal %s\n",
                     hdr.code, args[0]);
      }
      break; // todo signal interpretation
    case RKTF_EXIT:
      if (res.res == 1) {
        s += RK_catlit(s, "Did not terminate\n");
      } else {
        s += sprintf(s,
                     "Expected: exit via code %d, got: "
                     "exit with code %s\n",
                     hdr.code, args[0]);
      }
      break;
    case RK_TEST_STDOUTEQ:
    case RK_TEST_STDOUTNEQ:
    case RK_TEST_STDERREQ:
    case RK_TEST_STDERRNEQ: exit(1); break; // todo not implemented
    }
  }
  return s - *fmtbuf;
#undef RK_catstub
}

rkt_fun void RKT_print_test_reg(const RKT_TestEntry* restrict e) {
  RKT_TestResult r   = e->res;
  FILE*          out = RKT_glob.output_types[RK_OUTPUTFORMAT_NORMAL];
  if (!out) { return; }
  fprintf(out,
          RK_INYELLOW("Running %s\n") "---------------------------------------"
                                      "-----------\n",
          e->name.str);
  size_t fmtcap = 1024;
  char*  fmtbuf = RKT_malloc(char, fmtcap);
  if (!fmtbuf) { fputs("Error: Realloc fmt failed\n", stderr), exit(1); }
  for (size_t i = 0; i < r.count; ++i) {
    size_t bytes = RKT_fmt_assertres(e, r.results[i], &fmtbuf, &fmtcap);
    fwrite(fmtbuf, 1, bytes, out);
  }
  free(fmtbuf);
  for (int j = 0; j <= 1; ++j) {
    RK_OutPutStreams i = (RK_OutPutStreams)j;
    RK_Verbosity     v = RKT_resolve_verbosity(e, i);
    if ((v != RK_VERBOSITY_NEVER)
        && ((v == RK_VERBOSITY_ALWAYS) || r.ended != RKT_PASSED)) {
      if (r.capt[i].len) {
        fputs(i == RK_OUTPUTSTREAMS_STDOUT ? "Stdout:\n" : "Stderr:\n", out);
        fwrite(r.capt[i].str, 1, r.capt[i].len, out);
        fputc('\n', out);
      }
    }
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
                                   "crashed unexpectedly with signal %d\n\n",
            r.fails, r.count - r.fails, r.count,
            RKT_interpret_code(r.term_code));
    break;
  case RKT_TIMEDOUT:
    fprintf(out,
            "-> " RK_INRED("TIMED OUT") ": %zu fails, %zu passed (total %zu), "
                                        "exceeded time of %ldms\n\n",
            r.fails, r.count - r.fails, r.count, RKT_resolve_timeout(e));
    break;
  case RKT_TESTERROR: // todo windows
#ifndef _WIN32
    if (WIFSIGNALED(r.term_code)) {
      fprintf(out,
              RK_INRED("Error in Test Function; Crashed with signal %d\n\n"),
              WTERMSIG(r.term_code));
    } else {
      fprintf(out, RK_INRED("Error in Test Function; Exited with code %d\n\n"),
              WEXITSTATUS(r.term_code));
    }
#else
      ;
#endif
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

// todo remove for consistency
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
  for (size_t i = 0, l = len; i < l; i++) {
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
  RKT_TestResult r   = e->res;
  FILE*          out = RKT_glob.output_types[RK_OUTPUTFORMAT_JSON];
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
    fprintf(out, "        \"ended\": %d,\n", r.ended);
    fprintf(out, "        \"term_code\": %d,\n", r.term_code);
    fprintf(out, "        \"assert_count\": %zu,\n", r.count);
    fprintf(out, "        \"assert_failures\": %zu,\n", r.fails);
    if (!r.count) {
      fputs("        \"assertions\": []", out);
    } else {
      fputs("        \"assertions\": [\n", out);
      for (size_t i = 0; i < r.count; i++) {
        RKT_AssertDat* ad = &r.results[i];
        fputs("          {\n", out);
        fputs("            \"expr\": \"", out);
        RKT_json_escape_n(ad->hdr.expr.str, ad->hdr.expr.len);
        fputs("\",\n", out);
        fprintf(out, "            \"pos\": %d,\n", ad->hdr.pos);
        static const char* res_strs[] = {"pass", "fail", "crash", "timeout"};
        const char*        resstr;
        switch (ad->res.res) {
        case RKT_ASSERTRES_PASS   : resstr = res_strs[0]; break;
        default                   : resstr = res_strs[1]; break;
        case RKT_ASSERTRES_EXIT   :
        case RKT_ASSERTRES_CRASH  : resstr = res_strs[2]; break;
        case RKT_ASSERTRES_TIMEOUT: resstr = res_strs[3]; break;
        }
        fprintf(out, "            \"res\": \"%s\",\n", resstr);
        fputs("            \"args\": [", out);
        for (int j = 0; j < 3; ++j) { // shit
          if (!ad->res.args[j + 1]) { break; }
          if (j > 0) { fputs(", ", out); }
          fputs("\"", out);
          RKT_json_escape_n(ad->res.args[j],
                            (size_t)(ad->res.args[j + 1] - ad->res.args[j]
                                     - 1)); // todo find a way to de-stringify
          fputs("\"", out);
        }
        fputs("]\n", out);
        fputs("          }", out);
        if (i != r.count - 1) { fputs(",", out); }
        fputc('\n', out);
      }
      fputs("        ]", out);
    }
    RK_Verbosity v = RKT_resolve_verbosity(e, RK_OUTPUTSTREAMS_STDOUT);
    if ((v != RK_VERBOSITY_NEVER)
        && ((v == RK_VERBOSITY_ALWAYS) || r.ended != RKT_PASSED)) {
      fputs(",\n        \"stdout\": \"", out);
      RKT_json_escape_n(e->res.capt[RK_OUTPUTSTREAMS_STDOUT].str,
                        e->res.capt[RK_OUTPUTSTREAMS_STDOUT].len);
      fputs("\"", out);
    }

    v = RKT_resolve_verbosity(e, RK_OUTPUTSTREAMS_STDERR);
    if ((v != RK_VERBOSITY_NEVER)
        && ((v == RK_VERBOSITY_ALWAYS) || r.ended != RKT_PASSED)) {
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

rkt_fun void RKT_cleanup_test_res(RKT_TestEntry* e) {
  while (e->res.count--) { free(e->res.results[e->res.count].res.args[0]); }
  free(e->res.capt[0].str), free(e->res.capt[1].str);
  free(e->res.results);
  e->res.count = 0;
}

rkt_fun void RKT_cleanup_suite_res(RKT_Suite* s) {
  for (RKT_TestEntry* e = s->tests; e; e = e->next) { RKT_cleanup_test_res(e); }
}

rkt_fun RKT_TestEndType RKT_run_test(RKT_TestEntry* e) {
  // printf("RUNNING %s\n", e->name); // for debugging
  rk_fd  pipes[3];
  rk_pid pid = RKT_init_test(e, pipes);
  e->res     = RKT_parent_loop(pid, pipes, RKT_resolve_timeout(e));
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
  FILE* const default_files[3] = {stdout, NULL, NULL};
  for (int i = 0; i < 3; ++i) {
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
  if (!RKT_glob.attrs.timeout_ms) { RKT_glob.attrs.timeout_ms = -1; }
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
/*
todo understand how to reap children
 */
#ifndef _WIN32
rkt_fun void RKT_handle_sigterm(int signum) {
  assert(signum == SIGTERM), (void)signum;
  kill(-getpid(), SIGTERM), _exit(1);
}
// TODO understand this
rkt_fun void RKT_init_group() {
  setpgid(0, 0);
  sigset_t m;
  sigemptyset(&m);
  sigaddset(&m, SIGINT), sigaddset(&m, SIGQUIT), sigaddset(&m, SIGTSTP);
  struct sigaction sa = {.sa_handler = RKT_handle_sigterm, .sa_mask = m};
  sigaction(SIGTERM, &sa, NULL);
}
RKT_noreturn rkt_fun void RKT_fatal(const char* str) {
  perror(str), fflush(NULL);
  kill(getpid(), SIGTERM), abort(); // todo maybe just sigkill
}
#else
static rk_pid RKT_glob_job;
rkt_fun void  RKT_init_group() {
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

/// PLATFORM DIFFERENCES
#ifndef _WIN32
# define RKT_CONSTRUCTOR(fn) __attribute__((constructor)) static void fn(void)
# define RK_TEST_SPECIALCHILDENTRY()

rkt_fun int RKT_reapchild(rk_pid pid) {
  int status;
  while (waitpid(pid, &status, 0) == -1) {
    if (errno != EINTR) { RKT_fatal("waitpid"); }
  };
  return status;
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
      perror("dup2"), exit(1);
    }
    if (e->attrs.init) { e->attrs.init(); }
    RKT_glob.RK__OUT = pipes[2];
    int val          = setjmp(RKT_glob.fret);
    if (val == 0) {
      e->func();
    } else {
      // todo jump
    }
    if (e->attrs.fini) { e->attrs.fini(); }
    exit(0);
  } else {
    for (int i = 0; i < 3; ++i) { // set pipes nonblocking
      int fd = pipes[i], flags = fcntl(fd, F_GETFL, 0);
      if (flags == -1 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        RKT_fatal("fcntl");
      }
    }
  }
  return pid;
}

rkt_fun long RKT_now_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

rkt_fun RKT_TestResult RKT_parent_loop(rk_pid pid, rk_fd* pipes,
                                       long timeout_ms) {
  size_t         rescap = 512, caps[2] = {512, 512};
  RKT_TestResult r = {RKT_ZINIT};
  r.results        = RKT_malloc(RKT_AssertDat, rescap);
  r.capt[0].str    = RKT_malloc(char, caps[0]);
  r.capt[1].str    = RKT_malloc(char, caps[1]);
  if (!r.results || !r.capt[0].str || !r.capt[1].str) {
    RKT_fatal("malloc buf");
  }
  long          time_start = RKT_now_ms();
  struct pollfd fds[3]
      = {{pipes[0], POLLIN, 0}, {pipes[1], POLLIN, 0}, {pipes[2], POLLIN, 0}};
  bool reading_res = 0;
  for (int nfds = 3; nfds > 0;) {
    for (int poll_timeout = -1;;) {
      if (timeout_ms != -1) {
        long elapsed = RKT_now_ms() - time_start;
        poll_timeout = (int)RK_MAX(timeout_ms - elapsed, 0);
      }
      switch (poll(fds, 3, poll_timeout)) {
      case -1:
        if (errno != EINTR) { RKT_fatal("poll"); }
        continue;
      case 0: {
        RKT_AssertRes res = {RKT_ASSERTRES_TIMEOUT, 0, {RKT_ZINIT}};
        if (reading_res) {
          r.results[r.count - 1].res = res;
        } else {
          memset(&r.results[r.count].hdr, 0, sizeof(RKT_AssertHdr));
          r.results[r.count++].res = res;
        }
        kill(pid, SIGKILL), RKT_reapchild(pid);
        close(pipes[0]), close(pipes[1]), close(pipes[2]);
        return ++r.fails, r.ended = RKT_TIMEDOUT, r;
      }
      }
      break;
    }
    if (fds[2].revents & POLLIN) {
      if (!reading_res) {
        if (r.count + 1 == rescap) { //+1 to simplify timeout code
          rescap    *= 2;
          r.results  = RKT_realloc(RKT_AssertDat, r.results, rescap);
          if (!r.results) { RKT_fatal("realloc 0"); }
        }
        RKT_AssertHdr hdr;
        switch (RKT_read_full_nb(fds[2].fd, &hdr, sizeof(hdr))) {
        case -1: RKT_fatal("read meta hdr");
        case 0 : fds[2].fd = -1, --nfds; break;
        case 1 : r.results[r.count++].hdr = hdr, reading_res = 1;
        }
      } else {
        RKT_AssertRes res = {RKT_ZINIT};
        switch (RKT_read_full_nb(fds[2].fd, &res, sizeof(RKT_AssertResPkg))) {
        case -1: RKT_fatal("read meta res");
        case 0 : fds[2].fd = -1, --nfds; break;
        case 1:
          if (res.len) {
            // shit
            res.args[0] = RKT_malloc(char, res.len);
            if (!res.args[0]) { RKT_fatal("malloc"); }
            if (RKT_read_full_nb(fds[2].fd, res.args[0], res.len) < 1) {
              RKT_fatal("read OWO");
            }
            size_t i = 1;
            // printf("Assert: %s\n", r.results[r.count - 1].hdr.exprstr);
            // printf("len: %zu\n", res.len);
            // printf("arg: %s\n", res.args[0]);
            for (size_t len = 0; len < res.len; ++len) {
              if (res.args[0][len] == '\0' && len + 1 < res.len) {
                // printf("arg: %s\n", res.args[i]);
                assert(i + 1 < sizeof(res.args) / sizeof(res.args[0]));
                res.args[i++] = res.args[0] + len + 1;
              } // todo important cpp errors
            }
            res.args[i] = res.args[0] + res.len; // points past last byte
            //  second to last is sentinel, check next one == NULL to detect it
          }
          if (res.res) { ++r.fails; }
          r.results[r.count - 1].res = res, reading_res = 0;
        }
      }
    }
    for (int i = 0; i < 2; ++i) {
      if (!(fds[i].revents & POLLIN)) { continue; }
      ssize_t rd = read(fds[i].fd, r.capt[i].str + r.capt[i].len,
                        caps[i] - r.capt[i].len - 1);
      if (rd == -1) {
        if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
          RKT_fatal("read out/err res");
        }
      } else if (rd == 0) {
        fds[i].fd = -1, --nfds;
      } else {
        r.capt[i].len += rd;
        if (r.capt[i].len + 1 == caps[i]) {
          caps[i]       *= 2;
          r.capt[i].str  = RKT_realloc(char, r.capt[i].str, caps[i]);
          if (!r.capt[i].str) { RKT_fatal("realloc"); }
          r.capt[i].str[r.capt[i].len - 1] = '\0';
        }
      }
    }
  }
  close(pipes[0]), close(pipes[1]), close(pipes[2]);
  r.term_code = RKT_reapchild(pid);
  if (reading_res) {
    RKT_AssertRes* lastres       = &r.results[r.count - 1].res;
    *lastres                     = (RKT_AssertRes){RKT_ZINIT};
    const RKT_AssertHdr* lasthdr = &r.results[r.count - 1].hdr;
    if (WIFSIGNALED(r.term_code)) {
      int sig = WTERMSIG(r.term_code);
      if (lasthdr->F == RKTF_CRASH) {
        if (!(lastres->args[0] = (char*)malloc(32))) { RKT_fatal("calloc"); }
        lastres->len = sprintf(lastres->args[0], "%d", sig) + 1;
        if (sig != lasthdr->code) { lastres->res = 2; }
      } else {
        lastres->res = RKT_ASSERTRES_CRASH, r.ended = RKT_CRASHED;
      }
    } else {
      int stat = WEXITSTATUS(r.term_code);
      if (lasthdr->F == RKTF_EXIT) {
        if (!(lastres->args[0] = (char*)malloc(32))) { RKT_fatal("calloc"); }
        lastres->len = sprintf(lastres->args[0], "%d", stat) + 1;
        if (stat != lasthdr->code) { lastres->res = 2; }
      } else {
        lastres->res = RKT_ASSERTRES_EXIT, r.ended = RKT_CRASHED;
      }
    }
    if (lastres->res) { ++r.fails; }
  } else if (r.term_code) { // error outside of assertions
    r.ended = RKT_TESTERROR;
  }
  if (r.ended == RKT_PASSED && r.fails) { r.ended = RKT_FAILED; }
  return r;
}

rkt_fun int RKT_read_full_nb(int fd, void* buf, size_t nbytes) {
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
    case 0 : return 0; // eof
    default: ptr += r, nbytes -= r;
    }
  }
  return 1; // read all, no eof
}

rkt_fun int RKT_write_full(int fd, const void* buf, size_t len) {
  const char* ptr = (const char*)buf;
  for (ssize_t w; len;) {
    if ((w = write(fd, ptr, len)) == -1) {
      if (errno != EINTR) { return -1; }
    } else {
      len -= (size_t)w, ptr += w;
    }
  }
  return 1;
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
        RKT_glob.RK__OUT = meta_fd;
        int val          = setjmp(RKT_glob.fret);
        if (val == 0) {
          e->func();
        } else {
          /*todo jump*/
        }
        if (e->attrs.fini) { e->attrs.fini(); }
        if (s->attrs.fini) { s->attrs.fini(); }
        if (RKT_glob.attrs.fini) { RKT_glob.attrs.fini(); }
        exit(0);
      }
    }
  }
  exit(1); /*function not found?*/
}
# define RK_TEST_SPECIALCHILDENTRY() RKT_Windows_Childentry()

rkt_fun DWORD RKT_reapchild(rk_pid pid) {
  DWORD status;
  WaitForSingleObject(pid.hProcess, INFINITE);
  GetExitCodeProcess(pid.hProcess, &status);
  CloseHandle(pid.hProcess), CloseHandle(pid.hThread);
  return status;
}

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
  size_t extra = lenof("RK_CHILD_FN=") + e->name.len + 1
               + lenof("RK_CHILD_META=") + 32 + 1 + 1;
  char *nenv = RKT_malloc(char, extra + plen), *s = nenv;
  if (!nenv) { exit(1); }
  s    += RK_catlit(s, "RK_CHILD_FN=");
  s    += RK_catstr(s, e->name.str, e->name.len);
  *s++  = '\0';

  s    += sprintf(s, "RK_CHILD_META=%" PRIuPTR,
                  (uintptr_t)w_ends[RK_OUTPUTSTREAMS_META])
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

rkt_fun long RKT_now_ms(void) {
  static LARGE_INTEGER freq = {RKT_ZINIT};
  if (freq.QuadPart == 0) { QueryPerformanceFrequency(&freq); }
  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);
  return (long)((counter.QuadPart * 1000) / freq.QuadPart);
}

rkt_fun RKT_TestResult RKT_parent_loop(rk_pid pid, rk_fd* pipes,
                                       long timeout_ms) {
  size_t         rescap = 512, caps[2] = {512, 512};
  RKT_TestResult r = {RKT_ZINIT};
  r.results        = RKT_malloc(RKT_AssertDat, rescap);
  r.capt[0].str    = RKT_malloc(char, caps[0]);
  r.capt[1].str    = RKT_malloc(char, caps[1]);
  if (!r.results || !r.capt[0].str || !r.capt[1].str) {
    RKT_fatal("malloc buf");
  }
  long  time_start  = RKT_now_ms();
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
        r.results[r.count - 1].res = res;
      } else {
        memset(&r.results[r.count].hdr, 0, sizeof(RKT_AssertHdr));
        r.results[r.count++].res = res;
      }
      TerminateProcess(pid.hProcess, 1), RKT_reapchild(pid);
      CloseHandle(pipes[0]), CloseHandle(pipes[1]), CloseHandle(pipes[2]);
      return ++r.fails, r.ended = RKT_TIMEDOUT, r;
    }
    int idx = wait - WAIT_OBJECT_0;
    if (handles[idx] == pipes[RK_OUTPUTSTREAMS_META]) {
      if (!reading_res) {
        if (r.count + 1 == rescap) {
          rescap    *= 2;
          r.results  = RKT_realloc(RKT_AssertDat, r.results, rescap);
          if (!r.results) { RKT_fatal("realloc 0"); }
        }
        RKT_AssertHdr hdr = {RKT_ZINIT};
        switch (RKT_read_full_nb(handles[idx], &hdr, sizeof(hdr))) {
        case -1: RKT_fatal("read meta hdr");
        case 0 : handles[idx] = INVALID_HANDLE_VALUE; break;
        case 1 : r.results[r.count++].hdr = hdr, reading_res = 1;
        }
      } else {
        RKT_AssertRes res = {RKT_ZINIT};
        switch (
            RKT_read_full_nb(handles[idx], &res, sizeof(RKT_AssertResPkg))) {
        case -1: RKT_fatal("read meta res");
        case 0 : handles[idx] = INVALID_HANDLE_VALUE; break;
        case 1:
          if (res.len) {
            res.args[0] = RKT_malloc(char, res.len);
            if (!res.args[0]) { RKT_fatal("malloc"); }
            if (RKT_read_full_nb(handles[idx], res.args[0], res.len) < 1) {
              RKT_fatal("read OWO");
            }
            for (size_t i = 1, len = 0; len < res.len - 1; ++len) {
              if (!res.args[0][len]) {
                assert(i < 3);
                res.args[i++] = res.args[0] + len + 1;
              }
            }
          }
          if (res.res) { ++r.fails; }
          r.results[r.count - 1].res = res, reading_res = 0;
        }
      }
    } else {
      size_t i;
      if (handles[idx] == pipes[RK_OUTPUTSTREAMS_STDOUT]) {
        i = (size_t)RK_OUTPUTSTREAMS_STDOUT;
      } else if (handles[idx] == pipes[RK_OUTPUTSTREAMS_STDERR]) {
        i = (size_t)RK_OUTPUTSTREAMS_STDERR;
      } else {
        assert(0); // sanity check
      }
      DWORD rd;
      if (!ReadFile(handles[idx], r.capt[i].str + r.capt[i].len,
                    (DWORD)(caps[i] - r.capt[i].len), &rd, NULL)) {
        if (GetLastError() == ERROR_BROKEN_PIPE) {
          handles[idx] = INVALID_HANDLE_VALUE;
        } else {
          RKT_fatal("ReadFile stdout/stderr");
        }
      } else if (rd == 0) {
        handles[idx] = INVALID_HANDLE_VALUE;
      } else {
        r.capt[i].len += rd;
        if (r.capt[i].len == caps[i]) {
          caps[i]       *= 2;
          r.capt[i].str  = (char*)realloc(r.capt[i].str, caps[i]);
          if (!r.capt[i].str) { RKT_fatal("realloc stdout/stderr"); }
        }
      }
    }
    for (int i = nfds = 0; i < 3; ++i) {
      if (handles[i] != INVALID_HANDLE_VALUE) { handles[nfds++] = handles[i]; }
    }
  }
  CloseHandle(pipes[0]), CloseHandle(pipes[1]), CloseHandle(pipes[2]);
  r.term_code = RKT_reapchild(pid);
  if (reading_res) {
    RKT_AssertRes* lastres = &r.results[r.count - 1].res;
    memset(lastres, 0, sizeof(RKT_AssertRes));
    const RKT_AssertHdr* lasthdr = &r.results[r.count - 1].hdr;
    if (lasthdr->F == RKTF_CRASH || lasthdr->F == RKTF_EXIT) {
      if (!(lastres->args[0] = (char*)malloc(32))) { RKT_fatal("malloc"); }
      lastres->len = sprintf(lastres->args[0], "%lu", r.term_code) + 1;
      if (r.term_code != lasthdr->code) { lastres->res = 2; }
    } else {
      lastres->res = RKT_ASSERTRES_EXIT, r.ended = RKT_CRASHED;
    }
    if (lastres->res) { ++r.fails; }
  } else if (r.term_code) { // error outside of assertions
    r.ended = RKT_TESTERROR;
  }
  if (r.ended == RKT_PASSED && r.fails) { r.ended = RKT_FAILED; }
  return r;
}

rkt_fun int RKT_read_full_nb(rk_fd h, void* buf, size_t nbytes) {
  char* ptr = (char*)buf;
  for (DWORD r; nbytes;) {
    if (!ReadFile(h, ptr, (DWORD)nbytes, &r, NULL)) {
      switch (GetLastError()) {
      case ERROR_BROKEN_PIPE: return 0;
      case ERROR_NO_DATA    : continue;
      default               : return -1;
      }
    }
    if (!r) { return 0; }
    ptr += r, nbytes -= r;
  }
  return 1;
}
rkt_fun int RKT_write_full(rk_fd h, const void* buf, size_t len) {
  const char* ptr = (const char*)buf;
  for (DWORD w; len > 0; ptr += w, len -= w) {
    if (!WriteFile(h, ptr, (DWORD)len, &w, NULL)) { return -1; }
  }
  return 1;
}

#endif

// to debug the testing framework - compare test result function to expected
#define RKT_VALIDATE(FUNNAME, RESULT)                                          \
  //RK_REGISTER_TEST("testmeta", test_##FUNNAME) {                               \
  //  RKT_TestEntry* e = &RKT_CONCAT(RKT_entry_, FUNNAME);                       \
  //  if (e->suite->attrs.init) { e->suite->attrs.init(); }                      \
  //  if (!e->res.count) {                                                       \
  //    rk_assert_eq(RKT_run_test(e), RESULT);                                   \
  //    RKT_cleanup_test_res(e);                                                 \
  //  } else {                                                                   \
  //    rk_assert_eq(e->res.ended, RESULT);                                      \
  //  }                                                                          \
  //  if (e->suite->attrs.fini) { e->suite->attrs.fini(); }                      \
  //}

#undef RK_catlit
#undef RK_test_GENFUNS_nofloat
#undef RK_test_GENFUNS_float
#undef RK_INRED
#undef RK_INGREEN
#undef RK_INBLUE
#undef RK_INYELLOW

// typedef struct RKT_Suite {
//   RKT_Strv              name;
//   RKT_CustomAttrs       attrs;
//   struct RKT_TestEntry* tests;
//   struct RKT_Suite*     next;
// } RKT_Suite;
/// @brief todo documentation
#define RKT_REGISTER_SUITE_IMPL(SUITENAME, ...)                                \
  RKT_CONSTRUCTOR(RKT_CONCAT(RK_test_suite_register_, __COUNTER__)) {          \
    RKT_CustomAttrs attrs = {__VA_ARGS__};                                     \
    RKT_Suite*      s;                                                         \
    for (s = RKT_glob.suites; s; s = s->next) {                                \
      if (!strcmp(s->name.str, SUITENAME)) {                                   \
        s->attrs = attrs;                                                      \
        return;                                                                \
      }                                                                        \
    }                                                                          \
    static RKT_Suite suite                                                     \
        = {{SUITENAME, lenof(SUITENAME)}, {RKT_ZINIT}, NULL, NULL};            \
    suite.attrs = attrs;                                                       \
    if (!RKT_glob.suites) {                                                    \
      RKT_glob.suites = &suite;                                                \
    } else {                                                                   \
      for (s = RKT_glob.suites; s->next; s = s->next);                         \
      s->next = &suite;                                                        \
    }                                                                          \
  }

/// @brief todo documentation
#define RKT_REGISTER_TEST_IMPL(SUITENAME, fn, ...)                             \
  static void          fn();                                                   \
  static RKT_TestEntry RKT_CONCAT(RKT_entry_, fn)                              \
      = {fn,                                                                   \
         {__FILE__, lenof(__FILE__)},                                          \
         {#fn, lenof(#fn)},                                                    \
         {__VA_ARGS__},                                                        \
         NULL,                                                                 \
         NULL,                                                                 \
         {RKT_ZINIT}};                                                         \
  RKT_CONSTRUCTOR(RK_test_register_##fn) {                                     \
    RKT_TestEntry* entry = &(RKT_CONCAT(RKT_entry_, fn));                      \
    RKT_Suite*     s;                                                          \
    for (s = RKT_glob.suites; s; s = s->next) {                                \
      if (!strcmp(s->name.str, SUITENAME)) { /*todo strncmp?*/                 \
        entry->suite = s;                                                      \
        if (!s->tests) {                                                       \
          s->tests = entry;                                                    \
          return;                                                              \
        }                                                                      \
        RKT_TestEntry* e;                                                      \
        for (e = s->tests; e->next; e = e->next);                              \
        e->next = entry;                                                       \
        return;                                                                \
      }                                                                        \
    }                                                                          \
    static RKT_Suite suite                                                     \
        = {{SUITENAME, lenof(SUITENAME)}, {RKT_ZINIT}, NULL, NULL};            \
    suite.tests = entry;                                                       \
    if (!RKT_glob.suites) {                                                    \
      RKT_glob.suites = &suite;                                                \
    } else {                                                                   \
      for (s = RKT_glob.suites; s->next; s = s->next);                         \
      s->next = &suite;                                                        \
    }                                                                          \
    entry->suite = &suite;                                                     \
  }                                                                            \
  static void fn()

/// @brief todo documentation
#define RKT_RUN_TESTS_IMPL(suites, nsuites, ...)                               \
  do {                                                                         \
    RK_TEST_SPECIALCHILDENTRY()                                                \
    RKT_CustomAttrs attrs = {__VA_ARGS__};                                     \
    RKT_run_all_tests(suites, nsuites, attrs);                                 \
  } while (0)

#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif
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
