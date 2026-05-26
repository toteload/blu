#include "blu.hh"

void Builder::init() {
  env_builtin = envs->alloc(nullptr);
  env_populate_with_builtins(env_builtin);
  env_root = envs->alloc(env_builtin);

  common.hint_nil = {
    .type     = types->type.nil,
    .location = NodeIndex::none(),
  };
}

void Builder::deinit() {
  if (env_root) {
    envs->dealloc(env_root);
  }

  if (env_builtin) {
    envs->dealloc(env_builtin);
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
        // - You are not allowed to overwrite builtin symbols.
        // - You are not allowed to have global symbols with the same name.
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
    auto *item = &root.items[i];
    auto  kind = nodes->kind(*item);

    Assert(kind == Ast_declaration);

    auto &data = nodes->data(*item);

    Declaration decl;
    Try(resolve_declaration(env_root, data.declaration.name, &decl));
  }

  return true;
}

b32 Builder::eval_expression(Env<Declaration> *env, NodeIndex node_index, ValueIndex *result) {
  if (node_index.kind == NodeIndex_value) {
    *result = node_index.idx.value;
    return true;
  }

  auto  kind = nodes->kind(node_index);
  auto &data = nodes->data(node_index);

  switch (kind) {

  case Ast_cast: {
    TypeIndex type_dst;
    copy_value_data(data.cast.type_dst.as_value_idx(), &type_dst);

    ValueIndex val;
    Try(eval_expression(env, data.cast.value, &val));
    Try(eval_cast(type_dst, val, result));
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

b32 Builder::check_and_eval_expression(
  Env<Declaration> *env, NodeIndex *node_index, TypeHint hint, ValueIndex *result
) {
  Try(check_expression(env, node_index, hint));

  ValueIndex value_declared_type;
  Try(eval_expression(env, *node_index, &value_declared_type));

  *result = value_declared_type;

  return true;
}

b32 Builder::check_and_eval_type_expression(
  Env<Declaration> *env, NodeIndex *node_index, ValueIndex *result
) {
  TypeHint must_be_type = {
    .type     = types->type.type,
    .location = *node_index,
  };

  return check_and_eval_expression(env, node_index, must_be_type, result);
}

b32 Builder::resolve_declaration(
  Env<Declaration> *env, TokenIndex identifier, Declaration *declaration
) {
  auto key = intern_identifier(identifier);

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

  auto &data = nodes->data(decl->node_index);

  ValueIndex value_declared_type;
  Try(check_and_eval_type_expression(env, &data.declaration.type, &value_declared_type));

  TypeIndex type_declared;
  copy_value_data(value_declared_type, &type_declared);

  TypeHint hint{};
  hint.type     = type_declared;
  hint.location = data.declaration.type;

  // Only top level are not immediately resolved, because they may be declared in any order.
  // Top level declaration values are implicitly const so we immediately evaluate them.

  ValueIndex value;
  Try(check_and_eval_expression(env, &data.declaration.value, hint, &value));

  decl->resolve_status = ResolveStatus_type_resolved;
  decl->node_index     = NodeIndex::from_value(value);

  *declaration = *decl;

  return true;
}

b32 Builder::check_expression(Env<Declaration> *env, NodeIndex *node_index, TypeHint hint) {
  auto  kind = nodes->kind(*node_index);
  auto &data = nodes->data(*node_index);

  switch (kind) {

  case Ast_declaration: {
    Declaration decl;
    Try(resolve_declaration(env, data.declaration.name, &decl));
    nodes->type(*node_index) = types->type.nil;
  } break;

  case Ast_identifier: {
    Declaration decl;
    Try(resolve_declaration(env, data.identifier.token_index, &decl));
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

    nodes->type(*node_index) = types->type.nil;
  } break;

  case Ast_type_function: {
    auto &f = data.type_function;

    u32 param_count = f.param_types.len();

    ValueIndex value_return_type;
    Try(check_and_eval_type_expression(env, &f.return_type, &value_return_type));

    TypeIndex return_type;
    copy_value_data(value_return_type, &return_type);

    auto snapshot = arena_tmp->take_snapshot();
    defer(arena_tmp->restore(snapshot));

    auto ty = alloc_type_function(arena_tmp, param_count);

    *ty = {
      .kind     = Type_function,
      .function = {
        .return_type = return_type,
        .param_count = param_count,
        .param_types = {},
      },
    };

    for (u32 i = 0; i < param_count; i++) {
      ValueIndex value_param_type;
      Try(check_and_eval_type_expression(env, &f.param_types[i], &value_param_type));

      TypeIndex param_type;
      copy_value_data(value_param_type, &param_type);

      ty->function.param_types[i] = param_type;
    }

    TypeIndex  type_idx = types->add(ty);
    ValueIndex val      = alloc_type(type_idx);

    *node_index = NodeIndex::from_value(val);
  } break;

  case Ast_function: {
    // - If there is a hint, then the type of this function MUST be coercible to the hint type.
    // - If there is no hint, then the type of this function MUST be fully known.
    //   This means that all its parameters have a type annotation and its return type is given.

    Assert(hint.is_some());

    Type *type_hint = types->get(hint.type);

    Assert(type_hint->kind == Type_function);
    Assert(type_hint->function.param_count == data.function.param_names.len());

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

    Value *value_function;
    auto   value_idx_function = values->alloc_value(&value_function);
    auto   payload            = values->alloc_data<NodeIndex>();
    memcpy(payload, node_index, sizeof(NodeIndex));
    *value_function = {
      .type = hint.type,
      .data = payload,
    };

    *node_index = NodeIndex::from_value(value_idx_function);
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

  case Ast_block: {
    if (data.block.items.len() == 0) {
      nodes->type(*node_index) = types->type.nil;
      break;
    }

    auto env_block = envs->alloc(env);
    defer(envs->dealloc(env_block));

    for (u32 i = 0; i < data.block.items.len() - 1; i++) {
      Try(check_expression(env_block, &data.block.items[i], common.hint_nil));
    }

    auto &last_item = data.block.items[data.block.items.len() - 1];

    Try(check_expression(env_block, &last_item, hint));

    nodes->type(*node_index) = get_type(last_item);
  } break;

  case Ast_if_else: {
    TypeHint hint_bool{
      .type = types->type.bool_,
      .location = data.if_else.cond,
    };

    Try(check_expression(env, &data.if_else.cond, hint_bool));

    TypeIndex cond_type = get_type(data.if_else.cond);

    Try(check_expression(env, &data.if_else.then, hint));

    if (data.if_else.otherwise.is_none()) {
      nodes->type(*node_index) = types->type.nil;
      break;
    }

    Try(check_expression(env, &data.if_else.otherwise, hint));

    TypeIndex then_type      = get_type(data.if_else.then);
    TypeIndex otherwise_type = get_type(data.if_else.otherwise);

    TypeIndex type_unified;
    Try(check_unification(
      data.if_else.then,
      then_type,
      data.if_else.otherwise,
      otherwise_type,
      &type_unified
    ));

    nodes->type(*node_index) = type_unified;
  } break;

  default:
    puts(ast_kind_string(kind));
    Todo();
    break;
  }

  Try(check_and_resolve_coercion(env, hint, node_index));

  return true;
}

StrKey Builder::intern_identifier(TokenIndex identifier) {
  auto s = get_token_str(text, tokens, identifier);
  return strings->add(s);
}

b32 Builder::check_unification(NodeIndex node_lhs, TypeIndex type_lhs, NodeIndex node_rhs, TypeIndex type_rhs, TypeIndex *type_unified) {
  if (types->unify(type_lhs, type_rhs, type_unified)) {
    return true;
  }

  messages->error(node_lhs, "Cannot unify types {type} and {type}.", type_lhs, type_rhs);

  return false;
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

b32 Builder::check_and_resolve_coercion(Env<Declaration> *env, TypeHint expected, NodeIndex *value) {
  auto type_src = get_type(*value);

  if (type_src == expected.type) {
    return true;
  }

  if (!types->is_coercible_to(type_src, expected.type)) {
    Todo();
    return false;
  }


  auto old_node_index = *value;
  auto node_index     = nodes->alloc();
  *value     = node_index;

  auto   val_idx           = alloc_type(expected.type);

  AstCast cast;
  cast.type_dst = NodeIndex::from_value(val_idx);
  cast.value    = old_node_index;

  nodes->type(node_index) = expected.type;
  nodes->set(
    node_index,
    {
      Ast_cast,
      {{0}, {0}}, // TODO replace this with something else. or maybe these nodes shouldn't live in
                  // `AstNodes`
      {.cast = cast},
    }
  );

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

b32 Builder::eval_cast(TypeIndex type_idx_dst, ValueIndex val_idx, ValueIndex *result) {
  // You may assume that the cast is valid, since it got past the type checker.

  Value *val = values->get(val_idx);

  TypeIndex type_idx_src = val->type;

  if (type_idx_dst == type_idx_src) {
    *result = val_idx;
    return true;
  }

  Type *type_dst = types->get(type_idx_dst);
  Type *type_src = types->get(type_idx_src);

  if (type_src->kind == Type_literal_function && type_dst->kind == Type_function) {
    Value *cast_value;
    *result     = values->alloc_value(&cast_value);
    auto data   = values->alloc_data(types->size_info(type_idx_dst));
    *cast_value = {.type = type_idx_dst, .data = data};

    memcpy(data, val->data, sizeof(NodeIndex));

    return true;
  }

  if (type_src->kind == Type_literal_int && type_dst->kind == Type_integer) {
    i64 i = *cast<i64 *>(val->data);

    auto signedness = type_dst->integer.signedness;
    auto bitwidth   = type_dst->integer.bitwidth;

    if (signedness == Signed) {
      i64 min_value = int_value_min(bitwidth);
      i64 max_value = int_value_max(bitwidth);
      if (i < min_value || i > max_value) {
        messages->error("integer constant out of range of destination type.");
        return false;
      }
    } else {
      u64 max_value = uint_value_max(bitwidth);
      if (i < 0 || cast<u64>(i) > max_value) {
        messages->error("integer constant out of range of destination type.");
        return false;
      }
    }

    Value *cast_value;
    *result     = values->alloc_value(&cast_value);
    auto data   = values->alloc_data(types->size_info(type_idx_dst));
    *cast_value = {.type = type_idx_dst, .data = data};

    // clang-format off
    switch (bitwidth) {
    case  8: signedness == Signed ? (*cast<i8  *>(data) = cast<i8 >(i)) : (*cast<u8  *>(data) = cast<u8 >(i)); break;
    case 16: signedness == Signed ? (*cast<i16 *>(data) = cast<i16>(i)) : (*cast<u16 *>(data) = cast<u16>(i)); break;
    case 32: signedness == Signed ? (*cast<i32 *>(data) = cast<i32>(i)) : (*cast<u32 *>(data) = cast<u32>(i)); break;
    case 64: signedness == Signed ? (*cast<i64 *>(data) = cast<i64>(i)) : (*cast<u64 *>(data) = cast<u64>(i)); break;
    default: Unreachable();
    }
    // clang-format on

    return true;
  }

  if (type_src->kind == Type_integer && type_dst->kind == Type_integer) {
    if (type_dst->integer.signedness == Signed && type_src->integer.signedness == Signed) {
      i64 i = read_value_i64(val_idx);

      i64 lo = int_value_min(type_dst->integer.bitwidth);
      i64 hi = int_value_max(type_dst->integer.bitwidth);

      if (i < lo || i > hi) {
        Todo("invalid cast: value out of range");
      }
    } else if (type_dst->integer.signedness == Unsigned && type_src->integer.signedness == Signed) {
      i64 i = read_value_i64(val_idx);

      if (i < 0) {
        Todo("invalid cast: value out of range");
      }

      if (type_src->integer.bitwidth > type_dst->integer.bitwidth) {
        u64 u = cast<u64>(i);

        u64 hi = uint_value_max(type_dst->integer.bitwidth);

        if (u > hi) {
          Todo("invalid cast: value out of range");
        }
      }
    } else if (type_dst->integer.signedness == Signed && type_src->integer.signedness == Unsigned) {
      u64 i = read_value_u64(val_idx);

      u64 hi = cast<u64>(int_value_max(type_dst->integer.bitwidth));

      if (i > hi) {
        Todo("invalid cast: value out of range");
      }
    } else if (type_dst->integer.signedness == Unsigned &&
               type_src->integer.signedness == Unsigned) {
      u64 i = read_value_u64(val_idx);

      u64 hi = uint_value_max(type_dst->integer.bitwidth);

      if (i > hi) {
        Todo("invalid cast: value out of range");
      }
    } else {
      Unreachable();
    }

    Value *v;
    *result = values->alloc_value(&v);

    auto size_info = types->size_info(type_idx_dst);
    auto data      = values->alloc_data(size_info);

    *v = {
      .type = type_idx_dst,
      .data = data,
    };

    memcpy(data, val->data, size_info.size);

    return true;
  }

  if (type_src->kind == Type_array && type_dst->kind == Type_slice) {
    u32 count = type_src->array.size;

    Value *v;
    *result = values->alloc_value(&v);

    auto size_info = types->size_info(type_idx_dst);
    auto data      = values->alloc_data(size_info);

    *cast<ValueSlice *>(data) = {
      .len   = count,
      .items = val->data,
    };

    *v = {
      .type = type_idx_dst,
      .data = data,
    };

    return true;
  }

  if (type_src->kind == Type_sequence) {
    TypeIndex base_type;
    if (type_dst->kind == Type_slice) {
      base_type = type_dst->slice.base_type;
    } else if (type_dst->kind == Type_array) {
      base_type = type_dst->array.base_type;
    } else {
      Unreachable();
    }

    u32  count     = type_src->sequence.count;
    auto size_info = types->size_info(base_type);

    ValueIndex *sequence_items = cast<ValueIndex *>(val->data);

    Todo();

    // auto items = values->alloc_data(size_info, count);
    // for (u32 i = 0; i < count; i++) {
    //   Try(coerce_value(base_type, sequence_items[i], ptr_offset(items, size_info.stride * i)));
    // }

    // if (type_dst->kind == Type_array) {
    //   *cast<void **>(out) = items;
    // } else {
    //   *cast<ValueSlice *>(out) = {.len = count, .items = items};
    // }

    return true;
  }

  Todo();

  return true;
}
