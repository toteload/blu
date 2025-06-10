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
  Slice<AstNode> nodes;

  void output_span(AstRef ref);
  void output_type(AstRef ref);
  void output_identifier(AstRef ref);
  void output_function_signature(AstRef ref);
  void output_function_declaration(AstRef ref);
  void output_function_definition(AstRef ref);
  void output_break(AstRef ref);
  void output_continue(AstRef ref);
  void output_while(AstRef ref);
  void output_statement(AstRef ref);
  void output_expression(AstRef ref);
  void output_literal_int(AstRef ref);
  void output_call(AstRef ref);
  void output_binary_op(AstRef ref);
  void output_unary_op(AstRef ref);
  void output_if_else(AstRef ref);
  void output_scope(AstRef ref);
  void output_module(AstRef ref);
};

void CGenerator::output_span(AstRef ref) {
  Str s = nodes[ref].span.str();
  fprintf(out, "%.*s", cast<i32>(s.len), s.str);
}

void CGenerator::output_type(AstRef ref) { output_span(ref); }

void CGenerator::output_identifier(AstRef ref) { output_span(ref); }

void CGenerator::output_function_signature(AstRef ref) {
  output_type(nodes[ref].function.return_type);
  fprintf(out, " ");
  output_identifier(nodes[ref].function.name);
  fprintf(out, "()");
}

void CGenerator::output_function_declaration(AstRef ref) {
  output_function_signature(ref);
  fprintf(out, ";\n");
}

void CGenerator::output_literal_int(AstRef ref) { output_span(ref); }

void CGenerator::output_break(AstRef ref) { fprintf(out, "break"); }
void CGenerator::output_continue(AstRef ref) { fprintf(out, "continue"); }

void CGenerator::output_scope(AstRef ref) {
  fprintf(out, "{\n");
  ForEachIndex(i, nodes[ref].scope.statements.len) {
    output_statement(nodes[ref].scope.statements[i]);
  }
  fprintf(out, "}\n");
}

void CGenerator::output_if_else(AstRef ref) {
  fprintf(out, "if (");
  output_expression(nodes[ref].if_else.cond);
  fprintf(out, ")\n");
  output_scope(nodes[ref].if_else.then);

  if (nodes[ref].if_else.otherwise != nil) {
    fprintf(out, "else\n");
    output_scope(nodes[ref].if_else.otherwise);
  }
}

void CGenerator::output_expression(AstRef ref) {
  switch (nodes[ref].kind) {
  case Ast_literal_int:
    output_literal_int(ref);
    return;
  case Ast_identifier:
    output_identifier(ref);
    return;
  case Ast_call:
    output_call(ref);
    return;
  case Ast_binary_op:
    output_binary_op(ref);
    return;
  case Ast_unary_op:
    output_unary_op(ref);
    return;
  case Ast_if_else:
    output_if_else(ref);
    return;
  default:
    fprintf(out, "#error \"Encountered unknown expression!\"");
    return;
  }
}

void CGenerator::output_call(AstRef ref) {
  output_expression(nodes[ref].call.f);
  fprintf(out, "(");

  if (nodes[ref].call.arguments.len > 0) {
    output_expression(nodes[ref].call.arguments[0]);
  }

  for (usize i = 1; i < nodes[ref].call.arguments.len; i += 1) {
    fprintf(out, ", ");
    output_expression(nodes[ref].call.arguments[i]);
  }

  fprintf(out, ")");
}

void CGenerator::output_binary_op(AstRef ref) {
  fprintf(out, "(");
  output_expression(nodes[ref].binary_op.lhs);

  switch (nodes[ref].binary_op.kind) {
  case Sub:
    fprintf(out, " - ");
    break;
  case Add:
    fprintf(out, " + ");
    break;
  case LessEqual:
    fprintf(out, " <= ");
    break;
  default:
    fprintf(out, " ??? ");
    break;
  }

  output_expression(nodes[ref].binary_op.rhs);
  fprintf(out, ")");
}

void CGenerator::output_unary_op(AstRef ref) {}

void CGenerator::output_statement(AstRef ref) {
  AstNode n = nodes[ref];

  if (n.kind == Ast_break) {
    output_break(ref);
    fprintf(out, ";\n");
    return;
  }

  if (n.kind == Ast_continue) {
    output_continue(ref);
    fprintf(out, ";\n");
    return;
  };

  if (n.kind == Ast_while) {
    fprintf(out, "while ");
    output_expression(n._while.cond);
    output_scope(n._while.body);
    return;
  }

  if (n.kind == Ast_return) {
    fprintf(out, "return ");
    output_expression(n.ret.value);
    fprintf(out, ";\n");
    return;
  }

  if (n.kind == Ast_declaration) {
    output_type(n.declaration.type);
    fprintf(out, " ");
    output_identifier(n.declaration.name);
    fprintf(out, " = ");
    output_expression(n.declaration.initial_value);
    fprintf(out, ";\n");
    return;
  }

  if (n.kind == Ast_assign) {
    output_expression(n.assign.lhs);
    fprintf(out, " = ");
    output_expression(n.assign.value);
    fprintf(out, ";\n");
    return;
  }

  output_expression(ref);
}

void CGenerator::output_function_definition(AstRef ref) {
  output_function_signature(ref);
  output_scope(nodes[ref].function.body);
}

void CGenerator::output_module(AstRef ref) {
  AstNode n = nodes[ref];
  ForEachIndex(i, n.module.items.len) { output_function_declaration(n.module.items[i]); }

  ForEachIndex(i, n.module.items.len) { output_function_definition(n.module.items[i]); }
}

b32 generate_c_code(FILE *out, Slice<AstNode> nodes, AstRef mod) {
  fprintf(out, "%s", preamble);

  CGenerator gen{out, nodes};

  gen.output_module(mod);

  return true;
}
