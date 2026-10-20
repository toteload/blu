#include "toteload.h"
#include "blu.h"
#include "symbol_map.h"
#include "test.h"

#include <stdlib.h>

internal void *test_alloc_fn(void *ctx, void *p, usize old_byte_size, usize new_byte_size, u32 align) {
  Unused(ctx, old_byte_size, align);

  if (!is_null(p) && new_byte_size == 0) {
    free(p);
    return Null;
  }

  return realloc(p, new_byte_size);
}

// StringIndex 0 means "no name", so test names start at 1.
enum { X = 1, Y, Z, LABEL };

internal void test_symbol_map(TestResult *test, void *user) {
  Unused(user);

  Arena scratch;
  arena_init(&scratch, &(ArenaOptions){ .reserve_size = MiB(1), .initial_commit_size = KiB(64) });

  SymbolMap syms;
  symbols_init(&syms, (Allocator){ .fn = test_alloc_fn }, &scratch);

  Test_assert(is_null(symbols_find(&syms, X)));
  Test_assert(is_null(symbols_get_enclosing_loop(&syms)));

  // Function with a parameter.
  SymbolsPushScopeResult fn = symbols_push_scope(&syms, ScopeEntry_function, 10, 0);
  Test_assert_eq(fn.err, SYMBOLS_OK);
  Test_assert_eq(symbols_push_local(&syms, ScopeEntry_param, X, 11).err, SYMBOLS_OK);

  ScopeEntry *e = symbols_find(&syms, X);
  Test_assert(!is_null(e));
  Test_assert_eq(e->kind, ScopeEntry_param);
  Test_assert_eq(e->inst, 11);

  // Outer block: a local, then shadow it at the same level.
  SymbolsPushScopeResult outer = symbols_push_scope(&syms, ScopeEntry_block, 20, 0);
  Test_assert_eq(outer.err, SYMBOLS_OK);
  Test_assert_eq(symbols_push_local(&syms, ScopeEntry_local, Y, 21).err, SYMBOLS_OK);
  Test_assert_eq(symbols_push_local(&syms, ScopeEntry_local, Y, 22).err, SYMBOLS_OK);
  Test_assert_eq(symbols_find(&syms, Y)->inst, 22);

  // Shadowing a symbol from an enclosing scope is not allowed.
  SymbolsPushLocalResult dup = symbols_push_local(&syms, ScopeEntry_local, X, 23);
  Test_assert_eq(dup.err, SYMBOLS_ERR_FOUND_EXISTING_SYMBOL);
  Test_assert_eq(dup.inst, 11);

  // Labeled loop.
  SymbolsPushScopeResult loop = symbols_push_scope(&syms, ScopeEntry_loop, 30, LABEL);
  Test_assert_eq(loop.err, SYMBOLS_OK);
  e = symbols_get_enclosing_loop(&syms);
  Test_assert(!is_null(e));
  Test_assert_eq(e->inst, 30);
  Test_assert_eq(symbols_find(&syms, LABEL)->inst, 30);
  Test_assert_eq(symbols_push_local(&syms, ScopeEntry_local, Z, 31).err, SYMBOLS_OK);

  // The loop stays the enclosing loop inside a nested block.
  SymbolsPushScopeResult inner = symbols_push_scope(&syms, ScopeEntry_block, 40, 0);
  Test_assert_eq(inner.err, SYMBOLS_OK);
  Test_assert_eq(symbols_get_enclosing_loop(&syms)->inst, 30);

  // Reusing a label from an enclosing scope fails.
  SymbolsPushScopeResult dup_label = symbols_push_scope(&syms, ScopeEntry_block, 41, LABEL);
  Test_assert_eq(dup_label.err, SYMBOLS_ERR_FOUND_EXISTING_SYMBOL);
  Test_assert_eq(dup_label.data.inst, 30);

  symbols_pop_scope(&syms, inner.data.scope_key);
  Test_assert_eq(symbols_get_enclosing_loop(&syms)->inst, 30);

  // Popping the loop removes its label and locals.
  symbols_pop_scope(&syms, loop.data.scope_key);
  Test_assert(is_null(symbols_get_enclosing_loop(&syms)));
  Test_assert(is_null(symbols_find(&syms, LABEL)));
  Test_assert(is_null(symbols_find(&syms, Z)));
  Test_assert_eq(symbols_find(&syms, Y)->inst, 22);

  // Popping the block removes both shadowed locals.
  symbols_pop_scope(&syms, outer.data.scope_key);
  Test_assert(is_null(symbols_find(&syms, Y)));
  Test_assert_eq(symbols_find(&syms, X)->inst, 11);

  // A name freed by popping can be reused.
  SymbolsPushScopeResult again = symbols_push_scope(&syms, ScopeEntry_block, 50, 0);
  Test_assert_eq(symbols_push_local(&syms, ScopeEntry_local, Y, 51).err, SYMBOLS_OK);
  Test_assert_eq(symbols_find(&syms, Y)->inst, 51);
  symbols_pop_scope(&syms, again.data.scope_key);

  // Popping the function removes its parameters.
  symbols_pop_scope(&syms, fn.data.scope_key);
  Test_assert(is_null(symbols_find(&syms, X)));

  symbols_deinit(&syms);
  arena_deinit(&scratch);
}

void register_symbol_map_tests(TestRunner *runner) {
  test_runner_register_test(runner, string_lit("test_symbol_map"), test_symbol_map, Null);
}
