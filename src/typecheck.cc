#include "blu.hh"

struct TypeHint {
  TypeIndex type;
  NodeIndex location;
};

struct TypeChecker {
  Messages                *messages;
  TypeInterner            *types;
  EnvManager<Declaration> *envs;
  StringInterner          *strings;
  Arena                   *work_arena;
  ParsedSource            *source;

  Slice<TypeIndex> annotations;

  b32 typecheck(NodeIndex idx_root);

  b32 resolve_declaration(Env<Declaration> *env, NodeIndex decl_node);

  b32 eval_type_expression(Env<Declaration> *env, NodeIndex node_index, TypeIndex *result);
  b32 check_expression(
    Env<Declaration> *env, NodeIndex node_index, TypeHint *hint, TypeIndex *result
  );

  b32 check_coercion(TypeIndex type_dst, TypeIndex type_src, NodeIndex node_index_src);
  b32 check_unification(
    NodeIndex lhs, TypeIndex type_lhs, NodeIndex rhs, TypeIndex type_rhs, TypeIndex *result
  );

  b32 check_is_of_type(NodeIndex at, TypeIndex type, TypeIndex expected_type);

  b32 check_is_declaration_resolved(Declaration decl, NodeIndex at);
  b32 check_is_declaration_of_type(Declaration decl, NodeIndex at);
  b32 check_is_indexable(TypeIndex type, NodeIndex at);
  b32 check_is_assignable(NodeIndex node_index);

  StrKey intern_identifier(TokenIndex identifier);

  b32 find_identifier(Env<Declaration> *env, TokenIndex identifier, Declaration *result);
};

struct RootEnvPopulator {
  Env<Declaration> *env;
  StringInterner   *strings;
  TypeInterner     *types;

  void insert(DeclarationKind kind, Str s, TypeIndex type) {
    auto key = strings->add(s);
    env->insert(key, {.kind = kind, .type = type});
  }

  void populate();
};

void RootEnvPopulator::populate() {
  // clang-format off
  insert(Declaration_of_type, STR("i8"),  types->type.i8_);
  insert(Declaration_of_type, STR("i16"), types->type.i16_);
  insert(Declaration_of_type, STR("i32"), types->type.i32_);
  insert(Declaration_of_type, STR("i64"), types->type.i64_);

  insert(Declaration_of_type, STR("u8"),  types->type.u8_);
  insert(Declaration_of_type, STR("u16"), types->type.u16_);
  insert(Declaration_of_type, STR("u32"), types->type.u32_);
  insert(Declaration_of_type, STR("u64"), types->type.u64_);

  insert(Declaration_of_type, STR("uint"), types->type.uint);

  insert(Declaration_of_type, STR("bool"),  types->type.bool_);
  insert(Declaration_of_type, STR("nil"),   types->type.nil);
  insert(Declaration_of_type, STR("never"), types->type.never);
  insert(Declaration_of_type, STR("type"),  types->type.type);

  insert(Declaration_of_value, STR("true"),  types->type.bool_);
  insert(Declaration_of_value, STR("false"), types->type.bool_);
  // clang-format on
}

b32 typecheck(
  TypeCheckContext *context, ParsedSource *source, Slice<TypeIndex> annotations, NodeIndex idx_root
) {
  TypeChecker checker = {
    .messages    = context->messages,
    .types       = context->types,
    .envs        = context->envs,
    .strings     = context->strings,
    .work_arena  = context->work_arena,
    .source      = source,
    .annotations = annotations,
  };

  return checker.typecheck(idx_root);
}

b32 TypeChecker::typecheck(NodeIndex idx_root) {
  Assert(source->nodes->kind(idx_root) == Ast_root);

  auto env_root = envs->alloc(nullptr);
  {
    RootEnvPopulator populator = {
      .env     = env_root,
      .strings = strings,
      .types   = types,
    };

    populator.populate();
  }
  auto env = envs->alloc(env_root);

  // Add all root level declarations to the environment.
  auto root = source->nodes->data(idx_root).root;
  for (u32 i = 0; i < root.items.len(); i++) {
    Assert(source->nodes->kind(root.items[i]) == Ast_declaration);

    auto decl = source->nodes->data(root.items[i]).declaration;

    auto key = intern_identifier(decl.name);

    env->insert(
      key,
      {
        .kind       = Declaration_unresolved,
        .node_index = root.items[i],
      }
    );
  }

  for (u32 i = 0; i < root.items.len(); i++) {
    Try(resolve_declaration(env, root.items[i]));
  }

  return true;
}

b32 TypeChecker::resolve_declaration(Env<Declaration> *env, NodeIndex decl_node) {
  auto decl = source->nodes->data(decl_node).declaration;
  auto key  = intern_identifier(decl.name);

  Declaration existing;
  if (env->lookup(key, &existing) && existing.kind != Declaration_unresolved) {
    return true;
  }

  // Mark as in-progress so recursive references during resolution can be detected as cycles.
  env->insert(key, {.kind = Declaration_resolving, .node_index = decl_node});

  TypeIndex declared_type;
  Try(eval_type_expression(env, decl.type, &declared_type));

  if (declared_type == types->type.type) {
    TypeIndex value_type;
    Try(eval_type_expression(env, decl.value, &value_type));
    env->insert(key, {.kind = Declaration_of_type, .type = value_type});
    return true;
  }

  // Insert with the declared type up front so that recursive references
  // inside the value expression can be resolved.
  env->insert(key, {.kind = Declaration_of_value, .type = declared_type});

  TypeHint hint = {
    .type     = declared_type,
    .location = decl.type,
  };

  TypeIndex value_type;
  Try(check_expression(env, decl.value, &hint, &value_type));
  Try(check_coercion(declared_type, value_type, decl.value));

  return true;
}

b32 TypeChecker::eval_type_expression(Env<Declaration> *env, NodeIndex node_index, TypeIndex *out) {
  auto kind = source->nodes->kind(node_index);

  TypeIndex result;

  switch (kind) {
  case Ast_type_function: {
    auto f = source->nodes->data(node_index).type_function;

    u32  param_count = f.param_types.len();
    auto ty          = alloc_type_function(work_arena, param_count);

    *ty = {
      .kind     = Type_function,
      .function = {
        .return_type = {},
        .param_count = param_count,
        .param_types = {},
      },
    };

    Try(eval_type_expression(env, f.return_type, &ty->function.return_type));

    for (u32 i = 0; i < param_count; i++) {
      Try(eval_type_expression(env, f.param_types[i], &ty->function.param_types[i]));
    }

    result = types->add(ty);
  } break;
  case Ast_type_slice: {
    auto      slice = source->nodes->data(node_index).type_slice;
    TypeIndex base_type;
    Try(eval_type_expression(env, slice.base, &base_type));
    Type type = {
      .kind  = Type_slice,
      .slice = {.base_type = base_type},
    };
    result = types->add(&type);
  } break;
  case Ast_type_array: {
    auto array = source->nodes->data(node_index).type_array;

    TypeIndex size_type;
    Try(check_expression(env, array.size, nullptr, &size_type));

    // For now hardcode that the size must be a literal int.
    Try(check_coercion(types->type.literal_int, size_type, array.size));

    TypeIndex base_type;
    Try(eval_type_expression(env, array.base, &base_type));

    auto token_index = source->nodes->data(array.size).literal_int.token_index;
    auto str         = get_token_str(source->text, source->tokens, token_index);
    auto size        = parse_i64(str);

    Type type = {
      .kind  = Type_array,
      .array = {
        .base_type = base_type,
        .size      = size,
      },
    };
    result = types->add(&type);
  } break;
  case Ast_identifier: {
    auto token_index = source->nodes->data(node_index).identifier.token_index;

    Declaration decl;
    Try(find_identifier(env, token_index, &decl));

    if (decl.kind == Declaration_unresolved) {
      Try(resolve_declaration(env, decl.node_index));
      Try(find_identifier(env, token_index, &decl));
    }

    Try(check_is_declaration_resolved(decl, node_index));
    Try(check_is_declaration_of_type(decl, node_index));

    result = decl.type;
  } break;

  case Ast_block:
  case Ast_builtin:
  case Ast_declaration:
  case Ast_literal_int:
  case Ast_function:
  case Ast_if_else:
  case Ast_assign:
  case Ast_literal_sequence:
  case Ast_literal_string:
  case Ast_call:
  case Ast_index:
  case Ast_unary_op:
  case Ast_binary_op:
  case Ast_for:
  case Ast_break:
  case Ast_continue:
  case Ast_return:
  case Ast_defer:
  case Ast_const:
  case Ast_cast:
  case Ast_kind_max:
  case Ast_root:
    Todo();
    return false;
  }

  annotations[node_index.idx] = result;
  *out                        = result;

  return true;
}

b32 TypeChecker::check_expression(
  Env<Declaration> *env, NodeIndex node_index, TypeHint *hint, TypeIndex *out
) {
  auto kind = source->nodes->kind(node_index);

  TypeIndex result;

  switch (kind) {
  case Ast_cast: {
    Todo();
  } break;

  case Ast_type_function: {
    auto f = source->nodes->data(node_index).type_function;

    TypeIndex return_type;
    Try(check_expression(env, f.return_type, nullptr, &return_type));
    Try(check_is_of_type(f.return_type, return_type, types->type.type));

    for (u32 i = 0; i < f.param_types.len(); i++) {
      TypeIndex param_type;
      Try(check_expression(env, f.param_types[i], nullptr, &param_type));
      Try(check_is_of_type(f.param_types[i], param_type, types->type.type));
    }

    result = types->type.type;
  } break;
  case Ast_type_slice: {
    result = types->type.type;
  } break;
  case Ast_type_array: {
    result = types->type.type;
  } break;
  case Ast_literal_int: {
    result = types->type.literal_int;
  } break;
  case Ast_literal_string: {
    auto token_index = source->nodes->data(node_index).literal_string.token_index;
    auto s           = get_token_str(source->text, source->tokens, token_index);
    auto size        = string_literal_byte_size(s);
    Type type        = {
      .kind  = Type_array,
      .array = {
        .base_type = types->type.u8_,
        .size      = size,
      },
    };
    result = types->add(&type);
  } break;
  case Ast_identifier: {
    auto token_index = source->nodes->data(node_index).identifier.token_index;

    Declaration decl;
    Try(find_identifier(env, token_index, &decl));

    if (decl.kind == Declaration_unresolved) {
      Try(resolve_declaration(env, decl.node_index));
      Try(find_identifier(env, token_index, &decl));
    }

    Try(check_is_declaration_resolved(decl, node_index));

    switch (decl.kind) {
    case Declaration_of_value:
      result = decl.type;
      break;
    case Declaration_of_type:
      result = types->type.type;
      break;
    case Declaration_unresolved:
    case Declaration_resolving:
      Unreachable();
      break;
    }
  } break;
  case Ast_function: {
    auto f           = source->nodes->data(node_index).function;
    u32  param_count = cast<u32>(f.param_names.len());

    Type *hint_type = nullptr;
    if (hint) {
      auto ht = types->get(hint->type);
      if (ht->kind == Type_function) {
        hint_type = ht;
      }
    }

    if (param_count > 0 && !hint_type) {
      messages->error(
        node_index,
        "Cannot infer function parameter types without a function-type annotation."
      );
      return false;
    }

    if (hint_type && hint_type->function.param_count != param_count) {
      messages->error(
        node_index,
        "Function literal has wrong number of parameters for type {type}.",
        hint->type
      );
      return false;
    }

    auto env_params = envs->alloc(env);
    defer(envs->dealloc(env_params));

    for (u32 i = 0; i < param_count; i++) {
      auto param_node  = f.param_names[i];
      auto token_index = source->nodes->data(param_node).identifier.token_index;
      auto key         = intern_identifier(token_index);
      auto param_type  = hint_type->function.param_types[i];

      env_params->insert(key, {.kind = Declaration_of_value, .type = param_type});
      annotations[param_node.idx] = param_type;
    }

    TypeIndex body_type;
    Try(check_expression(env_params, f.body, nullptr, &body_type));

    if (hint_type) {
      Try(check_coercion(hint_type->function.return_type, body_type, f.body));
      result = hint->type;
    } else {
      Type function_type = {
        .kind             = Type_literal_function,
        .literal_function = {
          .return_type = body_type,
          .param_count = param_count,
        },
      };
      result = types->add(&function_type);
    }
  } break;
  case Ast_block: {
    auto block = source->nodes->data(node_index).block;
    if (block.items.len() == 0) {
      result = types->type.nil;
      break;
    }

    auto env_block = envs->alloc(env);
    defer(envs->dealloc(env_block));

    for (u32 i = 0; i < block.items.len() - 1; i++) {
      TypeIndex e;
      Try(check_expression(env_block, block.items[i], nullptr, &e));
    }

    Try(check_expression(env_block, block.items[block.items.len() - 1], nullptr, &result));
  } break;
  case Ast_if_else: {
    auto if_else = source->nodes->data(node_index).if_else;

    TypeIndex cond_type;
    Try(check_expression(env, if_else.cond, nullptr, &cond_type));
    Try(check_coercion(types->type.bool_, cond_type, if_else.cond));

    TypeIndex then_type;
    Try(check_expression(env, if_else.then, nullptr, &then_type));

    if (if_else.otherwise.is_none()) {
      result = types->type.nil;
      break;
    }

    TypeIndex otherwise_type;
    Try(check_expression(env, if_else.otherwise, nullptr, &otherwise_type));

    TypeIndex final_type;
    if (types->is_coercible_to(then_type, otherwise_type)) {
      final_type = otherwise_type;
    } else if (types->is_coercible_to(otherwise_type, then_type)) {
      final_type = then_type;
    } else {
      Todo();
    }

    result = final_type;
  } break;
  case Ast_declaration: {
    auto decl = source->nodes->data(node_index).declaration;
    auto key  = intern_identifier(decl.name);

    TypeIndex declared_type;
    Try(eval_type_expression(env, decl.type, &declared_type));

    TypeHint hint = {
      .type     = declared_type,
      .location = decl.type,
    };

    TypeIndex value_type;
    Try(check_expression(env, decl.value, &hint, &value_type));

    Try(check_coercion(declared_type, value_type, decl.value));

    // TODO: You probably want a unification of the declared type and the actual type here.

    Declaration d;
    if (declared_type == types->type.type) {
      d = {
        .kind = Declaration_of_type,
        .type = value_type,
      };
    } else {
      d = {
        .kind = Declaration_of_value,
        .type = declared_type,
      };
    }

    env->insert(key, d);

    result = types->type.nil;
  } break;
  case Ast_literal_sequence: {
    auto seq = source->nodes->data(node_index).literal_sequence;

    auto type = alloc_type_sequence(work_arena, seq.items.len());
    *type     = {
      .kind     = Type_sequence,
      .sequence = {.count = cast<u32>(seq.items.len()), .item_types = {}},
    };

    for (u32 i = 0; i < seq.items.len(); i++) {
      Try(check_expression(env, seq.items[i], nullptr, &type->sequence.item_types[i]));
    }

    result = types->add(type);
  } break;

  case Ast_index: {
    auto node = source->nodes->data(node_index).index;

    TypeIndex type_indexable;
    Try(check_expression(env, node.indexable, nullptr, &type_indexable));
    Try(check_is_indexable(type_indexable, node.indexable));

    TypeIndex type_index_at;
    Try(check_expression(env, node.index_at, nullptr, &type_index_at));
    Try(check_coercion(types->type.uint, type_index_at, node.index_at));

    auto ty = types->get(type_indexable);

    Assert(ty->kind != Type_sequence); // For now, don't support this case.

    TypeIndex base_type;
    if (ty->kind == Type_array) {
      base_type = ty->array.base_type;
    } else if (ty->kind == Type_slice) {
      base_type = ty->slice.base_type;
    } else {
      Unreachable();
    }

    result = base_type;
  } break;

  case Ast_binary_op: {
    auto node = source->nodes->data(node_index).binary_op;

    TypeIndex type_lhs;
    Try(check_expression(env, node.lhs, nullptr, &type_lhs));

    TypeIndex type_rhs;
    Try(check_expression(env, node.rhs, nullptr, &type_rhs));

    switch (node.kind) {
    case Logical_and:
    case Logical_or:
      Try(check_is_of_type(node.lhs, type_lhs, types->type.bool_));
      Try(check_is_of_type(node.rhs, type_lhs, types->type.bool_));
      result = types->type.bool_;
      break;
    case Cmp_equal:
    case Cmp_not_equal:
    case Cmp_less_than:
    case Cmp_less_equal:
    case Cmp_greater_than:
    case Cmp_greater_equal: {
      TypeIndex unified;
      Try(check_unification(node.lhs, type_lhs, node.rhs, type_rhs, &unified));
      result = types->type.bool_;
    } break;
    case Mul:
    case Div:
    case Mod:
    case Sub:
    case Add:
    case Bit_shift_left:
    case Bit_shift_right:
    case Bit_and:
    case Bit_or:
    case Bit_xor:
      Try(check_unification(node.lhs, type_lhs, node.rhs, type_rhs, &result));
      break;
    case BinaryOpKind_max:
      Unreachable();
      return false;
    }
  } break;

  case Ast_builtin: {
    auto builtin = source->nodes->data(node_index).builtin;

    switch (builtin.kind) {
    case Builtin_print: {
      if (builtin.args.len() == 0) {
        messages->error(node_index, "#print needs at least one argument.");
        return false;
      }

      TypeIndex arg_type;
      Try(check_expression(env, builtin.args[0], nullptr, &arg_type));
      Try(check_coercion(types->type.slice_u8, arg_type, builtin.args[0]));

      result = types->type.nil;
    } break;
    }
  } break;

  case Ast_defer: {
    auto      defer_ = source->nodes->data(node_index).defer;
    TypeIndex deferred_type;
    Try(check_expression(env, defer_.value, nullptr, &deferred_type));
    result = types->type.nil;
  } break;

  case Ast_const: {
    auto node = source->nodes->data(node_index);
    Try(check_expression(env, node.const_.expr, nullptr, &result));
  } break;

  case Ast_call: {
    auto node = source->nodes->data(node_index).call;

    TypeIndex callee_type;
    Try(check_expression(env, node.callee, nullptr, &callee_type));

    auto ty = types->get(callee_type);

    if (ty->kind == Type_literal_function) {
      // This simplifies some of the type checking, but will likely change in the future.
      messages->error(node.callee, "Function literals may not be called directly");
      return false;
    }

    if (ty->kind != Type_function) {
      messages->error(node.callee, "Cannot call value of type {type}.", callee_type);
      return false;
    }

    u32 expected_param_count =
      ty->kind == Type_function ? ty->function.param_count : ty->literal_function.param_count;

    if (node.args.len() != expected_param_count) {
      messages
        ->error(node_index, "Wrong number of arguments to function of type {type}.", callee_type);
      return false;
    }

    for (u32 i = 0; i < node.args.len(); i++) {
      TypeHint type_hint_arg = {
        .type     = ty->function.param_types[i],
        .location = node.callee, // TODO improve this location
      };

      TypeIndex arg_type;
      Try(check_expression(env, node.args[i], &type_hint_arg, &arg_type));

      if (ty->kind == Type_function) {
        Try(check_coercion(ty->function.param_types[i], arg_type, node.args[i]));
      }
    }

    result =
      ty->kind == Type_function ? ty->function.return_type : ty->literal_function.return_type;
  } break;

  case Ast_for: {
    auto node = source->nodes->data(node_index).for_;

    TypeIndex iterable_type;
    Try(check_expression(env, node.iterable, nullptr, &iterable_type));
    Try(check_is_indexable(iterable_type, node.iterable));

    auto ty = types->get(iterable_type);

    TypeIndex element_type;
    if (ty->kind == Type_array) {
      element_type = ty->array.base_type;
    } else if (ty->kind == Type_slice) {
      element_type = ty->slice.base_type;
    } else {
      Assert(ty->kind == Type_sequence);
      Todo();
    }

    auto env_loop = envs->alloc(env);
    defer(envs->dealloc(env_loop));

    auto iterator_token = source->nodes->data(node.iterator).identifier.token_index;
    auto key            = intern_identifier(iterator_token);
    env_loop->insert(key, {.kind = Declaration_of_value, .type = element_type});
    annotations[node.iterator.idx] = element_type;

    TypeIndex body_type;
    Try(check_expression(env_loop, node.body, nullptr, &body_type));

    result = types->type.nil;
  } break;

  case Ast_assign: {
    auto node = source->nodes->data(node_index).assign;

    Assert(node.kind == Assign_normal);

    TypeIndex lhs_type;
    Try(check_expression(env, node.lhs, nullptr, &lhs_type));
    Try(check_is_assignable(node.lhs));

    TypeIndex value_type;
    Try(check_expression(env, node.value, nullptr, &value_type));

    Try(check_coercion(lhs_type, value_type, node.value));

    result = types->type.nil;
  } break;
  case Ast_unary_op:
  case Ast_break:
  case Ast_continue:
  case Ast_return:
  case Ast_root:
  case Ast_kind_max:
    Todo();
    return false;
  }

  annotations[node_index.idx] = result;
  *out                        = result;

  return true;
}

b32 TypeChecker::check_coercion(TypeIndex type_dst, TypeIndex type_src, NodeIndex node_index_src) {
  if (types->is_coercible_to(type_src, type_dst)) {
    return true;
  }

  messages->error(node_index_src, "Cannot coerce type {type} to {type}.", type_src, type_dst);

  return false;
}

b32 TypeChecker::check_unification(
  NodeIndex lhs, TypeIndex type_lhs, NodeIndex rhs, TypeIndex type_rhs, TypeIndex *result
) {
  if (types->unify(type_lhs, type_rhs, result)) {
    return true;
  }

  messages->error(lhs, "Cannot unify types {type} and {type}.", type_lhs, type_rhs);

  return false;
}

b32 TypeChecker::check_is_of_type(NodeIndex at, TypeIndex type, TypeIndex expected_type) {
  if (type == expected_type) {
    return true;
  }

  messages->error(at, "Expected type, but got {type}.", type);

  return false;
}

b32 TypeChecker::check_is_declaration_of_type(Declaration decl, NodeIndex at) {
  if (decl.kind == Declaration_of_type) {
    return true;
  }

  messages->error(at, "Expected type, but got something else.");

  return false;
}

b32 TypeChecker::check_is_indexable(TypeIndex type, NodeIndex at) {
  auto ty = types->get(type);

  if (ty->kind == Type_slice || ty->kind == Type_array || ty->kind == Type_sequence) {
    return true;
  }

  messages->error(at, "Type is not indexable. Got type {type}.", type);

  return false;
}

b32 TypeChecker::check_is_declaration_resolved(Declaration decl, NodeIndex at) {
  if (decl.kind != Declaration_resolving) {
    return true;
  }

  messages->error(at, "Circular declaration detected");
  return false;
}

b32 TypeChecker::check_is_assignable(NodeIndex node_index) {
  auto kind = source->nodes->kind(node_index);

  if (kind == Ast_identifier) {
    return true;
  }

  if (kind == Ast_index) {
    return check_is_assignable(source->nodes->data(node_index).index.indexable);
  }

  messages->error(node_index, "Cannot assign");

  return false;
}

StrKey TypeChecker::intern_identifier(TokenIndex identifier) {
  auto s = get_token_str(source->text, source->tokens, identifier);
  return strings->add(s);
}

b32 TypeChecker::find_identifier(
  Env<Declaration> *env, TokenIndex identifier, Declaration *result
) {
  auto key = intern_identifier(identifier);

  auto found = env->lookup(key, result);

  if (!found) {
    messages->error(identifier, "Could not find identifier '{strkey}'.", key);
  }

  return found;
}
