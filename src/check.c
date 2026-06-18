#include "blu.h"
#include "messages.h"
#include "string_interner.h"
#include "tokens.h"
#include "ast.h"
#include "types.h"
#include "env.h"
#include "check.h"
#include "value.h"

// 1. Read, tokenize and parse all required files.
// 2. Canonicalize all identifiers and built up one map for the whole program.
//   - After this step either an error is thrown because some identifier cannot be resolved OR
//     each identifier can be found somewhere in the program.
// 3. For each declaration in the program: perform semantic analysis and generate code.

internal ValueIndex alloc_type_value(Checker *checker, TypeIndex type);
internal ValueIndex alloc_value(Checker *checker, Value **val);
internal void *alloc_value_data(Checker *checker, usize size, u32 align);
internal void env_insert_builtin_type(Checker *checker, Env *env, String s, TypeIndex type);
internal b32 check_source_file(Checker *checker, SourceFile *source);

internal void env_insert_builtin_type(Checker *checker, Env *env, String s, TypeIndex type) {
  ValueIndex value = alloc_type_value(checker, type);

  Declaration decl = {
    .resolve_status = ResolveStatus_resolved,
    .scope          = Scope_builtin,
    .attributes     = 0,
    .ast_index      = 0,
    .value          = value,
  };

  StringIndex idx = strings_add(checker->strings, s);

  env_insert(env, idx, decl);
}

void checker_init(Checker *checker, CheckerOptions *options) {
  checker->messages = options->messages;
  checker->strings = options->strings;
  checker->types = options->types;
  checker->envs = options->envs;
  checker->values = options->values;
  checker->source_count = options->source_count;
  checker->sources = options->sources;

  checker->env_root = envs_alloc(checker->envs, Null);

  // clang-format off
  checker->ty_i8  = types_add(checker->types, &(Type){ .kind = Type_integer, .data.integer = { .signedness = Signed,   .bitwidth =  8 } });
  checker->ty_i16 = types_add(checker->types, &(Type){ .kind = Type_integer, .data.integer = { .signedness = Signed,   .bitwidth = 16 } });
  checker->ty_i32 = types_add(checker->types, &(Type){ .kind = Type_integer, .data.integer = { .signedness = Signed,   .bitwidth = 32 } });
  checker->ty_i64 = types_add(checker->types, &(Type){ .kind = Type_integer, .data.integer = { .signedness = Signed,   .bitwidth = 64 } });

  checker->ty_u8  = types_add(checker->types, &(Type){ .kind = Type_integer, .data.integer = { .signedness = Unsigned, .bitwidth =  8 } });
  checker->ty_u16 = types_add(checker->types, &(Type){ .kind = Type_integer, .data.integer = { .signedness = Unsigned, .bitwidth = 16 } });
  checker->ty_u32 = types_add(checker->types, &(Type){ .kind = Type_integer, .data.integer = { .signedness = Unsigned, .bitwidth = 32 } });
  checker->ty_u64 = types_add(checker->types, &(Type){ .kind = Type_integer, .data.integer = { .signedness = Unsigned, .bitwidth = 64 } });

  checker->ty_type = types_add(checker->types, &(Type){ .kind = Type_type });

  env_insert_builtin_type(checker, checker->env_root, string_lit("i8"),  checker->ty_i8);
  env_insert_builtin_type(checker, checker->env_root, string_lit("i16"), checker->ty_i16);
  env_insert_builtin_type(checker, checker->env_root, string_lit("i32"), checker->ty_i32);
  env_insert_builtin_type(checker, checker->env_root, string_lit("i64"), checker->ty_i64);

  env_insert_builtin_type(checker, checker->env_root, string_lit("u8"),  checker->ty_u8);
  env_insert_builtin_type(checker, checker->env_root, string_lit("u16"), checker->ty_u16);
  env_insert_builtin_type(checker, checker->env_root, string_lit("u32"), checker->ty_u32);
  env_insert_builtin_type(checker, checker->env_root, string_lit("u64"), checker->ty_u64);

  env_insert_builtin_type(checker, checker->env_root, string_lit("type"), checker->ty_type);
  // clang-format on
}

b32 check_code(Checker *checker) {
  b32 ok = False;
  for (u32 i = 0; i < checker->source_count; i++) {
    ok = check_source_file(checker, &checker->sources[i]);
    if (!ok) {
      return False;
    }
  }

  return True;
}

internal b32 check_source_file(Checker *checker, SourceFile *source) {
  Panic();
  return True;
}

internal ValueIndex alloc_value(Checker *checker, Value **val) {
}

internal void *alloc_value_data(Checker *checker, usize size, u32 align) {
}

internal ValueIndex alloc_type_value(Checker *checker, TypeIndex type) {
  Value *val;
  ValueIndex idx = alloc_value(checker, &val);
  void *data = alloc_value_data(checker, sizeof(TypeIndex), Align_of(TypeIndex));

  memcpy(data, &type, sizeof(TypeIndex));

  *val = (Value){
    .type = checker->ty_type,
    .data = data,
  };

  return idx;
}
