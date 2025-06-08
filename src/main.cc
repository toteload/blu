#include <stdio.h>

#include "blu.hh"

void *stdlib_alloc_fn(void *ctx, void *p, usize old_byte_size, usize new_byte_size, u32 align) {
  if (!Is_null(p) && new_byte_size == 0) {
    free(p);
    return nullptr;
  }

  return realloc(p, new_byte_size);
}

char const *read_file(char const *filename, usize *len) {
  FILE *f = fopen(filename, "rb");
  if (!f) {
    return nullptr;
  }

  fseek(f, 0, SEEK_END);

  u32 size   = ftell(f);
  char *data = Cast(char* ,malloc(size+1));
  if (!data) {
    return nullptr;
  }

  fseek(f, 0, SEEK_SET);

  u64 bytes_read = fread(data, 1, size, f);
  if (bytes_read != size) {
    return nullptr;
  }

  // Set a zero-terminated byte, but don't include it in the length.
  data[size] = '\0';

  *len = size;

  return data;
}

char const *token_string[] = {
  "colon      ",
  "semicolon  ",
  "equals     ",
  "minus      ",
  "literal_int",
  "brace_open ",
  "brace_close",
  "paren_open ",
  "paren_close",
  "keyword_fn ",
  "keyword_return",
  "identifier ",
  "<illegal>  ",
};

char const *token_kind_string(u32 kind) {
  if (kind >= Tok_kind_max) {
    return token_string[Tok_kind_max];
  }

  return token_string[kind];
}

void print_tokens(FILE *out, Token *tokens, usize count) {
  for (usize i = 0; i < count; i++) {
    Token tok = tokens[i];

    fprintf(
      out,
      "%s [%4d:%4d] = \"%.*s\"\n",
      token_kind_string(tok.kind),
      tok.span.start.line,
      tok.span.start.col,
      Cast(int, tok.span.len()),
      tok.span.start.p
    );
  }
}

void pad(FILE *out, u32 depth) {
  fprintf(out, "%*s", 2 * depth, "");
}

char const *ast_string[] = {
  "module",
  "function",
  "scope",
  "identifier",
  "literal-int",
  "declaration",
  "assign",
  "return",
  "illegal",
};

char const *ast_kind_string(AstKind kind) {
  if (kind >= Ast_kind_max) {
    return ast_string[Ast_kind_max];
  }

  return ast_string[kind];
}

void print_ast(FILE *out, Slice<AstNode> ast, AstRef idx, u32 depth = 0) {
  AstNode n = ast[idx];

  pad(out, depth);
  fprintf(out, "%s\n", ast_kind_string(n.kind));

  switch (n.kind) {
  case Ast_module: {
    for EachIndex(i, n.module.items.len) {
      print_ast(out, ast, n.module.items[i], depth + 1);
    }
  } break;
  case Ast_function: {
    print_ast(out, ast, n.function.name, depth+1);
    print_ast(out, ast, n.function.return_type, depth+1);
    print_ast(out, ast, n.function.body, depth+1);
  } break;
  case Ast_assign: {
    print_ast(out, ast, n.assign.lhs, depth+1);
    print_ast(out, ast, n.assign.value, depth+1);
  } break;
  case Ast_return: {
    print_ast(out, ast, n.ret.value, depth+1);
  } break;
  case Ast_scope: {
    for EachIndex(i, n.scope.statements.len) {
      print_ast(out, ast, n.scope.statements[i], depth+1);
    }
  } break;
  case Ast_identifier: {
    Str identifier = n.span.str();
    pad(out, depth+1);
    fprintf(out, " '%.*s'\n", identifier.len, identifier.str);
  } break;
  case Ast_declaration: {
    print_ast(out, ast, n.declaration.name, depth + 1);
    print_ast(out, ast, n.declaration.type, depth + 1);
    print_ast(out, ast, n.declaration.initial_value, depth + 1);
  } break;
  case Ast_literal_int: {
    Str literal = n.span.str();
    pad(out, depth+1);
    fprintf(out, " '%.*s'\n", literal.len, literal.str);
  } break;
  case Ast_kind_max: {} break;
  }
}

int main() { 
  Allocator stdlib_alloc{ stdlib_alloc_fn, nullptr };

  usize source_len = 0;
  char const *source = read_file("test.blu", &source_len);
  if (source == nullptr) {
    printf("Could not find source\n");
    return 1;
  }

  Vector<Token> tokens;
  tokens.init(stdlib_alloc);

  b32 ok;
  ok = tokenize(source, source_len, &tokens);
  if (!ok) {
    printf("Encountered error during tokenizing.\n");
    return 1;
  }

  print_tokens(stdout, tokens.data, tokens.len);

  StringInterner strings;
  strings.init(stdlib_alloc, stdlib_alloc, stdlib_alloc);

  Vector<AstNode> nodes;
  nodes.init(stdlib_alloc);

  ParseContext parse_context{
    &strings,
    stdlib_alloc,
    &nodes,
  };

  AstRef root;
  ok = parse(&parse_context, tokens.slice(), &root);
  if (!ok) {
    printf("Encountered error during parsing.\n");
    return 1;
  }

  print_ast(stdout, nodes.slice(), root);

  FILE *out = fopen("main.c", "w");
  generate_c_code(out, nodes.slice(), root);
  fclose(out);

  printf("Done!\n");

  return 0; 
}
