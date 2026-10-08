#ifndef TEST_DEQUE_H
#define TEST_DEQUE_H
#include "conf.h"

#define RK_IMPL
#include "../include/rklib.h"

RKI_HEADER_BEGIN
RKI_IGNWARN_CLANG_BEG("-Wunused-variable")

DEQUE_DEFINE(int)

/* ------------------------------------------------------------------------ */
/* Parameterized correctness test: a small "script" of push/pop operations  */
/* replayed against a fresh Deque(int), checked against its expected final  */
/* logical contents. Collapses what would otherwise be one near-identical   */
/* test body per scenario into a single test run once per case.             */
/* ------------------------------------------------------------------------ */
typedef enum { OP_PUSH_BACK, OP_PUSH_FRONT, OP_POP_BACK, OP_POP_FRONT } DequeOpKind;

typedef struct {
  DequeOpKind kind;
  int         value; // unused for OP_POP_*
} DequeOp;

typedef struct {
  const char* name;
  DequeOp     ops[12];
  size_t      n_ops;
  int         expect[8];
  size_t      n_expect;
} DequeScript;

static const DequeScript deque_scripts[] = {
    {
        "push_back_only",
        {{OP_PUSH_BACK, 1}, {OP_PUSH_BACK, 2}, {OP_PUSH_BACK, 3}},
        3,
        {1, 2, 3},
        3,
    },
    {
        "push_front_only",
        {{OP_PUSH_FRONT, 1}, {OP_PUSH_FRONT, 2}, {OP_PUSH_FRONT, 3}},
        3,
        {3, 2, 1},
        3,
    },
    {
        "mixed_ends",
        {{OP_PUSH_BACK, 2},
         {OP_PUSH_BACK, 3},
         {OP_PUSH_FRONT, 1},
         {OP_PUSH_FRONT, 0},
         {OP_PUSH_BACK, 4}},
        5,
        {0, 1, 2, 3, 4},
        5,
    },
    {
        "pop_front_then_push",
        {{OP_PUSH_BACK, 1},
         {OP_PUSH_BACK, 2},
         {OP_PUSH_BACK, 3},
         {OP_POP_FRONT, 0},
         {OP_POP_FRONT, 0},
         {OP_PUSH_BACK, 4},
         {OP_PUSH_BACK, 5}},
        7,
        {3, 4, 5},
        3,
    },
    {
        "pop_back_then_push",
        {{OP_PUSH_BACK, 1},
         {OP_PUSH_BACK, 2},
         {OP_PUSH_BACK, 3},
         {OP_PUSH_BACK, 4},
         {OP_PUSH_BACK, 5},
         {OP_POP_BACK, 0},
         {OP_POP_BACK, 0}},
        7,
        {1, 2, 3},
        3,
    },
    {
        // Starting from a zero-initialized (cap 0) deque, this crosses two
        // growth events, the second of which happens while head has already
        // wrapped backward several times -- the trickiest case for a circular
        // buffer's grow-and-unwrap logic.
        "growth_while_wrapped",
        {{OP_PUSH_BACK, 0},
         {OP_PUSH_BACK, 1},
         {OP_PUSH_BACK, 2},
         {OP_PUSH_FRONT, -1},
         {OP_PUSH_FRONT, -2},
         {OP_PUSH_FRONT, -3},
         {OP_PUSH_FRONT, -4},
         {OP_PUSH_FRONT, -5}},
        8,
        {-5, -4, -3, -2, -1, 0, 1, 2},
        8,
    },
    {
        "drain_fully",
        {{OP_PUSH_BACK, 1},
         {OP_PUSH_BACK, 2},
         {OP_PUSH_BACK, 3},
         {OP_PUSH_BACK, 4},
         {OP_PUSH_BACK, 5},
         {OP_POP_FRONT, 0},
         {OP_POP_FRONT, 0},
         {OP_POP_FRONT, 0},
         {OP_POP_FRONT, 0},
         {OP_POP_FRONT, 0}},
        10,
        {0},
        0,
    },
};

static void deque_apply_op(Deque(int) * q, const DequeOp* op) {
  switch (op->kind) {
  case OP_PUSH_BACK : deque_push_back(q, op->value); break;
  case OP_PUSH_FRONT: deque_push_front(q, op->value); break;
  case OP_POP_BACK  : (void)deque_pop_back(q); break;
  case OP_POP_FRONT : (void)deque_pop_front(q); break;
  }
}

triax_test(deque, scripted, .params = triax_as_params(deque_scripts)) {
  const DequeScript* c = triax_param(DequeScript);
  Deque(int) q         = deque_init(int, 0);

  for (size_t i = 0; i < c->n_ops; ++i) { deque_apply_op(&q, &c->ops[i]); }

  triax_expect(deque_count(&q) == c->n_expect, "case: %s", c->name);
  for (size_t i = 0; i < c->n_expect; ++i) {
    int* p = deque_peek_at(&q, i);
    triax_expect_nonnull(p);
    if (p) { triax_expect_eq(*p, c->expect[i]); }
  }

  deque_release(&q);
}

/* ------------------------------------------------------------------------ */
/* Non-parameterized tests: everything not naturally expressible as an      */
/* op-script -- construction, accessors, iteration, capacity management.    */
/* ------------------------------------------------------------------------ */
triax_test(deque, init_and_empty) {
  Deque(int) q = deque_init(int, 0);
  triax_expect_true(deque_is_empty(&q));
  triax_expect_eq(deque_count(&q), 0u);
  triax_expect_null(deque_peek_front(&q));
  triax_expect_null(deque_peek_back(&q));
  triax_expect_null(deque_peek_at(&q, 0));
  deque_release(&q);

  // a plain zero-initialized struct is documented as a valid empty deque too
  Deque(int) z = {0};
  triax_expect_true(deque_is_empty(&z));
  triax_expect_eq(deque_count(&z), 0u);
  deque_push_back(&z, 7);
  triax_expect_eq(deque_front(&z), 7);
  deque_release(&z);

  // explicit initial capacity
  Deque(int) c = deque_init(int, 32);
  triax_expect_true(deque_cap(&c) >= 32u);
  triax_expect_true(deque_is_empty(&c));
  deque_release(&c);
}

// ---- accessors: _front/_back/_at are asserted lvalues, _peek_* are nullable pointers ----

// Builds 0, 1, 2, 3 with a wrapped head, so logical indices differ from storage indices.
static Deque(int) deque_wrapped_0123(void) {
  Deque(int) q = deque_init(int, 0);
  deque_push_back(&q, 2);
  deque_push_back(&q, 3);
  deque_push_front(&q, 1);
  deque_push_front(&q, 0);
  return q;
}

triax_test(deque, at_front_back_are_writable_lvalues) {
  Deque(int) q = deque_wrapped_0123();
  deque_front(&q) = 10;
  deque_at(&q, 1) += 10;
  deque_back(&q)  = 13;
  triax_expect_eq(deque_at(&q, 0), 10);
  triax_expect_eq(deque_at(&q, 1), 11);
  triax_expect_eq(deque_at(&q, 2), 2);
  triax_expect_eq(deque_at(&q, 3), 13);
  triax_expect_eq(&deque_at(&q, 3), &deque_back(&q));
  deque_release(&q);
}

triax_test(deque, peek_in_bounds_matches_at) {
  Deque(int) q = deque_wrapped_0123();
  triax_expect_eq(deque_peek_front(&q), &deque_front(&q));
  triax_expect_eq(deque_peek_back(&q), &deque_back(&q));
  for (size_t i = 0; i < 4; ++i) {
    triax_expect_eq(deque_peek_at(&q, i), &deque_at(&q, i));
    triax_expect_eq(*deque_peek_at(&q, i), (int)i);
  }
  int* p2 = deque_peek_at(&q, 2);
  triax_assert_nonnull(p2);
  if (p2) { *p2 = 20; }
  triax_expect_eq(deque_at(&q, 2), 20);
  deque_release(&q);
}

triax_test(deque, peek_out_of_bounds_returns_null) {
  Deque(int) q = deque_wrapped_0123();
  triax_expect_null(deque_peek_at(&q, 4));
  triax_expect_null(deque_peek_at(&q, (size_t)-1));
  deque_release(&q);

  Deque(int) z = {0};
  triax_expect_null(deque_peek_front(&z));
  triax_expect_null(deque_peek_back(&z));
  triax_expect_null(deque_peek_at(&z, 0));
}

triax_test(deque, accessors_propagate_deque_constness) {
  Deque(int)        q  = deque_wrapped_0123();
  const Deque(int)* cq = &q;
  static_assert(_Generic(&deque_at(cq, 0), const int*: 1, default: 0), "const yields const");
  static_assert(_Generic(&deque_front(cq), const int*: 1, default: 0), "const yields const");
  static_assert(_Generic(&deque_back(cq), const int*: 1, default: 0), "const yields const");
  static_assert(_Generic(deque_peek_at(cq, 0), const int*: 1, default: 0),
                "const yields const");
  static_assert(_Generic(deque_peek_front(cq), const int*: 1, default: 0),
                "const yields const");
  static_assert(_Generic(deque_peek_back(cq), const int*: 1, default: 0),
                "const yields const");
  static_assert(_Generic(&deque_at(&q, 0), int*: 1, default: 0), "mutable yields mutable");
  static_assert(_Generic(deque_peek_at(&q, 0), int*: 1, default: 0),
                "mutable yields mutable");
  triax_expect_eq(deque_at(cq, 3), 3);
  triax_expect_eq(*deque_peek_back(cq), 3);
  deque_release(&q);
}

// An expected fault ends the test process, so each case needs its own test.
#ifdef RKLIB_DEBUG
triax_test(deque, at_out_of_bounds_asserts, .isolation = TRIAX_ISOLATION_ON) {
  Deque(int) q = deque_wrapped_0123();
  triax_assert_fault(TRIAX_FAULT_ABORT, { (void)deque_at(&q, 4); });
}

triax_test(deque, front_of_empty_asserts, .isolation = TRIAX_ISOLATION_ON) {
  Deque(int) q = deque_init(int, 0);
  triax_assert_fault(TRIAX_FAULT_ABORT, { (void)deque_front(&q); });
}

triax_test(deque, back_of_empty_asserts, .isolation = TRIAX_ISOLATION_ON) {
  Deque(int) q = deque_init(int, 0);
  triax_assert_fault(TRIAX_FAULT_ABORT, { (void)deque_back(&q); });
}
#endif

triax_test(deque, try_pop) {
  Deque(int) q = deque_init(int, 0);

  int out      = -1;
  triax_expect_false(deque_try_pop_front(&q, &out));
  triax_expect_eq(out, -1); // left untouched
  triax_expect_false(deque_try_pop_back(&q, &out));
  triax_expect_eq(out, -1);

  deque_push_back(&q, 1);
  deque_push_back(&q, 2);
  deque_push_back(&q, 3);

  triax_expect_true(deque_try_pop_front(&q, &out));
  triax_expect_eq(out, 1);
  triax_expect_true(deque_try_pop_back(&q, &out));
  triax_expect_eq(out, 3);
  triax_expect_eq(deque_count(&q), 1u);

  triax_expect_true(deque_try_pop_front(&q, &out));
  triax_expect_eq(out, 2);
  triax_expect_true(deque_is_empty(&q));

  triax_expect_false(deque_try_pop_front(&q, &out));
  triax_expect_eq(out, 2); // still untouched by the failed try_pop

  deque_release(&q);
}

triax_test(deque, push_n) {
  // push_back_n: arr's order preserved, appended after existing elements
  {
    Deque(int) q = deque_init(int, 0);
    deque_push_back(&q, 1);
    int arr[] = {2, 3, 4};
    deque_push_back_n(&q, arr, 3);
    int expect[] = {1, 2, 3, 4};
    triax_expect_eq(deque_count(&q), 4u);
    for (size_t i = 0; i < 4; ++i) { triax_expect_eq(deque_at(&q, i), expect[i]); }
    deque_release(&q);
  }

  // push_front_n: arr's own order preserved as a prefix block, NOT reversed
  // (unlike 3 individual push_front calls, which would reverse it)
  {
    Deque(int) q = deque_init(int, 0);
    deque_push_back(&q, 4);
    int arr[] = {1, 2, 3};
    deque_push_front_n(&q, arr, 3);
    int expect[] = {1, 2, 3, 4};
    triax_expect_eq(deque_count(&q), 4u);
    for (size_t i = 0; i < 4; ++i) { triax_expect_eq(deque_at(&q, i), expect[i]); }
    deque_release(&q);
  }

  // n == 0 is a no-op
  {
    Deque(int) q = deque_init(int, 0);
    deque_push_back(&q, 1);
    deque_push_back_n(&q, (int*)rk_null, 0);
    deque_push_front_n(&q, (int*)rk_null, 0);
    triax_expect_eq(deque_count(&q), 1u);
    triax_expect_eq(deque_front(&q), 1);
    deque_release(&q);
  }

  // push_back_n where the write range wraps the physical buffer end
  {
    Deque(int) q = deque_init(int, 8);
    for (int i = 0; i < 6; ++i) { deque_push_back(&q, i); }
    for (int i = 0; i < 4; ++i) { (void)deque_pop_front(&q); } // logical: 4, 5
    int arr[] = {6, 7, 8, 9, 10};
    deque_push_back_n(&q, arr, 5);
    int expect[] = {4, 5, 6, 7, 8, 9, 10};
    triax_expect_eq(deque_count(&q), 7u);
    for (size_t i = 0; i < 7; ++i) { triax_expect_eq(deque_at(&q, i), expect[i]); }
    deque_release(&q);
  }

  // push_front_n where the write range wraps past physical index 0, and
  // triggers growth in the same call
  {
    Deque(int) q = deque_init(int, 8);
    for (int i = 0; i < 6; ++i) { deque_push_back(&q, i); }
    for (int i = 0; i < 4; ++i) { (void)deque_pop_front(&q); } // logical: 4, 5
    int arr[] = {-1, -2, -3, -4, -5};
    deque_push_front_n(&q, arr, 5);
    int expect[] = {-1, -2, -3, -4, -5, 4, 5};
    triax_expect_eq(deque_count(&q), 7u);
    for (size_t i = 0; i < 7; ++i) { triax_expect_eq(deque_at(&q, i), expect[i]); }
    deque_release(&q);
  }
}

triax_test(deque, foreach_and_reversed) {
  Deque(int) q   = deque_init(int, 0);

  size_t visited = 0;
  deque_foreach(&q, it) {
    (void)it;
    ++visited;
  }
  triax_expect_eq(visited, 0u);

  deque_push_back(&q, 1);
  deque_push_back(&q, 2);
  deque_push_front(&q, 0);
  deque_push_back(&q, 3);
  // logical contents: 0, 1, 2, 3

  int    forward[4] = {0};
  size_t i          = 0;
  deque_foreach(&q, it) { forward[i++] = *it; }
  triax_expect_eq(i, 4u);
  int expect_fwd[] = {0, 1, 2, 3};
  for (size_t j = 0; j < 4; ++j) { triax_expect_eq(forward[j], expect_fwd[j]); }

  int reversed[4] = {0};
  i               = 0;
  deque_foreach_reversed(&q, it) { reversed[i++] = *it; }
  triax_expect_eq(i, 4u);
  int expect_rev[] = {3, 2, 1, 0};
  for (size_t j = 0; j < 4; ++j) { triax_expect_eq(reversed[j], expect_rev[j]); }

  deque_release(&q);
}

// Iterating a const Deque yields const element pointers; a mutable Deque yields mutable ones.
triax_test(deque, foreach_const_deque_yields_const_pointers) {
  Deque(int) q = deque_init(int, 0);
  deque_push_back(&q, 1);
  deque_push_back(&q, 2);
  const Deque(int)* cq  = &q;
  int               sum = 0;
  deque_foreach(cq, it) {
    static_assert(_Generic(it, const int*: 1, default: 0), "const deque must yield const int*");
    sum += *it;
  }
  deque_foreach_reversed(cq, it) {
    static_assert(_Generic(it, const int*: 1, default: 0), "const deque must yield const int*");
    sum += *it;
  }
  deque_foreach(&q, it) {
    static_assert(_Generic(it, int*: 1, default: 0), "mutable deque must yield int*");
    *it += 10;
  }
  triax_expect_eq(sum, 6);
  triax_expect_eq(deque_front(&q), 11);
  deque_release(&q);
}

static Deque(int) * rki_mark_eval_deque(Deque(int) * q, int* count) {
  ++*count;
  return q;
}

triax_test(deque, foreach_break_stops_iteration) {
  Deque(int) q = deque_init(int, 0);
  for (int i = 0; i < 5; ++i) { deque_push_back(&q, i); }
  int sum = 0, visits = 0;
  deque_foreach(&q, it) {
    if (*it == 3) { break; }
    sum += *it;
    ++visits;
  }
  triax_expect_eq(sum, 3); // 0+1+2, stops before 3
  triax_expect_eq(visits, 3);
  deque_release(&q);
}

triax_test(deque, foreach_continue_skips_element) {
  Deque(int) q = deque_init(int, 0);
  for (int i = 0; i < 5; ++i) { deque_push_back(&q, i); }
  int sum = 0, visits = 0;
  deque_foreach(&q, it) {
    if (*it == 2) { continue; }
    sum += *it;
    ++visits;
  }
  triax_expect_eq(sum, 8); // 0+1+3+4
  triax_expect_eq(visits, 4);
  deque_release(&q);
}

triax_test(deque, foreach_reversed_break_stops_iteration) {
  Deque(int) q = deque_init(int, 0);
  for (int i = 0; i < 5; ++i) { deque_push_back(&q, i); }
  int sum = 0, visits = 0;
  deque_foreach_reversed(&q, it) {
    if (*it == 1) { break; }
    sum += *it;
    ++visits;
  }
  triax_expect_eq(sum, 9); // 4+3+2, stops before 1
  triax_expect_eq(visits, 3);
  deque_release(&q);
}

triax_test(deque, foreach_reversed_continue_skips_element) {
  Deque(int) q = deque_init(int, 0);
  for (int i = 0; i < 5; ++i) { deque_push_back(&q, i); }
  int sum = 0, visits = 0;
  deque_foreach_reversed(&q, it) {
    if (*it == 2) { continue; }
    sum += *it;
    ++visits;
  }
  triax_expect_eq(sum, 8); // 4+3+1+0
  triax_expect_eq(visits, 4);
  deque_release(&q);
}

triax_test(deque, foreach_evaluates_deque_argument_once) {
  Deque(int) q = deque_init(int, 0);
  for (int i = 1; i <= 3; ++i) { deque_push_back(&q, i); }
  int evals = 0, sum = 0;
  deque_foreach(rki_mark_eval_deque(&q, &evals), it) { sum += *it; }
  triax_expect_eq(evals, 1);
  triax_expect_eq(sum, 6);
  deque_release(&q);
}

triax_test(deque, foreach_reversed_evaluates_deque_argument_once) {
  Deque(int) q = deque_init(int, 0);
  for (int i = 1; i <= 3; ++i) { deque_push_back(&q, i); }
  int evals = 0, sum = 0;
  deque_foreach_reversed(rki_mark_eval_deque(&q, &evals), it) { sum += *it; }
  triax_expect_eq(evals, 1);
  triax_expect_eq(sum, 6);
  deque_release(&q);
}

triax_test(deque, erase_if_evaluates_deque_argument_once) {
  Deque(int) q = deque_init(int, 0);
  for (int i = 1; i <= 5; ++i) { deque_push_back(&q, i); }
  int evals = 0;
  deque_erase_if(rki_mark_eval_deque(&q, &evals), it, *it % 2 == 0);
  triax_expect_eq(evals, 1);
  triax_expect_eq(deque_count(&q), 3u); // 1, 3, 5 remain
  deque_release(&q);
}

triax_test(deque, erase_if_evaluates_predicate_once_per_element) {
  Deque(int) q = deque_init(int, 0);
  for (int i = 1; i <= 5; ++i) { deque_push_back(&q, i); }
  int pred_calls = 0;
  deque_erase_if(&q, it, (++pred_calls, *it % 2 == 0));
  triax_expect_eq(pred_calls, 5);
  triax_expect_eq(deque_count(&q), 3u);
  deque_release(&q);
}

triax_test(deque, foreach_reversed_empty_is_noop) {
  Deque(int) q = deque_init(int, 0);
  int count    = 0;
  deque_foreach_reversed(&q, it) {
    (void)it;
    ++count;
  }
  triax_expect_eq(count, 0);
  deque_release(&q);
}

triax_test(deque, foreach_on_null_deque_is_noop) {
  Deque(int)* q = rk_null;
  int count     = 0;
  deque_foreach(q, it) {
    (void)it;
    ++count;
  }
  deque_foreach_reversed(q, it) {
    (void)it;
    ++count;
  }
  triax_expect_eq(count, 0);
}

triax_test(deque, erase_if_empty_is_noop) {
  Deque(int) q   = deque_init(int, 0);
  int pred_calls = 0;
  deque_erase_if(&q, it, (++pred_calls, (void)it, true));
  triax_expect_eq(pred_calls, 0);
  triax_expect_eq(deque_count(&q), 0u);
  deque_release(&q);
}

triax_test(deque, erase_if_on_null_deque_is_noop) {
  Deque(int)* q  = rk_null;
  int pred_calls = 0;
  deque_erase_if(q, it, (++pred_calls, (void)it, true));
  triax_expect_eq(pred_calls, 0);
}

triax_test(deque, clear_is_an_expression) {
  Deque(int) q = deque_init(int, 0);
  deque_push_back(&q, 1);
  deque_push_back(&q, 2);

  // deque_clear must be usable as an expression (a real function call under
  // the hood), not just a bare statement -- exercised here via a ternary and
  // a comma expression.
  int cond = 1;
  cond ? (void)deque_clear(&q) : (void)0;
  triax_expect_true(deque_is_empty(&q));

  deque_push_back(&q, 5);
  deque_push_back(&q, 6);
  (deque_clear(&q), (void)0);
  triax_expect_true(deque_is_empty(&q));
  triax_expect_eq(deque_count(&q), 0u);

  // still usable after
  deque_push_back(&q, 9);
  triax_expect_eq(deque_front(&q), 9);

  deque_release(&q);
}

triax_test(deque, allocator_and_cap) {
  Deque(int) q = deque_init(int, 0);
  triax_expect_eq(deque_cap(&q), 0u);

  Allocator a = deque_allocator(&q);
  (void)a; // just verify it's callable and type-checks

  deque_push_back(&q, 1);
  triax_expect_true(deque_cap(&q) >= 8u); // minimum growth capacity
  triax_expect_true(stdc_has_single_bit(deque_cap(&q)));

  deque_release(&q);
}

triax_test(deque, reserve_and_shrink_to_fit) {
  Deque(int) q = deque_init(int, 0);
  for (int i = 0; i < 5; ++i) { deque_push_back(&q, i); }

  deque_reserve(&q, 100);
  triax_expect_true(deque_cap(&q) >= 100u);
  triax_expect_eq(deque_count(&q), 5u);
  for (int i = 0; i < 5; ++i) { triax_expect_eq(deque_at(&q, (size_t)i), i); }

  // reserving at or below the current capacity is a no-op
  size_t cap_before = deque_cap(&q);
  deque_reserve(&q, 4);
  triax_expect_eq(deque_cap(&q), cap_before);

  deque_shrink_to_fit(&q);
  triax_expect_true(deque_cap(&q) < cap_before);
  triax_expect_true(deque_cap(&q) >= deque_count(&q));
  triax_expect_eq(deque_count(&q), 5u);
  for (int i = 0; i < 5; ++i) { triax_expect_eq(deque_at(&q, (size_t)i), i); }

  // shrink_to_fit on an empty deque fully frees the backing buffer
  deque_clear(&q);
  deque_shrink_to_fit(&q);
  triax_expect_eq(deque_cap(&q), 0u);
  triax_expect_true(deque_is_empty(&q));

  // still usable after
  deque_push_back(&q, 42);
  triax_expect_eq(deque_front(&q), 42);

  deque_release(&q);
}

triax_test(deque, assign) {
  // replaces contents, reuses buffer when it already fits
  {
    Deque(int) q = deque_init(int, 0);
    deque_push_back(&q, 1);
    deque_push_back(&q, 2);
    deque_push_front(&q, 0);
    void* before_ptr = q.base.data;

    int  arr[]      = {10, 20, 30};
    deque_assign(&q, arr, 3);
    triax_expect_eq(deque_count(&q), 3u);
    triax_expect_eq(q.base.data, before_ptr); // reused, no reallocation
    for (size_t i = 0; i < 3; ++i) { triax_expect_eq(deque_at(&q, i), arr[i]); }
    deque_release(&q);
  }

  // assign growing beyond current capacity
  {
    Deque(int) q = deque_init(int, 0);
    deque_push_back(&q, 1);
    int big[50];
    for (int i = 0; i < 50; ++i) { big[i] = i; }
    deque_assign(&q, big, 50);
    triax_expect_eq(deque_count(&q), 50u);
    for (size_t i = 0; i < 50; ++i) { triax_expect_eq(deque_at(&q, i), (int)i); }
    deque_release(&q);
  }

  // n == 0 empties the deque
  {
    Deque(int) q = deque_init(int, 0);
    deque_push_back(&q, 1);
    deque_assign(&q, (int*)rk_null, 0);
    triax_expect_true(deque_is_empty(&q));
    deque_release(&q);
  }

  // assigning into an already-wrapped deque (head nonzero) still produces
  // correct front-to-back order
  {
    Deque(int) q = deque_init(int, 8);
    for (int i = 0; i < 6; ++i) { deque_push_back(&q, i); }
    for (int i = 0; i < 4; ++i) { (void)deque_pop_front(&q); }
    int arr[] = {100, 200};
    deque_assign(&q, arr, 2);
    triax_expect_eq(deque_count(&q), 2u);
    triax_expect_eq(deque_at(&q, 0), 100);
    triax_expect_eq(deque_at(&q, 1), 200);
    deque_release(&q);
  }
}

triax_test(deque, large_randomized_stress) {
  srand(4242);
  for (int trial = 0; trial < 200; ++trial) {
    Deque(int) q = deque_init(int, 0);
    enum { REF_CAP = 8192 };
    int ref[REF_CAP];
    int start = REF_CAP / 2, len = 0, next_val = 0;

    int ops = rand() % 400;
    for (int op = 0; op < ops; ++op) {
      int choice = rand() % 8;
      if (choice == 0) {
        int v = next_val++;
        deque_push_back(&q, v);
        ref[start + len] = v;
        ++len;
      } else if (choice == 1) {
        int v = next_val++;
        deque_push_front(&q, v);
        --start;
        ref[start] = v;
        ++len;
      } else if (choice == 2 && len > 0) {
        int v = deque_pop_front(&q);
        triax_assert_eq(v, ref[start]);
        ++start;
        --len;
      } else if (choice == 3 && len > 0) {
        int v = deque_pop_back(&q);
        triax_assert_eq(v, ref[start + len - 1]);
        --len;
      } else if (choice == 4) {
        deque_shrink_to_fit(&q);
      } else if (choice == 5) {
        deque_reserve(&q, (size_t)(rand() % 200));
      } else if (choice == 6) {
        int n = rand() % 6;
        int arr[6];
        for (int i = 0; i < n; ++i) { arr[i] = next_val++; }
        deque_push_back_n(&q, arr, (size_t)n);
        for (int i = 0; i < n; ++i) { ref[start + len + i] = arr[i]; }
        len += n;
      } else if (choice == 7) {
        int n = rand() % 6;
        int arr[6];
        for (int i = 0; i < n; ++i) { arr[i] = next_val++; }
        deque_push_front_n(&q, arr, (size_t)n);
        start -= n;
        for (int i = 0; i < n; ++i) { ref[start + i] = arr[i]; }
        len += n;
      }
      size_t len_u = (size_t)len;
      triax_assert_eq(deque_count(&q), len_u);
    }

    size_t len_u = (size_t)len;
    triax_assert_eq(deque_count(&q), len_u);
    for (int i = 0; i < len; ++i) {
      int* p = deque_peek_at(&q, (size_t)i);
      triax_assert_nonnull(p);
      triax_assert_eq(*p, ref[start + i]);
    }
    size_t fi = 0;
    deque_foreach(&q, it) {
      triax_assert_eq(*it, ref[start + (int)fi]);
      ++fi;
    }
    triax_assert_eq(fi, len_u);

    deque_release(&q);
  }
}

RKI_IGNWARN_CLANG_END()
RKI_HEADER_END
#endif
