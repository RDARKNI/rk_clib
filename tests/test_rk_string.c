#ifndef TEST_STRING_H
#define TEST_STRING_H
#include "conf.h"

RKI_HEADER_BEGIN
RKI_IGNWARN_CLANG_BEG("-Wunused-variable")

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
  triax_expect_false(str_starts_with("hello", 'a'));
  triax_expect_true(str_starts_with("hello", 'h'));
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

// ---- accessors: _front/_back/_at are asserted lvalues, _peek_* are nullable pointers ----

triax_test(string, at_front_back_are_writable_lvalues) {
  Str s = str_from_literal("hello");
  str_front(s) = 'j';
  str_at(s, 1) = 'E';
  str_back(s)  = 'y';
  triax_expect_streq(s.str, "jElly");
  triax_expect_eq(&str_at(s, 4), &str_back(s));
  str_release(&s);

  char  buf[] = "abc";
  char* p     = buf;
  str_at(p, 1) = 'B';
  str_back(p)  = 'C';
  triax_expect_streq(buf, "aBC");
}

triax_test(string, peek_in_bounds_matches_at) {
  Str  s  = str_from_literal("hello");
  Strv sv = s.v;
  triax_expect_eq(str_peek_front(s), &str_front(s));
  triax_expect_eq(str_peek_back(s), &str_back(s));
  for (size_t i = 0; i < 5; ++i) {
    triax_expect_eq(str_peek_at(s, i), &str_at(s, i));
    triax_expect_eq(str_peek_at(sv, i), s.str + i);
  }
  *str_peek_at(s, 0) = 'H';
  triax_expect_eq(str_at(sv, 0), 'H');
  triax_expect_eq(*str_peek_back("xyz"), 'z');
  str_release(&s);
}

triax_test(string, peek_out_of_bounds_returns_null) {
  Str s = str_from_literal("hi");
  triax_expect_null(str_peek_at(s, 2)); // the terminating null is not an element
  triax_expect_null(str_peek_at(s, (size_t)-1));
  str_release(&s);

  Str empty = str_from_literal("");
  triax_expect_null(str_peek_front(empty));
  triax_expect_null(str_peek_back(empty));
  triax_expect_null(str_peek_at(empty, 0));
  str_release(&empty);

  Str  unalloc = {RKI_ZINIT};
  Strv nullv   = {rk_null, 0};
  triax_expect_null(str_peek_front(unalloc));
  triax_expect_null(str_peek_back(nullv));
  triax_expect_null(str_peek_at(nullv, 0));
  triax_expect_null(str_peek_front(""));
}

triax_test(string, accessors_follow_stringlike_constness) {
  Str         s  = str_from_literal("abc");
  Strv        sv = s.v;
  char*       p  = s.str;
  const char* cp = s.str;
  static_assert(_Generic(&str_at(s, 0), char*: 1, default: 0), "Str yields char");
  static_assert(_Generic(&str_at(p, 0), char*: 1, default: 0), "char* yields char");
  static_assert(_Generic(&str_at(sv, 0), const char*: 1, default: 0), "Strv yields const char");
  static_assert(_Generic(&str_at(cp, 0), const char*: 1, default: 0), "const char* yields const");
  static_assert(_Generic(&str_front(sv), const char*: 1, default: 0), "Strv yields const char");
  static_assert(_Generic(&str_back(s), char*: 1, default: 0), "Str yields char");
  static_assert(_Generic(str_peek_at(s, 0), char*: 1, default: 0), "Str yields char");
  static_assert(_Generic(str_peek_at(cp, 0), const char*: 1, default: 0), "const yields const");
  static_assert(_Generic(str_begin(s), char*: 1, default: 0), "Str yields char");
  static_assert(_Generic(str_end(sv), const char*: 1, default: 0), "Strv yields const char");
  triax_expect_eq(str_at(cp, 2), 'c');
  str_release(&s);
}

triax_test(string, begin_end_cover_exactly_len_chars) {
  Str s = str_from_literal("hello");
  triax_expect_eq(str_begin(s), s.str);
  triax_expect_eq((size_t)(str_end(s) - str_begin(s)), str_len(s));
  triax_expect_eq(*str_end(s), '\0'); // the terminator, for a null-terminated string
  triax_expect_eq((size_t)(str_end("abc") - str_begin("abc")), 3u);

  Strv sub = strv_from_cstrn(s.str, 3); // a view need not end at a terminator
  triax_expect_eq(str_end(sub), s.str + 3);
  triax_expect_eq(*str_end(sub), 'l');

  size_t n = 0;
  for (const char* it = str_begin(sub); it != str_end(sub); ++it) { ++n; }
  triax_expect_eq(n, 3u);
  str_release(&s);
}

triax_test(string, begin_end_of_empty_strings_form_empty_ranges) {
  Str empty = str_from_literal("");
  triax_expect_nonnull(str_begin(empty));
  triax_expect_eq(str_begin(empty), str_end(empty));
  str_release(&empty);

  Str  unalloc = {RKI_ZINIT};
  Strv nullv   = {rk_null, 0};
  triax_expect_null(str_begin(unalloc));
  triax_expect_null(str_end(unalloc));
  triax_expect_null(str_begin(nullv));
  triax_expect_null(str_end(nullv));
  triax_expect_eq(str_begin(""), str_end(""));
}

// An expected fault ends the test process, so each case needs its own test. The results are
// discarded on purpose: the bounds check must still run (see rki_str_at_ptr).
#ifdef RKLIB_DEBUG
triax_test(string, at_out_of_bounds_asserts, .isolation = TRIAX_ISOLATION_ON) {
  Strv sv = strv_from_literal("abc");
  triax_assert_fault(TRIAX_FAULT_ABORT, { (void)str_at(sv, 3); });
}

triax_test(string, front_of_empty_asserts, .isolation = TRIAX_ISOLATION_ON) {
  Strv sv = strv_from_literal("");
  triax_assert_fault(TRIAX_FAULT_ABORT, { (void)str_front(sv); });
}

triax_test(string, back_of_empty_asserts, .isolation = TRIAX_ISOLATION_ON) {
  Str s = {RKI_ZINIT};
  triax_assert_fault(TRIAX_FAULT_ABORT, { (void)str_back(s); });
}
#endif

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

  Str s4 = {RKI_ZINIT};
  triax_expect_nonnull(str_null_terminate(&s4));

  triax_expect_true(str_is_null_terminated(str_null_terminate(&s4)));
  str_release(&s2), str_release(&s3);
}

triax_test(string, str_cap) {
  Str s = str_init(4);
  triax_expect_eq(str_cap(&s), s.cap);
  triax_expect_true(str_cap(&s) >= 4u);

  size_t before = str_cap(&s);
  str_reserve(&s, 100);
  triax_expect_true(str_cap(&s) >= 100u);
  triax_expect_true(str_cap(&s) > before);
  triax_expect_eq(str_cap(&s), s.cap);

  str_release(&s);
  triax_expect_eq(str_cap(&s), 0u);

  Str z = {0};
  triax_expect_eq(str_cap(&z), 0u);
}

static unsigned char   str_alloc_storage[4096];
rk_unused static Arena STR_TEST_ARENA = arena_init_static(str_alloc_storage);

triax_test(string, str_allocator) {
  {
    rk_unused Str z = {0};
    RK_IFALLOC(triax_expect_memeq((Allocator[]){str_allocator(&z)}, &alloc_ctx, sizeof(alloc_ctx));)
  }
  {
    Str s = str_init(4);
    RK_IFALLOC(triax_expect_memeq((Allocator[]){str_allocator(&s)}, &alloc_ctx, sizeof(alloc_ctx));)
    str_release(&s);
  }
  {
    rk_unused Allocator used_alloc = alloc_ctx;
#if RK_CUSTOM_ALLOCATORS
    used_alloc = arena_to_alloc(&STR_TEST_ARENA);
#endif
    Str s = str_init(4 RK_IFALLOC(, used_alloc));
    RK_IFALLOC(
        triax_expect_memeq((Allocator[]){str_allocator(&s)}, &used_alloc, sizeof(used_alloc));)
    str_release(&s);
  }
}

triax_test(string, clone_zero_initialized) {
  Str empty = {0};
  Str copy = str_from(empty);
  triax_expect_eq(copy.len, 0u);
  triax_expect_true(str_is_null_terminated(&copy));
  RK_IFALLOC(triax_expect_memeq((Allocator[]){str_allocator(&copy)}, &alloc_ctx, sizeof(alloc_ctx));)
  str_release(&copy);
}

triax_test(string, mayalias_terminator) {
  for (int grow = 0; grow < 2; ++grow) {
    Str appended = str_from("a");
    if (!grow) { str_reserve(&appended, 8); }
    Strv terminator = {appended.str + appended.len, 1};
    str_cat_mayalias(&appended, terminator);
    triax_expect_eq(appended.len, 2u);
    triax_expect_memeq(appended.str, "a\0", 3);
    str_release(&appended);

    Str inserted = str_from("a");
    if (!grow) { str_reserve(&inserted, 8); }
    terminator = (Strv){inserted.str + inserted.len, 1};
    str_insert_at_mayalias(&inserted, 0, terminator);
    triax_expect_eq(inserted.len, 2u);
    triax_expect_memeq(inserted.str, "\0a", 3);
    str_release(&inserted);
  }
}

triax_test(string, mayalias_append_overlapping_view) {
  Str s = str_from("ab");
  str_reserve(&s, 16);
  Strv including_terminator = {s.str, s.len + 1};
  str_cat_mayalias(&s, including_terminator);
  triax_expect_eq(s.len, 5u);
  triax_expect_memeq(s.str, "abab\0", 6);
  str_release(&s);
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

// ---- str_pop: asserted; str_try_pop: recoverable ----

triax_test(string, pop_removes_last_and_keeps_terminator) {
  Str s = str_from_literal("abc");
  triax_expect_eq(str_pop(&s), 'c');
  triax_expect_eq(str_len(s), 2u);
  triax_expect_eq(s.str[2], '\0');
  triax_expect_eq(str_pop(&s), 'b');
  triax_expect_eq(str_pop(&s), 'a');
  triax_expect_true(str_is_empty(s));
  triax_expect_streq(s.str, "");
  str_release(&s);
}

triax_test(string, try_pop_nonempty_removes_last) {
  Str  s   = str_from_literal("xy");
  char out = '?';
  triax_expect_true(str_try_pop(&s, &out));
  triax_expect_eq(out, 'y');
  triax_expect_streq(s.str, "x");
  triax_expect_true(str_try_pop(&s, &out));
  triax_expect_eq(out, 'x');
  triax_expect_false(str_try_pop(&s, &out)); // now empty
  triax_expect_eq(out, 'x');                 // left untouched
  str_release(&s);
}

triax_test(string, try_pop_empty_or_unallocated_fails_and_leaves_out) {
  Str  empty = str_from_literal("");
  char out   = '?';
  triax_expect_false(str_try_pop(&empty, &out));
  triax_expect_eq(out, '?');
  str_release(&empty);

  Str unalloc = {RKI_ZINIT};
  triax_expect_false(str_try_pop(&unalloc, &out));
  triax_expect_eq(out, '?');
  triax_expect_null(unalloc.str);
}

// A Str tracks its length, so it can hold an embedded '\0'. Popping it is a real character, which
// a sentinel return value could not distinguish from an empty Str.
triax_test(string, pop_embedded_nul_is_a_character) {
  Str s = str_from_literal("a");
  str_push(&s, '\0');
  triax_expect_eq(str_len(s), 2u);
  char out = '?';
  triax_expect_true(str_try_pop(&s, &out));
  triax_expect_eq(out, '\0');
  triax_expect_eq(str_len(s), 1u);
  triax_expect_eq(str_pop(&s), 'a');
  triax_expect_false(str_try_pop(&s, &out));
  str_release(&s);
}

triax_test(string, pop_n_zero_is_noop_even_unallocated) {
  Str s = str_from_literal("abc");
  str_pop_n(&s, 0);
  triax_expect_streq(s.str, "abc");
  str_pop_n(&s, 2);
  triax_expect_streq(s.str, "a");
  str_release(&s);

  Str unalloc = {RKI_ZINIT};
  str_pop_n(&unalloc, 0);
  triax_expect_null(unalloc.str);
}

// An expected fault ends the test process, so each case needs its own test.
#ifdef RKLIB_DEBUG
triax_test(string, pop_of_empty_asserts, .isolation = TRIAX_ISOLATION_ON) {
  Str s = str_from_literal("");
  triax_assert_fault(TRIAX_FAULT_ABORT, { (void)str_pop(&s); });
}

triax_test(string, pop_of_unallocated_asserts, .isolation = TRIAX_ISOLATION_ON) {
  Str s = {RKI_ZINIT};
  triax_assert_fault(TRIAX_FAULT_ABORT, { (void)str_pop(&s); });
}

triax_test(string, pop_n_more_than_len_asserts, .isolation = TRIAX_ISOLATION_ON) {
  Str s = str_from_literal("ab");
  triax_assert_fault(TRIAX_FAULT_ABORT, { str_pop_n(&s, 3); });
}
#endif

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

static Str* rki_mark_eval_str(Str* s, int* count) {
  ++*count;
  return s;
}

triax_test(string, foreach_visits_all_chars) {
  Str  s      = str_from_literal("abcde");
  char buf[6] = {0};
  int  i      = 0;
  str_foreach(&s, it) { buf[i++] = *it; }
  triax_expect_eq(i, 5);
  triax_expect_streq(buf, "abcde");
  str_release(&s);
}

triax_test(string, foreach_reversed_visits_all_chars) {
  Str  s      = str_from_literal("abcde");
  char buf[6] = {0};
  int  i      = 0;
  str_foreach_reversed(&s, it) { buf[i++] = *it; }
  triax_expect_eq(i, 5);
  triax_expect_streq(buf, "edcba");
  str_release(&s);
}

triax_test(string, foreach_break_stops_iteration) {
  Str s      = str_from_literal("abcde");
  int visits = 0;
  str_foreach(&s, it) {
    if (*it == 'c') { break; }
    ++visits;
  }
  triax_expect_eq(visits, 2); // 'a', 'b'
  str_release(&s);
}

triax_test(string, foreach_continue_skips_char) {
  Str s      = str_from_literal("abcde");
  int visits = 0;
  str_foreach(&s, it) {
    if (*it == 'c') { continue; }
    ++visits;
  }
  triax_expect_eq(visits, 4); // all but 'c'
  str_release(&s);
}

triax_test(string, foreach_reversed_break_stops_iteration) {
  Str s      = str_from_literal("abcde");
  int visits = 0;
  str_foreach_reversed(&s, it) {
    if (*it == 'c') { break; }
    ++visits;
  }
  triax_expect_eq(visits, 2); // 'e', 'd'
  str_release(&s);
}

triax_test(string, foreach_reversed_continue_skips_char) {
  Str s      = str_from_literal("abcde");
  int visits = 0;
  str_foreach_reversed(&s, it) {
    if (*it == 'c') { continue; }
    ++visits;
  }
  triax_expect_eq(visits, 4); // all but 'c'
  str_release(&s);
}

triax_test(string, foreach_evaluates_str_argument_once) {
  Str s     = str_from_literal("abc");
  int evals = 0, visits = 0;
  str_foreach(rki_mark_eval_str(&s, &evals), it) {
    (void)it;
    ++visits;
  }
  triax_expect_eq(evals, 1);
  triax_expect_eq(visits, 3);
  str_release(&s);
}

triax_test(string, foreach_reversed_evaluates_str_argument_once) {
  Str s     = str_from_literal("abc");
  int evals = 0, visits = 0;
  str_foreach_reversed(rki_mark_eval_str(&s, &evals), it) {
    (void)it;
    ++visits;
  }
  triax_expect_eq(evals, 1);
  triax_expect_eq(visits, 3);
  str_release(&s);
}

triax_test(string, erase_if_evaluates_str_argument_once) {
  Str s     = str_from_literal("abcdefgh");
  int evals = 0;
  str_erase_if(rki_mark_eval_str(&s, &evals), it, *it == 'a');
  triax_expect_eq(evals, 1);
  triax_expect_streq(s.str, "bcdefgh");
  str_release(&s);
}

triax_test(string, erase_if_evaluates_predicate_once_per_char) {
  Str s          = str_from_literal("abcdefgh");
  int pred_calls = 0;
  str_erase_if(&s, it, (++pred_calls, *it == 'a'));
  triax_expect_eq(pred_calls, 8);
  triax_expect_streq(s.str, "bcdefgh");
  str_release(&s);
}

triax_test(string, foreach_empty_is_noop) {
  Str s     = str_init(0);
  int count = 0;
  str_foreach(&s, it) {
    (void)it;
    ++count;
  }
  triax_expect_eq(count, 0);
  str_release(&s);
}

triax_test(string, foreach_reversed_empty_is_noop) {
  Str s     = str_init(0);
  int count = 0;
  str_foreach_reversed(&s, it) {
    (void)it;
    ++count;
  }
  triax_expect_eq(count, 0);
  str_release(&s);
}

triax_test(string, foreach_on_zero_initialized_str_is_noop) {
  Str s     = {RKI_ZINIT};
  int count = 0;
  str_foreach(&s, it) {
    (void)it;
    ++count;
  }
  str_foreach_reversed(&s, it) {
    (void)it;
    ++count;
  }
  triax_expect_eq(count, 0);
}

triax_test(string, erase_if_empty_is_noop) {
  Str s          = str_init(0);
  int pred_calls = 0;
  str_erase_if(&s, it, (++pred_calls, (void)it, true));
  triax_expect_eq(pred_calls, 0);
  str_release(&s);
}

triax_test(string, erase_if_on_zero_initialized_str_is_noop) {
  Str s          = {RKI_ZINIT};
  int pred_calls = 0;
  str_erase_if(&s, it, (++pred_calls, (void)it, true));
  triax_expect_eq(pred_calls, 0);
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
# define str_front(strlike)      (*RKI_qcharptr(strlike, RKI_str_front_ptr(strv_from(strlike))))

# define str_back(strlike)       (*RKI_qcharptr(strlike, RKI_str_back_ptr(strv_from(strlike))))

// ----------------------------------------
// Section: String Lifetime/Ownership
// ----------------------------------------

# define str_init(init_cap, ...) RKI_OVERLOAD(RKI_str_init, init_cap, ##__VA_ARGS__)

# define str_from(strlike, ...)  RKI_OVERLOAD(RKI_str_from, strlike, ##__VA_ARGS__)

# define str_from_literal(strlit, ...)     RKI_OVERLOAD(RKI_str_fromlit, strlit, ##__VA_ARGS__)

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

# define str_terminate(self, suffix)       RKI_str_terminate(self, suffix)
static_fun Str* str_push(Str* restrict self, char c);
static_fun Str* str_push_unchecked(Str* restrict self, char c);
static_fun Str* str_push_raw(Str* restrict self, char c);
static_fun Str* str_push_unchecked_raw(Str* restrict self, char c);

# define str_cat(self, strlike)            RKI_str_cat(self, strlike)

# define str_cat_mayalias(self, strlike)   RKI_str_cat_mayalias(self, strlike)

# define str_cat_literal(self, strlit)     str_cat_strv(self, (Strv)strv_from_literal(strlit))


extern_fun Str* str_cat_fmt(Str* self, const char* fmt, ...);

# define str_insert_at(self, idx, strlike) RKI_str_insert_at(self, idx, strlike)

# define str_insert_at_mayalias(self, idx, strlike) RKI_str_insert_at_mayalias(self, idx, strlike)


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

# define strv_from(strlike)                         RKI_strv_from(strlike)

# define strv_slice(strlike, start, end)            strv_slice_strv(strv_from(strlike), start, end)

# define strv_slice_static(arr, start, end)                                                        \
   {.str = (const char*)((arr) + (start)), .len = (end) - (start)}

# define str_split(strvptr, delims) RKI_str_split(strvptr, delims)

# define STR_SPLIT_END              ((size_t)-1) // todo use

# define str_trimmed(strlike)       str_trimmed_strv(strv_from(strlike))
# define str_trimmed_left(strlike)  str_trimmed_left_strv(strv_from(strlike))
# define str_trimmed_right(strlike) str_trimmed_right_strv(strv_from(strlike))

# define str_split_alloc(strlike, delims, out_count, ...)                                          \
   RKI_OVERLOAD(RKI_str_tok_alloc, strv_from(strlike), strv_from(delims), out_count, ##__VA_ARGS__)

// ----------------------------------------
// Section: Strlike Query/Search Functions
// ----------------------------------------

# define str_compare(strlike1, strlike2) str_compare_strv(strv_from(strlike1), strv_from(strlike2))

# define str_equals(strlike1, strlike2)  str_equals_strv(strv_from(strlike1), strv_from(strlike2))

# define str_find(strlike, subs)         RKI_str_find(strlike, subs)

# define str_findr(strlike, subs)        RKI_str_findr(strlike, subs)

# define str_contains(strlike, subs)     RKI_str_contains(strlike, subs)

# define str_starts_with(strlike, pre)   RKI_str_starts_with(strlike, pre)

# define str_ends_with(strlike, suf)     RKI_str_ends_with(strlike, suf)

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
  Str    s2 = {RKI_ZINIT};
  Strv   v2 = s2.v;
  Strv   buf2[64];
  size_t i2 = 0;
  while (v2.len != STR_SPLIT_END) { buf2[i2++] = str_split(&v2, ' '); }
  triax_assert_eq(i2, 1);
  triax_expect_eq(buf2[0].len, 0u);
}
triax_test(string, string_test_empty) {
  Str s = {RKI_ZINIT};
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
  s = (Str){RKI_ZINIT};
  str_push(&s, 'c');
  triax_expect_streq(s.str, "c");
  str_release(&s); // shall not crash
  s = (Str){RKI_ZINIT};

  str_cat(&s, "5");
  triax_expect_streq(s.str, "5");
  str_release(&s); // shall not crash
  s = (Str){RKI_ZINIT};
  str_cat_fmt(&s, "%d", 5);
  triax_expect_streq(s.str, "5");
  triax_expect_eq(s.len, 1u);

  // triax_expect_strv_eq(s1, s2);
  // triax_expect_strv_neq(s1, s3);
  // triax_expect_strv_neq(s1, s4);
}

RKI_IGNWARN_CLANG_END()
RKI_HEADER_END
#endif
