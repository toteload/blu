#include "blu.hh"

Type *infer_ast_type(CompilerContext *ctx, Env *env, AstNode *type);
Type *infer_function_type(CompilerContext *ctx, Env *env, AstNode *function);
Type *infer_expression_type(CompilerContext *ctx, Env *env, AstNode *expression);

Type *infer_ast_type(CompilerContext *ctx, Env *env, AstNode *type) {
  Debug_assert(
    type->kind == Ast_identifier || type->kind == Ast_type_pointer || type->kind == Ast_type_slice
  );

  if (type->kind == Ast_type_pointer) {
    Todo();
  }

  if (type->kind == Ast_type_slice) {
    Type *base      = infer_ast_type(ctx, env, type->slice.base);
    Type slice_type = Type::make_slice(base);

    return ctx->types.add(&slice_type);
  }

  StrKey idkey = type->identifier.key;
  Value *v     = env->lookup(idkey);

  if (!v) {
    Str identifier = ctx->strings.get(idkey);
    Str msg        = ctx->arena.push_format_string(
      "Expected defined identifier %.*s but it could not be found.\n",
      identifier.len,
      identifier.str
    );
    ctx->messages.push({type->span, Error, msg});

    return nullptr;
  }

  return v->type;
}

b32 is_type_coercible_to(Type *src, Type *dst) {
  if (src == dst) {
    return true;
  }

  if (src->kind == Type_IntegerConstant && dst->kind == Type_Integer) {
    return true;
  }

  return false;
}

b32 is_assignable(CompilerContext *ctx, Env *env, AstNode *n) { return true; }

Type *get_coerced_type(CompilerContext *ctx, Type *a, Type *b) {
  if (a->kind == Type_Integer && b->kind == Type_IntegerConstant) {
    return a;
  }

  if (b->kind == Type_Integer && a->kind == Type_IntegerConstant) {
    return b;
  }

  Todo();

  return nullptr;
}

Type *determine_binary_op_type(CompilerContext *ctx, BinaryOpKind op, Type *lhs, Type *rhs) {
  switch (op) {
  case Add:
  case Sub:
  case Div:
  case Mul: {
    if (is_type_coercible_to(lhs, rhs)) {
      return rhs;
    }

    if (is_type_coercible_to(rhs, lhs)) {
      return lhs;
    }

    Todo();
  } break;
  case Cmp_equal:
  case Cmp_less_equal: {
    if (is_type_coercible_to(lhs, rhs) || is_type_coercible_to(rhs, lhs)) {
      return ctx->types._bool;
    }

    Todo();
  } break;
  case Logical_or:
  case Logical_and: {
    if (lhs == rhs && lhs->kind == Type_Boolean) {
      return ctx->types._bool;
    }

    Todo();
  } break;

  // All cases should be handled
  default:
    Unreachable();
    return nullptr;
  }
}

Type *infer_expression_type(CompilerContext *ctx, Env *env, AstNode *e) {
  Type *res = nullptr;

  switch (e->kind) {
  case Ast_declaration: {
    res = ctx->types._void;

    auto &decl = e->declaration;

    Type *declared_type = infer_ast_type(ctx, env, decl.type);
    Type *v             = infer_expression_type(ctx, env, decl.value);

    if (!is_type_coercible_to(v, declared_type)) {
      // Type error
      Unimplemented;
    }

    if (declared_type != v) {
      // Add an explicit cast if the type is coercible and not the same

      AstNode *n               = ctx->nodes.alloc();
      n->type                  = declared_type;
      n->kind                  = Ast_cast;
      n->cast.destination_type = nullptr;
      n->cast.value            = decl.value;

      decl.value = n;
    }

    env->insert(decl.name->identifier.key, Value::make_local(e, declared_type));

    res = declared_type;
  } break;
  case Ast_assign: {
    Debug_assert(is_assignable(ctx, env, e));

    Type *type = infer_expression_type(ctx, env, e->assign.value);
    Debug_assert(type);
  } break;
  case Ast_continue:
  case Ast_break: {
    res = ctx->types._never;
  } break;
  case Ast_return: {
    res = ctx->types._never;

    if (!e->_return.value) {
      break;
    }

    Type *type = infer_expression_type(ctx, env, e->_return.value);
    Debug_assert(type);
  } break;
  case Ast_while: {
    res = ctx->types._void;

    Type *cond = infer_expression_type(ctx, env, e->_while.cond);
    Type *body = infer_expression_type(ctx, env, e->_while.body);

    Debug_assert(cond);
    Debug_assert(body);
  } break;
  case Ast_for: {
    res = ctx->types._void;

    Type *iterable_type = infer_expression_type(ctx, env, e->_for.iterable);

    Debug_assert(iterable_type->kind == Type_slice);

    Type *base_type = iterable_type->slice.base_type;

    e->_for.item->type = base_type;

    Env *for_env = ctx->environments.alloc(env);
    for_env->insert(e->_for.item->identifier.key, Value::make_iter_item(base_type, e->_for.item));

    infer_expression_type(ctx, for_env, e->_for.body);
  } break;
  case Ast_scope: {
    Type *scope_type = ctx->types._void;
    auto expressions = e->scope.expressions;
    ForEachAstNode(e, expressions) {
      infer_expression_type(ctx, env, e);

      if (!e->next) {
        scope_type = e->type;
      }
    }

    res = scope_type;
  } break;
  case Ast_identifier: {
    auto idkey = e->identifier.key;
    Value *p   = env->lookup(idkey);

    if (!p) {
      Str identifier = ctx->strings.get(idkey);
      Str msg        = ctx->arena.push_format_string(
        "Could not find identifier %.*s.",
        identifier.len,
        identifier.str
      );
      ctx->messages.push({e->span, Error, msg});
      break;
    }

    res = p->type;
  } break;
  case Ast_literal_int: {
    res = ctx->types._integer_constant;
  } break;
  case Ast_if_else: {
    Type *cond = infer_expression_type(ctx, env, e->if_else.cond);
    if (cond->kind != Type_Boolean) {
      Str msg = ctx->arena.push_format_string("Condition in if-expression is not of type 'bool'.");
      ctx->messages.push({e->if_else.cond->span, Error, msg});
    }

    Type *then = infer_expression_type(ctx, env, e->if_else.then);
    if (e->if_else.otherwise) {
      Type *otherwise = infer_expression_type(ctx, env, e->if_else.otherwise);

      Type *final_type = nullptr;
      if (is_type_coercible_to(then, otherwise)) {
        final_type = otherwise;
      } else if (is_type_coercible_to(otherwise, then)) {
        final_type = then;
      } else {
        Todo();
        break;
      }

      e->if_else.then->type      = final_type;
      e->if_else.otherwise->type = final_type;

      res = final_type;

      break;
    }

    // We only have an if-branch, then the return type of the branch should be void.
    if (then->kind != Type_Void) {
      Str msg = ctx->arena.push_format_string("If-expression should return type 'void'.");
      ctx->messages.push({e->if_else.then->span, Error, msg});
    }

    res = then;
  } break;
  case Ast_call: {
    Type *callee = infer_expression_type(ctx, env, e->call.callee);
    Debug_assert(callee->kind == Type_Function);

    u32 param_count = callee->function.param_count;

    u32 i = 0;
    ForEachAstNode(arg, e->call.args) {
      if (i == param_count) {
        // More arguments provided than the function takes parameters.
        break;
      }

      Type *arg_type   = infer_expression_type(ctx, env, arg);
      Type *param_type = callee->function.params[i];
      Debug_assert(is_type_coercible_to(arg_type, param_type));
      i += 1;
    }

    u32 arg_count = i;

    Debug_assert(param_count == arg_count);

    res = callee->function.return_type;
  } break;
  case Ast_binary_op: {
    Type *lhs = infer_expression_type(ctx, env, e->binary_op.lhs);
    Type *rhs = infer_expression_type(ctx, env, e->binary_op.rhs);

    Debug_assert(lhs && rhs);

    if (e->binary_op.kind == AddAssign) {
      Debug_assert(is_assignable(ctx, env, e->binary_op.lhs));
      Debug_assert(is_type_coercible_to(rhs, lhs));
      res = ctx->types._void;
      break;
    }

    res = determine_binary_op_type(ctx, e->binary_op.kind, lhs, rhs);
  } break;
  case Ast_unary_op: {
    res = infer_expression_type(ctx, env, e->unary_op.value);

    // TODO: make sure that the unary operator is defined for the type of the expression
  } break;
  default:
    Unreachable();
  }

  e->type = res;

  return res;
}

Type *infer_function_type(CompilerContext *ctx, Env *env, AstNode *function) {
  Debug_assert(function->kind == Ast_function);

  Type *return_type = infer_ast_type(ctx, env, function->function.return_type);

  u32 param_count = 0;
  ForEachAstNode(param, function->function.params) { param_count += 1; }

  Type *function_type =
    cast<Type *>(ctx->arena.raw_alloc(sizeof(Type) + param_count * sizeof(Type *), Align_of(Type)));
  function_type->kind                 = Type_Function;
  function_type->function.param_count = param_count;
  function_type->function.return_type = return_type;

  b32 has_param_type_error = false;

  u32 i = 0;
  ForEachAstNode(param, function->function.params) {
    Type *t = infer_ast_type(ctx, env, param->param.type);
    if (!t) {
      has_param_type_error = true;
      break;
    }

    param->type = t;

    function_type->function.params[i] = t;

    i += 1;
  }

  if (!return_type || has_param_type_error) {
    return nullptr;
  }

  Type *intern_type = ctx->types.add(function_type);

  function->type = intern_type;

  return intern_type;
}

void infer_types_in_function(CompilerContext *ctx, Env *env, AstNode *function) {
  Debug_assert(function->kind == Ast_function);

  Env *param_env = ctx->environments.alloc(env);

  ForEachAstNode(param, function->function.params) {
    Type *param_type = infer_ast_type(ctx, env, param->param.type);
    param_env->insert(param->param.name->identifier.key, Value::make_param(param, param_type));
  }

  Type *t = infer_expression_type(ctx, param_env, function->function.body);

  Debug_assert(t);

  ctx->environments.dealloc(param_env);
}

b32 infer_types(CompilerContext *ctx, AstNode *mod) {
  Env *mod_environment = ctx->environments.alloc(ctx->global_environment);

  // Declarations are added to the module environment.

  ForEachAstNode(item, mod->module.items) {
    Debug_assert(item->kind == Ast_declaration);

    AstNode *value = item->declaration.value;
    Debug_assert(value->kind == Ast_function);

    Type *type = infer_function_type(ctx, mod_environment, value);

    Debug_assert(type);
    Debug_assert(item->declaration.name->kind == Ast_identifier);

    item->type = type;

    mod_environment->insert(item->declaration.name->identifier.key, Value::make_local(item, type));
  }

  ForEachAstNode(item, mod->module.items) {
    Debug_assert(item->kind == Ast_declaration);

    AstNode *function = item->declaration.value;
    Debug_assert(function->kind == Ast_function);

    infer_types_in_function(ctx, mod_environment, function);
  }

  return true;
}
