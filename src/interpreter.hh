#pragma once

struct InterpreterContext {
  TypeInterner   *types;
  StringInterner *strings;
  Messages       *messages;

  Str              text;
  Tokens          *tokens;
  AstNodes        *nodes;
  Slice<TypeIndex> node_types;
};

struct Interpreter {
  EnvManager<ValueIndex> envs;
  ValueStore             values;
  Arena                  work_arena;
  Env<ValueIndex>       *env_root;

  // Externally owned
  TypeInterner    *types;
  StringInterner  *strings;
  Messages        *messages;
  Str              text;
  Tokens          *tokens;
  AstNodes        *nodes;
  Slice<TypeIndex> node_types;

  struct {
    ValueIndex nil;
  } common;

  void init();
  void deinit();

  // `run_const_code` runs const code and sets up the root environment.
  // this is to simulate compilation of the code.
  // const values are saved and used when the program is run.
  bool run_const_code(InterpreterContext *context);

  bool run_main(ValueIndex *result);

  OptionalValueIndex _lookup(Env<ValueIndex> *env, Str identifier) {
    ValueIndex idx;
    b32        found = env->lookup(strings->add(identifier), &idx);
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
  b32 const_walk(Env<ValueIndex> *env, NodeIndex *slot);
  b32 eval_binary_op(
    BinaryOpKind op, ValueIndex lhs, ValueIndex rhs, NodeIndex expr, ValueIndex *result
  );

  void builtin_print(Str format, Slice<ValueIndex> args);

  ValueIndex lookup_identifier(Env<ValueIndex> *env, NodeIndex identifier);

  b32 eval_place(Env<ValueIndex> *env, NodeIndex node, void **out_ptr, TypeIndex *out_type);
  b32 eval_cast(TypeIndex type_dst, ValueIndex val, ValueIndex *result);

  b32  add_declaration(Env<ValueIndex> *env, NodeIndex declaration);
  bool coerce_value(TypeIndex type_dst, ValueIndex src, void *out);
  b32  coerce_slot_to(NodeIndex *slot, TypeIndex dst_type);
  TypeIndex slot_type(NodeIndex slot);

  u64 get_uint(ValueIndex idx);

  // Can read any signed integer type and convert it to i64.
  i64 get_as_i64(ValueIndex idx) {
    i64 res;
    coerce_value(types->type.i64_, idx, &res);
    return res;
  }

  // Can read any unsigned integer type and convert it to u64.
  u64 get_as_u64(ValueIndex idx) {
    i64 res;
    coerce_value(types->type.u64_, idx, &res);
    return res;
  }

  void populate_root_env(Env<ValueIndex> *env);

  Str get_token_str(TokenIndex idx) { return ::get_token_str(text, tokens, idx); }
};
