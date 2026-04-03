#ifndef RK__TESTDUMMY
# include "../rk_test/rk_test.h"
#else
# include "../rk_test/rk_test_dummy.h"
#endif
#define RK_IMPL
#include "../include/rklib_includeall.h"

RK_HEADER_BEGIN
RK__IGNWARN_CLANG_BEG("-Wunused-variable")

// #define CAP 239539
// #define CAP 999
#define CAP 70

RK_REGISTER_TEST("bitset", test_bitset_singlebit) {
  bitset(CAP) b     = {0};
  size_t fail_index = -1;
  for (size_t i = 0; i < CAP; ++i) {
    bitset_set(b, CAP, i);
    if (!bitset_test(b, CAP, i)) {
      fail_index = i;
      break;
    }
  }
  rk_expect_eq(fail_index, -1);

  for (size_t i = 0; i < CAP; ++i) {
    bitset_clear(b, CAP, i);
    rk_expect_false(bitset_test(b, CAP, i));
  }
  for (size_t i = 0; i < CAP; ++i) {
    bitset_flip(b, CAP, i);
    rk_expect_true(bitset_test(b, CAP, i));
  }
}
RK_REGISTER_TEST("bitset", test_bitset_scf_all) {
  bitset(CAP) b = {0};
  bitset_set_all(b, CAP);
  for (size_t i = 0; i < CAP; ++i) { rk_expect_true(bitset_test(b, CAP, i)); }
  bitset_clear_all(b, CAP);
  for (size_t i = 0; i < CAP; ++i) { rk_expect_true(!bitset_test(b, CAP, i)); }
  bitset_flip_all(b, CAP);
  for (size_t i = 0; i < CAP; ++i) { rk_expect_true(bitset_test(b, CAP, i)); }
}
RK_REGISTER_TEST("bitset", test_bitset_count) {
  bitset(CAP) b = {0};
  rk_expect_true(!bitset_any(b, CAP) && bitset_none(b, CAP)
                 && !bitset_all(b, CAP));

  bitset_set_all(b, CAP);
  rk_expect_true(bitset_any(b, CAP) && !bitset_none(b, CAP)
                 && bitset_all(b, CAP));
  rk_expect_true(bitset_count_ones(b, CAP) == CAP);
  rk_expect_true(bitset_count_zeros(b, CAP) == 0);

  bitset_clear_all(b, CAP);
  for (size_t i = 0; i < CAP; ++i) { bitset_set(b, CAP, i); }
  rk_expect_true(bitset_any(b, CAP) && !bitset_none(b, CAP)
                 && bitset_all(b, CAP));
  rk_expect_true(bitset_count_ones(b, CAP) == CAP);
  rk_expect_true(bitset_count_zeros(b, CAP) == 0);

  bitset_set_all(b, CAP);
  for (size_t i = 0; i < CAP; ++i) { bitset_clear(b, CAP, i); }
  rk_expect_true(!bitset_any(b, CAP) && bitset_none(b, CAP)
                 && !bitset_all(b, CAP));
  rk_expect_true(bitset_count_ones(b, CAP) == 0);
  rk_expect_true(bitset_count_zeros(b, CAP) == CAP);

  bitset_set_all(b, CAP);
  for (size_t i = 0; i < CAP; ++i) { bitset_clear(b, CAP, i); }
  rk_expect_false(bitset_has_single_bit(b, CAP));
  for (size_t i = 1; i < CAP; ++i) {
    bitset_clear(b, CAP, i - 1);
    rk_expect_false(bitset_has_single_bit(b, CAP));
    bitset_set(b, CAP, i);
    rk_expect_true(bitset_has_single_bit(b, CAP));
  }
}
RK_REGISTER_TEST("bitset", test_bitset_range) {
  for (size_t s = 0; s < CAP; ++s) {
    for (size_t e = s; e < CAP; ++e) {
      size_t i      = 0;
      bitset(CAP) b = {0};
      bitset_set_range(b, CAP, s, e);
      for (i = 0; i < s; ++i) { rk_expect_true(!bitset_test(b, CAP, i)); }
      for (i = s; i < e; ++i) { rk_expect_true(bitset_test(b, CAP, i)); }
      for (i = e; i < CAP; ++i) { rk_expect_true(!bitset_test(b, CAP, i)); }
    }
  }

  for (size_t s = 0; s < CAP; ++s) {
    for (size_t e = s; e < CAP; ++e) {
      size_t i      = 0;
      bitset(CAP) b = {0};
      bitset_set_all(b, CAP);
      bitset_clear_range(b, CAP, s, e);
      for (i = 0; i < s; ++i) { rk_expect_true(bitset_test(b, CAP, i)); }
      for (i = s; i < e; ++i) { rk_expect_true(!bitset_test(b, CAP, i)); }
      for (i = e; i < CAP; ++i) { rk_expect_true(bitset_test(b, CAP, i)); }
    }
  }
  for (size_t s = 0; s < CAP; ++s) {
    for (size_t e = s; e < CAP; ++e) {
      size_t i      = 0;
      bitset(CAP) b = {0};
      bitset_flip_range(b, CAP, s, e);
      for (i = 0; i < s; ++i) { rk_expect_true(!bitset_test(b, CAP, i)); }
      for (i = s; i < e; ++i) { rk_expect_true(bitset_test(b, CAP, i)); }
      for (i = e; i < CAP; ++i) { rk_expect_true(!bitset_test(b, CAP, i)); }
      bitset_set_all(b, CAP);
      bitset_flip_range(b, CAP, s, e);
      for (i = 0; i < s; ++i) { rk_expect_true(bitset_test(b, CAP, i)); }
      for (i = s; i < e; ++i) { rk_expect_true(!bitset_test(b, CAP, i)); }
      for (i = e; i < CAP; ++i) { rk_expect_true(bitset_test(b, CAP, i)); }
    }
  }
}
RK_REGISTER_TEST("bitset", test_bitset_leading_trailing) {
  {
    bitset(CAP) b;
    bitset_set_all(b, CAP);
    rk_expect_true(bitset_first_leading_one(b, CAP) == 1);
    rk_expect_true(bitset_first_trailing_one(b, CAP) == 1);
    for (size_t i = 0; i < CAP; ++i) { bitset_clear(b, CAP, i); }
    rk_expect_true(bitset_first_leading_one(b, CAP) == 0);
    rk_expect_true(bitset_first_trailing_one(b, CAP) == 0);
    bitset_set(b, CAP, 0);
    rk_expect_true(bitset_first_leading_one(b, CAP) == CAP);
    rk_expect_true(bitset_first_trailing_one(b, CAP) == 1);
    for (size_t i = 1; i < CAP; ++i) {
      bitset_clear(b, CAP, i - 1);
      bitset_set(b, CAP, i);
      size_t bsflo = bitset_first_leading_one(b, CAP);
      rk_expect_true(bsflo == CAP - i);
      size_t bsfto = bitset_first_trailing_one(b, CAP);
      rk_expect_true(bsfto == i + 1);
    }
  }
  {
    bitset(CAP) b;
    bitset_clear_all(b, CAP);
    rk_expect_true(bitset_first_leading_zero(b, CAP) == 1);
    rk_expect_true(bitset_first_trailing_zero(b, CAP) == 1);
    for (size_t i = 0; i < CAP; ++i) { bitset_set(b, CAP, i); }
    rk_expect_true(bitset_first_leading_zero(b, CAP) == 0);
    rk_expect_true(bitset_first_trailing_zero(b, CAP) == 0);
    bitset_clear(b, CAP, 0);
    rk_expect_true(bitset_first_leading_zero(b, CAP) == CAP);
    rk_expect_true(bitset_first_trailing_zero(b, CAP) == 1);
    for (size_t i = 1; i < CAP; ++i) {
      bitset_set(b, CAP, i - 1);
      bitset_clear(b, CAP, i);
      size_t bsflo = bitset_first_leading_zero(b, CAP);
      size_t bsfto = bitset_first_trailing_zero(b, CAP);
      rk_expect_true(bsflo == CAP - i);
      rk_expect_true(bsfto == i + 1);
    }
  }
}

RK_REGISTER_TEST("bitset", test_bitset_bitwise) {

  bitset(CAP) a   = {0};
  bitset(CAP) b   = {0};
  bitset(CAP) dst = {0};

  // Initialize test patterns
  for (size_t i = 0; i < CAP; ++i) {
    if (i % 2) {
      bitset_set(a, CAP, i); // a = 010101010...
    }
    if (i % 3 == 0) {
      bitset_set(b, CAP, i); // b = 100100100...
    }
  }

  // Test OR
  memcpy(dst, a, sizeof(dst));
  bitset_or(dst, CAP, b);
  for (size_t i = 0; i < CAP; ++i) {
    bool expected = bitset_test(a, CAP, i) || bitset_test(b, CAP, i);
    rk_expect_eq(bitset_test(dst, CAP, i), expected);
  }

  // Test AND
  memcpy(dst, a, sizeof(dst));
  bitset_and(dst, CAP, b);
  for (size_t i = 0; i < CAP; ++i) {
    bool expected = bitset_test(a, CAP, i) && bitset_test(b, CAP, i);
    rk_expect_eq(bitset_test(dst, CAP, i), expected);
  }

  // Test XOR
  memcpy(dst, a, sizeof(dst));
  bitset_xor(dst, CAP, b);
  for (size_t i = 0; i < CAP; ++i) {
    bool expected = bitset_test(a, CAP, i) != bitset_test(b, CAP, i);
    rk_expect_eq(bitset_test(dst, CAP, i), expected);
  }

  // Test SUB (a &= ~b)
  memcpy(dst, a, sizeof(dst));
  bitset_sub(dst, CAP, b);
  for (size_t i = 0; i < CAP; ++i) {
    bool expected = bitset_test(a, CAP, i) && !bitset_test(b, CAP, i);
    rk_expect_eq(bitset_test(dst, CAP, i), expected);
  }
}

RK_REGISTER_TEST("bitset", test_bitset_iterations) {
  bitset(CAP) bs = {0};
  for (size_t i = 0; i < CAP; ++i) {
    bitset_set(bs, CAP, i);
    size_t fo = bitset_first_set(bs, CAP);
    rk_expect_eq(fo, i);
    rk_expect_eq(fo, bitset_next_set(bs, CAP, -1));
    bitset_clear(bs, CAP, i);
  }
  bitset_set_all(bs, CAP);
  for (size_t i = 0; i < CAP; ++i) {
    bitset_clear(bs, CAP, i);
    size_t fo = bitset_first_clear(bs, CAP);
    rk_expect_eq(fo, i);
    rk_expect_eq(fo, bitset_next_clear(bs, CAP, -1));
    bitset_set(bs, CAP, i);
  }
  for (size_t i = -1, j = 0; (i = bitset_next_set(bs, CAP, i)) != (size_t)-1;
       ++j) {
    rk_expect_eq(j, i);
  }
  bitset_clear_all(bs, CAP);
  for (size_t i = -1, j = 0; (i = bitset_next_clear(bs, CAP, i)) != (size_t)-1;
       ++j) {
    rk_expect_eq(j, i);
  }
  // todo more
}

// void bitset_test_shift() {
//     bitset(CAP) a = {0};
//     bitset_set(a, CAP, 0);
//     assert_true(a[0]);
//
//     bitset_shift_right(a, CAP, 1);
//     for (size_t i = 0; i < CAP; ++i) { assert_true(!bitset_test(a, CAP, i));
//     }
// }

RK__IGNWARN_CLANG_END()
RK_HEADER_END
