#include "blu.hh"
#include <stdio.h>

char const *preamble = "#include <stdint.h>\n"
                       "typedef int8_t i8;\n"
                       "typedef uint8_t u8;\n"
                       "typedef int16_t i16;\n"
                       "typedef uint16_t u16;\n"
                       "typedef int32_t i32;\n"
                       "typedef uint32_t u32;\n"
                       "typedef int64_t i64;\n"
                       "typedef uint64_t u64;\n"
                       "typedef float f32;\n"
                       "typedef double f64;\n\n";

struct CGenerator {
  FILE *out;

  void output_span(AstRef n);
  void output_type(AstRef n);
  void output_identifier(AstRef n);
  void output_function_signature(AstRef n);
  void output_function_declaration(AstRef n);
  void output_function_definition(AstRef n);
  void output_break(AstRef n);
  void output_continue(AstRef n);
  void output_while(AstRef n);
  void output_statement(AstRef n);
  void output_expression(AstRef n);
  void output_literal_int(AstRef n);
  void output_call(AstRef n);
  void output_binary_op(AstRef n);
  void output_unary_op(AstRef n);
  void output_if_else(AstRef n);
  void output_scope(AstRef n);
  void output_module(AstRef n);
};

void CGenerator::output_span(AstRef n) {
  Str s = n->span.str();
  fprintf(out, "%.*s", cast<i32>(s.len), s.str);
}

void CGenerator::output_type(AstRef n) { output_span(n); }

void CGenerator::output_identifier(AstRef n) { output_span(n); }

void CGenerator::output_function_signature(AstRef n) {
  output_type(n->function.return_type);
  fprintf(out, " ");
  output_identifier(n->function.name);
  fprintf(out, "()");
}

void CGenerator::output_function_declaration(AstRef n) {
  output_function_signature(n);
  fprintf(out, ";\n");
}

void CGenerator::output_literal_int(AstRef n) { output_span(n); }

void CGenerator::output_break(AstRef n) { fprintf(out, "break"); }
void CGenerator::output_continue(AstRef n) { fprintf(out, "continue"); }

void CGenerator::output_scope(AstRef n) {
  fprintf(out, "{\n");
  ForEachIndex(i, n->scope.statements.len) { output_statement(n->scope.statements[i]); }
  fprintf(out, "}\n");
}

void CGenerator::output_if_else(AstRef n) {
  fprintf(out, "if (");
  output_expression(n->if_else.cond);
  fprintf(out, ")\n");
  output_scope(n->if_else.then);

  if (n->if_else.otherwise != nil) {
    fprintf(out, "else\n");
    output_scope(n->if_else.otherwise);
  }
}

void CGenerator::output_expression(AstRef n) {
  switch (n->kind) {
  case Ast_literal_int:
    output_literal_int(n);
    return;
  case Ast_identifier:
    output_identifier(n);
    return;
  case Ast_call:
    output_call(n);
    return;
  case Ast_binary_op:
    output_binary_op(n);
    return;
  case Ast_unary_op:
    output_unary_op(n);
    return;
  case Ast_if_else:
    output_if_else(n);
    return;
  default:
    fprintf(out, "#error \"Encountered unknown expression!\"");
    return;
  }
}

void CGenerator::output_call(AstRef n) {
  output_expression(n->call.f);
  fprintf(out, "(");

  if (n->call.arguments.len > 0) {
    output_expression(n->call.arguments[0]);
  }

  for (usize i = 1; i < n->call.arguments.len; i += 1) {
    fprintf(out, ", ");
    output_expression(n->call.arguments[i]);
  }

  fprintf(out, ")");
}

void CGenerator::output_binary_op(AstRef n) {
  fprintf(out, "(");
  output_expression(n->binary_op.lhs);

  // clang-format off
  switch (n->binary_op.kind) {
  case Sub:   fprintf(out, " - "); break;
  case Add:   fprintf(out, " + "); break;
  case Mul:   fprintf(out, " * "); break;
  case Div:   fprintf(out, " / "); break;
  case Mod:   fprintf(out, " %% "); break;
  case Cmp_equal: fprintf(out, " == "); break;
  case Cmp_not_equal: fprintf(out, " != "); break;
  case Cmp_greater_than: fprintf(out, " > "); break;
  case Cmp_greater_equal: fprintf(out, " >= "); break;
  case Cmp_less_than: fprintf(out, " < "); break;
  case Cmp_less_equal: fprintf(out, " <= "); break;
  case Logical_and: fprintf(out, " && "); break;
  case Logical_or: fprintf(out, " || "); break;
  case Bit_and: fprintf(out, " & "); break;
  case Bit_or: fprintf(out, " | "); break;
  case Bit_xor: fprintf(out, " ^ "); break;
  case Bit_shift_left: fprintf(out, " << "); break;
  case Bit_shift_right: fprintf(out, " >> "); break;
  default:
    fprintf(out, " ??? ");
    break;
  }
  // clang-format on

  output_expression(n->binary_op.rhs);
  fprintf(out, ")");
}

void CGenerator::output_unary_op(AstRef n) {}

void CGenerator::output_statement(AstRef n) {
  if (n->kind == Ast_break) {
    output_break(n);
    fprintf(out, ";\n");
    return;
  }

  if (n->kind == Ast_continue) {
    output_continue(n);
    fprintf(out, ";\n");
    return;
  };

  if (n->kind == Ast_while) {
    fprintf(out, "while ");
    output_expression(n->_while.cond);
    output_scope(n->_while.body);
    return;
  }

  if (n->kind == Ast_return) {
    fprintf(out, "return ");
    output_expression(n->ret.value);
    fprintf(out, ";\n");
    return;
  }

  if (n->kind == Ast_declaration) {
    output_type(n->declaration.type);
    fprintf(out, " ");
    output_identifier(n->declaration.name);
    fprintf(out, " = ");
    output_expression(n->declaration.initial_value);
    fprintf(out, ";\n");
    return;
  }

  if (n->kind == Ast_assign) {
    output_expression(n->assign.lhs);
    fprintf(out, " = ");
    output_expression(n->assign.value);
    fprintf(out, ";\n");
    return;
  }

  output_expression(n);
}

void CGenerator::output_function_definition(AstRef n) {
  output_function_signature(n);
  output_scope(n->function.body);
}

void CGenerator::output_module(AstRef n) {
  ForEachIndex(i, n->module.items.len) { output_function_declaration(n->module.items[i]); }

  ForEachIndex(i, n->module.items.len) { output_function_definition(n->module.items[i]); }
}

b32 generate_c_code(FILE *out, AstRef mod) {
  fprintf(out, "%s", preamble);

  CGenerator gen{out};
  gen.output_module(mod);

  return true;
}
