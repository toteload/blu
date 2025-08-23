#include "blu.hh"

struct TypeChecker {
  TypeCheckContext ctx;

  void init(TypeCheckContext ctx);

  b32 read_ast_type(Env *env, AstNode *type, Type **out);

  b32 infer_function_type(Env *env, AstNode *function, Type **out);
  b32 infer_expression_type(Env *env, AstNode *expression, Type **out);

  b32 infer_literal_tuple_type(Env *env, AstNode *tuple, Type **out);

  b32 check_function_body(Env *env, AstNode *function);

  b32 check_toplevel(AstNode *root, Env *env);
  b32 check_toplevel_function_bodies(AstNode *root, Env *env);
};

b32 is_type_coercible_to(Type *src, Type *dst) {
  if (src == dst) {
    return true;
  }

  if (src->kind == Type_IntegerConstant && dst->kind == Type_Integer) {
    return true;
  }

  if (src->kind == Type_tuple && dst->kind == Type_slice) {
    ForEachIndex(i, src->tuple.count) {
      if (!is_type_coercible_to(src->tuple.items[i], dst->slice.base_type)) {
        return false;
      }
    }

    return true;
  }

  if (dst->kind == Type_distinct) {
    if (src->kind == Type_distinct && dst->distinct.uid != src->distinct.uid) {
      return false;
    }

    return is_type_coercible_to(src, dst->distinct.base);
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

b32 TypeChecker::read_ast_type(Env *env, AstNode *type, Type **out) {
  Debug_assert(type->kind == Ast_type);

  Type *t = nullptr;

  switch (type->ast_type.kind) {
  case Ast_type_pointer: {
    Type *base = nullptr;
    Try(read_ast_type(env, type->ast_type.base, &base));
    Type pointer_type = Type::make_pointer(base);
    t                 = ctx.types->add(&pointer_type);
  } break;
  case Ast_type_slice: {
    Type *base = nullptr;
    Try(read_ast_type(env, type->ast_type.base, &base));
    Type slice_type = Type::make_slice(base);
    t               = ctx.types->add(&slice_type);
  } break;
  case Ast_type_identifier: {
      // TODO: also support field accessed identifiers
    Debug_assert(type->ast_type.base->kind == Ast_identifier);

    StrKey idkey = type->ast_type.base->identifier.key;
    Value *v     = env->lookup(idkey);

    if (!v || v->type->kind != Type_type) {
      Todo();
    }

    t = v->type->type.base;
  } break;
  }

  Debug_assert(t);

  if (type->ast_type.flags & Distinct) {
    t = ctx.types->add_as_distinct(t);
  }

  *out = t;

  return true;
}

b32 TypeChecker::infer_literal_tuple_type(Env *env, AstNode *tuple, Type **out) {
  u32 item_count = 0;

  ForEachAstNode(item, tuple->tuple.items) { item_count += 1; }

  Type *t = cast<Type *>(
    ctx.work_arena->raw_alloc(sizeof(Type) + item_count * sizeof(Type *), Align_of(Type))
  );

  t->kind        = Type_tuple;
  t->tuple.count = item_count;

  u32 i = 0;
  ForEachAstNode(item, tuple->tuple.items) {
    Type *item_type = nullptr;
    Try(infer_expression_type(env, item, &item_type));

    t->tuple.items[i]  = item_type;
    i                 += 1;
  }

  *out = ctx.types->add(t);

  return true;
}

b32 TypeChecker::infer_expression_type(Env *env, AstNode *e, Type **out) {
  Type *res = nullptr;

  switch (e->kind) {
  case Ast_declaration: {
    res = ctx.types->void_;

    auto &decl = e->declaration;

    Type *declared_type = nullptr;
    Try(read_ast_type(env, decl.type, &declared_type));

    Type *v = nullptr;
    Try(infer_expression_type(env, decl.value, &v));

    if (!is_type_coercible_to(v, declared_type)) {
      ctx.messages->error(0, {}, "Cannot coerce type {type} to type {type}", v, declared_type);

      return false;
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
  case Ast_type: {
    Type *type = nullptr;
    Try(read_ast_type(env, e, &type));
    Type *type_type = ctx.types->add_as_type(type);
    res             = type_type;
  } break;
  case Ast_assign: {
    res = ctx.types->void_;

    if (!is_assignable(env, e->assign.lhs)) {
      Todo();
      // ctx.messages->error();
      return false;
    }

    Type *type = nullptr;
    Try(infer_expression_type(env, e->assign.value, &type));
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

    Type *type = nullptr;
    Try(infer_expression_type(env, e->return_.value, &type));
  } break;
  case Ast_while: {
    res = ctx.types->void_;

    Type *cond = nullptr;
    Try(infer_expression_type(env, e->while_.cond, &cond));

    Type *body = nullptr;
    Try(infer_expression_type(env, e->while_.body, &body));
  } break;
  case Ast_for: {
    res = ctx.types->void_;

    Type *iterable_type = nullptr;
    Try(infer_expression_type(env, e->for_.iterable, &iterable_type));

    if (iterable_type->kind != Type_slice) {
      Todo();
      return false;
    }

    Type *base_type = iterable_type->slice.base_type;

    e->for_.item->type = base_type;

    Env *for_env = ctx.envs->alloc(env);
    for_env->insert(e->for_.item->identifier.key, Value::make_iter_item(base_type, e->for_.item));

    Type *body_type = nullptr;
    Try(infer_expression_type(for_env, e->for_.body, &body_type));
  } break;
  case Ast_scope: {
    res = ctx.types->void_;

    auto exprs = e->scope.expressions;
    ForEachAstNode(e, exprs) {
      Type *t = nullptr;
      Try(infer_expression_type(env, e, &t));
    }
  } break;
  case Ast_identifier: {
    auto idkey = e->identifier.key;
    Value *p   = env->lookup(idkey);

    if (!p) {
      ctx.messages->error(
        UINT32_MAX, // TODO
        {},         // TODO
        "Could not find identifier {strkey} at {astnode}.",
        idkey,
        e
      );
      Todo();
      return false;
    }

    res = p->type;
  } break;
  case Ast_literal_int: {
    res = ctx.types->integer_constant;
  } break;
  case Ast_literal_tuple: {
    if (e->tuple.declared_type) {
      // Slice literals must be written with an explicit type.
      Todo();
    }

    Type *tuple_type = nullptr;
    Try(infer_literal_tuple_type(env, e, &tuple_type));

    res = tuple_type;
  } break;
  case Ast_if_else: {
    Type *cond = nullptr;
    Try(infer_expression_type(env, e->if_else.cond, &cond));
    if (cond->kind != Type_Boolean) {
      Todo();
      // Str msg = ctx.arena->push_format_string("Condition in if-expression is not of type
      // 'bool'."); ctx.messages->push({e->if_else.cond->span, Error, msg});
    }

    Type *then = nullptr;
    Try(infer_expression_type(env, e->if_else.then, &then));

    if (e->if_else.otherwise) {
      Type *otherwise = nullptr;
      Try(infer_expression_type(env, e->if_else.otherwise, &otherwise));

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
    Type *callee = nullptr;
    Try(infer_expression_type(env, e->call.callee, &callee));
    Debug_assert(callee->kind == Type_Function);

    u32 param_count = callee->function.param_count;

    u32 i = 0;
    ForEachAstNode(arg, e->call.args) {
      if (i == param_count) {
        // More arguments provided than the function takes parameters.
        break;
      }

      Type *arg_type = nullptr;
      Try(infer_expression_type(env, arg, &arg_type));
      Type *param_type = callee->function.params[i];

      if (!is_type_coercible_to(arg_type, param_type)) {
        Todo();
        return false;
      }

      i += 1;
    }

    u32 arg_count = i;

    Debug_assert(param_count == arg_count);

    res = callee->function.return_type;
  } break;
  case Ast_binary_op: {
    Type *lhs = nullptr;
    Try(infer_expression_type(env, e->binary_op.lhs, &lhs));

    Type *rhs = nullptr;
    Try(infer_expression_type(env, e->binary_op.rhs, &rhs));

    if (e->binary_op.kind == AddAssign) {
      Debug_assert(is_assignable(env, e->binary_op.lhs));
      Debug_assert(is_type_coercible_to(rhs, lhs));
      res = ctx.types->void_;
      break;
    }

    res = determine_binary_op_type(ctx.types, e->binary_op.kind, lhs, rhs);
  } break;
  case Ast_unary_op: {
    Try(infer_expression_type(env, e->unary_op.value, &res));

    // TODO: make sure that the unary operator is defined for the type of the expression
  } break;
  default:
    Unreachable();
  }

  e->type = res;

  *out = res;

  return true;
}

b32 TypeChecker::infer_function_type(Env *env, AstNode *function, Type **out) {
  Debug_assert(function->kind == Ast_function);

  Type *return_type = nullptr;
  Try(read_ast_type(env, function->function.return_type, &return_type));

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
    Type *t = nullptr;
    Try(read_ast_type(env, param->param.type, &t));

    if (!t) {
      has_param_type_error = true;
      break;
    }

    param->type = t;

    function_type->function.params[i] = t;

    i += 1;
  }

  if (!return_type || has_param_type_error) {
    return false;
  }

  Type *intern_type = ctx.types->add(function_type);

  function->type = intern_type;

  *out = intern_type;

  return true;
}

b32 TypeChecker::check_function_body(Env *env, AstNode *function) {
  Debug_assert(function->kind == Ast_function);

  Env *param_env = ctx.envs->alloc(env);

  ForEachAstNode(param, function->function.params) {
    Type *param_type = nullptr;
    Try(read_ast_type(env, param->param.type, &param_type));
    param_env->insert(param->param.name->identifier.key, Value::make_param(param, param_type));
  }

  Type *t = nullptr;
  Try(infer_expression_type(param_env, function->function.body, &t));

  // TODO: This is a memory leak if we encountered an error.
  ctx.envs->dealloc(param_env);

  return true;
}

b32 TypeChecker::check_toplevel(AstNode *root, Env *env) {
  // TODO
  // The current setup does not take into consideration that the value of a declaration may depend
  // on the value of a declaration ahead.
  // You can use functions that have not been declared yet, but that is it.

  b32 ok = true;

  ForEachAstNode(item, root->module.items) {
    if (item->kind == Ast_builtin) {
      if (item->builtin.kind != Builtin_include) {
        Todo();
      }

      ok = check_toplevel((*ctx.sources)[item->builtin.src_idx].mod, env);
      continue;
    }

    Debug_assert(item->kind == Ast_declaration);
    Debug_assert(item->declaration.name->kind == Ast_identifier);

    AstNode *value = item->declaration.value;

    if (value->kind == Ast_module) {
      continue;
    }

    Type *type = nullptr;

    if (value->kind == Ast_function) {
      Try(infer_function_type(env, value, &type));
    } else {
      Try(infer_expression_type(env, value, &type));
    }

    item->type = type;

    env->insert(item->declaration.name->identifier.key, Value::make_local(item, type));
  }

  return ok;
}

b32 TypeChecker::check_toplevel_function_bodies(AstNode *root, Env *env) {
  b32 ok = true;

  ForEachAstNode(item, root->module.items) {
    if (item->kind == Ast_builtin) {
      if (item->builtin.kind != Builtin_include) {
        Todo();
      }

      ok = check_toplevel_function_bodies((*ctx.sources)[item->builtin.src_idx].mod, env);
      continue;
    }

    Debug_assert(item->kind == Ast_declaration);

    AstNode *value = item->declaration.value;

    if (value->kind == Ast_function) {
      Try(check_function_body(env, value));
    }
  }

  return ok;
}

b32 type_check_module(TypeCheckContext ctx, AstNode *module) {
  TypeChecker typechecker;
  typechecker.init(ctx);
  auto snapshot = ctx.work_arena->take_snapshot();

  Env *root_environment = ctx.envs->alloc(ctx.envs->global_env);
  Try(typechecker.check_toplevel(module, root_environment));
  Try(typechecker.check_toplevel_function_bodies(module, root_environment));
  // TODO free root_environment
  ctx.work_arena->restore(snapshot);
  return true;
}
