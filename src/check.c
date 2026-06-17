#include "blu.h"
#include "messages.h"
#include "string_interner.h"
#include "tokens.h"
#include "ast.h"
#include "types.h"
#include "env.h"

typedef struct {
  String   filename;
  String   source;
  AstNodes ast;
  Tokens   tokens;
} SourceFile;

// 1. Read, tokenize and parse all required files.
// 2. Canonicalize all identifiers and built up one map for the whole program.
//   - After this step either an error is thrown because some identifier cannot be resolved OR
//     each identifier can be found somewhere in the program.
// 3. For each declaration in the program: perform semantic analysis and generate code.

typedef struct {
  Messages       *messages;
  StringInterner *strings;
  TypeInterner   *types;
  EnvAllocator   *envs;

  AstNodes       *nodes;
  Tokens         *tokens;

  Env *env_builtin;
  Env *env_root;

  TypeIndex ty_i8;
  TypeIndex ty_i16;
  TypeIndex ty_i32;
  TypeIndex ty_i64;
  TypeIndex ty_u8;
  TypeIndex ty_u16;
  TypeIndex ty_u32;
  TypeIndex ty_u64;
  TypeIndex ty_type;
} Checker;

internal b32 check_and_eval_comptime_code(Checker *checker);
internal b32 check_mod_section(Checker *checker, NodeIndex idx);

internal ValueIndex alloc_type(Checker *checker, TypeIndex type) {
  Value *val;
  ValueIndex idx = values_alloc(checker->values, &val);
  void *data = values_alloc_data(checker->values, sizeof(TypeIndex));

  memcpy(data, &type, sizeof(TypeIndex));

  *val = (Value){
    .type = checker->ty_type,
    .data = data,
  };

  return idx;
}

internal void env_insert_builtin_type(Checker *checker, Env *env, String s, TypeIndex type) {
  ValueIndex value = alloc_type(checker, type);

  Declaration decl = {
    .resolve_status = ResolveStatus_resolved,
    .scope = Scope_builtin,
    .attributes = 0,
    .ast_index = 0,
    .value = value,
  };

  StringIndex idx = strings_add(checker->strings, s);

  env_insert(env, idx, decl);
}

internal void env_populate_with_builtins(Checker *checker, Env *env) {
  env_insert_builtin_type(checker, env, string_lit("i8"),  checker->ty_i8);
  env_insert_builtin_type(checker, env, string_lit("i16"), checker->ty_i16));
}

b32 check_and_eval_comptime_code(Checker *checker) {
  AstIndex idx_root = nodes_begin(checker->nodes);

  Assert(*nodes_kind(checker->nodes, idx_root) == Ast_root);

  for (u32 i = 0; ;) {
    AstIndex idx_mod_section = *list_ptr_at_unchecked();
    Try(check_mod_section(checker, idx_mod_section));
  }

  return True;
}

b32 check_mod_section(Checker *checker) {
  for () {

  }

  return True;
}
