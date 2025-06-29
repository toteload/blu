#include "blu.hh"

Type *infer_ast_type(CompilerContext *ctx, Env *env, AstNode *type);
Type *infer_function_type(CompilerContext *ctx, Env *env, AstNode *function);
Type *infer_expression_type(CompilerContext *ctx, Env *env, AstNode *expression);

void infer_types_in_statement(CompilerContext *ctx, Env *env, AstNode *statement);

Type *infer_ast_type(CompilerContext *ctx, Env *env, AstNode *type) {
  Debug_assert(type->kind == Ast_identifier);

  StrKey idkey = type->identifier.key;
  AstNode *v   = env->lookup(idkey);

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

Type *determine_binary_op_type(CompilerContext *ctx, BinaryOpKind op, Type *lhs, Type *rhs) {
  switch (op) {
  case Add:
  case Sub:
  case Mul: {
    // if both have same type and are integer types = return type is same type
    // if one is integer type and other is integer_constant = return type is integer type

    if (lhs == rhs && lhs->is_integer_or_integer_constant()) {
      return lhs;
    }

    if (lhs->kind == Type_Integer && rhs->kind == Type_IntegerConstant) {
      return lhs;
    }

    Todo();
  } break;
  case Cmp_equal:
  case Cmp_less_equal: {
    if (lhs == rhs) {
      return ctx->types._bool;
    }

    if (lhs->kind == Type_Integer && rhs->kind == Type_IntegerConstant) {
      return ctx->types._bool;
    }

    Todo();
  } break;
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

Type *infer_expression_type(CompilerContext *ctx, Env *env, AstNode *expression) {
  Type *res = nullptr;

  switch (expression->kind) {
  case Ast_identifier: {
    auto idkey = expression->identifier.key;
    AstNode *p   = env->lookup(idkey);

    if (!p) {
      Str identifier = ctx->strings.get(idkey);
      Str msg        = ctx->arena.push_format_string(
        "Could not find identifier %.*s.",
        identifier.len,
        identifier.str
      );
      ctx->messages.push({expression->span, Error, msg});
      break;
    }

    res = p->type;
  } break;
  case Ast_literal_int: {
    res = ctx->types._integer_constant;
  } break;
  case Ast_if_else: {
    Type *cond = infer_expression_type(ctx, env, expression->if_else.cond);
    if (cond->kind != Type_Boolean) {
      Str msg = ctx->arena.push_format_string("Condition in if-expression is not of type 'bool'.");
      ctx->messages.push({expression->if_else.cond->span, Error, msg});
    }

    Type *then = infer_expression_type(ctx, env, expression->if_else.then);
    if (expression->if_else.otherwise) {
      Type *otherwise = infer_expression_type(ctx, env, expression->if_else.otherwise);

      if (then != otherwise) {
        // oopsie, the if and else branch have different types.
        // This is not always a problem.
        // For example, if one of the branches returns an integer constant and the other an
        // integer than they are different types but can be coerced to agree.
        Todo();
        break;
      }

      break;
    }

    // We only have an if-branch, then the return type of the branch should be void.
    if (then->kind != Type_Void) {
      Str msg = ctx->arena.push_format_string("If-expression should return type 'void'.");
      ctx->messages.push({expression->if_else.then->span, Error, msg});
    }

    Todo();
  } break;
  case Ast_call: {
    Type *callee = infer_expression_type(ctx, env, expression->call.f);
    Debug_assert(callee->kind == Type_Function);

    ForEachAstNode(arg, expression->call.args) { infer_expression_type(ctx, env, arg); }

    Todo();
  } break;
  case Ast_binary_op: {
    Type *lhs = infer_expression_type(ctx, env, expression->binary_op.lhs);
    Type *rhs = infer_expression_type(ctx, env, expression->binary_op.rhs);

    Debug_assert(lhs && rhs);

    res = determine_binary_op_type(ctx, expression->binary_op.kind, lhs, rhs);
  } break;
  case Ast_unary_op: {
    res = infer_expression_type(ctx, env, expression->unary_op.value);
  } break;
  case Ast_scope: {
    ForEachAstNode(statement, expression->scope.statements) {
      infer_types_in_statement(ctx, env, statement);
    }

    Todo();
  } break;
  default:
    Unreachable();
  }

  expression->type = res;

  return res;
}

void infer_types_in_statement(CompilerContext *ctx, Env *env, AstNode *statement) {
  switch (statement->kind) {
  case Ast_assign: {
    Type *type = infer_expression_type(ctx, env, statement->assign.value);
    Debug_assert(type);
  } break;
  case Ast_return: {
    if (!statement->_return.value) {
      break;
    }

    Type *type = infer_expression_type(ctx, env, statement->_return.value);
    Debug_assert(type);
  } break;
  case Ast_while: {
    infer_expression_type(ctx, env, statement->_while.cond);
    infer_expression_type(ctx, env, statement->_while.body);
  } break;
  case Ast_declaration: {
    Debug_assert(statement->declaration.type);

    Type *type = infer_ast_type(ctx, env, statement->declaration.type);
    Debug_assert(type);

    statement->type = type;

    StrKey name = statement->declaration.name->identifier.key;
    env->insert(name, statement);
  } break;
  default:
    infer_expression_type(ctx, env, statement);
  }
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

    function_type->function.params[i]  = t;
    i                                 += 1;
  }

  if (!return_type || has_param_type_error) {
    return nullptr;
  }

  Type *intern_type = ctx->types.add(function_type);

  return intern_type;
}

// Assumes that the AstNode `function` already has an associated type defined.
void infer_types_in_function(CompilerContext *ctx, Env *env, AstNode *function) {
  Debug_assert(function->kind == Ast_scope);

  Env *param_env = ctx->environments.alloc(env);

  ForEachAstNode(param, function->function.params) {
    param_env->insert(param->param.name->identifier.key, param);
  }

  ForEachAstNode(statement, function->scope.statements) {
    infer_types_in_statement(ctx, param_env, statement);
  }

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
    assert(type);

    value->type = type;

    Debug_assert(item->declaration.name->kind == Ast_identifier);

    mod_environment->insert(item->declaration.name->identifier.key, item->declaration.value);
  }

  ForEachAstNode(item, mod->module.items) {
    Debug_assert(item->kind == Ast_declaration);

    AstNode *function = item->declaration.value;
    Debug_assert(function->kind == Ast_function);

    infer_types_in_function(ctx, mod_environment, function);
  }

  return true;
}
