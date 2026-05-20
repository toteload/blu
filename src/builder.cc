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
      auto &data = nodes->data(idx_item);
      StrKey key; Todo();

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
    auto idx_item = root.items[i];

    Try(resolve_declaration(env_root, idx_item));
  }
}

b32 Builder::resolve_declaration(Env<Declaration> *env, NodeIndex idx) {
  Assert(nodes->kind(idx) == Ast_declaration);

  auto &data = nodes->data(idx);
  auto  key  = intern_identifier(data.declaration.name);

  {
    Declaration *decl;
    auto         found = env->lookup_ptr(key, &decl);

    Assert(found);

    if (decl->resolve_status == ResolveStatus_type_resolved) {
      return true;
    }

    if (decl->resolve_status == ResolveStatus_type_resolving) {
      Todo();
      return false;
    }

    decl->resolve_status = ResolveStatus_type_resolving;
  }

  ValueIndex value_declared_type;
  Try(check_and_eval_type_expression(env, data.declaration.type, &value_declared_type));

  Try(check_expression(env, data.declaration.value));

  TypeIndex type_declared;
  copy_value_data(value_declared_type, &type_declared);

  Try(resolve_possible_coercion(env, type_declared, &data.declaration.value));

  Todo();

  return true;
}

b32 Builder::eval_expression(Env<Declaration> *env, NodeIndex node_index, ValueIndex *result) {
  if (node_index.kind == NodeIndex_value) {
    *result = node_index.idx.value;
    return true;
  }

  auto kind = nodes->kind(node_index);

  switch (kind) {
  case Ast_cast: {
    auto node = nodes->data(node_index).cast;

    TypeIndex type_dst = get_type(node.type_dst);

    ValueIndex val_idx;
    Try(eval_expr(env, node.value, &val_idx));
    Try(eval_cast(type_dst, val_idx, result));
  } break;

  case Ast_builtin: {
    auto builtin = nodes->data(node_index).builtin;
    switch (builtin.kind) {
    case Builtin_print: {
      ValueIndex arg_format;
      Try(eval_expr(env, builtin.args[0], &arg_format));

      ValueSlice slice;
      copy_value_data(arg_format, &slice);

      auto snapshot = work_arena.take_snapshot();
      defer(work_arena.restore(snapshot));

      auto args = work_arena.alloc_slice<ValueIndex>(builtin.args.len() - 1);
      for (u32 i = 1; i < builtin.args.len(); i++) {
        Try(eval_expr(env, builtin.args[i], &args[i - 1]));
      }

      Str format = Str::from_ptr_and_len(cast<char const *>(slice.items), slice.len);

      builtin_print(format, args);

      *result = common.nil;
    } break;
    }
  } break;

  case Ast_block: {
    auto block = nodes->data(node_index).block;
    if (block.items.len() == 0) {
      *result = common.nil;
      break;
    }

    auto env_block = envs.alloc(env);
    defer(envs.dealloc(env_block));

    auto snapshot = work_arena.take_snapshot();
    defer(work_arena.restore(snapshot));

    NodeIndex *defers      = work_arena.alloc<NodeIndex>(block.items.len());
    u32        defer_count = 0;

    ValueIndex last = common.nil;

    for (u32 i = 0; i < block.items.len(); i++) {
      auto item = block.items[i];
      if (nodes->kind(item) == Ast_defer) {
        defers[defer_count++] = nodes->data(item).defer.value;
        last                  = common.nil;
      } else {
        Try(eval_expr(env_block, item, &last));
      }
    }

    *result = last;

    for (u32 i = defer_count; i > 0; i--) {
      ValueIndex e;
      Try(eval_expr(env_block, defers[i - 1], &e));
    }
  } break;

  case Ast_function: {
    Value *v;
    *result   = values->alloc_value(&v);
    auto data = values->alloc_data<NodeIndex>();

    *data = node_index;

    *v = {
      .type = get_type(node_index),
      .data = data,
    };
  } break;

  case Ast_identifier: {
    *result = lookup_identifier(env, node_index);
  } break;

  case Ast_if_else: {
    auto if_else = nodes->data(node_index).if_else;

    ValueIndex cond;
    Try(eval_expr(env, if_else.cond, &cond));

    auto v = values->get(cond);

    if (*cast<u8 *>(v->data) == 1) {
      Try(eval_expr(env, if_else.then, result));
    } else if (if_else.otherwise.is_some()) {
      Try(eval_expr(env, if_else.otherwise, result));
    } else {
      *result = common.nil;
    }
  } break;

  case Ast_declaration: {
    Try(eval_declaration(env, node_index, result));
  } break;

  case Ast_binary_op: {
    auto binop = nodes->data(node_index).binary_op;
    auto op    = binop.kind;

    ValueIndex lhs;
    Try(eval_expr(env, binop.lhs, &lhs));

    ValueIndex rhs;
    Try(eval_expr(env, binop.rhs, &rhs));

    Try(eval_binary_op(op, lhs, rhs, node_index, result));
  } break;

  case Ast_index: {
    auto node = nodes->data(node_index).index;

    ValueIndex idx_indexable;
    Try(eval_expr(env, node.indexable, &idx_indexable));

    ValueIndex idx_index_at;
    Try(eval_expr(env, node.index_at, &idx_index_at));

    u64 i;
    Todo();

    auto indexable      = values->get(idx_indexable);
    auto type_indexable = types->get(indexable->type);

    TypeIndex base_type;
    void     *elem_ptr;

    if (type_indexable->kind == Type_slice) {
      base_type  = type_indexable->slice.base_type;
      auto slice = cast<ValueSlice *>(indexable->data);
      elem_ptr   = ptr_offset(slice->items, i * types->size_info(base_type).stride);
    } else if (type_indexable->kind == Type_array) {
      base_type = type_indexable->array.base_type;
      elem_ptr  = ptr_offset(indexable->data, i * types->size_info(base_type).stride);
    } else {
      Todo("implement indexing for other types");
      break;
    }

    auto elem_size_info = types->size_info(base_type);
    auto data           = values->alloc_data(elem_size_info);
    memcpy(data, elem_ptr, elem_size_info.size);

    Value *vout;
    *result = values->alloc_value(&vout);
    *vout   = {.type = base_type, .data = data};
  } break;

  case Ast_literal_string: {
    auto ty    = get_type(node_index);
    auto t     = types->get(ty);
    u32  count = t->array.size;

    Value *v;
    auto   res   = values->alloc_value(&v);
    void  *bytes = values->alloc_data(types->size_info(t->array.base_type), count);

    *v = {
      .type = ty,
      .data = bytes,
    };

    auto token_index = nodes->data(node_index).literal_string.token_index;
    auto literal     = get_token_str(token_index);

    decode_string_literal(literal, cast<char *>(bytes));

    *result = res;
  } break;

  case Ast_defer: {
    auto       defer_ = nodes->data(node_index).defer;
    ValueIndex e;
    Try(eval_expr(env, defer_.value, &e));
    *result = common.nil;
  } break;

  case Ast_call: {
    auto call = nodes->data(node_index).call;

    ValueIndex callee_idx;
    Try(eval_expr(env, call.callee, &callee_idx));

    u32 arg_count = cast<u32>(call.args.len());

    auto snapshot = work_arena.take_snapshot();
    defer(work_arena.restore(snapshot));

    ValueIndex *args = work_arena.alloc<ValueIndex>(arg_count);
    for (u32 i = 0; i < arg_count; i++) {
      Try(eval_expr(env, call.args[i], &args[i]));
    }

    auto callee = values->get(callee_idx);
    Try(eval_call(env, callee, {args, arg_count}, result));
  } break;

  case Ast_for: {
    auto node = nodes->data(node_index).for_;

    ValueIndex iterable_idx;
    Try(eval_expr(env, node.iterable, &iterable_idx));

    auto iterable      = values->get(iterable_idx);
    auto iterable_type = types->get(iterable->type);

    u64       count;
    void     *items;
    TypeIndex element_type;

    if (iterable_type->kind == Type_array) {
      count        = iterable_type->array.size;
      items        = iterable->data;
      element_type = iterable_type->array.base_type;
    } else if (iterable_type->kind == Type_slice) {
      auto slice   = cast<ValueSlice *>(iterable->data);
      count        = slice->len;
      items        = slice->items;
      element_type = iterable_type->slice.base_type;
    } else {
      Unreachable();
    }

    auto elem_size_info = types->size_info(element_type);

    auto iterator_token = nodes->data(node.iterator).identifier.token_index;
    auto iter_str       = get_token_str(iterator_token);
    auto iter_key       = strings->add(iter_str);

    auto env_loop = envs.alloc(env);
    defer(envs.dealloc(env_loop));

    for (u64 i = 0; i < count; i++) {
      Value *v;
      auto   iter_value_idx = values->alloc_value(&v);
      auto   data           = values->alloc_data(elem_size_info);
      memcpy(data, ptr_offset(items, i * elem_size_info.stride), elem_size_info.size);
      *v = {.type = element_type, .data = data};

      env_loop->insert(iter_key, iter_value_idx);

      ValueIndex body_result;
      Try(eval_expr(env_loop, node.body, &body_result));
    }

    *result = common.nil;
  } break;

  case Ast_assign: {
    auto node = nodes->data(node_index).assign;

    Assert(node.kind == Assign_normal);

    void     *lhs_ptr;
    TypeIndex lhs_type;
    Try(eval_place(env, node.lhs, &lhs_ptr, &lhs_type));

    ValueIndex value_idx;
    Try(eval_expr(env, node.value, &value_idx));

    copy_value(lhs_type, value_idx, lhs_ptr);

    *result = common.nil;
  } break;

  // Should only be called during const evaluation.
  case Ast_literal_int: {
    auto token_index = nodes->data(node_index).literal_int.token_index;
    auto str         = get_token_str(token_index);
    auto i           = parse_i64(str);

    Value *v;
    *result   = values->alloc_value(&v);
    auto data = values->alloc_data<i64>();
    *data     = i;

    *v = {
      .type = types->type.literal_int,
      .data = data,
    };
  } break;

  case Ast_literal_sequence: {
    auto seq   = nodes->data(node_index).literal_sequence;
    auto count = seq.items.len();

    Value      *v;
    auto        res   = values->alloc_value(&v);
    ValueIndex *items = values->alloc_data<ValueIndex>(count);

    *v = {
      .type = get_type(node_index),
      .data = items,
    };

    for (u32 i = 0; i < count; i++) {
      Try(eval_expr(env, seq.items[i], &items[i]));
    }

    *result = res;
  } break;

  case Ast_const: {
    Todo("should not exist at runtime");
    // return eval_expr(env, nodes->data(node_index).const_.expr, result);
  } break;

  case Ast_type_function: {
    auto &param_types = data.type_function.param_types;

    auto snapshot = arena_tmp.take_snapshot();
    defer(arena_tmp.restore(snapshot));

    Type *t = alloc_type_function(arena_tmp, param_types.len());

    t->function.param_count = param_types.len();

    ValueIndex value_return_type;
    Try(check_and_eval_type_expression(env, data.type_function.return_type, &value_return_type));
    copy_value_data(value_return_type, &t->function.return_type);

    for (u32 i = 0; i < param_types.len(); i++) {
      ValueIndex value_param;
      Try(check_and_eval_type_expression(env, param_types[i], &value_param));
      copy_value_data(value_param, t->function.param_types[i]);
    }

    TypeIndex type_idx = types->add(t);
    *result = alloc_type(type_idx);
  } break;

  case Ast_type_slice:
  case Ast_type_array:
  case Ast_unary_op:
  case Ast_kind_max:
  case Ast_root:
    Todo();
  }
}

b32 Builder::check_and_eval_type_expression(Env<Declaration> *env, NodeIndex node_index, ValueIndex *result) {
  Try(check_expression(env, node_index));

  ValueIndex value_declared_type;
  Try(eval_expression(env, node_index, &value_declared_type));
  Try(check_is_expected_type(node_index, types->type.type, value_declared_type->type)); 

  *result = value_declared_type;

  return true;
}

b32 Builder::check_expression(Env<Declaration> *env, NodeIndex node_index, TypeHint hint) {
  auto kind = nodes->kind(node_index);
  auto &data = nodes->data(node_index);

  switch (kind) {
  case Ast_cast: {
    ValueIndex value_type_dst;
    Try(check_and_eval_type_expression(env, data.cast.type_dst, &value_type_dst));

    Try(check_expression(env, data.cast.value));

    TypeIndex type_expr = get_type(data.cast.value);

    TypeIndex type_dst;
    copy_value_data(value_type_dst, &type_dst);

    Try(check_is_valid_cast(node_index, type_dst, type_expr));

    data.cast.type_dst = NodeIndex::from_value(value_type_dst);

    nodes->type(node_index) = type_dst;
  } break;

  case Ast_const: {
    Try(check_expression(env, data.const.expr));

    ValueIndex val;
    Try(eval_expression(env, data.const.expr, &val));

    data.const.expr = NodeIndex::from_value(val);

    nodes->type(node_index) = get_type(data.const.expr);
  } break;

  case Ast_assign: {
    Assert(data.assign.kind == Assign_normal);

    Try(check_expression(env, node.lhs));
    Try(check_is_assignable(node.lhs));

    TypeIndex lhs_type = get_type(node.lhs);

    TypeHint hint_value = {
      .type     = lhs_type,
      .location = node.lhs,
    };

    Try(check_expression(env, node.value, hint_value));

    Try(resolve_possible_coercion(env, lhs_type, &node.value));

    nodes->type(node_index) = types->type.nil;
  } break;

  case Ast_type_function: {
    Todo();
    // TODO recurse on its individual bits and make sure they are all types
    nodes->type(node_index) = types->type.type;
  } break;

  }

  return true;
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
