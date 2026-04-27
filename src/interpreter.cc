#include "blu.hh"

void Interpreter::init(
  StringInterner *strings,
  TypeInterner *types,
  ValueStore *values,
  EnvManager *envs,
  Arena *work_arena
) {
  this->strings    = strings;
  this->types      = types;
  this->values     = values;
  this->envs       = envs;
  this->work_arena = work_arena;

  common.nil = values->add({
    .kind = Val_type,
.type = types->type.type,
    .data = { .type = types->type.nil },
  });
}

b32 Interpreter::intern_type(Env *env, NodeIndex node_index, TypeIndex *result) {
  auto kind = source->nodes->kind(node_index);
  switch (kind) {
  case Ast_type_function: {
    auto f = source->nodes->data(node_index).type_function;

    u32 param_count = f.param_types.len();
    auto ty = alloc_type_function(work_arena, param_count);

    *ty = {
      .kind = Type_function,
      .function = { .param_count = param_count, },
    };

    Try(intern_type(env, f.return_type, &ty->function.return_type));

    for (u32 i = 0; i < param_count; i++) {
      Try(intern_type(env, f.param_types[i], &ty->function.param_types[i]));
    }

    *result = types->add(ty);
  } break;
  case Ast_identifier: {
    auto str = source->get_token_str(source->nodes->data(node_index).identifier.token_index);
    auto val = values->get(_lookup(env, str).as_index());
    Assert(val->kind == Val_type); 
    *result = val->data.type;
  } break;
  default: Todo(); break;
  }

  return true;
}

b32 Interpreter::run(Source *source, ValueIndex *result) {
  this->source = source;

  auto nodes = source->nodes;

  auto env_global = envs->create_global_env(strings, types, values);
  auto env = envs->alloc(env_global);
  defer({
    envs->dealloc(env_global);
    envs->dealloc(env);
  });

  NodeIndex root_idx = {0};
  Assert(nodes->kind(root_idx) == Ast_root);

  auto root = nodes->data(root_idx);
  for (u32 i = 0; i < root.root.items.len(); i++) {
    auto item_idx = root.root.items[i];

    auto decl = nodes->data(item_idx).declaration;

    TypeIndex decl_type;
    Try(intern_type(env, decl.type, &decl_type));

    ValueIndex decl_value;
    Try(eval_expr(env, decl.value, &decl_value));

    // TODO ensure that the type of the value is coercible to the declared type.

    auto str = source->get_token_str(decl.name);
    auto key = strings->add(str);
    env->insert(key, decl_value);
  }

  auto main = values->get(_lookup(env, Str_make("main")).as_index());

  TypeIndex main_function_type;
  {
    Type *t = alloc_type_function(work_arena, 0);
    *t = {
      .kind = Type_function,
      .function = {
        .return_type = types->type.i32_,
        .param_count = 0,
      },
    };
    main_function_type = types->add(t);
  }

  Assert(types->is_coercible_to(main->type, main_function_type));

  Try(call_function(env, main, {}, result));

  return true;
}

b32 Interpreter::eval_expr(Env *env, NodeIndex node_index, ValueIndex *result) {
  auto kind = source->nodes->kind(node_index);

  switch (kind) {
  case Ast_literal_int: {
    auto token_index = source->nodes->data(node_index).literal_int.token_index;
    auto str = source->get_token_str(token_index);
    auto i = parse_i64(str);
    *result = values->add({
        .kind = Val_int,
        .type = types->type.literal_int,
        .data = { .int64 = i, },
      });
  } break;
  case Ast_block: {
    auto block = source->nodes->data(node_index).block;
    if (block.items.len() == 0) {
      *result = common.nil;
      break;
    }

    auto env_block = envs->alloc(env);
    defer(envs->dealloc(env_block));

    for (u32 i = 0; i < block.items.len() - 1; i++) {
      ValueIndex e;
      Try(eval_expr(env_block, block.items[i], &e));
    }

    Try(eval_expr(env_block, block.items[block.items.len()-1], result));
  } break;
  case Ast_function: {
    *result = values->add({
      .kind = Val_function,
      .type = types->type.literal_function,
      .data = { .node_index = node_index },
    });
  } break;
  default: Todo(); break;
  }

  return true;
}

b32 Interpreter::call_function(Env *env, Value *function, Slice<ValueIndex> arguments, ValueIndex *result) {
  // TODO: make sure the arguments match the expected parameters
  // TODO: add parameters to env

  auto function_node = source->nodes->data(function->data.node_index).function;
  Try(eval_expr(env, function_node.body, result));

  return true;
}
