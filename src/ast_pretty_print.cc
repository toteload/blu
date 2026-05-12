#include "blu.hh"
#include <stdio.h>

static char const *binary_op_src_string(BinaryOpKind kind) {
  switch (kind) {
  case Mul:               return "*";
  case Div:               return "/";
  case Mod:               return "%";
  case Sub:               return "-";
  case Add:               return "+";
  case Bit_shift_left:    return "<<";
  case Bit_shift_right:   return ">>";
  case Bit_and:           return "&";
  case Bit_or:            return "|";
  case Bit_xor:           return "^";
  case Cmp_equal:         return "==";
  case Cmp_not_equal:     return "!=";
  case Cmp_greater_than:  return ">";
  case Cmp_greater_equal: return ">=";
  case Cmp_less_than:     return "<";
  case Cmp_less_equal:    return "<=";
  case Logical_and:       return "and";
  case Logical_or:        return "or";
  case BinaryOpKind_max:  break;
  }
  return "?";
}

static char const *unary_op_src_string(UnaryOpKind kind) {
  switch (kind) {
  case Negate:          return "-";
  case Not:             return "!";
  case UnaryOpKind_max: break;
  }
  return "?";
}

// Lower group binds tighter (matches parse.cc's op_precedence_group).
static u8 binop_prec_group(BinaryOpKind kind) {
  switch (kind) {
  case Mul:
  case Div:
  case Mod:               return 10;
  case Sub:
  case Add:               return 20;
  case Bit_shift_left:
  case Bit_shift_right:   return 30;
  case Bit_and:
  case Bit_or:
  case Bit_xor:           return 40;
  case Cmp_equal:
  case Cmp_not_equal:
  case Cmp_greater_than:
  case Cmp_greater_equal:
  case Cmp_less_than:
  case Cmp_less_equal:    return 50;
  case Logical_and:
  case Logical_or:        return 60;
  case BinaryOpKind_max:  break;
  }
  return 255;
}

struct AstPrinter {
  Str       text;
  Tokens   *tokens;
  AstNodes *nodes;

  Str token_str(TokenIndex idx) { return get_token_str(text, tokens, idx); }

  void print_indent(u32 indent) {
    for (u32 i = 0; i < indent; i++) {
      fputs("  ", stdout);
    }
  }

  void print(NodeIndex node, u32 indent);

  void print_binop_operand(NodeIndex node, u8 parent_group, bool is_right, u32 indent) {
    if (nodes->kind(node) == Ast_binary_op) {
      u8   child_group = binop_prec_group(nodes->data(node).binary_op.kind);
      bool need_parens = child_group > parent_group ||
                         (child_group == parent_group && is_right);
      if (need_parens) {
        fputs("{", stdout);
        print(node, indent);
        fputs("}", stdout);
        return;
      }
    }
    print(node, indent);
  }

  void print_unary_operand(NodeIndex node, u32 indent) {
    auto kind = nodes->kind(node);
    if (kind == Ast_binary_op || kind == Ast_unary_op) {
      fputs("{", stdout);
      print(node, indent);
      fputs("}", stdout);
      return;
    }
    print(node, indent);
  }
};

void AstPrinter::print(NodeIndex node, u32 indent) {
  auto kind = nodes->kind(node);
  auto data = nodes->data(node);

  switch (kind) {
  case Ast_root: {
    for (u32 i = 0; i < data.root.items.len(); i++) {
      if (i > 0) {
        fputs("\n", stdout);
      }
      print_indent(indent);
      print(data.root.items[i], indent);
      fputs("\n", stdout);
    }
  } break;

  case Ast_block: {
    if (data.block.items.len() == 0) {
      fputs("{ }", stdout);
      break;
    }
    fputs("{\n", stdout);
    for (u32 i = 0; i < data.block.items.len(); i++) {
      print_indent(indent + 1);
      print(data.block.items[i], indent + 1);
      fputs("\n", stdout);
    }
    print_indent(indent);
    fputs("}", stdout);
  } break;

  case Ast_type_slice: {
    fputs("[]", stdout);
    print(data.type_slice.base, indent);
  } break;

  case Ast_type_array: {
    fputs("[", stdout);
    print(data.type_array.size, indent);
    fputs("]", stdout);
    print(data.type_array.base, indent);
  } break;

  case Ast_type_function: {
    fputs("(", stdout);
    for (u32 i = 0; i < data.type_function.param_types.len(); i++) {
      if (i > 0) {
        fputs(", ", stdout);
      }
      print(data.type_function.param_types[i], indent);
    }
    fputs("): ", stdout);
    print(data.type_function.return_type, indent);
  } break;

  case Ast_builtin: {
    switch (data.builtin.kind) {
    case Builtin_print: {
      fputs("#print(", stdout);
      for (u32 i = 0; i < data.builtin.args.len(); i++) {
        if (i > 0) {
          fputs(", ", stdout);
        }
        print(data.builtin.args[i], indent);
      }
      fputs(")", stdout);
    } break;
    }
  } break;

  case Ast_declaration: {
    auto name = token_str(data.declaration.name);
    printf("%.*s : ", cast<int>(name.len()), name.str);
    print(data.declaration.type, indent);
    fputs(" = ", stdout);
    print(data.declaration.value, indent);
  } break;

  case Ast_const: {
    fputs("const ", stdout);
    print(data.const_.expr, indent);
  } break;

  case Ast_assign: {
    print(data.assign.lhs, indent);
    fputs(" = ", stdout);
    print(data.assign.value, indent);
  } break;

  case Ast_literal_sequence: {
    fputs(".{ ", stdout);
    for (u32 i = 0; i < data.literal_sequence.items.len(); i++) {
      if (i > 0) {
        fputs(", ", stdout);
      }
      print(data.literal_sequence.items[i], indent);
    }
    fputs(" }", stdout);
  } break;

  case Ast_literal_int: {
    auto s = token_str(data.literal_int.token_index);
    printf("%.*s", cast<int>(s.len()), s.str);
  } break;

  case Ast_literal_string: {
    auto s = token_str(data.literal_string.token_index);
    printf("%.*s", cast<int>(s.len()), s.str);
  } break;

  case Ast_identifier: {
    auto s = token_str(data.identifier.token_index);
    printf("%.*s", cast<int>(s.len()), s.str);
  } break;

  case Ast_call: {
    print(data.call.callee, indent);
    fputs("(", stdout);
    for (u32 i = 0; i < data.call.args.len(); i++) {
      if (i > 0) {
        fputs(", ", stdout);
      }
      print(data.call.args[i], indent);
    }
    fputs(")", stdout);
  } break;

  case Ast_index: {
    print(data.index.indexable, indent);
    fputs("[", stdout);
    print(data.index.index_at, indent);
    fputs("]", stdout);
  } break;

  case Ast_unary_op: {
    fputs(unary_op_src_string(data.unary_op.kind), stdout);
    print_unary_operand(data.unary_op.value, indent);
  } break;

  case Ast_binary_op: {
    u8 group = binop_prec_group(data.binary_op.kind);
    print_binop_operand(data.binary_op.lhs, group, false, indent);
    printf(" %s ", binary_op_src_string(data.binary_op.kind));
    print_binop_operand(data.binary_op.rhs, group, true, indent);
  } break;

  case Ast_function: {
    fputs("|", stdout);
    for (u32 i = 0; i < data.function.param_names.len(); i++) {
      if (i > 0) {
        fputs(", ", stdout);
      }
      print(data.function.param_names[i], indent);
    }
    fputs("| ", stdout);
    print(data.function.body, indent);
  } break;

  case Ast_if_else: {
    fputs("if ", stdout);
    print(data.if_else.cond, indent);
    fputs(" ", stdout);
    print(data.if_else.then, indent);
    if (data.if_else.otherwise.is_some()) {
      fputs(" else ", stdout);
      print(data.if_else.otherwise.as_index(), indent);
    }
  } break;

  case Ast_for: {
    fputs("for ", stdout);
    print(data.for_.iterable, indent);
    fputs(" do ", stdout);
    print(data.for_.iterator, indent);
    fputs(" ", stdout);
    print(data.for_.body, indent);
  } break;

  case Ast_break:    fputs("break", stdout); break;
  case Ast_continue: fputs("continue", stdout); break;

  case Ast_return: {
    fputs("return ", stdout);
    print(data.return_.value, indent);
  } break;

  case Ast_defer: {
    fputs("defer ", stdout);
    print(data.defer.value, indent);
  } break;

  case Ast_kind_max: break;
  }
}

void ast_pretty_print(Str text, Tokens *tokens, AstNodes *nodes) {
  if (nodes->len() == 0) {
    return;
  }

  AstPrinter printer = {
    .text   = text,
    .tokens = tokens,
    .nodes  = nodes,
  };
  printer.print(NodeIndex{0}, 0);
}
