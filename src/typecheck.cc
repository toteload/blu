#include "blu.hh"

struct TypeChecker {
  MessageManager *messages;
  Arena *work_arena;
  EnvManager *envs;
  TypeInterner *types;
  StringInterner *strings;
  Source *source;
  Nodes *nodes;

  Type *return_type_of_current_function = nullptr;

  // -

  b32 check_root(NodeIndex root, Env *env);

  b32 determine_type(NodeIndex type, Env *env, Type **out);

  b32 determine_unary_op_type(UnaryOpKind op, Type *value, Type **out);
  b32 determine_binary_op_type(BinaryOpKind op, Type *lhs, Type *rhs, Type **out);

  b32 is_type_coercible_to(Type *src, Type *dst);

  b32 check_expression(NodeIndex expr_node_index, Env *env, Type **out);
  b32 check_block(NodeIndex node_index, Env *env, Type **out);
  b32 check_declaration(NodeIndex node_index, Env *env, Type **out);
  b32 check_assign(NodeIndex node_index, Env *env, Type **out);
  b32 check_identifier(NodeIndex node_index, Env *env, Type **out);
  b32 check_function(NodeIndex function, Env *env, Type **out);
  b32 check_call(NodeIndex function, Env *env, Type **out);
  b32 check_unary_op(NodeIndex function, Env *env, Type **out);
  b32 check_binary_op(NodeIndex function, Env *env, Type **out);
  b32 check_while(NodeIndex function, Env *env, Type **out);
  b32 check_if_else(NodeIndex function, Env *env, Type **out);
  b32 check_return(NodeIndex node_index, Env *env, Type **out);

  b32 is_assignable(NodeIndex node_index, Env *env) {
    AstKind kind = nodes->kind(node_index);
    if (kind != Ast_identifier) {
      return false;
    }
    return true;
  }
};

b32 TypeChecker::is_type_coercible_to(Type *src, Type *dst) {
  if (src == dst) {
    return true;
  }

  if (src->kind == Type_never) {
    return true;
  }

  if (src->kind == Type_integer_constant && dst->kind == Type_integer) {
    return true;
  }

  if (src->kind == Type_sequence && dst->kind == Type_slice) {
    ForEachIndex(i, src->sequence.count) {
      if (!is_type_coercible_to(src->sequence.items[i], dst->slice.base_type)) {
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

b32 TypeChecker::determine_unary_op_type(UnaryOpKind op, Type *value, Type **out) {
  switch (op) {
  case Negate: {
    if (value->kind == Type_integer_constant || (value->kind == Type_integer && value->integer.signedness == Signed)) {
      *out = value;
      return true;
    }
  } break;
  case Not: {
    if (value->kind == Type_integer_constant || value->kind == Type_integer || value->kind == Type_boolean) {
      *out = value;
      return true;
    }
  } break;
  }

  Todo();

  return false;
}

b32 TypeChecker::determine_binary_op_type(BinaryOpKind op, Type *lhs, Type *rhs, Type **out) {
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
    if (lhs == rhs && lhs->kind == Type_boolean) {
      *out = types->bool_;
      return true;
    }
  } break;
  }

  Todo();

  return false;
}

// b32 TypeChecker::infer_literal_tuple_type(Env *env, AstNode *tuple, Type **out) {
//   u32 item_count = 0;
//
//   ForEachAstNode(item, tuple->tuple.items) { item_count += 1; }
//
//   Type *t = cast<Type *>(
//     ctx.work_arena->raw_alloc(sizeof(Type) + item_count * sizeof(Type *), Align_of(Type))
//   );
//
//   t->kind        = Type_tuple;
//   t->tuple.count = item_count;
//
//   u32 i = 0;
//   ForEachAstNode(item, tuple->tuple.items) {
//     Type *item_type = nullptr;
//     Try(infer_expression_type(env, item, &item_type));
//
//     t->tuple.items[i]  = item_type;
//     i                 += 1;
//   }
//
//   *out = ctx.types->add(t);
//
//   return true;
// }
//
// b32 TypeChecker::infer_expression_type(Env *env, AstNode *e, Type **out) {
//   Type *res = nullptr;
//
//   switch (e->kind) {
//   case Ast_declaration: {
//     res = ctx.types->void_;
//
//     auto &decl = e->declaration;
//
//     Type *declared_type = nullptr;
//     Try(read_ast_type(env, decl.type, &declared_type));
//
//     Type *v = nullptr;
//     Try(infer_expression_type(env, decl.value, &v));
//
//     if (!is_type_coercible_to(v, declared_type)) {
//       ctx.messages->error(0, {}, "Cannot coerce type {type} to type {type}", v, declared_type);
//
//       return false;
//     }
//
//     if (declared_type != v) {
//       // Add an explicit cast if the type is coercible and not the same
//
//       AstNode *n               = ctx.nodes->alloc();
//       n->type                  = declared_type;
//       n->kind                  = Ast_cast;
//       n->cast.destination_type = nullptr;
//       n->cast.value            = decl.value;
//
//       decl.value = n;
//     }
//
//     env->insert(decl.name->identifier.key, Value::make_local(e, declared_type));
//
//     res = declared_type;
//   } break;
//   case Ast_type: {
//     Type *type = nullptr;
//     Try(read_ast_type(env, e, &type));
//     Type *type_type = ctx.types->add_as_type(type);
//     res             = type_type;
//   } break;
//   case Ast_assign: {
//     res = ctx.types->void_;
//
//     if (!is_assignable(env, e->assign.lhs)) {
//       Todo();
//       // ctx.messages->error();
//       return false;
//     }
//
//     Type *type = nullptr;
//     Try(infer_expression_type(env, e->assign.value, &type));
//   } break;
//   case Ast_continue:
//   case Ast_break: {
//     res = ctx.types->never;
//   } break;
//   case Ast_return: {
//     res = ctx.types->never;
//
//     if (!e->return_.value) {
//       break;
//     }
//
//     Type *type = nullptr;
//     Try(infer_expression_type(env, e->return_.value, &type));
//   } break;
//   case Ast_while: {
//     res = ctx.types->void_;
//
//     Type *cond = nullptr;
//     Try(infer_expression_type(env, e->while_.cond, &cond));
//
//     Type *body = nullptr;
//     Try(infer_expression_type(env, e->while_.body, &body));
//   } break;
//   case Ast_for: {
//     res = ctx.types->void_;
//
//     Type *iterable_type = nullptr;
//     Try(infer_expression_type(env, e->for_.iterable, &iterable_type));
//
//     if (iterable_type->kind != Type_slice) {
//       Todo();
//       return false;
//     }
//
//     Type *base_type = iterable_type->slice.base_type;
//
//     e->for_.item->type = base_type;
//
//     Env *for_env = ctx.envs->alloc(env);
//     for_env->insert(e->for_.item->identifier.key, Value::make_iter_item(base_type,
//     e->for_.item));
//
//     Type *body_type = nullptr;
//     Try(infer_expression_type(for_env, e->for_.body, &body_type));
//   } break;
//   case Ast_scope: {
//     res = ctx.types->void_;
//
//     auto exprs = e->scope.expressions;
//     ForEachAstNode(e, exprs) {
//       Type *t = nullptr;
//       Try(infer_expression_type(env, e, &t));
//     }
//   } break;
//   case Ast_identifier: {
//     auto idkey = e->identifier.key;
//     Value *p   = env->lookup(idkey);
//
//     if (!p) {
//       ctx.messages->error(
//         UINT32_MAX, // TODO
//         {},         // TODO
//         "Could not find identifier {strkey} at {astnode}.",
//         idkey,
//         e
//       );
//       Todo();
//       return false;
//     }
//
//     res = p->type;
//   } break;
//   case Ast_literal_int: {
//     res = ctx.types->integer_constant;
//   } break;
//   case Ast_literal_tuple: {
//     if (e->tuple.declared_type) {
//       // Slice literals must be written with an explicit type.
//       Todo();
//     }
//
//     Type *tuple_type = nullptr;
//     Try(infer_literal_tuple_type(env, e, &tuple_type));
//
//     res = tuple_type;
//   } break;
//   case Ast_if_else: {
//     Type *cond = nullptr;
//     Try(infer_expression_type(env, e->if_else.cond, &cond));
//     if (cond->kind != Type_Boolean) {
//       Todo();
//       // Str msg = ctx.arena->push_format_string("Condition in if-expression is not of type
//       // 'bool'."); ctx.messages->push({e->if_else.cond->span, Error, msg});
//     }
//
//     Type *then = nullptr;
//     Try(infer_expression_type(env, e->if_else.then, &then));
//
//     if (e->if_else.otherwise) {
//       Type *otherwise = nullptr;
//       Try(infer_expression_type(env, e->if_else.otherwise, &otherwise));
//
//       Type *final_type = nullptr;
//       if (is_type_coercible_to(then, otherwise)) {
//         final_type = otherwise;
//       } else if (is_type_coercible_to(otherwise, then)) {
//         final_type = then;
//       } else {
//         Todo();
//         break;
//       }
//
//       e->if_else.then->type      = final_type;
//       e->if_else.otherwise->type = final_type;
//
//       res = final_type;
//
//       break;
//     }
//
//     // We only have an if-branch, then the return type of the branch should be void.
//     if (!(then->kind == Type_Never || then->kind == Type_Void)) {
//       Todo();
//       // Str msg = ctx.arena->push_format_string("If-expression should return type 'void'.");
//       // ctx.messages->push({e->if_else.then->span, Error, msg});
//     }
//
//     res = then;
//   } break;
//   case Ast_call: {
//     Type *callee = nullptr;
//     Try(infer_expression_type(env, e->call.callee, &callee));
//     Debug_assert(callee->kind == Type_Function);
//
//     u32 param_count = callee->function.param_count;
//
//     u32 i = 0;
//     ForEachAstNode(arg, e->call.args) {
//       if (i == param_count) {
//         // More arguments provided than the function takes parameters.
//         break;
//       }
//
//       Type *arg_type = nullptr;
//       Try(infer_expression_type(env, arg, &arg_type));
//       Type *param_type = callee->function.params[i];
//
//       if (!is_type_coercible_to(arg_type, param_type)) {
//         Todo();
//         return false;
//       }
//
//       i += 1;
//     }
//
//     u32 arg_count = i;
//
//     Debug_assert(param_count == arg_count);
//
//     res = callee->function.return_type;
//   } break;
//   case Ast_binary_op: {
//     Type *lhs = nullptr;
//     Try(infer_expression_type(env, e->binary_op.lhs, &lhs));
//
//     Type *rhs = nullptr;
//     Try(infer_expression_type(env, e->binary_op.rhs, &rhs));
//
//     if (e->binary_op.kind == AddAssign) {
//       Debug_assert(is_assignable(env, e->binary_op.lhs));
//       Debug_assert(is_type_coercible_to(rhs, lhs));
//       res = ctx.types->void_;
//       break;
//     }
//
//     res = determine_binary_op_type(ctx.types, e->binary_op.kind, lhs, rhs);
//   } break;
//   case Ast_unary_op: {
//     Try(infer_expression_type(env, e->unary_op.value, &res));
//
//     // TODO: make sure that the unary operator is defined for the type of the expression
//   } break;
//   default:
//     Unreachable();
//   }
//
//   e->type = res;
//
//   *out = res;
//
//   return true;
// }

// b32 TypeChecker::infer_function_type(Env *env, AstNode *function, Type **out) {
//   Debug_assert(function->kind == Ast_function);
//
//   Type *return_type = nullptr;
//   Try(read_ast_type(env, function->function.return_type, &return_type));
//
//   u32 param_count = 0;
//   ForEachAstNode(param, function->function.params) { param_count += 1; }
//
//   Type *function_type = cast<Type *>(
//     ctx.work_arena->raw_alloc(sizeof(Type) + param_count * sizeof(Type *), Align_of(Type))
//   );
//
//   function_type->kind                 = Type_Function;
//   function_type->function.param_count = param_count;
//   function_type->function.return_type = return_type;
//
//   b32 has_param_type_error = false;
//
//   u32 i = 0;
//   ForEachAstNode(param, function->function.params) {
//     Type *t = nullptr;
//     Try(read_ast_type(env, param->param.type, &t));
//
//     if (!t) {
//       has_param_type_error = true;
//       break;
//     }
//
//     param->type = t;
//
//     function_type->function.params[i] = t;
//
//     i += 1;
//   }
//
//   if (!return_type || has_param_type_error) {
//     return false;
//   }
//
//   Type *intern_type = ctx.types->add(function_type);
//
//   function->type = intern_type;
//
//   *out = intern_type;
//
//   return true;
// }

// b32 TypeChecker::check_function_body(Env *env, AstNode *function) {
//   Debug_assert(function->kind == Ast_function);
//
//   Env *param_env = ctx.envs->alloc(env);
//
//   ForEachAstNode(param, function->function.params) {
//     Type *param_type = nullptr;
//     Try(read_ast_type(env, param->param.type, &param_type));
//     param_env->insert(param->param.name->identifier.key, Value::make_param(param, param_type));
//   }
//
//   Type *t = nullptr;
//   Try(infer_expression_type(param_env, function->function.body, &t));
//
//   // TODO: This is a memory leak if we encountered an error.
//   ctx.envs->dealloc(param_env);
//
//   return true;
// }

b32 TypeChecker::check_root(NodeIndex root, Env *env) {
  auto items = &nodes->data(root).root.items;

  // Add declarations to the environment
  for (usize i = 0; i < items->len(); i += 1) {
    Debug_assert(nodes->kind(items->at(i)) == Ast_declaration);

    auto decl = nodes->data(items->at(i)).declaration;

    StrKey key = strings->add(source->get_token_str(decl.name));

    env->insert(key, Value::make_declaration(decl.type, decl.value));
  }

  // Determine the declared types for all declarations
  for (usize i = 0; i < items->len(); i += 1) {
    auto decl = nodes->data(items->at(i)).declaration;

    StrKey key = strings->add(source->get_token_str(decl.name));

    Value *val = env->lookup(key);

    if (val->is_type_known()) {
      continue;
    }

    Try(determine_type(val->data.declaration.type_node_index, env, &val->type));
  }

  // At this point each declaration has its declared type determined.

  for (usize i = 0; i < items->len(); i += 1) {
    auto decl = nodes->data(items->at(i)).declaration;

    AstKind value_kind = nodes->kind(decl.value);

    Type *value_type = nullptr;
    Try(check_expression(decl.value, env, &value_type));

    // TODO: make sure that the declared type and the value type are compatible
  }

  return true;
}

b32 TypeChecker::determine_type(NodeIndex node_index, Env *env, Type **out) {
  auto data = nodes->data(node_index).type;

  Type *type = nullptr;

  switch (data.kind) {
  case Ast_type_identifier: {
    StrKey key = strings->add(source->get_token_str(data.data.token_index));
    Value *val = env->lookup(key);
    if (!val || val->kind == Value_param) {
      Todo();
    }

    if (val->kind == Value_lazy_declaration) {
      Try(determine_type(val->data.declaration.value_node_index, env, &val->type));
    }

    Debug_assert(val->type);

    type = val->type;
  } break;
  case Ast_type_slice: {
    Type *base = nullptr;
    Try(determine_type(data.data.base, env, &base));

    Type slice_type = Type::make_slice(base);

    type = types->add(&slice_type);
  } break;
  case Ast_type_function: {
    Type *return_type = nullptr;
    Try(determine_type(data.data.function.return_type, env, &return_type));

    usize param_count = data.data.function.params.len();

    Type *function_type = cast<Type *>(
      work_arena->raw_alloc(sizeof(Type) + param_count * sizeof(Type *), Align_of(Type))
    );

    function_type->kind                 = Type_function;
    function_type->function.return_type = return_type;
    function_type->function.param_count = param_count;

    for (usize i = 0; i < param_count; i += 1) {
      Try(determine_type(data.data.function.params[i], env, &function_type->function.params[i]));
    }

    type = types->add(function_type);
  } break;
  }

  Debug_assert(type);

  if (data.flags & Distinct) {
    type = types->add_as_distinct(type);
  }

  *out = type;

  return true;
}

b32 TypeChecker::check_expression(NodeIndex expr_node_index, Env *env, Type **out) {
  AstKind kind = nodes->kind(expr_node_index);

  switch (kind) {
    // clang-format off
  case Ast_type:        Try(determine_type(expr_node_index, env, out)); break;
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
    // clang-format on
  }

  return true;
}

b32 TypeChecker::check_block(NodeIndex node_index, Env *env, Type **out) {
  auto items = &nodes->data(node_index).block.items;

  if (items->len() == 0) {
    *out = types->nil;
    return true;
  }

  Env *block_env = envs->alloc(env);

  for (usize i = 0; i < items->len() - 1; i += 1) {
    Type *type;
    Try(check_expression(items->at(i), block_env, &type));
  }

  Try(check_expression(items->at(items->len() - 1), block_env, out));

  envs->dealloc(block_env);

  return true;
}

b32 TypeChecker::check_return(NodeIndex node_index, Env *env, Type **out) {
  Type *return_type;
  Try(check_expression(nodes->data(node_index).return_.value, env, &return_type));

  if (!is_type_coercible_to(return_type, return_type_of_current_function)) {
    Todo();
  }

  *out = types->never;

  return true;
}

b32 TypeChecker::check_function(NodeIndex function_node_index, Env *env, Type **out) {
  auto function = nodes->data(function_node_index).function;

  Env *param_env = envs->alloc(env);

  usize param_count = function.params.len();

  Type *function_type =
    cast<Type *>(work_arena->raw_alloc(sizeof(Type) + param_count * sizeof(Type *), Align_of(Type))
    );

  function_type->kind                 = Type_function;
  function_type->function.param_count = param_count;

  for (usize i = 0; i < param_count; i += 1) {
    TokenIndex name = function.params[i].name;
    StrKey key      = strings->add(source->get_token_str(name));

    Type *param_type;
    Try(determine_type(function.params[i].type, env, &param_type));

    function_type->function.params[i] = param_type;

    param_env->insert(key, Value::make_param(name, param_type));
  }

  Type *return_type;
  Try(determine_type(function.return_type, env, &return_type));

  function_type->function.return_type = return_type;

  return_type_of_current_function = return_type;

  Type *body_type;
  Try(check_expression(function.body, param_env, &body_type));

  return_type_of_current_function = nullptr;

  if (!is_type_coercible_to(body_type, return_type)) {
    Todo();
  }

  *out = function_type;

  return true;
}

b32 TypeChecker::check_if_else(NodeIndex expr, Env *env, Type **out) {
  auto data = nodes->data(expr).if_else;

  Type *cond;
  Try(check_expression(data.cond, env, &cond));

  Type *then;
  Try(check_expression(data.then, env, &then));

  if (data.otherwise.is_none()) {
    if (!is_type_coercible_to(then, types->never)) {
      Todo();
    }

    *out = types->never;

    return true;
  }

  Type *otherwise;
  Try(check_expression(NodeIndex::from_optional(data.otherwise), env, &otherwise));

  Type *final_type;
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

b32 TypeChecker::check_unary_op(NodeIndex node_index, Env *env, Type **out) {
  auto unary_op = nodes->data(node_index).unary_op;

  Type *type;
  Try(check_expression(unary_op.value, env, &type));

  Try(determine_unary_op_type(unary_op.kind, type, out));

  return true;
}

b32 TypeChecker::check_binary_op(NodeIndex node_index, Env *env, Type **out) {
  auto binary_op = nodes->data(node_index).binary_op;

  Type *lhs;
  Try(check_expression(binary_op.lhs, env, &lhs));

  Type *rhs;
  Try(check_expression(binary_op.rhs, env, &rhs));

  Try(determine_binary_op_type(binary_op.kind, lhs, rhs, out));

  return true;
}

b32 TypeChecker::check_while(NodeIndex node_index, Env *env, Type **out) {
  auto while_ = nodes->data(node_index).while_;

  Type *cond_type;
  Try(check_expression(while_.cond, env, &cond_type));

  if (cond_type->kind != Type_boolean) {
    Todo();
  }

  Type *body_type;
  Try(check_expression(while_.body, env, &body_type));

  *out = types->nil;

  return true;
}

b32 TypeChecker::check_call(NodeIndex node_index, Env *env, Type **out) {
  auto call = nodes->data(node_index).call;

  Type *callee_type;
  Try(check_expression(call.callee, env, &callee_type));

  if (callee_type->kind != Type_function) {
    Todo();
  }

  if (callee_type->function.param_count != call.args.len()) {
    Todo();
  }

  for (usize i = 0; i < call.args.len(); i += 1) {
    Type *arg_type;
    Try(check_expression(call.args[i], env, &arg_type));

    if (!is_type_coercible_to(arg_type, callee_type->function.params[i])) {
      Todo();
    }
  }

  *out = callee_type->function.return_type;

  return true;
}

b32 TypeChecker::check_assign(NodeIndex node_index, Env *env, Type **out) {
  auto assign = nodes->data(node_index).assign;

  Type *lhs_type;
  Try(check_expression(assign.lhs, env, &lhs_type));

  if (!is_assignable(assign.lhs, env)) {
    Todo();
  }

  Type *value_type;
  Try(check_expression(assign.value, env, &value_type));

  if (!is_type_coercible_to(value_type, lhs_type)) {
    Todo();
  }

  *out = types->nil;

  return true;
}

b32 TypeChecker::check_identifier(NodeIndex node_index, Env *env, Type **out) {
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

b32 TypeChecker::check_declaration(NodeIndex node_index, Env *env, Type **out) {
  auto decl = nodes->data(node_index).declaration;

  Type *declared_type;
  Try(determine_type(decl.type, env, &declared_type));

  Type *value_type;
  Try(check_expression(decl.value, env, &value_type));

  if (!is_type_coercible_to(value_type, declared_type)) {
    Todo();
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
