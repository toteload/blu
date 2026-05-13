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

void Interpreter::init() {
  work_arena.init(MiB(1));
  values.init(stdlib_alloc);
  envs.init(stdlib_alloc, stdlib_alloc);
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

bool Interpreter::run_const_code(InterpreterContext *context) {
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
    *v         = {
      .type = types->type.nil,
      .data = nullptr,
    };
  }

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

  auto &root_items = nodes->datas[root_idx.idx].root.items;
  for (u32 i = 0; i < root_items.len(); i++) {
    Try(add_declaration(env_root, root_items[i]));
  }

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

    auto items = values.alloc_memory(size_info, count);
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

b32 Interpreter::add_declaration(Env<ValueIndex> *env, NodeIndex declaration) {
  Assert(nodes->kind(declaration) == Ast_declaration);

  auto decl = nodes->data(declaration).declaration;

  TypeIndex decl_type = get_type(decl.type);

  ValueIndex decl_value;
  Try(eval_expr(env, decl.value, &decl_value));

  Value *v;
  auto   val  = values.alloc_value(&v);
  auto   data = values.alloc_memory(types->size_info(decl_type));
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
    Todo();
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
    auto data           = values.alloc_memory(elem_size_info);
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
    void  *bytes = values.alloc_memory(types->size_info(t->array.base_type), count);

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
      auto   data           = values.alloc_memory(elem_size_info);
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
  case Ast_break:
  case Ast_continue:
  case Ast_return:
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
    if (env->lookup(key, &val)) {
      *slot = NodeIndex{NodeIndex_value, val.idx};
    }
  } break;

  case Ast_binary_op: {
    Try(const_walk(env, &nodes->datas[idx.idx].binary_op.lhs));
    Try(const_walk(env, &nodes->datas[idx.idx].binary_op.rhs));

    auto lhs = nodes->data(idx).binary_op.lhs;
    auto rhs = nodes->data(idx).binary_op.rhs;
    if (lhs.kind == NodeIndex_value && rhs.kind == NodeIndex_value) {
      ValueIndex val;
      Try(eval_expr(env, idx, &val));
      *slot = NodeIndex{NodeIndex_value, val.idx};
    }
  } break;

  case Ast_unary_op: {
    Try(const_walk(env, &nodes->datas[idx.idx].unary_op.value));

    auto v = nodes->data(idx).unary_op.value;
    if (v.kind == NodeIndex_value) {
      ValueIndex val;
      Try(eval_expr(env, idx, &val));
      *slot = NodeIndex{NodeIndex_value, val.idx};
    }
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
    Try(const_walk(env, &nodes->datas[idx.idx].index.indexable));
    Try(const_walk(env, &nodes->datas[idx.idx].index.index_at));

    auto a = nodes->data(idx).index.indexable;
    auto b = nodes->data(idx).index.index_at;
    if (a.kind == NodeIndex_value && b.kind == NodeIndex_value) {
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
    Try(const_walk(env, &nodes->datas[idx.idx].call.callee));
    auto &args = nodes->datas[idx.idx].call.args;
    for (u32 i = 0; i < args.len(); i++) {
      Try(const_walk(env, &args[i]));
    }
  } break;

  case Ast_declaration: {
    Try(const_walk(env, &nodes->datas[idx.idx].declaration.value));
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
    Try(const_walk(env_fn, &nodes->datas[idx.idx].function.body));
  } break;

  case Ast_if_else: {
    Try(const_walk(env, &nodes->datas[idx.idx].if_else.cond));
    Try(const_walk(env, &nodes->datas[idx.idx].if_else.then));
    Try(const_walk(env, &nodes->datas[idx.idx].if_else.otherwise));
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

  case Ast_return: {
    Try(const_walk(env, &nodes->datas[idx.idx].return_.value));
  } break;

  case Ast_assign: {
    Try(const_walk(env, &nodes->datas[idx.idx].assign.value));
  } break;

  case Ast_builtin: {
    auto &builtin = nodes->datas[idx.idx].builtin;
    Assert(builtin.kind == Builtin_print);

    for (u32 i = 0; i < builtin.args.len(); i++) {
      Try(const_walk(env, &builtin.args[i]));
    }
  } break;

  case Ast_root: {
    auto &items = nodes->datas[idx.idx].root.items;
    for (u32 i = 0; i < items.len(); i++) {
      Try(const_walk(env, &items[i]));
    }
  } break;

  case Ast_break:
  case Ast_continue:
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
      switch (op) {
      case Cmp_equal:
        res = a == b;
        break;
      case Cmp_not_equal:
        res = a != b;
        break;
      case Cmp_less_than:
        res = a < b;
        break;
      case Cmp_less_equal:
        res = a <= b;
        break;
      case Cmp_greater_than:
        res = a > b;
        break;
      case Cmp_greater_equal:
        res = a >= b;
        break;
      default:
        Unreachable();
      }
    } else {
      i64 a = get_as_i64(lhs);
      i64 b = get_as_i64(rhs);
      switch (op) {
      case Cmp_equal:
        res = a == b;
        break;
      case Cmp_not_equal:
        res = a != b;
        break;
      case Cmp_less_than:
        res = a < b;
        break;
      case Cmp_less_equal:
        res = a <= b;
        break;
      case Cmp_greater_than:
        res = a > b;
        break;
      case Cmp_greater_equal:
        res = a >= b;
        break;
      default:
        Unreachable();
      }
    }

    Value *v;
    auto   idx  = values.alloc_value(&v);
    auto   data = cast<u8 *>(values.alloc_memory(types->size_info(types->type.bool_)));
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
      void  *data      = values.alloc_memory(size_info);
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
      void  *data      = values.alloc_memory(size_info);
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
    auto   data            = values.alloc_memory(types->size_info(param_type));
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
  auto data = values.alloc_memory(types->size_info(return_type_idx));
  *v        = {.type = return_type_idx, .data = data};

  Try(coerce_value(return_type_idx, body_result, data));

  return true;
}

TypeIndex Interpreter::get_type(NodeIndex node_index) { return node_types[node_index.idx]; }

b32 Interpreter::eval_cast(TypeIndex type_idx_dst, ValueIndex val_idx, ValueIndex *result) {
  Value *val = values.get(val_idx);

  if (types->is_coercible_to(val->type, type_idx_dst)) {
    Value *v;
    *result = values.alloc_value(&v);

    auto size_info = types->size_info(type_idx_dst);
    auto data      = values.alloc_memory(size_info);

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
      } else if (
        type_dst->integer.signedness == Unsigned && type_val->integer.signedness == Signed
      ) {
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
      } else if (
        type_dst->integer.signedness == Signed && type_val->integer.signedness == Unsigned
      ) {
        u64 i = get_as_u64(val_idx);

        u64 hi = cast<u64>(int_value_max(type_dst->integer.bitwidth));

        if (i > hi) {
          Todo("invalid cast: value out of range");
        }
      } else if (
        type_dst->integer.signedness == Unsigned && type_val->integer.signedness == Unsigned
      ) {
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
      auto data      = values.alloc_memory(size_info);

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
    if (
      c == '{' && i + 3 < format.len() && format[i + 1] == '{' && format[i + 2] == '}' &&
      format[i + 3] == '}'
    ) {
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
        if (
          types->get(t->slice.base_type)->kind == Type_integer &&
          types->get(t->slice.base_type)->integer.bitwidth == 8
        ) {
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
