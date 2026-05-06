#include "blu.hh"

void Interpreter::init(
  StringInterner *strings,
  TypeInterner *types,
  ValueStore *values,
  EnvManager<ValueIndex> *envs,
  Arena *work_arena,
  Messages *messages
) {
  this->strings    = strings;
  this->types      = types;
  this->values     = values;
  this->envs       = envs;
  this->work_arena = work_arena;
  this->messages   = messages;

  {
    Value *v;
    common.nil = values->alloc_value(&v);
    auto data  = values->alloc_data<TypeIndex>();
    *data      = types->type.nil;
    *v         = {
      .type = types->type.type,
      .data = data,
    };
  }
}

struct PopulateRootEnv {
  StringInterner *strings;
  TypeInterner *types;
  ValueStore *values;
  Env<ValueIndex> *env;

  template<typename T> void insert(Str s, TypeIndex type, T val) {
    auto key = strings->add(s);

    Value *v;
    auto idx  = values->alloc_value(&v);
    auto data = values->alloc_data<T>();
    *data     = val;
    *v        = {
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

b32 Interpreter::run(Source *source, ValueIndex *result) {
  this->source = source;

  auto nodes = source->nodes;

  Env<ValueIndex> *env_global = envs->alloc(nullptr);
  {
    PopulateRootEnv populate = {
      .strings = strings,
      .types   = types,
      .values  = values,
      .env     = env_global,
    };

    populate.populate();
  }
  auto env = envs->alloc(env_global);
  defer({
    envs->dealloc(env);
    envs->dealloc(env_global);
  });

  NodeIndex root_idx = {0};
  Assert(nodes->kind(root_idx) == Ast_root);

  auto root = nodes->data(root_idx);
  for (u32 i = 0; i < root.root.items.len(); i++) {
    auto item_idx = root.root.items[i];
    Try(add_declaration(env, item_idx));
  }

  auto main = values->get(_lookup(env, STR("main")).as_index());

  TypeIndex main_function_type;
  {
    Type *t = alloc_type_function(work_arena, 0);
    *t      = {
      .kind     = Type_function,
      .function = {
        .return_type = types->type.i32_,
        .param_count = 0,
        .param_types = {},
      },
    };
    main_function_type = types->add(t);
  }

  Assert(types->is_coercible_to(main->type, main_function_type));

  Try(call_function(env, main, {}, result));

  return true;
}

// Assume: The coercion is always possible.
void Interpreter::coerce_value(TypeIndex type_dst, ValueIndex src, void *out) {
  auto v      = values->get(src);
  auto ty_src = types->get(v->type);

  if (type_dst == v->type) {
    memcpy(out, v->data, ty_src->size_info().size);
    return;
  }

  auto ty_dst = types->get(type_dst);

  if (ty_src->kind == Type_literal_function && ty_dst->kind == Type_function) {
    memcpy(out, v->data, sizeof(NodeIndex));
    return;
  }

  if (ty_src->kind == Type_literal_int && ty_dst->kind == Type_integer) {
    if (ty_dst->integer.signedness == Signed) {
    if (ty_dst->integer.bitwidth == 8) {
      *cast<i8 *>(out) = cast<i8>(*cast<i64 *>(v->data));
      return;
    }

    if (ty_dst->integer.bitwidth == 16) {
      *cast<i16 *>(out) = cast<i16>(*cast<i64 *>(v->data));
      return;
    }

    if (ty_dst->integer.bitwidth == 32) {
      *cast<i32 *>(out) = cast<i32>(*cast<i64 *>(v->data));
      return;
    }

    if (ty_dst->integer.bitwidth == 64) {
      *cast<i64 *>(out) = cast<i64>(*cast<i64 *>(v->data));
      return;
    }
    }

    Todo();
  }

  if (ty_src->kind == Type_integer && ty_dst->kind == Type_integer) {
    auto signedness = ty_dst->integer.signedness;
    if (signedness == Signed) {
      if (ty_src->integer.bitwidth == 8 && ty_dst->integer.bitwidth == 32) {
        *cast<i32 *>(out) = cast<i32>(*cast<i8 *>(v->data));
        return;
      }

      if (ty_src->integer.bitwidth == 32 && ty_dst->integer.bitwidth == 64) {
        *cast<i64 *>(out) = cast<i64>(*cast<i32 *>(v->data));
        return;
      }
    }

    Todo();
  }

  if (ty_src->kind == Type_array && ty_dst->kind == Type_slice) {
    u32 count                = ty_src->array.size;
    *cast<ValueSlice *>(out) = {
      .len   = count,
      .items = v->data,
    };
    return;
  }

  if (ty_src->kind == Type_sequence) {
    TypeIndex base_type_idx;
    if (ty_dst->kind == Type_slice) {
      base_type_idx = ty_dst->slice.base_type;
    } else if (ty_dst->kind == Type_array) {
      base_type_idx = ty_dst->array.base_type;
    } else {
      Unreachable();
    }

    auto base_type = types->get(base_type_idx);

    u32 count      = ty_src->sequence.count;
    auto size_info = base_type->size_info();
    auto items     = values->alloc_memory(size_info, count);

    ValueIndex *sequence_items = cast<ValueIndex *>(v->data);

    for (u32 i = 0; i < count; i++) {
      coerce_value(base_type_idx, sequence_items[i], ptr_offset(items, size_info.stride * i));
    }

    if (ty_dst->kind == Type_slice) {
      *cast<ValueSlice *>(out) = {
        .len   = count,
        .items = items,
      };
    } else if (ty_dst->kind == Type_array) {
      *cast<void **>(out) = items;
    } else {
      Unreachable();
    }

    return;
  }

  Todo();
}

b32 Interpreter::add_declaration(Env<ValueIndex> *env, NodeIndex declaration) {
  Assert(source->nodes->kind(declaration) == Ast_declaration);

  auto decl = source->nodes->data(declaration).declaration;

  TypeIndex decl_type_idx = get_type(decl.type);

  ValueIndex decl_value;
  Try(eval_expr(env, decl.value, &decl_value));

  Type *decl_type = types->get(decl_type_idx);

  Value *v;
  auto val  = values->alloc_value(&v);
  auto data = values->alloc_memory(decl_type->size_info());
  *v        = {
    .type = decl_type_idx,
    .data = data,
  };

  coerce_value(decl_type_idx, decl_value, data);

  auto str = source->get_token_str(decl.name);
  auto key = strings->add(str);

  env->insert(key, val);

  return true;
}

b32 Interpreter::eval_expr(Env<ValueIndex> *env, NodeIndex node_index, ValueIndex *result) {
  auto kind = source->nodes->kind(node_index);

  switch (kind) {
  case Ast_literal_int: {
    auto token_index = source->nodes->data(node_index).literal_int.token_index;
    auto str         = source->get_token_str(token_index);
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
  case Ast_builtin: {
    auto builtin = source->nodes->data(node_index).builtin;
    switch (builtin.kind) {
    case Builtin_print: {
      ValueIndex arg_format;
      Try(eval_expr(env, builtin.args[0], &arg_format));

      ValueSlice slice;
      coerce_value(types->type.slice_u8, arg_format, &slice);

      printf("%.*s\n", cast<int>(slice.len), cast<char const *>(slice.items));

      *result = common.nil;
    } break;
    }
  } break;
  case Ast_block: {
    auto block = source->nodes->data(node_index).block;
    if (block.items.len() == 0) {
      *result = common.nil;
      break;
    }

    auto env_block = envs->alloc(env);
    defer(envs->dealloc(env_block));

    auto snapshot = work_arena->take_snapshot();
    defer(work_arena->restore(snapshot));

    NodeIndex *defers = work_arena->alloc<NodeIndex>(block.items.len());
    u32 defer_count   = 0;

    ValueIndex last = common.nil;

    for (u32 i = 0; i < block.items.len(); i++) {
      auto item = block.items[i];
      if (source->nodes->kind(item) == Ast_defer) {
        defers[defer_count++] = source->nodes->data(item).defer.value;
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
    auto if_else = source->nodes->data(node_index).if_else;

    ValueIndex cond;
    Try(eval_expr(env, if_else.cond, &cond));

    auto v = values->get(cond);

    if (*cast<u8 *>(v->data) == 1) {
      Try(eval_expr(env, if_else.then, result));
    } else if (if_else.otherwise.is_some()) {
      Try(eval_expr(env, if_else.otherwise.as_index(), result));
    } else {
      *result = common.nil;
    }

    values->drop(cond);
  } break;
  case Ast_declaration: {
    Try(add_declaration(env, node_index));
  } break;
  case Ast_binary_op: {
    auto binop = source->nodes->data(node_index).binary_op;
    auto op    = binop.kind;

    ValueIndex lhs;
    Try(eval_expr(env, binop.lhs, &lhs));

    ValueIndex rhs;
    Try(eval_expr(env, binop.rhs, &rhs));

    Try(eval_binary_op(op, lhs, rhs, node_index, result));

    values->drop(lhs);
    values->drop(rhs);
  } break;
  case Ast_literal_sequence: {
    auto seq   = source->nodes->data(node_index).literal_sequence;
    auto count = seq.items.len();

    Value *v;
    auto res          = values->alloc_value(&v);
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
  case Ast_index: {
    auto node = source->nodes->data(node_index).index;

    ValueIndex idx_indexable;
    Try(eval_expr(env, node.indexable, &idx_indexable));

    ValueIndex idx_index_at;
    Try(eval_expr(env, node.index_at, &idx_index_at));

    u64 i = get_uint(idx_index_at);

    auto indexable      = values->get(idx_indexable);
    auto type_indexable = types->get(indexable->type);

    if (type_indexable->kind == Type_slice) {
      auto size_info = types->get(type_indexable->slice.base_type)->size_info();
      auto offset    = i * size_info.stride;
      auto slice     = cast<ValueSlice *>(indexable->data);
      auto p         = ptr_offset(slice->items, offset);

      Value *vout;
      *result = values->alloc_value(&vout);

      Todo("return some sort of L-value");

      //*vout   = {
      //  .type = type_indexable->slice.base_type,
      //  .data = indexable->data.slice.items[i],
      //};
      break;
    }

    Todo("implement indexing");
  } break;
  case Ast_literal_string: {
    auto ty        = get_type(node_index);
    auto t         = types->get(ty);
    auto base_type = types->get(t->array.base_type);
    u32 count      = t->array.size;

    Value *v;
    auto res    = values->alloc_value(&v);
    void *bytes = values->alloc_memory(base_type->size_info(), count);

    *v = {
      .type = ty,
      .data = bytes,
    };

    auto token_index = source->nodes->data(node_index).literal_string.token_index;
    auto literal     = source->get_token_str(token_index);
    auto s           = literal.sub(1, literal.len() - 1);

    // TODO handle escape codes
    memcpy(bytes, s.str, s.len());

    *result = res;
  } break;

  case Ast_defer: {
    // Normally collected by the enclosing Ast_block before eval_expr is called.
    // Evaluate immediately if encountered outside a block context.
    auto defer_ = source->nodes->data(node_index).defer;
    ValueIndex e;
    Try(eval_expr(env, defer_.value, &e));
    *result = common.nil;
  } break;

  case Ast_assign:
  case Ast_type_slice:
  case Ast_type_array:
  case Ast_type_function:
  case Ast_call:
  case Ast_unary_op:
  case Ast_while:
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

b32 Interpreter::eval_binary_op(
  BinaryOpKind op, ValueIndex lhs, ValueIndex rhs, NodeIndex expr, ValueIndex *result
) {
  switch (op) {
  case Mul:
  case Div:
  case Add:
  case Sub: {
    auto left  = values->get(lhs);
    auto right = values->get(rhs);

    auto left_type  = types->get(left->type);
    auto right_type = types->get(right->type);

    Assert(left_type->is_integer_or_literal_int() && right_type->is_integer_or_literal_int());

    auto signedness = left_type->integer.signedness;

    if (signedness == Signed) {
      i64 a         = get_as_i64(lhs);
      i64 b         = get_as_i64(rhs);
      bool overflow = false;
      i64 res;

      // clang-format off
      switch (op) {
      case Div: res = a / b; break;
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

      i64 min_value = int_value_min(result_type->integer.bitwidth);
      i64 max_value = int_value_max(result_type->integer.bitwidth);

      if (res > max_value || res < min_value) {
        Todo();
      }

      Value *v;
      auto idx      = values->alloc_value(&v);
      void *data    = values->alloc_memory(result_type->size_info());
      u32 byte_size = result_type->integer.bitwidth / 8;
      memcpy(data, &res, byte_size);
      *v = {
        .type = result_type_idx,
        .data = data,
      };

      *result = idx;

      return true;
    } else {
      u64 a         = get_as_u64(lhs);
      u64 b         = get_as_u64(rhs);
      bool overflow = false;
      u64 res;

      // clang-format off
      switch (op) {
      case Div: res = a / b; break;
      case Add: overflow = __builtin_add_overflow(a, b, &res); break;
      case Sub: overflow = __builtin_sub_overflow(a, b, &res); break;
      case Mul: overflow = __builtin_mul_overflow(a, b, &res); break;
      default: Unreachable(); break;
      }
      // clang-format on

      Todo("unsigned int op: make sure nothing overflowed");
    }
  } break;
  default:
    Todo();
    break;
  }
  return true;
}

ValueIndex Interpreter::lookup_identifier(Env<ValueIndex> *env, NodeIndex identifier) {
  auto token_index = source->nodes->data(identifier).identifier.token_index;
  auto str         = source->get_token_str(token_index);
  auto key         = strings->add(str);

  ValueIndex val;
  auto found = env->lookup(key, &val);

  Assert(found);

  return val;
}

b32 Interpreter::call_function(
  Env<ValueIndex> *env, Value *function, Slice<ValueIndex> arguments, ValueIndex *result
) {
  // TODO: make sure the arguments match the expected parameters
  // TODO: add parameters to env

  auto node_index    = *cast<NodeIndex *>(function->data);
  auto function_node = source->nodes->data(node_index).function;

  ValueIndex body_result;
  Try(eval_expr(env, function_node.body, &body_result));

  TypeIndex return_type_idx = types->get(function->type)->function.return_type;
  auto return_type          = types->get(return_type_idx);

  Value *v;
  *result   = values->alloc_value(&v);
  auto data = values->alloc_memory(return_type->size_info());
  *v        = {.type = return_type_idx, .data = data};

  coerce_value(return_type_idx, body_result, data);

  return true;
}

TypeIndex Interpreter::get_type(NodeIndex node_index) { return type_annotations[node_index.idx]; }

u64 Interpreter::get_uint(ValueIndex idx) {
  u64 res;
  coerce_value(types->type.uint, idx, &res);
  return res;
}
