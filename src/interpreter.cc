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
    *v         = {
      .type = types->type.type,
      .data = {.type = types->type.nil},
    };
  }
}

struct PopulateRootEnv {
  StringInterner *strings;
  TypeInterner *types;
  ValueStore *values;
  Env<ValueIndex> *env;

  void insert(Str key, Value val);
  void insert_type(Str key, TypeIndex type);
  void populate();
};

void PopulateRootEnv::insert(Str s, Value val) {
  auto key = strings->add(s);
  Value *v;
  auto idx = values->alloc_value(&v);
  *v       = val;
  env->insert(key, idx);
}

void PopulateRootEnv::insert_type(Str s, TypeIndex type) {
  insert(s, {.type = types->type.type, .data = {.type = type}});
}

void PopulateRootEnv::populate() {
  insert_type(STR("i8"), types->type.i8_);
  insert_type(STR("i16"), types->type.i16_);
  insert_type(STR("i32"), types->type.i32_);
  insert_type(STR("i64"), types->type.i64_);

  insert_type(STR("u8"), types->type.u8_);
  insert_type(STR("u16"), types->type.u16_);
  insert_type(STR("u32"), types->type.u32_);
  insert_type(STR("u64"), types->type.u64_);

  insert_type(STR("nil"), types->type.nil);
  insert_type(STR("never"), types->type.never);
  insert_type(STR("type"), types->type.type);
  insert_type(STR("bool"), types->type.bool_);

  insert(STR("true"), {.type = types->type.bool_, .data = {.int64 = 1}});
  insert(STR("false"), {.type = types->type.bool_, .data = {.int64 = 0}});
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
    memcpy(out, &v->data, ty_src->size_info().size);
    return;
  }

  auto ty_dst = types->get(type_dst);

  if (ty_src->kind == Type_literal_function && ty_dst->kind == Type_function) {
    *cast<ValuePayload *>(out) = v->data;
    return;
  }

  if (ty_src->kind == Type_literal_int && ty_dst->kind == Type_integer) {
    if (ty_dst->integer.signedness == Signed && ty_dst->integer.bitwidth == 32) {
      *cast<i32 *>(out) = cast<i32>(v->data.int64);
      return;
    }
    Todo();
    return;
  }

  if (ty_src->kind == Type_array && ty_dst->kind == Type_slice) {
    Todo();
    u32 count                  = ty_src->array.size;
    *cast<ValuePayload *>(out) = {
      .slice = {
        .len    = count,
        .memory = v->data.memory,
      },
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

    ValuePayload *sequence_items = cast<ValuePayload *>(v->data.memory);

    for (u32 i = 0; i < count; i++) {
      coerce_value(base_type_idx, sequence_items[i].any, ptr_offset(items, size_info.stride * i));
    }

    if (ty_dst->kind == Type_slice) {
      *cast<ValuePayload *>(out) = {
        .slice = {
          .len    = count,
          .memory = items,
        }
      };
    } else if (ty_dst->kind == Type_array) {
      *cast<ValuePayload *>(out) = {.ptr = items};
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

  TypeIndex decl_type = get_type(decl.type);

  ValueIndex decl_value;
  Try(eval_expr(env, decl.value, &decl_value));

  Value *v;
  auto val = values->alloc_value(&v);
  v->type  = decl_type;

  coerce_value(decl_type, decl_value, &v->data);

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
    *result = values->alloc_value(&v);
    *v      = {
      .type = types->type.literal_int,
      .data = {
        .int64 = i,
      },
    };
  } break;
  case Ast_builtin: {
    auto builtin = source->nodes->data(node_index).builtin;
    switch (builtin.kind) {
    case Builtin_print: {
      ValueIndex arg_format;
      Try(eval_expr(env, builtin.args[0], &arg_format));

      ValuePayload payload;
      coerce_value(types->type.slice_u8, arg_format, &payload);

      printf("%.*s\n", cast<int>(payload.slice.len), cast<char const *>(payload.slice.memory));

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

    for (u32 i = 0; i < block.items.len() - 1; i++) {
      ValueIndex e;
      Try(eval_expr(env_block, block.items[i], &e));
    }

    Try(eval_expr(env_block, block.items[block.items.len() - 1], result));
  } break;
  case Ast_function: {
    Value *v;
    *result = values->alloc_value(&v);
    *v      = {
      .type = get_type(node_index),
      .data = {.node_index = node_index},
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

    if (v->data.int64 == 1) {
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
    auto res            = values->alloc_value(&v);
    ValuePayload *items = values->alloc_data<ValuePayload>(count);
    auto ty             = get_type(node_index);

    *v = {
      .type = ty,
      .data = {.memory = items},
    };

    for (u32 i = 0; i < count; i++) {
      Try(eval_expr(env, seq.items[i], &items[i].any));
    }

    *result = res;
  } break;
  case Ast_index: {
    auto node = source->nodes->data(node_index).index;

    ValueIndex idx_indexable;
    Try(eval_expr(env, node.indexable, &idx_indexable));

    ValueIndex idx_index_at;
    Try(eval_expr(env, node.index_at, &idx_index_at));

    i64 i = get_int_value(idx_index_at);

    auto indexable      = values->get(idx_indexable);
    auto type_indexable = types->get(indexable->type);

    if (type_indexable->kind == Type_slice) {
      auto size_info = types->get(type_indexable->slice.base_type)->size_info();
      auto offset    = i * size_info.stride;
      auto p         = ptr_offset(indexable->data.slice.memory, offset);

      Value *vout;
      *result = values->alloc_value(&vout);

      Todo(":)");

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
      .data = {
        .memory = bytes,
      },
    };

    auto token_index = source->nodes->data(node_index).literal_string.token_index;
    auto literal     = source->get_token_str(token_index);
    auto s           = literal.sub(1, literal.len() - 1);

    // TODO handle escape codes
    memcpy(bytes, s.str, s.len());

    *result = res;
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

    i64 res;
    switch (op) {
    case Mul:
      res = left->data.int64 * right->data.int64;
      break;
    case Div:
      res = left->data.int64 / right->data.int64;
      break;
    case Add:
      res = left->data.int64 + right->data.int64;
      break;
    case Sub:
      res = left->data.int64 - right->data.int64;
      break;
    default:
      Unreachable();
      break;
    }

    TypeIndex result_type;
    if (types->is_coercible_to(left->type, right->type)) {
      result_type = right->type;
    } else if (types->is_coercible_to(right->type, left->type)) {
      result_type = left->type;
    } else {
      Unreachable();
    }

    Value *v;
    *result = values->alloc_value(&v);
    *v      = {
      .type = result_type,
      .data = {
        .int64 = res,
      },
    };
  } break;
  default:
    Todo();
    break;
  }
  return true;
}

i64 Interpreter::get_int_value(ValueIndex idx) {
  Value *v = values->get(idx);
  return v->data.int64;
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

  auto function_node = source->nodes->data(function->data.node_index).function;
  Try(eval_expr(env, function_node.body, result));

  return true;
}

TypeIndex Interpreter::get_type(NodeIndex node_index) { return type_annotations[node_index.idx]; }
