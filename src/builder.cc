#include "blu.hh"

void Builder::init() {
  env_builtin = envs->alloc(nullptr);
  env_populate_with_builtins(env_builtin);
  env_root = envs->alloc(env_builtin);

  common.hint_nil = {
    .type     = types->type.nil,
    .location = NodeIndex::none(),
  };

  {
    Value *v;
    common.val.nil = values->alloc_value(&v);
    *v             = {
      .type = types->type.nil,
      .data = nullptr,
    };
  }
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
  AstIndex idx_root = nodes->first_valid_index();

  Assert(nodes->kind(idx_root) == Ast_root);

  auto &root = nodes->data(idx_root).root;
  for (u32 i = 0; i < root.items.len(); i++) {
    auto idx_item = root.items[i].as_ast_idx();
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
          .scope          = Scope_toplevel,
          .resolve_status = ResolveStatus_unresolved,
          .is_const       = false,
          .ast_index      = idx_item,
        }
      );
    } else {
      Todo();
    }
  }

  for (u32 i = 0; i < root.items.len(); i++) {
    auto item = root.items[i].as_ast_idx();
    auto kind = nodes->kind(item);

    Assert(kind == Ast_declaration);

    auto &data = nodes->data(item);

    Try(resolve_declaration(env_root, data.declaration.name));
  }

  return true;
}

b32 Builder::eval_expression(Env<Declaration> *env, NodeIndex node_index, ValueIndex *result) {
  if (node_index.kind == NodeIndex_value) {
    *result = node_index.idx.value;
    return true;
  }

  AstIndex ast_index = node_index.as_ast_idx();

  auto  kind = nodes->kind(ast_index);
  auto &data = nodes->data(ast_index);

  switch (kind) {

  case Ast_cast: {
    TypeIndex type_dst;
    copy_value_data(data.cast.type_dst.as_value_idx(), &type_dst);

    ValueIndex val;
    Try(eval_expression(env, data.cast.value, &val));
    Try(eval_cast(type_dst, val, result));
  } break;

  case Ast_identifier: {
    auto        key = intern_identifier(data.identifier);
    Declaration decl;
    b32         found = env->lookup(key, &decl);
    Assert(found);
    Assert(decl.resolve_status == ResolveStatus_resolved);

    *result = decl.value;
  } break;

  case Ast_declaration: {
    auto       key = intern_identifier(data.declaration.name);
    ValueIndex value;
    Try(eval_expression(env, data.declaration.value, &value));
    env->insert(
      key,
      {
        .scope          = Scope_local,
        .resolve_status = ResolveStatus_resolved,
        .is_const       = false,
        .value          = value,
      }
    );
    *result = common.val.nil;
  } break;

  case Ast_block: {
    auto &block = data.block;
    if (block.items.len() == 0) {
      *result = common.val.nil;
      break;
    }

    auto env_block = envs->alloc(env);
    defer(envs->dealloc(env_block));

    auto snapshot = arena_tmp->take_snapshot();
    defer(arena_tmp->restore(snapshot));

    NodeIndex *defers      = arena_tmp->alloc<NodeIndex>(block.items.len());
    u32        defer_count = 0;

    ValueIndex last = common.val.nil;

    for (u32 i = 0; i < block.items.len(); i++) {
      auto item = block.items[i];

      if (item.kind == NodeIndex_ast) {
        auto ast_index_item = item.as_ast_idx();
        auto kind_item      = nodes->kind(ast_index_item);
        if (kind_item == Ast_defer) {
          defers[defer_count++] = nodes->data(ast_index_item).defer.value;
          last                  = common.val.nil;
          continue;
        }
      }

      Try(eval_expression(env_block, item, &last));
    }

    *result = last;

    for (u32 i = defer_count; i > 0; i--) {
      ValueIndex e;
      Try(eval_expression(env_block, defers[i - 1], &e));
    }
  } break;

  case Ast_if_else: {
    auto &if_else = data.if_else;

    ValueIndex cond;
    Try(eval_expression(env, if_else.cond, &cond));

    Value *v = values->get(cond);

    if (*cast<u8 *>(v->data) == 1) {
      Try(eval_expression(env, if_else.then, result));
    } else if (if_else.otherwise.is_some()) {
      Try(eval_expression(env, if_else.otherwise, result));
    } else {
      *result = common.val.nil;
    }
  } break;

  case Ast_binary_op: {
    auto binop = data.binary_op;

    ValueIndex lhs;
    Try(eval_expression(env, binop.lhs, &lhs));

    ValueIndex rhs;
    Try(eval_expression(env, binop.rhs, &rhs));

    Try(eval_binary_op(binop.kind, lhs, rhs, result));
  } break;

  case Ast_call: {
    Todo();
  } break;

  default:
    puts(ast_kind_string(kind));
    Todo();
    break;
  }

  return true;
}

b32 Builder::eval_binary_op(BinaryOpKind op, ValueIndex lhs, ValueIndex rhs, ValueIndex *result) {
  switch (op) {
  case Cmp_equal:
  case Cmp_not_equal:
  case Cmp_less_than:
  case Cmp_less_equal:
  case Cmp_greater_than:
  case Cmp_greater_equal: {
    auto left      = values->get(lhs);
    auto left_type = types->get(left->type);

    Assert(left_type->is_integer_or_literal_int());

    bool is_unsigned = left_type->kind == Type_integer && left_type->integer.signedness == Unsigned;

    bool res;
    if (is_unsigned) {
      u64 a = read_value_u64(lhs);
      u64 b = read_value_u64(rhs);
      // clang-format off
      switch (op) {
      case Cmp_equal: res = a == b; break;
      case Cmp_not_equal: res = a != b; break;
      case Cmp_less_than: res = a < b; break;
      case Cmp_less_equal: res = a <= b; break;
      case Cmp_greater_than: res = a > b; break;
      case Cmp_greater_equal: res = a >= b; break;
      default: Unreachable();
      }
      // clang-format on
    } else {
      i64 a = read_value_i64(lhs);
      i64 b = read_value_i64(rhs);
      // clang-format off
      switch (op) {
      case Cmp_equal: res = a == b; break;
      case Cmp_not_equal: res = a != b; break;
      case Cmp_less_than: res = a < b; break;
      case Cmp_less_equal: res = a <= b; break;
      case Cmp_greater_than: res = a > b; break;
      case Cmp_greater_equal: res = a >= b; break;
      default: Unreachable();
      }
      // clang-format on
    }

    Value *v;
    auto   idx  = values->alloc_value(&v);
    auto   data = cast<u8 *>(values->alloc_data(types->size_info(types->type.bool_)));
    *data       = res ? 1 : 0;
    *v          = {.type = types->type.bool_, .data = data};
    *result     = idx;

    return true;
  } break;
  case Mul:
  case Div:
  case Mod:
  case Add:
  case Sub: {
    auto left  = values->get(lhs);
    auto right = values->get(rhs);

    auto left_type  = types->get(left->type);
    auto right_type = types->get(right->type);

    Assert(left_type->is_integer_or_literal_int() && right_type->is_integer_or_literal_int());

    auto signedness = left_type->integer.signedness;

    if (signedness == Signed) {
      i64  a        = read_value_i64(lhs);
      i64  b        = read_value_i64(rhs);
      bool overflow = false;
      i64  res;

      // clang-format off
      switch (op) {
      case Div: res = a / b; break;
      case Mod: res = a % b; break;
      case Add: overflow = __builtin_add_overflow(a, b, &res); break;
      case Sub: overflow = __builtin_sub_overflow(a, b, &res); break;
      case Mul: overflow = __builtin_mul_overflow(a, b, &res); break;
      default: Unreachable(); break;
      }
      // clang-format on

      if (overflow) {
        Todo();
      }

      TypeIndex result_type_idx;
      types->unify(left->type, right->type, &result_type_idx);

      auto result_type = types->get(result_type_idx);

      if (result_type->kind == Type_integer) {
        i64 min_value = int_value_min(result_type->integer.bitwidth);
        i64 max_value = int_value_max(result_type->integer.bitwidth);

        if (res > max_value || res < min_value) {
          Todo();
        }
      }

      auto   size_info = types->size_info(result_type_idx);
      Value *v;
      auto   idx       = values->alloc_value(&v);
      void  *data      = values->alloc_data(size_info);
      u32    byte_size = size_info.size;
      memcpy(data, &res, byte_size);
      *v = {
        .type = result_type_idx,
        .data = data,
      };

      *result = idx;

      return true;
    } else {
      u64  a        = read_value_u64(lhs);
      u64  b        = read_value_u64(rhs);
      bool overflow = false;
      u64  res;

      // clang-format off
      switch (op) {
      case Div: res = a / b; break;
      case Mod: res = a % b; break;
      case Add: overflow = __builtin_add_overflow(a, b, &res); break;
      case Sub: overflow = __builtin_sub_overflow(a, b, &res); break;
      case Mul: overflow = __builtin_mul_overflow(a, b, &res); break;
      default: Unreachable(); break;
      }
      // clang-format on

      if (overflow) {
        Todo();
      }

      TypeIndex result_type_idx;
      types->unify(left->type, right->type, &result_type_idx);

      auto result_type = types->get(result_type_idx);

      if (result_type->kind == Type_integer) {
        u64 max_value = uint_value_max(result_type->integer.bitwidth);

        if (res > max_value) {
          Todo();
        }
      }

      auto   size_info = types->size_info(result_type_idx);
      Value *v;
      auto   idx       = values->alloc_value(&v);
      void  *data      = values->alloc_data(size_info);
      u32    byte_size = size_info.size;
      memcpy(data, &res, byte_size);
      *v = {
        .type = result_type_idx,
        .data = data,
      };

      *result = idx;

      return true;
    }
  } break;
  default:
    Todo();
    break;
  }
  return true;
}

b32 Builder::eval_call(
  Env<Declaration> *env, ValueIndex function, Slice<ValueIndex> args, ValueIndex *result
) {
  Value *f = values->get(function);

  auto env_args = envs->alloc(env);
  defer(envs->dealloc(env_args));

  AstIndex ast_function = *cast<AstIndex *>(f->data);
  auto    &data         = nodes->data(ast_function);

  for (u32 i = 0; i < data.function.params.len(); i++) {
    auto ast_index_param = data.function.params[i].as_ast_idx();
    auto data_param      = nodes->data(ast_index_param).param;
    auto token_index     = data_param.name;
    auto key             = intern_identifier(token_index);

    env_args->insert(
      key,
      {
        .scope          = Scope_parameter,
        .resolve_status = ResolveStatus_resolved,
        .is_const       = false,
        .value          = args[i],
      }
    );
  }

  return eval_expression(env_args, data.function.body, result);
}

b32 Builder::check_and_eval_expression(
  Env<Declaration> *env, NodeIndex *node_index, TypeHint hint, ValueIndex *result
) {
  Try(check_expression(env, node_index, hint));
  Try(eval_expression(env, *node_index, result));
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

b32 Builder::find_declaration(Env<Declaration> *env, TokenIndex identifier, Declaration **decl) {
  auto key = intern_identifier(identifier);

  auto found = env->lookup_ptr(key, decl);

  if (!found) {
    messages->error("Could not find identifier {strkey}.", key);
    return false;
  }

  return true;
}

b32 Builder::resolve_declaration_type(
  Env<Declaration> *env, Declaration *decl, TypeIndex *type_declaration
) {
  if (decl->resolve_status == ResolveStatus_resolving_type) {
    messages->error("Circular declaration encountered.");
    return false;
  }

  if (decl->resolve_status > ResolveStatus_resolving_type) {
    Value *val        = values->get(decl->value);
    *type_declaration = val->type;
    return true;
  }

  Assert(decl->resolve_status == ResolveStatus_unresolved);

  decl->resolve_status = ResolveStatus_resolving_type;

  auto &data = nodes->data(decl->ast_index);

  ValueIndex value_declared_type;
  Try(check_and_eval_type_expression(env, &data.declaration.type, &value_declared_type));

  decl->resolve_status = ResolveStatus_resolved_type;

  TypeIndex type_declared;
  copy_value_data(value_declared_type, &type_declared);

  // TODO: Is it possible that the `decl` pointer is invalidated????!!?!???!

  Value *val;
  decl->value = values->alloc_value(&val);

  *val = {
    .type = type_declared,
    .data = nullptr,
  };

  *type_declaration = type_declared;

  return true;
}

b32 Builder::resolve_declaration(Env<Declaration> *env, TokenIndex identifier) {
  Declaration *decl;
  Try(find_declaration(env, identifier, &decl));

  TypeIndex type_declared;
  Try(resolve_declaration_type(env, decl, &type_declared));

  auto &data = nodes->data(decl->ast_index);

  TypeHint hint{};
  hint.type     = type_declared;
  hint.location = data.declaration.type;

  ValueIndex value;
  Try(check_and_eval_expression(env, &data.declaration.value, hint, &value));

  decl->resolve_status = ResolveStatus_resolved;
  decl->value          = value;

  return true;
}

b32 Builder::check_expression(Env<Declaration> *env, NodeIndex *node_index, TypeHint hint) {
  AstIndex ast_index = node_index->as_ast_idx();

  auto  kind = nodes->kind(ast_index);
  auto &data = nodes->data(ast_index);

  switch (kind) {

  case Ast_declaration: {
    ValueIndex value_declared_type;
    Try(check_and_eval_type_expression(env, &data.declaration.type, &value_declared_type));

    TypeIndex type_declared;
    copy_value_data(value_declared_type, &type_declared);

    TypeHint hint{};
    hint.type     = type_declared;
    hint.location = data.declaration.type;

    Try(check_expression(env, &data.declaration.value, hint));

    Value     *val;
    ValueIndex val_idx = values->alloc_value(&val);

    *val = {
      .type = type_declared,
      .data = nullptr,
    };

    auto key = intern_identifier(data.declaration.name);
    env->insert(
      key,
      {
        .scope          = Scope_local,
        .resolve_status = ResolveStatus_resolved,
        .is_const       = false,
        .value          = val_idx,
      }
    );

    nodes->type(ast_index) = types->type.nil;
  } break;

  case Ast_identifier: {
    Declaration *decl;
    Try(find_declaration(env, data.identifier, &decl));

    TypeIndex type_declared;
    Try(resolve_declaration_type(env, decl, &type_declared));

    if (decl->resolve_status != ResolveStatus_resolved) {
      // if the declaration is const or the type is const then the value should be immediately evaluated and resolved.
      if ((decl->is_const || types->is_const(type_declared))) {
        if (decl->resolve_status == ResolveStatus_resolving_value) {
          messages->error("Circular declaration encountered.");
          return false;
        }

        auto &ast_declaration = nodes->data(decl->ast_index).declaration;

        // If this is a function then we consider this declaration as resolved already, because
        // a function can be recursive. 

        decl->resolve_status = ResolveStatus_resolving_value;

        TypeHint hint_declaration{};
        hint_declaration.type     = type_declared;
        hint_declaration.location = ast_declaration.type;

        ValueIndex value;
        Try(check_and_eval_expression(env, &ast_declaration.value, hint_declaration, &value));

        decl->resolve_status = ResolveStatus_resolved;
        decl->value          = value;
      }
    }

    nodes->type(ast_index) = type_declared;
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

    nodes->type(ast_index) = type_dst;
  } break;

  case Ast_const: {
    Try(check_expression(env, &data.const_.expr));

    ValueIndex val;
    Try(eval_expression(env, data.const_.expr, &val));

    *node_index = NodeIndex::from_value(val);
  } break;

  case Ast_assign: {
    Assert(data.assign.kind == Assign_normal);

    Todo();

    nodes->type(ast_index) = types->type.nil;
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
    Assert(type_hint->function.param_count == data.function.params.len());

    auto env_params = envs->alloc(env);
    defer(envs->dealloc(env_params));

    for (u32 i = 0; i < data.function.params.len(); i++) {
      auto param_node  = data.function.params[i].as_ast_idx();
      auto token_index = nodes->data(param_node).identifier;
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
        {
          .scope          = Scope_parameter,
          .resolve_status = ResolveStatus_resolved,
          .is_const       = false,
          .value          = value_idx_param,
        }
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
    auto   payload            = values->alloc_data<AstIndex>();
    memcpy(payload, &ast_index, sizeof(AstIndex));
    *value_function = {
      .type = hint.type,
      .data = payload,
    };

    *node_index = NodeIndex::from_value(value_idx_function);
  } break;

  case Ast_literal_int: {
    auto token_index = data.literal_int;
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
      nodes->type(ast_index) = types->type.nil;
      break;
    }

    auto env_block = envs->alloc(env);
    defer(envs->dealloc(env_block));

    for (u32 i = 0; i < data.block.items.len() - 1; i++) {
      Try(check_expression(env_block, &data.block.items[i], common.hint_nil));
    }

    auto &last_item = data.block.items[data.block.items.len() - 1];

    Try(check_expression(env_block, &last_item, hint));

    nodes->type(ast_index) = get_type(last_item);
  } break;

  case Ast_if_else: {
    TypeHint hint_bool{
      .type     = types->type.bool_,
      .location = data.if_else.cond,
    };

    Try(check_expression(env, &data.if_else.cond, hint_bool));

    TypeIndex cond_type = get_type(data.if_else.cond);

    Try(check_expression(env, &data.if_else.then, hint));

    if (data.if_else.otherwise.is_none()) {
      nodes->type(ast_index) = types->type.nil;
      break;
    }

    Try(check_expression(env, &data.if_else.otherwise, hint));

    TypeIndex type_unified;
    Try(check_and_resolve_unification(&data.if_else.then, &data.if_else.otherwise, &type_unified));

    nodes->type(ast_index) = type_unified;
  } break;

  case Ast_binary_op: {
    NodeIndex &lhs = data.binary_op.lhs;
    NodeIndex &rhs = data.binary_op.rhs;

    switch (data.binary_op.kind) {
    case Logical_and:
    case Logical_or: {
      TypeHint hint_bool{};
      hint_bool.type     = types->type.bool_;
      hint_bool.location = *node_index;

      Try(check_expression(env, &lhs, hint_bool));
      Try(check_expression(env, &rhs, hint_bool));

      nodes->type(ast_index) = types->type.bool_;
    } break;
    case Cmp_equal:
    case Cmp_not_equal:
    case Cmp_less_than:
    case Cmp_less_equal:
    case Cmp_greater_than:
    case Cmp_greater_equal: {
      Try(check_expression(env, &lhs));
      Try(check_expression(env, &rhs));

      TypeIndex type_unified;
      Try(check_and_resolve_unification(&lhs, &rhs, &type_unified));
      Try(ensure_is_type_comparable(ast_index, type_unified));

      nodes->type(ast_index) = types->type.bool_;
    } break;
    case Mul:
    case Div:
    case Mod:
    case Sub:
    case Add:
    case Bit_shift_left:
    case Bit_shift_right:
    case Bit_and:
    case Bit_or:
    case Bit_xor: {
      Try(check_expression(env, &lhs));
      Try(check_expression(env, &rhs));

      TypeIndex type_unified;
      Try(check_and_resolve_unification(&lhs, &rhs, &type_unified));

      nodes->type(ast_index) = type_unified;
    } break;
    case BinaryOpKind_max:
      Unreachable();
      return false;
    }
  } break;

  case Ast_call: {
    Try(check_expression(env, &data.call.callee));

    TypeIndex type_idx_callee = get_type(data.call.callee);

    Type *type_callee = types->get(type_idx_callee);

    if (type_callee->kind != Type_function) {
      messages
        ->error(data.call.callee, "Callee must be a function. Found {type}.", type_idx_callee);
      break;
    }

    if (type_callee->function.param_count != data.call.args.len()) {
      messages->error(data.call.callee, "Function called with incorrect number of arguments.");
      break;
    }

    u32 param_count = data.call.args.len();

    for (u32 i = 0; i < param_count; i++) {
      TypeHint hint_arg{};
      hint_arg.type = type_callee->function.param_types[i];

      Try(check_expression(env, &data.call.args[i], hint_arg));
    }

    nodes->type(ast_index) = type_callee->function.return_type;
  } break;

  default:
    puts(ast_kind_string(kind));
    Todo();
    break;
  }

  Try(check_and_resolve_coercion(hint, node_index));

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
  AstIndex ast_index = node_index.as_ast_idx();

  auto kind = nodes->kind(ast_index);

  if (kind == Ast_identifier) {
    return true;
  }

  if (kind == Ast_index) {
    return ensure_is_assignable(nodes->data(ast_index).index.indexable);
  }

  messages->error(node_index, "Cannot assign");

  return false;
}

b32 Builder::ensure_is_type_comparable(AstIndex location, TypeIndex type) { return true; }

void Builder::env_populate_with_builtins(Env<Declaration> *env) {
  // clang-format off
  env_insert_builtin_value(env, STR("i8"),  alloc_type(types->type.i8_));
  env_insert_builtin_value(env, STR("i16"), alloc_type(types->type.i16_));
  env_insert_builtin_value(env, STR("i32"), alloc_type(types->type.i32_));
  env_insert_builtin_value(env, STR("i64"), alloc_type(types->type.i64_));

  env_insert_builtin_value(env, STR("u8"),  alloc_type(types->type.u8_));
  env_insert_builtin_value(env, STR("u16"), alloc_type(types->type.u16_));
  env_insert_builtin_value(env, STR("u32"), alloc_type(types->type.u32_));
  env_insert_builtin_value(env, STR("u64"), alloc_type(types->type.u64_));

  env_insert_builtin_value(env, STR("uint"), alloc_type(types->type.uint));

  env_insert_builtin_value(env, STR("bool"),  alloc_type(types->type.bool_));
  env_insert_builtin_value(env, STR("nil"),   alloc_type(types->type.nil));
  env_insert_builtin_value(env, STR("never"), alloc_type(types->type.never));
  env_insert_builtin_value(env, STR("type"),  alloc_type(types->type.type));

  env_insert_builtin_value(env, STR("true"),  alloc_bool(1));
  env_insert_builtin_value(env, STR("false"), alloc_bool(0));
  // clang-format on
}

void Builder::env_insert_builtin_value(Env<Declaration> *env, Str identifier, ValueIndex value) {
  env->insert(
    strings->add(identifier),
    {
      .scope          = Scope_builtin,
      .resolve_status = ResolveStatus_resolved,
      .is_const       = true,
      .value          = value,
    }
  );
}

ValueIndex Builder::alloc_type(TypeIndex type_idx) {
  auto type_of_val = types->type.type;

  Value *val;
  auto   idx  = values->alloc_value(&val);
  auto   data = values->alloc_data(types->size_info(type_of_val));

  memcpy(data, &type_idx, sizeof(TypeIndex));

  *val = {
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

  *val = {
    .type = type_of_val,
    .data = data,
  };

  return idx;
}

b32 Builder::check_and_resolve_unification(
  NodeIndex *lhs, NodeIndex *rhs, TypeIndex *type_unified
) {
  TypeIndex type_lhs = get_type(*lhs);
  TypeIndex type_rhs = get_type(*rhs);

  TypeIndex unified;
  if (!types->unify(type_lhs, type_rhs, &unified)) {
    messages->error("Unable to unify types {type} and {type}.", type_lhs, type_rhs);
    return false;
  }

  if (type_lhs != unified) {
    insert_cast(lhs, unified);
  }

  if (type_rhs != unified) {
    insert_cast(rhs, unified);
  }

  *type_unified = unified;

  return true;
}

void Builder::insert_cast(NodeIndex *value, TypeIndex type_dst) {
  auto old_node_index = *value;
  auto ast_index      = nodes->alloc();
  *value              = NodeIndex::from_ast_index(ast_index);

  auto val_idx = alloc_type(type_dst);

  AstCast cast;
  cast.type_dst = NodeIndex::from_value(val_idx);
  cast.value    = old_node_index;

  nodes->type(ast_index) = type_dst;
  nodes->set(
    ast_index,
    {
      Ast_cast,
      {{0}, {0}}, // TODO replace this with something else. or maybe these nodes shouldn't live in
                  // `AstNodes`
      {.cast = cast},
    }
  );
}

b32 Builder::check_and_resolve_coercion(TypeHint expected, NodeIndex *value) {
  if (expected.is_none()) {
    return true;
  }

  auto type_src = get_type(*value);

  if (type_src == expected.type) {
    return true;
  }

  if (!types->is_coercible_to(type_src, expected.type)) {
    messages->error(*value, "Cannot coerce a {type} to a {type}.", type_src, expected.type);
    return false;
  }

  insert_cast(value, expected.type);

  return true;
}

TypeIndex Builder::get_type(NodeIndex node_index) {
  if (node_index.kind == NodeIndex_ast) {
    return nodes->type(node_index.as_ast_idx());
  } else {
    Value *v = values->get(node_index.as_value_idx());
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

    memcpy(data, val->data, sizeof(AstIndex));

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
    } else if (
      type_dst->integer.signedness == Unsigned && type_src->integer.signedness == Unsigned
    ) {
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
