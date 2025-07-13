#include "blu.hh"
#include <stdio.h>
#include <stdarg.h>

char const *preamble = "#include <stdint.h>\n"
                       "#define true 1\n"
                       "#define false 0\n"
                       "typedef uint32_t b32;\n"
                       "typedef int8_t i8;\n"
                       "typedef uint8_t u8;\n"
                       "typedef int16_t i16;\n"
                       "typedef uint16_t u16;\n"
                       "typedef int32_t i32;\n"
                       "typedef uint32_t u32;\n"
                       "typedef int64_t i64;\n"
                       "typedef uint64_t u64;\n"
                       "typedef float f32;\n"
                       "typedef double f64;\n\n"
                       "typedef struct slice { void *data; i64 len; } slice;\n"
                       "\n";

struct CFileWriter {
  FILE *out;

  void vprintf(char const *format, va_list arg) { vfprintf(out, format, arg); }
};

template<typename Writer> struct CGenerator {
  CompilerContext *ctx;
  Writer writer;

  u32 sym_id = 0;

  void print(char const *format, ...) {
    va_list vl;
    va_start(vl, format);
    writer.vprintf(format, vl);
    va_end(vl);
  }

  Str gensym() {
    u32 nums = 4;
    char *s  = ctx->arena.alloc<char>(nums + 2);
    snprintf(s, nums + 2, "a%04d", sym_id);
    sym_id += 1;
    return {s, nums + 1};
  }

  void output_str(Str s);

  void output_function_signature(Str name, AstNode *function_value);
  void output_function_declaration(Str name, AstNode *function_value);
  void output_function_definition(Str name, AstNode *function_value);

  void output_variable_declaration(Str name, Type *type, AstNode *value = nullptr);

  void output_type(Type *t);

  void output_param(AstNode *n);

  void output_expression(AstNode *n, b32 is_terminal = false, Str dst = {nullptr, 0});

  void output_span(AstNode *n);
  void output_identifier(AstNode *n);
  void output_break(AstNode *n);
  void output_while(AstNode *n);
  void output_literal_int(AstNode *n);
  void output_call(AstNode *n);
  void output_binary_op(AstNode *n);
  void output_unary_op(AstNode *n);
  void output_if_else(AstNode *n, Str dst = {nullptr, 0});
  void output_scope(AstNode *n);
  void output_module(AstNode *n);
};

template<typename Writer> void CGenerator<Writer>::output_str(Str s) {
  print("%.*s", cast<i32>(s.len), s.str);
}

template<typename Writer> void CGenerator<Writer>::output_span(AstNode *n) {
  Str s = n->span.str();
  print("%.*s", cast<i32>(s.len), s.str);
}

template<typename Writer> void CGenerator<Writer>::output_type(Type *t) {
  switch (t->kind) {
  case Type_slice: {
    print("slice");
  } break;
  case Type_Void:
    print("void");
    break;
  case Type_Integer: {
    if (t->integer.signedness == Unsigned) {
      print("u");
    } else {
      print("i");
    }

    print("%d", t->integer.bitwidth);
  } break;
  default:
    Todo();
  }
}

template<typename Writer> void CGenerator<Writer>::output_identifier(AstNode *n) { output_span(n); }

template<typename Writer> void CGenerator<Writer>::output_function_signature(Str name, AstNode *n) {
  Debug_assert(n->kind == Ast_function);

  output_type(n->type->function.return_type);
  print(" ");
  output_str(name);
  print("(");

  AstNode *params = n->function.params;

  if (params) {
    output_param(params);

    ForEachAstNode(p, params->next) {
      print(", ");
      output_param(p);
    }
  }

  print(")");
}

template<typename Writer> void CGenerator<Writer>::output_param(AstNode *n) {
  Debug_assert(n->kind == Ast_param);
  Debug_assert(n->type);

  output_type(n->type);
  print(" ");
  output_identifier(n->param.name);
}

template<typename Writer>
void CGenerator<Writer>::output_function_declaration(Str name, AstNode *n) {
  output_function_signature(name, n);
  print(";\n");
}

template<typename Writer>
void CGenerator<Writer>::output_function_definition(Str name, AstNode *n) {
  output_function_signature(name, n);
  print("{\n");

  Str sym;
  b32 should_output_return = n->function.body->type->is_sized_type();
  if (should_output_return) {
    sym = gensym();
    output_variable_declaration(sym, n->type->function.return_type);
  }

  output_expression(n->function.body, true, sym);

  if (should_output_return) {
    print("return ");
    output_str(sym);
    print(";\n");
  }
  print("}\n");
}

template<typename Writer> void CGenerator<Writer>::output_literal_int(AstNode *n) {
  output_span(n);
}

template<typename Writer> void CGenerator<Writer>::output_break(AstNode *n) { print("break"); }

template<typename Writer> void CGenerator<Writer>::output_if_else(AstNode *n, Str dst) {
  Str sym = dst;
  if (!dst.is_ok() && n->type->is_sized_type()) {
    sym = gensym();
    output_variable_declaration(sym, n->type);
  }

  print("if (");
  output_expression(n->if_else.cond);
  print(") {\n");
  output_expression(n->if_else.then, true, sym);

  if (n->if_else.otherwise != nullptr) {
    print("} else {\n");
    output_expression(n->if_else.otherwise, true, sym);
  }

  print("}\n");
}

template<typename Writer>
void CGenerator<Writer>::output_variable_declaration(Str name, Type *type, AstNode *value) {
  output_type(type);
  print(" ");
  output_str(name);

  if (value) {
    print(" = ");
    output_expression(value);
  }

  print(";\n");
}

template<typename Writer> void CGenerator<Writer>::output_expression(AstNode *n, b32 is_terminal, Str dst) {
  switch (n->kind) {
  case Ast_scope: {
    Str sym = dst;
    if (!dst.is_ok() && n->type->is_sized_type()) {
      sym = gensym();
      output_variable_declaration(sym, n->type);
    }

    ForEachAstNode(e, n->scope.expressions) {
      if (e->next) {
        output_expression(e, true);
        continue;
      }

      output_expression(e, true, sym);
    }

  } break;
  case Ast_declaration: {
    Str name = n->declaration.name->span.str();
    output_variable_declaration(name, n->type, n->declaration.value);
  } break;
  case Ast_while: {
    print("while (");
    output_expression(n->_while.cond);
    print(") {\n");
    output_expression(n->_while.body);
    print("}\n");
  } break;
  case Ast_for: {
    Str i = gensym();
    Str iterable = gensym();

    output_variable_declaration(iterable, n->_for.iterable->type, n->_for.iterable);

    print("for (i64 ");
    output_str(i);
    print(" = 0; ");
    output_str(i);
    print(" < ");
    output_str(iterable);
    print(".len; ");
    output_str(i);
    print(" += 1) {\n");

    Str item = n->_for.item->span.str();
    output_type(n->_for.item->type);
    print(" ");
    output_str(item);
    print(" = ");
    print("((");
    output_type(n->_for.item->type);
    print("*)");
    output_str(iterable);
    print(".data)[");
    output_str(i);
    print("];\n");
    output_expression(n->_for.body);
    print("}\n");
  } break;
  case Ast_assign: {
    output_expression(n->assign.lhs);
    print(" = ");
    output_expression(n->assign.value);
    print(";\n");
  } break;
  case Ast_continue: {
    print("continue;\n");
  } break;
  case Ast_break: {
    print("break;\n");
  } break;
  case Ast_return: {
    print("return ");
    output_expression(n->_return.value);
    print(";\n");
  } break;
  case Ast_literal_int:
    if (dst.is_ok()) {
      output_str(dst);
      print(" = ");
      output_literal_int(n);
      print(";\n");
      return;
    }

    output_literal_int(n);
    return;
  case Ast_identifier:
    if (dst.is_ok()) {
      output_str(dst);
      print(" = ");
      output_identifier(n);
      print(";\n");
      return;
    }

    output_identifier(n);
    if (is_terminal) { print(";\n"); }
    return;
  case Ast_call:
    if (dst.is_ok()) {
      output_str(dst);
      print(" = ");
      output_call(n);
      print(";\n");
      return;
    }

    output_call(n);
    if (is_terminal) { print(";\n"); }
    return;
  case Ast_binary_op:
    if (dst.is_ok()) {
      output_str(dst);
      print(" = ");
      output_binary_op(n);
      print(";\n");
      return;
    }

    output_binary_op(n);
    if (is_terminal) { print(";\n"); }
    return;
  case Ast_unary_op:
    output_unary_op(n);
    if (is_terminal) { print(";\n"); }
    return;
  case Ast_if_else:
    output_if_else(n, dst);
    return;
  default:
    print("#error \"Encountered unknown expression!\"\n");
    return;
  }
}

template<typename Writer> void CGenerator<Writer>::output_call(AstNode *n) {
  output_expression(n->call.callee);
  print("(");

  if (n->call.args) {
    output_expression(n->call.args);
  }

  ForEachAstNode(arg, n->call.args->next) {
    print(", ");
    output_expression(arg);
  }

  print(")");
}

template<typename Writer> void CGenerator<Writer>::output_binary_op(AstNode *n) {
  print("(");
  output_expression(n->binary_op.lhs);

  // clang-format off
  switch (n->binary_op.kind) {
  case Sub:               print(" - ");  break;
  case Add:               print(" + ");  break;
  case Mul:               print(" * ");  break;
  case Div:               print(" / ");  break;
  case Mod:               print(" %% "); break;
  case Cmp_equal:         print(" == "); break;
  case Cmp_not_equal:     print(" != "); break;
  case Cmp_greater_than:  print(" > ");  break;
  case Cmp_greater_equal: print(" >= "); break;
  case Cmp_less_than:     print(" < ");  break;
  case Cmp_less_equal:    print(" <= "); break;
  case Logical_and:       print(" && "); break;
  case Logical_or:        print(" || "); break;
  case Bit_and:           print(" & ");  break;
  case Bit_or:            print(" | ");  break;
  case Bit_xor:           print(" ^ ");  break;
  case Bit_shift_left:    print(" << "); break;
  case Bit_shift_right:   print(" >> "); break;
  case AddAssign:         print(" += "); break;
  default:
    print(" ??? ");
    break;
  }
  // clang-format on

  output_expression(n->binary_op.rhs);
  print(")");
}

template<typename Writer> void CGenerator<Writer>::output_unary_op(AstNode *n) {}

template<typename Writer> void CGenerator<Writer>::output_module(AstNode *n) {
  ForEachAstNode(item, n->module.items) {
    Debug_assert(item->kind == Ast_declaration);

    AstNode *name = item->declaration.name;

    output_function_declaration(name->span.str(), item->declaration.value);
  }

  ForEachAstNode(item, n->module.items) {
    Debug_assert(item->kind == Ast_declaration);

    AstNode *name = item->declaration.name;

    output_function_definition(name->span.str(), item->declaration.value);
  }
}

b32 generate_c_code(CompilerContext *ctx, FILE *out, AstNode *mod) {
  CFileWriter writer;
  writer.out = out;

  CGenerator<CFileWriter> gen{ctx, writer};
  gen.print(preamble);
  gen.output_module(mod);

  return true;
}
