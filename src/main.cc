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
  char *data = Cast(char *, malloc(size + 1));
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
  "colon",      "arrow",           "semicolon", "equals",      "minus",      "plus",
  "less-than",  "less-equal-than", "comma",     "literal_int", "brace_open", "brace_close",
  "paren_open", "paren_close",     "fn",        "return",      "if",         "else",
  "while", "break", "continue",      "identifier",      "<illegal>",
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
      "[%4d:%4d] %s = \"%.*s\"\n",
      tok.span.start.line,
      tok.span.start.col,
      token_kind_string(tok.kind),
      Cast(int, tok.span.len()),
      tok.span.start.p
    );
  }
}

void pad(FILE *out, u32 depth) { fprintf(out, "%*s", 2 * depth, ""); }

char const *ast_string[] = {
  "module",
  "param",
  "function",
  "scope",
  "identifier",
  "literal-int",
  "declaration",
  "assign",
  "while",
  "break",
  "continue",
  "call",
  "if-else",
  "binary-op",
  "unary-op",
  "return",
  "illegal",
};

char const *ast_kind_string(AstKind kind) {
  if (kind >= Ast_kind_max) {
    return ast_string[Ast_kind_max];
  }

  return ast_string[kind];
}

char const *binary_op_string[] = {
  "- (Sub)",
  "+ (Add)",
  "<= (LessEqual)",
  "illegal",
};

char const *binary_op_kind_string(BinaryOpKind kind) {
  if (kind >= BinaryOpKind_max) {
    return binary_op_string[BinaryOpKind_max];
  }

  return binary_op_string[kind];
}

void print_ast(FILE *out, Slice<AstNode> ast, AstRef idx, u32 depth = 0) {
  AstNode n = ast[idx];

  pad(out, depth);
  fprintf(out, "%s\n", ast_kind_string(n.kind));

  switch (n.kind) {
  case Ast_module: {
    ForEachIndex(i, n.module.items.len) { print_ast(out, ast, n.module.items[i], depth + 1); }
  } break;
  case Ast_while: {
    print_ast(out, ast, n._while.cond, depth + 1);
    print_ast(out, ast, n._while.body, depth + 1);
  } break;
  case Ast_param: {
    print_ast(out, ast, n.param.name, depth + 1);
    print_ast(out, ast, n.param.type, depth + 1);
  } break;
  case Ast_function: {
    print_ast(out, ast, n.function.name, depth + 1);
    print_ast(out, ast, n.function.return_type, depth + 1);
    print_ast(out, ast, n.function.body, depth + 1);
  } break;
  case Ast_assign: {
    print_ast(out, ast, n.assign.lhs, depth + 1);
    print_ast(out, ast, n.assign.value, depth + 1);
  } break;
  case Ast_return: {
    print_ast(out, ast, n.ret.value, depth + 1);
  } break;
  case Ast_scope: {
    ForEachIndex(i, n.scope.statements.len) {
      print_ast(out, ast, n.scope.statements[i], depth + 1);
    }
  } break;
  case Ast_identifier: {
    Str identifier = n.span.str();
    pad(out, depth + 1);
    fprintf(out, " '%.*s'\n", cast<i32>(identifier.len), identifier.str);
  } break;
  case Ast_declaration: {
    print_ast(out, ast, n.declaration.name, depth + 1);
    print_ast(out, ast, n.declaration.type, depth + 1);
    print_ast(out, ast, n.declaration.initial_value, depth + 1);
  } break;
  case Ast_literal_int: {
    Str literal = n.span.str();
    pad(out, depth + 1);
    fprintf(out, " '%.*s'\n", cast<i32>(literal.len), literal.str);
  } break;
  case Ast_call: {
    print_ast(out, ast, n.call.f, depth + 1);
    ForEachIndex(i, n.call.arguments.len) { print_ast(out, ast, n.call.arguments[i], depth + 1); }
  } break;
  case Ast_if_else: {
    print_ast(out, ast, n.if_else.cond, depth + 1);
    print_ast(out, ast, n.if_else.then, depth + 1);
    if (n.if_else.otherwise != nil) {
      print_ast(out, ast, n.if_else.otherwise, depth + 1);
    }
  } break;
  case Ast_binary_op: {
    pad(out, depth + 1);
    fprintf(out, "op: %s\n", binary_op_kind_string(n.binary_op.kind));
    print_ast(out, ast, n.binary_op.lhs, depth + 1);
    print_ast(out, ast, n.binary_op.rhs, depth + 1);
  } break;
  case Ast_unary_op: {
    print_ast(out, ast, n.unary_op.value, depth + 1);
  } break;

  default: break;
  }
}

void display_message(FILE *out, Message *msg) {
  char const *severity = "unknown";
  switch (msg->severity) {
  case Error:
    severity = "error";
    break;
  case Warning:
    severity = "warning";
    break;
  case Info:
    severity = "info";
    break;
  };

  fprintf(
    out,
    "[%d:%d] %s: %.*s\n",
    msg->span.start.line,
    msg->span.start.col,
    severity,
    cast<i32>(msg->message.len),
    msg->message.str
  );
}

int main() {
  Allocator stdlib_alloc{stdlib_alloc_fn, nullptr};

  usize source_len   = 0;
  char const *source = read_file("fib.blu", &source_len);
  if (source == nullptr) {
    printf("Could not find source\n");
    return 1;
  }

  Arena arena;
  arena.init(MiB(32));

  Vector<Message> messages;
  messages.init(stdlib_alloc);

  CompilerContext compiler_context;
  compiler_context.arena    = &arena;
  compiler_context.messages = messages.move();

  Vector<Token> tokens;
  tokens.init(stdlib_alloc);

  b32 ok;
  ok = tokenize(&compiler_context, source, source_len, &tokens);
  if (!ok) {
    printf("Encountered error during tokenizing.\n");
    ForEachIndex(i, compiler_context.messages.len) {
      Message *msg = &compiler_context.messages[i];
      display_message(stdout, msg);
    }
    return 1;
  }

  print_tokens(stdout, tokens.data, tokens.len);

  StringInterner strings;
  strings.init(stdlib_alloc, stdlib_alloc, stdlib_alloc);

  Vector<AstNode> nodes;
  nodes.init(stdlib_alloc);

  ParseContext parse_context{
    &compiler_context,
    &strings,
    stdlib_alloc,
    &nodes,
  };

  AstRef root;
  ok = parse(&parse_context, tokens.slice(), &root);
  if (!ok) {
    printf("Encountered error during parsing.\n");
    ForEachIndex(i, compiler_context.messages.len) {
      Message *msg = &compiler_context.messages[i];
      display_message(stdout, msg);
    }
    return 1;
  }

  print_ast(stdout, nodes.slice(), root);

  // FILE *out = fopen("main.c", "w");
  generate_c_code(stdout, nodes.slice(), root);
  // fclose(out);

  printf("Done!\n");

  return 0;
}
