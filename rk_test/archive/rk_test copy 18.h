
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

#ifdef __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
# pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
# pragma GCC diagnostic ignored "-Wc2x-extensions"
#endif
#ifdef _WIN32
# pragma section(".CRT$XCU", read)
# include <inttypes.h>
# include <windows.h>
#else
# include <fcntl.h>
# include <poll.h>
# include <signal.h>
# include <sys/ioctl.h>
# include <sys/stat.h>  // defines struct stat and stat/fstat functions
# include <sys/types.h> // defines types like off_t
# include <sys/wait.h>
# include <unistd.h> // defines fstat() and other POSIX functions
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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define RKT_CONCAT(a, b)  RKT_CONCAT2(a, b)
#define RKT_CONCAT2(a, b) a##b
/// @brief todo documentation
#define RK_REGISTER_SUITE(SUITENAME, ...)                                      \
    RKT_CONSTRUCTOR(RKT_CONCAT(RK_test_suite_register_, __COUNTER__)) {        \
        RKT_CustomAttrs attrs = {__VA_ARGS__};                                 \
        RKT_Suite*      s;                                                     \
        for (s = RKT_glob.suites; s; s = s->next) {                            \
            if (!strcmp(s->name, SUITENAME)) {                                 \
                s->attrs = attrs;                                              \
                return;                                                        \
            }                                                                  \
        }                                                                      \
        static RKT_Suite suite;                                                \
        suite.name = SUITENAME, suite.attrs = attrs;                           \
        if (!RKT_glob.suites) {                                                \
            RKT_glob.suites = &suite;                                          \
        } else {                                                               \
            for (s = RKT_glob.suites; s->next; s = s->next);                   \
            s->next = &suite;                                                  \
        }                                                                      \
    }

/// @brief todo documentation
#define RK_REGISTER_TEST(SUITENAME, fn, ...)                                   \
    static void fn(rk_fd RK_OUT);                                              \
    RKT_CONSTRUCTOR(RK_test_register_##fn) {                                   \
        static RKT_TestEntry entry = {fn,                                      \
                                      {__FILE__, lenof(__FILE__)},             \
                                      {#fn, lenof(#fn)},                       \
                                      {__VA_ARGS__},                           \
                                      NULL,                                    \
                                      NULL};                                   \
        RKT_Suite*           s;                                                \
        for (s = RKT_glob.suites; s; s = s->next) {                            \
            if (!strcmp(s->name, SUITENAME)) {                                 \
                entry.suite = s;                                               \
                if (!s->tests) {                                               \
                    s->tests = &entry;                                         \
                    return;                                                    \
                }                                                              \
                RKT_TestEntry* e;                                              \
                for (e = s->tests; e->next; e = e->next);                      \
                e->next = &entry;                                              \
                return;                                                        \
            }                                                                  \
        }                                                                      \
        static RKT_Suite suite = {0};                                          \
        suite.name = SUITENAME, suite.tests = &entry;                          \
        entry.suite = &suite;                                                  \
        if (!RKT_glob.suites) {                                                \
            RKT_glob.suites = &suite;                                          \
        } else {                                                               \
            for (s = RKT_glob.suites; s->next; s = s->next);                   \
            s->next = &suite;                                                  \
        }                                                                      \
    }                                                                          \
    static void fn(rk_fd RK_OUT)

/// @brief todo documentation
#define RK_RUN_TESTS(suites, nsuites, ...)                                     \
    do {                                                                       \
        RK_TEST_SPECIALCHILDENTRY()                                            \
        RKT_CustomAttrs attrs = {__VA_ARGS__};                                 \
        RKT_run_all_tests(suites, nsuites, attrs);                             \
                                                                               \
    } while (0)

// clang-format off
#define rk_expect_true(EXPR_)                  RK_EXPECT(RKTf_true, RKTF_TRUE, "rk_expect_true("#EXPR_")", EXPR_)
#define rk_expect_false(EXPR_)                 RK_EXPECT(RKTf_false, RKTF_FALSE, "rk_expect_false("#EXPR_")", EXPR_)
#define rk_expect_null(PTR_)                   RK_EXPECT(RKTf_null, RKTF_NULL, "rk_expect_null("#PTR_")", PTR_)
#define rk_expect_nonnull(PTR_)                RK_EXPECT(RKTf_nnull, RKTF_NNULL, "rk_expect_nonnull("#PTR_")", PTR_)
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
#define rk_expect_strneq(str1, str2)          RK_EXPECT(RKTf_strneq, RKTF_STRNEQ,"strneq(" #str1 ", " #str2 ")",str1,str2)                                    \

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

// todo not implemented
#define rk_assert_stdouteq(errstr, ...)  RK_test_sendhdr("stderror prints " #errstr, RK_TEST_STDOUTEQ, RK_TEST_T_vp)
#define rk_assert_stdoutneq(errstr, ...) RK_test_sendhdr("stderror prints " #errstr, RK_TEST_STDOUTNEQ, RK_TEST_T_vp)
#define rk_assert_stderreq(errstr, ...)  RK_test_sendhdr("stderror prints " #errstr, RK_TEST_STDERREQ, RK_TEST_T_vp)
#define rk_assert_stderrneq(errstr, ...) RK_test_sendhdr("stderror prints " #errstr, RK_TEST_STDERRNEQ, RK_TEST_T_vp)

// clang-format on
#define rk_assert_crash(signal, ...)                                           \
    do {                                                                       \
        RK_test_sendhdr_sig("assert_crash(" #signal ", " #__VA_ARGS__ ")",     \
                            RKTF_CRASH, signal);                               \
        __VA_ARGS__;                                                           \
        {                                                                      \
            RKT_AssertResPkg RK_PKG = {0, 1};                                  \
            RKT_write_full(RK_OUT, &RK_PKG, sizeof(RK_PKG));                   \
        }                                                                      \
        exit(0);                                                               \
    } while (0)

#define rk_assert_exit(code, ...)                                              \
    do {                                                                       \
        RK_test_sendhdr_sig("rk_assert_exit(" #code ", " #__VA_ARGS__ ")",     \
                            RKTF_EXIT, code);                                  \
        __VA_ARGS__;                                                           \
        {                                                                      \
            RKT_AssertResPkg RK_PKG = {0, 1};                                  \
            RKT_write_full(RK_OUT, &RK_PKG, sizeof(RK_PKG));                   \
        }                                                                      \
        exit(0);                                                               \
    } while (0)

#if defined(__cplusplus) && __cplusplus >= 201103L
# define RKT_noreturn [[noreturn]]
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
# include <stdnoreturn.h>
# define RKT_noreturn noreturn
#elif defined(_MSC_VER)
# define RKT_noreturn __declspec(noreturn)
#elif defined(__GNUC__)
# define RKT_noreturn __attribute__((noreturn))
#else
# define RKT_noreturn
#endif

#if !defined(restrict) && (defined(_MSC_VER) || defined(__cplusplus))
# define restrict __restrict
#endif

#ifndef unreachable
# ifdef __GNUC__
#  define unreachable() __builtin_unreachable()
# elif defined(_MSC_VER)
#  define unreachable() __assume(false)
# else
#  define unreachable() (assert(0 && "unreachable code reached"), abort())
# endif
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

#define lenof(strlit)           (sizeof("" strlit "") - 1)
#define RK_catlit(s, l)         (memcpy(s, l, lenof(l)), lenof(l))
#define RK_catstr(S, STR, LEN)  (memcpy(S, STR, LEN), LEN)

#define RK_MIN(x, y)            ((x) <= (y) ? (x) : (y))
#define RK_MAX(x, y)            ((x) >= (y) ? (x) : (y))

#define RKT_malloc(TYPE, COUNT) ((TYPE*)malloc(sizeof(TYPE) * COUNT))
#define RKT_realloc(TYPE, ptr, COUNT)                                          \
    ((TYPE*)realloc(ptr, sizeof(TYPE) * COUNT))

#define RK_TEST_TYPELIST_NOFLOAT(Y, ...)                                       \
    Y(unsigned char, uc, "%hhu", ##__VA_ARGS__)                                \
    Y(unsigned short, us, "%hu", ##__VA_ARGS__)                                \
    Y(unsigned, ui, "%u", ##__VA_ARGS__)                                       \
    Y(unsigned long, ul, "%lu", ##__VA_ARGS__)                                 \
    Y(unsigned long long, ull, "%llu", ##__VA_ARGS__)                          \
    Y(char, c, "%c", ##__VA_ARGS__)                                            \
    Y(short, s, "%hd", ##__VA_ARGS__)                                          \
    Y(int, i, "%d", ##__VA_ARGS__)                                             \
    Y(long, sl, "%ld", ##__VA_ARGS__)                                          \
    Y(long long, sll, "%lld", ##__VA_ARGS__)                                   \
    Y(bool, b, "%d", ##__VA_ARGS__)                                            \
    Y(const void*, vp, "%p", ##__VA_ARGS__)
#define RK_TEST_TYPELIST_FLOAT(Y, ...)                                         \
    Y(float, f, "%f", ##__VA_ARGS__)                                           \
    Y(double, d, "%f", ##__VA_ARGS__)                                          \
    Y(long double, ld, "%Lf", ##__VA_ARGS__)
#define RK_TEST_TYPELIST(Y, ...)                                               \
    RK_TEST_TYPELIST_FLOAT(Y, ##__VA_ARGS__)                                   \
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

typedef enum RKT_OutPutType {
    RKT_PIPE_STDOUT, // captured stdout of the child process
    RKT_PIPE_STDERR, // captured stderr of the child process
    RKT_PIPE_META,   // assertion information
} RKT_OutPutType;

typedef enum RKT_Verbosity {
    RK_TEST_LOG_GLOB_DEFAULT,
    RK_TEST_LOG_DEFAULT,
    RK_TEST_LOG_ALWAYS,
    RK_TEST_LOG_NEVER
} RKT_Verbosity;

typedef enum RKT_TestEndType {
    RKT_PASSED,
    RKT_FAILED,
    RKT_CRASHED,
    RKT_TIMEDOUT,
    RKT_TESTERROR
} RKT_TestEndType;
// todo clearer semantics
// todo only for children: 'Follow parent'

/*

RK_TEST_LOG_GLOB_DEFAULT: If defined globally, same as RK_TEST_LOG_DEFAULT,
otherwise, follow global settings


RKT_PIPE_STDOUT:
 - RK_TEST_LOG_DEFAULT: prints output only if test fails
 - RK_TEST_LOG_ALWAYS:  always prints output
 - RK_TEST_LOG_NEVER:   never prints output

RKT_PIPE_STDERR:
 - RK_TEST_LOG_DEFAULT: always prints output
 - RK_TEST_LOG_ALWAYS:  always prints output
 - RK_TEST_LOG_NEVER:   never prints output

RKT_PIPE_META:s
 - RK_TEST_LOG_DEFAULT: prints output only if test fails
 - RK_TEST_LOG_ALWAYS:  always prints output
 - RK_TEST_LOG_NEVER:   never prints output (not recommended)
*/

/// @brief customisable attriutes (global, suite or test-level)
typedef struct RKT_CustomAttrs {
    const char* tags;
    void (*init)(void), (*fini)(void);
    const char* custom_paths[3];
    RKT_Verbosity
         verbosity_levels[3]; /// 0: relegate to higher level
                              /// (test->suite->global), with global todo
    long timeout_ms; ///< 0: relegate to higher level (test->suite->global, with
                     ///< global defaulted as -1 (no timeout))
} RKT_CustomAttrs;

typedef struct RKT_Strv {
    char*  str;
    size_t len;
} RKT_Strv;

/// @brief Each test function has one
typedef struct RKT_TestEntry {
    void (*const func)(rk_fd RK_OUT);
    RKT_Strv              file;
    RKT_Strv              name;
    RKT_CustomAttrs       attrs;
    struct RKT_Suite*     suite;
    struct RKT_TestEntry* next;
} RKT_TestEntry;

typedef struct RKT_Suite {
    const char*       name;
    RKT_CustomAttrs   attrs;
    RKT_TestEntry*    tests;
    struct RKT_Suite* next;
} RKT_Suite;

/// @brief Global settings/variables for the testing framework
static struct {
    RKT_CustomAttrs attrs;
    FILE*           files[3];      // todo rename to default files?
    char            resbufr[2048]; // buffer for optimisation
    RKT_Suite*      suites;
} RKT_glob;

/// @brief Header to be sent to the test runner before assert/expect
typedef struct RKT_AssertHdr {
    char        exprstr[244]; ///< The stringified assertion
    int         pos;          ///< The line of the function
    RK_test_FUN F;            ///< The assert function (assert_eq, assert_true)
    int         code;         ///< The expected error code (if any)
} RKT_AssertHdr;

/// @brief the arguments and result (true/false or other state) is stored here
typedef struct RKT_AssertResPkg {
    size_t len; ///< length of the args[] section
    size_t res; ///< Result of the test (0 is success)
    char   args[];
} RKT_AssertResPkg;

/// @brief the arguments and result (true/false or other state) is stored here
typedef struct RKT_AssertRes {
    size_t len;
    size_t res;
    char*  args[4];
} RKT_AssertRes;

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

RKT_noreturn static inline void RKT_fatal(rk_pid pid, const char* str);
static inline int  RKT_write_full(rk_fd fd, const void* buf, size_t len);
static inline int  RKT_read_full_nb(rk_fd fd, void* buf, size_t nbytes);
static inline void RKT_setup_files(const RKT_TestEntry* restrict e, rk_pid pid,
                                   FILE** restrict files);
static inline RKT_TestEndType RKT_run_test(const RKT_TestEntry* t);
static inline rk_pid          RKT_init_test(const RKT_TestEntry* restrict e,
                                            rk_fd* restrict pipes,
                                            FILE** restrict files);
static inline RKT_TestResult  RKT_parent_loop(rk_pid pid, rk_fd* pipes,
                                              long timeout_ms);
static inline RKT_TestEndType RKT_print_testres(const RKT_TestEntry* restrict e,
                                                RKT_TestResult t,
                                                FILE** restrict files);
static inline size_t RKT_print_assertres(const RKT_TestEntry* restrict e,
                                         RKT_AssertDat cur,
                                         char** restrict fmtbuf,
                                         size_t* restrict fmtcap);

#ifndef __cplusplus
# define RK_test_sendhdr_sig(EXPRSTR, FUNENUM, sig)                            \
     RKT_write_full(RK_OUT,                                                    \
                    (RKT_AssertHdr[]){{EXPRSTR, __LINE__, FUNENUM, sig}},      \
                    sizeof(RKT_AssertHdr))
#else
# define RK_test_sendhdr_sig(EXPRSTR, FUNENUM, sig)                            \
     [](rk_fd _fd) {                                                           \
      RKT_AssertHdr hdr = {EXPRSTR, __LINE__, FUNENUM, sig};                   \
      RKT_write_full(_fd, &hdr, sizeof(hdr));                                  \
     }(RK_OUT)
#endif
#define RK_test_sendhdr(EXPRSTR, FUNENUM)                                      \
    RK_test_sendhdr_sig(EXPRSTR, FUNENUM, 0)

#define RK_EXPECT(FUN, FUNENUM, EXPRSTR, ...)                                  \
    (RK_test_sendhdr(EXPRSTR, FUNENUM), FUN(RK_OUT, __VA_ARGS__))

#define RK_ASSERT(FUN, FUNENUM, EXPRSTR, ...)                                  \
    (RK_test_sendhdr(EXPRSTR, FUNENUM),                                        \
     (FUN(RK_OUT, __VA_ARGS__) ? (fflush(stdout), fflush(stderr), _exit(0))    \
                               : ((void)0)))

/// @brief returns first position where strings differ +1 (0 if same)
static inline size_t RK_test_strdiff(const char* e1, size_t l1, const char* e2,
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
     _Generic((VAL)RK_TEST_TYPELIST(RKT_SELFUN_, FTYPE),                       \
         default: RKTf_##FTYPE##_vp)

// todo really wrong
# define RKT_SELFUN_TOL_(T, N, F, FTYPE) , T : RKTf_##FTYPE##_##N
# define RKT_SELFUN_TOL(FTYPE, EXP)                                            \
     _Generic((EXP)RK_TEST_TYPELIST_FLOAT(RKT_SELFUN_TOL_, FTYPE))

# define RK_test_GENFUN2_sig(T, N, FMT, name)                                  \
     static inline size_t RKTf_##name##_##N(rk_fd fd, T e1, T e2)
# define RK_test_GENFUN3_sig(T, N, FMT, name)                                  \
     static inline size_t RKTf_##name##_##N(rk_fd fd, T e1, T e2, T e3)

#else
# define RKT_SELFUN(FTYPE, VAL)     RKTf_##FTYPE
# define RKT_SELFUN_TOL(FTYPE, ...) RKTf_##FTYPE
# define RK_test_GENFUN2_sig(T, N, FMT, name)                                  \
     static inline size_t RKTf_##name(rk_fd fd, T e1, T e2)
# define RK_test_GENFUN3_sig(T, N, FMT, name)                                  \
     static inline size_t RKTf_##name(rk_fd fd, T e1, T e2, T e3)

# if __cpp_lib_format >= 201907L // C++20 std::format available
#  include <format>
template <typename T, typename CharT = char>
concept RK_formattable = requires (T const& value) {
    std::formatter<T, CharT>{};
    std::format(std::basic_string<CharT>{"{}"}, value);
};
# endif

template <typename...>
using RK_void_t = void;
template <typename T, typename U = void>
struct RK_is_streamable : std::false_type {};
template <typename T>
struct RK_is_streamable<
    T, RK_void_t<decltype(std::declval<std::ostream&>() << std::declval<T>())>>
    : std::true_type {};

class RK_test_streambuf : public std::streambuf {
  public:
    std::vector<char> buf;
    RK_test_streambuf() { buf.resize(hdr_size, 0); }

    void write_full(rk_fd fd, size_t res_val) {
        size_t siz = buf.size() - hdr_size;
        memcpy(&buf[0], &siz, sizeof(siz));
        memcpy(&buf[sizeof(siz)], &res_val, sizeof(res_val));
        RKT_write_full(fd, buf.data(), buf.size());
    }
    void set_header(size_t res_val) {
        size_t siz = buf.size() - hdr_size;
        memcpy(&buf[0], &siz, sizeof(size_t));
        memcpy(&buf[sizeof(size_t)], &res_val, sizeof(size_t));
    }

  protected:
    static constexpr const size_t hdr_size = sizeof(size_t) * 2;

    virtual std::streamsize xsputn(const char* s, std::streamsize n) override {
        buf.insert(buf.end(), s, s + n);
        return n;
    }
    virtual int_type overflow(int_type ch) override {
        if (ch != traits_type::eof()) { buf.push_back(ch); }
        return ch;
    }
};

template <class T>
void RK_test_to_string(std::ostream& oss, const T& val) {
    // # if __cpp_lib_format >= 201907L
    //     if (RK_formattable<T>) {
    //         oss << std::format("{}", val);
    //     } else
    // # endif
    // todo
    if (RK_is_streamable<T>::value) {
        oss << val; // todo
    } else {
        oss << static_cast<const void*>(&val);
    }
    oss.put('\0');
}

// todo undef
# define RK_CPP_GENFUNS2(F)                                                    \
     F(==, eq)                                                                 \
     F(!=, neq)                                                                \
     F(>, gt)                                                                  \
     F(>=, geq)                                                                \
     F(<, lt)                                                                  \
     F(<=, leq)
# define RK_CPP_GENFUN(OP, OPNAME, ...)                                        \
     template <class T, class U>                                               \
     static inline size_t RKTf_##OPNAME(rk_fd fd, T e1, U e2) {                \
         RK_test_streambuf buf;                                                \
         std::ostream      oss(&buf);                                          \
         RK_test_to_string(oss, e1), RK_test_to_string(oss, e2);               \
         size_t res = !(e1 OP e2);                                             \
         buf.write_full(fd, (size_t)res);                                      \
         return res;                                                           \
     }

RK_CPP_GENFUNS2(RK_CPP_GENFUN)

template <class T, class U, class O>
static inline size_t RKTf_inrange(rk_fd fd, T e1, U e2, O e3) {
    RK_test_streambuf buf;
    std::ostream      oss(&buf);
    RK_test_to_string(oss, e1);
    RK_test_to_string(oss, e2);
    RK_test_to_string(oss, e3);
    size_t res = (e1 < e2 ? 1 : (e1 > e3 ? 2 : 0)); // todo
    buf.write_full(fd, (size_t)res);
    return res;
}
#endif

#define construct_send_buf0(fd, RES)                                           \
    RKT_AssertResPkg* _res = (RKT_AssertResPkg*)RKT_glob.resbufr;              \
    _res->res = (RES), _res->len = 0;                                          \
    RKT_write_full(fd, _res, sizeof(*_res) + _res->len);                       \
    return (RES);
#define construct_send_buf1(fd, RES, arg1, FMT1)                               \
    RKT_AssertResPkg* _res = (RKT_AssertResPkg*)RKT_glob.resbufr;              \
    _res->res              = (RES);                                            \
    _res->len              = sprintf(_res->args, FMT1, e1) + 1;                \
    RKT_write_full(fd, _res, sizeof(*_res) + _res->len);                       \
    return (RES);
#define construct_send_buf2(fd, RES, arg1, FMT1, arg2, FMT2)                   \
    RKT_AssertResPkg* _res  = (RKT_AssertResPkg*)RKT_glob.resbufr;             \
    _res->res               = (RES);                                           \
    _res->len               = sprintf(_res->args, FMT1, e1) + 1;               \
    _res->len              += sprintf(_res->args + _res->len, FMT2, e2) + 1;   \
    RKT_write_full(fd, _res, sizeof(*_res) + _res->len);                       \
    return (RES);
#define construct_send_buf3(fd, RES, arg1, FMT1, arg2, FMT2, arg3, FMT3)       \
    RKT_AssertResPkg* _res  = (RKT_AssertResPkg*)RKT_glob.resbufr;             \
    _res->res               = (RES);                                           \
    _res->len               = sprintf(_res->args, FMT1, e1) + 1;               \
    _res->len              += sprintf(_res->args + _res->len, FMT2, e2) + 1;   \
    _res->len              += sprintf(_res->args + _res->len, FMT3, e3) + 1;   \
    RKT_write_full(fd, _res, sizeof(*_res) + _res->len);                       \
    return (RES);

#define RK_test_GENFUN2(T, N, FMT, name, expr)                                 \
    RK_test_GENFUN2_sig(T, N, FMT, name) {                                     \
        construct_send_buf2(fd, (expr), e1, FMT, e2, FMT);                     \
    }
#define RK_test_GENFUN3(T, N, FMT, name, expr)                                 \
    RK_test_GENFUN3_sig(T, N, FMT, name) {                                     \
        construct_send_buf3(fd, (expr), e1, FMT, e2, FMT, e3, FMT);            \
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

static inline size_t RKTf_true(rk_fd fd, bool e1) {
    construct_send_buf0(fd, !e1);
}
// clang-format on
static inline size_t RKTf_false(rk_fd fd, bool e1) {
    construct_send_buf0(fd, !!e1);
}
static inline size_t RKTf_null(rk_fd fd, const void* e1) {
    construct_send_buf1(fd, !(e1 == NULL), e1, "%p");
}
static inline size_t RKTf_nnull(rk_fd fd, const void* e1) {
    construct_send_buf1(fd, !(e1 != NULL), e1, "%p");
}
static inline size_t RKTf_memeq(rk_fd fd, const void* e1, const void* e2,
                                size_t e3) {
    size_t _result = !((e1 == e2) || (e1 && e2 && !memcmp(e1, e2, e3)));
    construct_send_buf3(fd, _result, e1, "%p", e2, "%p", e3, "%zu");
}
static inline size_t RKTf_memneq(rk_fd fd, const void* e1, const void* e2,
                                 size_t e3) {
    size_t _result = !!((e1 == e2) || (e1 && e2 && !memcmp(e1, e2, e3)));
    construct_send_buf3(fd, _result, e1, "%p", e2, "%p", e3, "%zu");
}
static inline size_t RKTf_memzero(rk_fd fd, const void* e1, size_t e2) {
    size_t dif = 0;
    for (size_t i = 0; i < e2; ++i) {
        if (((const char*)e1)[i]) {
            dif = i + 1;
            break;
        }
    }
    construct_send_buf2(fd, dif, e1, "%p", e2, "%zu");
}

static inline size_t RKTf_memnzero(rk_fd fd, const void* e1, size_t e2) {
    const void* loc;
    size_t      result = (!e2 || !(loc = memchr(e1, 0, e2)))
                           ? 0
                           : ((char*)loc - (char*)e1 + 1);
    construct_send_buf2(fd, result, e1, "%p", e2, "%zu");
}

static inline size_t RKTf_streq(rk_fd fd, const char* e1, const char* e2) {
    RKT_AssertResPkg* res;
    size_t            l1 = strlen(e1), l2 = strlen(e2);
    size_t            dif = RK_test_strdiff(e1, l1, e2, l2);
    if (l1 + l2 + 2 <= sizeof(RKT_glob.resbufr)) {
        res = (RKT_AssertResPkg*)RKT_glob.resbufr;
    } else {
        res = (RKT_AssertResPkg*)malloc(sizeof(RKT_AssertResPkg) + l1 + l2 + 2);
        if (!res) { exit(1); }
    }
    res->res = dif, res->len = l1 + l2 + 2;
    memcpy(res->args, e1, l1 + 1), memcpy(res->args + l1 + 1, e2, l2 + 1);
    RKT_write_full(fd, res, sizeof(*res) + res->len);
    if (l1 + l2 + 2 > sizeof(RKT_glob.resbufr)) { free(res); }
    return dif;
}

static inline size_t RKTf_strneq(rk_fd fd, const char* e1, const char* e2) {
    RKT_AssertResPkg* res;
    size_t            l1 = strlen(e1), l2 = strlen(e2);
    size_t            dif = RK_test_strdiff(e1, l1, e2, l2);
    if (l1 + l2 + 2 <= sizeof(RKT_glob.resbufr)) {
        res = (RKT_AssertResPkg*)RKT_glob.resbufr;
    } else {
        res = (RKT_AssertResPkg*)malloc(sizeof(RKT_AssertResPkg) + l1 + l2 + 2);
        if (!res) { exit(1); }
    }
    res->res = !(dif != 0), res->len = l1 + l2 + 2;
    memcpy(res->args, e1, l1 + 1), memcpy(res->args + l1 + 1, e2, l2 + 1);
    RKT_write_full(fd, res, sizeof(*res) + res->len);
    if (l1 + l2 + 2 > sizeof(RKT_glob.resbufr)) { free(res); }
    return !(dif != 0);
}

// PARENT FUNCTIONS
static inline void RKT_run_suite(const RKT_Suite* s) {
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
    printf("--- Passed %zu, failed %zu, crashed %zu, timed-out %zu out of %zu "
           "tests ---\n",
           total - failed - crashed, failed, crashed, timed_out, total);
}
static inline void RKT_run_all_tests(const char**    suites_strs,
                                     size_t          nsuites_strs,
                                     RKT_CustomAttrs attrs) {
    FILE* files_default[] = {stdout, stderr, stdout};
    RKT_glob.attrs        = attrs;
    for (int i = 0; i < 3; ++i) {
        if (!attrs.custom_paths[i]) {
            RKT_glob.files[i] = files_default[i];
        } else if (!(RKT_glob.files[i] = fopen(attrs.custom_paths[i], "w"))) {
            perror("fopen"), exit(1); // todo better error
        }
    }
    if (!RKT_glob.attrs.timeout_ms) { RKT_glob.attrs.timeout_ms = -1; }
    if (RKT_glob.attrs.init) { RKT_glob.attrs.init(); }
    for (RKT_Suite* s = RKT_glob.suites; s; s = s->next) {
        if (nsuites_strs) {
            for (size_t i = 0; i < nsuites_strs; ++i) {
                if (!strcmp(suites_strs[i], s->name)) { goto runsuite; }
            }
            continue;
        }
    runsuite:
        RKT_run_suite(s);
    }
    if (RKT_glob.attrs.fini) { RKT_glob.attrs.fini(); }
    // printf("--- Passed %zu, fails %zu, crashed %zu out of %zu tests ---\n",
    //        total - failed - crashed, failed, crashed, total);
}
static inline long RKT_resolve_timeout(const RKT_TestEntry* restrict e) {
    if (!e->attrs.timeout_ms) {
        if (!e->suite->attrs.timeout_ms) { return RKT_glob.attrs.timeout_ms; }
        return e->suite->attrs.timeout_ms;
    }
    return e->attrs.timeout_ms;
}
static inline RKT_Verbosity
    RKT_resolve_verbosity(const RKT_TestEntry* restrict e,
                          RKT_OutPutType type) {
    if (e->attrs.verbosity_levels[type] == RK_TEST_LOG_GLOB_DEFAULT) {
        if (e->suite->attrs.verbosity_levels[type]
            == RK_TEST_LOG_GLOB_DEFAULT) {
            return RKT_glob.attrs.verbosity_levels[type];
        }
        return e->suite->attrs.verbosity_levels[type];
    }
    return e->attrs.verbosity_levels[type];
}

#define RK_RED           "\x1b[31m"
#define RK_RESET         "\x1b[0m"
#define RK_GREEN         "\x1b[32m"
#define RK_INRED(str)    RK_RED str RK_RESET
#define RK_INGREEN(str)  RK_GREEN str RK_RESET
#define RK_INBLUE(str)   "\x1b[32m" str "\x1b[0m"
#define RK_INYELLOW(str) "\x1b[33m" str "\x1b[0m"
#define RK_TFMT_P        RK_INGREEN("[P] ")
#define RK_TFMT_F        RK_INRED("[F] ")
#define RK_TFMT_C        RK_INRED("[C] ")
#define RK_TFMT_E        RK_INRED("[E] ")
#define RK_TFMT_T        RK_INRED("[T] ")

static inline RKT_TestEndType RKT_run_test(const RKT_TestEntry* e) {
    printf(RK_INYELLOW("Running %s\n") "---------------------------------------"
                                       "-----------\n",
           e->name.str); // todo print to meta file
    rk_fd          pipes[3];
    FILE*          files[3];
    rk_pid         pid = RKT_init_test(e, pipes, files);
    RKT_TestResult results
        = RKT_parent_loop(pid, pipes, RKT_resolve_timeout(e));

    RKT_TestEndType res = RKT_print_testres(e, results, files);
    while (results.count--) {
        free(results.results[results.count].res.args[0]);
    }
    free(results.capt[0].str), free(results.capt[1].str);
    free(results.results);
    for (int i = 0; i < 3; ++i) {
        if (e->attrs.custom_paths[i] || e->suite->attrs.custom_paths[i]) {
            fclose(files[i]);
        }
    }
    return res;
}

static inline void RKT_setup_files(const RKT_TestEntry* restrict e, rk_pid pid,
                                   FILE** restrict files) {
    for (int i = 0; i < 3; ++i) {
        if (e->attrs.custom_paths[i]) {
            if (!(files[i] = fopen(e->attrs.custom_paths[i], "w"))) {
                RKT_fatal(pid, "fopen"); // todo better error message
            }
        } else if (e->suite->attrs.custom_paths[i]) {
            if (!(files[i] = fopen(e->suite->attrs.custom_paths[i], "w"))) {
                RKT_fatal(pid, "fopen"); // todo better error message
            }
        } else {
            files[i] = RKT_glob.files[i];
        }
    }
}

static inline size_t RK_fmt_stub(char* restrict s,
                                 const RKT_TestEntry* restrict e,
                                 const RKT_AssertHdr hdr) {
    char* _s  = s;
    s        += RK_catstr(s, e->file.str, e->file.len);
    *s++      = ':';
    s        += sprintf(s, "%d ", hdr.pos);
    s        += RK_catstr(s, hdr.exprstr, strlen(hdr.exprstr));
    return s - _s;
}
#define RK_catstub(S, PASSTYPE, E, HDR)                                        \
    (RK_catlit((S), PASSTYPE) + RK_fmt_stub(S + lenof(PASSTYPE), E, HDR))

static inline size_t RKT_print_assertres(const RKT_TestEntry* restrict e,
                                         RKT_AssertDat cur,
                                         char** restrict fmtbuf,
                                         size_t* restrict fmtcap) {
    RKT_AssertRes res  = cur.res;
    char**        args = res.args;
    RKT_AssertHdr hdr  = cur.hdr;
    char*         s    = *fmtbuf;

    switch (res.res) {
    case RKT_ASSERTRES_PASS:
        if (RKT_resolve_verbosity(e, RKT_PIPE_META) == RK_TEST_LOG_ALWAYS) {
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
            if (!*fmtbuf) {
                fputs("Error: Realloc fmt failed\n", stderr), exit(1);
            }
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
            break; // todo
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
}

static inline int RKT_interpret_code(int code) {
    if (WIFEXITED(code)) {
        return WEXITSTATUS(code);
    } else if (WIFSIGNALED(code)) {
        return WTERMSIG(code);
    }
    return code;
}
static inline RKT_TestEndType RKT_print_testres(const RKT_TestEntry* restrict e,
                                                RKT_TestResult r,
                                                FILE** restrict files) {

    size_t fmtcap = 1024;
    char*  fmtbuf = RKT_malloc(char, fmtcap);
    if (!fmtbuf) { fputs("Error: Realloc fmt failed\n", stderr), exit(1); }
    for (size_t i = 0; i < r.count; ++i) {
        size_t bytes = RKT_print_assertres(e, r.results[i], &fmtbuf, &fmtcap);
        fwrite(fmtbuf, 1, bytes, files[2]);
    }
    free(fmtbuf);
    if (r.capt[RKT_PIPE_STDOUT].str) {
        RKT_Verbosity v = RKT_resolve_verbosity(e, RKT_PIPE_STDOUT);
        if ((v != RK_TEST_LOG_NEVER)
            && ((v == RK_TEST_LOG_ALWAYS) || r.ended != RKT_PASSED)) {
            fwrite(r.capt[RKT_PIPE_STDOUT].str, 1, r.capt[RKT_PIPE_STDOUT].len,
                   files[RKT_PIPE_STDOUT]);
        }
    }
    if (r.capt[RKT_PIPE_STDERR].str) {
        RKT_Verbosity v = RKT_resolve_verbosity(e, RKT_PIPE_STDERR);
        if ((v != RK_TEST_LOG_NEVER)
            && ((v == RK_TEST_LOG_ALWAYS) || r.ended != RKT_PASSED)) {
            fwrite(r.capt[RKT_PIPE_STDERR].str, 1, r.capt[RKT_PIPE_STDERR].len,
                   files[RKT_PIPE_STDERR]);
        }
    }
    switch (r.ended) {
    case RKT_PASSED:
        fprintf(files[RKT_PIPE_META],
                "-> " RK_INGREEN("PASS") ": All %zu assertions succeeded\n\n",
                r.count); // NOLINT
        break;
    case RKT_FAILED:
        fprintf(
            files[RKT_PIPE_META],
            "-> " RK_INRED("FAIL") ": %zu fails, %zu passed (total %zu)\n\n",
            r.fails, r.count - r.fails, r.count); // NOLINT
        break;
    case RKT_CRASHED:
        fprintf(
            files[RKT_PIPE_META],
            "-> " RK_INRED("FAIL") ": %zu fails, %zu passed (total %zu), "
                                   "crashed unexpectedly with signal %d\n\n",
            r.fails, r.count - r.fails, r.count,
            RKT_interpret_code(r.term_code)); // NOLINT
        break;
    case RKT_TIMEDOUT:
        fprintf(
            files[RKT_PIPE_META],
            "-> " RK_INRED("TIMED OUT") ": %zu fails, %zu passed (total %zu), "
                                        "exceeded time of %ldms\n\n",
            r.fails, r.count - r.fails, r.count,
            RKT_resolve_timeout(e)); // NOLINT
        break;
    case RKT_TESTERROR: // todo windows
#ifndef _WIN32
        if (WIFSIGNALED(r.term_code)) {
            printf(
                RK_INRED("Error in Test Function; Crashed with signal %d\n\n"),
                WTERMSIG(r.term_code));
        } else {
            printf(RK_INRED("Error in Test Function; Exited with code %d\n\n"),
                   WEXITSTATUS(r.term_code));
        }
#else
        ;
#endif
    }
    return r.ended;
}
// case RKT_ASSERTRES_EXIT:
//     fprintf(files[RKT_PIPE_META],
//             "-> " RK_INRED("FAIL") ": %zu fails, %zu passed (total %zu),
//             "
//                                    "exited unexpectedly with code
//                                    %d\n\n",
//             t.fails, t.count - t.fails, t.count, t.term_code); // NOLINT
//     break;

/*
    if (t.ended == RKT_TESTERROR) { // todo windows
        if (WIFSIGNALED(t.term_code)) {
            printf(
                RK_INRED("Error in Test Function; Crashed with signal
   %d\n\n"), WTERMSIG(t.term_code)); } else { printf(RK_INRED("Error in Test
   Function; Exited with code %d\n\n"), WEXITSTATUS(t.term_code));
        }
        return RKT_TESTERROR;
    }
*/

/// PLATFORM DIFFERENCES
#ifndef _WIN32
# define RKT_CONSTRUCTOR(fn) __attribute__((constructor)) static void fn(void)
# define RK_TEST_SPECIALCHILDENTRY()

RKT_noreturn static inline void RKT_fatal(rk_pid pid, const char* str) {
    perror(str);
    if (pid > 0) {
        kill(pid, SIGKILL);
        while (waitpid(pid, NULL, 0) == -1) {
            if (errno != EINTR) {
                perror("waitpid");
                break;
            }
        }
    }
    exit(1);
}

static inline int RKT_reapchild(rk_pid pid) {
    int status;
    while (waitpid(pid, &status, 0) == -1) {
        if (errno != EINTR) { RKT_fatal(pid, "waitpid"); }
    };
    return status;
}

static inline rk_pid RKT_init_test(const RKT_TestEntry* restrict e,
                                   rk_fd* restrict pipes,
                                   FILE** restrict files) {
    int    pps[6];
    rk_pid pid;
    for (int i = 0; i < 3; ++i) {
        if (pipe(pps + i * 2) < 0) { RKT_fatal(-1, "pipe"); };
    }
    if ((pid = fork()) == -1) { RKT_fatal(-1, "fork"); }
    for (int i = 0, end = (pid > 0) ^ 1; i < 3; ++i) {
        close(pps[i * 2 + !end]), pipes[i] = pps[i * 2 + end];
    }
    if (pid == 0) {
        if (dup2(pipes[0], STDOUT_FILENO) < 0
            || dup2(pipes[1], STDERR_FILENO) < 0) {
            perror("dup2"), exit(1);
        }
        if (e->attrs.init) { e->attrs.init(); }
        e->func(pipes[2]);
        if (e->attrs.fini) { e->attrs.fini(); }
        exit(0);
    } else {
        for (int i = 0; i < 3; ++i) { // set pipes nonblocking
            int fd = pipes[i], flags = fcntl(fd, F_GETFL, 0);
            if (flags == -1 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
                RKT_fatal(pid, "fcntl");
            }
        }
        RKT_setup_files(e, pid, files);
    }
    return pid;
}

static inline long RKT_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static inline RKT_TestResult RKT_parent_loop(rk_pid pid, rk_fd* pipes,
                                             long timeout_ms) {
    size_t         rescap  = 512;
    size_t         caps[2] = {512, 512};
    RKT_TestResult r       = {.count   = 0,
                              .fails   = 0,
                              .results = RKT_malloc(RKT_AssertDat, rescap),
                              .capt    = {{.str = RKT_malloc(char, caps[0])},
                                          {.str = RKT_malloc(char, caps[1])}}};
    if (!r.results || !r.capt[0].str || !r.capt[1].str) {
        RKT_fatal(pid, "malloc buf");
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
            int p = poll(fds, 3, poll_timeout);
            if (p == -1) {
                if (errno != EINTR) { RKT_fatal(pid, "poll"); }
            } else if (p == 0) {
                RKT_AssertRes res = {.res = RKT_ASSERTRES_TIMEOUT};
                if (reading_res) {
                    r.results[r.count - 1].res = res;
                } else {
                    RKT_AssertDat dat    = {{RKT_ZINIT}, res};
                    r.results[r.count++] = dat;
                }
                kill(pid, SIGKILL), RKT_reapchild(pid); // todo maybe not
                close(pipes[0]), close(pipes[1]), close(pipes[2]);
                return ++r.fails, r.ended = RKT_TIMEDOUT, r;
            } else {
                break;
            }
        }
        if (fds[2].revents & POLLIN) {
            if (!reading_res) {
                if (r.count + 1 == rescap) { //+1 to simplify timeout code
                    rescap    *= 2;
                    r.results  = RKT_realloc(RKT_AssertDat, r.results, rescap);
                    if (!r.results) { RKT_fatal(pid, "realloc 0"); }
                }
                RKT_AssertHdr hdr = {RKT_ZINIT};
                switch (RKT_read_full_nb(fds[2].fd, &hdr, sizeof(hdr))) {
                case -1: RKT_fatal(pid, "read meta hdr");
                case 0 : fds[2].fd = -1, --nfds; break;
                case 1 : r.results[r.count++].hdr = hdr, reading_res = 1;
                }
            } else {
                RKT_AssertRes res = {RKT_ZINIT};
                switch (RKT_read_full_nb(fds[2].fd, &res,
                                         sizeof(RKT_AssertResPkg))) {
                case -1: RKT_fatal(pid, "read meta res");
                case 0 : fds[2].fd = -1, --nfds; break;
                case 1:
                    if (res.len) {
                        res.args[0] = RKT_malloc(char, res.len);
                        if (!res.args[0]) { RKT_fatal(pid, "malloc"); }
                        if (RKT_read_full_nb(fds[2].fd, res.args[0], res.len)
                            < 1) {
                            RKT_fatal(pid, "read OWO");
                        }
                        // todo probably buggy
                        for (size_t i = 1, len = 0; len <= res.len - 1; ++len) {
                            if (!res.args[0][len]) {
                                res.args[i++] = res.args[0] + len + 1;
                            }
                        }
                    }
                    if (res.res) { ++r.fails; }
                    r.results[r.count - 1].res = res, reading_res = 0;
                }
            }
        }
        for (int i = 0; i < 2; ++i) { // todo maybe drain pipe?
            if (!(fds[i].revents & POLLIN)) { continue; }
            ssize_t rd = read(fds[i].fd, r.capt[i].str + r.capt[i].len,
                              caps[i] - r.capt[i].len);
            if (rd == -1) {
                if (errno == EINTR) { continue; }
                if (errno == EAGAIN || errno == EWOULDBLOCK) { break; }
                RKT_fatal(pid, "read out/err res");
            } else if (rd == 0) {
                fds[i].fd = -1, --nfds;
                break;
            } else if ((r.capt[i].len = r.capt[i].len + rd) == caps[i]) {
                caps[i]       *= 2;
                r.capt[i].str  = RKT_realloc(char, r.capt[i].str, caps[i]);
                if (!r.capt[i].str) { RKT_fatal(pid, "realloc"); }
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
                if (!(lastres->args[0] = (char*)calloc(1, 32))) {
                    RKT_fatal(-1, "calloc");
                }
                lastres->len = sprintf(lastres->args[0], "%d", sig) + 1;
                if (sig != lasthdr->code) { lastres->res = 2; }
            } else {
                lastres->res = RKT_ASSERTRES_CRASH, r.ended = RKT_CRASHED;
            }
        } else {
            int stat = WEXITSTATUS(r.term_code);
            if (lasthdr->F == RKTF_EXIT) {
                if (!(lastres->args[0] = (char*)calloc(1, 32))) {
                    RKT_fatal(-1, "calloc");
                }
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

static inline int RKT_read_full_nb(int fd, void* buf, size_t nbytes) {
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

static inline int RKT_write_full(int fd, const void* buf, size_t len) {
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
     static void fn(void);                                                     \
     __declspec(allocate(".CRT$XCU")) static void (*fn##_ptr)(void) = fn;      \
     static void fn(void)

# define RK_TEST_SPECIALCHILDENTRY()                                           \
     do {                                                                      \
         char buf[256];                                                        \
         if (!GetEnvironmentVariableA("RK_CHILD_FN", buf, sizeof(buf))) {      \
             break;                                                            \
         }                                                                     \
         for (RKT_Suite* s = RKT_glob.suites; s; s = s->next) {                \
             for (RKT_TestEntry* e = s->tests; e; e = e->next) {               \
                 if (!strcmp(buf, e->name.str)) {                              \
                     if (!GetEnvironmentVariableA("RK_CHILD_META", buf,        \
                                                  sizeof(buf))) {              \
                         exit(1);                                              \
                     }                                                         \
                     char* ep;                                                 \
                     rk_fd meta_fd                                             \
                         = (rk_fd)(uintptr_t)strtoumax(buf, &ep, 10);          \
                     if (*ep != '\0') { exit(1); }                             \
                     if (RKT_glob.attrs.init) { RKT_glob.attrs.init(); }       \
                     if (s->attrs.init) { s->attrs.init(); }                   \
                     if (e->attrs.init) { e->attrs.init(); }                   \
                     e->func(meta_fd);                                         \
                     if (e->attrs.fini) { e->attrs.fini(); }                   \
                     if (s->attrs.fini) { s->attrs.fini(); }                   \
                     if (RKT_glob.attrs.fini) { RKT_glob.attrs.fini(); }       \
                     exit(0);                                                  \
                 }                                                             \
             }                                                                 \
         }                                                                     \
         exit(1); /*function not found?*/                                      \
     } while (0);

RKT_noreturn static inline void RKT_fatal(rk_pid pid, const char* msg) {
    DWORD err = GetLastError();
    char* buf = NULL;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
                       | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (LPSTR)&buf, 0, NULL);
    fprintf(stderr, "%s: %s\n", msg, buf ? buf : "Unknown error");
    if (buf) { LocalFree(buf); }
    if (pid.hProcess != NULL && pid.hProcess != INVALID_HANDLE_VALUE) {
        TerminateProcess(pid.hProcess, 1);
        CloseHandle(pid.hProcess), CloseHandle(pid.hThread);
    }
    exit(1);
}

static inline DWORD RKT_reapchild(rk_pid pid) {
    DWORD status;
    WaitForSingleObject(pid.hProcess, INFINITE);
    GetExitCodeProcess(pid.hProcess, &status);
    CloseHandle(pid.hProcess), CloseHandle(pid.hThread);
    return status;
}

static inline rk_pid RKT_init_test(const RKT_TestEntry* restrict e,
                                   rk_fd* restrict pipes,
                                   FILE** restrict files) {
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

    s += sprintf(s, "RK_CHILD_META=%" PRIuPTR, (uintptr_t)w_ends[RKT_PIPE_META])
       + 1;
    if (plen) { s += (memcpy(s, penv, plen), plen); }
    *s = '\0'; /* ensure double-null terminator */

    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si)); // Clear everything
    si.cb         = sizeof(si);
    si.dwFlags    = STARTF_USESTDHANDLES;
    si.hStdOutput = w_ends[RKT_PIPE_STDOUT];
    si.hStdError  = w_ends[RKT_PIPE_STDERR];
    rk_pid pid;
    BOOL   worked = CreateProcessA(path, path, NULL, NULL, TRUE, 0, nenv, NULL,
                                   &si, &pid);
    if (penv) { FreeEnvironmentStringsA(penv); }
    free(nenv);
    CloseHandle(w_ends[0]), CloseHandle(w_ends[1]), CloseHandle(w_ends[2]);
    if (!worked) { exit(1); }
    RKT_setup_files(e, pid, files);
    return pid;
}

static inline long RKT_now_ms(void) {
    static LARGE_INTEGER freq = {0};
    if (freq.QuadPart == 0) { QueryPerformanceFrequency(&freq); }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (long)((counter.QuadPart * 1000) / freq.QuadPart);
}

static inline RKT_TestResult RKT_parent_loop(rk_pid pid, rk_fd* pipes,
                                             long timeout_ms) {
    size_t         rescap  = 512;
    size_t         caps[2] = {512, 512};
    RKT_TestResult r       = {0};
    r.results              = RKT_malloc(RKT_AssertDat, rescap);
    r.capt[0].str          = RKT_malloc(char, caps[0]);
    r.capt[1].str          = RKT_malloc(char, caps[1]);
    r.ended                = RKT_PASSED;
    if (!r.results || !r.capt[0].str || !r.capt[1].str) {
        RKT_fatal(pid, "malloc buf");
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
            RKT_fatal(pid, "WaitForMultipleObjects");
        } else if (wait == WAIT_TIMEOUT) {
            RKT_AssertRes res = {0};
            res.res           = RKT_ASSERTRES_TIMEOUT;
            if (reading_res) {
                r.results[r.count ? r.count - 1 : 0].res = (res);
            } else {
                RKT_AssertHdr hdr    = {RKT_ZINIT};
                RKT_AssertDat dat    = {hdr, res};
                r.results[r.count++] = dat;
            }
            ++r.fails;
            return r;
        }
        int idx = wait - WAIT_OBJECT_0;
        if (handles[idx] == pipes[RKT_PIPE_META]) {
            if (!reading_res) {
                if (r.count + 1 == rescap) {
                    rescap    *= 2;
                    r.results  = RKT_realloc(RKT_AssertDat, r.results, rescap);
                    if (!r.results) { RKT_fatal(pid, "realloc 0"); }
                }
                RKT_AssertHdr hdr = {RKT_ZINIT};
                switch (RKT_read_full_nb(handles[idx], &hdr, sizeof(hdr))) {
                case -1: RKT_fatal(pid, "read meta hdr");
                case 0 : handles[idx] = INVALID_HANDLE_VALUE; break;
                case 1 : r.results[r.count++].hdr = hdr, reading_res = 1;
                }
            } else {
                RKT_AssertRes res = {RKT_ZINIT};
                switch (RKT_read_full_nb(handles[idx], &res,
                                         sizeof(RKT_AssertResPkg))) {
                case -1: RKT_fatal(pid, "read meta res");
                case 0 : handles[idx] = INVALID_HANDLE_VALUE; break;
                case 1:
                    if (res.len) {
                        res.args[0] = RKT_malloc(char, res.len);
                        if (!res.args[0]) { RKT_fatal(pid, "malloc"); }
                        if (RKT_read_full_nb(handles[idx], res.args[0], res.len)
                            < 1) {
                            RKT_fatal(pid, "read OWO");
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
            if (handles[idx] == pipes[RKT_PIPE_STDOUT]) {
                i = (size_t)RKT_PIPE_STDOUT;
            } else if (handles[idx] == pipes[RKT_PIPE_STDERR]) {
                i = (size_t)RKT_PIPE_STDERR;
            } else {
                assert(0); // sanity check
            }
            DWORD rd;
            if (!ReadFile(handles[idx], r.capt[i].str + r.capt[i].len,
                          (DWORD)(caps[i] - r.capt[i].len), &rd, NULL)) {
                if (GetLastError() == ERROR_BROKEN_PIPE) {
                    handles[idx] = INVALID_HANDLE_VALUE;
                } else {
                    RKT_fatal(pid, "ReadFile stdout/stderr");
                }
            } else if (rd == 0) {
                handles[idx] = INVALID_HANDLE_VALUE;
            } else {
                r.capt[i].len += rd;
                if (r.capt[i].len == caps[i]) {
                    caps[i]       *= 2;
                    r.capt[i].str  = (char*)realloc(r.capt[i].str, caps[i]);
                    if (!r.capt[i].str) {
                        RKT_fatal(pid, "realloc stdout/stderr");
                    }
                }
            }
        }
        for (int i = nfds = 0; i < 3; ++i) {
            if (handles[i] != INVALID_HANDLE_VALUE) {
                handles[nfds++] = handles[i];
            }
        }
    }
    CloseHandle(pipes[0]), CloseHandle(pipes[1]), CloseHandle(pipes[2]);
    DWORD exit_code = RKT_reapchild(pid);
    if (reading_res) {
        RKT_AssertRes res = {0};
        if (r.results[r.count - 1].hdr.F == RKTF_CRASH) {
            res.args[0] = (char*)calloc(1, 32);
            if (!res.args[0]) { RKT_fatal(pid, "calloc"); }
            res.len = sprintf(res.args[0], "%lu", exit_code) + 1;
            if ((int)exit_code != r.results[r.count - 1].hdr.code) {
                res.res = 2;
            }
        } else if (r.results[r.count - 1].hdr.F == RKTF_EXIT) {
            res.args[0] = (char*)calloc(1, 32);
            if (!res.args[0]) { RKT_fatal(pid, "calloc"); }
            res.len = sprintf(res.args[0], "%lu", exit_code) + 1;
            if ((int)exit_code != r.results[r.count - 1].hdr.code) {
                res.res = 2;
            }
        } else {
            res.res = RKT_ASSERTRES_EXIT, r.term_code = (int)exit_code;
        }
        if (res.res) { ++r.fails; }
        r.results[r.count - 1].res = res;
    }
    return r;
}

static inline int RKT_read_full_nb(rk_fd h, void* buf, size_t nbytes) {
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
static inline int RKT_write_full(rk_fd h, const void* buf, size_t len) {
    const char* ptr = (const char*)buf;
    for (DWORD w; len > 0; ptr += w, len -= w) {
        if (!WriteFile(h, ptr, (DWORD)len, &w, NULL)) { return -1; }
    }
    return 1;
}

#endif

#undef RK_catlit
#undef RK_test_GENFUNS_nofloat
#undef RK_test_GENFUNS_float
#undef RK_INRED
#undef RK_INGREEN
#undef RK_INBLUE
#undef RK_INYELLOW

#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif
#endif
