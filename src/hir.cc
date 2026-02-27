#include "blu.hh"

struct HirGenerator {
  MessageManager *messages = nullptr;
  Source *source = nullptr;
  StringInterner *strings = nullptr;
  HirCode *code = nullptr;

  void init(HirGeneratorContext *context, Source *source, HirCode *code);
  void deinit() {}

  b32 generate_from_root(NodeIndex root);
  b32 generate_function(NodeIndex node_index, HirIndex *out);
  b32 generate_block(NodeIndex node_index, HirIndex *out);
  b32 generate_expression(NodeIndex node_index, HirIndex *out);
  b32 generate_type(NodeIndex node_index, HirIndex *out);
};

void HirGenerator::init(HirGeneratorContext *context, Source *source, HirCode *code) {
  messages = context->messages;
  strings = context->strings;

  this->source = source;
  this->code = code;
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
        .type = INVALID_HIR_INDEX, 
        .value = INVALID_HIR_INDEX, }, });

    HirIndex type;
    Try(generate_type(ast_decl.type, &type));

    HirIndex value;
    Try(generate_expression(ast_decl.value, &value));

    *code->data_ptr(decl) = { .declaration = {
      .type = type,
      .value = value,
    }};
  }

  return true;
}

b32 HirGenerator::generate_function(NodeIndex node_index, HirIndex *out) {
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

  HirIndex body;
  Try(generate_block(function.body, &body));

  auto function_idx_end = code->add(Hir_return, { .idx = body, });

  u32 instruction_count = function_idx_end.inner() - function_idx.inner() + 1;
  code->data_ptr(function_idx)->function.instruction_count = instruction_count;

  *out = function_idx;

  return true;
}

b32 HirGenerator::generate_block(NodeIndex block_node_index, HirIndex *out) {
  auto block = source->nodes->data(block_node_index).block;

  HirIndex block_idx = code->add(
    Hir_block,
    { .block = { .instruction_count = UINT32_MAX, }, }
  );

  // For now only allow non-empty blocks.
  Assert(block.items.len() > 0);

  for (usize i = 0; i < block.items.len() - 1; i += 1) {
    HirIndex e;
    Try(generate_expression(block.items[i], &e));
  }

  HirIndex last_expression;
  if (block.items.len() > 0) {
    Try(generate_expression(block.items[block.items.len()-1], &last_expression));
  }

  HirIndex break_idx = code->add(
    Hir_break,
    { .break_ = { .value = last_expression, .block = block_idx, }, }
  );

  u32 instruction_count = break_idx.inner() - block_idx.inner() + 1;
  code->data_ptr(block_idx)->block.instruction_count = instruction_count;

  *out = block_idx;

  return true;
}

i64 parse_integer(Str s) {
  // TODO
  return 0;
}

b32 HirGenerator::generate_expression(NodeIndex node_index, HirIndex *out) {
  auto kind = source->nodes->kind(node_index);

  switch (kind) {
    case Ast_function: { Try(generate_function(node_index, out)); } break;
    case Ast_if_else: {
      auto if_else = source->nodes->data(node_index).if_else;
      HirIndex cond;
      Try(generate_expression(if_else.cond, &cond));

      auto extra_index = code->alloc_extra(2);

      auto hir_condbr = code->add(Hir_condbr, { .condbr = { cond, extra_index, }, });
      auto hir_block = code->add(Hir_block, {});

      // TODO

    } break;
    case Ast_literal_int: {
      TokenIndex tok = source->nodes->data(node_index).literal_int.token_index;
      Str literal = source->get_token_str(tok);
      i64 val = parse_integer(literal);

      *out = code->add(Hir_literal_int, { .int64 = val, });
    } break;
    default: Todo();
  }

  return true;
}

b32 HirGenerator::generate_type(NodeIndex node_index, HirIndex *out) {
  AstKind kind = source->nodes->kind(node_index);

  switch (kind) {
  Ast_type_function: {
  } break;
  Ast_identifier: {
    Str s = source->get_token_str(source->nodes->data(node_index).identifier.token_index);
    StrKey key = strings->add(s);

    if (key == strings->add(Str_make("i32"))) {
      *out = code->add(Hir_type_int, { .type_int = { .signedness = Signed, .bitwidth = 32, }});
      break;
    }

    Todo();
  } break;
  }

  return true;
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
