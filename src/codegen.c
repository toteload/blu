#include "blu.h"
#include "value.h"
#include "tokens.h"
#include "messages.h"
#include "string_interner.h"
#include "source_file.h"
#include "compiler.h"
#include "codegen.h"
#include "ir.h"

typedef struct {
  b32 has_error;
  IrBuilder builder;
  ArenaSnapshot scope_scratch;

  Arena *perm;
  Arena *scratch;

  Common *common;
  MessageSink *msg_sink;
  StringInterner *strings;
  DeclarationInterner *decls;
  ValueStore *values;

  Source *source;

  // the module stack that the current declaration is in
  u32 mod_depth;
  DeclarationIndex *mods;
} CodeGen;

void codegen_init(CodeGen *gen, CodeGenContext *context, Source *source, u32 tree_idx) {
  *gen = (CodeGen){
    .perm = context->perm,
    .scratch = context->scratch,
    .common = context->common,
    .strings = context->strings,
    .msg_sink = context->msg_sink,
    .decls = context->decls,
    .values = context->values,
    .source = source,
    .builder = (IrBuilder){.scratch = context->scratch},
  };

  gen->scope_scratch = arena_scope_begin(context->scratch);

  DeclarationIndex *mods = arena_push_array(DeclarationIndex, context->scratch, Max_module_depth);
  u32 i = source->decls[tree_idx].parent;
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

void codegen_deinit(CodeGen *gen) { arena_scope_end(gen->scratch, gen->scope_scratch); }

internal InstructionIndex inst_add_lookup_value(CodeGen *gen, TokenIndex name, AstIndex source) {
  StringIndex str =
    strings_add(gen->strings, token_string(&gen->source->tokens, gen->source->text, name));

  DeclarationIndex decl;
  b32 found = lookup_identifier(gen->decls, gen->mods, gen->mod_depth, str, &decl);
  if (!found) {
    Message_error(
      gen->msg_sink,
      (MessageLocation){
        .kind = MessageLocation_token_index,
        .source_idx = gen->source->idx,
        .data.token_index = name,
      },
      string_lit("Could not find identifier.")
    );
    gen->has_error = True;
    decl = 0;
  }

  InstructionIndex lookup = inst_alloc(&gen->builder);
  inst_set_opcode(&gen->builder, lookup, IR_lookup_value);
  inst_set_ast_source(&gen->builder, lookup, source);
  inst_set_data(&gen->builder, lookup, decl);

  return lookup;
}

internal InstructionIndex inst_add_lookup_value_typeof(CodeGen *gen, TokenIndex name, AstIndex source) {
  StringIndex str =
    strings_add(gen->strings, token_string(&gen->source->tokens, gen->source->text, name));

  DeclarationIndex decl;
  b32 found = lookup_identifier(gen->decls, gen->mods, gen->mod_depth, str, &decl);
  if (!found) {
    Message_error(
      gen->msg_sink,
      (MessageLocation){
        .kind = MessageLocation_token_index,
        .source_idx = gen->source->idx,
        .data.token_index = name,
      },
      string_lit("Could not find identifier.")
    );
    gen->has_error = True;
    decl = 0;
  }

  InstructionIndex lookup = inst_alloc(&gen->builder);
  inst_set_opcode(&gen->builder, lookup, IR_lookup_typeof);
  inst_set_ast_source(&gen->builder, lookup, source);
  inst_set_data(&gen->builder, lookup, decl);

  return lookup;
}

IrRef gen_code(CodeGen *gen, AstIndex idx_ast, IrRef type_destination) {
  String text = gen->source->text;
  Tokens *tokens = &gen->source->tokens;
  AstNodes *ast = &gen->source->ast;
  IrBuilder *builder = &gen->builder;

  IrRef res = {0};

  u8 kind = ast->kinds[idx_ast];
  switch (Cast(AstKind, kind)) {
  case Ast_identifier: {
    TokenIndex *name = ast_data(ast, idx_ast);
    res = ir_ref_from_instruction_index(inst_add_lookup_value(gen, *name, idx_ast));
  } break;
  case Ast_block: {
    AstBlock *block = ast_data(ast, idx_ast);
    u32 count = block->count;
    if (count == 0) {
      Todo();
    }

    for (u32 i = 0; i < count-1; i++) {
      gen_code(gen, block->items[i], (IrRef){0});
    }

    res = gen_code(gen, block->items[count-1], type_destination);
  } break;
  case Ast_if_else: {
    AstIfElse *if_else = ast_data(ast, idx_ast);

    InstructionIndex block = inst_block_begin(builder);

    InstructionIndex cond_block = inst_block_begin(builder);
    IrRef cond_val = gen_code(gen, if_else->cond, ir_ref_from_value_index(gen->common->val.bool));
    inst_block_end_with_value(builder, cond_block, cond_val);
    IrRef cond = ir_ref_from_instruction_index(cond_block);

    InstructionIndex condbr = inst_alloc(builder);
    inst_set_opcode(builder, condbr, IR_condbr);

    InstructionIndex then_block = inst_block_begin(builder);
    IrRef then_val = gen_code(gen, if_else->then, type_destination);
    inst_block_end_with_value_and_target(builder, then_block, block, then_val);

    IrCondBr *data_condbr = inst_push_data(builder, condbr, IrCondBr);

    if (if_else->otherwise) {
      InstructionIndex else_block = inst_block_begin(builder);
      IrRef else_val = gen_code(gen, if_else->otherwise, type_destination);
      inst_block_end_with_value_and_target(builder, else_block, block, else_val);

      *data_condbr = (IrCondBr){
        .cond = cond,
        .then = then_block,
        .otherwise = else_block,
      };
    } else {
      Todo();
    }

    inst_block_end(builder, block);

    res = ir_ref_from_instruction_index(block);
  } break;
  case Ast_type_function: {
    AstTypeFunction *func = ast_data(ast, idx_ast);
    IrRef ref_ret_type =
      gen_code(gen, func->return_type, ir_ref_from_value_index(gen->common->val.type));

    for (u32 i = 0; i < func->count; i++) {
      // TODO Add the parameter types
      Panic();
    }

    InstructionIndex inst_type = inst_alloc(&gen->builder);

    inst_set_opcode(&gen->builder, inst_type, IR_type);
    inst_set_ast_source(&gen->builder, inst_type, idx_ast);

    u32 arg_count = func->count + 1; // parameters + return type

    IrType *data_type = inst_push_data_raw(
      &gen->builder,
      inst_type,
      sizeof(IrType) + arg_count * sizeof(IrRef),
      Align_of(IrType)
    );
    *data_type = (IrType){
      .kind = Type_function,
      .arg_count = arg_count,
    };

    data_type->args[0] = ref_ret_type;

    res = ir_ref_from_instruction_index(inst_type);
  } break;
  case Ast_function: {
    AstFunction *func = ast_data(ast, idx_ast);

    InstructionIndex inst_return_type = inst_alloc(builder);
    inst_set_opcode(builder, inst_return_type, IR_return_type);
    inst_set_data(builder, inst_return_type, ref_to_u32(type_destination));

    for (u32 i = 0; i < func->count; i++) {
      InstructionIndex inst_param_type = inst_alloc(builder);
      inst_set_opcode(builder, inst_param_type, IR_param_type);
      IrParamType *param_type = inst_push_data(builder, inst_param_type, IrParamType);
      *param_type = (IrParamType){
        .function = type_destination,
        .param_index = i,
      };
    }

    InstructionIndex inst_func = inst_alloc(builder);
    inst_set_opcode(builder, inst_func, IR_func);
    inst_set_ast_source(builder, inst_func, idx_ast);
    IrFunc *data_func = inst_push_data(builder, inst_func, IrFunc);

    InstructionIndex first_param_type = inst_return_type + 1;

    for (u32 i = 0; i < func->count; i++) {
      InstructionIndex inst_param = inst_alloc(builder);
      inst_set_opcode(builder, inst_param, IR_param);
      inst_set_ast_source(builder, inst_param, func->params[i]);
      inst_set_data(builder, inst_param, ref_to_u32(ir_ref_from_instruction_index(first_param_type + i)));
    }

    IrRef inst_body = gen_code(gen, func->body, ir_ref_from_instruction_index(inst_return_type));

    InstructionIndex inst_ret = inst_alloc(&gen->builder);
    inst_set_opcode(&gen->builder, inst_ret, IR_ret);
    inst_set_data(&gen->builder, inst_ret, ref_to_u32(inst_body));

    u32 func_instruction_count = inst_offset(&gen->builder, inst_func);

    *data_func = (IrFunc){
      .param_count = func->count,
      .instruction_count = func_instruction_count,
    };

    return ir_ref_from_instruction_index(inst_func);
  } break;
  case Ast_declaration: {
    AstDeclaration *decl = ast_data(ast, idx_ast);

    InstructionIndex block = inst_block_begin(builder);
    inst_set_ast_source(builder, block, idx_ast);

    IrRef ref_type = ir_ref_from_value_index(gen->common->val.type);

    IrRef ref_decl_type = {0};
    if (decl->type) {
      ref_decl_type = gen_code(gen, decl->type, ref_type);
    }
    IrRef ref_decl_val = gen_code(gen, decl->value, ref_decl_type);
    InstructionIndex br = inst_alloc(builder);
      inst_set_opcode(builder, br, IR_br);
      IrBr *data = inst_push_data(builder, br, IrBr);
      *data = (IrBr){
        .block = block,
        .value = ref_decl_val,
      };
    inst_block_end(builder, block);

    return ir_ref_from_instruction_index(block);
  } break;
  case Ast_literal_int: {
    TokenIndex *tok = ast_data(ast, idx_ast);
    i64 value = parse_i64(token_string(tokens, text, *tok));

    Value *v;
    ValueIndex idx = values_alloc(gen->values, &v);
    ComptimeInt *data = values_alloc_data(gen->values, sizeof(ComptimeInt), Align_of(ComptimeInt));
    *data = value;
    *v = (Value){
      .type = gen->common->type.comptime_int,
      .data_size = sizeof(ComptimeInt),
      .data = data,
    };

    IrRef ref_literal = ir_ref_from_value_index(idx);

    res = ref_literal;
  } break;
  case Ast_call: {
    AstCall *ast_call = ast_data(ast, idx_ast);

    InstructionIndex inst_type = inst_alloc(builder);
    inst_set_opcode(builder, inst_type, IR_type);
    u32 arg_count = ast_call->count + 1;
    IrType *data_type = inst_push_data_raw(builder, inst_type, sizeof(IrType) + arg_count * sizeof(IrRef), Align_of(IrType));
    *data_type = (IrType){
      .kind = Type_function,
      .arg_count = arg_count,
    };
    data_type->args[0] = type_destination;

    // Add parameter types
    for (u32 i = 0; i < ast_call->count; i++) {
      Todo(); // :) haha you still have to implement this
    }

    // TODO as a type destination you could at least set it to a function that must return a type
    // of the current type destination. and you also know with how many arguments it is called.
    // could that be useful in any way?
    IrRef callee = gen_code(gen, ast_call->callee, ir_ref_from_instruction_index(inst_type));

    // Evaluate all the arguments
    for (u32 i = 0; i < ast_call->count; i++) {
      Todo(); // :) haha you still have to implement this
    }

    InstructionIndex inst_call = inst_alloc(builder);
    inst_set_opcode(builder, inst_call, IR_call);
    inst_set_ast_source(builder, inst_call, idx_ast);

    IrCall *call = inst_push_data(builder, inst_call, IrCall);
    *call = (IrCall){
      .func = callee,
      .arg_count = ast_call->count,
    };

    Assert(ast_call->count == 0); // add the args to IR
    res = ir_ref_from_instruction_index(inst_call);
  } break;
  default:
    Todo();
  }

  if (!ref_is_nil(type_destination)) {
    return ir_ref_from_instruction_index(inst_as(&gen->builder, type_destination, res, idx_ast));
  }

  return res;
}

// For some constructions a part of the declared type is allowed to live in the value.
// This function is for adding type checking code for those constructs.
IrRef gen_declaration_type_of_val_code(CodeGen *gen, AstIndex idx_ast, IrRef declared_type) {
  AstNodes *ast = &gen->source->ast;
  IrBuilder *builder = &gen->builder;

  u8 kind = ast->kinds[idx_ast];
  switch (Cast(AstKind, kind)) {
  case Ast_identifier: {
    return declared_type;
  } break;
  case Ast_function: {
    AstFunction *func = ast_data(ast, idx_ast);

    IrRef ref_ret_type = {0};
    if (func->return_type) {
      ref_ret_type =
        gen_code(gen, func->return_type, ir_ref_from_value_index(gen->common->val.type));
    }

    // Output param type expressions
    for (u32 i = 0; i < func->count; i++) {
      Todo();
    }

    InstructionIndex inst_type = inst_alloc(builder);
    inst_set_opcode(builder, inst_type, IR_type);
    inst_set_ast_source(builder, inst_type, idx_ast);

    u32 arg_count = func->count + 1; // parameters + return type

    Assert(func->count == 0);

    IrType *data_type = inst_push_data_raw(
      &gen->builder,
      inst_type,
      sizeof(IrType) + arg_count * sizeof(IrRef),
      Align_of(IrType)
    );
    *data_type = (IrType){
      .kind = Type_function,
      .arg_count = arg_count,
    };

    data_type->args[0] = ref_ret_type;

    InstructionIndex inst_unify = inst_alloc(builder);
    inst_set_opcode(builder, inst_unify, IR_unify);
    IrUnify *data_unify = inst_push_data(builder, inst_unify, IrUnify);
    *data_unify = (IrUnify){
      .type_lhs = declared_type,
      .type_rhs = ir_ref_from_instruction_index(inst_type),
    };

    return ir_ref_from_instruction_index(inst_unify);
  } break;
  default:
    Todo();
  }

  Todo();

  return (IrRef){0};
}

b32 generate_code(CodeGenContext *context, Declaration *decl) {
  // NOTE: It is possible to output dependencies on other declarations for the pieces of code.
  // However, these dependencies may contain false positives, because whether another declaration is
  // actually used can depend on running a piece of comptime code.
  // For example, `a := if buzz() { foo() } else { bar() }`.
  // Whether `a` will use foo or bar depends on buzz(). But we can output both foo and bar and
  // accept that one of them will be a false positive. This may still be useful in sorting jobs.

  Source *source = decl->data.decl.source;

  CodeGen gen;
  codegen_init(&gen, context, source, decl->data.decl.tree_idx);

  AstIndex ast_idx_decl = source->decls[decl->data.decl.tree_idx].node;
  AstDeclaration *ast_decl = ast_data(&source->ast, ast_idx_decl);

  Assert(gen.source->ast.kinds[ast_idx_decl] == Ast_declaration);

  IrBuilder *builder = &gen.builder;

  InstructionIndex block = inst_alloc(builder);
  inst_set_opcode(builder, block, IR_eval_block);
  inst_set_ast_source(builder, block, ast_idx_decl);

  IrRef ref_decl_type = {0};
  if (ast_decl->type) {
    IrRef ref_type = ir_ref_from_value_index(gen.common->val.type);
    ref_decl_type = gen_code(&gen, ast_decl->type, ref_type);
  }

  IrRef ref_decl_type_of_val =
    gen_declaration_type_of_val_code(&gen, ast_decl->value, ref_decl_type);

  u32 typecheck_end = inst_offset(builder, block);

  decl->data.decl.typecheck_end = typecheck_end;

  IrRef ref_decl_val = gen_code(&gen, ast_decl->value, ref_decl_type_of_val);

  IrRef res =
    ir_ref_from_instruction_index(inst_as(builder, ref_decl_type_of_val, ref_decl_val, ast_decl->value));

  InstructionIndex br = inst_alloc(builder);
  inst_set_opcode(builder, br, IR_br);
  IrBr *data = inst_push_data(builder, br, IrBr);
  *data = (IrBr){
    .block = block,
    .value = res,
  };

  inst_block_end(builder, block);

  irbuilder_flatten(builder, context->perm, &decl->data.decl.chunk);

  codegen_deinit(&gen);

  return !gen.has_error;
}
