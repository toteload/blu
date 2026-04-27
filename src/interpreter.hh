#pragma once

struct Interpreter {
  StringInterner *strings;
  TypeInterner *types;
  ValueStore *values;
  EnvManager *envs;
  Arena *work_arena;
  Messages *messages;

  Source *source;

  struct {
    ValueIndex nil;
  } common;

  void init(
    StringInterner *strings,
    TypeInterner *types,
    ValueStore *values,
    EnvManager *envs,
    Arena *work_arena,
    Messages *messages
  );
  void deinit();

  b32 run(Source *source, ValueIndex *result);

  OptionalValueIndex _lookup(Env *env, Str identifier) {
    return env->lookup(strings->add(identifier));
  }

  b32 intern_type(Env *env, NodeIndex node_index, TypeIndex *result);

  b32 call_function(Env *env, Value *function, Slice<ValueIndex> arguments, ValueIndex *result);
  b32 eval_expr(Env *env, NodeIndex node_index, ValueIndex *result);

  b32 check_is_of_type(ValueIndex e, TypeIndex expected_type, NodeIndex location);
  b32 check_lookup_identifier(Env *env, NodeIndex identifier, ValueIndex *value);

  b32 eval_binary_op(BinaryOpKind op, ValueIndex lhs, ValueIndex rhs, NodeIndex expr, ValueIndex *result);

  StrKey intern_identifier(TokenIndex identifier);

  b32 add_declaration(Env *env, NodeIndex declaration);
};
