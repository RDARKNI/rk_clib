
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#pragma GCC diagnostic ignored "-Wc2x-extensions"

#ifdef _WIN32
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
/// @brief RK_REGISTER_TEST(fn, .tags=NULL, .suite=NULL) {fn-body}
#define RK_REGISTER_TEST(fn, ...)                                              \
    static void fn(rk_fd RK_OUT);                                              \
    RKT_CONSTRUCTOR(RK_test_register_##fn) {                                   \
        static RKT_TestEntry entry = {.func  = fn,                             \
                                      .file  = __FILE__,                       \
                                      .name  = #fn,                            \
                                      .attrs = {0, __VA_ARGS__}};              \
        if (!RKT_glob.tests.head) {                                            \
            RKT_glob.tests.head = RKT_glob.tests.tail = &entry;                \
        } else {                                                               \
            RKT_glob.tests.tail = RKT_glob.tests.tail->next = &entry;          \
        }                                                                      \
    }                                                                          \
    static void fn(rk_fd RK_OUT)

#define RK_RUN_TESTS(...)                                                      \
    do {                                                                       \
        RK_TEST_SPECIALCHILDENTRY()                                            \
        RKT_run_all_tests((RKT_CustomAttrs){__VA_ARGS__});                     \
                                                                               \
    } while (0)

// clang-format off
#define rk_expect_true(expr)                  RK_EXPECT(RKTf_true, RKTF_TRUE, "rk_expect_true("#expr")", expr)
#define rk_expect_false(expr)                 RK_EXPECT(RKTf_false, RKTF_FALSE, "rk_expect_false("#expr")", expr)
#define rk_expect_null(ptr)                   RK_EXPECT(RKTf_eqnull, RKTF_EQNULL, "rk_expect_null("#ptr")", ptr)
#define rk_expect_nonnull(ptr)                RK_EXPECT(RKTf_neqnull, RKTF_NEQNULL, "rk_expect_nonnull("#ptr")", ptr)
#define rk_expect_memeq(ptr1, ptr2, siz)      RK_EXPECT(RKTf_memeq, RKTF_MEMEQ, "rk_expect_memeq(" #ptr1 ", " #ptr2 ", " #siz ")", ptr1, ptr2, siz)
#define rk_expect_memneq(ptr1, ptr2, siz)     RK_EXPECT(RKTf_memneq, RKTF_MEMNEQ, "rk_expect_memneq(" #ptr1 ", " #ptr2 ", " #siz ")", ptr1, ptr2, siz)
#define rk_expect_memzero(ptr, siz)           RK_EXPECT(RKTf_memzero, RKTF_MEMZERO, "rk_expect_memzero(" #ptr ",  " #siz ")", ptr, siz)
#define rk_expect_memnzero(ptr, siz)          RK_EXPECT(RKTf_memnzero, RKTF_MEMNZERO, "rk_expect_memnzero(" #ptr ",  " #siz ")", ptr, siz)

#define rk_expect_eq(exp, act)                RK_EXPECT(RK_test_SEL_TF_FUN(eq, exp), RKTF_EQ, "rk_expect_eq("#exp", " #act")", exp, act)
#define rk_expect_neq(exp, act)               RK_EXPECT(RK_test_SEL_TF_FUN(neq, exp), RKTF_NEQ, "rk_expect_neq("#exp", " #act")", exp, act)
#define rk_expect_lt(exp, act)                RK_EXPECT(RK_test_SEL_TF_FUN(eq, exp), RKTF_LT, "rk_expect_lt("#exp", " #act")", exp, act)
#define rk_expect_leq(exp, act)               RK_EXPECT(RK_test_SEL_TF_FUN(neq, exp), RKTF_LEQ, "rk_expect_leq("#exp", " #act")", exp, act)
#define rk_expect_gt(exp, act)                RK_EXPECT(RK_test_SEL_TF_FUN(eq, exp), RKTF_GT, "rk_expect_gt("#exp", " #act")", exp, act)
#define rk_expect_geq(exp, act)               RK_EXPECT(RK_test_SEL_TF_FUN(neq, exp), RKTF_GEQ, "rk_expect_geq("#exp", " #act")", exp, act)
#define rk_expect_inrange(val, low, high)     RK_EXPECT(RK_test_SEL_TF_FUN(inrange, val), RKTF_INRANGE, "rk_expect_inrange("#low", " #val ", " #high ")", val, low, high)
#define rk_expect_floateq_tol(exp, act, tol)  RK_EXPECT(RK_test_SEL_TF_FUN_TOL(eq, tol), RKTF_EQTOL, "rk_expect_floateq_tol("#exp", " #act ", " #tol ")", exp, act, tol)
#define rk_expect_floatneq_tol(exp, act, tol) RK_EXPECT(RK_test_SEL_TF_FUN_TOL(neq, tol), RKTF_NEQTOL,  "rk_expect_floatneq_tol("#exp", " #act ", " #tol ")", exp, act, tol)
#define rk_expect_streq(str1, str2)           RK_EXPECT(RKTf_streq, RKTF_STREQ, "streq(" #str1 ", " #str2 ")",str1,str2)
#define rk_expect_strneq(str1, str2)          RK_EXPECT(RKTf_strneq, RKTF_STRNEQ,"strneq(" #str1 ", " #str2 ")",str1,str2)                                    \

// todo not implemented 
#define rk_expect_stdouteq(errstr, ...)       RK_test_sendhdr("stderror prints " #errstr, RK_TEST_STDOUTEQ, RK_TEST_T_vp)
#define rk_expect_stdoutneq(errstr, ...)      RK_test_sendhdr("stderror prints " #errstr, RK_TEST_STDOUTNEQ, RK_TEST_T_vp)
#define rk_expect_stderreq(errstr, ...)       RK_test_sendhdr("stderror prints " #errstr, RK_TEST_STDERREQ, RK_TEST_T_vp)
#define rk_expect_stderrneq(errstr, ...)      RK_test_sendhdr("stderror prints " #errstr, RK_TEST_STDERRNEQ, RK_TEST_T_vp)

#define rk_assert_true(expr)                  RK_ASSERT(RKTf_true, RKTF_TRUE, "rk_assert_true("#expr")", expr)
#define rk_assert_false(expr)                 RK_ASSERT(RKTf_false, RKTF_FALSE, "rk_assert_false("#expr")", expr)
#define rk_assert_null(ptr)                   RK_ASSERT(RKTf_eqnull, RKTF_EQNULL, "rk_assert_null("#ptr")", ptr)
#define rk_assert_nonnull(ptr)                RK_ASSERT(RKTf_neqnull, RKTF_NEQNULL, "rk_assert_nonnull("#ptr")", ptr)
#define rk_assert_memeq(ptr1, ptr2, siz)      RK_ASSERT(RKTf_memeq, RKTF_MEMEQ, "rk_assert_memeq(" #ptr1 ", " #ptr2 ", " #siz ")", ptr1, ptr2, siz)
#define rk_assert_memneq(ptr1, ptr2, siz)     RK_ASSERT(RKTf_memneq, RKTF_MEMNEQ, "rk_assert_memneq(" #ptr1 ", " #ptr2 ", " #siz ")", ptr1, ptr2, siz)
#define rk_assert_memzero(ptr, siz)           RK_ASSERT(RKTf_memzero, RKTF_MEMZERO, "rk_assert_memzero(" #ptr ",  " #siz ")", ptr, siz)
#define rk_assert_memnzero(ptr, siz)          RK_ASSERT(RKTf_memnzero, RKTF_MEMNZERO, "rk_assert_memnzero(" #ptr ",  " #siz ")", ptr, siz)

#define rk_assert_eq(exp, act)                RK_ASSERT(RK_test_SEL_TF_FUN(eq, exp), RKTF_EQ, "rk_assert_eq("#exp", " #act")", exp, act)
#define rk_assert_neq(exp, act)               RK_ASSERT(RK_test_SEL_TF_FUN(neq, exp), RKTF_NEQ, "rk_assert_neq("#exp", " #act")", exp, act)
#define rk_assert_lt(exp, act)                RK_ASSERT(RK_test_SEL_TF_FUN(eq, exp), RKTF_LT, "rk_assert_lt("#exp", " #act")", exp, act)
#define rk_assert_leq(exp, act)               RK_ASSERT(RK_test_SEL_TF_FUN(neq, exp), RKTF_LEQ, "rk_assert_leq("#exp", " #act")", exp, act)
#define rk_assert_gt(exp, act)                RK_ASSERT(RK_test_SEL_TF_FUN(eq, exp), RKTF_GT, "rk_assert_gt("#exp", " #act")", exp, act)
#define rk_assert_geq(exp, act)               RK_ASSERT(RK_test_SEL_TF_FUN(neq, exp), RKTF_GEQ, "rk_assert_geq("#exp", " #act")", exp, act)
#define rk_assert_inrange(val, low, high)     RK_ASSERT(RK_test_SEL_TF_FUN(inrange, val), RKTF_INRANGE, "rk_assert_inrange("#low", " #val ", " #high ")", val, low, high)
#define rk_assert_floateq_tol(exp, act, tol)  RK_ASSERT(RK_test_SEL_TF_FUN_TOL(eq, tol), RKTF_EQTOL, "rk_assert_floateq_tol("#exp", " #act ", " #tol ")", exp, act, tol)
#define rk_assert_floatneq_tol(exp, act, tol) RK_ASSERT(RK_test_SEL_TF_FUN_TOL(neq, tol), RKTF_NEQTOL,  "rk_assert_floatneq_tol("#exp", " #act ", " #tol ")", exp, act, tol)
#define rk_assert_streq(str1, str2)           RK_ASSERT(RKTf_streq, RKTF_STREQ, "streq(" #str1 ", " #str2 ")",str1,str2)
#define rk_assert_strneq(str1, str2)          RK_ASSERT(RKTf_strneq, RKTF_STRNEQ,"strneq(" #str1 ", " #str2 ")",str1,str2)  

// todo not implemented
#define rk_assert_stdouteq(errstr, ...)  RK_test_sendhdr("stderror prints " #errstr, RK_TEST_STDOUTEQ, RK_TEST_T_vp)
#define rk_assert_stdoutneq(errstr, ...) RK_test_sendhdr("stderror prints " #errstr, RK_TEST_STDOUTNEQ, RK_TEST_T_vp)
#define rk_assert_stderreq(errstr, ...)  RK_test_sendhdr("stderror prints " #errstr, RK_TEST_STDERREQ, RK_TEST_T_vp)
#define rk_assert_stderrneq(errstr, ...) RK_test_sendhdr("stderror prints " #errstr, RK_TEST_STDERRNEQ, RK_TEST_T_vp)

#define rk_assert_crash(signal, ...)                                           \
    do {                                                                       \
        RK_test_sendhdr_sig(1, "assert_crash(" #signal ", " #__VA_ARGS__ ")",  \
                            RKTF_CRASH, signal);                            \
        __VA_ARGS__;                                                           \
        RKT_write_full(RK_OUT, &(RKT_AssertResPkG){.res = 1}, sizeof(RKT_AssertResPkG));    \
        exit(0);                                                               \
    } while (0)

#define rk_assert_exit(code, ...)                                              \
    do {                                                                       \
        RK_test_sendhdr_sig(1, "rk_assert_exit(" #code ", " #__VA_ARGS__ ")",  \
                            RKTF_EXIT, code);                               \
        __VA_ARGS__;                                                           \
        RKT_write_full(RK_OUT, &(RKT_AssertResPkG){.res = 1}, sizeof(RKT_AssertResPkG)); \
        exit(0);                                                               \
    } while (0)

#if defined(__cplusplus) && __cplusplus >= 201103L
# define NORETURN [[noreturn]]
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
# include <stdnoreturn.h>
# define NORETURN noreturn
#elif defined(_MSC_VER)
# define NORETURN __declspec(noreturn)
#elif defined(__GNUC__)
# define NORETURN __attribute__((noreturn))
#else
# define NORETURN
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

#ifdef _WIN32
typedef HANDLE              rk_fd;
typedef PROCESS_INFORMATION rk_pid;
#else
typedef int   rk_fd;
typedef pid_t rk_pid;
#endif

#define lenof(strlit)   (sizeof("" strlit "") - 1)
#define RK_catlit(s, l) (memcpy(s, l, lenof(l)), lenof(l))

#define RK_MIN(x, y)    ((x) <= (y) ? (x) : (y))
#define RK_MAX(x, y)    ((x) >= (y) ? (x) : (y))


#define RKT_malloc(TYPE, COUNT) ((TYPE*)malloc(sizeof(TYPE) * COUNT))
#define RKT_realloc(TYPE, ptr, COUNT)                                          \
    ((TYPE*)realloc(ptr, sizeof(TYPE) * COUNT))


#define RK_RESET         "\x1b[0m"
#define RK_RED           "\x1b[31m"
#define RK_GREEN         "\x1b[32m"
#define RK_BLUE          "\x1b[32m"
#define RK_YELLOW        "\x1b[33m"
#define RK_INRED(str)    RK_RED str RK_RESET
#define RK_INGREEN(str)  RK_GREEN str RK_RESET
#define RK_INBLUE(str)   RK_BLUE str RK_RESET
#define RK_INYELLOW(str) RK_YELLOW str RK_RESET

#define RK_TFMT_TRUE         "Expected " RK_INGREEN("'true'") ", got: " RK_INRED("'false'") "."     
#define RK_TFMT_FALSE        "Expected " RK_INGREEN("'false'") ", got: " RK_INRED("'true'") "."        
#define RK_TFMT_NULL(RES)    "Expected " RK_INGREEN("'NULL'") ", got: " RK_INRED("'%s'") ".",          \
                               (RES).args[0]
#define RK_TFMT_NONNULL(RES) "Expected " RK_INGREEN("nonnull pointer") ", got: " RK_INRED("'%s'") ".", \
                               (RES).args[1]
#define RK_TFMT_MEMEQ(RES)   "Memory Regions %s and %s (%s bytes long) " RK_INRED("not equal") ".",   \
                               (RES).args[0], (RES).args[1], (RES).args[2]
#define RK_TFMT_MEMNEQ(RES)  "Memory Regions %s and %s (%s bytes long) " RK_INRED("equal") ".",       \
                               (RES).args[0], (RES).args[1], (RES).args[2]
#define RK_TFMT_MEMZERO(RES) "%s (%s bytes long) nonzero at byte " RK_INRED("%zu") ".",               \
                               (RES).args[0], (RES).args[1], (RES).res - 1
#define RK_TFMT_MEMNZER(RES) "%s (%s bytes long) zero at byte " RK_INRED("%zu") ".",                  \
                               (RES).args[0], (RES).args[1], (RES).res - 1
#define RK_TFMT_STREQ(RES)   "strings %s, %s " RK_INRED("not equal") " at position %zu.",    \
                               (RES).args[0], (RES).args[1], (RES).res - 1
#define RK_TFMT_STRNEQ(RES)  "strings " RK_INRED("equal") "."
#define RK_TFMT_DEATH(HDR)   " - Expected: termination via signal %d, got: no termination",            \
                               (int)((HDR).code)
#define RK_TFMT_EXIT(HDR)    " - Expected: exit with code %d, got: no exit",                           \
                               (int)((HDR).code)
#define RK_TFMT_INRANGE(RES) "%s %s %s", (RES).args[0], (RES).res == 1 ? "<" : ">",                     \
                              (RES).args[1 + ((RES).res == 2)]
#define RKTFMT_GEQ(RES)      "%s < %s", (RES).args[0], (RES).args[1]
#define RKTFMT_GT(RES)       "%s %s %s", (RES).args[0], (RES).res == 1 ? "==" : "<", (RES).args[1]
  
#define RK_TFMT_P RK_INGREEN("[P]")
#define RK_TFMT_F   RK_INRED("[F]")
#define RK_TFMT_C   RK_INRED("[C]")
#define RK_TFMT_E   RK_INRED("[E]")

#define RK_TFMT_STUB(ASRTRES, FILENAME, HDR)     "%s %s:%d %s",(ASRTRES), (FILENAME), (HDR).pos, (HDR).exprstr
#define RK_TFMT_ASRT_FE(exp,act) " - Expected: exit with code %d, got: exit with code %d\n",                \
                                 exp, act
#define RK_TFMT_ASRT_FC(exp,act) " - Expected: termination via signal %d, got: termination via signal %d\n",\
                                 exp, act

#define RK_TFMT_TEST_RUNSTART(t)                                                                            \
    RK_INYELLOW("Running %s\n")"--------------------------------------------------\n",                   \
    t->name
#define RKT_FMTTEST_P(t)                                                                                \
    "-> " RK_INGREEN("PASS") ": All %zu assertions succeeded\n\n",                                           \
    t.count
#define RKT_FMTTEST_F(t)                                                                                \
    "-> " RK_INRED("FAIL") ": %zu fails, %zu passed (total %zu)\n\n",                                          \
    t.fails,  t.count - t.fails, t.count
#define RKT_FMTTEST_UC(t, sig)                                                                         \
    "-> " RK_INRED("FAIL") ": %zu fails, %zu passed (total %zu), crashed unexpectedly with signal %d\n\n",     \
    t.fails, t.count - t.fails, t.count, sig
#define RKT_FMTTEST_UE(t, code)                                                                         \
    "-> " RK_INRED("FAIL") ": %zu fails, %zu passed (total %zu), exited unexpectedly with code %d\n\n",        \
    t.fails, t.count - t.fails, t.count, code

#define RK_TFMT_TOTALRES(total, fails, crashed)                                                             \
    "--- Passed %zu, fails %zu, crashed %zu out of %zu tests ---\n",                                        \
    total - fails - crashed, fails, crashed, total

// clang-format on
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
#define RK_TEST_GEN_TYPE_ENUM_EL(T, N, ...)        RK_TEST_T_##N,
#define RK_TEST_GEN_TYPE_STRUCT_EL(T, N, FMT, ...) T N;

enum RK_test_FUN {
    RKTF_TRUE,
    RKTF_FALSE,
    RKTF_EQNULL,
    RKTF_NEQNULL,
    RKTF_EQ,
    RKTF_EQTOL,
    RKTF_NEQ,
    RKTF_NEQTOL,
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
};

typedef struct RKT_FileOpts {
    enum {
        RK_TEST_LOG_DEFAULT,
        RK_TEST_LOG_ALWAYS,
        RK_TEST_LOG_NEVER
    } verbosity; // todo clearer semantics
    const char* path;
} RKT_FileOpts;

/// @brief customisable attriutes (global or per test)
typedef struct RKT_CustomAttrs {
    const char *suite, *tags;
    union {
        struct {
            RKT_FileOpts meta, out, err;
        };
        RKT_FileOpts _[3];
    };
} RKT_CustomAttrs;

/// @brief Each test function has one
typedef struct RKT_TestEntry {
    void (*const func)(rk_fd RK_OUT);
    const char *const file, *const name;
    RKT_CustomAttrs       attrs;
    struct RKT_TestEntry* next;
} RKT_TestEntry;

/// @brief Global settings/variables for the testing framework
static struct {
    RKT_CustomAttrs attrs;
    FILE*           files[3];      // todo rename to default files?
    char            resbufr[2048]; // buffer for optimisation
    struct {
        RKT_TestEntry *head, *tail;
    } tests;
} RKT_glob;

/// @brief the arguments and result (true/false or other state) is stored here
typedef struct RKT_AssertResPkG {
    size_t len; ///< length of the args[] section
    size_t res; ///< Result of the test (0 is success)
    char   args[];
} RKT_AssertResPkG;

/// @brief Header to be sent to the test runner before assert/expect
typedef struct RKT_AssertHdr {
    const char*      exprstr; /// todo windows portable?
    int              pos;     ///< The line of the function
    enum RK_test_FUN F;       ///< The assert function (assert_eq, assert_true)
    int              code;    ///< The expected error code (if any)
} RKT_AssertHdr;

typedef struct RKT_AssertRes {
    size_t len;
    size_t res;
    char*  args[3];
} RKT_AssertRes;
#define RKT_PREMATURE_EXIT 16
#define RKT_PREMATURE_SIGN 32
// todo clashes with pos of strdif

/// @brief Data packet for each assertion
typedef struct RKT_AssertDat {
    RKT_AssertHdr hdr;
    RKT_AssertRes res;
} RKT_AssertDat;

/// @brief Summarised results of each Test function; all pointers are owned and
/// must be freed via RKT_cleanup()
typedef struct RKT_TestResult {
    size_t         count;   ///< Number of assertions in the test
    size_t         fails;   ///< Failed assertions in the test
    RKT_AssertDat* results; ///< Result of each assertion in the test
    struct {
        char*  dat;
        size_t len;
    } capt[2]; ///< captured stdout(0), stderr(1)
    int term_code;
} RKT_TestResult;

NORETURN static inline void RKT_fatal(rk_pid pid, const char* str);
static inline int  RKT_write_full(rk_fd fd, const void* buf, size_t len);
static inline int  RKT_read_full_nb(rk_fd fd, void* buf, size_t nbytes);
static inline void RKT_setup_files(const RKT_TestEntry* restrict e, rk_pid pid,
                                   FILE** restrict files);
static inline rk_pid         RKT_init_test(const RKT_TestEntry* e, rk_fd* pipes,
                                           FILE** files);
static inline void           RKT_cleanup(const RKT_TestEntry* t, FILE** files,
                                         RKT_TestResult* res);
static inline int            RKT_run_test(const RKT_TestEntry* t);
static inline RKT_TestResult RKT_parent_loop(rk_pid pid, rk_fd* pipes);
static inline size_t RKT_print_assertres(const RKT_TestEntry* restrict e,
                                         RKT_AssertDat cur,
                                         char** restrict fmtbuf,
                                         size_t* restrict fmtcap);

// todo complete

#define RK_test_sendhdr_sig(IS_ASSERT, EXPRSTR, FUNENUM, sig)                  \
    RKT_write_full(                                                            \
        RK_OUT,                                                                \
        &(RKT_AssertHdr){                                                      \
            .exprstr = EXPRSTR, .pos = __LINE__, .F = FUNENUM, .code = sig},   \
        sizeof(RKT_AssertHdr))
#define RK_test_sendhdr(IS_ASSERT, EXPRSTR, FUNENUM)                           \
    RK_test_sendhdr_sig(IS_ASSERT, EXPRSTR, FUNENUM, 0)

#define RK_EXPECT(FUN, FUNENUM, EXPRSTR, ...)                                  \
    (RK_test_sendhdr(0, EXPRSTR, FUNENUM), FUN(RK_OUT, __VA_ARGS__))

#define RK_ASSERT(FUN, FUNENUM, EXPRSTR, ...)                                  \
    (RK_test_sendhdr(1, EXPRSTR, FUNENUM),                                     \
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
# define RK_test_SEL_TF_FUN_(T, N, F, FTYPE) , T : RKTf_##FTYPE##_##N
# define RK_test_SEL_TF_FUN(FTYPE, VAL)                                        \
     _Generic((VAL)RK_TEST_TYPELIST(RK_test_SEL_TF_FUN_, FTYPE),               \
         default: RKTf_##FTYPE##_vp)

# define RK_test_SEL_TF_FUN_TOL_(T, N, F, FTYPE) , T : RKTf_##FTYPE##_tol_##N
# define RK_test_SEL_TF_FUN_TOL(FTYPE, ...)                                    \
     _Generic((__VA_ARGS__)RK_TEST_TYPELIST_FLOAT(RK_test_SEL_TF_FUN_TOL_,     \
                                                  FTYPE))
# define RK_test_GENFUN2_sig(T, N, FMT, name)                                  \
     static inline size_t RKTf_##name##_##N(rk_fd fd, T e1, T e2)
# define RK_test_GENFUN3_sig(T, N, FMT, name)                                  \
     static inline size_t RKTf_##name##_##N(rk_fd fd, T e1, T e2, T e3)

#else
# define RK_test_SEL_TF_FUN(FTYPE, VAL)     RKTf_##FTYPE
# define RK_test_SEL_TF_FUN_TOL(FTYPE, ...) RKTf_##FTYPE##_tol
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
        oss << static_cast<void*>(&val);
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
     static inline size_t RK_test_##OPNAME(rk_fd fd, T e1, U e2) {             \
         RK_test_streambuf buf;                                                \
         std::ostream      oss(&buf);                                          \
         RK_test_to_string(oss, e1), RK_test_to_string(oss, e2);               \
         size_t res = !(e1 OP e2);                                             \
         buf.write_full(fd, (size_t)res);                                      \
         return res;                                                           \
     }

RK_CPP_GENFUNS2(RK_CPP_GENFUN)

template <class T, class U, class O>
static inline size_t RK_test_inrange(rk_fd fd, T e1, U e2, O e3) {
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
    do {                                                                       \
        RKT_AssertResPkG* _res = (RKT_AssertResPkG*)RKT_glob.resbufr;          \
        _res->res = (RES), _res->len = 0;                                      \
        RKT_write_full(fd, _res, sizeof(*_res) + _res->len);                   \
        return (RES);                                                          \
    } while (0)
#define construct_send_buf1(fd, RES, arg1, FMT1)                               \
    do {                                                                       \
        RKT_AssertResPkG* _res = (RKT_AssertResPkG*)RKT_glob.resbufr;          \
        _res->res              = (RES);                                        \
        _res->len              = sprintf(_res->args, FMT1, e1) + 1;            \
        RKT_write_full(fd, _res, sizeof(*_res) + _res->len);                   \
        return (RES);                                                          \
    } while (0)
#define construct_send_buf2(fd, RES, arg1, FMT1, arg2, FMT2)                     \
    do {                                                                         \
        RKT_AssertResPkG* _res  = (RKT_AssertResPkG*)RKT_glob.resbufr;           \
        _res->res               = (RES);                                         \
        _res->len               = sprintf(_res->args, FMT1, e1) + 1;             \
        _res->len              += sprintf(_res->args + _res->len, FMT2, e2) + 1; \
        RKT_write_full(fd, _res, sizeof(*_res) + _res->len);                     \
        return (RES);                                                            \
    } while (0)
#define construct_send_buf3(fd, RES, arg1, FMT1, arg2, FMT2, arg3, FMT3)         \
    do {                                                                         \
        RKT_AssertResPkG* _res  = (RKT_AssertResPkG*)RKT_glob.resbufr;           \
        _res->res               = (RES);                                         \
        _res->len               = sprintf(_res->args, FMT1, e1) + 1;             \
        _res->len              += sprintf(_res->args + _res->len, FMT2, e2) + 1; \
        _res->len              += sprintf(_res->args + _res->len, FMT3, e3) + 1; \
        RKT_write_full(fd, _res, sizeof(*_res) + _res->len);                     \
        return (RES);                                                            \
    } while (0)

#define RK_test_GENFUN2(T, N, FMT, name, expr)                                 \
    RK_test_GENFUN2_sig(T, N, FMT, name) {                                     \
        construct_send_buf2(fd, (expr), e1, FMT, e2, FMT);                     \
    }
#define RK_test_GENFUN3(T, N, FMT, name, expr)                                 \
    RK_test_GENFUN3_sig(T, N, FMT, name) {                                     \
        construct_send_buf3(fd, (expr), e1, FMT, e2, FMT, e3, FMT);            \
    }

// clang-format off
#define RK_test_GENFUNS_alltypes(T, N, FMT, ...)                                     \
    RK_test_GENFUN2(T, N,FMT, lt, (e1 < e2 ? 0 : (e1 == e2 ? 1 : 2)))               \
    RK_test_GENFUN2(T, N,FMT, leq, !(e1 <= e2))                                     \
    RK_test_GENFUN2(T, N,FMT, gt, (e1 > e2 ? 0 : (e1 == e2 ? 1 : 2)))               \
    RK_test_GENFUN2(T, N,FMT, geq, !(e1 >= e2))                                     \
    RK_test_GENFUN3(T, N,FMT, inrange, (e1 < e2 ? 1 : (e1 > e3 ? 2 : 0)))
#define RK_test_GENFUNS_nofloat(T, N, FMT, ...)                                      \
    RK_test_GENFUN2(T, N,FMT, eq, !(e1 == e2))                                      \
    RK_test_GENFUN2(T, N,FMT, neq, !(e1 != e2))                       
#define RK_test_GENFUNS_float(T, N, FMT, eps_default, fabs_fn)                  \
    RK_test_GENFUN2(T, N,FMT, eq, !(fabs_fn(e1 - e2) <= eps_default))               \
    RK_test_GENFUN2(T, N,FMT, neq, !(fabs_fn(e1 - e2) > eps_default))               \
    RK_test_GENFUN3(T, N,FMT, eq_tol, !(fabs_fn(e1 - e2) <= e3))                    \
    RK_test_GENFUN3(T, N,FMT, neq_tol, !(fabs_fn(e1 - e2) > e3))

RK_TEST_TYPELIST_NOFLOAT(RK_test_GENFUNS_nofloat)                
RK_TEST_TYPELIST(RK_test_GENFUNS_alltypes)                       
RK_test_GENFUNS_float(float, f, "%g", 1e-6f, fabsf)              
RK_test_GENFUNS_float(double, d, "%g", 1e-12, fabs)          
RK_test_GENFUNS_float(long double, ld, "%Lg", 1e-12L, fabsl)

// clang-format off
static inline size_t RKTf_true(rk_fd fd, bool e1) {
    construct_send_buf0(fd, !e1);
}
static inline size_t RKTf_false(rk_fd fd, bool e1) {
    construct_send_buf0(fd, !!e1);
}
static inline size_t RKTf_eqnull(rk_fd fd, const void* e1) {
    construct_send_buf1(fd, !(e1 == NULL), e1, "%p");
}
static inline size_t RKTf_neqnull(rk_fd fd, const void* e1) {
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
    RKT_AssertResPkG* res;
    size_t            l1 = strlen(e1), l2 = strlen(e2);
    size_t            dif = RK_test_strdiff(e1, l1, e2, l2);
    if (l1 + l2 + 2 <= sizeof(RKT_glob.resbufr)) {
        res = (RKT_AssertResPkG*)RKT_glob.resbufr;
    } else {
        res = (RKT_AssertResPkG*)malloc(sizeof(RKT_AssertResPkG) + l1 + l2 + 2);
        if (!res) { exit(1); }
    }
    res->res = dif, res->len = l1 + l2 + 2;
    memcpy(res->args, e1, l1 + 1), memcpy(res->args + l1 + 1, e2, l2 + 1);
    RKT_write_full(fd, res, sizeof(*res) + res->len);
    if (l1 + l2 + 2 > sizeof(RKT_glob.resbufr)) { free(res); }
    return dif;
}
static inline size_t RKTf_strneq(rk_fd fd, const char* e1, const char* e2) {
    RKT_AssertResPkG* res;
    size_t            l1 = strlen(e1), l2 = strlen(e2);
    size_t            dif = RK_test_strdiff(e1, l1, e2, l2);
    if (l1 + l2 + 2 <= sizeof(RKT_glob.resbufr)) {
        res = (RKT_AssertResPkG*)RKT_glob.resbufr;
    } else {
        res = (RKT_AssertResPkG*)malloc(sizeof(RKT_AssertResPkG) + l1 + l2 + 2);
        if (!res) { exit(1); }
    }
    res->res = !(dif != 0), res->len = l1 + l2 + 2;
    memcpy(res->args, e1, l1 + 1), memcpy(res->args + l1 + 1, e2, l2 + 1);
    RKT_write_full(fd, res, sizeof(*res) + res->len);
    if (l1 + l2 + 2 > sizeof(RKT_glob.resbufr)) { free(res); }
    return !(dif != 0);
}

static inline void RKT_run_all_tests(RKT_CustomAttrs attrs) {
    FILE* files_default[] = {stdout, stdout, stderr};
    for (int i = 0; i < 3; ++i) {
        if (!attrs._[i].path) {
            RKT_glob.files[i] = files_default[i];
        } else if (!(RKT_glob.files[i] = fopen(attrs._[i].path, "w"))) {
            perror("fopen meta"), exit(1);
        }
    }
    RKT_glob.attrs = attrs;
    size_t total = 0, failed = 0, crashed = 0;
    for (RKT_TestEntry* t = RKT_glob.tests.head; t; t = t->next) {
        if (attrs.suite
            && (!t->attrs.suite || strcmp(attrs.suite, t->attrs.suite))) {
            continue; // wrong suite
        }
        if (!attrs.tags) { // no tags to look for
        run_test:
            switch (++total, RKT_run_test(t)) {
            case 0 : continue;
            case 1 : ++failed; continue;
            default: ++crashed; continue;
            }
        }
        if (!t->attrs.tags) { continue; }
        for (const char *cur = attrs.tags, *end = cur; *cur != '\0';
             cur = end + 1) {
            for (; *end && *end != ','; ++end) {}
            size_t len = end - cur;
            for (const char* p = t->attrs.tags; *p; ++p) {
                if (!strncmp(p, cur, len) && (p[len] == ',' || !p[len])) {
                    goto run_test;
                }
            }
            if (*end == '\0') { break; }
        }
    }
    printf(RK_TFMT_TOTALRES(total, failed, crashed));
}

static inline void RKT_setup_files(const RKT_TestEntry* restrict e, rk_pid pid,
                                   FILE** restrict files) {
    for (int i = 0; i < 3; ++i) {
        if (!e->attrs._[i].path) {
            files[i] = RKT_glob.files[i];
        } else if (!(files[i] = fopen(e->attrs._[i].path, "w"))) {
            RKT_fatal(pid, "fopen");
        }
    }
}

static inline size_t RKT_print_assertres(const RKT_TestEntry* restrict e,
                                         RKT_AssertDat cur,
                                         char** restrict fmtbuf,
                                         size_t* restrict fmtcap) {
    RKT_AssertRes res = cur.res;
    RKT_AssertHdr hdr = cur.hdr;
    char*         s   = *fmtbuf;
    if (!res.res) {
        if (RKT_glob.attrs.meta.verbosity == RK_TEST_LOG_ALWAYS) {
            s    += sprintf(s, RK_TFMT_STUB(RK_TFMT_P, e->file, hdr));
            *s++  = '\n';
        }
        return s - *fmtbuf;
    } else if (res.res >= RKT_PREMATURE_EXIT) {
        if (res.res == RKT_PREMATURE_EXIT) {
            s += sprintf(s, RK_TFMT_STUB(RK_TFMT_E, e->file, hdr));
        } else {
            s += sprintf(s, RK_TFMT_STUB(RK_TFMT_C, e->file, hdr));
        }
        *s++ = '\n';
        return s - *fmtbuf;
    }
    size_t argslen = res.len;
    if (argslen + 256 > *fmtcap) {
        *fmtcap = argslen + 256;
        *fmtbuf = RKT_realloc(char, *fmtbuf, *fmtcap);
        if (!*fmtbuf) { RKT_fatal(-1, "malloc"); }
        s = *fmtbuf;
    }
    s    += sprintf(s, RK_TFMT_STUB(RK_TFMT_F, e->file, hdr));
    *s++  = ' ';
    switch (hdr.F) {
    case RKTF_TRUE   : s += RK_catlit(s, RK_TFMT_TRUE); break;
    case RKTF_FALSE  : s += RK_catlit(s, RK_TFMT_FALSE); break;
    case RKTF_EQNULL : s += sprintf(s, RK_TFMT_NULL(res)); break;
    case RKTF_NEQNULL: s += sprintf(s, RK_TFMT_NONNULL(res)); break;
    case RKTF_EQ:
    case RKTF_EQTOL:
        s += sprintf(s, "%s != %s", res.args[0], res.args[1]);
        if (cur.hdr.F == RKTF_EQTOL) {
            s += sprintf(s, " (tol: %s)", res.args[2]);
        }
        break;
    case RKTF_NEQ:
    case RKTF_NEQTOL:
        s += sprintf(s, "%s == %s", res.args[0], res.args[1]);
        if (cur.hdr.F == RKTF_NEQTOL) {
            s += sprintf(s, " (tol: %s)", res.args[2]);
        }
        break;
    case RKTF_LT:
        s += sprintf(s, "%s %s %s", res.args[0], res.res == 1 ? "==" : ">",
                     res.args[1]);
        break;
    case RKTF_LEQ:
        s += sprintf(s, "%s > %s", res.args[0], res.args[1]);
        break;
    case RKTF_GT      : s += sprintf(s, RKTFMT_GT(res)); break;
    case RKTF_GEQ     : s += sprintf(s, RKTFMT_GEQ(res)); break;
    case RKTF_INRANGE : s += sprintf(s, RK_TFMT_INRANGE(res)); break;
    case RKTF_MEMEQ   : s += sprintf(s, RK_TFMT_MEMEQ(res)); break;
    case RKTF_MEMNEQ  : s += sprintf(s, RK_TFMT_MEMNEQ(res)); break;
    case RKTF_MEMZERO : s += sprintf(s, RK_TFMT_MEMZERO(res)); break;
    case RKTF_MEMNZERO: s += sprintf(s, RK_TFMT_MEMNZER(res)); break;
    case RKTF_STREQ   : s += sprintf(s, RK_TFMT_STREQ(res)); break;
    case RKTF_STRNEQ  : s += sprintf(s, RK_TFMT_STRNEQ(res)); break;
    case RKTF_CRASH:
        if (res.res == 1) {
            s += sprintf(s, RK_TFMT_DEATH(hdr));
        } else {
            s += sprintf(s,
                         " - Expected: termination via signal %d, got: "
                         "termination with signal %s",
                         hdr.code, res.args[0]);
        }
        break; // todo
    case RKTF_EXIT:
        if (res.res == 1) {
            s += sprintf(s, RK_TFMT_EXIT(hdr));
        } else {
            s += sprintf(s,
                         " - Expected: exit via code %d, got: "
                         "exit with code %s",
                         hdr.code, res.args[0]);
        }
        break;
    case RK_TEST_STDOUTEQ:
    case RK_TEST_STDOUTNEQ:
    case RK_TEST_STDERREQ:
    case RK_TEST_STDERRNEQ: exit(1); break; // todo not implemented
    }
    *s++ = '\n';
    return s - *fmtbuf;
}

static inline size_t RKT_print_testres(const RKT_TestEntry* restrict e,
                                       RKT_TestResult t,
                                       FILE** restrict files) {
    printf(RK_TFMT_TEST_RUNSTART(e));
    enum { INITSIZ = 1024 };
    size_t fmtcap = INITSIZ;
    char*  fmtbuf = RKT_malloc(char, fmtcap);
    if (!fmtbuf) { RKT_fatal(-1, "malloc"); }
    for (size_t i = 0; i < t.count; ++i) {
        size_t bytes = RKT_print_assertres(e, t.results[i], &fmtbuf, &fmtcap);
        fwrite(fmtbuf, 1, bytes, files[0]);
    }
    free(fmtbuf);
    int state = t.fails > 0;
    if (t.count
        && (t.results[t.count - 1].res.res == RKT_PREMATURE_EXIT
            || t.results[t.count - 1].res.res == RKT_PREMATURE_SIGN)) {
        state = t.results[t.count - 1].res.res;
    }
    for (int i = 0; i < 2; ++i) {
        if (t.capt[i].dat
            && (state || e->attrs._[1 + i].verbosity == RK_TEST_LOG_ALWAYS)) {
            fwrite(t.capt[i].dat, 1, t.capt[i].len, files[1 + i]);
        }
    }
    switch (state) {
    case 0: fprintf(files[0], RKT_FMTTEST_P(t)); break; // NOLINT
    case 1: fprintf(files[0], RKT_FMTTEST_F(t)); break; // NOLINT
    case RKT_PREMATURE_SIGN:
        fprintf(files[0], RKT_FMTTEST_UC(t, t.term_code)); // NOLINT
        break;
    case RKT_PREMATURE_EXIT:
        fprintf(files[0], RKT_FMTTEST_UE(t, t.term_code)); // NOLINT
        break;
    }
    return state;
}

static inline void RKT_cleanup(const RKT_TestEntry* t, FILE** files,
                               RKT_TestResult* res) {
    while (res->count--) { free(res->results[res->count].res.args[0]); }
    free(res->capt[0].dat), free(res->capt[1].dat);
    free(res->results);
    for (int i = 0; i < 3; ++i) {
        if (t->attrs._[i].path) { fclose(files[i]); }
    }
}

static inline int RKT_run_test(const RKT_TestEntry* t) {
    rk_fd          pipes[3];
    FILE*          files[3];
    rk_pid         pid     = RKT_init_test(t, pipes, files);
    RKT_TestResult results = RKT_parent_loop(pid, pipes);
    int            res     = RKT_print_testres(t, results, files);
    RKT_cleanup(t, files, &results);
    return res;
}
// clang-format on
/// PLATFORM DIFFERENCES
#ifndef _WIN32
# define RKT_CONSTRUCTOR(fn) __attribute__((constructor)) static void fn(void)
# define RK_TEST_SPECIALCHILDENTRY()

NORETURN static inline void RKT_fatal(rk_pid pid, const char* str) {
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

static inline RKT_TestResult RKT_parent_loop(rk_pid pid, rk_fd* pipes) {
    enum { INITSIZ = 512 };
    size_t         rescap  = INITSIZ;
    size_t         caps[2] = {INITSIZ, INITSIZ};
    RKT_TestResult r       = {.count   = 0,
                              .fails   = 0,
                              .results = RKT_malloc(RKT_AssertDat, rescap),
                              .capt    = {{.dat = RKT_malloc(char, caps[0])},
                                          {.dat = RKT_malloc(char, caps[1])}}};
    if (!r.results || !r.capt[0].dat || !r.capt[1].dat) {
        RKT_fatal(pid, "malloc buf");
    }
    struct pollfd fds[3]
        = {{pipes[1], POLLIN, 0}, {pipes[2], POLLIN, 0}, {pipes[0], POLLIN, 0}};
    bool reading_res = 0;
    for (int nfds = 3; nfds > 0;) {
        while (poll(fds, 3, -1) == -1) {
            if (errno != EINTR) { RKT_fatal(pid, "poll"); }
        }
        if (fds[2].revents & POLLIN) {
            if (reading_res == 0) {
                if (r.count == rescap) {
                    rescap    *= 2;
                    r.results  = RKT_realloc(RKT_AssertDat, r.results, rescap);
                    if (!r.results) { RKT_fatal(pid, "realloc 0"); }
                }
                RKT_AssertHdr hdr = {};
                switch (RKT_read_full_nb(fds[2].fd, &hdr, sizeof(hdr))) {
                case -1: RKT_fatal(pid, "read meta hdr");
                case 0 : fds[2].fd = -1, --nfds; break;
                case 1 : r.results[r.count++].hdr = hdr, reading_res = 1;
                }
            } else {
                RKT_AssertRes res = {};
                switch (RKT_read_full_nb(fds[2].fd, &res,
                                         sizeof(RKT_AssertResPkG))) {
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
                        for (size_t i = 1, len = 0; len < res.len - 1; ++len) {
                            assert(i < 3);
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
        for (int i = 0; i < 2; ++i) {
            if (!(fds[i].revents & POLLIN)) { continue; }
            for (;;) {
                ssize_t rd = read(fds[i].fd, r.capt[i].dat + r.capt[i].len,
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
                    r.capt[i].dat  = RKT_realloc(char, r.capt[i].dat, caps[i]);
                    if (!r.capt[i].dat) { RKT_fatal(pid, "realloc"); }
                }
            }
        }
    }
    close(pipes[0]), close(pipes[1]), close(pipes[2]);
    int status = RKT_reapchild(pid);
    if (reading_res) {
        RKT_AssertRes res = {};
        if (WIFSIGNALED(status)) {
            if (r.results[r.count - 1].hdr.F == RKTF_CRASH) {
                res.args[0] = (char*)calloc(1, 32);
                if (!res.args[0]) { RKT_fatal(-1, "calloc"); }
                res.len = sprintf(res.args[0], "%d", WTERMSIG(status)) + 1;
                if (WTERMSIG(status) != r.results[r.count - 1].hdr.code) {
                    res.res = 2;
                }
            } else {
                res.res = RKT_PREMATURE_SIGN, r.term_code = WTERMSIG(status);
            }
        } else {
            if (r.results[r.count - 1].hdr.F == RKTF_EXIT) {
                res.args[0] = (char*)calloc(1, 32);
                if (!res.args[0]) { RKT_fatal(-1, "calloc"); }
                res.len = sprintf(res.args[0], "%d", WEXITSTATUS(status)) + 1;
                if (WEXITSTATUS(status) != r.results[r.count - 1].hdr.code) {
                    res.res = 2;
                }
            } else {
                res.res = RKT_PREMATURE_EXIT, r.term_code = WEXITSTATUS(status);
            }
        }
        if (res.res) { ++r.fails; }
        r.results[r.count - 1].res = res;
    }
    return r;
}

#else

static inline DWORD RKT_reapchild(RKT_TestEntry* t) {
    DWORD status;
    WaitForSingleObject(t->pid.hProcess, INFINITE);
    GetExitCodeProcess(t->pid.hProcess, &status);
    CloseHandle(t->pid.hProcess), CloseHandle(t->pid.hThread);
    return status;
}

# define RK_ENVBLOCK_SIZE 256
# define RKT_CONSTRUCTOR(fn)                                                   \
     static void fn(void);                                                     \
     __declspec(allocate(".CRT$XCU")) static void (*fn##_ptr)(void) = fn;      \
     static void fn(void)
# define RK_TEST_SPECIALCHILDENTRY()                                           \
     do {                                                                      \
         char buf[RK_ENVBLOCK_SIZE];                                           \
         if (GetEnvironmentVariableA("RK_CHILD_FN", buf, sizeof(buf))) {       \
             for (RKT_TestEntry* t = RKT_glob.tests.head; t; t = t->next) {    \
                 if (!strcmp(buf, t->name)) {                                  \
                     GetEnvironmentVariableA("RK_CHILD_META", buf,             \
                                             sizeof(buf));                     \
                     t->func((rk_fd)(uintptr_t)strtoumax(buf, NULL, 10));      \
                     exit(0);                                                  \
                 }                                                             \
             }                                                                 \
         }                                                                     \
     } while (0);

static inline rk_pid RKT_init_test(const RKT_TestEntry* e, rk_fd* pipes,
                                   FILE** files) {
    SECURITY_ATTRIBUTES sa = {.nLength              = sizeof(sa),
                              .lpSecurityDescriptor = NULL,
                              .bInheritHandle       = TRUE};
    rk_fd               w_ends[3];
    for (int i = 0; i < 3; ++i) {
        if (!CreatePipe(&pipes[i], &w_ends[i], &sa, 0)
            || !SetHandleInformation(pipes[i], HANDLE_FLAG_INHERIT, 0)) {
            exit(1);
        }
    }
    // todo memset pi 0
    char path[MAX_PATH];
    if (GetModuleFileNameA(NULL, path, MAX_PATH) == 0) { exit(1); }
    size_t extra = lenof("RK_CHILD_FN=") + strlen(e->name) + 1
                 + lenof("RK_CHILD_META=") + 32 + 1 + 1;
    LPCH   penv = GetEnvironmentStringsA();
    size_t plen = 0;
    if (penv) {
        LPCH p = penv;
        while (*p) { p += strlen(p) + 1; }
        plen = p - penv; // double null not included here
    }
    char *nenv = RKT_malloc(char, extra + plen), *s = nenv;
    if (!nenv) { exit(1); }
    s += sprintf(s, "RK_CHILD_FN=%s", e->name) + 1;
    s += sprintf(s, "RK_CHILD_META=%" PRIuPTR, (uintptr_t)w_ends[0]) + 1;
    if (plen) { s += (memcpy(s, penv, plen), plen); }
    *s             = '\0'; /* ensure double-null terminator */
    STARTUPINFO si = {.cb         = sizeof(si),
                      .hStdOutput = w_ends[1],
                      .hStdError  = w_ends[2],
                      .dwFlags    = STARTF_USESTDHANDLES};
    rk_pid      pid;
    if (!CreateProcessA(path, path, NULL, NULL, TRUE, 0, nenv, NULL, &si,
                        &pid)) {
        if (penv) { FreeEnvironmentStringsA(penv); }
        free(nenv);
        CloseHandle(w_ends[0]), CloseHandle(w_ends[1]), CloseHandle(w_ends[2]);
        exit(1);
    }
    if (penv) { FreeEnvironmentStringsA(penv); }
    free(nenv);
    CloseHandle(w_ends[0]), CloseHandle(w_ends[1]), CloseHandle(w_ends[2]);
    RKT_setup_files(e, pid, files);
    return pid;
}

static inline int RKT_parent_loop(RKT_TestEntry* t, rk_fd* pipes) {
    printf(RK_TFMT_TEST_RUNSTART(t));
    enum { INITSIZ = 512 };
    size_t lens[3] = {}, caps[3] = {INITSIZ, INITSIZ, INITSIZ};
    size_t fmtlen  = 16384;
    char*  bufs[3] = {(char*)malloc(caps[0]), (char*)malloc(caps[1]),
                      (char*)malloc(caps[2])};
    char*  fmtbuf  = malloc(fmtlen);
    if (!bufs[0] || !bufs[1] || !bufs[2] || !fmtbuf) {
        RKT_fatal(t, "malloc buf");
    }
    char*            s = fmtbuf + lenof(RK_TFMT_P);
    RKT_AssertHdr    hdr;
    RKT_AssertResPkG res;
    FILE*            fm          = w_files[0];
    int              vb          = RKT_glob.attrs.meta.verbosity;
    rk_fd            handles[4]  = {pipes[0], pipes[1], pipes[2]};
    bool             reading_res = 0;
    for (int nfds = 3; nfds > 0;) {
        DWORD wait = WaitForMultipleObjects(nfds, handles, FALSE, INFINITE);
        if (wait == WAIT_FAILED) { RKT_fatal(t, "WaitForMultipleObjects"); }
        int idx = wait - WAIT_OBJECT_0;
        if (handles[idx] == pipes[0]) {
            if (!reading_res) {
                switch (RKT_read_full_nb(handles[idx], &hdr, sizeof(hdr))) {
                case -1: RKT_fatal(t, "read meta hdr");
                case 0 : handles[idx] = INVALID_HANDLE_VALUE; break;
                case 1:
                    s += sprintf(s, RK_TFMT_STUB(hdr, t));
                    ++t->total, reading_res = 1;
                }
            } else {
                switch (RKT_read_full_nb(handles[idx], &res, sizeof(res))) {
                case -1: RKT_fatal(t, "read meta res");
                case 0 : handles[idx] = INVALID_HANDLE_VALUE; break;
                case 1:
                    if (res.len > caps[0]) {
                        caps[0] *= 2;
                        bufs[0]  = (char*)realloc(bufs[0], caps[0]);
                        if (!bufs[0]) { RKT_fatal(t, "realloc 0"); }
                    }
                    if (RKT_read_full_nb(handles[idx], bufs[0], res.len)) {
                        RKT_fatal(t, "read");
                    }
                    if (res.res) {
                        ++t->fails;
                        if (256 + res.len > fmtlen) {
                            size_t offs = s - fmtbuf;
                            fmtlen      = 256 + res.len;
                            fmtbuf      = (char*)realloc(fmtbuf, fmtlen);
                            if (!fmtbuf) { RKT_fatal(t, "realloc fmtbuf"); }
                            s = fmtbuf + offs;
                        }
                        RK_catlit(fmtbuf, RK_TFMT_F);
                        s += RK_test_log_failmsg(hdr, res, bufs[0], res.len, s);
                        fwrite(fmtbuf, 1, s - fmtbuf, fm);
                    } else if (vb == RK_TEST_LOG_ALWAYS) {
                        RK_catlit(fmtbuf, RK_TFMT_P), *s++ = '\n';
                        fwrite(fmtbuf, 1, s - fmtbuf, fm);
                    }
                    s = fmtbuf + lenof(RK_TFMT_P), reading_res = 0;
                }
            }
        } else {
            size_t i = handles[idx] == pipes[1] ? 1 : 2;
            DWORD  r;
            if (!ReadFile(pipes[i], bufs[i] + lens[i],
                          (DWORD)(caps[i] - lens[i]), &r, NULL)) {
                if (GetLastError() == ERROR_BROKEN_PIPE) {
                    handles[idx] = handles[idx + 1], --nfds;
                } else {
                    RKT_fatal(t, "ReadFile stdout/stderr");
                }
            } else if (r == 0) {
                handles[idx] = INVALID_HANDLE_VALUE;
            } else {
                lens[i] += r;
                if (lens[i] == caps[i]) {
                    caps[i] *= 2, bufs[i] = (char*)realloc(bufs[i], caps[i]);
                    if (!bufs[i]) { RKT_fatal(t, "realloc stdout/stderr"); }
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
    return RK_test_end_parent_loop(t, hdr, RKT_reapchild(t), reading_res, bufs,
                                   lens, fmtbuf, s);
}

#endif

#ifndef _WIN32

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

static inline rk_pid RKT_init_test(const RKT_TestEntry* e, rk_fd* pipes,
                                   FILE** files) {
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
        if (dup2(pipes[1], STDOUT_FILENO) < 0
            || dup2(pipes[2], STDERR_FILENO) < 0) {
            perror("dup2"), exit(1);
        }
        e->func(pipes[0]), _exit(0);
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

#else

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
#undef RK_test_GENFUN_cmpbasic
#undef RK_test_GENFUN_inrange
#undef RK_test_GENFUNS_floatable
#undef RK_test_GENFUNS_floatonly
#undef RK_test_GENFUNS_nofloat
#undef RK_test_GENFUNS_float
#undef RK_INRED
#undef RK_INGREEN
#undef RK_INBLUE
#undef RK_INYELLOW
#undef RK_TEST_GEN_TYPE_STRUCT_EL
#undef RK_TEST_GEN_T_SWITCHCASE
#undef RK_TEST_GEN_TYPE_ENUM_EL
#pragma GCC diagnostic pop

#endif
