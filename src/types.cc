#include "blu.hh"

#if 0
b32 find_type();

void visit_module(CompilerContext *ctx, AstRef ref) {
  ForEachIndex(i, nodes[ref].module.items.len) {
    visit(ctx, nodes[ref].module.items[i]);
  }
}

void visit_function(CompilerContext *ctx, AstRef ref) {
  ForEachIndex(i, nodes[ref].function.params.len) {
    visit(ctx, nodes[ref].function.params[i]);
  }

  find_type(ctx, ctx->ast[ref].function.return_type);

  visit(ctx, nodes[ref].function.body);
}

void visit_param(CompilerContext *ctx, AstRef ref) {
  find_type(ctx, ctx->ast[ref].param.type);
}

void visit_scope(CompilerContext *ctx, AstRef ref) {
  ForEachIndex(i, nodes[ref].scope.statements.len) {
    visit(ctx, nodes[ref].scope.statements[i]);
  }
}

void visit_identifier(CompilerContext *ctx, AstRef ref) {}
void visit_literal_int(CompilerContext *ctx, AstRef ref) {}
void visit_declaration(CompilerContext *ctx, AstRef ref) {}
void visit_assign(CompilerContext *ctx, AstRef ref) {}
void visit_while(CompilerContext *ctx, AstRef ref) {}
void visit_break(CompilerContext *ctx, AstRef ref) {}
void visit_continue(CompilerContext *ctx, AstRef ref) {}
void visit_call(CompilerContext *ctx, AstRef ref) {}
void visit_if_else(CompilerContext *ctx, AstRef ref) {}
void visit_binary_op(CompilerContext *ctx, AstRef ref) {}
void visit_unary_op(CompilerContext *ctx, AstRef ref) {}
void visit_return(CompilerContext *ctx, AstRef ref) {}

void visit(CompilerContext *ctx, AstRef root) {
  AstNode n = nodes[root];

  // clang-format off
  switch (n.kind) {
    case Ast_module: visit_module(ctx, root); break;
    case Ast_function: visit_function(ctx, root); break;
    case Ast_param: visit_param(ctx, root); break;
    case Ast_scope: visit_scope(ctx, root); break;
    case Ast_identifier: visit_identifier(ctx, root); break;
    case Ast_literal_int: visit_literal_int(ctx, root); break;
    case Ast_declaration: visit_declaration(ctx, root); break;
    case Ast_assign: visit_assign(ctx, root); break;
    case Ast_while: visit_while(ctx, root); break;
    case Ast_continue: visit_break(ctx, root); break;
    case Ast_call: visit_call(ctx, root); break;
    case Ast_if_else: visit_if_else(ctx, root); break;
    case Ast_binary_op: visit_binary_op(ctx, root); break;
    case Ast_unary_op: visit_unary_op(ctx, root); break;
    case Ast_return: visit_return(ctx, root); break;
  }
  // clang-format on

  return true;
}
#endif

Type *infer_ast_type(CompilerContext *ctx, Env *env, AstNode *type) {
  Debug_assert(type->kind == Ast_identifier);

  StrKey idkey = type->identifier.key;
  Value *v     = env->lookup(idkey);

  if (!v) {
    Str identifier = ctx->strings.get(idkey);
    Str msg        = ctx->arena.push_format_string(
      "Expected type for identifier %.*s but it could not be found.\n",
      identifier.len,
      identifier.str
    );
    ctx->messages.push({type->span, Error, msg});

    return nullptr;
  }

  if (v->kind != Value_type) {
    Str identifier = ctx->strings.get(idkey);
    Str msg        = ctx->arena.push_format_string(
      "Expected type for identifier %.*s but it was not a type.\n",
      identifier.len,
      identifier.str
    );
    ctx->messages.push({type->span, Error, msg});

    return nullptr;
  }

  return v->type;
}

Type *infer_function_type(CompilerContext *ctx, Env *env, AstNode *function) {
  Debug_assert(function->kind == Ast_function);

  Type *return_type = infer_ast_type(ctx, env, function->function.return_type);
  Type *param_types = nullptr;

  b32 has_param_type_error = false;

  ForEachIndex(i, function->function.params.len) {
    AstNode *param = function->function.params[i];
    Type *t = infer_ast_type(ctx, env, param);
    if (!t) {
      has_param_type_error = true;
      break;
    }

    t->next = param_types;
    param_types = t;
  }

  if (!return_type || has_param_type_error) {
    return nullptr;
  } 

  Type *function_type = ctx->types.add(Type::make_function(return_type, param_types));

  return function_type;
}

b32 infer_types(CompilerContext *ctx, AstRef mod) {
  // - Add all items to environment.
  // - For each item, infer the types inside it.

  #if 0
  Env *mod_environment = ctx->environments.alloc(ctx->global_environment);

  b32 has_function_type_error = false;

  ForEachIndex(i, mod->module.items.len) {
    AstNode *item = mod->module.items[i];

    if (item->kind == Ast_function) {
      Type *t = infer_function_type(ctx, mod_environment, item);
      if (!t) {
        has_function_type_error = true;
        continue;
      }

      mod_environment->insert(t);
      continue;
    }

    Unreachable();
  }
  #endif

  return true;
}
