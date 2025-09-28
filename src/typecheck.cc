#include "blu.hh"

struct TypeChecker {
  MessageManager *messages;
  Arena *work_arena;
  EnvManager *envs;
  TypeInterner *types;
  StringInterner *strings;
  Source *source;
  Nodes *nodes;

  TypeIndex return_type_of_current_function;

  // -

  b32 check_root(NodeIndex root, Env *env);

  b32 determine_type(NodeIndex type, Env *env, TypeIndex *out);

  b32 determine_unary_op_type(UnaryOpKind op, TypeIndex value, TypeIndex *out);
  b32 determine_binary_op_type(BinaryOpKind op, TypeIndex lhs, TypeIndex rhs, TypeIndex *out);

  b32 is_type_coercible_to(TypeIndex src, TypeIndex dst);

  b32 check_expression(NodeIndex expr_node_index, Env *env, TypeIndex *out);
  b32 check_block(NodeIndex node_index, Env *env, TypeIndex *out);
  b32 check_declaration(NodeIndex node_index, Env *env, TypeIndex *out);
  b32 check_assign(NodeIndex node_index, Env *env, TypeIndex *out);
  b32 check_identifier(NodeIndex node_index, Env *env, TypeIndex *out);
  b32 check_function(NodeIndex function, Env *env, TypeIndex *out);
  b32 check_call(NodeIndex function, Env *env, TypeIndex *out);
  b32 check_unary_op(NodeIndex function, Env *env, TypeIndex *out);
  b32 check_binary_op(NodeIndex function, Env *env, TypeIndex *out);
  b32 check_while(NodeIndex function, Env *env, TypeIndex *out);
  b32 check_if_else(NodeIndex function, Env *env, TypeIndex *out);
  b32 check_return(NodeIndex node_index, Env *env, TypeIndex *out);

  b32 is_assignable(NodeIndex node_index, Env *env) {
    AstKind kind = nodes->kind(node_index);
    if (kind != Ast_identifier) {
      return false;
    }
    return true;
  }
};

b32 TypeChecker::is_type_coercible_to(TypeIndex src, TypeIndex dst) {
  if (src == dst) {
    return true;
  }

  // `never` can be coerced to everything.
  if (src == types->never) {
    return true;
  }

  auto src_type = types->get(src);
  auto dst_type = types->get(dst);

  if (src_type->kind == Type_integer_constant && dst_type->kind == Type_integer) {
    return true;
  }

  if (src_type->kind == Type_sequence && dst_type->kind == Type_slice) {
    ForEachIndex(i, src_type->sequence.count) {
      if (!is_type_coercible_to(src_type->sequence.item_types[i], dst_type->slice.base_type)) {
        return false;
      }
    }

    return true;
  }

  if (dst_type->kind == Type_distinct) {
    if (src_type->kind == Type_distinct && dst_type->distinct.uid != src_type->distinct.uid) {
      return false;
    }

    return is_type_coercible_to(src, dst_type->distinct.base_type);
  }

  return false;
}

b32 TypeChecker::determine_unary_op_type(UnaryOpKind op, TypeIndex value, TypeIndex *out) {
  auto value_type = types->get(value);

  switch (op) {
  case Negate: {
    if (value == types->integer_constant || (value_type->kind == Type_integer && value_type->integer.signedness == Signed)) {
      *out = value;
      return true;
    }
  } break;
  case Not: {
    if (value == types->integer_constant || value_type->kind == Type_integer || value == types->bool_) {
      *out = value;
      return true;
    }
  } break;
  }

  Todo();

  return false;
}

b32 TypeChecker::determine_binary_op_type(
  BinaryOpKind op, TypeIndex lhs, TypeIndex rhs, TypeIndex *out
) {
  switch (op) {
  case Add:
  case Sub:
  case Div:
  case Mul: {
    if (is_type_coercible_to(lhs, rhs)) {
      *out = rhs;
      return true;
    }

    if (is_type_coercible_to(rhs, lhs)) {
      *out = lhs;
      return true;
    }
  } break;
  case Cmp_equal:
  case Cmp_less_equal: {
    if (is_type_coercible_to(lhs, rhs) || is_type_coercible_to(rhs, lhs)) {
      *out = types->bool_;
      return true;
    }
  } break;
  case Logical_or:
  case Logical_and: {
    if (lhs == rhs && lhs == types->bool_) {
      *out = types->bool_;
      return true;
    }
  } break;
  }

  Todo();

  return false;
}

b32 TypeChecker::check_root(NodeIndex root, Env *env) {
  auto items = nodes->data(root).root.items;

  // Add declarations to the environment
  for (usize i = 0; i < items.len(); i += 1) {
    Debug_assert(nodes->kind(items.at(i)) == Ast_declaration);

    auto decl = nodes->data(items.at(i)).declaration;

    StrKey key = strings->add(source->get_token_str(decl.name));

    env->insert(key, Value::make_declaration(decl.type, decl.value));
  }

  // Determine the declared types for all declarations
  for (usize i = 0; i < items.len(); i += 1) {
    auto decl = nodes->data(items.at(i)).declaration;

    StrKey key = strings->add(source->get_token_str(decl.name));

    Value *val = env->lookup(key);

    if (val->is_type_known()) {
      continue;
    }

    Try(determine_type(val->data.declaration.type_node_index, env, &val->type));
  }

  // At this point each declaration has its declared type determined.

  for (usize i = 0; i < items.len(); i += 1) {
    auto decl = nodes->data(items.at(i)).declaration;

    AstKind value_kind = nodes->kind(decl.value);

    TypeIndex value_type;
    Try(check_expression(decl.value, env, &value_type));

    // TODO: make sure that the declared type and the value type are compatible
  }

  return true;
}

b32 TypeChecker::determine_type(NodeIndex node_index, Env *env, TypeIndex *out) {
  auto kind = nodes->kind(node_index);

  if (kind == Ast_identifier) {
    auto token_index = nodes->data(node_index).identifier.token_index;
    StrKey key       = strings->add(source->get_token_str(token_index));
    Value *val       = env->lookup(key);
    if (!val) {
      messages->error("Unrecognized identifier '{strkey}' encountered", key);
      return false;
    }

    switch (val->kind) {
    case Value_param:
      Todo();
      break;
    case Value_lazy_declaration: {
      Try(determine_type(val->data.declaration.value_node_index, env, &val->data.type));
      *out = val->data.type;
    } break;
    case Value_type:
      *out = val->data.type;
      break;
    default:
      Todo();
      break;
    }

    return true;
  }

  if (kind == Ast_type_slice) {
    auto data = nodes->data(node_index).type_slice;

    TypeIndex base;
    Try(determine_type(data.base, env, &base));

    Type slice_type = Type::make_slice(base);

    *out = types->add(&slice_type);

    return true;
  }

  if (kind == Ast_type_function) {
    auto data = nodes->data(node_index).type_function;

    TypeIndex return_type;
    Try(determine_type(data.return_type, env, &return_type));

    usize param_count = data.params.len();

    Type *function_type = cast<Type *>(
      work_arena->raw_alloc(sizeof(Type) + param_count * sizeof(TypeIndex), Align_of(Type))
    );

    function_type->kind                 = Type_function;
    function_type->function.return_type = return_type;
    function_type->function.param_count = param_count;

    for (usize i = 0; i < param_count; i += 1) {
      Try(determine_type(data.params[i], env, &function_type->function.param_types[i]));
    }

    *out = types->add(function_type);

    return true;
  }

  return false;
}

b32 TypeChecker::check_expression(NodeIndex expr_node_index, Env *env, TypeIndex *out) {
  AstKind kind = nodes->kind(expr_node_index);

  // clang-format off
  switch (kind) {
  case Ast_type_slice:
  case Ast_type_function: {
    Try(determine_type(expr_node_index, env, out));
  } break;
  case Ast_block:       Try(check_block(expr_node_index, env, out)); break;
  case Ast_declaration: Try(check_declaration(expr_node_index, env, out)); break;
  case Ast_assign:      Try(check_assign(expr_node_index, env, out)); break;
  //case Ast_literal_sequence: Try(check_literal_sequence(expr_node_index, env, out)); break;
  case Ast_literal_int: *out = types->integer_constant; break;
  case Ast_identifier:  Try(check_identifier(expr_node_index, env, out)); break;
  case Ast_function:    Try(check_function(expr_node_index, env, out)); break;
  case Ast_call:        Try(check_call(expr_node_index, env, out)); break;
  case Ast_unary_op:    Try(check_unary_op(expr_node_index, env, out)); break;
  case Ast_binary_op:   Try(check_binary_op(expr_node_index, env, out)); break;
  case Ast_while:       Try(check_while(expr_node_index, env, out)); break;
  case Ast_if_else:     Try(check_if_else(expr_node_index, env, out)); break;
  case Ast_break:       *out = types->never; break;
  case Ast_continue:    *out = types->never; break;
  case Ast_return:      Try(check_return(expr_node_index, env, out)); break;
  }
  // clang-format on

  return true;
}

b32 TypeChecker::check_block(NodeIndex node_index, Env *env, TypeIndex *out) {
  auto items = nodes->data(node_index).block.items;

  if (items.len() == 0) {
    *out = types->nil;
    return true;
  }

  Env *block_env = envs->alloc(env);

  for (usize i = 0; i < items.len() - 1; i += 1) {
    TypeIndex type;
    Try(check_expression(items.at(i), block_env, &type));
  }

  Try(check_expression(items.at(items.len() - 1), block_env, out));

  envs->dealloc(block_env);

  return true;
}

b32 TypeChecker::check_return(NodeIndex node_index, Env *env, TypeIndex *out) {
  TypeIndex return_type;
  Try(check_expression(nodes->data(node_index).return_.value, env, &return_type));

  if (!is_type_coercible_to(return_type, return_type_of_current_function)) {
    messages->error(
      "Cannot return value of type '{type}'. Expected type '{type}'",
      return_type,
      return_type_of_current_function
    );
    return false;
  }

  *out = types->never;

  return true;
}

b32 TypeChecker::check_function(NodeIndex function_node_index, Env *env, TypeIndex *out) {
  auto function = nodes->data(function_node_index).function;

  Env *param_env = envs->alloc(env);

  usize param_count = function.params.len();

  Type *function_type = cast<Type *>(
    work_arena->raw_alloc(sizeof(Type) + param_count * sizeof(TypeIndex), Align_of(Type))
  );

  function_type->kind                 = Type_function;
  function_type->function.param_count = param_count;

  for (usize i = 0; i < param_count; i += 1) {
    TokenIndex name = function.params[i].name;
    StrKey key      = strings->add(source->get_token_str(name));

    TypeIndex param_type;
    Try(determine_type(function.params[i].type, env, &param_type));

    function_type->function.param_types[i] = param_type;

    param_env->insert(key, Value::make_param(name, param_type));
  }

  TypeIndex return_type;
  Try(determine_type(function.return_type, env, &return_type));

  function_type->function.return_type = return_type;

  return_type_of_current_function = return_type;

  TypeIndex body_type;
  Try(check_expression(function.body, param_env, &body_type));

  if (!is_type_coercible_to(body_type, return_type)) {
    messages
      ->error("Cannot coerce value of type '{type}' to type '{type}'", body_type, return_type);
    return false;
  }

  *out = types->add(function_type);

  return true;
}

b32 TypeChecker::check_if_else(NodeIndex expr, Env *env, TypeIndex *out) {
  auto data = nodes->data(expr).if_else;

  TypeIndex cond;
  Try(check_expression(data.cond, env, &cond));

  TypeIndex then;
  Try(check_expression(data.then, env, &then));

  if (data.otherwise.is_none()) {
    if (!is_type_coercible_to(then, types->never)) {
      messages->error(
        "if-expression without an else clause cannot return a value. Found '{type}'",
        then
      );
      return false;
    }

    *out = types->never;

    return true;
  }

  TypeIndex otherwise;
  Try(check_expression(NodeIndex::from_optional(data.otherwise), env, &otherwise));

  TypeIndex final_type;
  if (is_type_coercible_to(then, otherwise)) {
    final_type = otherwise;
  } else if (is_type_coercible_to(otherwise, then)) {
    final_type = then;
  } else {
    Todo();
  }

  *out = final_type;

  return true;
}

b32 TypeChecker::check_unary_op(NodeIndex node_index, Env *env, TypeIndex *out) {
  auto unary_op = nodes->data(node_index).unary_op;

  TypeIndex type;
  Try(check_expression(unary_op.value, env, &type));

  Try(determine_unary_op_type(unary_op.kind, type, out));

  return true;
}

b32 TypeChecker::check_binary_op(NodeIndex node_index, Env *env, TypeIndex *out) {
  auto binary_op = nodes->data(node_index).binary_op;

  TypeIndex lhs;
  Try(check_expression(binary_op.lhs, env, &lhs));

  TypeIndex rhs;
  Try(check_expression(binary_op.rhs, env, &rhs));

  Try(determine_binary_op_type(binary_op.kind, lhs, rhs, out));

  return true;
}

b32 TypeChecker::check_while(NodeIndex node_index, Env *env, TypeIndex *out) {
  auto while_ = nodes->data(node_index).while_;

  TypeIndex cond_type;
  Try(check_expression(while_.cond, env, &cond_type));

  if (cond_type != types->bool_) {
    Todo();
  }

  TypeIndex body_type;
  Try(check_expression(while_.body, env, &body_type));

  *out = types->nil;

  return true;
}

b32 TypeChecker::check_call(NodeIndex node_index, Env *env, TypeIndex *out) {
  auto call = nodes->data(node_index).call;

  TypeIndex callee_type_index;
  Try(check_expression(call.callee, env, &callee_type_index));

  Type *callee_type = types->get(callee_type_index);

  if (callee_type->kind != Type_function) {
    messages->error("Callee is not a function. Found type '{type}'.", callee_type);
    return false;
  }

  if (callee_type->function.param_count != call.args.len()) {
    messages->error("Function was called with incorrect number of arguments.");
    return false;
  }

  for (usize i = 0; i < call.args.len(); i += 1) {
    TypeIndex arg_type;
    Try(check_expression(call.args[i], env, &arg_type));

    TypeIndex param_type = callee_type->function.param_types[i];
    if (!is_type_coercible_to(arg_type, param_type)) {
      messages
        ->error("Cannot coerce value of type '{type}' to type '{type}'.", arg_type, param_type);
      return false;
    }
  }

  *out = callee_type->function.return_type;

  return true;
}

b32 TypeChecker::check_assign(NodeIndex node_index, Env *env, TypeIndex *out) {
  auto assign = nodes->data(node_index).assign;

  TypeIndex lhs_type;
  Try(check_expression(assign.lhs, env, &lhs_type));

  if (!is_assignable(assign.lhs, env)) {
    messages->error("Expression is not assignable");
    return false;
  }

  TypeIndex value_type;
  Try(check_expression(assign.value, env, &value_type));

  if (!is_type_coercible_to(value_type, lhs_type)) {
    messages->error("Cannot coerce value of type '{type}' to type '{type}'", value_type, lhs_type);
    return false;
  }

  *out = types->nil;

  return true;
}

b32 TypeChecker::check_identifier(NodeIndex node_index, Env *env, TypeIndex *out) {
  TokenIndex token_index = nodes->data(node_index).identifier.token_index;
  StrKey key             = strings->add(source->get_token_str(token_index));
  Value *val             = env->lookup(key);

  if (!val) {
    messages->error("Unrecognized identifier '{strkey}' encountered", key);
    return false;
  }

  *out = val->type;

  return true;
}

b32 TypeChecker::check_declaration(NodeIndex node_index, Env *env, TypeIndex *out) {
  auto decl = nodes->data(node_index).declaration;

  TypeIndex declared_type;
  Try(determine_type(decl.type, env, &declared_type));

  TypeIndex value_type;
  Try(check_expression(decl.value, env, &value_type));

  if (!is_type_coercible_to(value_type, declared_type)) {
    messages
      ->error("Cannot coerce value of type '{type}' to type '{type}'", value_type, declared_type);
    return false;
  }

  StrKey key = strings->add(source->get_token_str(decl.name));
  Value val  = Value::make_declaration(decl.type, decl.value);
  val.type   = declared_type;

  env->insert(key, val);

  return true;
}

b32 type_check(TypeCheckContext *ctx, Source *source) {
  TypeChecker typechecker;
  typechecker.messages   = ctx->messages;
  typechecker.work_arena = ctx->work_arena;
  typechecker.envs       = ctx->envs;
  typechecker.types      = ctx->types;
  typechecker.strings    = ctx->strings;
  typechecker.source     = source;
  typechecker.nodes      = source->nodes;

  auto snapshot         = ctx->work_arena->take_snapshot();
  Env *root_environment = ctx->envs->alloc(ctx->envs->global_env);

  Try(typechecker.check_root({0}, root_environment));

  ctx->envs->dealloc(root_environment);
  ctx->work_arena->restore(snapshot);

  return true;
}
