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
    ValueIndex _nil;
    Try(eval_declaration(env_root, root_items[i], &_nil));
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

  Try(eval_call(env_root, main, {}, result));

  return true;
}

b32 Interpreter::coercion_resolve_walk(NodeIndex *node) {
  Assert(node->kind == NodeIndex_ast_node);

  // NOTE: instead of a recursive walk, this can probably be replaced with a linear walk.

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
    auto      type_callee     = types->get(type_idx_callee);
    Assert(type_callee->kind == Type_function);
    for (u32 i = 0; i < type_callee->function.param_count; i++) {
      resolve_possible_coercion(type_callee->function.param_types[i], &data.call.args[i]);
    }
  } break;

  case Ast_function: {
  } break;
  case Ast_block: {
  } break;

  case Ast_builtin: {
    switch (data.builtin.kind) {
    case Builtin_print: {
      resolve_possible_coercion(types->type.slice_u8, &data.builtin.args[0]);
    } break;
    }
  } break;

  case Ast_identifier:
  case Ast_type_slice:
  case Ast_type_array:
  case Ast_type_function:
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

void Interpreter::copy_value(TypeIndex type, ValueIndex src, void *dst) {
  auto val = values.get(src);
  Assert(val->type == type);
  memcpy(dst, val->data, types->size_info(type).size);
}

b32 Interpreter::eval_declaration(Env<ValueIndex> *env, NodeIndex declaration, ValueIndex *result) {
  Assert(nodes->kind(declaration) == Ast_declaration);

  auto decl = nodes->data(declaration).declaration;

  ValueIndex decl_value;
  Try(eval_expr(env, decl.value, &decl_value));

  auto str = get_token_str(decl.name);
  auto key = strings->add(str);

  env->insert(key, decl_value);

  *result = common.nil;

  return true;
}

b32 Interpreter::eval_expr(Env<ValueIndex> *env, NodeIndex node_index, ValueIndex *result) {
  if (node_index.kind == NodeIndex_value) {
    *result = ValueIndex{node_index.idx};
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
      copy_value(types->type.slice_u8, arg_format, &slice);

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
    Try(eval_call(env, callee, {args, arg_count}, result));
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

    copy_value(lhs_type, value_idx, lhs_ptr);

    *result = common.nil;
  } break;

  // Should only be called during const evaluation.
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

  case Ast_const: {
    Todo("should not exist at runtime");
    // return eval_expr(env, nodes->data(node_index).const_.expr, result);
  } break;

  case Ast_type_slice:
  case Ast_type_array:
  case Ast_type_function:
  case Ast_unary_op:
  case Ast_kind_max:
  case Ast_root:
    Todo();
  }

  return true;
}

b32 Interpreter::const_walk(Env<ValueIndex> *env, NodeIndex *slot) {
  Assert(slot->is_some());

  if (slot->kind == NodeIndex_value) {
    return true;
  }

  auto idx  = *slot;
  auto kind = nodes->kind(idx);
  auto data = nodes->data(idx);

  switch (kind) {
  case Ast_const: {
    Try(const_walk(env, &data.const_.expr));
    Try(eval_expr(env, data.const_.expr, &slot->value_idx));
  } break;

  case Ast_literal_int:
  case Ast_literal_string: {
    Try(eval_expr(env, idx, &slot->value_idx));
  } break;

  case Ast_identifier: {
    auto token_idx = data.identifier.token_index;
    auto str       = get_token_str(token_idx);
    auto key       = strings->add(str);

    ValueIndex val;

    auto found_in_const_env = env->lookup(key, &val);

    if (found_in_const_env) {
      Value *v = values.get(val);
      Type  *t = types->get(v->type);
      if (t->kind == Type_literal_function || t->kind == Type_function) {
        break;
      }

      *slot = NodeIndex{NodeIndex_value, {val.idx}};
    }
  } break;

  case Ast_binary_op: {
    Try(const_walk(env, &data.binary_op.lhs));
    Try(const_walk(env, &data.binary_op.rhs));

    if (data.binary_op.lhs.kind == NodeIndex_value && data.binary_op.rhs.kind == NodeIndex_value) {
      Try(eval_expr(env, idx, &slot->value_idx));
    }
  } break;

  case Ast_unary_op: {
    Try(const_walk(env, &data.unary_op.value));

    if (data.unary_op.value.kind == NodeIndex_value) {
      Try(eval_expr(env, idx, &slot->value_idx));
    }
  } break;

  case Ast_cast: {
    Try(const_walk(env, &data.cast.value));

    if (data.cast.value.kind == NodeIndex_value) {
      Try(eval_expr(env, idx, &slot->value_idx));
    }
  } break;

  case Ast_index: {
    Try(const_walk(env, &data.index.indexable));
    Try(const_walk(env, &data.index.index_at));

    if (data.index.indexable.kind == NodeIndex_value &&
        data.index.index_at.kind == NodeIndex_value) {
      Try(eval_expr(env, idx, &slot->value_idx));
    }
  } break;

  case Ast_literal_sequence: {
    auto &items     = data.literal_sequence.items;
    bool  all_const = true;
    for (u32 i = 0; i < items.len(); i++) {
      Try(const_walk(env, &items[i]));
      if (items[i].kind != NodeIndex_value) {
        all_const = false;
      }
    }

    if (all_const) {
      Try(eval_expr(env, idx, &slot->value_idx));
    }
  } break;

  case Ast_call: {
    Try(const_walk(env, &data.call.callee));
    for (u32 i = 0; i < data.call.args.len(); i++) {
      Try(const_walk(env, &data.call.args[i]));
    }
  } break;

  case Ast_declaration: {
    Try(const_walk(env, &data.declaration.value));
  } break;

  case Ast_block: {
    auto env_block = envs.alloc(env);
    defer(envs.dealloc(env_block));

    auto &items = data.block.items;
    for (u32 i = 0; i < items.len(); i++) {
      Try(const_walk(env_block, &items[i]));
    }
  } break;

  case Ast_function: {
    Try(const_walk(env, &data.function.body));
  } break;

  case Ast_if_else: {
    Try(const_walk(env, &data.if_else.cond));
    Try(const_walk(env, &data.if_else.then));
    Try(const_walk(env, &data.if_else.otherwise));

    // TODO eval this
  } break;

  case Ast_for: {
    Try(const_walk(env, &data.for_.iterable));
    Try(const_walk(env, &data.for_.body));
  } break;

  case Ast_defer: {
    Try(const_walk(env, &data.defer.value));
  } break;

  case Ast_assign: {
    Try(const_walk(env, &data.assign.value));
  } break;

  case Ast_builtin: {
    Assert(data.builtin.kind == Builtin_print);

    for (u32 i = 0; i < data.builtin.args.len(); i++) {
      Try(const_walk(env, &data.builtin.args[i]));
    }
  } break;

  case Ast_root: {
    // Is this case ever encountered??
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
    u64 i; Todo();

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

b32 Interpreter::eval_call(
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
    auto param_node  = function_node.param_names[i];
    auto token_index = nodes->data(param_node).identifier.token_index;
    auto str         = get_token_str(token_index);
    auto key         = strings->add(str);

    env_args->insert(key, arguments[i]);
  }

  eval_expr(env_args, function_node.body, result);

  return true;
}

TypeIndex Interpreter::get_type(NodeIndex node_index) {
  if (node_index.kind == NodeIndex_ast_node) {
    return node_types[node_index.idx];
  } else {
    Value *v = values.get({node_index.idx});
    return v->type;
    // return *cast<TypeIndex *>(v->data);
  }
}

b32 Interpreter::eval_cast(TypeIndex type_idx_dst, ValueIndex val_idx, ValueIndex *result) {
  // You may assume that the cast is valid, since it got past the type checker.

  Value *val = values.get(val_idx);

  TypeIndex type_idx_src = val->type;

  if (type_idx_dst == type_idx_src) {
    *result = val_idx;
    return true;
  }

  Type *type_dst = types->get(type_idx_dst);
  Type *type_src = types->get(type_idx_src);

  if (type_src->kind == Type_literal_function && type_dst->kind == Type_function) {
    Value *cast_value;
    *result     = values.alloc_value(&cast_value);
    auto data   = values.alloc_data(types->size_info(type_idx_dst));
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
    *result     = values.alloc_value(&cast_value);
    auto data   = values.alloc_data(types->size_info(type_idx_dst));
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
      i64 i = get_as_i64(val_idx);

      i64 lo = int_value_min(type_dst->integer.bitwidth);
      i64 hi = int_value_max(type_dst->integer.bitwidth);

      if (i < lo || i > hi) {
        Todo("invalid cast: value out of range");
      }
    } else if (type_dst->integer.signedness == Unsigned && type_src->integer.signedness == Signed) {
      i64 i = get_as_i64(val_idx);

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
      u64 i = get_as_u64(val_idx);

      u64 hi = cast<u64>(int_value_max(type_dst->integer.bitwidth));

      if (i > hi) {
        Todo("invalid cast: value out of range");
      }
    } else if (type_dst->integer.signedness == Unsigned &&
               type_src->integer.signedness == Unsigned) {
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

  if (type_src->kind == Type_array && type_dst->kind == Type_slice) {
    u32 count = type_src->array.size;

    Value *v;
    *result = values.alloc_value(&v);

    auto size_info = types->size_info(type_idx_dst);
    auto data      = values.alloc_data(size_info);

    *cast<ValueSlice *>(data) = {
      .len   = count,
      .items = v->data,
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

    // auto items = values.alloc_data(size_info, count);
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
      {{0}, {0}}, // TODO replace this with something else. or maybe these nodes shouldn't live in
                  // `AstNodes`
      {.cast = cast},
    }
  );
}
