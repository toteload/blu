#include "blu.hh"
#include <stdio.h>

char const *preamble =
"#include <stdio.h>\n"
"typedef int32_t i32;\n\n";

void output_span(FILE *out, Slice<AstNode> nodes, AstRef ref) {
  Str s = nodes[ref].span.str();
  fprintf(out, "%.*s", s.len, s.str);
}

void output_type(FILE *out, Slice<AstNode> nodes, AstRef ref) {
  output_span(out, nodes, ref);
}

void output_identifier(FILE *out, Slice<AstNode> nodes, AstRef ref) {
  output_span(out, nodes, ref);
}

void output_function_signature(FILE *out, Slice<AstNode> nodes, AstRef ref) {
  output_type(out, nodes, nodes[ref].function.return_type);
  fprintf(out, " ");
  output_identifier(out, nodes, nodes[ref].function.name);
  fprintf(out, "()");
}

void output_function_declaration(FILE *out, Slice<AstNode> nodes, AstRef ref) {
  output_function_signature(out, nodes, ref);
  fprintf(out, ";\n");
}

void output_literal_int(FILE *out, Slice<AstNode> nodes, AstRef ref) {
  output_span(out, nodes, ref);
}

void output_expression(FILE *out, Slice<AstNode> nodes, AstRef ref) {
  if (nodes[ref].kind == Ast_literal_int) {
    output_literal_int(out, nodes, ref);
    return;
  }

  if (nodes[ref].kind == Ast_identifier) {
    output_identifier(out, nodes, ref);
    return;
  }

  fprintf(out, "encountered unknown expression!");
}

void output_statement(FILE *out, Slice<AstNode> nodes, AstRef ref) {
  AstNode n = nodes[ref];

  if (n.kind == Ast_return) {
    fprintf(out, "return ");
    output_expression(out, nodes, n.ret.value);
    fprintf(out, ";\n");
    return;
  }

  if (n.kind == Ast_declaration) {
    output_type(out, nodes, n.declaration.type);
    fprintf(out, " ");
    output_identifier(out, nodes, n.declaration.name);
    fprintf(out, " = ");
    output_expression(out, nodes, n.declaration.initial_value);
    fprintf(out, ";\n");
    return;
  }

  if (n.kind == Ast_assign) {
    output_expression(out, nodes, n.assign.lhs);
    fprintf(out, " = ");
    output_expression(out, nodes, n.assign.value);
    fprintf(out, ";\n");
    return;
  }

  fprintf(out, "unknown statement encountered!\n");
}

void output_function_definition(FILE *out, Slice<AstNode> nodes, AstRef ref) {
  output_function_signature(out, nodes, ref);

  fprintf(out, " {\n");

  AstNode body = nodes[nodes[ref].function.body];
  for EachIndex(i, body.scope.statements.len) {
    output_statement(out, nodes, body.scope.statements[i]);
  }

  fprintf(out, "}\n");
}

b32 generate_c_code(FILE *out, Slice<AstNode> nodes, AstRef mod) {
  fprintf(out, "%s", preamble);
  
  AstNode n = nodes[mod];
  for EachIndex(i, n.module.items.len) {
    output_function_declaration(out, nodes, n.module.items[i]);
  }

  for EachIndex(i, n.module.items.len) {
    output_function_definition(out, nodes, n.module.items[i]);
  }

  return true;
}
