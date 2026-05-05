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

  ValueIndex lookup_identifier(Env<ValueIndex> *env, NodeIndex identifier);

  b32 add_declaration(Env<ValueIndex> *env, NodeIndex declaration);
  void coerce_value(TypeIndex type_dst, ValueIndex src, void *out);

  u64 get_uint(ValueIndex idx);

  // Can read any signed integer type and convert it to i64.
  i64 get_as_i64(ValueIndex idx);

  // Can read any unsigned integer type and convert it to u64.
  u64 get_as_u64(ValueIndex idx);

  void populate_root_env(Env<ValueIndex> *env);
};
