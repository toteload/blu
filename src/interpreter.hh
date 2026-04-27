#pragma once

struct Interpreter {
  StringInterner *strings;
  TypeInterner *types;
  ValueStore *values;
  EnvManager *envs;
  Arena *work_arena;

  Source *source;

  struct {
    ValueIndex nil;
  } common;

  void init(
    StringInterner *strings,
    TypeInterner *types,
    ValueStore *values,
    EnvManager *envs,
    Arena *work_arena
  );
  void deinit();

  b32 run(Source *source, ValueIndex *result);

  OptionalValueIndex _lookup(Env *env, Str identifier) {
    return env->lookup(strings->add(identifier));
  }

  b32 intern_type(Env *env, NodeIndex node_index, TypeIndex *result);

  b32 call_function(Env *env, Value *function, Slice<ValueIndex> arguments, ValueIndex *result);
  b32 eval_expr(Env *env, NodeIndex node_index, ValueIndex *result);
};
