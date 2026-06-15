#include "blu.h"

typedef struct {
  Messages       *messages;
  StringInterner *strings;
  TypeInterner   *types;
  AstNodes       *nodes;
  Tokens         *tokens;

  Env *env_builtin;
  Env *env_root;
} Checker;

b32 check_and_eval_comptime_code(Checker *checker) {
  AstIndex idx_root = nodes_begin(checker->nodes);

  Assert(*nodes_kind(check->nodes, idx_root) == Ast_root);

  for (u32 i = 0; ;) {
    AstIndex idx_mod_section = *list_ptr_at_unchecked();
    Try(check_mod_section(checker, idx_mod_section));
  }
}
