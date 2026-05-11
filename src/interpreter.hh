#pragma once

struct Interpreter {
  EnvManager<ValueIndex> envs;
  ValueStore             values;
  Arena                  work_arena;
  Env<ValueIndex>       *env_root;
  SourceUnit            *source;

  struct {
    ValueIndex nil;
  } common;

  void init();
  void deinit();

  bool load_root(SourceUnit *source);
  bool run_main(ValueIndex *result);

  OptionalValueIndex _lookup(Env<ValueIndex> *env, Str identifier) {
    ValueIndex idx;
    b32        found = env->lookup(source->strings.add(identifier), &idx);
    if (!found) {
      return OptionalValueIndex::none();
    }

    return OptionalValueIndex::from_index(idx);
  }

  TypeIndex get_type(NodeIndex node_index);

  b32 call_function(
    Env<ValueIndex> *env, Value *function, Slice<ValueIndex> arguments, ValueIndex *result
  );
  b32 eval_expr(Env<ValueIndex> *env, NodeIndex node_index, ValueIndex *result);
  b32 eval_binary_op(
    BinaryOpKind op, ValueIndex lhs, ValueIndex rhs, NodeIndex expr, ValueIndex *result
  );

  void builtin_print(Str format, Slice<ValueIndex> args);

  ValueIndex lookup_identifier(Env<ValueIndex> *env, NodeIndex identifier);

  b32  add_declaration(Env<ValueIndex> *env, NodeIndex declaration);
  bool coerce_value(TypeIndex type_dst, ValueIndex src, void *out);

  u64 get_uint(ValueIndex idx);

  // Can read any signed integer type and convert it to i64.
  i64 get_as_i64(ValueIndex idx) {
    i64 res;
    coerce_value(source->types.type.i64_, idx, &res);
    return res;
  }

  // Can read any unsigned integer type and convert it to u64.
  u64 get_as_u64(ValueIndex idx) {
    i64 res;
    coerce_value(source->types.type.u64_, idx, &res);
    return res;
  }

  void populate_root_env(Env<ValueIndex> *env);

  Str get_token_str(TokenIndex idx) { return ::get_token_str(source->text, &source->tokens, idx); }
};
