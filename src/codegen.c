#include "blu.h"
#include "value.h"
#include "tokens.h"
#include "messages.h"
#include "string_interner.h"
#include "source_file.h"
#include "compiler.h"
#include "codegen.h"
#include "ir.h"

// https://github.com/skeeto/hash-prospector/issues/19
// score: 0.10734781817103507
internal u32 lowbias32(u32 x) {
    x ^= x >> 16;
    x *= 0x21f0aaad;
    x ^= x >> 15;
    x *= 0xf35a2d97;
    x ^= x >> 15;

    return x;
}

internal u32 hash_string_index(void *context, StringIndex s) {
  Unused(context);
  return lowbias32(s);
}

internal b32 cmp_string_index(void *context, StringIndex a, StringIndex b) {
  Unused(context);
  return a == b;
}

#define HASHMAP_NAME       LocalsMap
#define HASHMAP_KEY_TYPE   StringIndex
#define HASHMAP_VALUE_TYPE InstructionIndex
#define HASHMAP_FUNCTION_PREFIX locals
#define HASHMAP_HASH_FN hash_string_index
#define HASHMAP_KEY_COMPARE_FN cmp_string_index
#define HASHMAP_OUTPUT_TYPES
#define HASHMAP_OUTPUT_DEFINITIONS
#include "hashmap.h"

#define Max_scope_entries 256

typedef enum {
  ScopeEntry_local,
  ScopeEntry_block,
} ScopeEntryKind;

typedef struct {
  u8 kind;
  StringIndex name;
} ScopeEntry;

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
  TypeInterner *types;

  Source *source;

  LocalsMap locals;

  Stack(ScopeEntry) scope_stack;

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
    .types = context->types,
    .source = source,
    .builder = (IrBuilder){.scratch = context->scratch},
  };

  locals_init(&gen->locals, &(HashMapOptions){
    .allocator = context->gpa,
    .initial_size = 8,
  });

  gen->scope_scratch = arena_scope_begin(context->scratch);

  stack_init(&gen->scope_stack, arena_push_array(ScopeEntry, context->scratch, Max_scope_entries), Max_scope_entries);

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

void codegen_deinit(CodeGen *gen) {
  locals_deinit(&gen->locals);
  arena_scope_end(gen->scratch, gen->scope_scratch);
}

internal InstructionIndex lookup(CodeGen *gen, StringIndex str, AstIndex ast_idx) {
  InstructionIndex *inst = locals_find(&gen->locals, str); 
  if (inst) {
    return *inst;
  }

  DeclarationIndex decl = 0;
  b32 found = lookup_identifier(gen->decls, gen->mods, gen->mod_depth, str, &decl);
  if (!found) {
    Message_error(
      gen->msg_sink,
      (MessageLocation){
        .kind = MessageLocation_token_index,
        .source_idx = gen->source->idx,
        .data.token_index = str,
      },
      string_lit("Could not find identifier.")
    );
    gen->has_error = True;
  }

  InstructionIndex lookup = inst_alloc(&gen->builder);
  inst_set_opcode(&gen->builder, lookup, IR_lookup_value);
  inst_set_source(&gen->builder, lookup, gen->source->idx, ast_idx);
  inst_set_data(&gen->builder, lookup, decl);

  return lookup;
}

IrRef gen_code_for_ptr(CodeGen *gen, AstIndex idx_ast, IrRef type_destination) {
  AstNodes *ast = &gen->source->ast;
  u8 kind = ast->kinds[idx_ast];

  SourceIndex source_idx = gen->source->idx;

  switch (Cast(AstKind, kind)) {
  case Ast_identifier: {
    TokenIndex *name = ast_data(ast, idx_ast);
    StringIndex str = strings_add(gen->strings, token_string(&gen->source->tokens, gen->source->text, *name));

    InstructionIndex ptr = lookup(gen, str, idx_ast);

    if (ref_is_nil(type_destination)) {
      return ir_ref_from_instruction_index(ptr);
    }

    return ir_ref_from_instruction_index(inst_as(&gen->builder, type_destination, ir_ref_from_instruction_index(ptr), source_idx, idx_ast));
  } break;
  default: Todo();
  }

  Todo();
}

IrRef gen_code(CodeGen *gen, AstIndex idx_ast, IrRef type_destination) {
  String text = gen->source->text;
  Tokens *tokens = &gen->source->tokens;
  AstNodes *ast = &gen->source->ast;
  IrBuilder *builder = &gen->builder;

  SourceIndex source_idx = gen->source->idx;

  u8 kind = ast->kinds[idx_ast];
  switch (Cast(AstKind, kind)) {
  case Ast_identifier: {
    TokenIndex *name = ast_data(ast, idx_ast);
    StringIndex str = strings_add(gen->strings, token_string(&gen->source->tokens, gen->source->text, *name));

    InstructionIndex ptr = lookup(gen, str, idx_ast);

    InstructionIndex inst_load = inst_alloc(builder);
    inst_set_opcode(builder, inst_load, IR_load);
    inst_set_data(builder, inst_load, ref_to_u32(ir_ref_from_instruction_index(ptr)));

    if (ref_is_nil(type_destination)) {
      return ir_ref_from_instruction_index(inst_load);
    }

    return ir_ref_from_instruction_index(inst_as(&gen->builder, type_destination, ir_ref_from_instruction_index(inst_load), source_idx, idx_ast));
  } break;
  case Ast_block: {
    AstBlock *block = ast_data(ast, idx_ast);
    u32 count = block->count;
    if (count == 0) {
      return ir_ref_from_value_index(gen->common->val.nil);
    }

    InstructionIndex inst_block = inst_block_begin(builder);

    stack_push(&gen->scope_stack, (ScopeEntry){ .kind = ScopeEntry_block });

    for (u32 i = 0; i < count-1; i++) {
      gen_code(gen, block->items[i], (IrRef){0});
    }

    IrRef res = gen_code(gen, block->items[count-1], type_destination);

    inst_block_end_with_value(builder, inst_block, res);

    while (True) {
      ScopeEntry entry = stack_pop(&gen->scope_stack);

      if (entry.kind == ScopeEntry_block) {
        break;
      }

      b32 found = locals_remove(&gen->locals, entry.name);
      Assert(found);
    }

    return ir_ref_from_instruction_index(inst_block);
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

    return ir_ref_from_instruction_index(block);
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
    inst_set_source(&gen->builder, inst_type, source_idx, idx_ast);

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

    IrRef res = ir_ref_from_instruction_index(inst_type);
    return ir_ref_from_instruction_index(inst_as(&gen->builder, type_destination, res, source_idx, idx_ast));
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
    inst_set_source(builder, inst_func, source_idx, idx_ast);
    IrFunc *data_func = inst_push_data(builder, inst_func, IrFunc);

    InstructionIndex first_param_type = inst_return_type + 1;

    for (u32 i = 0; i < func->count; i++) {
      InstructionIndex inst_param = inst_alloc(builder);
      inst_set_opcode(builder, inst_param, IR_param);
      inst_set_source(builder, inst_param, source_idx, func->params[i]);
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
      .return_type = ir_ref_from_instruction_index(inst_return_type),
    };

    return ir_ref_from_instruction_index(inst_func);
  } break;
  case Ast_declaration: {
    AstDeclaration *decl = ast_data(ast, idx_ast);

    Assert(decl->type);

    StringIndex str = strings_add(gen->strings, token_string(&gen->source->tokens, gen->source->text, decl->name));

    IrRef ref_type = ir_ref_from_value_index(gen->common->val.type);

    IrRef ref_decl_type = gen_code(gen, decl->type, ref_type);

    InstructionIndex inst_var = inst_alloc(builder);
    inst_set_opcode(builder, inst_var, IR_alloc);
    inst_set_data(builder, inst_var, ref_to_u32(ref_decl_type));

    stack_push(&gen->scope_stack, ((ScopeEntry){ .kind = ScopeEntry_local, .name = str }));
    locals_insert(&gen->locals, str, inst_var);

    IrRef ref_decl_val = gen_code(gen, decl->value, ref_decl_type);

    InstructionIndex inst_store = inst_alloc(builder);
    inst_set_opcode(builder, inst_store, IR_store);
    IrStore *store = inst_push_data(builder, inst_store, IrStore);
    *store = (IrStore){
      .dst = ir_ref_from_instruction_index(inst_var),
      .value = ref_decl_val,
    };

    return ir_ref_from_value_index(gen->common->val.nil);
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

    return ir_ref_from_instruction_index(inst_as(&gen->builder, type_destination, ref_literal, source_idx, idx_ast));
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
    inst_set_source(builder, inst_call, source_idx, idx_ast);

    IrCall *call = inst_push_data(builder, inst_call, IrCall);
    *call = (IrCall){
      .func = callee,
      .arg_count = ast_call->count,
    };

    Assert(ast_call->count == 0); // TODO: add the args to IR

    IrRef res = ir_ref_from_instruction_index(inst_call);
    return ir_ref_from_instruction_index(inst_as(&gen->builder, type_destination, res, source_idx, idx_ast));
  } break;
  case Ast_builtin: {
    AstBuiltin *builtin = ast_data(ast, idx_ast);
    switch (Cast(BuiltinKind, builtin->kind)) {
    case Builtin_debug: {
      AstIndex e = builtin->args[0];
      IrRef val = gen_code(gen, e, type_destination);

      InstructionIndex inst_debug = inst_alloc(builder);
      inst_set_opcode(builder, inst_debug, IR_builtin_debug);
      inst_set_data(builder, inst_debug, ref_to_u32(val));

      return ir_ref_from_instruction_index(inst_debug);
    } break;
    }
  } break;
  case Ast_literal_string: {
    TokenIndex *name = ast_data(ast, idx_ast);
    
    String s = token_string(&gen->source->tokens, gen->source->text, *name);

    void *data = values_alloc_data(gen->values, s.len, 1);
    u32 len;

    u32 err = decode_string_literal(s, data, &len);
    if (err) {
      Todo();
    }

    TypeIndex type = types_add(gen->types, &(Type){
      .kind = Type_array,
      .data.array = {
        .base_type = gen->common->type.u8,
        .size = len,
      },
    });

    Value *v;
    ValueIndex idx = values_alloc(gen->values, &v);
    *v = (Value){
      .type = type,
      .data = data,
      .data_size = s.len,
    };

    IrRef ref_literal = ir_ref_from_value_index(idx);

    return ir_ref_from_instruction_index(inst_as(&gen->builder, type_destination, ref_literal, source_idx, idx_ast));
  } break;
  case Ast_source: { Todo(); } break;
  case Ast_mod_section: { Todo(); } break;
  case Ast_type_slice: { Todo(); } break;
  case Ast_type_array: { Todo(); } break;
  case Ast_assign: { 
    AstAssign *assign = ast_data(ast, idx_ast);

    Assert(assign->kind == Assign_normal);

    IrRef lhs_ptr = gen_code_for_ptr(gen, assign->lhs, (IrRef){0});

    InstructionIndex inst_typeof_lhs = inst_alloc(builder);
    inst_set_opcode(builder, inst_typeof_lhs, IR_typeof);
    inst_set_data(builder, inst_typeof_lhs, ref_to_u32(lhs_ptr));

    InstructionIndex inst_basetype = inst_alloc(builder);
    inst_set_opcode(builder, inst_basetype, IR_base_type);
    inst_set_data(builder, inst_basetype, ref_to_u32(ir_ref_from_instruction_index(inst_typeof_lhs)));

    IrRef value = gen_code(gen, assign->value, ir_ref_from_instruction_index(inst_basetype));

    InstructionIndex inst_store = inst_alloc(builder);
    inst_set_opcode(builder, inst_store, IR_store);
    IrStore *store = inst_push_data(builder, inst_store, IrStore);
    *store = (IrStore){
      .dst = lhs_ptr,
      .value = value,
    };
    
    return ir_ref_from_instruction_index(inst_store);
  } break;
  case Ast_label: { Todo(); } break;
  case Ast_index: { Todo(); } break;
  case Ast_unary_op: {
    InstructionIndex nop = inst_alloc(builder);
    return ir_ref_from_instruction_index(nop);
  } break;
  case Ast_binary_op: {
    InstructionIndex nop = inst_alloc(builder);
    return ir_ref_from_instruction_index(nop);
  } break;
  case Ast_param: { Todo(); } break;
  case Ast_for: { Todo(); } break;
  case Ast_while: { 
    AstWhile *ast_while = ast_data(ast, idx_ast);

    InstructionIndex loop = inst_loop_begin(builder);

    IrRef cond_val = gen_code(gen, ast_while->cond, ir_ref_from_value_index(gen->common->val.bool));

    InstructionIndex inst_condbr = inst_alloc(builder);
    inst_set_opcode(builder, inst_condbr, IR_condbr);

    IrCondBr *data_condbr = inst_push_data(builder, inst_condbr, IrCondBr);

    InstructionIndex exit_block = inst_block_begin(builder);
    inst_block_end_with_target(builder, exit_block, loop);

    InstructionIndex body_block = inst_block_begin(builder);
    IrRef body = gen_code(gen, ast_while->body, (IrRef){0});

    inst_block_end_repeat(builder, body_block, loop);

    *data_condbr = (IrCondBr){
      .cond = cond_val,
      .then = exit_block,
      .otherwise = body_block,
    };

    return ir_ref_from_instruction_index(loop);
  } break;
  case Ast_defer: { Todo(); } break;
  case Ast_const: { Todo(); } break;
  case Ast_cast: { Todo(); } break;
  case Ast_as: { Todo(); } break;
  case Ast_break: { Todo(); } break;
  }

  Unreachable();
}

// For some constructions a part of the declared type is allowed to live in the value.
// This function is for adding type checking code for those constructs.
IrRef gen_declaration_type_of_val_code(CodeGen *gen, AstIndex idx_ast, IrRef declared_type) {
  AstNodes *ast = &gen->source->ast;
  IrBuilder *builder = &gen->builder;

  SourceIndex source_idx = gen->source->idx;

  IrRef res;

  u8 kind = ast->kinds[idx_ast];
  switch (Cast(AstKind, kind)) {
  case Ast_identifier: {
    res = declared_type;
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
    inst_set_source(builder, inst_type, source_idx, idx_ast);

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

    res = ir_ref_from_instruction_index(inst_unify);
  } break;
  default:
    Todo();
  }

  return res;
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

  InstructionIndex ir_decl = inst_alloc(builder);
  inst_set_opcode(builder, ir_decl, IR_declaration);
  inst_set_source(builder, ir_decl, source->idx, ast_idx_decl);
  IrDeclaration *data_decl = inst_push_data(builder, ir_decl, IrDeclaration);

  IrRef decl_type;
  {
    InstructionIndex block_decl_type = inst_eval_block_begin(builder);

    IrRef ref_decl_type = {0};
    if (ast_decl->type) {
      IrRef ref_type = ir_ref_from_value_index(gen.common->val.type);
      ref_decl_type = gen_code(&gen, ast_decl->type, ref_type);
    }

    IrRef ref_decl_type_of_val =
      gen_declaration_type_of_val_code(&gen, ast_decl->value, ref_decl_type);

    inst_block_end_with_value(builder, block_decl_type, ref_decl_type_of_val);

    decl_type = ir_ref_from_instruction_index(block_decl_type);
  }

  IrRef decl_val;
  {
    InstructionIndex block = inst_eval_block_begin(builder);

    IrRef ref_decl_val = gen_code(&gen, ast_decl->value, decl_type);

    inst_block_end_with_value(builder, block, ref_decl_val);
    
    decl_val = ir_ref_from_instruction_index(block);
  }

  *data_decl = (IrDeclaration){
    .declared_type = decl_type,
    .value = decl_val,
  };

  irbuilder_flatten(builder, context->perm, &decl->data.decl.chunk);

  codegen_deinit(&gen);

  return !gen.has_error;
}
