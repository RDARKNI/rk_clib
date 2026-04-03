
#if 0 
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
# ifndef RK_TEST_H
#  define RK_TEST_H

#  ifdef _WIN32
#   include <inttypes.h>
#   include <windows.h>
#  else
#   include <fcntl.h>
#   include <poll.h>
#   include <sys/ioctl.h>
#   include <sys/stat.h>  // defines struct stat and stat/fstat functions
#   include <sys/types.h> // defines types like off_t
#   include <sys/wait.h>
#   include <unistd.h> // defines fstat() and other POSIX functions
#  endif
#  include <assert.h>
#  include <errno.h>
#  ifdef __cplusplus
#   include <cmath>
#   include <sstream> // needed for std::ostringstream
#   include <type_traits>
#  else
#   include <math.h>
#  endif
#  include <stdbool.h>
#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>
#  include <time.h>
/// @brief RK_REGISTER_TEST(fn, .tags=NULL, .suite=NULL) {fn-body}
#  define RK_REGISTER_TEST(fn, ...)                                            \
      static void fn(rk_fd RK_OUT);                                            \
      RK_CTOR(RK_test_register_##fn) {                                         \
          static RK_test_entry entry = {.func  = fn,                           \
                                        .file  = __FILE__,                     \
                                        .name  = #fn,                          \
                                        .attrs = {0, __VA_ARGS__}};            \
          if (!RK_test_glob.tests.head) {                                      \
              RK_test_glob.tests.head = RK_test_glob.tests.tail = &entry;      \
          } else {                                                             \
              RK_test_glob.tests.tail = RK_test_glob.tests.tail->next          \
                  = &entry;                                                    \
          }                                                                    \
      }                                                                        \
      static void fn(rk_fd RK_OUT)

#  define RK_RUN_TESTS(...)                                                    \
      do {                                                                     \
          RK_TEST_SPECIALCHILDENTRY()                                          \
          RK_test_run_all_tests((RK_test_CustomAttrs){__VA_ARGS__});           \
                                                                               \
      } while (0)

// clang-format off
#define rk_expect_true(expr)                  RK_EXPECT(RK_test_true, RK_TEST_TRUE, "rk_expect_true("#expr")", expr)
#define rk_expect_false(expr)                 RK_EXPECT(RK_test_false, RK_TEST_FALSE, "rk_expect_false("#expr")", expr)
#define rk_expect_null(ptr)                   RK_EXPECT(RK_test_eqnull, RK_TEST_EQNULL, "rk_expect_null("#ptr")", ptr)
#define rk_expect_nonnull(ptr)                RK_EXPECT(RK_test_neqnull, RK_TEST_NEQNULL, "rk_expect_nonnull("#ptr")", ptr)
#define rk_expect_memeq(ptr1, ptr2, siz)      RK_EXPECT(RK_test_memeq, RK_TEST_MEMEQ, "rk_expect_memeq(" #ptr1 ", " #ptr2 ", " #siz ")", ptr1, ptr2, siz)
#define rk_expect_memneq(ptr1, ptr2, siz)     RK_EXPECT(RK_test_memneq, RK_TEST_MEMNEQ, "rk_expect_memneq(" #ptr1 ", " #ptr2 ", " #siz ")", ptr1, ptr2, siz)
#define rk_expect_memzero(ptr, siz)           RK_EXPECT(RK_test_memzero, RK_TEST_MEMZERO, "rk_expect_memzero(" #ptr ",  " #siz ")", ptr, siz)
#define rk_expect_memnzero(ptr, siz)          RK_EXPECT(RK_test_memnzero, RK_TEST_MEMNZERO, "rk_expect_memnzero(" #ptr ",  " #siz ")", ptr, siz)

#define rk_expect_eq(exp, act)                RK_EXPECT(RK_test_SEL_TF_FUN(eq, exp), RK_TEST_EQ, "rk_expect_eq("#exp", " #act")", exp, act)
#define rk_expect_neq(exp, act)               RK_EXPECT(RK_test_SEL_TF_FUN(neq, exp), RK_TEST_NEQ, "rk_expect_neq("#exp", " #act")", exp, act)
#define rk_expect_lt(exp, act)                RK_EXPECT(RK_test_SEL_TF_FUN(eq, exp), RK_TEST_LT, "rk_expect_lt("#exp", " #act")", exp, act)
#define rk_expect_leq(exp, act)               RK_EXPECT(RK_test_SEL_TF_FUN(neq, exp), RK_TEST_LEQ, "rk_expect_leq("#exp", " #act")", exp, act)
#define rk_expect_gt(exp, act)                RK_EXPECT(RK_test_SEL_TF_FUN(eq, exp), RK_TEST_GT, "rk_expect_gt("#exp", " #act")", exp, act)
#define rk_expect_geq(exp, act)               RK_EXPECT(RK_test_SEL_TF_FUN(neq, exp), RK_TEST_GEQ, "rk_expect_geq("#exp", " #act")", exp, act)
#define rk_expect_inrange(val, low, high)     RK_EXPECT(RK_test_SEL_TF_FUN(inrange, val), RK_TEST_INRANGE, "rk_expect_inrange("#low", " #val ", " #high ")", val, low, high)
#define rk_expect_floateq_tol(exp, act, tol)  RK_EXPECT(RK_test_SEL_TF_FUN_TOL(eq, tol), RK_TEST_FLOATEQ_TOL, "rk_expect_floateq_tol("#exp", " #act ", " #tol ")", exp, act, tol)
#define rk_expect_floatneq_tol(exp, act, tol) RK_EXPECT(RK_test_SEL_TF_FUN_TOL(neq, tol), RK_TEST_FLOATNEQ_TOL,  "rk_expect_floatneq_tol("#exp", " #act ", " #tol ")", exp, act, tol)
#define rk_expect_streq(str1, str2)           RK_EXPECT(RK_test_streq, RK_TEST_STREQ, "streq(" #str1 ", " #str2 ")",str1,str2)
#define rk_expect_strneq(str1, str2)          RK_EXPECT(RK_test_strneq, RK_TEST_STRNEQ,"strneq(" #str1 ", " #str2 ")",str1,str2)                                    \

// todo not implemented 
#define rk_expect_stdouteq(errstr, ...)       RK_test_sendhdr("stderror prints " #errstr, RK_TEST_STDOUTEQ, RK_TEST_T_vp)
#define rk_expect_stdoutneq(errstr, ...)      RK_test_sendhdr("stderror prints " #errstr, RK_TEST_STDOUTNEQ, RK_TEST_T_vp)
#define rk_expect_stderreq(errstr, ...)       RK_test_sendhdr("stderror prints " #errstr, RK_TEST_STDERREQ, RK_TEST_T_vp)
#define rk_expect_stderrneq(errstr, ...)      RK_test_sendhdr("stderror prints " #errstr, RK_TEST_STDERRNEQ, RK_TEST_T_vp)

#define rk_assert_true(expr)                  RK_ASSERT(RK_test_true, RK_TEST_TRUE, "rk_assert_true("#expr")", expr)
#define rk_assert_false(expr)                 RK_ASSERT(RK_test_false, RK_TEST_FALSE, "rk_assert_false("#expr")", expr)
#define rk_assert_null(ptr)                   RK_ASSERT(RK_test_eqnull, RK_TEST_EQNULL, "rk_assert_null("#ptr")", ptr)
#define rk_assert_nonnull(ptr)                RK_ASSERT(RK_test_neqnull, RK_TEST_NEQNULL, "rk_assert_nonnull("#ptr")", ptr)
#define rk_assert_memeq(ptr1, ptr2, siz)      RK_ASSERT(RK_test_memeq, RK_TEST_MEMEQ, "rk_assert_memeq(" #ptr1 ", " #ptr2 ", " #siz ")", ptr1, ptr2, siz)
#define rk_assert_memneq(ptr1, ptr2, siz)     RK_ASSERT(RK_test_memneq, RK_TEST_MEMNEQ, "rk_assert_memneq(" #ptr1 ", " #ptr2 ", " #siz ")", ptr1, ptr2, siz)
#define rk_assert_memzero(ptr, siz)           RK_ASSERT(RK_test_memzero, RK_TEST_MEMZERO, "rk_assert_memzero(" #ptr ",  " #siz ")", ptr, siz)
#define rk_assert_memnzero(ptr, siz)          RK_ASSERT(RK_test_memnzero, RK_TEST_MEMNZERO, "rk_assert_memnzero(" #ptr ",  " #siz ")", ptr, siz)

#define rk_assert_eq(exp, act)                RK_ASSERT(RK_test_SEL_TF_FUN(eq, exp), RK_TEST_EQ, "rk_assert_eq("#exp", " #act")", exp, act)
#define rk_assert_neq(exp, act)               RK_ASSERT(RK_test_SEL_TF_FUN(neq, exp), RK_TEST_NEQ, "rk_assert_neq("#exp", " #act")", exp, act)
#define rk_assert_lt(exp, act)                RK_ASSERT(RK_test_SEL_TF_FUN(eq, exp), RK_TEST_LT, "rk_assert_lt("#exp", " #act")", exp, act)
#define rk_assert_leq(exp, act)               RK_ASSERT(RK_test_SEL_TF_FUN(neq, exp), RK_TEST_LEQ, "rk_assert_leq("#exp", " #act")", exp, act)
#define rk_assert_gt(exp, act)                RK_ASSERT(RK_test_SEL_TF_FUN(eq, exp), RK_TEST_GT, "rk_assert_gt("#exp", " #act")", exp, act)
#define rk_assert_geq(exp, act)               RK_ASSERT(RK_test_SEL_TF_FUN(neq, exp), RK_TEST_GEQ, "rk_assert_geq("#exp", " #act")", exp, act)
#define rk_assert_inrange(val, low, high)     RK_ASSERT(RK_test_SEL_TF_FUN(inrange, val), RK_TEST_INRANGE, "rk_assert_inrange("#low", " #val ", " #high ")", val, low, high)
#define rk_assert_floateq_tol(exp, act, tol)  RK_ASSERT(RK_test_SEL_TF_FUN_TOL(eq, tol), RK_TEST_FLOATEQ_TOL, "rk_assert_floateq_tol("#exp", " #act ", " #tol ")", exp, act, tol)
#define rk_assert_floatneq_tol(exp, act, tol) RK_ASSERT(RK_test_SEL_TF_FUN_TOL(neq, tol), RK_TEST_FLOATNEQ_TOL,  "rk_assert_floatneq_tol("#exp", " #act ", " #tol ")", exp, act, tol)
#define rk_assert_streq(str1, str2)           RK_ASSERT(RK_test_streq, RK_TEST_STREQ, "streq(" #str1 ", " #str2 ")",str1,str2)
#define rk_assert_strneq(str1, str2)          RK_ASSERT(RK_test_strneq, RK_TEST_STRNEQ,"strneq(" #str1 ", " #str2 ")",str1,str2)  

// todo not implemented
#define rk_assert_stdouteq(errstr, ...)  RK_test_sendhdr("stderror prints " #errstr, RK_TEST_STDOUTEQ, RK_TEST_T_vp)
#define rk_assert_stdoutneq(errstr, ...) RK_test_sendhdr("stderror prints " #errstr, RK_TEST_STDOUTNEQ, RK_TEST_T_vp)
#define rk_assert_stderreq(errstr, ...)  RK_test_sendhdr("stderror prints " #errstr, RK_TEST_STDERREQ, RK_TEST_T_vp)
#define rk_assert_stderrneq(errstr, ...) RK_test_sendhdr("stderror prints " #errstr, RK_TEST_STDERRNEQ, RK_TEST_T_vp)

#define rk_assert_crash(signal, ...)                                           \
    do {                                                                       \
        RK_test_sendhdr_sig(1, "assert_crash(" #signal ", " #__VA_ARGS__ ")",  \
                            RK_TEST_DEATH, signal);                            \
        __VA_ARGS__;                                                           \
        rk_write_full(RK_OUT, (RK_TestRes[]){{.res = 1}}, sizeof(RK_TestRes)); \
        exit(0);                                                               \
    } while (0)

#define rk_assert_exit(code, ...)                                              \
    do {                                                                       \
        RK_test_sendhdr_sig(1, "rk_assert_exit(" #code ", " #__VA_ARGS__ ")",  \
                            RK_TEST_EXIT, code);                               \
        __VA_ARGS__;                                                           \
        rk_write_full(RK_OUT, (RK_TestRes[]){{.res = 1}}, sizeof(RK_TestRes)); \
        exit(0);                                                               \
    } while (0)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

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


// clang-format off

#define RK_RESET         "\x1b[0m"
#define RK_RED           "\x1b[31m"
#define RK_GREEN         "\x1b[32m"
#define RK_BLUE          "\x1b[32m"
#define RK_YELLOW        "\x1b[33m"
#define RK_INRED(str)    RK_RED str RK_RESET
#define RK_INGREEN(str)  RK_GREEN str RK_RESET
#define RK_INBLUE(str)   RK_BLUE str RK_RESET
#define RK_INYELLOW(str) RK_YELLOW str RK_RESET

#define RK_TFMT_TRUE(res)    "Expected " RK_INGREEN("'true'") ", got: " RK_INRED("'false'") "."     
#define RK_TFMT_FALSE(res)   "Expected " RK_INGREEN("'false'") ", got: " RK_INRED("'true'") "."        
#define RK_TFMT_NULL(res)    "Expected " RK_INGREEN("'NULL'") ", got: " RK_INRED("'%s'") ".",          \
                               res.objs[0]
#define RK_TFMT_NONNULL(res) "Expected " RK_INGREEN("nonnull pointer") ", got: " RK_INRED("'%s'") ".", \
                               res.objs[0]
#define RK_TFMT_MEMEQ(res)   "Memory Regions %s and %s (%s bytes long) " RK_INRED("not equal") ".",   \
                               res.objs[0], res.objs[1], res.objs[2]
#define RK_TFMT_MEMNEQ(res)  "Memory Regions %s and %s (%s bytes long) " RK_INRED("equal") ".",       \
                               res.objs[0], res.objs[1], res.objs[2]
#define RK_TFMT_MEMZERO(res) "%s (%s bytes long) nonzero at byte " RK_INRED("%zu") ".",               \
                               res.objs[0], res.objs[1], res.res - 1
#define RK_TFMT_MEMNZER(res) "%s (%s bytes long) zero at byte " RK_INRED("%zu") ".",                  \
                               res.objs[0], res.objs[1], res.res - 1
#define RK_TFMT_STREQ(res)   "strings %s, %s " RK_INRED("not equal") " at position %zu.",    \
                               res.objs[0], res.objs[1], res.res - 1
#define RK_TFMT_STRNEQ(res)  "strings (len: %s) " RK_INRED("equal") ".",                              \
                               res.objs[0]
#define RK_TFMT_DEATH(hdr)   " - Expected: termination via signal %d, got: no termination",            \
                               (int)(hdr.T)
#define RK_TFMT_EXIT(hdr)    " - Expected: exit with code %d, got: no exit",                           \
                               (int)(hdr.T)
                   
#define RK_TFMT_P RK_INGREEN("[P] ")
#define RK_TFMT_F   RK_INRED("[F] ")
#define RK_TFMT_C   RK_INRED("[C] ")
#define RK_TFMT_E   RK_INRED("[E] ")

#define RK_TFMT_STUB(hdr, t)     "%s:%d %s", t->file, hdr.pos, hdr.exprstr
#define RK_TFMT_ASRT_FE(exp,act) " - Expected: exit with code %d, got: exit with code %d\n",                \
                                 exp.code, act.code
#define RK_TFMT_ASRT_FC(exp,act) " - Expected: termination via signal %d, got: termination via signal %d\n",\
                                 exp.code, act.code
#define RK_TFMT_ASRT_UC(exp)     " - Expected: exit with code %d\n",                                        \
                                 exp.code
#define RK_TFMT_ASRT_UE(exp)     " - Expected: termination via signal %d\n",                                \
                                 exp.code

#define RK_TFMT_TEST_RUNSTART(t)                                                                            \
    RK_INYELLOW("Running %s...\n")"--------------------------------------------------\n",                   \
    t->name
#define RK_TFMT_TEST_PASS(t)                                                                                \
    "-> " RK_INGREEN("PASS") ": All %u assertions succeeded\n\n",                                           \
    t->total
#define RK_TFMT_TEST_FAIL(t)                                                                                \
    "-> " RK_INRED("FAIL") ": %u fails, %u passed (total %u)\n\n",                                          \
    t->fails, t->total - t->fails, t->total
#define RK_TFMT_TEST_UCRASH(t, sig)                                                                         \
    "-> " RK_INRED("FAIL") ": %u fails, %u passed (total %u), crashed unexpectedly with signal %d\n\n",     \
    t->fails, t->total - t->fails, t->total, sig
#define RK_TFMT_TEST_UEXIT(t, code)                                                                         \
    "-> " RK_INRED("FAIL") ": %u fails, %u passed (total %u), exited unexpectedly with code %d\n\n",        \
    t->fails, t->total - t->fails, t->total, code

#define RK_TFMT_TOTALRES(total, fails, crashed)                                                             \
    "--- Passed %zu, fails %zu, crashed %zu out of %zu tests ---\n",                                        \
    total - fails - crashed, fails, crashed, total

// clang-format on
#  define RK_TEST_TYPELIST_NOFLOAT(Y, ...)                                     \
      Y(unsigned char, uc, "%hhu", ##__VA_ARGS__)                              \
      Y(unsigned short, us, "%hu", ##__VA_ARGS__)                              \
      Y(unsigned, ui, "%u", ##__VA_ARGS__)                                     \
      Y(unsigned long, ul, "%lu", ##__VA_ARGS__)                               \
      Y(unsigned long long, ull, "%llu", ##__VA_ARGS__)                        \
      Y(char, c, "%c", ##__VA_ARGS__)                                          \
      Y(short, s, "%hd", ##__VA_ARGS__)                                        \
      Y(int, i, "%d", ##__VA_ARGS__)                                           \
      Y(long, sl, "%ld", ##__VA_ARGS__)                                        \
      Y(long long, sll, "%lld", ##__VA_ARGS__)                                 \
      Y(bool, b, "%d", ##__VA_ARGS__)                                          \
      Y(const void*, vp, "%p", ##__VA_ARGS__)
#  define RK_TEST_TYPELIST_FLOAT(Y, ...)                                       \
      Y(float, f, "%f", ##__VA_ARGS__)                                         \
      Y(double, d, "%f", ##__VA_ARGS__)                                        \
      Y(long double, ld, "%Lf", ##__VA_ARGS__)
#  define RK_TEST_TYPELIST(Y, ...)                                             \
      RK_TEST_TYPELIST_FLOAT(Y, ##__VA_ARGS__)                                 \
      RK_TEST_TYPELIST_NOFLOAT(Y, ##__VA_ARGS__)
#  define RK_TEST_GEN_TYPE_ENUM_EL(T, N, ...)        RK_TEST_T_##N,
#  define RK_TEST_GEN_TYPE_STRUCT_EL(T, N, FMT, ...) T N;

enum RK_test_FUN {
    RK_TEST_TRUE,
    RK_TEST_FALSE,
    RK_TEST_EQNULL,
    RK_TEST_NEQNULL,
    RK_TEST_EQ,
    RK_TEST_FLOATEQ_TOL,
    RK_TEST_NEQ,
    RK_TEST_FLOATNEQ_TOL,
    RK_TEST_LT,
    RK_TEST_LEQ,
    RK_TEST_GT,
    RK_TEST_GEQ,
    RK_TEST_INRANGE,
    RK_TEST_MEMEQ,
    RK_TEST_MEMNEQ,
    RK_TEST_MEMZERO,
    RK_TEST_MEMNZERO,
    RK_TEST_STREQ,
    RK_TEST_STRNEQ,
    RK_TEST_STDOUTEQ,
    RK_TEST_STDOUTNEQ,
    RK_TEST_STDERREQ,
    RK_TEST_STDERRNEQ,
    RK_TEST_DEATH,
    RK_TEST_EXIT
};

enum RK_test_verbosity {
    RK_TEST_LOG_DEFAULT,
    RK_TEST_LOG_ALWAYS,
    RK_TEST_LOG_NEVER
} verbosity;

typedef struct RK_TestStreamOpts {
    enum RK_test_verbosity verbosity;
    const char*            path;
} RK_TestStreamOpts;

// customisable attriutes (global or per test)
typedef struct RK_test_CustomAttrs {
    const char *suite, *tags;
    union {
        struct {
            RK_TestStreamOpts meta, out, err;
        };
        RK_TestStreamOpts _[3];
    };
} RK_test_CustomAttrs;

typedef struct RK_test_entry {
    void (*const func)(rk_fd RK_OUT);
    const char *const file, *const name;
    RK_test_CustomAttrs attrs;
    struct {
        unsigned total;
        unsigned fails;
        unsigned crashed;
    };
    struct RK_test_entry* next;
} RK_test_entry;

static struct {
    RK_test_CustomAttrs attrs;
    FILE*               files[3];
    char                resbufr[2048];
    struct {
        RK_test_entry *head, *tail;
    } tests;
} RK_test_glob;

/// @brief Header to be sent to the test runner before assert/expect
typedef struct {
    enum RK_test_FUN F; ///< The assert function (assert_eq, assert_true)
    int              T;
    int              pos; ///< The line of the function
    const char*      exprstr;
} RK_TestHdr;

/// @brief the arguments and result (true/false or other state) is stored here
typedef struct RK_TestRes {
    size_t len;
    size_t res; ///< The result of the test (0 = pass, other values possible)
    char   args[];
} RK_TestRes;

typedef struct RK_TestResSmall {
    size_t len;
    size_t res; ///< The result of the test (0 = pass, other values possible)
    char   args[1028];
} RK_TestResSmall;

/// Result of I/O operations.
typedef enum RK_IO_RESULT_t {
    RK_IO_RESULT_OK,   ///< All bytes processed successfully
    RK_IO_RESULT_EOF,  ///< Peer closed pipe (read returned 0, write got EPIPE)
    RK_IO_RESULT_ERR,  ///< Unexpected error
    RK_IO_RESULT_AGAIN ///< Would block (nonblocking I/O)
} RK_IO_RESULT_t;
static inline RK_IO_RESULT_t rk_write_full(rk_fd fd, const void* buf,
                                           size_t len);
static inline RK_IO_RESULT_t rk_read_full_nb(rk_fd fd, void* buf,
                                             size_t nbytes);
static inline RK_IO_RESULT_t rk_read_full(rk_fd fd, void* buf, size_t nbytes);
static inline int fwrite_full(FILE* fd, const char* buf, size_t len) {
    for (size_t w; len > 0; buf += w, len -= w) {
        if (!(w = fwrite(buf, 1, len, fd))) { return -1; }
    }
    return 0;
}

static inline void RK_test_send_hdr(rk_fd meta_w, RK_TestHdr s) {
    rk_write_full(meta_w, &s, sizeof(s));
}
static inline void RK_test_send_res_expect(rk_fd meta_w, RK_TestRes res) {
    rk_write_full(meta_w, &res, sizeof(res));
}
static inline void RK_test_send_res_assert(rk_fd meta_w, RK_TestRes res) {
    rk_write_full(meta_w, &res, sizeof(res));
    if (res.res) { fflush(stdout), fflush(stderr), _exit(0); }
}

#  define RK_test_sendhdr_sig(IS_ASSERT, EXPRSTR, FUNENUM, sig)                \
      RK_test_send_hdr(RK_OUT, (RK_TestHdr){.F       = FUNENUM,                \
                                            .T       = sig,                    \
                                            .pos     = __LINE__,               \
                                            .exprstr = EXPRSTR})
#  define RK_test_sendhdr(IS_ASSERT, EXPRSTR, FUNENUM)                         \
      RK_test_sendhdr_sig(IS_ASSERT, EXPRSTR, FUNENUM, 0)

#  define RK_EXPECT(FUN, FUNENUM, EXPRSTR, ...)                                \
      (RK_test_sendhdr(0, EXPRSTR, FUNENUM),                                   \
       RK_test_send_res_expect(RK_OUT, FUN(__VA_ARGS__)))

#  define RK_ASSERT(FUN, FUNENUM, EXPRSTR, ...)                                \
      (RK_test_sendhdr(1, EXPRSTR, FUNENUM),                                   \
       RK_test_send_res_assert(RK_OUT, FUN(__VA_ARGS__)))

/// @brief  returns first position where strings differ +1 (0 if same)
static inline size_t RK_test_strdiff(const char* e1, size_t l1, const char* e2,
                                     size_t l2) {
    size_t min = RK_MIN(l1, l2);
    for (size_t i = 0; i < min; ++i) {
        if (e1[i] != e2[i]) { return i + 1; }
    }
    return l1 == l2 ? 0 : (min + 1);
}

#  ifndef __cplusplus
#   define RK_test_SEL_TF_FUN_(T, N, F, FTYPE) , T : RK_test_##FTYPE##_##N
#   define RK_test_SEL_TF_FUN(FTYPE, VAL)                                      \
       _Generic((VAL)RK_TEST_TYPELIST(RK_test_SEL_TF_FUN_, FTYPE),             \
           default: RK_test_##FTYPE##_vp)

#   define RK_test_SEL_TF_FUN_TOL_(T, N, F, FTYPE)                             \
       , T : RK_test_##FTYPE##_tol_##N
#   define RK_test_SEL_TF_FUN_TOL(FTYPE, ...)                                  \
       _Generic((__VA_ARGS__)RK_TEST_TYPELIST_FLOAT(RK_test_SEL_TF_FUN_TOL_,   \
                                                    FTYPE))

#   define RK_test_GENFUN2_sig(T, N, FMT, name)                                \
       static inline RK_TestRes* RK_test_##name##_##N(T e1, T e2)
#   define RK_test_GENFUN3_sig(T, N, FMT, name)                                \
       static inline RK_TestRes* RK_test_##name##_##N(T e1, T e2, T e3)

#  else
#   define RK_test_SEL_TF_FUN(FTYPE, VAL)     RK_test_##FTYPE
#   define RK_test_SEL_TF_FUN_TOL(FTYPE, ...) RK_test_##FTYPE##_tol
#   define RK_test_GENFUN2_sig(T, N, FMT, name)                                \
       static inline RK_TestRes* RK_test_##name(T e1, T e2)
#   define RK_test_GENFUN3_sig(T, N, FMT, name)                                \
       static inline RK_TestRes* RK_test_##name(T e1, T e2, T e3)
#   if __cpp_lib_format >= 201907L // C++20 std::format available
#    include <format>
template <typename T, typename CharT = char>
concept RK_formattable = requires (T const& value) {
    std::formatter<T, CharT>{};
    std::format(std::basic_string<CharT>{"{}"}, value);
};
#   endif

template <typename...>
using RK_void_t = void;
template <typename T, typename U = void>
struct RK_is_streamable : std::false_type {};

template <typename T>
struct RK_is_streamable<
    T, RK_void_t<decltype(std::declval<std::ostream&>() << std::declval<T>())>>
    : std::true_type {};

template <typename T, std::size_t N>
void RK_test_format_to(T value, char (&arr)[N]) {
#   if __cpp_lib_format >= 201907L
    if constexpr (RK_formattable<T>) {
        std::string tmp = std::format("{}", value);
        std::strncpy(arr, tmp.c_str(), N - 1);
        arr[N - 1] = '\0';
    } else // todo if constexpr
#   endif
        if constexpr (RK_is_streamable<T>::value) {
        std::ostringstream oss;
        oss << value;
        std::string tmp = oss.str();
        strncpy(arr, tmp.c_str(), N - 1);
        arr[N - 1] = '\0';
    } else {
        // Neither formattable nor streamable — write empty string
        if (N > 0) { arr[0] = '\0'; }
    }
}
template <class T, class U>
static inline RK_TestRes* RK_test_eq(T e1, U e2) {
    RK_TestRes* res; // TODO
    res.res = !(e1 == e2);
    RK_test_format_to(e1, res.objs[0]), RK_test_format_to(e2, res.objs[1]);
    return res;
}
template <class T, class U>
static inline RK_TestRes* RK_test_neq(T e1, U e2) {
    RK_TestRes res;
    res.res = !(e1 != e2);
    RK_test_format_to(e1, res.objs[0]), RK_test_format_to(e2, res.objs[1]);
    return res;
}
template <class T, class U>
static inline RK_TestRes* RK_test_gt(T e1, U e2) {
    RK_TestRes res;
    res.res = !(e1 > e2);
    RK_test_format_to(e1, res.objs[0]), RK_test_format_to(e2, res.objs[1]);
    return res;
}
template <class T, class U>
static inline RK_TestRes* RK_test_geq(T e1, U e2) {
    RK_TestRes res;
    res.res = !(e1 >= e2);
    RK_test_format_to(e1, res.objs[0]), RK_test_format_to(e2, res.objs[1]);
    return res;
}
template <class T, class U>
static inline RK_TestRes* RK_test_lt(T e1, U e2) {
    RK_TestRes res;
    res.res = !(e1 < e2);
    RK_test_format_to(e1, res.objs[0]), RK_test_format_to(e2, res.objs[1]);
    return res;
}
template <class T, class U>
static inline RK_TestRes* RK_test_leq(T e1, U e2) {
    RK_TestRes res;
    res.res = !(e1 <= e2);
    RK_test_format_to(e1, res.objs[0]), RK_test_format_to(e2, res.objs[1]);
    return res;
}
template <class T, class U, class O>
static inline RK_TestRes* RK_test_inrange(T e1, U e2, O e3) {
    RK_TestRes res;
    res.res = !(e1 < e2 ? 1 : (e1 > e3 ? 2 : 0));
    RK_test_format_to(e1, res.objs[0]);
    RK_test_format_to(e2, res.objs[1]);
    RK_test_format_to(e3, res.objs[2]);
    return res;
}
#  endif

// clang-format off
#define RK_test_GENFUN2(T, N, FMT, name, expr)    \
RK_test_GENFUN2_sig(T, N, FMT, name)                                   {                 \
    RK_TestRes* res=(RK_TestRes*)RK_test_glob.resbufr;\
    res->res = (expr);\
    res->len=0;\
    res->len+=sprintf(res->args,FMT,e1)+1;\
    res->len+=sprintf(res->args+res->len,FMT,e2)+1;\
    return res;\
}
#define RK_test_GENFUN3(T, N, FMT, name, expr)                                       \
 RK_test_GENFUN3_sig(T, N, FMT, name) {           \
    RK_TestRes* res=(RK_TestRes*)RK_test_glob.resbufr;\
    res->res = (expr);\
    res->len=0;\
    res->len+=sprintf(res->args,FMT,e1)+1;\
    res->len+=sprintf(res->args+res->len,FMT,e2)+1;\
    res->len+=sprintf(res->args+res->len,FMT,e3)+1;\
    return res;\
}
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


static inline RK_TestRes* RK_test_true(bool e1) {
    RK_TestRes* res=(RK_TestRes*)RK_test_glob.resbufr;
    res->res = !e1, res->len=0;
    return res;
}
static inline RK_TestRes* RK_test_false(bool e1) {
    RK_TestRes* res=(RK_TestRes*)RK_test_glob.resbufr;
    res->res = !!e1, res->len=0;
    return res;
}
static inline RK_TestRes* RK_test_eqnull(const void* e1) {
    RK_TestRes* res=(RK_TestRes*)RK_test_glob.resbufr;
    res->res = !(e1 == NULL);
    res->len = sprintf(res->args, "%p", e1)+1;
    return res;
}
static inline RK_TestRes* RK_test_neqnull(const void* e1) {
    RK_TestRes* res=(RK_TestRes*)RK_test_glob.resbufr;
    res->res = !(e1 != NULL);
    res->len = sprintf(res->args, "%p", e1)+1;
    return res;
}
static inline RK_TestRes* RK_test_memeq(const void* e1, const void* e2, size_t e3) {
    RK_TestRes* res=(RK_TestRes*)RK_test_glob.resbufr;
    res->res = !((e1 == e2) || (e1 && e2 && !memcmp(e1, e2, e3)));
    res->len = sprintf(res->args, "%p", e1)+1;
    res->len += sprintf(res->args+res->len, "%p", e2)+1;
    res->len += sprintf(res->args+res->len, "%zu", e3)+1;
    return res;
}
static inline RK_TestRes* RK_test_memneq(const void* e1, const void* e2, size_t e3) {
    RK_TestRes* res=(RK_TestRes*)RK_test_glob.resbufr;
    res->res = !!((e1 == e2) || (e1 && e2 && !memcmp(e1, e2, e3)));
    res->len = sprintf(res->args, "%p", e1)+1;
    res->len += sprintf(res->args+res->len, "%p", e2)+1;
    res->len += sprintf(res->args+res->len, "%zu", e3)+1;
    return res;
}
static inline RK_TestRes* RK_test_memzero(const void* e1, size_t e2) {
    size_t      dif = 0;
    for (size_t i = 0; i < e2; ++i) {
        if (((const char*)e1)[i]) { dif = i + 1; break; }
    }
    RK_TestRes* res=(RK_TestRes*)RK_test_glob.resbufr;
    res->res = dif;
    res->len = sprintf(res->args, "%p", e1) + 1;
    res->len += sprintf(res->args+res->len, "%zu", e2) + 1;
    return res;
}
static inline RK_TestRes* RK_test_memnzero(const void* e1, size_t e2) {
    const void* loc;
    RK_TestRes* res=(RK_TestRes*)RK_test_glob.resbufr;

    res->res = (!e2 || !(loc = memchr(e1, 0, e2)))
                                  ? 0
                                  : ((char*)loc - (char*)e1 + 1);
    res->len = sprintf(res->args, "%p", e1) + 1;
    res->len += sprintf(res->args+res->len, "%zu", e2) + 1;
    return res;
}
static inline RK_TestRes* RK_test_streq(const char* e1, const char* e2) {
    size_t     l1 =  strlen(e1), l2 =  strlen(e2);
    size_t     dif = RK_test_strdiff(e1, l1, e2, l2);
    RK_TestRes* res = (RK_TestRes*)malloc(l1 + l2 + 2);
    res->res = dif;
    res->len = l1 + l2 + 2;
    memcpy(res->args, e1, l1+1);
    memcpy(res->args+l1+1, e1, l2+1);
    return res;
}
static inline RK_TestRes* RK_test_strneq(const char* e1, const char* e2) {
    size_t     l1 =  strlen(e1), l2 =  strlen(e2);
    size_t     dif = RK_test_strdiff(e1, l1, e2, l2);
    RK_TestRes* res = (RK_TestRes*)malloc(l1+l2+2);
    res->res = !(dif != 0);
    res->len=l1+l2+2;
    memcpy(res->args, e1, l1+1);
    memcpy(res->args+l1+1, e1, l2+1);
    return res;
}

// clang-format on

// uberb
static inline size_t RK_test_log_failmsg(RK_TestHdr hdr, RK_TestRes res,
                                         char* buf) {
    char* s = buf;
    switch (hdr.F) {
    case RK_TEST_TRUE   : s += RK_catlit(s, RK_TFMT_TRUE(res)); break;
    case RK_TEST_FALSE  : s += RK_catlit(s, RK_TFMT_FALSE(res)); break;
    case RK_TEST_EQNULL : s += sprintf(s, RK_TFMT_NULL(res)); break;
    case RK_TEST_NEQNULL: s += sprintf(s, RK_TFMT_NONNULL(res)); break;
    case RK_TEST_EQ:
    case RK_TEST_FLOATEQ_TOL:
    case RK_TEST_NEQ:
    case RK_TEST_FLOATNEQ_TOL:
    case RK_TEST_LT:
    case RK_TEST_LEQ:
    case RK_TEST_GT:
    case RK_TEST_GEQ:
    case RK_TEST_INRANGE:
        s += RK_catlit(s, " " RK_RED), s += sprintf(s, "%s", res.objs[0]);
        switch (hdr.F) {
        case RK_TEST_EQ:
        case RK_TEST_FLOATEQ_TOL : s += RK_catlit(s, " != "); break;
        case RK_TEST_NEQ         :
        case RK_TEST_FLOATNEQ_TOL: s += RK_catlit(s, " == "); break;
        case RK_TEST_LT:
            if (res.res == 1) {
                s += RK_catlit(s, " == ");
                break;
            } // fallthrough
        case RK_TEST_LEQ: s += RK_catlit(s, " > "); break;
        case RK_TEST_GT:
            if (res.res == 1) {
                s += RK_catlit(s, " == ");
                break;
            } // fallthrough
        case RK_TEST_GEQ: s += RK_catlit(s, " < "); break;
        case RK_TEST_INRANGE:
            s += res.res == 1 ? RK_catlit(s, " < ") : RK_catlit(s, " > ");
            s += sprintf(s, "%s", res.objs[1 + (res.res == 2)]);
            goto afterop2;
        default: unreachable();
        }
        s += sprintf(s, "%s", res.objs[1]);
    afterop2:
        s += RK_catlit(s, RK_RESET);
        if (hdr.F == RK_TEST_FLOATEQ_TOL || hdr.F == RK_TEST_FLOATNEQ_TOL) {
            s += sprintf(s, "(tol: %s)", res.objs[2]);
        }
        break;
    case RK_TEST_MEMEQ   : s += sprintf(s, RK_TFMT_MEMEQ(res)); break;
    case RK_TEST_MEMNEQ  : s += sprintf(s, RK_TFMT_MEMNEQ(res)); break;
    case RK_TEST_MEMZERO : s += sprintf(s, RK_TFMT_MEMZERO(res)); break;
    case RK_TEST_MEMNZERO: s += sprintf(s, RK_TFMT_MEMNZER(res)); break;
    case RK_TEST_STREQ   : s += sprintf(s, RK_TFMT_STREQ(res)); break;
    case RK_TEST_STRNEQ  : s += sprintf(s, RK_TFMT_STRNEQ(res)); break;
    case RK_TEST_DEATH   : s += sprintf(s, RK_TFMT_DEATH(hdr)); break;
    case RK_TEST_EXIT    : s += sprintf(s, RK_TFMT_EXIT(hdr)); break;
    case RK_TEST_STDOUTEQ:
    case RK_TEST_STDOUTNEQ:
    case RK_TEST_STDERREQ:
    case RK_TEST_STDERRNEQ:
        exit(1); // todo not implemented
        break;
    }
    *s++ = '\n';
    return s - buf;
}

static inline void RK_test_run_all_tests(RK_test_CustomAttrs attrs);
static inline int  RK_test_run_test(RK_test_entry* t);
static inline int  RK_test_parent_loop(RK_test_entry* t, rk_pid pid,
                                       rk_fd* r_fds, FILE** w_files);
static inline int  RK_test_end_parent_loop(RK_test_entry* t, RK_TestHdr hdr,
                                           FILE** w_files, int status,
                                           bool reading_res, char** bufs,
                                           size_t* lens, char* buf, char* s);

static inline void RK_test_run_all_tests(RK_test_CustomAttrs attrs) {
    for (int i = 0; i < 3; ++i) {
        if (!attrs._[i].path) {
            RK_test_glob.files[i] = (FILE*[]){stdout, stdout, stderr}[i];
        } else if (!(RK_test_glob.files[i] = fopen(attrs._[i].path, "w"))) {
            perror("fopen meta"), exit(1);
        }
    }
    RK_test_glob.attrs = attrs;
    size_t total = 0, failed = 0, crashed = 0;
    for (RK_test_entry* t = RK_test_glob.tests.head; t; t = t->next) {
        if (attrs.suite
            && (!t->attrs.suite || strcmp(attrs.suite, t->attrs.suite))) {
            continue; // wrong suite
        }
        if (!attrs.tags) { // no tags to look for
        run_test:
            switch (++total, RK_test_run_test(t)) {
            case 2 : ++crashed; continue;
            case 1 : ++failed; continue;
            default: continue;
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

static inline int RK_test_end_parent_loop(RK_test_entry* t, RK_TestHdr hdr,
                                          FILE** w_files, int status,
                                          bool reading_res, char** bufs,
                                          size_t* lens, char* buf, char* s) {
    enum _kind { RK__NORMAL, RK__EXIT, RK__SIGN };
    struct {
        enum _kind kind;
        int        code;
    } exp, act;
    FILE* fm = w_files[0];
    switch (hdr.F) {
    case RK_TEST_DEATH: exp.kind = RK__SIGN, exp.code = hdr.T; break;
    case RK_TEST_EXIT : exp.kind = RK__EXIT, exp.code = hdr.T; break;
    default           : exp.kind = RK__NORMAL, exp.code = 0;
    }
#  ifndef _WIN32
    switch ((act.kind = !reading_res        ? RK__NORMAL
                      : WIFSIGNALED(status) ? RK__SIGN
                                            : RK__EXIT)) {
    case RK__NORMAL: act.code = 0; break;
    case RK__SIGN  : act.code = WTERMSIG(status); break;
    case RK__EXIT  : act.code = WEXITSTATUS(status); break;
    }
#  else
    act.kind = reading_res * (1 + (status >= 0xC0000000));
    act.code = (int)status;
#  endif
    enum { P, F, C, E } test_state = t->fails == 0 ? P : F;
    if (act.kind != RK__NORMAL) { /* Others are handled by res already */
        if (act.kind != exp.kind) {
            if (act.kind == RK__SIGN) {
                ++t->fails, test_state          = C;
                RK_catlit(buf, RK_TFMT_C), *s++ = '\n';
            } else if (act.kind == RK__EXIT) {
                ++t->fails, test_state          = E;
                RK_catlit(buf, RK_TFMT_E), *s++ = '\n';
            }
        } else {
            if (act.code != exp.code) {
                ++t->fails, test_state = F;
                RK_catlit(buf, RK_TFMT_F);
                if (act.kind == RK__SIGN) {
                    s += sprintf(s, RK_TFMT_ASRT_FC(exp, act));
                } else if (act.kind == RK__EXIT) {
                    s += sprintf(s, RK_TFMT_ASRT_FE(exp, act));
                }
            } else {
                if (RK_test_glob.attrs._[0].verbosity != RK_TEST_LOG_ALWAYS) {
                    goto nomore;
                }
                RK_catlit(buf, RK_TFMT_P), *s++ = '\n';
            }
        }
        fwrite(buf, 1, s - buf, w_files[0]);
    }
nomore:
    for (int i = 1; i < 3; ++i) {
        if (test_state == P ? t->attrs._[i].verbosity == RK_TEST_LOG_ALWAYS
                            : t->attrs._[i].verbosity != RK_TEST_LOG_NEVER) {
            if (fwrite_full(w_files[i], bufs[i], lens[i]) < 0) {
                perror("fwrite");
            }
        }
    }
    switch (test_state) {
    case P: fprintf(fm, RK_TFMT_TEST_PASS(t)); break;
    case F: fprintf(fm, RK_TFMT_TEST_FAIL(t)); break;
    case C: fprintf(fm, RK_TFMT_TEST_UCRASH(t, act.code)); break;
    case E: fprintf(fm, RK_TFMT_TEST_UEXIT(t, act.code)); break;
    }
    free(bufs[1]), free(bufs[2]);
    return test_state == P ? 0 : test_state == F ? 1 : 2;
}

///
/// PLATFORM DIFFERENCES
///

#  ifdef _WIN32
#   define RK_ENVBLOCK_SIZE 256
#   define RK_CTOR(fn)                                                         \
       static void fn(void);                                                   \
       __declspec(allocate(".CRT$XCU")) static void (*fn##_ptr)(void) = fn;    \
       static void fn(void)
#   define RK_TEST_SPECIALCHILDENTRY()                                         \
       do {                                                                    \
           char buf[RK_ENVBLOCK_SIZE];                                         \
           if (GetEnvironmentVariableA("RK_CHILD_FN", buf, sizeof(buf))) {     \
               for (RK_test_entry* t = RK_test_glob.tests.head; t;             \
                    t                = t->next) {                                             \
                   if (!strcmp(buf, t->name)) {                                \
                       GetEnvironmentVariableA("RK_CHILD_META", buf,           \
                                               sizeof(buf));                   \
                       t->func((rk_fd)(uintptr_t)strtoumax(buf, NULL, 10));    \
                       exit(0);                                                \
                   }                                                           \
               }                                                               \
           }                                                                   \
       } while (0);

static inline RK_IO_RESULT_t rk_read_full(rk_fd h, void* buf, size_t nbytes) {
    char* ptr = (char*)buf;
    for (DWORD r; nbytes; ptr += r, nbytes -= r) {
        if (!ReadFile(h, ptr, (DWORD)nbytes, &r, NULL)) {
            return GetLastError() == ERROR_BROKEN_PIPE ? RK_IO_RESULT_EOF
                                                       : RK_IO_RESULT_ERR;
        }
        if (!r) { return RK_IO_RESULT_EOF; }
    }
    return RK_IO_RESULT_OK;
}
static inline RK_IO_RESULT_t rk_read_full_nb(rk_fd h, void* buf,
                                             size_t nbytes) {
    char* ptr = (char*)buf;
    for (DWORD r; nbytes;) {
        if (!ReadFile(h, ptr, (DWORD)nbytes, &r, NULL)) {
            switch (GetLastError()) {
            case ERROR_BROKEN_PIPE: return RK_IO_RESULT_EOF;
            case ERROR_NO_DATA:
                if (ptr == buf) { return RK_IO_RESULT_AGAIN; }
                continue;
            default: return RK_IO_RESULT_ERR;
            }
        }
        if (!r) { return RK_IO_RESULT_EOF; }
        ptr += r, nbytes -= r;
    }
    return RK_IO_RESULT_OK;
}
static inline RK_IO_RESULT_t rk_write_full(rk_fd h, const void* buf,
                                           size_t len) {
    const char* ptr = (const char*)buf;
    for (DWORD w; len > 0; ptr += w, len -= w) {
        if (!WriteFile(h, ptr, (DWORD)len, &w, NULL)) {
            return RK_IO_RESULT_ERR; // todo
        }
    }
    return RK_IO_RESULT_OK;
}

static inline int RK_test_run_test(RK_test_entry* t) {
    SECURITY_ATTRIBUTES sa = {.nLength = sizeof(SECURITY_ATTRIBUTES),
                              .lpSecurityDescriptor = NULL,
                              .bInheritHandle       = TRUE};
    rk_fd               r_ends[3], w_ends[3];
    for (int i = 0; i < 3; ++i) {
        if (!CreatePipe(&r_ends[i], &w_ends[i], &sa, 0)
            || !SetHandleInformation(r_ends[i], HANDLE_FLAG_INHERIT, 0)) {
            exit(1);
        }
    }
    FILE* files[3];
    for (int i = 0; i < 3; ++i) {
        if (!t->attrs._[i].path) {
            files[i] = RK_test_glob.files[i];
        } else {
            if (!(files[i] = fopen(t->attrs._[i].path, "w"))) {
                perror("fopen"), exit(1);
            }
        }
    }

    PROCESS_INFORMATION pi = {0};
    { // spawn the child
        char path[MAX_PATH];
        if (GetModuleFileNameA(NULL, path, MAX_PATH) == 0) { exit(1); }
        STARTUPINFO si    = {.cb         = sizeof(si),
                             .hStdOutput = w_ends[1],
                             .hStdError  = w_ends[2],
                             .dwFlags    = STARTF_USESTDHANDLES};

        size_t      extra = lenof("RK_CHILD_FN=") + strlen(t->name) + 1
                     + lenof("RK_CHILD_META=") + 32 + 1 + 1;
        LPCH   penv = GetEnvironmentStringsA();
        size_t plen = 0;
        if (penv) {
            LPCH p = penv;
            while (*p) { p += strlen(p) + 1; }
            plen = p - penv; // double null not included here
        }
        char *nenv = (char*)malloc(extra + plen), *s = nenv;
        if (!nenv) { exit(1); }
        s += sprintf(s, "RK_CHILD_FN=%s", t->name) + 1;
        s += sprintf(s, "RK_CHILD_META=%" PRIuPTR, (uintptr_t)w_ends[0]) + 1;
        if (plen) { s += (memcpy(s, penv, plen), plen); }
        *s = '\0'; /* ensure double-null terminator */
        if (!CreateProcessA(path, path, NULL, NULL, TRUE, 0, nenv, NULL, &si,
                            &pi)) {
            if (penv) { FreeEnvironmentStringsA(penv); }
            free(nenv), exit(1);
        }
        free(nenv);
        if (penv) { FreeEnvironmentStringsA(penv); }
    }

    CloseHandle(w_ends[0]), CloseHandle(w_ends[1]), CloseHandle(w_ends[2]);
    int res = RK_test_parent_loop(t, pi, r_ends, files);
    for (int i = 0; i < 3; ++i) {
        if (t->attrs._[i].path) { fclose(files[i]); }
    }
    CloseHandle(r_ends[0]), CloseHandle(r_ends[1]), CloseHandle(r_ends[2]);
    CloseHandle(pi.hProcess), CloseHandle(pi.hThread);
    return res;
}
static inline int RK_test_parent_loop(RK_test_entry* t, rk_pid pi, rk_fd* r_fds,
                                      FILE** w_files) {
    printf(RK_TFMT_TEST_RUNSTART(t));
    enum { INITSIZ = 512, MAXSIZ = 1024 * 1024 * 50 };
    char*  bufs[3] = {[1] = malloc(INITSIZ), [2] = malloc(INITSIZ)};
    size_t lens[3] = {0};
    size_t caps[3] = {[1] = INITSIZ, [2] = INITSIZ};
    if (!bufs[1] || !bufs[2]) { perror("malloc buf"), exit(1); }

    RK_TestHdr hdr = {0};
    RK_TestRes res = {0};
    enum { RK_PREFLEN = lenof(RK_TFMT_P) };
    FILE* fm = w_files[0];
    int   vb = RK_test_glob.attrs.meta.verbosity;

    char  buf[16384], *s = buf + RK_PREFLEN;
    bool  reading_res = 0;
    for (int nfds = 3; nfds > 0;) {
        DWORD avails[3] = {0};
        for (int i = 0; i < 3; ++i) {
            if (!r_fds[i]) { continue; }
            if (!PeekNamedPipe(r_fds[i], NULL, 0, NULL, &avails[i], NULL)) {
                if (GetLastError() != ERROR_BROKEN_PIPE) {
                    perror("PeekNamedPipe"), exit(1);
                }
                avails[i] = 0, r_fds[i] = NULL, --nfds;
            }
        }
        if (avails[0]) {
            if (!reading_res) {
                switch (rk_read_full_nb(r_fds[0], &hdr, sizeof(hdr))) {
                case RK_IO_RESULT_AGAIN: break;
                case RK_IO_RESULT_ERR  : perror("read meta hdr"), exit(1);
                case RK_IO_RESULT_EOF  : r_fds[0] = NULL, --nfds; break;
                case RK_IO_RESULT_OK:
                    s += sprintf(s, RK_TFMT_STUB(hdr, t));
                    ++t->total, reading_res = 1;
                    break;
                }
            } else {
                switch (rk_read_full_nb(r_fds[0], &res, sizeof(res))) {
                case RK_IO_RESULT_AGAIN: break;
                case RK_IO_RESULT_ERR  : perror("read meta res"), exit(1);
                case RK_IO_RESULT_EOF  : r_fds[0] = NULL, --nfds; break;
                case RK_IO_RESULT_OK:
                    reading_res = 0;
                    if (!res.res) {
                        RK_catlit(buf, RK_TFMT_P);
                        if (vb == RK_TEST_LOG_ALWAYS) {
                            *s++ = '\n', fwrite(buf, 1, s - buf, fm);
                        }
                    } else {
                        ++t->fails;
                        RK_catlit(buf, RK_TFMT_F);
                        s += RK_test_log_failmsg(hdr, res, s);
                        fwrite(buf, s - buf, 1, fm);
                        int nstrs = hdr.F == RK_TEST_STREQ  ? 2
                                  : hdr.F == RK_TEST_STRNEQ ? 1
                                                            : 0;
                        RK_test_log_streqstrs(fm, r_fds[0], res, buf,
                                              sizeof(buf), nstrs);
                    }
                    s = buf + lenof(RK_TFMT_P);
                }
            }
        }
        for (int i = 1; i <= 2; ++i) {
            size_t av = avails[i];
            if (!av) { continue; }
            if (av > caps[i] - lens[i]) {
                do { caps[i] *= 2; } while (av > caps[i] - lens[i]);
                if (caps[i] > MAXSIZ) { perror("too big"), exit(1); }
                bufs[i] = (char*)realloc(bufs[i], caps[i]);
                if (bufs[i] == NULL) { perror("realloc"), exit(1); }
            }
            switch (rk_read_full_nb(r_fds[i], bufs[i], av)) {
            case RK_IO_RESULT_ERR  : perror("read out/err"), exit(1);
            case RK_IO_RESULT_EOF  : r_fds[i] = NULL, --nfds; break;
            case RK_IO_RESULT_OK   : break;
            case RK_IO_RESULT_AGAIN: break;
            }
        }
    }
    DWORD status;
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &status);
    return RK_test_end_parent_loop(t, hdr, w_files, status, reading_res, bufs,
                                   lens, buf, s);
}

#  else
#   define RK_CTOR(fn) __attribute__((constructor)) static void fn(void)
#   define RK_TEST_SPECIALCHILDENTRY()

static inline RK_IO_RESULT_t rk_read_full(int fd, void* buf, size_t nbytes) {
    for (char* ptr = (char*)buf; nbytes;) {
        ssize_t r = read(fd, ptr, nbytes);
        switch (r) {
        default: ptr += r, nbytes -= r; break;
        case 0 : return RK_IO_RESULT_EOF;
        case -1:
            if (errno != EINTR) { return RK_IO_RESULT_ERR; }
        }
    }
    return RK_IO_RESULT_OK; // read all, no eof
}
static inline RK_IO_RESULT_t rk_read_full_nb(int fd, void* buf, size_t nbytes) {
    for (char* ptr = (char*)buf; nbytes;) {
        ssize_t r = read(fd, ptr, nbytes);
        switch (r) {
        default: ptr += r, nbytes -= r; break;
        case 0 : return RK_IO_RESULT_EOF; // eof
        case -1:
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (ptr == buf) { return RK_IO_RESULT_AGAIN; }
            }
            if (errno != EINTR) { return RK_IO_RESULT_ERR; }
        }
    }
    return RK_IO_RESULT_OK; // read all, no eof
}
static inline RK_IO_RESULT_t rk_write_full(int fd, const void* buf,
                                           size_t len) {
    const char* ptr = (const char*)buf;
    for (ssize_t w; len;) {
        if ((w = write(fd, ptr, len)) == -1) {
            if (errno == EINTR) { continue; }
            if (errno == EPIPE) { return RK_IO_RESULT_EOF; }
            return RK_IO_RESULT_ERR;
        }
        len -= w, ptr += w;
    }
    return RK_IO_RESULT_OK;
}
enum UNIPIPE_DIR {
    UNIPIPE_DIR_CTOP,
    UNIPIPE_DIR_PTOC,
};
/// create npipes unidirectional pipes and fork
static inline pid_t RK_fork_unipipe_n(int* fdptr, enum UNIPIPE_DIR dir,
                                      unsigned char npipes) {
    int   pps[256 * 2], opened;
    pid_t pid;
    for (opened = 0; opened < npipes; ++opened) {
        if (pipe(pps + opened * 2) < 0) {
            pid = -1;
            goto err;
        };
    }
    if ((pid = fork()) < 0) {
        pid = -2;
        goto err;
    }
    for (int i = 0, end = (pid > 0) ^ (dir == UNIPIPE_DIR_CTOP); i < npipes;
         ++i) {
        close(pps[i * 2 + !end]), fdptr[i] = pps[i * 2 + end];
    }
    return pid;
err:
    for (int i = 0; i < opened; ++i) {
        close(pps[i * 2]), close(pps[i * 2 + 1]);
    }
    return pid;
}

static inline int RK_test_run_test(RK_test_entry* t) {
    int   res;
    rk_fd pipes[3];
    FILE* files[3];
    pid_t pid = RK_fork_unipipe_n(pipes, UNIPIPE_DIR_CTOP, 3);
    switch (pid) {
    case -2: perror("fork"), exit(1);
    case -1: perror("pipe"), exit(1);
    case 0:
        if (dup2(pipes[1], STDOUT_FILENO) < 0
            || dup2(pipes[2], STDERR_FILENO) < 0) {
            perror("dup2"), exit(1);
        }
        t->func(pipes[0]), _exit(0);
    default:
        for (int i = 0; i < 3; ++i) {
            int fd = pipes[i], flags = fcntl(fd, F_GETFL, 0);
            if (flags == -1 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
                perror("fcntl"), exit(1);
            }
        }
        for (int i = 0; i < 3; ++i) {
            if (!t->attrs._[i].path) {
                files[i] = RK_test_glob.files[i];
            } else {
                if (!(files[i] = fopen(t->attrs._[i].path, "w"))) {
                    perror("fopen"), exit(1);
                }
            }
        }
        res = RK_test_parent_loop(t, pid, pipes, files);
        for (int i = 0; i < 3; ++i) {
            if (t->attrs._[i].path) { fclose(files[i]); }
        }
        close(pipes[0]), close(pipes[1]), close(pipes[2]);
        return res;
    }
}
// uberm
static inline int RK_test_parent_loop(RK_test_entry* t, rk_pid pid,
                                      rk_fd* r_fds, FILE** w_files) {
    printf(RK_TFMT_TEST_RUNSTART(t));
    struct pollfd fds[3]
        = {{r_fds[0], POLLIN}, {r_fds[1], POLLIN}, {r_fds[2], POLLIN}};
    enum { INITSIZ = 512, MAXSIZ = 1024 * 1024 * 50 };
    char*  bufs[3] = {0, (char*)malloc(INITSIZ), (char*)malloc(INITSIZ)};
    size_t lens[3] = {0};
    size_t caps[3] = {0, INITSIZ, INITSIZ};

    if (!bufs[1] || !bufs[2]) { perror("malloc buf"), exit(1); }

    RK_TestHdr hdr = {};
    RK_TestRes res = {0};
    enum { RK_PREFLEN = lenof(RK_TFMT_P) };
    FILE* fm = w_files[0];
    int   vb = RK_test_glob.attrs.meta.verbosity;

    char  buf[16384], *s = buf + RK_PREFLEN;
    bool  reading_res = 0;
    for (int nfds = 3; nfds;) {
        if (!poll(fds, 3, -1)) { continue; }
        if (fds[0].revents & POLLIN) {
            if (reading_res == 0) {
                switch (rk_read_full_nb(fds[0].fd, &hdr, sizeof(hdr))) {
                case RK_IO_RESULT_AGAIN: break;
                case RK_IO_RESULT_ERR  : perror("read meta hdr"), exit(1);
                case RK_IO_RESULT_EOF  : fds[0].fd = -1, --nfds; break;
                case RK_IO_RESULT_OK:
                    s += sprintf(s, RK_TFMT_STUB(hdr, t));
                    ++t->total, reading_res = 1;
                }
            } else {
                switch (rk_read_full_nb(fds[0].fd, &res, sizeof(res))) {
                case RK_IO_RESULT_AGAIN: break;
                case RK_IO_RESULT_ERR  : perror("read meta res"), exit(1);
                case RK_IO_RESULT_EOF  : fds[0].fd = -1, --nfds; break;
                case RK_IO_RESULT_OK:
                    reading_res = 0;
                    if (!res.res) {
                        RK_catlit(buf, RK_TFMT_P);
                        if (vb == RK_TEST_LOG_ALWAYS) {
                            *s++ = '\n', fwrite(buf, 1, s - buf, fm);
                        }
                    } else {
                        ++t->fails;
                        RK_catlit(buf, RK_TFMT_F);
                        s += RK_test_log_failmsg(hdr, res, s);
                        fwrite(buf, s - buf, 1, fm);
                    }
                    s = buf + lenof(RK_TFMT_P);
                }
            }
        }
        for (int i = 1; i < 3; ++i) {
            if (!(fds[i].revents & POLLIN)) { continue; }
            for (;;) {
                ssize_t r
                    = read(fds[i].fd, bufs[i] + lens[i], caps[i] - lens[i]);
                if (r == -1) {
                    if (errno == EINTR) { continue; }
                    if (errno == EAGAIN || errno == EWOULDBLOCK) { break; }
                    perror("read out/err res"), exit(1);
                } else if (r == 0) {
                    fds[i].fd = -1, --nfds;
                    break;
                } else if ((lens[i] = lens[i] + r) == caps[i]) {
                    if (caps[i] >= MAXSIZ) { perror("too much"), exit(1); }
                    caps[i] *= 2, bufs[i] = (char*)realloc(bufs[i], caps[i]);
                    if (!bufs[i]) { perror("realloc membuf"), exit(1); }
                }
            }
        }
    }
    int status;
    while (waitpid(pid, &status, 0) == -1) {
        if (errno != EINTR) {
            perror("waitpid");
            break;
        }
    };
    return RK_test_end_parent_loop(t, hdr, w_files, status, reading_res, bufs,
                                   lens, buf, s);
}

// todo maybe remove
// static inline void RK_die_parent(pid_t pid, const char* str) {
//    perror(str);
//    if (pid > 0) {
//        kill(pid, SIGKILL);
//        waitpid(pid, NULL, 0);
//    }
//    exit(1);
//}

#  endif

#  undef RK_catlit
#  undef RK_test_GENFUN_cmpbasic
#  undef RK_test_GENFUN_inrange
#  undef RK_test_GENFUNS_floatable
#  undef RK_test_GENFUNS_floatonly
#  undef RK_test_GENFUNS_nofloat
#  undef RK_test_GENFUNS_float
#  undef RK_INRED
#  undef RK_INGREEN
#  undef RK_INBLUE
#  undef RK_INYELLOW
#  undef RK_TEST_GEN_TYPE_STRUCT_EL
#  undef RK_TEST_GEN_T_SWITCHCASE
#  undef RK_TEST_GEN_TYPE_ENUM_EL
#  pragma GCC diagnostic pop

# endif

#endif
