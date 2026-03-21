#include "blu.hh"

struct HirGenerator {
  MessageManager *messages = nullptr;
  Source *source = nullptr;
  StringInterner *strings = nullptr;
  HirCode *code = nullptr;

  StrKey true_, false_;

  void init(HirGeneratorContext *context, Source *source, HirCode *code);
  void deinit() {}

  b32 generate_from_root(NodeIndex root);
  HirIndex generate_function(NodeIndex node_index);
  HirIndex generate_block(NodeIndex node_index);
  HirIndex generate_inline_block(NodeIndex node_index);
  HirIndex generate_expression(NodeIndex node_index);
  HirIndex generate_type(NodeIndex node_index);
};

void HirGenerator::init(HirGeneratorContext *context, Source *source, HirCode *code) {
  messages = context->messages;
  strings = context->strings;

  this->source = source;
  this->code = code;

  true_ = strings->add(Str_make("true"));
  false_ = strings->add(Str_make("false"));
}

b32 HirGenerator::generate_from_root(NodeIndex root_node_index) {
  auto root = source->nodes->data(root_node_index).root;

  // Generate code for all declarations
  for (usize i = 0; i < root.items.len(); i += 1) {
    Assert(source->nodes->kind(root.items[i]) == Ast_declaration);

    auto ast_decl = source->nodes->data(root.items[i]).declaration;

    HirIndex decl = code->add(
        Hir_declaration, 
        { .declaration = { 
        .value = INVALID_HIR_INDEX,
        .instruction_count = UINT32_MAX,
        }, });

    HirIndex type = generate_type(ast_decl.type);

    HirIndex value = generate_expression(ast_decl.value);

    *code->data_ptr(decl) = { .declaration = {
      .value = value,
      .instruction_count = (code->len() - decl.inner()) + 1,
    }};
  }

  return true;
}

HirIndex HirGenerator::generate_function(NodeIndex node_index) {
  auto function = source->nodes->data(node_index).function;

  auto function_idx = code->add(
    Hir_function, 
    { 
      .function = { 
        .param_count = cast<u32>(function.param_names.len()),
        .instruction_count = UINT32_MAX,
      },
    }
  );

  for (usize i = 0; i < function.param_names.len(); i += 1) {
    code->add(Hir_param, { .parameter = { } });
  }

  HirIndex body = generate_inline_block(function.body);

  auto function_idx_end = code->add(Hir_return, { .idx = body, });

  u32 instruction_count = function_idx_end.inner() - function_idx.inner() + 1;
  code->data_ptr(function_idx)->function.instruction_count = instruction_count;

  return function_idx;
}

HirIndex HirGenerator::generate_block(NodeIndex block_node_index) {
  auto hir_block = code->add(Hir_block, {});
  auto offset_start = code->len();

  HirIndex last_expression = generate_inline_block(block_node_index);
  code->add(Hir_break, { .break_ = { .value = last_expression, .block = hir_block, }, });
  
  auto offset_end = code->len();
  
  code->get(hir_block).data->block.instruction_count = offset_end - offset_start;

  return hir_block;
}

HirIndex HirGenerator::generate_inline_block(NodeIndex node_index) {
  auto block = source->nodes->data(node_index).block;

  HirIndex last_expression;

  if (block.items.len() == 0) {
    last_expression = code->add(Hir_nil, {});
  } else {
    for (usize i = 0; i < block.items.len(); i += 1) {
      last_expression = generate_expression(block.items[i]);
    }
  }

  return last_expression;
}

i64 parse_integer(Str s) {
  // TODO
  return 0;
}

HirIndex HirGenerator::generate_expression(NodeIndex node_index) {
  auto kind = source->nodes->kind(node_index);

  switch (kind) {
    case Ast_function: { return generate_function(node_index); } break;
    case Ast_if_else: {
      auto if_else_block = code->add(Hir_block, {});

      auto offset_start = code->len();

      auto if_else = source->nodes->data(node_index).if_else;
      HirIndex cond = generate_expression(if_else.cond);

      auto hir_condbr = code->add(Hir_condbr, { .condbr = { .cond = cond, }, });

      auto offset_then = code->len();
      HirIndex then = generate_inline_block(if_else.then);
      code->add(Hir_break, { .break_ = { .value = then, .block = if_else_block, }, });

      auto offset_otherwise = code->len();
      HirIndex otherwise;
      if (if_else.otherwise.is_some()) {
        otherwise = generate_inline_block(if_else.otherwise.as_index());
      } else {
        otherwise = code->add(Hir_nil, {});
      }
      code->add(Hir_break, { .break_ = { .value = otherwise, .block = if_else_block, }, });

      auto offset_end = code->len();

      code->get(hir_condbr).data->condbr.then_instruction_count = offset_otherwise - offset_then;
      code->get(if_else_block).data->block.instruction_count = offset_end - offset_start;

      return if_else_block;
    } break;
    case Ast_literal_int: {
      TokenIndex tok = source->nodes->data(node_index).literal_int.token_index;
      Str literal = source->get_token_str(tok);
      i64 val = parse_integer(literal);

      return code->add(Hir_literal_int, { .int64 = val, });
    } break;
    case Ast_block: { return generate_block(node_index); } break;
    case Ast_identifier: {
      TokenIndex ti = source->nodes->data(node_index).identifier.token_index;
      StrKey key = strings->add(source->get_token_str(ti));
      if (key == true_) {
        return code->add(Hir_true, {});
      }
      if (key == false_) {
        return code->add(Hir_false, {});
      }
      Todo();
    } break;
    default: Todo();
  }
}

HirIndex HirGenerator::generate_type(NodeIndex node_index) {
  AstKind kind = source->nodes->kind(node_index);

  switch (kind) {
  case Ast_type_function: {
    auto node = source->nodes->data(node_index).type_function;

    Assert(node.param_types.len() == 0); // TODO

    auto inst = code->add(
      Hir_type_function,
      {
        .type_function = {
          .param_count = Cast(u32, node.param_types.len()),
          .return_type = INVALID_HIR_INDEX,
        },
      }
    );

    code->data_ptr(inst)->type_function.return_type = generate_type(node.return_type);

    return inst;
  } break;
  case Ast_identifier: {
    Str s = source->get_token_str(source->nodes->data(node_index).identifier.token_index);
    StrKey key = strings->add(s);

    if (key == strings->add(Str_make("i32"))) {
      return code->add(Hir_type_int, { .type_int = { .signedness = Signed, .bitwidth = 32, }});
      break;
    }

    Todo();
  } break;
  default: Todo();
  }
}

b32 generate_hir(HirGeneratorContext *context, Source *source, HirCode *code) {
  HirGenerator gen;
  gen.init(context, source, code);

  auto root_node_index = source->root_node_index();
  Assert(source->nodes->kind(root_node_index) == Ast_root);

  b32 ok = gen.generate_from_root(root_node_index);

  gen.deinit();

  return ok;
}

static u32 log10(u32 n) {
  if (n >= 1000000000) return 9;
  if (n >= 100000000)  return 8;
  if (n >= 10000000)   return 7;
  if (n >= 1000000)    return 6;
  if (n >= 100000)     return 5;
  if (n >= 10000)      return 4;
  if (n >= 1000)       return 3;
  if (n >= 100)        return 2;
  if (n >= 10)         return 1;
  return 0;
}

void HirCode::print() {
  u32 width = 1 + log10(len());

  for (u32 i = 0; i < len(); i++) {
    auto inst = get({i});
    auto kind = *inst.kind;

    printf("%*d | %s", width, i, hir_kind_string(kind));

    switch (kind) {
    case Hir_type_int: printf(" %s%d", inst.data->type_int.signedness == Signed ? "i" : "u", inst.data->type_int.bitwidth); break;
    case Hir_literal_int: printf(" $%llu", inst.data->int64); break;
    case Hir_break: printf(" %%%d, %%%d", inst.data->break_.value.inner(), inst.data->break_.block.inner()); break;
    case Hir_condbr: {
      auto cond = inst.data->condbr.cond.inner();
      auto then = i + 1;
      auto otherwise = then + inst.data->condbr.then_instruction_count;
      printf(u8" %%%d, %%%d, %%%d", cond, then, otherwise); 
    } break;
    }

    printf("\n");
  }
}
