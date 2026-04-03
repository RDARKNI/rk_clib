rk_test.h — Single-Header Test Framework for C
===============================================

Overview
--------

`rk_test.h` is a single-header, Criterion-like, header-only C test framework that runs each test in its own child process. It captures stdout/stderr and test metadata via pipes, supports a variety of assertion and expectation macros, allows per-test or global output redirection, and enables simple test selection via suite/tags.

Design highlights
-----------------

- Tests are registered with `RK_REGISTER_TEST(fn, ...)` which uses a constructor function to add the test to a global registry.
- Each test runs in a separate process (fork on POSIX, CreateProcess on Windows) to isolate crashes and exits.
- The child communicates assertion/expectation headers and results to the parent using a small header (`RK_TestHdr`) and result struct (`RK_TestRes`).
- Type-generic comparison functions are selected using C11 `_Generic` over a pre-defined type list.
- Output verbosity and destinations can be set globally or per-test using `RK_test_CustomAttrs` and `RK_TestStreamOpts`.

Public API
----------

- Registration / Runner
  - `RK_REGISTER_TEST(fn, ...)` — define and register a test. Optional attributes (passed as a struct initializer) include `.tags`, `.suite`, and per-test stream options `meta`, `out`, `err` (each an `RK_TestStreamOpts`).
  - `RK_RUN_TESTS(...)` — run all registered tests. Accepts the same `RK_test_CustomAttrs` fields to set global behaviour.

- Expectations (non-fatal; child continues executing)
  - `rk_expect_true(expr)`, `rk_expect_false(expr)`
  - `rk_expect_null(ptr)`, `rk_expect_nonnull(ptr)`
  - `rk_expect_memeq(ptr1, ptr2, siz)`, `rk_expect_memneq(...)`
  - `rk_expect_memzero(ptr, siz)`, `rk_expect_memnzero(ptr, siz)`
  - Generic comparisons: `rk_expect_eq(exp, act)`, `rk_expect_neq`, `rk_expect_lt`, `rk_expect_leq`, `rk_expect_gt`, `rk_expect_geq`, `rk_expect_inrange(val, low, high)`
  - Float tolerant comparisons: `rk_expect_floateq_tol(exp, act, tol)`, `rk_expect_floatneq_tol(...)`
  - String comparisons: `rk_expect_streq(str1, str2)`, `rk_expect_strneq(...)`
  - Note: stdout/stderr equality macros exist but are marked "not implemented" in the header.

- Assertions (fatal; child exits on failure)
  - `rk_assert_true`, `rk_assert_false`, `rk_assert_null`, `rk_assert_nonnull`, `rk_assert_memeq`, etc.
  - Generic comparison assertions: `rk_assert_eq`, `rk_assert_neq`, `rk_assert_lt`, ...
  - Crash/exit assertions: `rk_assert_crash(signal, ...)` and `rk_assert_exit(code, ...)`.

- Configuration data types
  - `RK_TestStreamOpts` — { verbosity, path, FILE* file }
  - `RK_test_CustomAttrs` — { suite, tags, meta, out, err }
  - `RK_test_verbosity` — `RK_TEST_LOG_DEFAULT`, `RK_TEST_LOG_ALWAYS`, `RK_TEST_LOG_NEVER`

Files / symbols of interest
---------------------------
- `RK_TestHdr` — header sent from child to parent describing the assertion (function, type, line, expr string)
- `RK_TestRes` — result payload containing a numeric result and up to 3 typed arguments (value union)
- `RK_test_entry` — registry entry that stores the test function pointer and attributes
- `RK_test_run_all_tests` — API used by `RK_RUN_TESTS` to run tests

Minimal usage examples
----------------------

Example 1 — simple tests (POSIX / macOS / Linux)

Create `tests/sample_test.c`:

```c
#include "rk_test.h"

RK_REGISTER_TEST(test_addition, .tags = "math,quick", .suite = "arith") {
    rk_expect_eq(2 + 2, 4);
    rk_expect_true(1 < 2);
    rk_assert_eq(1 + 2, 3);
}

RK_REGISTER_TEST(test_strings) {
    rk_assert_streq("hello", "hello");
    rk_expect_strneq("one", "two");
}

int main(void) {
    RK_RUN_TESTS();
    return 0;
}
```

Compile and run:

```bash
cc -std=c11 -Wall -Wextra -o sample_test tests/sample_test.c
./sample_test
```

Example 2 — assert crash

```c
RK_REGISTER_TEST(test_crash) {
    rk_assert_crash(SIGSEGV, {
        int *p = NULL;
        *p = 1; // should cause SIGSEGV
    });
}
```

Platform notes
--------------

- The header is C11 and uses `_Generic`. Use a modern compiler (clang, gcc, MSVC with appropriate flags).
- On POSIX, the implementation forks and uses pipes. On Windows it uses `CreateProcess` and environment variables to route which test the child should execute.
- Stdout/stderr comparison features are present but not implemented; attempting to use those macros will either be a no-op or exit with a message in the current implementation.

Limitations and TODOs
---------------------
- Stdout/stderr equality assertions are marked "todo not implemented".
- Some code paths and logging variables are complex and may require cleanup when refactoring.
- The header assumes serial execution of tests and is not designed for concurrent test execution.
- The API depends on a constructor attribute (`__attribute__((constructor))`) on POSIX to auto-register tests; behavior may vary across toolchains.

Suggested small improvements
---------------------------
- Add a sentinel `RK_TEST_COUNT` to `enum RK_test_FUN` for bounds checking and iterating.
- Ensure consistent naming for `RK_test_send_hdr` vs `RK_test_sendhdr` (there's a macro and an inline function with similar names).
- Make unimplemented stdout/stderr comparison macros either provide a compile-time diagnostic or a clear runtime message.
- Provide a small example in the repository (this README plus `tests/sample_test.c`) and a `Makefile` or short compile commands.

Next steps
----------
If you'd like I can:
- Add `tests/sample_test.c` to the repo and run a quick compile on macOS to verify it works.
- Add the README into `tests/rk_test/` (done here) and open a PR branch with small improvements (sentinel enum, minor naming cleanups).

