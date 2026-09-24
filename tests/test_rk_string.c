#ifndef TEST_STRING_H
#define TEST_STRING_H
#include "conf.h"

RK_HEADER_BEGIN
RK__IGNWARN_CLANG_BEG("-Wunused-variable")

triax_test(string, str_len) {
  triax_expect_eq(str_len("hello"), lenof("hello"));
  char arr[] = "world";
  triax_expect_eq(str_len(arr), 5u);
  const char arr2[] = "world";
  triax_expect_eq(str_len(arr2), 5u);
  const char* ptr = "ok";
  triax_expect_eq(str_len(ptr), 2u);
  const char* ptr2 = "ok";
  triax_expect_eq(str_len(ptr2), 2u);
}

// triax_test(string, str_tok_single) {
//     Strv sv = strv_from_literal(",oh dear");
//     str_tok_by_single(&sv, ',');
//     triax_expect_eq(str_len("hello"), lenof("hello"));
//     char arr[] = "world";
//     triax_expect_eq(str_len(arr), 5);
//     const char arr2[] = "world";
//     triax_expect_eq(str_len(arr2), 5);
//     char* ptr = "ok";
//     triax_expect_eq(str_len(ptr), 2);
//     const char* ptr2 = "ok";
//     triax_expect_eq(str_len(ptr2), 2);
// }

triax_test(string, str_starts_with) {
  str_starts_with("hello", 'a');
  triax_expect_eq(str_len("hello"), lenof("hello"));
  char arr[] = "world";
  triax_expect_eq(str_len(arr), 5u);
  const char arr2[] = "world";
  triax_expect_eq(str_len(arr2), 5u);
  char* ptr = (char*)"ok";
  triax_expect_eq(str_len(ptr), 2u);
  const char* ptr2 = "ok";
  triax_expect_eq(str_len(ptr2), 2u);
}

triax_test(string, str_init_and_from_literal) {
  Str s1 = str_init(16);
  triax_expect_eq(s1.len, 0u);
  triax_expect_eq(s1.cap, 16u);
  triax_expect_nonnull(s1.str);
  triax_expect_true(str_is_null_terminated(&s1));
  Str s2 = str_from_literal("hello");
  triax_expect_true(str_is_null_terminated(&s2));
  triax_expect_streq(s2.str, "hello");

  Str s3 = str_from_literal("");
  triax_expect_true(str_is_null_terminated(&s3));
  triax_expect_streq(s3.str, "");

  Str s4 = {RK_ZINIT};
  triax_expect_nonnull(str_null_terminate(&s4));

  triax_expect_true(str_is_null_terminated(str_null_terminate(&s4)));
  str_release(&s2), str_release(&s3);
}

triax_test(string, str_clone_and_assign) {
  Str s  = str_from_literal("abc");
  Str s2 = str_from(s);
  triax_expect_streq(s2.str, "abc");

  str_assign(&s2, "xyz");
  triax_expect_streq(s2.str, "xyz");
  str_assign(&s2, "ADSFJADFKSLDIEJFND,SLNERFFJ");
  triax_expect_streq(s2.str, "ADSFJADFKSLDIEJFND,SLNERFFJ");
  str_assign(&s2, "Y");
  triax_expect_streq(s2.str, "Y");

  str_assign(&s2, "a");
  triax_expect_streq(s2.str, "a");

  str_assign(&s2, s);
  triax_expect_streq(s2.str, "abc");
  str_release(&s2);
}

triax_test(string, str_cat_and_push) {
  Str s = str_from_literal("hi");
  str_push(&s, '!');
  triax_expect_streq(s.str, "hi!");

  str_cat(&s, " there");
  triax_expect_streq(s.str, "hi! there");
  str_release(&s);
}

triax_test(string, str_cat_fmt) {
  Str s = str_init(8);
  str_cat_fmt(&s, "number: %d", 42);
  triax_expect_streq(s.str, "number: 42");
  str_cat_fmt(&s, "number: %d", 42);
  triax_expect_streq(s.str, "number: 42number: 42");
  str_release(&s);
}

triax_test(string, str_insert_and_erase) {

  Str s = str_from_literal("helo");
  str_insert_at_char(&s, 3, 'l'); // insert 'l' at index 3
  printf("%s\n", s.str);
  triax_expect_streq(s.str, "hello");
  printf("%s\n", s.str);

  str_erase_at(&s, 1); // remove 'e'
  printf("%s\n", s.str);

  triax_expect_streq(s.str, "hllo"); // todo error
  printf("%s\n", s.str);

  str_erase_at_n(&s, 1, 2); // remove 'll'
  printf("%s\n", s.str);

  triax_expect_streq(s.str, "ho");
  printf("%s\n", s.str);

  str_release(&s);
}

triax_test(string, str_replace_and_case) {
  Str s = str_from_literal("abcabc");
  str_replace(&s, 'a', 'x');
  triax_expect_streq(s.str, "xbcxbc");

  str_to_upper(&s);
  triax_expect_streq(s.str, "XBCXBC");

  str_to_lower(&s);
  triax_expect_streq(s.str, "xbcxbc");
  str_release(&s);
}

triax_test(string, str_reverse) {
  Str s = str_from_literal("abcdef");
  str_reverse(&s);
  triax_expect_streq(s.str, "fedcba");
  str_release(&s);
}

triax_test(string, str_trim) {
  const char* t   = "   hello world  ";
  Strv        svl = str_trimmed_left(t);
  triax_expect_eq(svl.len, 13u);
  triax_expect_memeq(svl.str, "hello world  ", 13);
  Strv svr = str_trimmed_right(t);
  triax_expect_eq(svr.len, 14u);
  triax_expect_memeq(svr.str, "   hello world", 14);
  Strv svb = str_trimmed(t);
  triax_expect_eq(svb.len, 11u);
  triax_expect_memeq(svb.str, "hello world", 11);
}
triax_test(string, slice) {
  Str s = str_from(strv_slice("abcdefgh", 2, 5));
  triax_expect_streq(s.str, "cde");
  str_release(&s);
}
triax_test(string, str_erase_if) {
  Str s = str_from_literal("abcdefgh");
  str_erase_if(&s, i, *i == 'a' && 1);
  triax_expect_streq(s.str, "bcdefgh");
  str_release(&s);
}

triax_test(string, str_find_and_compare) {
  Str         s = str_from_literal("hello world");
  const char* p = str_find(s, "world");

  triax_expect_true(p != rk_null && strncmp(p, "world", 5) == 0);

  const char* pr = str_findr(s, "l");
  triax_expect_true(pr != rk_null && *pr == 'l');
  triax_expect_true(str_compare("abc", "abd") < 0);
  triax_expect_true(str_compare("abc", "abc") == 0);
  triax_expect_true(str_compare("abd", "abc") > 0);
  triax_expect_true(str_starts_with(s, "hello"));
  triax_expect_true(str_ends_with(s, "world"));
  str_release(&s);
}

triax_test(string, str_split_alloc) {
  Str    s = str_from_literal("one,two,three");
  size_t count;
  Strv*  tokens = str_split_alloc(s, ",", &count);
  triax_expect_eq(count, 3u);

  triax_expect_true(tokens[0].len == 3u && !memcmp(tokens[0].str, "one", 3));
  triax_expect_true(tokens[1].len == 3u && !memcmp(tokens[1].str, "two", 3));
  triax_expect_true(tokens[2].len == 5u && !memcmp(tokens[2].str, "three", 5));
  alloc_delete(tokens, count);
  str_release(&s);
}
triax_test(string, str_find) {
  const char* s;
  char        c  = 'k';
  const char  cc = 'k';
  s              = str_find("hello", "ok");
  triax_expect_null(s);
  s = str_find("hello", "ello");
  triax_expect_nonnull(s);
  s = str_find("hello", 'o');
  triax_expect_nonnull(s);
  s = str_find("hello", c);
  triax_expect_null(s);
  s = str_find("hello", cc);
  triax_expect_null(s);

  Strv sv = strv_from_literal("hello");
  s       = str_find(sv, "ok");
  triax_expect_null(s);
  s = str_find(sv, "ello");
  triax_expect_nonnull(s);
  s = str_find(sv, 'o');
  triax_expect_nonnull(s);
  s = str_find(sv, c);
  triax_expect_null(s);
  s = str_find(sv, cc);
  triax_expect_null(s);
  // end

  const char* lit_cstr = "hello";
  s                    = str_find(lit_cstr, "ok");
  triax_expect_null(s);
  s = str_find(lit_cstr, "ello");
  triax_expect_nonnull(s);
  s = str_find(lit_cstr, 'o');
  triax_expect_nonnull(s);
  s = str_find(lit_cstr, c);
  triax_expect_null(s);
  s = str_find(lit_cstr, cc);
  triax_expect_null(s);

  char* ccstr = (char*)lit_cstr;
  s           = str_find(ccstr, "ok");
  triax_expect_null(s);
  s = str_find(ccstr, "ello");
  triax_expect_nonnull(s);
  s = str_find(ccstr, 'o');
  triax_expect_nonnull(s);
  s = str_find(ccstr, c);
  triax_expect_null(s);
  s = str_find(ccstr, cc);
  triax_expect_null(s);
}
triax_test(string, str_tok_single_empty) {
  {
    Strv        res;
    const char* strs[] = {""};
    Str         s      = str_from_literal("");
    size_t      i      = 0;
    for (Strv cons = s.v; cons.len != (size_t)-1; ++i) {
      res = str_split(&cons, ',');
      triax_assert_true(i < countof(strs));
      triax_expect_memeq(res.str, strs[i], strlen(strs[i]));
      printf("%.*s vs %.*s\n", (int)res.len, res.str, (int)strlen(strs[i]), strs[i]);
    }
    triax_assert_eq(i, countof(strs));
    str_release(&s);
  }
  {
    Strv        res;
    const char* strs[] = {"", "", "ab", ""};
    Str         s      = str_from_literal(",,ab,");
    size_t      i      = 0;
    for (Strv cons = s.v; cons.len != (size_t)-1; ++i) {
      res = str_split(&cons, ',');
      triax_assert_true(i < countof(strs));
      triax_expect_memeq(res.str, strs[i], strlen(strs[i]));
      printf("%.*s vs %.*s\n", (int)res.len, res.str, (int)strlen(strs[i]), strs[i]);
    }
    triax_assert_eq(i, countof(strs));
    str_release(&s);
  }
}
triax_test(string, str_tok_single_normal) {
  Strv        res;
  const char* strs[] = {"one", "two", "three", "four", "why"};
  Str         s      = str_from_literal("one,two,three,four");
  size_t      i      = 0;
  for (Strv cons = s.v; cons.len != (size_t)-1; ++i) {
    res = str_split(&cons, ',');
    triax_assert_true(i < countof(strs));
    triax_expect_memeq(res.str, strs[i], strlen(strs[i]));
    printf("%.*s vs %.*s\n", (int)res.len, res.str, (int)strlen(strs[i]), strs[i]);
  }
  str_release(&s);
}

triax_test(string, str_tok_single_third) {
  Strv        res;
  const char* strs[] = {"", "", "ab", ""};
  Str         s      = str_from_literal(",,ab,");
  size_t      i      = 0;
  for (Strv cons = s.v; cons.len != (size_t)-1; ++i) {
    res = str_split(&cons, ',');
    triax_assert_true(i < countof(strs));
    triax_expect_memeq(res.str, strs[i], strlen(strs[i]));
    printf("%.*s vs %.*s\n", (int)res.len, res.str, (int)strlen(strs[i]), strs[i]);
  }
  triax_assert_eq(i, countof(strs));
  str_release(&s);
}

triax_test(string, str_tok_single_4) {
  {
    Strv        res;
    const char* strs[] = {"", "", "", "", ""};
    Str         s      = str_from_literal(",,,,");
    size_t      i      = 0;
    for (Strv cons = s.v; cons.len != (size_t)-1; ++i) {
      res = str_split(&cons, ',');
      triax_assert_true(i < countof(strs));
      triax_expect_memeq(res.str, strs[i], strlen(strs[i]));
      printf("%.*s vs %.*s\n", (int)res.len, res.str, (int)strlen(strs[i]), strs[i]);
    }
    triax_assert_eq(i, countof(strs));
    str_release(&s);
  }
  {
    Strv        res;
    const char* strs[] = {"hello"};
    Str         s      = str_from_literal("hello");
    size_t      i      = 0;
    for (Strv cons = s.v; cons.len != (size_t)-1; ++i) {
      res = str_split_char(&cons, ',');
      triax_assert_true(i < countof(strs));
      triax_expect_memeq(res.str, strs[i], strlen(strs[i]));
      printf("%.*s vs %.*s\n", (int)res.len, res.str, (int)strlen(strs[i]), strs[i]);
    }
    triax_assert_eq(i, countof(strs));
    str_release(&s);
  }
  {
    Strv        res;
    const char* strs[] = {"", "hello"};
    Str         s      = str_from_literal(",hello");
    size_t      i      = 0;
    for (Strv cons = s.v; cons.len != (size_t)-1; ++i) {
      res = str_split(&cons, ',');
      triax_assert_true(i < countof(strs));
      triax_expect_memeq(res.str, strs[i], strlen(strs[i]));
      printf("%.*s vs %.*s\n", (int)res.len, res.str, (int)strlen(strs[i]), strs[i]);
    }
    triax_assert_eq(i, countof(strs));
    str_release(&s);
  }
}

triax_test(string, str_tok_all) {
  {
    Strv        res;
    const char* strs[] = {""};
    Str         s      = str_from_literal("");
    size_t      i      = 0;
    for (Strv cons = s.v; cons.len != (size_t)-1; ++i) {
      res = str_split(&cons, ",");
      triax_assert_true(i < countof(strs));
      triax_expect_memeq(res.str, strs[i], strlen(strs[i]));
      printf("%.*s vs %.*s\n", (int)res.len, res.str, (int)strlen(strs[i]), strs[i]);
    }
    triax_assert_eq(i, countof(strs));
    str_release(&s);
  }
  {
    Strv        res;
    const char* strs[] = {"", "", "ab", ""};
    Str         s      = str_from_literal(",,ab,");
    size_t      i      = 0;
    for (Strv cons = s.v; cons.len != (size_t)-1; ++i) {
      res = str_split(&cons, ",");
      triax_assert_true(i < countof(strs));
      triax_expect_memeq(res.str, strs[i], strlen(strs[i]));
      printf("%.*s vs %.*s\n", (int)res.len, res.str, (int)strlen(strs[i]), strs[i]);
    }
    triax_assert_eq(i, countof(strs));
    str_release(&s);
  }
  {
    Strv        res;
    const char* strs[] = {"", "", "", "", ""};
    Str         s      = str_from_literal(",,,,");
    size_t      i      = 0;
    for (Strv cons = s.v; cons.len != (size_t)-1; ++i) {
      res = str_split(&cons, ",");
      triax_assert_true(i < countof(strs));
      triax_expect_memeq(res.str, strs[i], strlen(strs[i]));
      printf("%.*s vs %.*s\n", (int)res.len, res.str, (int)strlen(strs[i]), strs[i]);
    }
    triax_assert_eq(i, countof(strs));
    str_release(&s);
  }
  {
    Strv        res;
    const char* strs[] = {",,,,"};
    Str         s      = str_from_literal(",,,,");
    size_t      i      = 0;
    for (Strv cons = s.v; cons.len != (size_t)-1; ++i) {
      res = str_split(&cons, "");
      triax_assert_true(i < countof(strs));
      triax_expect_memeq(res.str, strs[i], strlen(strs[i]));
      printf("%.*s vs %.*s\n", (int)res.len, res.str, (int)strlen(strs[i]), strs[i]);
    }
    triax_assert_eq(i, countof(strs));
    str_release(&s);
  }
  {
    Strv        res;
    const char* strs[] = {"hello"};
    Str         s      = str_from_literal("hello");
    size_t      i      = 0;
    for (Strv cons = s.v; cons.len != (size_t)-1; ++i) {
      res = str_split(&cons, ",");
      triax_assert_true(i < countof(strs));
      triax_expect_memeq(res.str, strs[i], strlen(strs[i]));
      printf("%.*s vs %.*s\n", (int)res.len, res.str, (int)strlen(strs[i]), strs[i]);
    }
    triax_assert_eq(i, countof(strs));
    str_release(&s);
  }
  {
    Strv        res;
    const char* strs[] = {",hello"};
    Str         s      = str_from_literal(",hello");
    size_t      i      = 0;
    for (Strv cons = s.v; cons.len != (size_t)-1; ++i) {
      res = str_split(&cons, "");
      triax_assert_true(i < countof(strs));
      triax_expect_memeq(res.str, strs[i], strlen(strs[i]));
      printf("%.*s vs %.*s\n", (int)res.len, res.str, (int)strlen(strs[i]), strs[i]);
    }
    triax_assert_eq(i, countof(strs));
    str_release(&s);
  }
}

triax_test(string, str_tok_all_2) {
  {
    Strv        res;
    const char* strs[] = {""};
    Str         s      = str_from_literal("");
    size_t      i      = 0;
    for (Strv cons = s.v; cons.len != (size_t)-1; ++i) {
      res = str_split(&cons, ",");
      triax_assert_true(i < countof(strs));
      triax_expect_streq(triax_str(res.str, res.len), strs[i]);
    }
    triax_assert_eq(i, countof(strs));
    str_release(&s);
  }
  {
    Strv        res;
    const char* strs[] = {"", "", "ab", ""};
    Str         s      = str_from_literal(",,ab,");
    size_t      i      = 0;
    for (Strv cons = s.v; cons.len != (size_t)-1; ++i) {
      res = str_split(&cons, ",");
      triax_assert_true(i < countof(strs));
      triax_expect_streq(triax_str(res.str, res.len), strs[i]);
    }
    triax_assert_eq(i, countof(strs));
    str_release(&s);
  }
  {
    Strv        res;
    const char* strs[] = {"", "", "", "", ""};
    Str         s      = str_from_literal(",,,,");
    size_t      i      = 0;
    for (Strv cons = s.v; cons.len != (size_t)-1; ++i) {
      res = str_split(&cons, ",");
      triax_assert_true(i < countof(strs));
      triax_expect_streq(triax_str(res.str, res.len), strs[i]);
    }
    triax_assert_eq(i, countof(strs));
    str_release(&s);
  }
  {
    Strv        res;
    const char* strs[] = {",,,,"};
    Str         s      = str_from_literal(",,,,");
    size_t      i      = 0;
    for (Strv cons = s.v; cons.len != (size_t)-1; ++i) {
      res = str_split(&cons, "");
      triax_assert_true(i < countof(strs));
      triax_expect_streq(triax_str(res.str, res.len), strs[i]);
    }
    triax_assert_eq(i, countof(strs));
    str_release(&s);
  }
  {
    Strv        res;
    const char* strs[] = {"hello"};
    Str         s      = str_from_literal("hello");
    size_t      i      = 0;
    for (Strv cons = s.v; cons.len != (size_t)-1; ++i) {
      res = str_split(&cons, ",");
      triax_assert_true(i < countof(strs));
      triax_expect_streq(triax_str(res.str, res.len), strs[i]);
    }
    triax_assert_eq(i, countof(strs));
    str_release(&s);
  }
  {
    Strv        res;
    const char* strs[] = {",hello"};
    Str         s      = str_from_literal(",hello");
    size_t      i      = 0;
    for (Strv cons = s.v; cons.len != (size_t)-1; ++i) {
      res = str_split(&cons, "");
      triax_assert_true(i < countof(strs));

      triax_expect_streq(triax_str(res.str, res.len), strs[i]);
    }
    triax_assert_eq(i, countof(strs));
    str_release(&s);
  }
}

// #define triax_expect_strv_eq(s1, s2)
//   (triax_expect_eq((s1).len, (s2).len),
//    triax_expect_streq((s1).str, (s2).str, rk_min((s1).len, (s2).len)))
// #define triax_expect_strv_neq(s1, s2)
//   (triax_expect_eq((s1).len, (s2).len),
//    triax_expect_strneq_n((s1).str, (s2).str, rk_min((s1).len, (s2).len)))

triax_test(string, str_tok_all_3) {
  Strv s1 = strv_from_literal("hello"), s2 = strv_from_literal("hello"),
       s3 = strv_from_literal("hell"), s4 = strv_from_literal("hollo");
  (void)s1, (void)s2, (void)s3, (void)s4;
  // triax_expect_strv_eq(s1, s2);
  // triax_expect_strv_neq(s1, s3);
  // triax_expect_strv_neq(s1, s4);
}

#if 0
# define str_front(strlike)      (*RK__qcharptr(strlike, RK__str_front_ptr(strv_from(strlike))))

# define str_back(strlike)       (*RK__qcharptr(strlike, RK__str_back_ptr(strv_from(strlike))))

// ----------------------------------------
// Section: String Lifetime/Ownership
// ----------------------------------------

# define str_init(init_cap, ...) rk_overload(RK__str_init, init_cap, ##__VA_ARGS__)

# define str_from(strlike, ...)  rk_overload(RK__str_from, strlike, ##__VA_ARGS__)

# define str_from_literal(strlit, ...)     rk_overload(RK__str_fromlit, strlit, ##__VA_ARGS__)

static_fun void str_release(Str* restrict self) {
  alloc_delete(self->str, self->cap, self->alloc);
  self->len = self->cap = 0, self->str = (char*)0;
}
static_fun size_t str_remaining(const Str* self) {
  return (self->cap - 1) - self->len;
}


static_fun bool str_is_null_terminated(const Str* self) {
  return self->cap > self->len && self->str[self->len] == '\0';
}
static_fun Str* str_reserve(Str* restrict self, size_t newcap);
static_fun Str* str_resize(Str* restrict self, size_t newlen);
static_fun Str* str_shrink_to_fit(Str* restrict self);
static_fun Str* str_shrink_to_fit_exact(Str* restrict self);
# define str_assign(self, strlike)         str_assign_strv(self, strv_from(strlike))
static_fun Str* str_null_terminate(Str* restrict self);
static_fun Str* str_null_terminate_unchecked(Str* restrict self);

# define str_terminate(self, suffix)       RK__str_terminate(self, suffix)
static_fun Str* str_push(Str* restrict self, char c);
static_fun Str* str_push_unchecked(Str* restrict self, char c);
static_fun Str* str_push_raw(Str* restrict self, char c);
static_fun Str* str_push_unchecked_raw(Str* restrict self, char c);

# define str_cat(self, strlike)            RK__str_cat(self, strlike)

# define str_cat_mayalias(self, strlike)   RK__str_cat_mayalias(self, strlike)

# define str_cat_literal(self, strlit)     str_cat_strv(self, (Strv)strv_from_literal(strlit))


extern_fun Str* str_cat_fmt(Str* self, const char* fmt, ...);

# define str_insert_at(self, idx, strlike) RK__str_insert_at(self, idx, strlike)

# define str_insert_at_mayalias(self, idx, strlike) RK__str_insert_at_mayalias(self, idx, strlike)


static_fun char str_pop(Str* restrict self);
static_fun void str_pop_n(Str* restrict self, size_t n);
extern_fun Str* str_erase_at(Str* restrict self, size_t idx);
extern_fun Str* str_erase_at_n(Str* restrict self, size_t idx, size_t count);
static_fun Str* str_clear(Str* restrict self) {
  return (self->str && (self->str[0] = '\0')), self->len = 0, self;
}

extern_fun Str* str_replace(Str* restrict self, char oldc, char newc);
static_fun char str_replace_at(Str* restrict self, size_t pos, char c);
extern_fun Str* str_to_upper(Str* restrict self);
extern_fun Str* str_to_lower(Str* restrict self);
extern_fun Str* str_reverse(Str* restrict self);

// ----------------------------------------
// Section: Strv Construction and View
// ----------------------------------------

# define strv_from_literal(strlit)                  {.str = strlit, .len = lenof(strlit)}

# define strv_from(strlike)                         RK__strv_from(strlike)

# define strv_slice(strlike, start, end)            strv_slice_strv(strv_from(strlike), start, end)

# define strv_slice_static(arr, start, end)                                                        \
   {.str = (const char*)((arr) + (start)), .len = (end) - (start)}

# define str_split(strvptr, delims) RK__str_split(strvptr, delims)

# define STR_SPLIT_END              ((size_t)-1) // todo use

# define str_trimmed(strlike)       str_trimmed_strv(strv_from(strlike))
# define str_trimmed_left(strlike)  str_trimmed_left_strv(strv_from(strlike))
# define str_trimmed_right(strlike) str_trimmed_right_strv(strv_from(strlike))

# define str_split_alloc(strlike, delims, out_count, ...)                                          \
   rk_overload(RK__str_tok_alloc, strv_from(strlike), strv_from(delims), out_count, ##__VA_ARGS__)

// ----------------------------------------
// Section: Strlike Query/Search Functions
// ----------------------------------------

# define str_compare(strlike1, strlike2) str_compare_strv(strv_from(strlike1), strv_from(strlike2))

# define str_equals(strlike1, strlike2)  str_equals_strv(strv_from(strlike1), strv_from(strlike2))

# define str_find(strlike, subs)         RK__str_find(strlike, subs)

# define str_findr(strlike, subs)        RK__str_findr(strlike, subs)

# define str_contains(strlike, subs)     RK__str_contains(strlike, subs)

# define str_starts_with(strlike, pre)   RK__str_starts_with(strlike, pre)

# define str_ends_with(strlike, suf)     RK__str_ends_with(strlike, suf)

#endif
triax_test(string, string_test_fmt) {
  Str s = str_init(1);
  str_cat_fmt(&s, "%d", 5);
  triax_expect_streq(s.str, "5");
  triax_expect_true(s.str[0] == '5');
  triax_expect_eq(s.len, 1u);
}
triax_test(string, string_test_split_example) {
  Str    s = str_from_literal("this is a test");
  Strv   v = s.v;
  Strv   buf[64];
  size_t i = 0;
  while (v.len != STR_SPLIT_END) { buf[i++] = str_split(&v, ' '); }
  triax_assert_eq(i, 4);
  triax_expect_true(buf[0].len == 4u && !memcmp(buf[0].str, "this", 4));
  triax_expect_true(buf[1].len == 2u && !memcmp(buf[1].str, "is", 2));
  triax_expect_true(buf[2].len == 1u && !memcmp(buf[2].str, "a", 1));
  triax_expect_true(buf[3].len == 4u && !memcmp(buf[3].str, "test", 4));
  Str    s2 = {RK_ZINIT};
  Strv   v2 = s2.v;
  Strv   buf2[64];
  size_t i2 = 0;
  while (v2.len != STR_SPLIT_END) { buf2[i2++] = str_split(&v2, ' '); }
  triax_assert_eq(i2, 1);
  triax_expect_eq(buf2[0].len, 0u);
}
triax_test(string, string_test_empty) {
  Str s = {RK_ZINIT};
  triax_expect_eq(str_dat(s), rk_null);
  triax_expect_eq(str_len(s), 0u);
  triax_expect_true(str_is_empty(s));
  triax_expect_false(str_is_null_terminated(&s));
  str_release(&s); // shall not crash

  str_reserve(&s, 0);          // should be nop
  str_shrink_to_fit(&s);       // should be nop
  str_shrink_to_fit_exact(&s); // should be nop
  str_resize(&s, 0);           // should initialise the string
  triax_expect_true(str_is_null_terminated(&s));
  str_release(&s); // shall not crash
  s = (Str){RK_ZINIT};
  str_push(&s, 'c');
  triax_expect_streq(s.str, "c");
  str_release(&s); // shall not crash
  s = (Str){RK_ZINIT};

  str_cat(&s, "5");
  triax_expect_streq(s.str, "5");
  str_release(&s); // shall not crash
  s = (Str){RK_ZINIT};
  str_cat_fmt(&s, "%d", 5);
  triax_expect_streq(s.str, "5");
  triax_expect_eq(s.len, 1u);

  // triax_expect_strv_eq(s1, s2);
  // triax_expect_strv_neq(s1, s3);
  // triax_expect_strv_neq(s1, s4);
}

RK__IGNWARN_CLANG_END()
RK_HEADER_END
#endif
