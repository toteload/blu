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
  "colon",        "arrow",         "semicolon",  "equals",      "minus",       "plus",
  "star",         "slash",         "percent",    "plus_equals", "exclamation", "ampersand",
  "bar",          "caret",         "tilde",      "left-shift",  "right-shift", "cmp-eq",
  "cmp-ne",       "cmp-gt",        "cmp-ge",     "cmp-lt",      "cmp-le",      "comma",
  "dot",          "literal_int",   "brace_open", "brace_close", "paren_open",  "paren_close",
  "bracket_open", "bracket_close", "fn",         "return",      "if",          "else",
  "while",        "break",         "continue",   "and",         "or",          "for",
  "in",           "identifier",    "#run",       "<illegal>",
};

char const *token_kind_string(u32 kind) {
  if (kind >= Tok_kind_max) {
    return token_string[Tok_kind_max];
  }

  return token_string[kind];
}

void print_tokens(FILE *out, Slice<Token> tokens) {
  for (usize i = 0; i < tokens.len; i++) {
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
  "module",    "param",    "function",     "scope",      "identifier", "literal-int", "declaration",
  "assign",    "while",    "break",        "continue",   "for",        "call",        "if-else",
  "binary-op", "unary-op", "type_pointer", "type_slice", "deref",      "return",      "illegal",
};

char const *ast_kind_string(AstKind kind) {
  if (kind >= Ast_kind_max) {
    return ast_string[Ast_kind_max];
  }

  return ast_string[kind];
}

char const *binary_op_string[] = {
  "* (Mul)",     "/ (Div)",           "% (Mod)",
  "- (Sub)",     "+ (Add)",           "<<",
  ">>",          "& (Bit_and)",       "| (Bit_or)",
  "^ (Bit_xor)", "== (CmpEq)",        "!= (CmpNe)",
  "> (CmpGt)",   ">= (CmpGe)",        "< (CmpLt)",
  "<= (CmpLe)",  "and (Logical_and)", "or (Logical_or)",
  "(Assign)",    "+= (AddAssign)",    "illegal",
};

char const *binary_op_kind_string(BinaryOpKind kind) {
  if (kind >= BinaryOpKind_max) {
    return binary_op_string[BinaryOpKind_max];
  }

  return binary_op_string[kind];
}

void print_ast(FILE *out, AstNode *ref, u32 depth = 0) {
  pad(out, depth);

  if (!ref) {
    fprintf(out, "NULL\n");
    return;
  }

  AstNode n = *ref;
  fprintf(out, "%s\n", ast_kind_string(n.kind));

  switch (n.kind) {
  case Ast_module: {
    ForEachAstNode(item, n.module.items) { print_ast(out, item, depth + 1); }
  } break;
  case Ast_while: {
    print_ast(out, n._while.cond, depth + 1);
    print_ast(out, n._while.body, depth + 1);
  } break;
  case Ast_for: {
    print_ast(out, n._for.item, depth+1);
    print_ast(out, n._for.iterable, depth+1);
    print_ast(out, n._for.body, depth+1);
  } break;
  case Ast_param: {
    print_ast(out, n.param.name, depth + 1);
    print_ast(out, n.param.type, depth + 1);
  } break;
  case Ast_function: {
    print_ast(out, n.function.return_type, depth + 1);
    ForEachAstNode(param, n.function.params) { print_ast(out, param, depth + 1); }
    print_ast(out, n.function.body, depth + 1);
  } break;
  case Ast_assign: {
    print_ast(out, n.assign.lhs, depth + 1);
    print_ast(out, n.assign.value, depth + 1);
  } break;
  case Ast_return: {
    print_ast(out, n._return.value, depth + 1);
  } break;
  case Ast_scope: {
    ForEachAstNode(e, n.scope.expressions) { print_ast(out, e, depth + 1); }
  } break;
  case Ast_identifier: {
    Str identifier = n.span.str();
    pad(out, depth + 1);
    fprintf(out, " '%.*s'\n", cast<i32>(identifier.len), identifier.str);
  } break;
  case Ast_declaration: {
    print_ast(out, n.declaration.name, depth + 1);
    print_ast(out, n.declaration.type, depth + 1);
    print_ast(out, n.declaration.value, depth + 1);
  } break;
  case Ast_literal_int: {
    Str literal = n.span.str();
    pad(out, depth + 1);
    fprintf(out, " '%.*s'\n", cast<i32>(literal.len), literal.str);
  } break;
  case Ast_call: {
    print_ast(out, n.call.callee, depth + 1);
    ForEachAstNode(arg, n.call.args) { print_ast(out, arg, depth + 1); }
  } break;
  case Ast_if_else: {
    print_ast(out, n.if_else.cond, depth + 1);
    print_ast(out, n.if_else.then, depth + 1);
    if (n.if_else.otherwise != nullptr) {
      print_ast(out, n.if_else.otherwise, depth + 1);
    }
  } break;
  case Ast_binary_op: {
    pad(out, depth + 1);
    fprintf(out, "op: %s\n", binary_op_kind_string(n.binary_op.kind));
    print_ast(out, n.binary_op.lhs, depth + 1);
    print_ast(out, n.binary_op.rhs, depth + 1);
  } break;
  case Ast_unary_op: {
    pad(out, depth + 1);
    fprintf(out, "op: %d\n", n.unary_op.kind);
    print_ast(out, n.unary_op.value, depth + 1);
  } break;
  case Ast_deref: {
    print_ast(out, n.deref.value, depth + 1);
  } break;

  default:
    break;
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
    "[%s:%d]\n"
    "[%d:%d] %s: %.*s\n",
    msg->file,
    msg->line,
    msg->span.start.line,
    msg->span.start.col,
    severity,
    cast<i32>(msg->message.len),
    msg->message.str
  );
}

void populate_global_environment(CompilerContext *ctx) {
#define Add_type(Identifier, T)                                                                    \
  {                                                                                                \
    auto _id  = ctx->strings.add(Str_make(Identifier));                                            \
    auto _tmp = T;                                                                                 \
    auto _t   = ctx->types.add(&_tmp);                                                             \
    ctx->global_environment->insert(_id, Value::make_type(_t));                                    \
  }

  // clang-format off
  Add_type("i8",  Type::make_integer(Signed,  8));
  Add_type("i16", Type::make_integer(Signed, 16));
  Add_type("i32", Type::make_integer(Signed, 32));
  Add_type("i64", Type::make_integer(Signed, 64));

  Add_type("u8",  Type::make_integer(Unsigned,  8));
  Add_type("u16", Type::make_integer(Unsigned, 16));
  Add_type("u32", Type::make_integer(Unsigned, 32));
  Add_type("u64", Type::make_integer(Unsigned, 64));

  Add_type("bool", Type::make_bool());
  Add_type("void", Type::make_void());
  // clang-format on

  {
    auto id = ctx->strings.add(Str_make("true"));
    ctx->global_environment->insert(id, Value::make_builtin(ctx->types._bool));
  }

#undef Add_type
}

int main() {
  Allocator stdlib_alloc{stdlib_alloc_fn, nullptr};

  usize source_len   = 0;
  char const *source = read_file("fib.blu", &source_len);
  if (source == nullptr) {
    printf("Could not find source\n");
    return 1;
  }

  CompilerContext compiler_context;
  compiler_context.arena.init(MiB(32));
  compiler_context.messages.init(stdlib_alloc);
  compiler_context.ref_allocator = stdlib_alloc;
  compiler_context.strings.init(stdlib_alloc, stdlib_alloc, stdlib_alloc);
  compiler_context.types.init(&compiler_context.arena, stdlib_alloc, stdlib_alloc);
  compiler_context.tokens.init(stdlib_alloc);
  compiler_context.nodes.init(stdlib_alloc);
  compiler_context.environments.init(stdlib_alloc, stdlib_alloc);
  compiler_context.global_environment = compiler_context.environments.alloc();

  populate_global_environment(&compiler_context);

  b32 ok;
  ok = tokenize(&compiler_context, source, source_len);
  if (!ok) {
    printf("Encountered error during tokenizing.\n");
    ForEachIndex(i, compiler_context.messages.len) {
      Message *msg = &compiler_context.messages[i];
      display_message(stdout, msg);
    }
    return 1;
  }

  print_tokens(stdout, compiler_context.tokens.slice());

  ok = parse(&compiler_context);
  if (!ok) {
    printf("Encountered error during parsing.\n");
    ForEachIndex(i, compiler_context.messages.len) {
      Message *msg = &compiler_context.messages[i];
      display_message(stdout, msg);
    }
    return 1;
  }

  print_ast(stdout, compiler_context.root);

  printf("Type inference ... ");
  ok = infer_types(&compiler_context, compiler_context.root);
  if (!ok) {
    printf("Encountered error during type inference\n");
    return 1;
  }
  printf("DONE\n");

  printf("C code generation ... ");
  FILE *out = fopen("fib.c", "w");
  generate_c_code(&compiler_context, out, compiler_context.root);
  fclose(out);
  printf("DONE\n");

  return 0;
}
