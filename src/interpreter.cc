#include "blu.hh"
#include "utils/stdlib.hh"

struct PopulateRootEnv {
  StringInterner  *strings;
  TypeInterner    *types;
  ValueStore      *values;
  Env<ValueIndex> *env;

  template<typename T> void insert(Str s, TypeIndex type, T val) {
    auto key = strings->add(s);

    Value *v;
    auto   idx  = values->alloc_value(&v);
    auto   data = values->alloc_data<T>();
    *data       = val;
    *v          = {
               .type = type,
               .data = data,
    };

    env->insert(key, idx);
  }

  void populate();
};

void PopulateRootEnv::populate() {
  auto type_type = types->type.type;

  // clang-format off
  insert<TypeIndex>(STR("i8"),  type_type, types->type.i8_);
  insert<TypeIndex>(STR("i16"), type_type, types->type.i16_);
  insert<TypeIndex>(STR("i32"), type_type, types->type.i32_);
  insert<TypeIndex>(STR("i64"), type_type, types->type.i64_);

  insert<TypeIndex>(STR("u8"),  type_type, types->type.u8_);
  insert<TypeIndex>(STR("u16"), type_type, types->type.u16_);
  insert<TypeIndex>(STR("u32"), type_type, types->type.u32_);
  insert<TypeIndex>(STR("u64"), type_type, types->type.u64_);

  insert<TypeIndex>(STR("nil"),   type_type, types->type.nil);
  insert<TypeIndex>(STR("never"), type_type, types->type.never);
  insert<TypeIndex>(STR("type"),  type_type, types->type.type);
  insert<TypeIndex>(STR("bool"),  type_type, types->type.bool_);

  insert<u8>(STR("true"),  types->type.bool_, 1);
  insert<u8>(STR("false"), types->type.bool_, 0);
  // clang-format on
}

void Interpreter::init(InterpreterContext *context) {
  work_arena.init(MiB(1));
  values.init(stdlib_alloc);
  envs.init(stdlib_alloc, stdlib_alloc);

  env_root = nullptr;

  types      = context->types;
  strings    = context->strings;
  messages   = context->messages;
  text       = context->text;
  tokens     = context->tokens;
  nodes      = context->nodes;
  node_types = context->node_types;

  {
    Value *v;
    common.nil = values.alloc_value(&v);

    *v = {
      .type = types->type.nil,
      .data = nullptr,
    };
  }
}

void Interpreter::deinit() {
  if (env_root) {
    auto at = env_root;
    while (at) {
      auto parent = at->parent;
      envs.dealloc(at);
      at = parent;
    }
  }

  envs.deinit();
  values.deinit();
  work_arena.deinit();

  memset(this, 0, sizeof(*this));
}

bool Interpreter::prepare_code() {
  Env<ValueIndex> *env_builtin = envs.alloc(nullptr);
  {
    PopulateRootEnv populator = {
      .strings = strings,
      .types   = types,
      .values  = &values,
      .env     = env_builtin,
    };

    populator.populate();
  }

  env_root = envs.alloc(env_builtin);

  NodeIndex root_idx = nodes->first_valid_index();
  Assert(nodes->kind(root_idx) == Ast_root);

  // Add root declarations to env.
  auto &root_items = nodes->datas[root_idx.idx].root.items;
  for (u32 i = 0; i < root_items.len(); i++) {
    Try(add_declaration(env_root, root_items[i]));
  }

  // Insert explicit cast for coercions.
  for (u32 i = 0; i < root_items.len(); i++) {
    Try(coercion_resolve_walk(&root_items[i]));
  }

  // Evaluate all the const code.
  for (u32 i = 0; i < root_items.len(); i++) {
    Try(const_walk(env_root, &root_items[i]));
  }

  return true;
}

bool Interpreter::run_main(ValueIndex *result) {
  auto main = values.get(_lookup(env_root, STR("main")).as_index());

  TypeIndex main_function_type;
  {
    auto snapshot = work_arena.take_snapshot();
    defer(work_arena.restore(snapshot));

    Type *t = alloc_type_function(&work_arena, 0);

    *t = {
      .kind     = Type_function,
      .function = {
        .return_type = types->type.i32_,
        .param_count = 0,
        .param_types = {},
      },
    };

    main_function_type = types->add(t);
  }

  Try(call_function(env_root, main, {}, result));

  return true;
}

b32 Interpreter::coercion_resolve_walk(NodeIndex *node) {
  Assert(node->kind == NodeIndex_ast_node);

  auto kind = nodes->kind(*node);
  auto data = nodes->data(*node);

  switch (kind) {
  case Ast_declaration: {
    auto declared_type = get_type(data.declaration.type);
    resolve_possible_coercion(declared_type, &data.declaration.value);
  } break;

  case Ast_if_else: {
    resolve_possible_coercion(types->type.bool_, &data.if_else.cond);
    if (data.if_else.otherwise.is_some()) {
      TypeIndex type_expr = get_type(*node);
      resolve_possible_coercion(type_expr, &data.if_else.then);
      resolve_possible_coercion(type_expr, &data.if_else.otherwise);
    }
  } break;

  case Ast_assign: {
    auto type_lhs = get_type(data.assign.lhs);
    resolve_possible_coercion(type_lhs, &data.assign.value);
  } break;

  case Ast_index: {
    resolve_possible_coercion(types->type.uint, &data.index.index_at);
  } break;

  case Ast_binary_op: {
    auto op_kind = data.binary_op.kind;

    switch (op_kind) {
    case Cmp_equal:
    case Cmp_not_equal:
    case Cmp_less_than:
    case Cmp_less_equal:
    case Cmp_greater_than:
    case Cmp_greater_equal: {
      TypeIndex t;
      types->unify(get_type(data.binary_op.lhs), get_type(data.binary_op.rhs), &t);
      resolve_possible_coercion(t, &data.binary_op.lhs);
      resolve_possible_coercion(t, &data.binary_op.lhs);
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
      TypeIndex type_expr = get_type(*node);
      resolve_possible_coercion(type_expr, &data.binary_op.lhs);
      resolve_possible_coercion(type_expr, &data.binary_op.rhs);
    } break;

    case Logical_and:
    case Logical_or:
    case BinaryOpKind_max:
      break;
    }
  } break;

  case Ast_call: {
    TypeIndex type_idx_callee = get_type(data.call.callee);
    auto type_callee = types->get(type_idx_callee);
    Assert(type_callee->kind == Type_function);
    for (u32 i = 0; i < type_callee->function.param_count; i++) {
      resolve_possible_coercion(type_callee->function.param_types[i], &data.call.args[i]);
    }
  } break;

  case Ast_function: {
  } break;
  case Ast_block: {
  } break;

  case Ast_identifier:
  case Ast_type_slice:
  case Ast_type_array:
  case Ast_type_function:
  case Ast_builtin:
  case Ast_unary_op:
  case Ast_root:
  case Ast_literal_int:
  case Ast_literal_string:
  case Ast_literal_sequence:
  case Ast_for:
  case Ast_defer:
  case Ast_const:
  case Ast_cast:
    break;

  case Ast_kind_max: {
    Todo();
  } break;
  }

  switch (kind) {
  case Ast_declaration: {
    coercion_resolve_walk(&data.declaration.value);
  } break;
  case Ast_if_else: {
    coercion_resolve_walk(&data.if_else.cond);
    coercion_resolve_walk(&data.if_else.then);
    coercion_resolve_walk(&data.if_else.otherwise);
  } break;
  case Ast_assign: {
    coercion_resolve_walk(&data.assign.lhs);
    coercion_resolve_walk(&data.assign.value);
  } break;
  case Ast_index: {
    coercion_resolve_walk(&data.index.indexable);
    coercion_resolve_walk(&data.index.index_at);
  } break;
  case Ast_binary_op: {
    coercion_resolve_walk(&data.binary_op.lhs);
    coercion_resolve_walk(&data.binary_op.rhs);
  } break;

  case Ast_call: {
    coercion_resolve_walk(&data.call.callee);
    for (u32 i = 0; i < data.call.args.len(); i++) {
      coercion_resolve_walk(&data.call.args[i]);
    }
  } break;

  case Ast_unary_op: {
    coercion_resolve_walk(&data.unary_op.value);
  } break;

  case Ast_block: {
    for (u32 i = 0; i < data.block.items.len(); i++) {
      coercion_resolve_walk(&data.block.items[i]);
    }
  } break;

  case Ast_function: {
    coercion_resolve_walk(&data.function.body);
  } break;

  case Ast_builtin: {
  } break;

  case Ast_for: {
    coercion_resolve_walk(&data.for_.iterable);
    coercion_resolve_walk(&data.for_.body);
  } break;

  case Ast_defer: {
    coercion_resolve_walk(&data.defer.value);
  } break;
  case Ast_const: {
    coercion_resolve_walk(&data.const_.expr);
  } break;
  case Ast_cast: {
    coercion_resolve_walk(&data.cast.value);
  } break;
  case Ast_literal_sequence: {
    for (u32 i = 0; i < data.literal_sequence.items.len(); i++) {
      coercion_resolve_walk(&data.literal_sequence.items[i]);
    }
  } break;

  case Ast_type_function:
  case Ast_type_array:
  case Ast_type_slice:
  case Ast_identifier:
  case Ast_literal_int:
  case Ast_literal_string:
    break;

  case Ast_root:
  case Ast_kind_max: {
    Todo();
  } break;
  }

  return true;
}

void Interpreter::resolve_possible_coercion(TypeIndex type_dst, NodeIndex *node) {
  Assert(node->kind == NodeIndex_ast_node);

  auto type = get_type(*node);

  if (type == type_dst) {
    return;
  }

  insert_cast_to(type_dst, node);
}

// Assume: The coercion is always possible.
bool Interpreter::coerce_value(TypeIndex type_dst, ValueIndex src, void *out) {
  auto v      = values.get(src);
  auto ty_src = types->get(v->type);

  if (type_dst == v->type) {
    memcpy(out, v->data, types->size_info(v->type).size);
    return true;
  }

  auto ty_dst = types->get(type_dst);

  if (ty_src->kind == Type_literal_function && ty_dst->kind == Type_function) {
    memcpy(out, v->data, sizeof(NodeIndex));
    return true;
  }

  if (ty_src->kind == Type_literal_int && ty_dst->kind == Type_integer) {
    i64  val        = *cast<i64 *>(v->data);
    auto signedness = ty_dst->integer.signedness;
    auto bitwidth   = ty_dst->integer.bitwidth;

    if (signedness == Signed) {
      i64 min_value = int_value_min(bitwidth);
      i64 max_value = int_value_max(bitwidth);
      if (val < min_value || val > max_value) {
        messages->error("integer constant out of range of destination type.");
        return false;
      }
    } else {
      u64 max_value = uint_value_max(bitwidth);
      if (val < 0 || cast<u64>(val) > max_value) {
        messages->error("integer constant out of range of destination type.");
        return false;
      }
    }

    // clang-format off
    switch (ty_dst->integer.bitwidth) {
    case  8: signedness == Signed ? (*cast<i8  *>(out) = cast<i8 >(val)) : (*cast<u8  *>(out) = cast<u8 >(val)); break;
    case 16: signedness == Signed ? (*cast<i16 *>(out) = cast<i16>(val)) : (*cast<u16 *>(out) = cast<u16>(val)); break;
    case 32: signedness == Signed ? (*cast<i32 *>(out) = cast<i32>(val)) : (*cast<u32 *>(out) = cast<u32>(val)); break;
    case 64: signedness == Signed ? (*cast<i64 *>(out) = cast<i64>(val)) : (*cast<u64 *>(out) = cast<u64>(val)); break;
    default: Unreachable();
    }
    // clang-format on

    return true;
  }

  if (ty_src->kind == Type_integer && ty_dst->kind == Type_integer) {
    auto signedness = ty_dst->integer.signedness;
    if (signedness == Signed) {
      if (ty_src->integer.bitwidth == 8 && ty_dst->integer.bitwidth == 32) {
        *cast<i32 *>(out) = cast<i32>(*cast<i8 *>(v->data));
        return true;
      }

      if (ty_src->integer.bitwidth == 32 && ty_dst->integer.bitwidth == 64) {
        *cast<i64 *>(out) = cast<i64>(*cast<i32 *>(v->data));
        return true;
      }
    } else {
      if (ty_src->integer.bitwidth == 32 && ty_dst->integer.bitwidth == 64) {
        *cast<u64 *>(out) = cast<u64>(*cast<u32 *>(v->data));
        return true;
      }
    }

    Todo();

    return false;
  }

  if (ty_src->kind == Type_array && ty_dst->kind == Type_slice) {
    u32 count = ty_src->array.size;

    *cast<ValueSlice *>(out) = {
      .len   = count,
      .items = v->data,
    };

    return true;
  }

  if (ty_src->kind == Type_sequence) {
    TypeIndex base_type;
    if (ty_dst->kind == Type_slice) {
      base_type = ty_dst->slice.base_type;
    } else if (ty_dst->kind == Type_array) {
      base_type = ty_dst->array.base_type;
    } else {
      Unreachable();
    }

    u32  count     = ty_src->sequence.count;
    auto size_info = types->size_info(base_type);

    ValueIndex *sequence_items = cast<ValueIndex *>(v->data);

    auto items = values.alloc_data(size_info, count);
    for (u32 i = 0; i < count; i++) {
      Try(coerce_value(base_type, sequence_items[i], ptr_offset(items, size_info.stride * i)));
    }

    if (ty_dst->kind == Type_array) {
      *cast<void **>(out) = items;
    } else {
      *cast<ValueSlice *>(out) = {.len = count, .items = items};
    }

    return true;
  }

  Todo();

  return false;
}

TypeIndex Interpreter::slot_type(NodeIndex slot) {
  if (slot.kind == NodeIndex_value) {
    return values.get(ValueIndex{slot.idx})->type;
  }

  return get_type(slot);
}

b32 Interpreter::coerce_slot_to(NodeIndex *node, TypeIndex dst_type) {
  Assert(node->is_some());

  if (node->kind == NodeIndex_value) {
    ValueIndex val_idx{node->idx};
    Value     *v = values.get(val_idx);

    if (v->type == dst_type) {
      return true;
    }

    if (!types->is_coercible_to(v->type, dst_type)) {
      return true;
    }

    Value *new_v;
    auto   new_idx = values.alloc_value(&new_v);
    auto   data    = values.alloc_data(types->size_info(dst_type));
    *new_v         = {.type = dst_type, .data = data};

    Try(coerce_value(dst_type, val_idx, data));

    *node = NodeIndex{NodeIndex_value, new_idx.idx};

    return true;
  }

  auto kind = nodes->kind(*node);
  switch (kind) {
  case Ast_block: {
    auto &block = nodes->datas[node->idx].block;
    if (block.items.len() == 0) {
      break;
    }

    Try(coerce_slot_to(&block.items[block.items.len() - 1], dst_type));
  } break;
  case Ast_if_else: {
    auto &ie = nodes->datas[node->idx].if_else;
    Try(coerce_slot_to(&ie.then, dst_type));
    if (ie.otherwise.is_some()) {
      Try(coerce_slot_to(&ie.otherwise, dst_type));
    }
  } break;
  default: {
    TypeIndex type_src = get_type(*node);
    if (type_src == dst_type) {
      break;
    }

    // We got through the type checker so this cast must be valid.
    insert_cast_to(dst_type, node);
  } break;
  }

  return true;
}

b32 Interpreter::add_declaration(Env<ValueIndex> *env, NodeIndex declaration) {
  Assert(nodes->kind(declaration) == Ast_declaration);

  auto decl = nodes->data(declaration).declaration;

  TypeIndex decl_type = get_type(decl.type);

  ValueIndex decl_value;
  Try(eval_expr(env, decl.value, &decl_value));

  Value *v;
  auto   val  = values.alloc_value(&v);
  auto   data = values.alloc_data(types->size_info(decl_type));
  *v          = {
             .type = decl_type,
             .data = data,
  };

  Try(coerce_value(decl_type, decl_value, data));

  auto str = get_token_str(decl.name);
  auto key = strings->add(str);

  env->insert(key, val);

  return true;
}

b32 Interpreter::eval_expr(Env<ValueIndex> *env, NodeIndex node_index, ValueIndex *result) {
  if (node_index.kind == NodeIndex_value) {
    *result = ValueIndex{node_index.idx};
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

  case Ast_literal_int: {
    auto token_index = nodes->data(node_index).literal_int.token_index;
    auto str         = get_token_str(token_index);
    auto i           = parse_i64(str);

    Value *v;
    *result   = values.alloc_value(&v);
    auto data = values.alloc_data<i64>();
    *data     = i;

    *v = {
      .type = types->type.literal_int,
      .data = data,
    };
  } break;
  case Ast_builtin: {
    auto builtin = nodes->data(node_index).builtin;
    switch (builtin.kind) {
    case Builtin_print: {
      ValueIndex arg_format;
      Try(eval_expr(env, builtin.args[0], &arg_format));

      ValueSlice slice;
      Try(coerce_value(types->type.slice_u8, arg_format, &slice));

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
    *result   = values.alloc_value(&v);
    auto data = values.alloc_data<NodeIndex>();
    *data     = node_index;
    *v        = {
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

    auto v = values.get(cond);

    if (*cast<u8 *>(v->data) == 1) {
      Try(eval_expr(env, if_else.then, result));
    } else if (if_else.otherwise.is_some()) {
      Try(eval_expr(env, if_else.otherwise, result));
    } else {
      *result = common.nil;
    }
  } break;
  case Ast_declaration: {
    Try(add_declaration(env, node_index));
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
  case Ast_literal_sequence: {
    auto seq   = nodes->data(node_index).literal_sequence;
    auto count = seq.items.len();

    Value      *v;
    auto        res   = values.alloc_value(&v);
    ValueIndex *items = values.alloc_data<ValueIndex>(count);

    *v = {
      .type = get_type(node_index),
      .data = items,
    };

    for (u32 i = 0; i < count; i++) {
      Try(eval_expr(env, seq.items[i], &items[i]));
    }

    *result = res;
  } break;
  case Ast_index: {
    auto node = nodes->data(node_index).index;

    ValueIndex idx_indexable;
    Try(eval_expr(env, node.indexable, &idx_indexable));

    ValueIndex idx_index_at;
    Try(eval_expr(env, node.index_at, &idx_index_at));

    u64 i = get_uint(idx_index_at);

    auto indexable      = values.get(idx_indexable);
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
    auto data           = values.alloc_data(elem_size_info);
    memcpy(data, elem_ptr, elem_size_info.size);

    Value *vout;
    *result = values.alloc_value(&vout);
    *vout   = {.type = base_type, .data = data};
  } break;
  case Ast_literal_string: {
    auto ty    = get_type(node_index);
    auto t     = types->get(ty);
    u32  count = t->array.size;

    Value *v;
    auto   res   = values.alloc_value(&v);
    void  *bytes = values.alloc_data(types->size_info(t->array.base_type), count);

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

  case Ast_const: {
    Todo("should not exist at runtime");
    // return eval_expr(env, nodes->data(node_index).const_.expr, result);
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

    auto callee = values.get(callee_idx);
    Try(call_function(env, callee, {args, arg_count}, result));
  } break;

  case Ast_for: {
    auto node = nodes->data(node_index).for_;

    ValueIndex iterable_idx;
    Try(eval_expr(env, node.iterable, &iterable_idx));

    auto iterable      = values.get(iterable_idx);
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
      auto   iter_value_idx = values.alloc_value(&v);
      auto   data           = values.alloc_data(elem_size_info);
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

    Try(coerce_value(lhs_type, value_idx, lhs_ptr));

    *result = common.nil;
  } break;

  case Ast_type_slice:
  case Ast_type_array:
  case Ast_type_function:
  case Ast_unary_op:
  case Ast_kind_max:
  case Ast_root:
    Todo();
    return false;
  }

  return true;
}

b32 Interpreter::const_walk(Env<ValueIndex> *env, NodeIndex *slot) {
  if (slot->kind == NodeIndex_value || slot->is_none()) {
    return true;
  }

  auto idx  = *slot;
  auto kind = nodes->kind(idx);

  switch (kind) {
  case Ast_const: {
    auto inner_slot = &nodes->datas[idx.idx].const_.expr;
    Try(const_walk(env, inner_slot));

    auto inner_idx = *inner_slot;
    bool is_decl   = inner_idx.kind == NodeIndex_ast_node && inner_idx.is_some() &&
                   nodes->kind(inner_idx) == Ast_declaration;

    if (is_decl) {
      Try(add_declaration(env, inner_idx));
      *slot = NodeIndex{NodeIndex_value, common.nil.idx};
    } else {
      ValueIndex val;
      Try(eval_expr(env, inner_idx, &val));
      *slot = NodeIndex{NodeIndex_value, val.idx};
    }
  } break;

  case Ast_literal_int:
  case Ast_literal_string: {
    ValueIndex val;
    Try(eval_expr(env, idx, &val));
    *slot = NodeIndex{NodeIndex_value, val.idx};
  } break;

  case Ast_identifier: {
    auto token_idx = nodes->data(idx).identifier.token_index;
    auto str       = get_token_str(token_idx);
    auto key       = strings->add(str);

    ValueIndex val;
    auto       found_in_const_env = env->lookup(key, &val);

    if (found_in_const_env) {
      Value *v = values.get(val);
      Type  *t = types->get(v->type);
      if (t->kind == Type_literal_function || t->kind == Type_function) {
        break;
      }

      *slot = NodeIndex{NodeIndex_value, val.idx};
    }
  } break;

  case Ast_binary_op: {
    auto &bop = nodes->datas[idx.idx].binary_op;
    Try(const_walk(env, &bop.lhs));
    Try(const_walk(env, &bop.rhs));

    if (bop.lhs.kind == NodeIndex_value && bop.rhs.kind == NodeIndex_value) {
      ValueIndex val;
      Try(eval_expr(env, idx, &val));
      *slot = NodeIndex{NodeIndex_value, val.idx};
      break;
    }

    TypeIndex lhs_t = slot_type(bop.lhs);
    TypeIndex rhs_t = slot_type(bop.rhs);
    TypeIndex unified;
    if (types->unify(lhs_t, rhs_t, &unified)) {
      Try(coerce_slot_to(&bop.lhs, unified));
      Try(coerce_slot_to(&bop.rhs, unified));
    }
  } break;

  case Ast_unary_op: {
    auto &uop = nodes->datas[idx.idx].unary_op;
    Try(const_walk(env, &uop.value));

    if (uop.value.kind == NodeIndex_value) {
      ValueIndex val;
      Try(eval_expr(env, idx, &val));
      *slot = NodeIndex{NodeIndex_value, val.idx};
      break;
    }

    Try(coerce_slot_to(&uop.value, get_type(idx)));
  } break;

  case Ast_cast: {
    Try(const_walk(env, &nodes->datas[idx.idx].cast.value));

    auto v = nodes->data(idx).cast.value;
    if (v.kind == NodeIndex_value) {
      ValueIndex e;
      Try(eval_expr(env, idx, &e));
      *slot = NodeIndex{NodeIndex_value, e.idx};
    }
  } break;

  case Ast_index: {
    auto &index_data = nodes->datas[idx.idx].index;
    Try(const_walk(env, &index_data.indexable));
    Try(const_walk(env, &index_data.index_at));

    Try(coerce_slot_to(&index_data.index_at, types->type.uint));

    if (index_data.indexable.kind == NodeIndex_value &&
        index_data.index_at.kind == NodeIndex_value) {
      ValueIndex val;
      Try(eval_expr(env, idx, &val));
      *slot = NodeIndex{NodeIndex_value, val.idx};
    }
  } break;

  case Ast_literal_sequence: {
    auto &items     = nodes->datas[idx.idx].literal_sequence.items;
    bool  all_const = true;
    for (u32 i = 0; i < items.len(); i++) {
      Try(const_walk(env, &items[i]));
      if (items[i].kind != NodeIndex_value) {
        all_const = false;
      }
    }
    if (all_const) {
      ValueIndex val;
      Try(eval_expr(env, idx, &val));
      *slot = NodeIndex{NodeIndex_value, val.idx};
    }
  } break;

  case Ast_call: {
    auto &call = nodes->datas[idx.idx].call;
    Try(const_walk(env, &call.callee));
    for (u32 i = 0; i < call.args.len(); i++) {
      Try(const_walk(env, &call.args[i]));
    }

    TypeIndex callee_t = slot_type(call.callee);
    Type     *ft       = types->get(callee_t);
    if (ft->kind == Type_function) {
      for (u32 i = 0; i < call.args.len(); i++) {
        Try(coerce_slot_to(&call.args[i], ft->function.param_types[i]));
      }
    }
  } break;

  case Ast_declaration: {
    auto &decl = nodes->datas[idx.idx].declaration;
    Try(const_walk(env, &decl.value));
    Try(coerce_slot_to(&decl.value, get_type(decl.type)));
  } break;

  case Ast_block: {
    auto env_block = envs.alloc(env);
    defer(envs.dealloc(env_block));

    auto &items = nodes->datas[idx.idx].block.items;
    for (u32 i = 0; i < items.len(); i++) {
      Try(const_walk(env_block, &items[i]));
    }
  } break;

  case Ast_function: {
    auto env_fn = envs.alloc(env);
    defer(envs.dealloc(env_fn));
    auto &fn = nodes->datas[idx.idx].function;
    Try(const_walk(env_fn, &fn.body));

    Type *ft = types->get(get_type(idx));
    if (ft->kind == Type_function) {
      Try(coerce_slot_to(&fn.body, ft->function.return_type));
    }
  } break;

  case Ast_if_else: {
    auto &ie = nodes->datas[idx.idx].if_else;
    Try(const_walk(env, &ie.cond));
    Try(const_walk(env, &ie.then));
    Try(const_walk(env, &ie.otherwise));

    Try(coerce_slot_to(&ie.cond, types->type.bool_));

    TypeIndex if_else_t = get_type(idx);
    Try(coerce_slot_to(&ie.then, if_else_t));
    if (ie.otherwise.is_some()) {
      Try(coerce_slot_to(&ie.otherwise, if_else_t));
    }
  } break;

  case Ast_for: {
    Try(const_walk(env, &nodes->datas[idx.idx].for_.iterable));
    auto env_for = envs.alloc(env);
    defer(envs.dealloc(env_for));
    Try(const_walk(env_for, &nodes->datas[idx.idx].for_.body));
  } break;

  case Ast_defer: {
    Try(const_walk(env, &nodes->datas[idx.idx].defer.value));
  } break;

  case Ast_assign: {
    auto &assign = nodes->datas[idx.idx].assign;
    Try(const_walk(env, &assign.value));
    Try(coerce_slot_to(&assign.value, get_type(assign.lhs)));
  } break;

  case Ast_builtin: {
    auto &builtin = nodes->datas[idx.idx].builtin;
    Assert(builtin.kind == Builtin_print);

    for (u32 i = 0; i < builtin.args.len(); i++) {
      Try(const_walk(env, &builtin.args[i]));
    }

    if (builtin.args.len() > 0) {
      Try(coerce_slot_to(&builtin.args[0], types->type.slice_u8));
    }
  } break;

  case Ast_root: {
    auto &items = nodes->datas[idx.idx].root.items;
    for (u32 i = 0; i < items.len(); i++) {
      Try(const_walk(env, &items[i]));
    }
  } break;

  case Ast_type_slice:
  case Ast_type_array:
  case Ast_type_function:
  case Ast_kind_max:
    Todo();
    break;
  }

  return true;
}

b32 Interpreter::eval_binary_op(
  BinaryOpKind op, ValueIndex lhs, ValueIndex rhs, NodeIndex expr, ValueIndex *result
) {
  switch (op) {
  case Cmp_equal:
  case Cmp_not_equal:
  case Cmp_less_than:
  case Cmp_less_equal:
  case Cmp_greater_than:
  case Cmp_greater_equal: {
    auto left      = values.get(lhs);
    auto left_type = types->get(left->type);

    Assert(left_type->is_integer_or_literal_int());

    bool is_unsigned = left_type->kind == Type_integer && left_type->integer.signedness == Unsigned;

    bool res;
    if (is_unsigned) {
      u64 a = get_as_u64(lhs);
      u64 b = get_as_u64(rhs);
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
      i64 a = get_as_i64(lhs);
      i64 b = get_as_i64(rhs);
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
    auto   idx  = values.alloc_value(&v);
    auto   data = cast<u8 *>(values.alloc_data(types->size_info(types->type.bool_)));
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
    auto left  = values.get(lhs);
    auto right = values.get(rhs);

    auto left_type  = types->get(left->type);
    auto right_type = types->get(right->type);

    Assert(left_type->is_integer_or_literal_int() && right_type->is_integer_or_literal_int());

    auto signedness = left_type->integer.signedness;

    if (signedness == Signed) {
      i64  a        = get_as_i64(lhs);
      i64  b        = get_as_i64(rhs);
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
      auto   idx       = values.alloc_value(&v);
      void  *data      = values.alloc_data(size_info);
      u32    byte_size = size_info.size;
      memcpy(data, &res, byte_size);
      *v = {
        .type = result_type_idx,
        .data = data,
      };

      *result = idx;

      return true;
    } else {
      u64  a        = get_as_u64(lhs);
      u64  b        = get_as_u64(rhs);
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
      auto   idx       = values.alloc_value(&v);
      void  *data      = values.alloc_data(size_info);
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

ValueIndex Interpreter::lookup_identifier(Env<ValueIndex> *env, NodeIndex identifier) {
  auto token_index = nodes->data(identifier).identifier.token_index;
  auto str         = get_token_str(token_index);
  auto key         = strings->add(str);

  ValueIndex val;
  auto       found = env->lookup(key, &val);

  Assert(found);

  return val;
}

b32 Interpreter::eval_place(
  Env<ValueIndex> *env, NodeIndex node, void **out_ptr, TypeIndex *out_type
) {
  auto kind = nodes->kind(node);

  if (kind == Ast_identifier) {
    auto val  = lookup_identifier(env, node);
    auto v    = values.get(val);
    *out_ptr  = v->data;
    *out_type = v->type;
    return true;
  }

  if (kind == Ast_index) {
    auto inode = nodes->data(node).index;

    void     *base_ptr;
    TypeIndex base_type;
    Try(eval_place(env, inode.indexable, &base_ptr, &base_type));

    ValueIndex idx_value;
    Try(eval_expr(env, inode.index_at, &idx_value));
    u64 i = get_uint(idx_value);

    auto      bt = types->get(base_type);
    TypeIndex elem_type;
    void     *items;
    if (bt->kind == Type_array) {
      elem_type = bt->array.base_type;
      items     = base_ptr;
    } else if (bt->kind == Type_slice) {
      auto slice = cast<ValueSlice *>(base_ptr);
      elem_type  = bt->slice.base_type;
      items      = slice->items;
    } else {
      Unreachable();
    }

    auto elem_size_info = types->size_info(elem_type);
    *out_ptr            = ptr_offset(items, i * elem_size_info.stride);
    *out_type           = elem_type;
    return true;
  }

  Unreachable();
  return false;
}

b32 Interpreter::call_function(
  Env<ValueIndex> *env, Value *function, Slice<ValueIndex> arguments, ValueIndex *result
) {
  auto node_index    = *cast<NodeIndex *>(function->data);
  auto function_node = nodes->data(node_index).function;
  auto function_type = types->get(function->type);

  Assert(function_type->kind == Type_function);
  Assert(arguments.len() == function_type->function.param_count);

  auto env_args = envs.alloc(env_root);
  defer(envs.dealloc(env_args));

  for (u32 i = 0; i < arguments.len(); i++) {
    auto param_type = function_type->function.param_types[i];

    Value *pv;
    auto   param_value_idx = values.alloc_value(&pv);
    auto   data            = values.alloc_data(types->size_info(param_type));
    *pv                    = {.type = param_type, .data = data};
    Try(coerce_value(param_type, arguments[i], data));

    auto param_node  = function_node.param_names[i];
    auto token_index = nodes->data(param_node).identifier.token_index;
    auto str         = get_token_str(token_index);
    auto key         = strings->add(str);

    env_args->insert(key, param_value_idx);
  }

  ValueIndex body_result;
  Try(eval_expr(env_args, function_node.body, &body_result));

  TypeIndex return_type_idx = function_type->function.return_type;

  Value *v;
  *result   = values.alloc_value(&v);
  auto data = values.alloc_data(types->size_info(return_type_idx));
  *v        = {.type = return_type_idx, .data = data};

  Try(coerce_value(return_type_idx, body_result, data));

  return true;
}

TypeIndex Interpreter::get_type(NodeIndex node_index) {
  if (node_index.kind == NodeIndex_ast_node) {
    return node_types[node_index.idx];
  } else {
    Value *v = values.get({node_index.idx});
    return *cast<TypeIndex *>(v->data);
  }
}

b32 Interpreter::eval_cast(TypeIndex type_idx_dst, ValueIndex val_idx, ValueIndex *result) {
  Value *val = values.get(val_idx);

  if (types->is_coercible_to(val->type, type_idx_dst)) {
    Value *v;
    *result = values.alloc_value(&v);

    auto size_info = types->size_info(type_idx_dst);
    auto data      = values.alloc_data(size_info);

    *v = {
      .type = type_idx_dst,
      .data = data,
    };

    return coerce_value(type_idx_dst, val_idx, data);
  }

  Type *type_dst = types->get(type_idx_dst);
  Type *type_val = types->get(val->type);

  if (type_dst->kind == Type_integer) {
    if (type_val->kind == Type_integer) {
      if (type_dst->integer.signedness == Signed && type_val->integer.signedness == Signed) {
        i64 i = get_as_i64(val_idx);

        i64 lo = int_value_min(type_dst->integer.bitwidth);
        i64 hi = int_value_max(type_dst->integer.bitwidth);

        if (i < lo || i > hi) {
          Todo("invalid cast: value out of range");
        }
      } else if (type_dst->integer.signedness == Unsigned &&
                 type_val->integer.signedness == Signed) {
        i64 i = get_as_i64(val_idx);

        if (i < 0) {
          Todo("invalid cast: value out of range");
        }

        if (type_val->integer.bitwidth > type_dst->integer.bitwidth) {
          u64 u = cast<u64>(i);

          u64 hi = uint_value_max(type_dst->integer.bitwidth);

          if (u > hi) {
            Todo("invalid cast: value out of range");
          }
        }
      } else if (type_dst->integer.signedness == Signed &&
                 type_val->integer.signedness == Unsigned) {
        u64 i = get_as_u64(val_idx);

        u64 hi = cast<u64>(int_value_max(type_dst->integer.bitwidth));

        if (i > hi) {
          Todo("invalid cast: value out of range");
        }
      } else if (type_dst->integer.signedness == Unsigned &&
                 type_val->integer.signedness == Unsigned) {
        u64 i = get_as_u64(val_idx);

        u64 hi = uint_value_max(type_dst->integer.bitwidth);

        if (i > hi) {
          Todo("invalid cast: value out of range");
        }
      } else {
        Unreachable();
      }

      Value *v;
      *result = values.alloc_value(&v);

      auto size_info = types->size_info(type_idx_dst);
      auto data      = values.alloc_data(size_info);

      *v = {
        .type = type_idx_dst,
        .data = data,
      };

      memcpy(data, val->data, size_info.size);

      return true;
    }
  }

  Todo();

  return true;
}

u64 Interpreter::get_uint(ValueIndex idx) {
  u64 res;
  Try(coerce_value(types->type.uint, idx, &res));
  return res;
}

void Interpreter::builtin_print(Str format, Slice<ValueIndex> args) {
  usize i         = 0;
  u32   arg_index = 0;
  while (i < format.len()) {
    char c = format[i];

    // "{{}}" — emit a verbatim "{}"
    if (c == '{' && i + 3 < format.len() && format[i + 1] == '{' && format[i + 2] == '}' &&
        format[i + 3] == '}') {
      fputs("{}", stdout);
      i += 4;
      continue;
    }

    // "{}" — interpolate the next argument
    if (c == '{' && i + 1 < format.len() && format[i + 1] == '}') {
      Assert(arg_index < args.len());

      Value *v = values.get(args[arg_index]);
      Type  *t = types->get(v->type);

      switch (t->kind) {
      case Type_literal_int:
        printf("%lld", *cast<i64 *>(v->data));
        break;
      case Type_integer: {
        if (t->integer.signedness == Signed) {
          i64 val;
          // clang-format off
          switch (t->integer.bitwidth) {
          case 8:  val = *cast<i8 *>(v->data); break;
          case 16: val = *cast<i16 *>(v->data); break;
          case 32: val = *cast<i32 *>(v->data); break;
          case 64: val = *cast<i64 *>(v->data); break;
          default: Unreachable();
          }
          // clang-format on
          printf("%lld", cast<long long>(val));
        } else {
          u64 val;
          // clang-format off
          switch (t->integer.bitwidth) {
          case 8:  val = *cast<u8 *>(v->data); break;
          case 16: val = *cast<u16 *>(v->data); break;
          case 32: val = *cast<u32 *>(v->data); break;
          case 64: val = *cast<u64 *>(v->data); break;
          default: Unreachable();
          }
          // clang-format on
          printf("%llu", cast<unsigned long long>(val));
        }
      } break;
      case Type_boolean: {
        u8 val = *cast<u8 *>(v->data);
        fputs(val ? "true" : "false", stdout);
      } break;
      case Type_slice: {
        if (types->get(t->slice.base_type)->kind == Type_integer &&
            types->get(t->slice.base_type)->integer.bitwidth == 8) {
          auto s = cast<ValueSlice *>(v->data);
          printf("%.*s", cast<int>(s->len), cast<char const *>(s->items));
        } else {
          Todo();
        }
      } break;
      case Type_array:
      case Type_sequence:
      case Type_function:
      case Type_literal_function:
      case Type_distinct:
      case Type_nil:
      case Type_never:
      case Type_type:
        Todo();
        break;
      }

      arg_index += 1;
      i         += 2;
      continue;
    }

    putchar(c);
    i += 1;
  }
}

ValueIndex Interpreter::alloc_value_type(TypeIndex type) {
  Value *val;
  auto   idx               = values.alloc_value(&val);
  auto   data              = values.alloc_data<TypeIndex>();
  *cast<TypeIndex *>(data) = type;

  *val = {.type = types->type.type, .data = data};

  return idx;
}

void Interpreter::insert_cast_to(TypeIndex type_dst, NodeIndex *node) {
  auto old_node_index = *node;
  auto node_index     = nodes->alloc();
  *node               = NodeIndex{NodeIndex_ast_node, node_index.idx};

  auto val_idx = alloc_value_type(type_dst);

  AstCast cast;
  cast.type_dst = NodeIndex{NodeIndex_value, val_idx.idx};
  cast.value    = old_node_index;

  nodes->set(
    node_index,
    {
      Ast_cast,
      {{0}, {0}}, // TODO replace this with something else. or maybe these nodes shouldn't live in `AstNodes`
      {.cast = cast},
    }
  );
}
