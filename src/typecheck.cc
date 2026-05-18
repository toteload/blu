#include "blu.hh"

struct TypeHint {
  TypeIndex type;
  NodeIndex location;
};

struct CoercionLocation {
  TypeIndex type_dst;

  // Storing a pointer to a NodeIndex is unsafe.
  // The AstNodeData is stored in a vector which may relocate memory.
  // When we start adding cast AstNodes for the coercions the vector may relocate memory and invalidate our pointers.
  NodeIndex *location;
};

struct TypeChecker {
  Messages                *messages;
  TypeInterner            *types;
  EnvManager<Declaration> *envs;
  StringInterner          *strings;
  Arena                   *work_arena;
  ValueStore              *values;
  ParsedSource            *source;

  Vector<CoercionLocation> coercions;

  bool is_const_in_expression;

  b32 typecheck(NodeIndex idx_root);

  TypeIndex get_type(NodeIndex idx) { return source->nodes->type(idx); }

  b32 resolve_declaration(Env<Declaration> *env, NodeIndex decl_node);
  b32 resolve_type_expression(Env<Declaration> *env, NodeIndex node_index);
  b32 resolve_expression(Env<Declaration> *env, NodeIndex node_index, TypeHint *hint);
  b32 resolve_coercion(Env<Declaration> *env, TypeIndex type_dst, NodeIndex *node_to_coerce);

  b32 check_coercion(TypeIndex type_dst, TypeIndex type_src, NodeIndex node_index_src);
  b32 check_unification(
    NodeIndex lhs, TypeIndex type_lhs, NodeIndex rhs, TypeIndex type_rhs, TypeIndex *result
  );

  b32 check_is_valid_cast(NodeIndex at, TypeIndex type_dst, TypeIndex type_expr);
  b32 check_is_of_type(NodeIndex at, TypeIndex type, TypeIndex expected_type);
  b32 check_is_declaration_resolved(Declaration decl, NodeIndex at);
  b32 check_is_declaration_of_type(Declaration decl, NodeIndex at);
  b32 check_is_indexable(TypeIndex type, NodeIndex at);
  b32 check_is_assignable(NodeIndex node_index);

  StrKey intern_identifier(TokenIndex identifier);

  b32 find_declaration_of_identifier(
    Env<Declaration> *env, TokenIndex identifier, Declaration *result
  );
};

struct RootEnvPopulator {
  Env<Declaration> *env;
  StringInterner   *strings;
  TypeInterner     *types;

  void insert(DeclarationKind kind, Str s, TypeIndex type) {
    auto key = strings->add(s);
    env->insert(
      key,
      {
        .kind           = kind,
        .resolve_status = ResolveStatus_type_resolved,
        .is_const       = true,
        .data           = {
          .type = type,
        },
      }
    );
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

b32 typecheck(TypeCheckContext *context, ParsedSource *source, NodeIndex idx_root) {
  TypeChecker checker = {
    .messages   = context->messages,
    .types      = context->types,
    .envs       = context->envs,
    .strings    = context->strings,
    .work_arena = context->work_arena,
    .source     = source,
    .values = context->values,
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
    // TODO allow const declarations
    Assert(source->nodes->kind(root.items[i]) == Ast_declaration);

    auto decl = source->nodes->data(root.items[i]).declaration;

    auto key = intern_identifier(decl.name);

    env->insert(
      key,
      {
        .kind           = Declaration_of_unknown,
        .resolve_status = ResolveStatus_type_unresolved,
        .is_const       = false,
        .data           = {.node_index = root.items[i]},
      }
    );
  }

  for (u32 i = 0; i < root.items.len(); i++) {
    Try(resolve_declaration(env, root.items[i]));
  }

  return true;
}

b32 TypeChecker::resolve_declaration(Env<Declaration> *env, NodeIndex node_index) {
  auto &data = source->nodes->data(node_index);
  auto  key  = intern_identifier(data.declaration.name);

  Declaration *decl;
  auto        found = env->lookup_ptr(key, &decl);

  Assert(found);

  if (decl->resolve_status == ResolveStatus_type_resolved) {
    return true;
  }

  if (decl->resolve_status == ResolveStatus_type_resolving) {
    Todo();
    return false;
  }

  decl->resolve_status = ResolveStatus_type_resolving;

  Try(resolve_type_expression(env, data.declaration.type));

  auto type_declared = get_type(data.declaration.type);

  if (type_declared == types->type.type) {
    Try(resolve_type_expression(env, data.declaration.value));

    auto type = get_type(data.declaration.value);

    *decl =  {
        .kind           = Declaration_of_type,
        .resolve_status = ResolveStatus_type_resolved,
        .is_const       = true,
        .data           = {.type = type},
      };

    return true;
  }

    *decl = {
      .kind           = Declaration_of_value,
      .resolve_status = ResolveStatus_type_resolved,
      .is_const       = is_const_in_expression,
      .data           = {.type = type_declared},
    };

  TypeHint hint = {
    .type     = type_declared,
    .location = data.declaration.type,
  };

  Try(resolve_expression(env, data.declaration.value, &hint));

  TypeIndex type_value = get_type(data.declaration.value);
  Try(check_coercion(type_declared, type_value, data.declaration.value));

  source->nodes->type(node_index) = types->type.nil;

  return true;
}

b32 TypeChecker::resolve_type_expression(Env<Declaration> *env, NodeIndex node_index) {
  auto kind = source->nodes->kind(node_index);

  TypeIndex type_node;

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

    Try(resolve_type_expression(env, f.return_type));

    ty->function.return_type = get_type(f.return_type);

    for (u32 i = 0; i < param_count; i++) {
      Try(resolve_type_expression(env, f.param_types[i]));

      ty->function.param_types[i] = get_type(f.param_types[i]);
    }

    type_node = types->add(ty);
  } break;

  case Ast_type_slice: {
    auto slice = source->nodes->data(node_index).type_slice;

    Try(resolve_type_expression(env, slice.base));

    TypeIndex base_type = get_type(slice.base);

    Type type = {
      .kind  = Type_slice,
      .slice = {.base_type = base_type},
    };

    type_node = types->add(&type);
  } break;

  case Ast_type_array: {
    auto array = source->nodes->data(node_index).type_array;

    is_const_in_expression = true;
    Try(resolve_expression(env, array.size, nullptr));
    TypeIndex size_type    = get_type(array.size);
    is_const_in_expression = false;

    Try(resolve_coercion(env, types->type.uint, &array.size));

    Try(resolve_type_expression(env, array.base));
    TypeIndex base_type = get_type(array.base);

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

    type_node = types->add(&type);
  } break;

  case Ast_identifier: {
    auto token_index = source->nodes->data(node_index).identifier.token_index;

    Declaration decl;
    Try(find_declaration_of_identifier(env, token_index, &decl));

    if (decl.resolve_status == ResolveStatus_type_unresolved) {
      Try(resolve_declaration(env, decl.data.node_index));
      Try(find_declaration_of_identifier(env, token_index, &decl));
    }

    Try(check_is_declaration_resolved(decl, node_index));
    Try(check_is_declaration_of_type(decl, node_index));

    type_node = decl.data.type;
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
  case Ast_defer:
  case Ast_const:
  case Ast_cast:
  case Ast_kind_max:
  case Ast_root:
    Todo();
    return false;
  }

  source->nodes->type(node_index)     = type_node;
  source->nodes->is_const(node_index) = true;

  return true;
}

b32 TypeChecker::resolve_expression(Env<Declaration> *env, NodeIndex node_index, TypeHint *hint) {
  auto kind = source->nodes->kind(node_index);

  bool is_const_node = false;

  TypeIndex type_node{};

  switch (kind) {
  case Ast_cast: {
    auto node = source->nodes->data(node_index);

    Try(resolve_type_expression(env, node.cast.type_dst));
    TypeIndex type_dst = get_type(node.cast.type_dst);

    Try(resolve_expression(env, node.cast.value, nullptr));
    TypeIndex expr_type = get_type(node.cast.value);

    Try(check_is_valid_cast(node_index, type_dst, expr_type));

    type_node = type_dst;
  } break;

  case Ast_type_function: {
    auto f = source->nodes->data(node_index).type_function;

    is_const_in_expression = true;
    defer(is_const_in_expression = false);

    Try(resolve_expression(env, f.return_type, nullptr));

    TypeIndex return_type = get_type(f.return_type);
    Try(check_is_of_type(f.return_type, return_type, types->type.type));

    for (u32 i = 0; i < f.param_types.len(); i++) {
      Try(resolve_expression(env, f.param_types[i], nullptr));

      TypeIndex param_type = get_type(f.param_types[i]);
      Try(check_is_of_type(f.param_types[i], param_type, types->type.type));
    }

    type_node     = types->type.type;
    is_const_node = true;
  } break;

  case Ast_type_slice: {
    type_node     = types->type.type;
    is_const_node = true;
  } break;

  case Ast_type_array: {
    type_node     = types->type.type;
    is_const_node = true;
  } break;

  case Ast_literal_int: {
    type_node     = types->type.literal_int;
    is_const_node = true;
  } break;

  case Ast_literal_string: {
    auto token_index = source->nodes->data(node_index).literal_string.token_index;
    auto s           = get_token_str(source->text, source->tokens, token_index);
    auto size        = string_literal_byte_size(s);

    Type type = {
      .kind  = Type_array,
      .array = {
        .base_type = types->type.u8_,
        .size      = size,
      },
    };

    type_node     = types->add(&type);
    is_const_node = true;
  } break;

  case Ast_identifier: {
    auto token_index = source->nodes->data(node_index).identifier.token_index;

    Declaration decl;
    Try(find_declaration_of_identifier(env, token_index, &decl));

    if (decl.resolve_status == ResolveStatus_type_unresolved) {
      Try(resolve_declaration(env, decl.data.node_index));
      Try(find_declaration_of_identifier(env, token_index, &decl));
    }

    Try(check_is_declaration_resolved(decl, node_index));

    is_const_node = decl.is_const;

    switch (decl.kind) {
    case Declaration_of_value:
      type_node = decl.data.type;
      break;
    case Declaration_of_type:
      type_node = types->type.type;
      break;
    case Declaration_of_unknown:
      Unreachable();
      break;
    }
  } break;

  case Ast_function: {
    Assert(hint);

    Type *hint_type = types->get(hint->type);

    Assert(hint_type->kind == Type_function);

    auto f = source->nodes->data(node_index).function;

    u32 param_count = cast<u32>(f.param_names.len());
    if (hint_type->function.param_count != param_count) {
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

      env_params->insert(
        key,
        {.kind           = Declaration_of_value,
         .resolve_status = ResolveStatus_type_resolved,
         .data           = {.type = param_type}}
      );
      source->nodes->type(param_node) = param_type;
    }

    auto return_type = hint_type->function.return_type;

    TypeHint hint_body = {
      .type     = return_type,
      .location = hint->location,
    };

    Try(resolve_expression(env_params, f.body, &hint_body));
    TypeIndex body_type = get_type(f.body);

    Try(check_coercion(return_type, body_type, f.body));

    if (return_type != body_type) {
      source->nodes->type(f.body) = return_type;
    }

    type_node = hint->type;
  } break;

  case Ast_block: {
    auto block = source->nodes->data(node_index).block;
    if (block.items.len() == 0) {
      type_node = types->type.nil;
      break;
    }

    auto env_block = envs->alloc(env);
    defer(envs->dealloc(env_block));

    for (u32 i = 0; i < block.items.len() - 1; i++) {
      Try(resolve_expression(env_block, block.items[i], nullptr));
    }

    auto last_item = block.items[block.items.len() - 1];

    Try(resolve_expression(env_block, last_item, hint));

    TypeIndex block_type = get_type(last_item);
    if (hint && hint->type != block_type) {
      Try(resolve_coercion(env, hint->type, &block.items[block.items.len() - 1]));
      // Try(check_coercion(hint_last_item->type, block_type, last_item));
      type_node = hint->type;
    } else {
      type_node = block_type;
    }
  } break;

  case Ast_if_else: {
    auto if_else = source->nodes->data(node_index).if_else;

    Try(resolve_expression(env, if_else.cond, nullptr));

    TypeIndex cond_type = get_type(if_else.cond);

    Try(check_is_of_type(if_else.cond, cond_type, types->type.bool_));

    Try(resolve_expression(env, if_else.then, hint));

    if (if_else.otherwise.is_none()) {
      type_node = types->type.nil;
      break;
    }

    Try(resolve_expression(env, if_else.otherwise, hint));

    TypeIndex then_type      = get_type(if_else.then);
    TypeIndex otherwise_type = get_type(if_else.otherwise);

    Try(check_unification(if_else.then, then_type, if_else.otherwise, otherwise_type, &type_node));
  } break;

  case Ast_declaration: {
    auto decl = source->nodes->data(node_index).declaration;
    auto key  = intern_identifier(decl.name);

    Try(resolve_type_expression(env, decl.type));

    TypeIndex declared_type = get_type(decl.type);

    TypeHint hint = {
      .type     = declared_type,
      .location = decl.type,
    };

    Try(resolve_expression(env, decl.value, &hint));

    TypeIndex value_type = get_type(decl.value);

    Try(resolve_coercion(env, declared_type, &decl.value));

    Declaration d;
    if (declared_type == types->type.type) {
      d = {
        .kind     = Declaration_of_type,
        .resolve_status = ResolveStatus_type_resolved,
        .is_const = true,
        .data     = {.type = value_type},
      };
    } else {
      d = {
        .kind     = Declaration_of_value,
        .resolve_status = ResolveStatus_type_resolved,
        .is_const = false,
        .data     = {.type = declared_type},
      };
    }

    env->insert(key, d);

    type_node = types->type.nil;
  } break;

  case Ast_literal_sequence: {
    auto seq = source->nodes->data(node_index).literal_sequence;

    auto type = alloc_type_sequence(work_arena, seq.items.len());

    *type = {
      .kind     = Type_sequence,
      .sequence = {.count = cast<u32>(seq.items.len()), .item_types = {}},
    };

    for (u32 i = 0; i < seq.items.len(); i++) {
      Try(resolve_expression(env, seq.items[i], nullptr));
      type->sequence.item_types[i] = get_type(seq.items[i]);
    }

    type_node = types->add(type);
  } break;

  case Ast_index: {
    auto node = source->nodes->data(node_index).index;

    Try(resolve_expression(env, node.indexable, nullptr));

    TypeIndex type_indexable = get_type(node.indexable);
    Try(check_is_indexable(type_indexable, node.indexable));

    TypeHint hint_index_at = {
      .type     = types->type.uint,
      .location = node.index_at,
    };

    Try(resolve_expression(env, node.index_at, &hint_index_at));

    Try(resolve_coercion(env, types->type.uint, &node.index_at));

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

    type_node = base_type;
  } break;

  case Ast_binary_op: {
    auto node = source->nodes->data(node_index).binary_op;

    Try(resolve_expression(env, node.lhs, nullptr));
    Try(resolve_expression(env, node.rhs, nullptr));

    TypeIndex type_lhs = get_type(node.lhs);
    TypeIndex type_rhs = get_type(node.rhs);

    switch (node.kind) {
    case Logical_and:
    case Logical_or:
      Try(check_is_of_type(node.lhs, type_lhs, types->type.bool_));
      Try(check_is_of_type(node.rhs, type_lhs, types->type.bool_));
      type_node = types->type.bool_;
      break;
    case Cmp_equal:
    case Cmp_not_equal:
    case Cmp_less_than:
    case Cmp_less_equal:
    case Cmp_greater_than:
    case Cmp_greater_equal: {
      TypeIndex _unified;
      Try(check_unification(node.lhs, type_lhs, node.rhs, type_rhs, &_unified));
      type_node = types->type.bool_;
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
      Try(check_unification(node.lhs, type_lhs, node.rhs, type_rhs, &type_node));
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

      Try(resolve_expression(env, builtin.args[0], nullptr));

      TypeIndex arg_type = get_type(builtin.args[0]);
      Try(check_coercion(types->type.slice_u8, arg_type, builtin.args[0]));

      type_node = types->type.nil;
    } break;
    }
  } break;

  case Ast_defer: {
    auto defer_ = source->nodes->data(node_index).defer;
    Try(resolve_expression(env, defer_.value, nullptr));
    type_node = types->type.nil;
  } break;

  case Ast_const: {
    auto node = source->nodes->data(node_index);
    Try(resolve_expression(env, node.const_.expr, hint));
    type_node = get_type(node_index);
  } break;

  case Ast_call: {
    auto node = source->nodes->data(node_index).call;

    Try(resolve_expression(env, node.callee, nullptr));
    TypeIndex callee_type = get_type(node.callee);

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

      Try(resolve_expression(env, node.args[i], &type_hint_arg));

      TypeIndex arg_type = get_type(node.args[i]);

      if (ty->kind == Type_function) {
        Try(check_coercion(ty->function.param_types[i], arg_type, node.args[i]));
      }
    }

    type_node =
      ty->kind == Type_function ? ty->function.return_type : ty->literal_function.return_type;
  } break;

  case Ast_for: {
    auto node = source->nodes->data(node_index).for_;

    Try(resolve_expression(env, node.iterable, nullptr));

    TypeIndex iterable_type = get_type(node.iterable);
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
    env_loop->insert(
      key,
      {.kind           = Declaration_of_value,
       .resolve_status = ResolveStatus_type_resolved,
       .is_const       = false,
       .data           = {.type = element_type}}
    );
    source->nodes->type(node.iterator) = element_type;

    Try(resolve_expression(env_loop, node.body, nullptr));

    type_node = types->type.nil;
  } break;

  case Ast_assign: {
    auto node = source->nodes->data(node_index).assign;

    Assert(node.kind == Assign_normal);

    Try(resolve_expression(env, node.lhs, nullptr));

    TypeIndex lhs_type = get_type(node.lhs);

    TypeHint hint_value = {
      .type     = lhs_type,
      .location = node.lhs,
    };

    Try(resolve_expression(env, node.value, &hint_value));

    Try(check_is_assignable(node.lhs));

    TypeIndex value_type = get_type(node.value);
    Try(check_coercion(lhs_type, value_type, node.value));

    type_node = types->type.nil;
  } break;

  case Ast_unary_op:
  case Ast_root:
  case Ast_kind_max:
    Todo();
    return false;
  }

  Assert(type_node.idx != 0);

  bool is_const_result = is_const_node || is_const_in_expression;

  source->nodes->type(node_index)     = type_node;
  source->nodes->is_const(node_index) = is_const_result;

  return true;
}

b32 TypeChecker::resolve_coercion(
  Env<Declaration> *env, TypeIndex type_dst, NodeIndex *node_to_coerce
) {
  TypeIndex type_src = get_type(*node_to_coerce);

  Try(check_coercion(type_dst, type_src, *node_to_coerce));

  if (type_dst == type_src) {
    return true;
  }

  auto old_node_index = *node_to_coerce;
  auto node_index     = source->nodes->alloc();
  *node_to_coerce     = NodeIndex{NodeIndex_ast_node, {node_index.idx}};

  Value *val;
  auto   val_idx               = values->alloc_value(&val);
  auto   data              = values->alloc_data<TypeIndex>();
  *cast<TypeIndex *>(data) = type_dst;

  *val = {.type = types->type.type, .data = data};

  AstCast cast;
  cast.type_dst = NodeIndex{NodeIndex_value, {val_idx.idx}};
  cast.value    = old_node_index;

  source->nodes->set(
    node_index,
    {
      Ast_cast,
      {{0}, {0}}, // TODO replace this with something else. or maybe these nodes shouldn't live in
                  // `AstNodes`
      {.cast = cast},
    }
  );

  source->nodes->type(node_index) = type_dst;

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

b32 TypeChecker::check_is_valid_cast(NodeIndex at, TypeIndex type_dst, TypeIndex type_expr) {
  if (types->is_coercible_to(type_expr, type_dst)) {
    return true;
  }

  Type *t_dst = types->get(type_dst);
  Type *t_src = types->get(type_expr);

  if (
    t_dst->kind == Type_integer && (t_src->kind == Type_integer || t_src->kind == Type_literal_int)
  ) {
    return true;
  }

  Todo();

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
  if (decl.resolve_status == ResolveStatus_type_resolved) {
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

b32 TypeChecker::find_declaration_of_identifier(
  Env<Declaration> *env, TokenIndex identifier, Declaration *result
) {
  auto key = intern_identifier(identifier);

  auto found = env->lookup(key, result);

  if (!found) {
    messages->error(identifier, "Could not find identifier '{strkey}'.", key);
  }

  return found;
}
