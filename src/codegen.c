#include "blu.h"
#include "value.h"
#include "tokens.h"
#include "messages.h"
#include "string_interner.h"
#include "source_file.h"
#include "compiler.h"
#include "codegen.h"
#include "ir.h"

#define SEGMENTLIST_NAME            DependencyList
#define SEGMENTLIST_TYPE            DeclarationIndex
#define SEGMENTLIST_FUNCTION_PREFIX depslist
#define SEGMENTLIST_LINKAGE         internal
#define SEGMENTLIST_MIN_SIZE_LOG2   8
#define SEGMENTLIST_SEGMENT_COUNT   24
#define SEGMENTLIST_OUTPUT_TYPES
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

typedef struct {
  b32      has_error;
  IrBuilder builder;
  DependencyList dependencies; // []DeclarationIndex
  ArenaSnapshot scope_scratch;

  Arena *arena;
  Arena *scratch;
  Common *common;
  MessageSink *msg_sink;
  StringInterner *strings;
  DeclarationInterner *decls;
  ValueStore *values;
  Source *source;

  // the module stack that the current declaration is in
  u32               mod_depth;
  DeclarationIndex *mods;
} CodeGen;

void codegen_init(CodeGen *gen, CodeGenContext *context, Source *source, u32 idx) {
  *gen = (CodeGen){
    .arena = context->arena,
    .scratch = context->scratch,
    .common = context->common,
    .strings = context->strings,
    .msg_sink = context->msg_sink,
    .decls = context->decls,
    .values = context->values,
    .source = source,
    .builder = (IrBuilder){ .scratch = context->scratch },
  };

  gen->scope_scratch = arena_scope_begin(context->scratch);

  DeclarationIndex *mods = arena_push_array(DeclarationIndex, context->scratch, Max_module_depth);
  u32 i = source->decls[source->tree_idxs[idx]].parent;
  u32 offset = 0;
  Assert(source->decls[0].kind == SourceDeclaration_root);
  while (i) {
    mods[offset++] = source->decl_idxs[i];
    i = source->decls[i].parent;
  }

  mods[offset++] = source->decl_idxs[0];

  gen->mod_depth = offset;
  gen->mods = mods;
}

void codegen_deinit(CodeGen *gen) {
  arena_scope_end(gen->scratch, gen->scope_scratch);
}


InstructionIndex inst_add_lookup(CodeGen *gen, TokenIndex name) {
  StringIndex str = strings_add(gen->strings, token_string(&gen->source->tokens, gen->source->text, name));

  DeclarationIndex decl;
  b32 found = lookup_identifier(gen->decls, gen->mods, gen->mod_depth, str, &decl);
  if (!found) {
    Message_error(
      gen->msg_sink,
      gen->source->idx,
      (MessageLocation){ .kind = MessageLocation_token_index, .data.token_index = name },
      string_lit("Could not find identifier.")
    );
    gen->has_error = True;
    decl = 0;
  }

  InstructionIndex lookup = inst_alloc(&gen->builder);
  inst_set_opcode(&gen->builder, lookup, IR_lookup);
  inst_set_data(&gen->builder, lookup, decl);

  depslist_append(&gen->dependencies, gen->scratch, decl);

  return lookup;
}

// TODO: renmae this as it is not an instruction :)
IrRef inst_add_const_type(CodeGen *gen, TypeIndex type) {
  Value *v;
  ValueIndex idx = values_alloc(gen->values, &v);
  TypeIndex *data = values_alloc_data(gen->values, sizeof(TypeIndex), Align_of(TypeIndex));
  *data = type;
  *v = (Value){
    .type = gen->common->type.type,
    .data_size = sizeof(TypeIndex),
    .data = data,
  };

  return ir_ref_from_value_index(idx);
}

IrRef gen_code(CodeGen *gen, AstIndex idx_ast, IrRef type_destination) {
  String    text   = gen->source->text;
  Tokens   *tokens = &gen->source->tokens;
  AstNodes *ast    = &gen->source->ast;

  IrRef res = 0;

  u8 kind = ast->kinds[idx_ast];
  switch (Cast(enum AstKind, kind)) {
  case Ast_identifier: {
    TokenIndex *name = ast_data(ast, idx_ast);
    res = ir_ref_from_instruction_index(inst_add_lookup(gen, *name));
  } break;
  case Ast_type_function: {
    AstTypeFunction *func = ast_data(ast, idx_ast);
    IrRef ref_ret_type = gen_code(gen, func->return_type, ir_ref_from_value_index(gen->common->val.type));

    for (u32 i = 0; i < func->count; i++) {
      // TODO Add the parameter types
      Panic();
    }

    InstructionIndex inst_type = inst_alloc(&gen->builder);

    inst_set_opcode(&gen->builder, inst_type, IR_type);

    u32 arg_count = func->count + 1; // parameters + return type

    IrType *data_type = inst_push_data_raw(&gen->builder, inst_type, sizeof(IrType) + arg_count * sizeof(IrRef), Align_of(IrType));
    *data_type = (IrType){
      .kind = Type_function,
      .arg_count = arg_count,
    };

    data_type->args[0] = ref_ret_type;

    res = ir_ref_from_instruction_index(inst_type);
  } break;
  case Ast_function: {
    AstFunction *func = ast_data(ast, idx_ast);

    InstructionIndex inst_func = inst_alloc(&gen->builder);
    inst_set_opcode(&gen->builder, inst_func, IR_func);

    IrFunc *data_func = inst_push_data(&gen->builder, inst_func, IrFunc);

    IrRef ref_ret_type = 0;
    if (func->return_type) {
      ref_ret_type = gen_code(gen, func->return_type, ir_ref_from_value_index(gen->common->val.type));
    }

    u32 arg_offset = inst_offset(&gen->builder, inst_func);

    for (u32 i = 0; i < func->count; i++) {
      Panic();
    }

    InstructionIndex inst_type = inst_alloc(&gen->builder);
    inst_set_opcode(&gen->builder, inst_type, IR_type);
    u32 arg_count = func->count + 1; // parameters + return type

    IrType *data_type = inst_push_data_raw(&gen->builder, inst_type, sizeof(IrType) + arg_count * sizeof(IrRef), Align_of(IrType));
    *data_type = (IrType){
      .kind = Type_function,
      .arg_count = arg_count,
    };

    data_type->args[0] = ref_ret_type;

    InstructionIndex inst_unify = inst_alloc(&gen->builder);
    inst_set_opcode(&gen->builder, inst_unify, IR_unify);
    IrUnify *data_unify = inst_push_data(&gen->builder, inst_unify, IrUnify);
    *data_unify = (IrUnify){
      .type_lhs = type_destination,
      .type_rhs = ir_ref_from_instruction_index(inst_type),
    };

    InstructionIndex inst_return_type = inst_alloc(&gen->builder);
    inst_set_opcode(&gen->builder, inst_return_type, IR_function_return_type);
    inst_set_data(&gen->builder, inst_return_type, ir_ref_from_instruction_index(inst_unify));

    IrRef inst_body = gen_code(gen, func->body, ir_ref_from_instruction_index(inst_return_type));

    InstructionIndex inst_ret = inst_alloc(&gen->builder);
    inst_set_opcode(&gen->builder, inst_ret, IR_ret);
    inst_set_data(&gen->builder, inst_ret, inst_body);

    u32 func_instruction_count = inst_offset(&gen->builder, inst_func);

    *data_func = (IrFunc){
      .return_type = ref_ret_type,
      .arg_offset = arg_offset,
      .arg_count = func->count,
      .instruction_count = func_instruction_count,
    };

    return ir_ref_from_instruction_index(inst_func);
  } break;
  case Ast_declaration: {
    AstDeclaration *decl = ast_data(ast, idx_ast);

    InstructionIndex idx_decl = inst_alloc(&gen->builder);

    inst_set_opcode(&gen->builder, idx_decl, IR_declaration);

    IrDeclaration *data_decl = inst_push_data(&gen->builder, idx_decl, IrDeclaration);

    IrRef ref_type = ir_ref_from_value_index(gen->common->val.type);
    IrRef ref_decl_type = 0;
    if (decl->type) {
      InstructionIndex block = inst_block_begin(&gen->builder);
      IrRef type = gen_code(gen, decl->type, ref_type);
      inst_block_end(&gen->builder, block, type);
      ref_decl_type = ir_ref_from_instruction_index(block);
    }
    IrRef ref_decl_val = gen_code(gen, decl->value, ref_decl_type);

    data_decl->declared_type = ref_decl_type;
    data_decl->value = ref_decl_val;

    return ir_ref_from_instruction_index(idx_decl);
  } break;
  case Ast_literal_int: {
    TokenIndex *tok = ast_data(ast, idx_ast);
    i64 value = parse_i64(token_string(tokens, text, *tok));

    Value *v;
    ValueIndex idx = values_alloc(gen->values, &v);
    i64 *data = values_alloc_data(gen->values, sizeof(i64), Align_of(i64));
    *data = value;
    *v = (Value){
      .type      = gen->common->type.comptime_int,
      .data_size = sizeof(i64),
      .data      = data,
    };

    IrRef ref_literal = ir_ref_from_value_index(idx);

    res = ref_literal;
  } break;
  default: Panic();
  }

  if (type_destination) {
    return ir_ref_from_instruction_index(inst_as(&gen->builder, type_destination, res));
  }

  return res;
}

b32 source_generate_code(CodeGenContext *context, Source *source, u32 idx) {
  CodeGen gen;
  codegen_init(&gen, context, source, idx);

  gen_code(&gen, source->decls[source->tree_idxs[idx]].node, ir_ref_from_value_index(context->common->val.nil));

  irbuilder_flatten(&gen.builder, context->arena, &source->ir_chunks[idx]);

  // TODO
  // - output all the dependencies somewhere.

  codegen_deinit(&gen);

  return !gen.has_error;
}
