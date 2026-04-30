#include "blu.hh"

struct ExpectedType {
  TypeIndex type;
  NodeIndex location;
};

struct TypeChecker {
  Messages *messages;
  EnvManager *envs;
  TypeInterner *types;
  StringInterner *strings;
  ValueStore *values;
  Arena *work_arena;
  Source *source;

  b32 typecheck();

  b32 eval_type_expression(Env *env, NodeIndex node_index, TypeIndex *result);
  b32 check_expression(Env *env, NodeIndex node_index, ExpectedType *hint, TypeIndex *result);

  b32 check_coercion(NodeIndex location, TypeIndex type_src, TypeIndex type_dst);

  b32 check_is_type(TypeIndex type, NodeIndex at);

  StrKey intern_identifier(TokenIndex identifier);

  b32 find_identifier(Env *env, TokenIndex identifier, ValueIndex *result);
};

b32 typecheck(TypeCheckContext *context, Source *source) {
  TypeChecker checker = {
    .messages   = context->messages,
    .envs       = context->envs,
    .types      = context->types,
    .strings    = context->strings,
    .values     = context->values,
    .work_arena = context->work_arena,
    .source     = source,
  };

  return checker.typecheck();
}

b32 TypeChecker::typecheck() {
  auto env_global = envs->create_global_env(strings, types, values);
  auto env        = envs->alloc(env_global);
  defer({
    envs->dealloc(env_global);
    envs->dealloc(env);
  });

  NodeIndex root_idx = {0};
  Assert(source->nodes->kind(root_idx) == Ast_root);

  // Add all root level declarations to the environment.
  auto root = source->nodes->data(root_idx).root;
  for (u32 i = 0; i < root.items.len(); i++) {
    Assert(source->nodes->kind(root.items[i]) == Ast_declaration);

    auto decl = source->nodes->data(root.items[i]).declaration;

    auto key = intern_identifier(decl.name);

    env->insert(
      key,
      values->add({
        .kind = Value_declaration,
        .data = {
          .node_index = root.items[i],
        },
      })
    );
  }

  for (u32 i = 0; i < root.items.len(); i++) {
    auto decl = source->nodes->data(root.items[i]).declaration;

    TypeIndex declared_type;
    Try(eval_type_expression(env, decl.type, &declared_type));

    ExpectedType expect_type = {
      .type     = declared_type,
      .location = decl.type,
    };

    TypeIndex value_type;
    Try(check_expression(env, decl.value, &expect_type, &value_type));

    Try(check_coercion(decl.value, value_type, declared_type));
  }

  return true;
}

b32 TypeChecker::eval_type_expression(Env *env, NodeIndex node_index, TypeIndex *result) {
  auto kind = source->nodes->kind(node_index);
  switch (kind) {
  case Ast_type_function: {
    auto f = source->nodes->data(node_index).type_function;

    u32 param_count = f.param_types.len();
    auto ty         = alloc_type_function(work_arena, param_count);

    *ty = {
      .kind     = Type_function,
      .function = {
        .param_count = param_count,
      },
    };

    Try(eval_type_expression(env, f.return_type, &ty->function.return_type));

    for (u32 i = 0; i < param_count; i++) {
      Try(eval_type_expression(env, f.param_types[i], &ty->function.param_types[i]));
    }

    *result = types->add(ty);
  } break;
  case Ast_identifier: {
    auto token_index = source->nodes->data(node_index).identifier.token_index;
    ValueIndex val_idx;
    Try(find_identifier(env, token_index, &val_idx));
    auto val = values->get(val_idx);
    Try(check_is_type(val->type, node_index));
    *result = val->data.type;
  } break;
  default:
    Todo();
    break;
  }

  return true;
}

b32 TypeChecker::check_expression(
  Env *env, NodeIndex node_index, ExpectedType *hint, TypeIndex *result
) {
  auto kind = source->nodes->kind(node_index);

  switch (kind) {
  case Ast_type_function: {
    *result = types->type.type;
  } break;
  case Ast_literal_int: {
    *result = types->type.literal_int;
  } break;
  case Ast_identifier: {
    auto token_index = source->nodes->data(node_index).identifier.token_index;
    ValueIndex val_idx;
    Try(find_identifier(env, token_index, &val_idx));
    auto val = values->get(val_idx);
    *result  = val->type;
  } break;
  case Ast_function: {
    auto f = source->nodes->data(node_index).function;
    Assert(f.param_names.len() == 0);

    TypeIndex body_type;
    Try(check_expression(env, f.body, nullptr, &body_type));

    Type function_type = {
      .kind             = Type_literal_function,
      .literal_function = {
        .return_type = body_type,
        .param_count = cast<u32>(f.param_names.len()),
      },
    };
    *result = types->add(&function_type);
  } break;
  case Ast_block: {
    auto block = source->nodes->data(node_index).block;
    if (block.items.len() == 0) {
      *result = types->type.nil;
      break;
    }

    auto env_block = envs->alloc(env);
    defer(envs->dealloc(env_block));

    for (u32 i = 0; i < block.items.len() - 1; i++) {
      TypeIndex e;
      Try(check_expression(env_block, block.items[i], nullptr, &e));
    }

    Try(check_expression(env_block, block.items[block.items.len() - 1], nullptr, result));
  } break;
  case Ast_if_else: {
    auto if_else = source->nodes->data(node_index).if_else;

    TypeIndex cond_type;
    Try(check_expression(env, if_else.cond, nullptr, &cond_type));
    Try(check_coercion(if_else.cond, cond_type, types->type.bool_));

    TypeIndex then_type;
    Try(check_expression(env, if_else.then, nullptr, &then_type));

    if (if_else.otherwise.is_none()) {
      *result = types->type.nil;
      break;
    }

    TypeIndex otherwise_type;
    Try(check_expression(env, if_else.otherwise.as_index(), nullptr, &otherwise_type));

    TypeIndex final_type;
    if (types->is_coercible_to(then_type, otherwise_type)) {
      final_type = otherwise_type;
    } else if (types->is_coercible_to(otherwise_type, then_type)) {
      final_type = then_type;
    } else {
      Todo();
    }

    *result = final_type;
  } break;
  default:
    Todo();
    break;
  }

  return true;
}

b32 TypeChecker::check_coercion(NodeIndex location, TypeIndex type_src, TypeIndex type_dst) {
  if (types->is_coercible_to(type_src, type_dst)) {
    return true;
  }

  messages->error(location, "Cannot coerce type {type} to {type}.", type_src, type_dst);

  return false;
}

b32 TypeChecker::check_is_type(TypeIndex type, NodeIndex at) {
  if (type == types->type.type) {
    return true;
  }

  messages->error(at, "Expected type");

  return false;
}

StrKey TypeChecker::intern_identifier(TokenIndex identifier) {
  Str s = source->get_token_str(identifier);
  return strings->add(s);
}

b32 TypeChecker::find_identifier(Env *env, TokenIndex identifier, ValueIndex *result) {
  auto key = intern_identifier(identifier);
  auto idx = env->lookup(key);
  if (!idx.is_some()) {
    messages->error(identifier, "Could not find identifier '{strkey}'.", key);
    return false;
  }

  *result = idx.as_index();
  return true;
}
