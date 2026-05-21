#include "blu.hh"

void Builder::init() {
  env_root = envs->alloc(nullptr);
  env_populate_with_builtins(env_root);
}

void Builder::deinit() {
  if (env_root) {
    envs->dealloc(env_root);
  }
}

b32 Builder::typecheck_and_eval_const_code() {
  NodeIndex idx_root = nodes->first_valid_index();

  Assert(nodes->kind(idx_root) == Ast_root);

  auto &root = nodes->data(idx_root).root;
  for (u32 i = 0; i < root.items.len(); i++) {
    auto idx_item = root.items[i];
    auto kind     = nodes->kind(idx_item);

    if (kind == Ast_const) {
      Todo();
    } else if (kind == Ast_declaration) {
      auto  &data = nodes->data(idx_item);
      StrKey key  = intern_identifier(data.declaration.name);

      if (env_root->has(key)) {
        Todo();
      }

      env_root->insert(
        key,
        {
          .resolve_status = ResolveStatus_type_unresolved,
          .is_const       = false,
          .node_index     = idx_item,
        }
      );
    } else {
      Todo();
    }
  }

  for (u32 i = 0; i < root.items.len(); i++) {
    Try(check_expression(env_root, &root.items[i], common.hint_nil));
  }
}

b32 Builder::eval_expression(Env<Declaration> *env, NodeIndex node_index, ValueIndex *result) {
  if (node_index.kind == NodeIndex_value) {
    *result = node_index.idx.value;
    return true;
  }

  auto  kind = nodes->kind(node_index);
  auto &data = nodes->data(node_index);

  switch (kind) {
  case Ast_type_function: {
    auto &f = data.type_function;

    u32 param_count = f.param_types.len();

    ValueIndex value_return_type;
    Try(eval_expression(env, f.return_type, &value_return_type));

    auto ty = alloc_type_function(arena_tmp, param_count);

    *ty = {
      .kind     = Type_function,
      .function = {
        .return_type = get_type(NodeIndex::from_value(value_return_type)),
        .param_count = param_count,
        .param_types = {},
      },
    };

    for (u32 i = 0; i < param_count; i++) {
      ValueIndex value_param_type;
      Try(eval_expression(env, f.param_types[i], &value_param_type));
      ty->function.param_types[i] = get_type(NodeIndex::from_value(value_param_type));
    }

    TypeIndex type_idx = types->add(ty);
    *result            = alloc_type(type_idx);
  } break;

  case Ast_identifier: {
    auto        key = intern_identifier(data.identifier.token_index);
    Declaration decl;
    env->lookup(key, &decl);
    *result = decl.node_index.as_value_idx();
  } break;

  default:
    puts(ast_kind_string(kind));
    Todo();
    break;
  }

  return true;
}

b32 Builder::check_and_eval_type_expression(
  Env<Declaration> *env, NodeIndex *node_index, ValueIndex *result
) {

    TypeHint hint_must_be_type = {
      .type     = types->type.type,
      .location = *node_index,
    };

  Try(check_expression(env, node_index, hint_must_be_type));

  ValueIndex value_declared_type;
  Try(eval_expression(env, *node_index, &value_declared_type));

  Value *v = values->get(value_declared_type);
  Try(ensure_is_expected_type(*node_index, types->type.type, v->type));

  *result = value_declared_type;

  return true;
}

b32 Builder::resolve_declaration(
  Env<Declaration> *env, NodeIndex location, TokenIndex identifier, Declaration *declaration
) {
  AstDeclaration ast_decl{};
  auto           key = intern_identifier(identifier);

  Declaration *decl;
  auto         found = env->lookup_ptr(key, &decl);

  if (!found) {
    Todo();
  }

  if (decl->resolve_status == ResolveStatus_type_resolved) {
    *declaration = *decl;
    return true;
  }

  if (decl->resolve_status == ResolveStatus_type_resolving) {
    Todo();
    return false;
  }

  decl->resolve_status = ResolveStatus_type_resolving;

  ast_decl = nodes->data(decl->node_index).declaration;

  ValueIndex value_declared_type;
  Try(check_and_eval_type_expression(env, &ast_decl.type, &value_declared_type));

  TypeIndex type_declared;
  copy_value_data(value_declared_type, &type_declared);

  TypeHint hint{};
  hint.type     = type_declared;
  hint.location = ast_decl.type;

  Try(check_expression(env, &ast_decl.value, hint));

  Try(resolve_possible_coercion(env, type_declared, &ast_decl.value));

  Todo();

  // The stored declaration needs to be updated to reference a Value instead of an AST node.
  // The value always needs to have a type.
  // The value needs to hold a value if the declaration is const.

  return true;
}

b32 Builder::check_expression(Env<Declaration> *env, NodeIndex *node_index, TypeHint hint) {
  auto  kind = nodes->kind(*node_index);
  auto &data = nodes->data(*node_index);

  switch (kind) {

  case Ast_declaration: {
    Declaration decl;
    resolve_declaration(env, *node_index, data.declaration.name, &decl);
    nodes->type(*node_index) = types->type.nil;
  } break;

  case Ast_identifier: {
    Declaration decl;
    Try(resolve_declaration(env, *node_index, data.identifier.token_index, &decl));
    nodes->type(*node_index) = get_type(decl.node_index);
  } break;

  case Ast_cast: {
    ValueIndex value_type_dst;
    Try(check_and_eval_type_expression(env, &data.cast.type_dst, &value_type_dst));

    Try(check_expression(env, &data.cast.value));

    TypeIndex type_expr = get_type(data.cast.value);

    TypeIndex type_dst;
    copy_value_data(value_type_dst, &type_dst);

    Try(ensure_is_valid_cast(*node_index, type_dst, type_expr));

    data.cast.type_dst = NodeIndex::from_value(value_type_dst);

    nodes->type(*node_index) = type_dst;
  } break;

  case Ast_const: {
    Try(check_expression(env, &data.const_.expr));

    ValueIndex val;
    Try(eval_expression(env, data.const_.expr, &val));

    *node_index = NodeIndex::from_value(val);
  } break;

  case Ast_assign: {
    Assert(data.assign.kind == Assign_normal);

    Try(check_expression(env, &data.assign.lhs));
    Try(ensure_is_assignable(data.assign.lhs));

    TypeIndex lhs_type = get_type(data.assign.lhs);

    TypeHint hint_value = {
      .type     = lhs_type,
      .location = data.assign.lhs,
    };

    Try(check_expression(env, &data.assign.value, hint_value));

    Try(resolve_possible_coercion(env, lhs_type, &data.assign.value));

    nodes->type(*node_index) = types->type.nil;
  } break;

  case Ast_type_function: {
    auto &f = data.type_function;

    u32 param_count = f.param_types.len();

    ValueIndex value_return_type;
    Try(check_and_eval_type_expression(env, &f.return_type, &value_return_type));

    auto snapshot = arena_tmp->take_snapshot();
    defer(arena_tmp->restore(snapshot));

    auto ty = alloc_type_function(arena_tmp, param_count);

    *ty = {
      .kind     = Type_function,
      .function = {
        .return_type = get_type(NodeIndex::from_value(value_return_type)),
        .param_count = param_count,
        .param_types = {},
      },
    };

    for (u32 i = 0; i < param_count; i++) {
      ValueIndex value_param_type;
      Try(check_and_eval_type_expression(env, &f.param_types[i], &value_param_type));
      ty->function.param_types[i] = get_type(NodeIndex::from_value(value_param_type));
    }

    TypeIndex type_idx = types->add(ty);
    ValueIndex val = alloc_type(type_idx);

    *node_index = NodeIndex::from_value(val);
  } break;

  case Ast_function: {
    // - If there is a hint, then the type of this function MUST be coercible to the hint type.
    // - If there is no hint, then the type of this function MUST be fully known.
    //   This means that all its parameters have a type annotation and its return type is given.

    Assert(hint.is_some());

    auto &f = data.function;

    Type *type_hint = types->get(hint.type);

    Assert(type_hint->kind == Type_function);

    auto env_params = envs->alloc(env);
    defer(envs->dealloc(env_params));

    for (u32 i = 0; i < data.function.param_names.len(); i++) {
      auto param_node  = data.function.param_names[i];
      auto token_index = nodes->data(param_node).identifier.token_index;
      auto key         = intern_identifier(token_index);

      auto param_type = type_hint->function.param_types[i];

      Value *value_param;
      auto   value_idx_param = values->alloc_value(&value_param);
      *value_param           = {
        .type = param_type,
        .data = nullptr,
      };

      env_params->insert(
        key,
        {.resolve_status = ResolveStatus_type_resolved,
         .is_const       = false,
         .node_index     = NodeIndex::from_value(value_idx_param)}
      );
    }

    auto return_type = type_hint->function.return_type;

    TypeHint hint_body = {
      .type     = return_type,
      .location = hint.location, // could be more precise
    };

    Try(check_expression(env_params, &data.function.body, hint_body));

    Try(resolve_possible_coercion(env, return_type, &data.function.body));

    nodes->type(*node_index) = hint.type;
  } break;

  case Ast_literal_int: {
    auto token_index = data.literal_int.token_index;
    auto str         = get_token_str(text, tokens, token_index);
    i64  i           = parse_i64(str);

    Value     *v;
    ValueIndex value_idx = values->alloc_value(&v);

    auto data = values->alloc_data<i64>();
    memcpy(data, &i, sizeof(i64));

    *v = {
      .type = types->type.literal_int,
      .data = data,
    };

    *node_index = NodeIndex::from_value(value_idx);
  } break;

  default:
    puts(ast_kind_string(kind));
    Todo();
    break;
  }

  return true;
}

StrKey Builder::intern_identifier(TokenIndex identifier) {
  auto s = get_token_str(text, tokens, identifier);
  return strings->add(s);
}

b32 Builder::ensure_is_expected_type(NodeIndex location, TypeIndex expected, TypeIndex actual) {
  if (actual == expected) {
    return true;
  }

  messages->error(location, "Expected {type}, but got {type}.", expected, actual);

  return false;
}

b32 Builder::ensure_is_valid_cast(NodeIndex at, TypeIndex type_dst, TypeIndex type_expr) {
  if (types->is_valid_cast(type_expr, type_dst)) {
    return true;
  }

  Todo();

  return false;
}

b32 Builder::ensure_is_assignable(NodeIndex node_index) {
  auto kind = nodes->kind(node_index);

  if (kind == Ast_identifier) {
    return true;
  }

  if (kind == Ast_index) {
    return ensure_is_assignable(nodes->data(node_index).index.indexable);
  }

  messages->error(node_index, "Cannot assign");

  return false;
}

void Builder::env_populate_with_builtins(Env<Declaration> *env) {
  // clang-format on
  env_insert_value(env, STR("i8"), alloc_type(types->type.i8_));
  env_insert_value(env, STR("i16"), alloc_type(types->type.i16_));
  env_insert_value(env, STR("i32"), alloc_type(types->type.i32_));
  env_insert_value(env, STR("i64"), alloc_type(types->type.i64_));

  env_insert_value(env, STR("u8"), alloc_type(types->type.u8_));
  env_insert_value(env, STR("u16"), alloc_type(types->type.u16_));
  env_insert_value(env, STR("u32"), alloc_type(types->type.u32_));
  env_insert_value(env, STR("u64"), alloc_type(types->type.u64_));

  env_insert_value(env, STR("uint"), alloc_type(types->type.uint));

  env_insert_value(env, STR("bool"), alloc_type(types->type.bool_));
  env_insert_value(env, STR("nil"), alloc_type(types->type.nil));
  env_insert_value(env, STR("never"), alloc_type(types->type.never));
  env_insert_value(env, STR("type"), alloc_type(types->type.type));

  env_insert_value(env, STR("true"), alloc_bool(1));
  env_insert_value(env, STR("false"), alloc_bool(0));
  // clang-format off
}

void Builder::env_insert_value(Env<Declaration> *env, Str identifier, ValueIndex value) {
  env->insert(
    strings->add(identifier),
    {
      .resolve_status = ResolveStatus_type_resolved,
      .is_const       = true,
      .node_index     = {
        .kind = NodeIndex_value,
        .idx  = {
          .value = value,
        },
      },
    }
  );
}

ValueIndex Builder::alloc_type(TypeIndex type_idx) {
  auto type_of_val = types->type.type;

  Value *val;
  auto   idx  = values->alloc_value(&val);
  auto   data = values->alloc_data(types->size_info(type_of_val));

  memcpy(data, &type_idx, sizeof(TypeIndex));

  *val        = {
    .type = type_of_val,
    .data = data,
  };

  return idx;
}

ValueIndex Builder::alloc_bool(u8 b) {
  auto type_of_val = types->type.bool_;

  Value *val;
  auto   idx  = values->alloc_value(&val);
  auto   data = values->alloc_data(types->size_info(type_of_val));

  memcpy(data, &b, sizeof(u8));

  *val        = {
    .type = type_of_val,
    .data = data,
  };

  return idx;
}

b32 Builder::resolve_possible_coercion(Env<Declaration> *env, TypeIndex type_dst, NodeIndex *value) {
  auto type_src = get_type(*value);

  if (type_src == type_dst) {
    return true;
  }

  Todo();

  // This function needs to:
  // - update the recorded type at ast index of `value` (if necessary).
  // - insert a cast if necessary.

  return true;
}

TypeIndex Builder::get_type(NodeIndex node_index) {
  if (node_index.kind == NodeIndex_ast) {
    return nodes->type(node_index);
  } else {
    Value *v = values->get(node_index.idx.value);
    return v->type;
  }
}

