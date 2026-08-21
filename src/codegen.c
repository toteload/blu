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

  InstructionIndex lookup = irbuilder_add(&gen->builder, SIR_lookup_decl_value);
  inst_set_source(&gen->builder, lookup, gen->source->idx, ast_idx);
  inst_set_data(&gen->builder, lookup, decl);

  return lookup;
}

SRef gen_code_for_ptr(CodeGen *gen, AstIndex idx_ast, SRef type_destination) {
  AstNodes *ast = &gen->source->ast;
  AstKind kind = ast->kinds[idx_ast];

  SourceIndex source_idx = gen->source->idx;

  switch (kind) {
  case Ast_identifier: {
    TokenIndex *name = ast_data(ast, idx_ast);
    StringIndex str = strings_add(gen->strings, token_string(&gen->source->tokens, gen->source->text, *name));

    InstructionIndex ptr = lookup(gen, str, idx_ast);

    if (sref_is_nil(type_destination)) {
      return sref_from_instruction(ptr);
    }

    InstructionIndex inst = irbuilder_add_sir_as(&gen->builder, type_destination, sref_from_instruction(ptr));
    inst_set_source(&gen->builder, inst, source_idx, idx_ast);

    return sref_from_instruction(inst);
  } break;
  default: Todo();
  }

  Todo();

  return (SRef){0};
}

SRef gen_code(CodeGen *gen, AstIndex idx_ast, SRef type_destination) {
  String text = gen->source->text;
  Tokens *tokens = &gen->source->tokens;
  AstNodes *ast = &gen->source->ast;
  IrBuilder *builder = &gen->builder;
  SourceIndex source_idx = gen->source->idx;

  AstKind kind = ast->kinds[idx_ast];
  switch (kind) {
  case Ast_identifier: {
    TokenIndex *name = ast_data(ast, idx_ast);
    StringIndex str = strings_add(gen->strings, token_string(&gen->source->tokens, gen->source->text, *name));

    InstructionIndex ptr = lookup(gen, str, idx_ast);

    InstructionIndex inst_load = irbuilder_add(builder, SIR_load);
    inst_set_data(builder, inst_load, sref_to_u32(sref_from_instruction(ptr)));

    if (sref_is_nil(type_destination)) {
      return sref_from_instruction(inst_load);
    }

    InstructionIndex i = irbuilder_add_sir_as(&gen->builder, type_destination, sref_from_instruction(inst_load));
    inst_set_source(builder, i, source_idx, idx_ast);
    return sref_from_instruction(i);
  } break;
  case Ast_block: {
    AstBlock *block = ast_data(ast, idx_ast);
    u32 count = block->count;
    if (count == 0) {
      return sref_from_value(gen->common->val.nil);
    }

    InstructionIndex inst_block = irbuilder_add(builder, SIR_block);

    stack_push(&gen->scope_stack, (ScopeEntry){ .kind = ScopeEntry_block });

    for (u32 i = 0; i < count-1; i++) {
      gen_code(gen, block->items[i], (SRef){0});
    }

    SRef res = gen_code(gen, block->items[count-1], type_destination);

    irbuilder_end_sir_block_with(builder, inst_block, inst_block, res);

    while (True) {
      ScopeEntry entry = stack_pop(&gen->scope_stack);

      if (entry.kind == ScopeEntry_block) {
        break;
      }

      b32 found = locals_remove(&gen->locals, entry.name);
      Assert(found);
    }

    return sref_from_instruction(inst_block);
  } break;
  case Ast_if_else: {
    AstIfElse *if_else = ast_data(ast, idx_ast);

    InstructionIndex block = irbuilder_add(builder, SIR_block);

    InstructionIndex cond_block = irbuilder_add(builder, SIR_block);
    SRef cond_val = gen_code(gen, if_else->cond, sref_from_value(gen->common->val.bool));
    irbuilder_end_sir_block_with(builder, cond_block, cond_block, cond_val);
    SRef cond = sref_from_instruction(cond_block);

    InstructionIndex condbr = irbuilder_add(builder, SIR_condbr);

    InstructionIndex then_block = irbuilder_add(builder, SIR_block);
    SRef then_val = gen_code(gen, if_else->then, type_destination);
    irbuilder_end_sir_block_with(builder, then_block, block, then_val);

    SCondbr *data_condbr = inst_push_data(builder, condbr, SCondbr);

    if (if_else->otherwise) {
      InstructionIndex else_block = irbuilder_add(builder, SIR_block);
      SRef else_val = gen_code(gen, if_else->otherwise, type_destination);
      irbuilder_end_sir_block_with(builder, else_block, block, else_val);

      *data_condbr = (SCondbr){
        .cond = cond,
        .then = then_block,
        .otherwise = else_block,
      };
    } else {
      Todo();
    }

    inst_set_data(builder, block, inst_offset(builder, block));

    return sref_from_instruction(block);
  } break;
  case Ast_type_function: {
    AstTypeFunction *func = ast_data(ast, idx_ast);
   SRef ref_ret_type = gen_code(gen, func->return_type, sref_from_value(gen->common->val.type));

    for (u32 i = 0; i < func->count; i++) {
      // TODO Add the parameter types
      Panic();
    }

    InstructionIndex inst_type = irbuilder_add(&gen->builder, SIR_type);
    inst_set_source(&gen->builder, inst_type, source_idx, idx_ast);

    u32 arg_count = func->count + 1; // parameters + return type

    SType *data_type = inst_push_data_raw(
      &gen->builder,
      inst_type,
      sizeof(SType) + arg_count * sizeof(SRef),
      Align_of(SType)
    );
    *data_type = (SType){
      .kind = Type_function,
      .arg_count = arg_count,
    };

    data_type->args[0] = ref_ret_type;

    InstructionIndex i = irbuilder_add_sir_as(&gen->builder, type_destination, sref_from_instruction(inst_type));
    inst_set_source(builder, i, source_idx, idx_ast);
    return sref_from_instruction(i);
  } break;
  case Ast_function: {
    AstFunction *func = ast_data(ast, idx_ast);

    InstructionIndex inst_return_type = irbuilder_add(builder, SIR_return_type);
    inst_set_data(builder, inst_return_type, sref_to_u32(type_destination));

    for (u32 i = 0; i < func->count; i++) {
      InstructionIndex inst_param_type = irbuilder_add(builder, SIR_param_type);
      SParamType *param_type = inst_push_data(builder, inst_param_type, SParamType);
      *param_type = (SParamType){
        .function = type_destination,
        .param_index = i,
      };
    }

    InstructionIndex inst_func = irbuilder_add(builder, SIR_func);
    inst_set_source(builder, inst_func, source_idx, idx_ast);
    SFunc *data_func = inst_push_data(builder, inst_func, SFunc);

    InstructionIndex first_param_type = inst_return_type + 1;

    for (u32 i = 0; i < func->count; i++) {
      InstructionIndex inst_param = irbuilder_add(builder, SIR_param);
      inst_set_source(builder, inst_param, source_idx, func->params[i]);
      inst_set_data(builder, inst_param, sref_to_u32(sref_from_instruction(first_param_type + i)));
    }

    SRef inst_body = gen_code(gen, func->body, sref_from_instruction(inst_return_type));

    InstructionIndex inst_ret = irbuilder_add(&gen->builder, SIR_ret);
    inst_set_data(&gen->builder, inst_ret, sref_to_u32(inst_body));

    u32 func_instruction_count = inst_offset(&gen->builder, inst_func);

    *data_func = (SFunc){
      .param_count = func->count,
      .instruction_count = func_instruction_count,
      .return_type = sref_from_instruction(inst_return_type),
    };

    return sref_from_instruction(inst_func);
  } break;
  case Ast_declaration: {
    AstDeclaration *decl = ast_data(ast, idx_ast);

    Assert(decl->type);

    StringIndex str = strings_add(gen->strings, token_string(&gen->source->tokens, gen->source->text, decl->name));

    SRef ref_type = sref_from_value(gen->common->val.type);

    SRef ref_decl_type = gen_code(gen, decl->type, ref_type);

    InstructionIndex inst_var = irbuilder_add(builder, SIR_alloc);
    inst_set_data(builder, inst_var, sref_to_u32(ref_decl_type));

    stack_push(&gen->scope_stack, ((ScopeEntry){ .kind = ScopeEntry_local, .name = str }));
    locals_insert(&gen->locals, str, inst_var);

    SRef ref_decl_val = gen_code(gen, decl->value, ref_decl_type);

    InstructionIndex inst_store = irbuilder_add(builder, SIR_store);
    SStore *store = inst_push_data(builder, inst_store, SStore);
    *store = (SStore){
      .dst = sref_from_instruction(inst_var),
      .value = ref_decl_val,
    };

    return sref_from_value(gen->common->val.nil);
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

    InstructionIndex i = irbuilder_add_sir_as(&gen->builder, type_destination, sref_from_value(idx));
    inst_set_source(builder, i, source_idx, idx_ast);
    return sref_from_instruction(i);
  } break;
  case Ast_call: {
    AstCall *ast_call = ast_data(ast, idx_ast);

    InstructionIndex inst_type = irbuilder_add(builder, SIR_type);
    u32 arg_count = ast_call->count + 1;
    SType *data_type = inst_push_data_raw(builder, inst_type, sizeof(SType) + arg_count * sizeof(SRef), Align_of(SType));
    *data_type = (SType){
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
    SRef callee = gen_code(gen, ast_call->callee, sref_from_instruction(inst_type));

    // Evaluate all the arguments
    for (u32 i = 0; i < ast_call->count; i++) {
      Todo(); // :) haha you still have to implement this
    }

    InstructionIndex inst_call = irbuilder_add(builder, SIR_call);
    inst_set_source(builder, inst_call, source_idx, idx_ast);

    SCall *call = inst_push_data(builder, inst_call, SCall);
    *call = (SCall){
      .func = callee,
      .arg_count = ast_call->count,
    };

    Assert(ast_call->count == 0); // TODO: add the args to IR

    InstructionIndex i = irbuilder_add_sir_as(&gen->builder, type_destination, sref_from_instruction(inst_call));
    inst_set_source(builder, i, source_idx, idx_ast);
    return sref_from_instruction(i);
  } break;
  case Ast_builtin: {
    AstBuiltin *builtin = ast_data(ast, idx_ast);
    switch (Cast(BuiltinKind, builtin->kind)) {
    case Builtin_debug: {
      AstIndex e = builtin->args[0];
      SRef val = gen_code(gen, e, type_destination);

      InstructionIndex inst_debug = irbuilder_add(builder, SIR_builtin_debug);
      inst_set_data(builder, inst_debug, sref_to_u32(val));

      return sref_from_instruction(inst_debug);
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

    InstructionIndex i = irbuilder_add_sir_as(&gen->builder, type_destination, sref_from_value(idx));
    inst_set_source(builder, i, source_idx, idx_ast);
    return sref_from_instruction(i);
  } break;
  case Ast_source: { Todo(); } break;
  case Ast_mod_section: { Todo(); } break;
  case Ast_type_slice: { Todo(); } break;
  case Ast_type_array: { Todo(); } break;
  case Ast_assign: { 
    AstAssign *assign = ast_data(ast, idx_ast);

    Assert(assign->kind == Assign_normal);

    SRef lhs_ptr = gen_code_for_ptr(gen, assign->lhs, (SRef){0});

    InstructionIndex inst_typeof_lhs = irbuilder_add(builder, SIR_typeof);
    inst_set_data(builder, inst_typeof_lhs, sref_to_u32(lhs_ptr));

    InstructionIndex inst_basetype = irbuilder_add(builder, SIR_base_type);
    inst_set_data(builder, inst_basetype, sref_to_u32(sref_from_instruction(inst_typeof_lhs)));

    SRef value = gen_code(gen, assign->value, sref_from_instruction(inst_basetype));

    InstructionIndex inst_store = irbuilder_add(builder, SIR_store);
    SStore *store = inst_push_data(builder, inst_store, SStore);
    *store = (SStore){
      .dst = lhs_ptr,
      .value = value,
    };
    
    return sref_from_instruction(inst_store);
  } break;
  case Ast_label: { Todo(); } break;
  case Ast_index: { Todo(); } break;
  case Ast_unary_op: {
    Todo();
  } break;
  case Ast_binary_op: {
    Todo();
  } break;
  case Ast_param: { Todo(); } break;
  case Ast_for: { Todo(); } break;
  case Ast_while: { 
    AstWhile *ast_while = ast_data(ast, idx_ast);

    InstructionIndex loop = irbuilder_add(builder, SIR_loop);

    SRef cond_val = gen_code(gen, ast_while->cond, sref_from_value(gen->common->val.bool));

    InstructionIndex inst_condbr = irbuilder_add(builder, SIR_condbr);

    SCondbr *data_condbr = inst_push_data(builder, inst_condbr, SCondbr);

    InstructionIndex exit_block = irbuilder_add(builder, SIR_block);
    irbuilder_end_sir_block_with(builder, exit_block, loop, (SRef){0});

    InstructionIndex body_block = irbuilder_add(builder, SIR_block);
    SRef body = gen_code(gen, ast_while->body, (SRef){0});
    Unused(body);

    InstructionIndex repeat = irbuilder_add(builder, SIR_repeat); 
    inst_set_data(builder, repeat, sref_to_u32(sref_from_instruction(loop)));

    inst_set_data(builder, loop, inst_offset(builder, loop));

    *data_condbr = (SCondbr){
      .cond = cond_val,
      .then = exit_block,
      .otherwise = body_block,
    };

    return sref_from_instruction(loop);
  } break;
  case Ast_defer: { Todo(); } break;
  case Ast_const: { Todo(); } break;
  case Ast_cast: { Todo(); } break;
  case Ast_as: { Todo(); } break;
  case Ast_break: { Todo(); } break;
  }

  Unreachable();
}

// For some constructions a part of the declaration type is allowed to live in the value.
// This function is for adding type checking code for those constructs.
SRef gen_code_for_declaration_type(CodeGen *gen, AstIndex idx_ast, SRef declared_type) {
  AstNodes *ast = &gen->source->ast;
  IrBuilder *builder = &gen->builder;
  SourceIndex source_idx = gen->source->idx;

  AstKind kind = ast->kinds[idx_ast];
  switch (kind) {
  case Ast_identifier: {
    return declared_type;
  } break;
  case Ast_function: {
    AstFunction *func = ast_data(ast, idx_ast);

    SRef ref_ret_type = {0};
    if (func->return_type) {
      ref_ret_type = gen_code(gen, func->return_type, sref_from_value(gen->common->val.type));
    }

    // Output param type expressions
    for (u32 i = 0; i < func->count; i++) {
      Todo();
    }

    InstructionIndex inst_type = irbuilder_add(builder, SIR_type);
    inst_set_source(builder, inst_type, source_idx, idx_ast);

    u32 arg_count = func->count + 1; // parameters + return type

    Assert(func->count == 0);

    SType *data_type = inst_push_data_raw(
      &gen->builder,
      inst_type,
      sizeof(SType) + arg_count * sizeof(SRef),
      Align_of(SType)
    );
    *data_type = (SType){
      .kind = Type_function,
      .arg_count = arg_count,
    };

    data_type->args[0] = ref_ret_type;

    InstructionIndex inst_unify = irbuilder_add(builder, SIR_unify);
    SUnify *data_unify = inst_push_data(builder, inst_unify, SUnify);
    *data_unify = (SUnify){
      .type_lhs = declared_type,
      .type_rhs = sref_from_instruction(inst_type),
    };

    return sref_from_instruction(inst_unify);
  } break;
  default:
    Todo();
  }

  Unreachable();
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

  InstructionIndex block_decl_type;
  {
    InstructionIndex block = irbuilder_add(builder, SIR_eval_block);

    SRef ref_decl_type = {0};
    if (ast_decl->type) {
      SRef ref_type = sref_from_value(gen.common->val.type);
      ref_decl_type = gen_code(&gen, ast_decl->type, ref_type);
    }

    SRef ref_decl_type_of_val = gen_code_for_declaration_type(&gen, ast_decl->value, ref_decl_type);

    irbuilder_end_sir_block_with(builder, block, block, ref_decl_type_of_val);

    block_decl_type = block;
  }

  InstructionIndex block_decl_val;
  {
    InstructionIndex block = irbuilder_add(builder, SIR_eval_block);

    SRef ref_decl_val = gen_code(&gen, ast_decl->value, sref_from_instruction(block_decl_type));

    irbuilder_end_sir_block_with(builder, block, block, ref_decl_val);
    
    block_decl_val = block;
  }

  irbuilder_flatten(builder, context->perm, &decl->data.decl.chunk);

  decl->data.decl.block_type = block_decl_type;
  decl->data.decl.block_val = block_decl_val;

  codegen_deinit(&gen);

  return !gen.has_error;
}
