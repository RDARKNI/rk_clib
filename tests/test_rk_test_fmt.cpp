
#ifndef TEST_TEST_H
#define TEST_TEST_H
#include "conf.h"
#include <iostream>
// ── Custom type ────────────────────────────────────────────────────────────
// Exercises the TRIAXI_TestStream path (operator<< fallback)

struct Point {
  int  x, y;
  bool operator==(const Point& o) const { return x == o.x && y == o.y; }
  bool operator!=(const Point& o) const { return !(*this == o); }
};
static std::ostream& operator<<(std::ostream& os, const Point& p) {
  return os << "(" << p.x << ", " << p.y << ")";
}

struct Opaque {
  int  v;
  bool operator==(const Opaque& o) const { return v == o.v; }
  bool operator!=(const Opaque& o) const { return v != o.v; }
}; // no operator<< — address fallback

triax_test(cpp_fmt, custom_type_pass) {
  Point a{1, 2}, b{1, 2};
  triax_assert_eq(a, b);
}
triax_test(cpp_fmt, custom_type_fail) {
  // expect output: eq((1, 2), (3, 4))
  Point a{1, 2}, b{3, 4};
  triax_expect_eq(a, b);
}
triax_test(cpp_fmt, custom_type_no_stream_fail) {
  // expect output: eq(&<addr>, &<addr>) — address fallback
  Opaque a{1}, b{2};
  triax_expect_eq(a, b);
}

// ── Integers (template path for non-long-long types) ──────────────────────

triax_test(cpp_fmt, int_pass) {
  int a = 42, b = 42;
  triax_assert_eq(a, b);
}

triax_test(cpp_fmt, int_warn_pass) { triax_assert_eq((unsigned)42, -1); }
triax_test(cpp_fmt, int_fail) {
  // expect output: eq(1, 2) — decimal, no truncation
  int a = 1, b = 2;
  triax_expect_eq(a, b);
}
triax_test(cpp_fmt, short_fail) {
  // expect output: eq(100, 200)
  short a = 100, b = 200;
  triax_expect_eq(a, b);
}
triax_test(cpp_fmt, mixed_int_fail) {
  // T != U — template with two type params
  // expect output: eq(1, 2)
  int   a = 1;
  short b = 2;
  triax_expect_eq(a, b);
}
triax_test(cpp_fmt, int_ordering_pass) {
  triax_assert_lt(1, 2);
  triax_assert_leq(2, 2);
  triax_assert_gt(3, 2);
  triax_assert_geq(2, 2);
  triax_assert_inrange(5, 1, 10);
}

// ── Float (must go through epsilon path, not template) ────────────────────

triax_test(cpp_fmt, float_epsilon_pass) { triax_assert_eq(0.1f + 0.2f, 0.3f); }
triax_test(cpp_fmt, double_epsilon_pass) { triax_assert_eq(0.1 + 0.2, 0.3); }
triax_test(cpp_fmt, float_double_mixed_pass) {
  // mixed: float + double — both should widen to long double, epsilon holds
  triax_assert_eq(0.1f + 0.2f, (double)(0.1f + 0.2f));
}
triax_test(cpp_fmt, float_fail) {
  // expect output showing both values at long double precision
  triax_expect_eq(1.0f, 2.0f);
}
triax_test(cpp_fmt, double_fail) { triax_expect_eq(1.0, 2.0); }
triax_test(cpp_fmt, float_double_mixed_fail) {
  // mixed types: float vs double, should fail with both values shown
  triax_expect_eq(1.0f, 2.0);
}
triax_test(cpp_fmt, float_tol_pass) { triax_assert_floateq_tol(1.0f, 1.0f + 1e-5f, 1e-4f); }
triax_test(cpp_fmt, float_tol_fail) {
  // expect output showing all three: val, expected, tolerance
  triax_expect_floateq_tol(1.0f, 2.0f, 1e-4f);
}
triax_test(cpp_fmt, float_ordering_pass) {
  triax_assert_lt(1.0f, 2.0f);
  triax_assert_leq(2.0f, 2.0f);
  triax_assert_gt(3.0, 2.0);
  triax_assert_inrange(1.5f, 1.0f, 2.0f);
}

// ── Float formatting boundaries and near-tolerance cases ──────────────────

triax_test(cpp_fmt, float_close_invisible_fail) {
  // differ by ~1 ULP — with %g both may print as "1" → "1 != 1" in output
  triax_expect_eq(1.0000001f, 1.0000002f);
}
triax_test(cpp_fmt, float_close_visible_fail) {
  // differ at 6th significant digit — visible with %.9g
  triax_expect_eq(1.000001f, 1.000002f);
}
triax_test(cpp_fmt, float_scientific_notation_fail) {
  // small magnitude — %g should switch to scientific notation
  triax_expect_eq(1.5e-8f, 2.5e-8f);
}
triax_test(cpp_fmt, float_tol_at_boundary_pass) {
  // |diff| == tol: !(|diff| <= tol) is false → pass
  triax_assert_floateq_tol(0.0f, 1e-4f, 1e-4f);
}
triax_test(cpp_fmt, float_tol_just_inside_pass) {
  triax_assert_floateq_tol(1.0f, 1.0f + 9e-5f, 1e-4f);
}
triax_test(cpp_fmt, float_tol_just_outside_fail) {
  // output shows all three args: value, expected, tolerance
  triax_expect_floateq_tol(1.0f, 1.0f + 1.1e-4f, 1e-4f);
}
triax_test(cpp_fmt, double_close_fail) {
  // close doubles — differ at the 13th significant digit
  triax_expect_eq(1.0000000000001, 1.0000000000002);
}

// ── Strings (explicit overloads) ──────────────────────────────────────────

triax_test(cpp_fmt, str_eq_pass) { triax_assert_eq("hello", "hello"); }
triax_test(cpp_fmt, str_lt_pass) {
  triax_assert_lt("apple", "banana");
  triax_assert_lt("ab", "abc"); // prefix: shorter < longer
  triax_assert_lt("", "a");
  triax_assert_lt((const char*)nullptr, "");
}
triax_test(cpp_fmt, str_ordering_pass) {
  triax_assert_leq("apple", "apple");
  triax_assert_gt("banana", "apple");
  triax_assert_geq("banana", "banana");
  triax_assert_inrange("cat", "apple", "dog");
}
triax_test(cpp_fmt, str_fail) {
  // expect output: eq(hello, world)
  triax_expect_eq("hello", "world");
}
triax_test(cpp_fmt, str_lt_fail) {
  // expect output showing both strings
  triax_expect_lt("banana", "apple");
}

#endif
