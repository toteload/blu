#include "blu.hh"

struct TypeChecker {
  TypeInterner *types;
  
  void check_declaration(HirIndex idx, Env *env);
  bool check_and_get_type_of_expr(HirIndex idx, Env *env, TypeIndex *out);

  bool check_and_get_return_type(HirIndex idx, Env *env, TypeIndex *out);

  bool read_type(HirIndex, Env *env, TypeIndex *out);
};

bool TypeChecker::check_declaration(HirIndex idx, Env *env) {
  auto inst = hir->get(idx);

  TypeIndex declared_type;
  Try(read_type({idx+1}, env, &declared_type));

  TypeIndex value_type;
  Try(check_and_get_type_of_expr(inst.data->declaration.value, env, &value_type));

  if (!is_coercible_to(value_type, declared_type)) {
    Todo(); // communicate error
    return false;
  }

  return true;
}

bool TypeChecker::check_and_get_type_of_expr(HirIndex idx, Env *env, TypeIndex *out) {
  auto inst = hir->get(idx);

  switch (inst.kind) {
  case Hir_literal_int: { *out = types->literal_int; } break;
  case Hir_true:
  case Hir_false: { *out = types->bool_; } break;
  case Hir_nil: { *out = types->nil; } break;
  case Hir_return: { *out = types->never; } break;
  case Hir_function: { Try(check_and_get_return_type(idx, env, out)); } break;
  default: Todo(); break;
  }

  return true;
}

bool TypeChecker::check_and_get_return_type(HirIndex idx, Env *env, TypeIndex *out) {
  
}

void check_types(HirCode *hir) {
}
