#pragma once

struct Interpreter {
  StringInterner *strings;
  TypeInterner *types;
  ValueStore *values;
  EnvManager<ValueIndex> *envs;
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
    EnvManager<ValueIndex> *envs,
    Arena *work_arena,
    Messages *messages
  );
  void deinit();

  b32 run(Source *source, ValueIndex *result);

  OptionalValueIndex _lookup(Env<ValueIndex> *env, Str identifier) {
    ValueIndex idx;
    b32 found = env->lookup(strings->add(identifier), &idx);
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

  b32 check_is_of_type(ValueIndex e, TypeIndex expected_type, NodeIndex location);
  ValueIndex lookup_identifier(Env<ValueIndex> *env, NodeIndex identifier);

  b32 get_int_value(ValueIndex v, i64 *i);

  b32 add_declaration(Env<ValueIndex> *env, NodeIndex declaration);
  ValueIndex coerce_value(TypeIndex type_dst, ValueIndex src);

  void populate_root_env(Env<ValueIndex> *env);
};
