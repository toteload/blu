#pragma once

struct Interpreter {
  StringInterner *strings;
  TypeInterner *types;
  ValueStore *values;
  EnvManager *envs;
  Arena *work_arena;
  Messages *messages;

  Source *source;
  Slice<TypeIndex> type_annotations;

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

  TypeIndex get_type(NodeIndex node_index);

  b32 call_function(Env *env, Value *function, Slice<ValueIndex> arguments, ValueIndex *result);
  b32 eval_expr(Env *env, NodeIndex node_index, ValueIndex *result);
  b32 eval_binary_op(
    BinaryOpKind op, ValueIndex lhs, ValueIndex rhs, NodeIndex expr, ValueIndex *result
  );

  b32 check_is_const(Env *env, NodeIndex expression);
  b32 check_is_of_type(ValueIndex e, TypeIndex expected_type, NodeIndex location);
  ValueIndex lookup_identifier(Env *env, NodeIndex identifier);

  b32 get_int_value(ValueIndex v, i64 *i);

  b32 add_declaration(Env *env, NodeIndex declaration);
};
