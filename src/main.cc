#include <stdio.h>

#include "blu.hh"

void print_tokens(FILE *out, Slice<Token> tokens) {
  for (usize i = 0; i < tokens.len(); i++) {
    Token tok = tokens[i];

    fprintf(
      out,
      "[%4d:%4d] %s = \"%.*s\"\n",
      tok.span.start.line,
      tok.span.start.col,
      token_kind_string(tok.kind),
      cast<int>(tok.str.len()),
      tok.str.str
    );
  }
}

void pad(FILE *out, u32 depth) { fprintf(out, "%*s", 2 * depth, ""); }

struct PrintAstContext {
  FILE *out;
  Slice<Token> tokens;
};

void print_ast(PrintAstContext *ctx, AstNode *ref, u32 depth = 0) {
  pad(ctx->out, depth);

  if (!ref) {
    fprintf(ctx->out, "NULL\n");
    return;
  }

  AstNode n = *ref;
  fprintf(ctx->out, "%s\n", ast_kind_string(n.kind));

  switch (n.kind) {
  case Ast_module: {
    ForEachAstNode(item, n.module.items) { print_ast(ctx, item, depth + 1); }
  } break;
  case Ast_type: {
    pad(ctx->out, depth + 1);
    fprintf(ctx->out, "kind: ");
    switch (n.ast_type.kind) {
    case Ast_type_identifier: {
      fprintf(ctx->out, "<type-identifier>\n");
      print_ast(ctx, n.ast_type.base, depth + 1);
    } break;
    case Ast_type_pointer: {
      fprintf(ctx->out, "<type-pointer>\n");
      print_ast(ctx, n.ast_type.base, depth + 1);
    } break;
    case Ast_type_slice: {
      fprintf(ctx->out, "<type-slice>\n");
      print_ast(ctx, n.ast_type.base, depth + 1);
    } break;
    }
  } break;
  case Ast_while: {
    print_ast(ctx, n.while_.cond, depth + 1);
    print_ast(ctx, n.while_.body, depth + 1);
  } break;
  case Ast_for: {
    print_ast(ctx, n.for_.item, depth + 1);
    print_ast(ctx, n.for_.iterable, depth + 1);
    print_ast(ctx, n.for_.body, depth + 1);
  } break;
  case Ast_param: {
    print_ast(ctx, n.param.name, depth + 1);
    print_ast(ctx, n.param.type, depth + 1);
  } break;
  case Ast_function: {
    print_ast(ctx, n.function.return_type, depth + 1);
    ForEachAstNode(param, n.function.params) { print_ast(ctx, param, depth + 1); }
    print_ast(ctx, n.function.body, depth + 1);
  } break;
  case Ast_assign: {
    print_ast(ctx, n.assign.lhs, depth + 1);
    print_ast(ctx, n.assign.value, depth + 1);
  } break;
  case Ast_return: {
    print_ast(ctx, n.return_.value, depth + 1);
  } break;
  case Ast_scope: {
    ForEachAstNode(e, n.scope.expressions) { print_ast(ctx, e, depth + 1); }
  } break;
  case Ast_identifier: {
    Str identifier = get_ast_str(ref, ctx->tokens);
    pad(ctx->out, depth + 1);
    fprintf(ctx->out, " '%.*s'\n", cast<i32>(identifier.len()), identifier.str);
  } break;
  case Ast_declaration: {
    print_ast(ctx, n.declaration.name, depth + 1);
    print_ast(ctx, n.declaration.type, depth + 1);
    print_ast(ctx, n.declaration.value, depth + 1);
  } break;
  case Ast_literal_int: {
    Str literal = get_ast_str(ref, ctx->tokens);
    pad(ctx->out, depth + 1);
    fprintf(ctx->out, " '%.*s'\n", cast<i32>(literal.len()), literal.str);
  } break;
  case Ast_call: {
    print_ast(ctx, n.call.callee, depth + 1);
    ForEachAstNode(arg, n.call.args) { print_ast(ctx, arg, depth + 1); }
  } break;
  case Ast_if_else: {
    print_ast(ctx, n.if_else.cond, depth + 1);
    print_ast(ctx, n.if_else.then, depth + 1);
    if (n.if_else.otherwise != nullptr) {
      print_ast(ctx, n.if_else.otherwise, depth + 1);
    }
  } break;
  case Ast_binary_op: {
    pad(ctx->out, depth + 1);
    fprintf(ctx->out, "op: %s\n", binary_op_kind_string(n.binary_op.kind));
    print_ast(ctx, n.binary_op.lhs, depth + 1);
    print_ast(ctx, n.binary_op.rhs, depth + 1);
  } break;
  case Ast_unary_op: {
    pad(ctx->out, depth + 1);
    fprintf(ctx->out, "op: %d\n", n.unary_op.kind);
    print_ast(ctx, n.unary_op.value, depth + 1);
  } break;
  case Ast_deref: {
    print_ast(ctx, n.deref.value, depth + 1);
  } break;
  case Ast_field_access: {
    print_ast(ctx, n.field_access.base, depth+1);
    print_ast(ctx, n.field_access.field, depth+1);
  } break;
  case Ast_continue:
  case Ast_break:
    break;

  default: {
    fprintf(ctx->out, "unimplemented\n");
  } break;
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
    cast<i32>(msg->format.len()),
    msg->format.str
  );
}

void completion_listener(Compiler *compiler, JobKind kind, Source *source) {
  if (kind != Job_parse) {
    return;
  }

  PrintAstContext ctx;
  ctx.out    = stdout;
  ctx.tokens = source->tokens.slice();

  print_ast(&ctx, source->mod);
}

int main(i32 arg_count, char const *const *args) {
  if (arg_count < 2) {
    printf("Please provide an input file\n");
    return 1;
  }

  Arena work_arena;
  work_arena.init(MiB(1));

  Str source_filename = Str::from_cstr(args[1]);

  Compiler compiler;

  compiler.init();
  compiler.register_job_completion_listener(completion_listener);
  compiler.compile_file(source_filename);

  printf("DONE\n");

  return 0;
}
