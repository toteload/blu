#include "blu.hh"

#include <stdio.h>
#include <stdarg.h>

struct StringWriter {
  Arena *arena;

  void vprintf(char const *format, va_list arg) {
    int len   = vsnprintf(nullptr, 0, format, arg)+1;
    char *buf = arena->alloc<char>(len);
    vsnprintf(buf, len, format, arg);

    // Kinda ugly.
    // vsnprintf always writes a null terminator even though we don't want that.
    // We allocate enough memory for the string + the null terminator, then we move the pointer
    // one byte back so that next time the null terminator gets overwritten.
    arena->at = ptr_offset(arena->at, -1);
  }

  void write_to_file(char const *filename) {
    FILE *f = fopen(filename, "w");
    fwrite(arena->base, 1, arena->size(), f);
    fclose(f);
  }
};

struct CodeGenerator {
  StringWriter writer;

  MessageManager *messages = nullptr;
  Source *source = nullptr;
  ValueStore *values;
  TypeAnnotations *type_annotations;
  TypeInterner *types;

  void output_format(char const *format, ...) {
    va_list vl;
    va_start(vl, format);
    writer.vprintf(format, vl);
    va_end(vl);
  }

  void output_str(Str s) {
    output_format("%.*s", cast<i32>(s.len()), s.str);
  }

  Str gensym() {
    return Str_make("a");
  }

  void output_function_signature(Str name, Type *function);

  void output_type(Type *type);
  void output_type_index(TypeIndex idx) { output_type(types->get(idx)); }

  void output_declaration(AstDeclaration decl);
  void output_definition(AstDeclaration decl);

  void output_expression(NodeIndex node_index, Str dst);

  void output_if_else_expression_without_value(AstIfElse if_else);
  void output_if_else_expression(AstIfElse if_else, Type *type, Str dst);

  void output_function_definition(Str name, Type *type, AstFunction func);

  void output_source();
};

void CodeGenerator::output_if_else_expression_without_value(AstIfElse if_else) {
  Str sym_cond;
  output_expression(if_else.cond, &sym_cond);  

  output_format("if (");
  output_str(sym_cond);
  output_format(") {\n");
  output_expression(if_else.then);
  if (if_else.otherwise.is_some()) {
    output_format("} else {\n");
    output_expression(if_else.otherwise.as_index());
  }
  output_format("}\n");
}

void CodeGenerator::output_if_else_expression(AstIfElse if_else, Str dst) {
  Str sym_val = gensym();
  output_type(type);
  output_format(" ");
  output_str(sym_val);
  output_format(";\n");

  Str sym_cond;
  output_expression(if_else.cond, &sym_cond);  

  output_format("if (");
  output_str(sym_cond);
  output_format(") {\n");

  Str sym_then;
  output_expression(if_else.then, &sym_then);

  output_str(sym_val);
  output_format(" = ");
  output_str(sym_then);
  output_format(";\n");

  if (if_else.otherwise.is_some()) {
    output_format("} else {\n");
    Str sym_otherwise;
    output_expression(if_else.otherwise.as_index(), &sym_otherwise);

    output_str(sym_val);
    output_format(" = ");
    output_str(sym_otherwise);
    output_format(";\n");
  }

  output_format("}\n");
}

void CodeGenerator::output_block() {
}

void CodeGenerator::output_expression(NodeIndex node_index, Str dst) {
  auto kind = source->nodes->kind(node_index);

  TypeIndex idx = type_annotations->get(node_index);
  Type *type = types->get(idx);

  b32 has_return_value = false;
  if (type->kind == Type_integer || type->kind == Type_boolean) {
    has_return_value = true;
  }

  // clang-format off
  switch (kind) {
  case Ast_if_else: {
    auto if_else = source->nodes->data(node_index).if_else;
    output_if_else_expression(node_index, dst);
  } break;
  case Ast_block: output_block(node_index, val); break;
  }
  // clang-format on
}

void CodeGenerator::output_type(Type *type) {
  switch (type->kind) {
  case Type_integer: {
    output_format("%sint%d_t", type->integer.signedness == Unsigned ? "u" : "", type->integer.bitwidth);
      break;
  default:
    Todo();
  } break;
  }
}

void CodeGenerator::output_block(NodeIndex node_index, Str *val) {
  output_format("{\n");
  output_format("}\n");
}

void CodeGenerator::output_function_signature(Str name, Type *function) {
  Debug_assert(function->kind == Type_function);

  output_type_index(function->function.return_type);
  output_format(" ");
  output_str(name);
  output_format("(");
  if (function->function.param_count > 0) {
    output_type_index(function->function.param_types[0]);
  }
  for (u32 i = 1; i < function->function.param_count; i += 1) {
    output_format(", ");
    output_type_index(function->function.param_types[i]);
  }
  output_format(")");
}

void CodeGenerator::output_function_definition(Str name, Type *type, AstFunction func) {
  Debug_assert(type->kind == Type_function);

  output_type_index(type->function.return_type);
  output_format(" ");
  output_str(name);
  output_format("(");
  if (type->function.param_count > 0) {
    output_type_index(type->function.param_types[0]);
    output_format(" ");
    Str param_name = source->get_token_str(func.params[0].name);
    output_str(param_name);
  }
  for (u32 i = 1; i < type->function.param_count; i += 1) {
    output_format(", ");
    output_type_index(type->function.param_types[i]);
    output_format(" ");
    Str param_name = source->get_token_str(func.params[i].name);
    output_str(param_name);
  }
  output_format(") {\n");
  Str sym_body;
  output_expression(func.body, &sym_body);
  if (sym_body.is_ok()) {
    output_format("return ");
    output_str(sym_body);
    output_format(";\n");
  }
  output_format("}\n");
}

void CodeGenerator::output_definition(AstDeclaration decl) {
  Str name = source->get_token_str(decl.name);
  TypeIndex typeof_decl = type_annotations->get(decl.type);
  Type *type = types->get(typeof_decl);

  switch (type->kind) {
  case Type_function: {
    output_function_definition(name, type, source->nodes->data(decl.value).function);
  } break;
  case Type_type: break;
  default: Todo();
  }
}

void CodeGenerator::output_declaration(AstDeclaration decl) {
  Str name = source->get_token_str(decl.name);
  TypeIndex typeof_decl = type_annotations->get(decl.type);
  Type *type = types->get(typeof_decl);

  switch (type->kind) {
  case Type_function: { output_function_signature(name, type); output_format(";\n"); } break;
  case Type_type: break;
  default:
    Todo();
  }
}

void CodeGenerator::output_source() {
  auto nodes = source->nodes;

  auto items = nodes->data({0}).root.items;

  for (usize i = 0; i < items.len(); i += 1) {
    auto decl = nodes->data(items.at(i)).declaration;
    output_declaration(decl);
  }

  for (usize i = 0; i < items.len(); i += 1) {
    auto decl = nodes->data(items.at(i)).declaration;
    output_definition(decl);
  }
}

b32 generate_c_code(CodeGeneratorContext *ctx, Source *source) {
  CodeGenerator gen;
  gen.writer.arena = ctx->output_arena;
  gen.messages = ctx->messages;
  gen.values = ctx->values;
  gen.type_annotations = ctx->type_annotations;
  gen.types = ctx->types;

  gen.source = source;

  gen.output_source();

  gen.writer.write_to_file("out.c");

  return true;
}
