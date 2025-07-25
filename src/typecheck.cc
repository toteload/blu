#include "blu.hh"

struct TypeChecker {
  TypeCheckContext ctx;

  void init(TypeCheckContext ctx);

  Type *infer_ast_type(Env *env, AstNode *type);
  Type *infer_function_type(Env *env, AstNode *function);
  Type *infer_expression_type(Env *env, AstNode *expression);

  b32 check_function_body(Env *env, AstNode *function);
  b32 check_module(AstNode *module);
};

b32 is_type_coercible_to(Type *src, Type *dst) {
  if (src == dst) {
    return true;
  }

  if (src->kind == Type_IntegerConstant && dst->kind == Type_Integer) {
    return true;
  }

  return false;
}

b32 is_assignable(Env *env, AstNode *n) { return true; }

Type *determine_binary_op_type(TypeInterner *types, BinaryOpKind op, Type *lhs, Type *rhs) {
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
      return types->bool_;
    }

    Todo();
  } break;
  case Logical_or:
  case Logical_and: {
    if (lhs == rhs && lhs->kind == Type_Boolean) {
      return types->bool_;
    }

    Todo();
  } break;

  // All cases should be handled
  default:
    Unreachable();
    return nullptr;
  }
}

void TypeChecker::init(TypeCheckContext ctx) { this->ctx = ctx; }

Type *TypeChecker::infer_ast_type(Env *env, AstNode *type) {
  Debug_assert(
    type->kind == Ast_identifier || type->kind == Ast_type_pointer || type->kind == Ast_type_slice
  );

  if (type->kind == Ast_type_pointer) {
    Todo();
  }

  if (type->kind == Ast_type_slice) {
    Type *base      = infer_ast_type(env, type->slice.base);
    Type slice_type = Type::make_slice(base);

    return ctx.types->add(&slice_type);
  }

  StrKey idkey = type->identifier.key;
  Value *v     = env->lookup(idkey);

  if (!v) {
    Todo();
    // TODO also display the identifier to the user. would be nice to know what is wrong :)
    // Str msg        = ctx.arena->push_format_string(
    //   "Expected defined identifier, but it could not be found.\n"
    //);
    // ctx.messages->push({get_ast_source_span(type->span, tokens), Error, msg});

    return nullptr;
  }

  return v->type;
}

Type *TypeChecker::infer_expression_type(Env *env, AstNode *e) {
  Type *res = nullptr;

  switch (e->kind) {
  case Ast_declaration: {
    res = ctx.types->void_;

    auto &decl = e->declaration;

    Type *declared_type = infer_ast_type(env, decl.type);
    Type *v             = infer_expression_type(env, decl.value);

    if (!is_type_coercible_to(v, declared_type)) {
      // Type error
      Todo();
    }

    if (declared_type != v) {
      // Add an explicit cast if the type is coercible and not the same

      AstNode *n               = ctx.nodes->alloc();
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
    Debug_assert(is_assignable(env, e));

    Type *type = infer_expression_type(env, e->assign.value);
    Debug_assert(type);
  } break;
  case Ast_continue:
  case Ast_break: {
    res = ctx.types->never;
  } break;
  case Ast_return: {
    res = ctx.types->never;

    if (!e->return_.value) {
      break;
    }

    Type *type = infer_expression_type(env, e->return_.value);
    Debug_assert(type);
  } break;
  case Ast_while: {
    res = ctx.types->void_;

    Type *cond = infer_expression_type(env, e->while_.cond);
    Type *body = infer_expression_type(env, e->while_.body);

    Debug_assert(cond);
    Debug_assert(body);
  } break;
  case Ast_for: {
    res = ctx.types->void_;

    Type *iterable_type = infer_expression_type(env, e->for_.iterable);

    Debug_assert(iterable_type->kind == Type_slice);

    Type *base_type = iterable_type->slice.base_type;

    e->for_.item->type = base_type;

    Env *for_env = ctx.envs->alloc(env);
    for_env->insert(e->for_.item->identifier.key, Value::make_iter_item(base_type, e->for_.item));

    infer_expression_type(for_env, e->for_.body);
  } break;
  case Ast_scope: {
    Type *scope_type = ctx.types->void_;
    auto expressions = e->scope.expressions;
    ForEachAstNode(e, expressions) {
      infer_expression_type(env, e);

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
      Todo();
      // Str msg        = ctx.arena->push_format_string( "Could not find identifier.");
      // ctx.messages->push({e->span, Error, msg});
      break;
    }

    res = p->type;
  } break;
  case Ast_literal_int: {
    res = ctx.types->integer_constant;
  } break;
  case Ast_if_else: {
    Type *cond = infer_expression_type(env, e->if_else.cond);
    if (cond->kind != Type_Boolean) {
      Todo();
      // Str msg = ctx.arena->push_format_string("Condition in if-expression is not of type
      // 'bool'."); ctx.messages->push({e->if_else.cond->span, Error, msg});
    }

    Type *then = infer_expression_type(env, e->if_else.then);
    if (e->if_else.otherwise) {
      Type *otherwise = infer_expression_type(env, e->if_else.otherwise);

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
    if (!(then->kind == Type_Never || then->kind == Type_Void)) {
      Todo();
      // Str msg = ctx.arena->push_format_string("If-expression should return type 'void'.");
      // ctx.messages->push({e->if_else.then->span, Error, msg});
    }

    res = then;
  } break;
  case Ast_call: {
    Type *callee = infer_expression_type(env, e->call.callee);
    Debug_assert(callee->kind == Type_Function);

    u32 param_count = callee->function.param_count;

    u32 i = 0;
    ForEachAstNode(arg, e->call.args) {
      if (i == param_count) {
        // More arguments provided than the function takes parameters.
        break;
      }

      Type *arg_type   = infer_expression_type(env, arg);
      Type *param_type = callee->function.params[i];
      Debug_assert(is_type_coercible_to(arg_type, param_type));
      i += 1;
    }

    u32 arg_count = i;

    Debug_assert(param_count == arg_count);

    res = callee->function.return_type;
  } break;
  case Ast_binary_op: {
    Type *lhs = infer_expression_type(env, e->binary_op.lhs);
    Type *rhs = infer_expression_type(env, e->binary_op.rhs);

    Debug_assert(lhs && rhs);

    if (e->binary_op.kind == AddAssign) {
      Debug_assert(is_assignable(env, e->binary_op.lhs));
      Debug_assert(is_type_coercible_to(rhs, lhs));
      res = ctx.types->void_;
      break;
    }

    res = determine_binary_op_type(ctx.types, e->binary_op.kind, lhs, rhs);
  } break;
  case Ast_unary_op: {
    res = infer_expression_type(env, e->unary_op.value);

    // TODO: make sure that the unary operator is defined for the type of the expression
  } break;
  default:
    Unreachable();
  }

  e->type = res;

  return res;
}

Type *TypeChecker::infer_function_type(Env *env, AstNode *function) {
  Debug_assert(function->kind == Ast_function);

  Type *return_type = infer_ast_type(env, function->function.return_type);

  u32 param_count = 0;
  ForEachAstNode(param, function->function.params) { param_count += 1; }

  Type *function_type = cast<Type *>(
    ctx.work_arena->raw_alloc(sizeof(Type) + param_count * sizeof(Type *), Align_of(Type))
  );
  function_type->kind                 = Type_Function;
  function_type->function.param_count = param_count;
  function_type->function.return_type = return_type;

  b32 has_param_type_error = false;

  u32 i = 0;
  ForEachAstNode(param, function->function.params) {
    Type *t = infer_ast_type(env, param->param.type);
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

  Type *intern_type = ctx.types->add(function_type);

  function->type = intern_type;

  return intern_type;
}

b32 TypeChecker::check_function_body(Env *env, AstNode *function) {
  Debug_assert(function->kind == Ast_function);

  Env *param_env = ctx.envs->alloc(env);

  ForEachAstNode(param, function->function.params) {
    Type *param_type = infer_ast_type(env, param->param.type);
    param_env->insert(param->param.name->identifier.key, Value::make_param(param, param_type));
  }

  Type *t = infer_expression_type(param_env, function->function.body);

  Debug_assert(t);

  ctx.envs->dealloc(param_env);

  return true;
}

b32 TypeChecker::check_module(AstNode *module) {
  Env *mod_environment = ctx.envs->alloc(ctx.envs->global_env);

  // Declarations are added to the module environment.

  ForEachAstNode(item, module->module.items) {
    if (item->kind == Ast_builtin) {
      continue;
    }

    Debug_assert(item->kind == Ast_declaration);
    Debug_assert(item->declaration.name->kind == Ast_identifier);

    AstNode *value = item->declaration.value;

    Type *type = nullptr;

    if (value->kind == Ast_function) {
      type = infer_function_type(mod_environment, value);
    } else {
      type = infer_expression_type(mod_environment, value);
    }

    Debug_assert(type);

    item->type = type;

    mod_environment->insert(item->declaration.name->identifier.key, Value::make_local(item, type));
  }

  ForEachAstNode(item, module->module.items) {
    if (item->kind == Ast_builtin) {
      continue;
    }

    Debug_assert(item->kind == Ast_declaration);

    AstNode *value = item->declaration.value;

    if (value->kind == Ast_function) {
      check_function_body(mod_environment, value);
    }
  }

  return true;
}

b32 type_check_module(TypeCheckContext ctx, AstNode *module) {
  TypeChecker typechecker;
  typechecker.init(ctx);
  auto snapshot = ctx.work_arena->take_snapshot();
  b32 ok        = typechecker.check_module(module);
  ctx.work_arena->restore(snapshot);
  return ok;
}
