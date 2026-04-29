#include "blu.hh"

struct Validator {
  MessageManager *messages;
  TypeInterner *types;
  Arena *tmp;

  b32 validate_symbol_use_root(HirCode *hir);
  b32 validate_symbol_use(HirCode *hir, HirIndex at);

  b32 annotate_types_root(HirCode *hir);
  b32 annotate_types(HirCode *hir, HirIndex at);

  b32 verify_type_use_root(HirCode *hir);
  b32 verify_type_use(HirCode *hir, HirIndex at);

  b32 validate(HirCode *hir);
};

template<typename F>
b32 for_each_declaration(HirCode *hir, F f) {
  HirIndex at = {0};

  while (true) {
    if (at.idx >= hir->len()) {
      break;
    }

    auto inst = hir->get(at);
    Debug_assert(*inst.kind == Hir_declaration);

    Try(f.on_declaration(hir, at));

    at = {at.idx + inst.data->declaration.instruction_count};
  }

  return true;
}

HirIndex declaration_type_idx(HirIndex at) {
  return {at.inner() + 1};
}

HirIndex declaration_value_idx(HirCode *hir, HirIndex at) {
  auto inst = hir->get(at);
  return {at.inner() + inst.data->function.param_count};
}

u32 type_function_param_idx(u32 i) {
  return i + 1;
}

struct ValidatorSymbolUse {
  HirCode *hir = nullptr;
  MessageManager *messages = nullptr;
  EnvManager *envs = nullptr;
  Env *root_env = nullptr;
  bool found_errors = false;

  void on_declaration(HirIndex at) {
    validate(at, root_env);
  }

  // You don't have to do full traversal of the tree!!!
  // The only instructions you have to check are the `Hir_declared_value` instructions.
  // You do have to keep track of what declared values are currently in scope or not.

  void validate(HirIndex at, Env *env) {
    auto inst = hir->get(at);

    switch (*inst.kind) {
      case Hir_declaration: {
        validate(declaration_type_idx(at), env);
        validate(declaration_value_idx(hir, at), env);
      } break;
      case Hir_type_function: {
        u32 count = inst.data->type_function.param_count;
        u32 start = type_function_param_idx(at.inner());
        for (u32 i = start; i < start + count; i++ ) {
          validate({i}, env);
        }
        validate(inst.data->type_function.return_type, env);
      } break;

      case Hir_function: {
        Env *params = envs->alloc(env);
        u32 param_count = inst.data->function.param_count;
        u32 param_start = function_param_idx(at.inner());
        u32 param_end = param_count + param_start;
        for (u32 i = param_start; i < param_end; i++) {
          // TODO add param to env
          Todo();
        }

        validate(function_body_idx(at), params);

        envs->dealloc(params);
      } break;
      case Hir_block: {
        u32 start = 0;
        u32 count = 0;
        u32 end = start + count;
        for (u32 i = start; i < end; i++) {
          // TODO
          Todo();
        }
      } break;

      case Hir_sub:
      case Hir_add:
      case Hir_mul:
      case Hir_div:
      case Hir_mod:
      case Hir_cmp_eq:
      case Hir_cmp_le:


      case Hir_literal_int:
      case Hir_type_nil:
      case Hir_type_int: break;
    }
  }
};

b32 Validator::validate_symbol_use_root(HirCode *hir) {
  ValidatorSymbolUse validator = {
    .messages = messages,
  };

  // First instruction in HIR should be a declaration.
  
  // 1. Add all top level declarations to the env.
  // 2. For each top level declaration:
  //   - For the type of the declaration -> find any use of declared values and verify.
  //   - For the value of the declaration -> find any declarations used.

  b32 ok = for_each_declaration(hir, validator);
}

b32 Validator::validate_symbol_use(HirCode *hir, HirIndex at) {
  auto inst = hir->get(at);

  switch (*inst.kind) {
  case Hir_declaration: {
    Try(validate_symbol_use(hir, {at.inner()+1}));
    Try(validate_symbol_use(hir, inst.data->declaration.value));
  } break;
  default: Todo();
  }

  return true;
}

struct TypeAnnotater {
  b32 on_declaration(HirCode *code, HirIndex idx) { return true; }
};

b32 Validator::annotate_types_root(HirCode *hir) {
  TypeAnnotater annotater;

  Try(for_each_declaration(hir, annotater));

  return true;
}

b32 Validator::annotate_types(HirCode *hir, HirIndex at) {
  auto inst = hir->get(at);

  switch (*inst.kind) {
  case Hir_declaration: {
    HirIndex type_idx = {at.inner()+1};
    Try(annotate_types(hir, type_idx));
    Try(annotate_types(hir, inst.data->declaration.value));
    *inst.type = hir->type(type_idx);
  } break;
  case Hir_type_int: {
    Type type_int = Type::make_integer(inst.data->type_int.signedness, inst.data->type_int.bitwidth);
    *inst.type = OptionalTypeIndex::from_index(types->add(&type_int));
  } break;
  case Hir_type_function: {
    auto snapshot = tmp->take_snapshot();
    Type *type_function = alloc_type_function(tmp, inst.data->type_function.param_count);

    // TODO

    tmp->restore(snapshot);
  } break;
  default: Todo();
  }

  return true;
}

b32 Validator::verify_type_use_root(HirCode *hir) {
  return true;
}

b32 Validator::validate(HirCode *hir) {
  HirIndex root = {0};
  Try(validate_symbol_use_root(hir));
  Try(annotate_types_root(hir));
  Try(verify_type_use_root(hir));

  return true;
}

b32 validate_hir(HirValidationContext *ctx, HirCode *hir) {
  Validator validator = {
    .messages = ctx->messages,
    .types = ctx->types,
    .tmp = ctx->tmp,
  };

  b32 ok = validator.validate(hir);

  return ok;
}
