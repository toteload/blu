#include "blu.hh"

void Interpreter::init(
  StringInterner *strings,
  TypeInterner *types,
  ValueStore *values,
  EnvManager *envs,
  Arena *work_arena,
  Messages *messages
) {
  this->strings    = strings;
  this->types      = types;
  this->values     = values;
  this->envs       = envs;
  this->work_arena = work_arena;
  this->messages   = messages;

  common.nil = values->add({
    .kind = Val_type,
    .type = types->type.type,
    .data = {.type = types->type.nil},
  });
}

#if 0
b32 Interpreter::intern_type(Env *env, NodeIndex node_index, TypeIndex *result) {
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

    Try(intern_type(env, f.return_type, &ty->function.return_type));

    for (u32 i = 0; i < param_count; i++) {
      Try(intern_type(env, f.param_types[i], &ty->function.param_types[i]));
    }

    *result = types->add(ty);
  } break;
  case Ast_identifier: {
    auto str = source->get_token_str(source->nodes->data(node_index).identifier.token_index);
    auto val = values->get(_lookup(env, str).as_index());
    Assert(val->kind == Val_type);
    *result = val->data.type;
  } break;
  case Ast_type_slice: {
    auto slice = source->nodes->data(node_index).type_slice;
    Type ty    = {
      .kind = Type_slice,
    };
    Try(intern_type(env, slice.base, &ty.slice.base_type));
    *result = types->add(&ty);
  } break;
  case Ast_type_array: {
    auto array = source->nodes->data(node_index).type_array;
    Type ty    = {
      .kind = Type_array,
    };
    Try(intern_type(env, array.base, &ty.array.base_type));

    ValueIndex size_result;
    Try(eval_expr(env, array.size, &size_result));

    Try(get_int_value(size_result, &ty.array.size));

    *result = types->add(&ty);
  } break;
  default:
    Todo();
    break;
  }

  return true;
}
#endif

b32 Interpreter::run(Source *source, ValueIndex *result) {
  this->source = source;

  auto nodes = source->nodes;

  auto env_global = envs->create_global_env(strings, types, values);
  auto env        = envs->alloc(env_global);
  defer({
    envs->dealloc(env_global);
    envs->dealloc(env);
  });

  NodeIndex root_idx = {0};
  Assert(nodes->kind(root_idx) == Ast_root);

  auto root = nodes->data(root_idx);
  for (u32 i = 0; i < root.root.items.len(); i++) {
    auto item_idx = root.root.items[i];
    Try(add_declaration(env, item_idx));
  }

  auto main = values->get(_lookup(env, Str_make("main")).as_index());

  TypeIndex main_function_type;
  {
    Type *t = alloc_type_function(work_arena, 0);
    *t      = {
      .kind     = Type_function,
      .function = {
        .return_type = types->type.i32_,
        .param_count = 0,
      },
    };
    main_function_type = types->add(t);
  }

  Assert(types->is_coercible_to(main->type, main_function_type));

  Try(call_function(env, main, {}, result));

  return true;
}

b32 Interpreter::add_declaration(Env *env, NodeIndex declaration) {
  Assert(source->nodes->kind(declaration) == Ast_declaration);

  auto decl = source->nodes->data(declaration).declaration;

  TypeIndex decl_type = get_type(decl.type);

  ValueIndex decl_value;
  Try(eval_expr(env, decl.value, &decl_value));

  // TODO: Should the value be coerced to the declared type??

  auto str = source->get_token_str(decl.name);
  auto key = strings->add(str);

  env->insert(key, decl_value);

  return true;
}

b32 Interpreter::eval_expr(Env *env, NodeIndex node_index, ValueIndex *result) {
  auto kind = source->nodes->kind(node_index);

  switch (kind) {
  case Ast_literal_int: {
    auto token_index = source->nodes->data(node_index).literal_int.token_index;
    auto str         = source->get_token_str(token_index);
    auto i           = parse_i64(str);

    *result = values->add({
      .kind = Val_int,
      .type = types->type.literal_int,
      .data = {
        .int64 = i,
      },
    });
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
    *result = values->add({
      .kind = Val_function,
      .type = get_type(node_index),
      .data = {.node_index = node_index},
    });
  } break;
  case Ast_identifier: {
    *result = lookup_identifier(env, node_index);
  } break;
  case Ast_if_else: {
    auto if_else = source->nodes->data(node_index).if_else;

    ValueIndex cond;
    Try(eval_expr(env, if_else.cond, &cond));

    // TODO: Double check this code. I think it's wrong.
    auto v = values->get(cond);
    if (v->kind == Val_true) {
      Try(eval_expr(env, if_else.then, result));
    } else if (if_else.otherwise.is_some()) {
      Try(eval_expr(env, if_else.otherwise.as_index(), result));
    } else {
      *result = common.nil;
    }
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
  } break;
  case Ast_literal_sequence: {
    auto seq   = source->nodes->data(node_index).literal_sequence;
    auto count = seq.items.len();
    auto ty    = alloc_type_sequence(work_arena, count);

    *ty = {
      .kind     = Type_sequence,
      .sequence = {
        .count = cast<u32>(count),
      },
    };

    for (u32 i = 0; i < count; i++) {
      ValueIndex res;
      Try(eval_expr(env, seq.items[i], &res));
    }

    *result = values->add({

		    });
  } break;

  case Ast_literal_string:

  case Ast_assign:
  case Ast_type_slice:
  case Ast_type_array:
  case Ast_type_function:
  case Ast_call:
  case Ast_index:
  case Ast_unary_op:
  case Ast_while:
  case Ast_break:
  case Ast_continue:
  case Ast_return:
  case Ast_kind_max:
  case Ast_root:
    Unreachable();
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

    *result = values->add({
      .kind = Val_int,
      .type = result_type,
      .data = {
        .int64 = res,
      },
    });
  } break;
  default:
    Todo();
    break;
  }
  return true;
}

b32 Interpreter::get_int_value(ValueIndex idx, i64 *i) {
  Value *v = values->get(idx);
  auto ty  = types->get(v->type);
  if (!ty->is_integer_or_literal_int()) {
    return false;
  }

  *i = v->data.int64;

  return true;
}

b32 Interpreter::check_is_of_type(ValueIndex e, TypeIndex expected_type, NodeIndex location) {
  auto v = values->get(e);
  if (v->type != expected_type) {
    // TODO: Write nice message.
    return false;
  }

  return true;
}

ValueIndex Interpreter::lookup_identifier(Env *env, NodeIndex identifier) {
  auto token_index = source->nodes->data(identifier).identifier.token_index;
  auto str         = source->get_token_str(token_index);
  auto key         = strings->add(str);
  auto val         = env->lookup(key);

  Assert(val.is_some());

  return val.as_index();
}

b32 Interpreter::call_function(
  Env *env, Value *function, Slice<ValueIndex> arguments, ValueIndex *result
) {
  // TODO: make sure the arguments match the expected parameters
  // TODO: add parameters to env

  auto function_node = source->nodes->data(function->data.node_index).function;
  Try(eval_expr(env, function_node.body, result));

  return true;
}

TypeIndex Interpreter::get_type(NodeIndex node_index) { return type_annotations[node_index.idx]; }
