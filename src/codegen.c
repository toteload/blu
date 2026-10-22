#include "blu.h"
#include "value.h"
#include "tokens.h"
#include "messages.h"
#include "string_interner.h"
#include "source_file.h"
#include "compiler.h"
#include "codegen.h"
#include "ir.h"
#include "symbol_map.h"

typedef struct {
  b32 has_error;
  ArenaSnapshot scope_scratch;

  SIrBuilder builder;
  SymbolMap syms;

  Source *source;

  // the module stack that the current declaration is in
  u32 mod_depth;
  DeclarationIndex *mods;
} CodeGen;

internal StringIndex intern_ast_label_string(CodeGenContext *ctx, Source *source, AstIndex idx) {
  if (!idx) {
    return 0;
  }


  TokenIndex *label = ast_data(&source->ast, idx);
  String str = token_string(&source->tokens, source->text, *label);
  return strings_add(ctx->strings, str);
}

b32 codegen_init(CodeGen *gen, CodeGenContext *context, Source *source, u32 tree_idx) {
  *gen = (CodeGen){
    .has_error = False,
    .scope_scratch = arena_scope_begin(context->scratch),
    .builder = (SIrBuilder){.scratch = context->scratch},
    .source = source,
  };

  symbols_init(&gen->syms, context->gpa, context->scratch);

  DeclarationIndex *mods = arena_push_array(DeclarationIndex, context->scratch, MAX_MODULE_DEPTH);
  u32 i = source->decls[tree_idx].parent;
  u32 offset = 0;
  Assert(source->decls[0].kind == SourceDeclaration_root);
  while (i && offset < (MAX_MODULE_DEPTH - 1)) {
    mods[offset++] = source->decl_idxs[i];
    i = source->decls[i].parent;
  }

  if (i != 0) {
    Todo(); // max module depth exceeded.
    return False;
  }

  mods[offset++] = source->decl_idxs[0];

  gen->mod_depth = offset;
  gen->mods = mods;

  return True;
}

void codegen_deinit(CodeGen *gen, CodeGenContext *ctx) {
  symbols_deinit(&gen->syms);
  arena_scope_end(ctx->scratch, gen->scope_scratch);
}

typedef enum {
  LOOKUP_PARAM,
  LOOKUP_LOCAL,
  LOOKUP_DECL,
} LookupResultKind;

typedef struct {
  u32 kind;
  union {
    InstructionIndex inst;
    DeclarationIndex decl_idx;
  } data;
} LookupResult;

// - Assumes that `name` is not a label
internal b32 lookup(CodeGen *gen, CodeGenContext *ctx, StringIndex name, LookupResult *res) {
  ScopeEntry *entry = symbols_find(&gen->syms, name); 
  if (entry) {
    *res = (LookupResult){
      .kind = (entry->kind == ScopeEntry_param) ? LOOKUP_PARAM : LOOKUP_LOCAL,
      .data.inst = entry->inst,
    };

    return True;
  }

  DeclarationIndex decl = 0;
  b32 found = lookup_identifier(ctx->decls, gen->mods, gen->mod_depth, name, &decl);
  if (!found) {
    return False;
  }

  *res = (LookupResult){ .kind = LOOKUP_DECL, .data.decl_idx = decl };

  return True;
}

b32 sref_is_terminator_instruction(SIrBuilder *builder, SRef ref) {
  if (!sref_is_instruction(ref)) {
    return False;
  }

  InstructionIndex inst = sref_to_instruction(ref);
  SIrOpcode op = sir_builder_get_opcode(builder, inst);

  switch (op) {
  case SIR_br:
  case SIR_repeat:
  case SIR_ret:
    return True;
  default:
  return False;
  }
}

SRef gen_code_for_ptr(CodeGen *gen, CodeGenContext *ctx, AstIndex idx_ast, SRef type_destination) {
  AstNodes *ast = &gen->source->ast;
  AstKind kind = ast->kinds[idx_ast];

  switch (kind) {
  case Ast_identifier: {
    TokenIndex *name = ast_data(ast, idx_ast);
    StringIndex str = strings_add(ctx->strings, token_string(&gen->source->tokens, gen->source->text, *name));

    LookupResult res;
    b32 ok = lookup(gen, ctx, str, &res);
    if (!ok) {
      Todo();
    }

    InstructionIndex inst;
    switch (Cast(LookupResultKind, res.kind)) {
    case LOOKUP_PARAM: Todo(); // cannot take pointer to a param
    case LOOKUP_DECL:  Todo();
    case LOOKUP_LOCAL: inst = res.data.inst; break;
    }

    return sir_builder_add_as(&gen->builder, type_destination, sref_from_instruction(inst), gen->source->idx, idx_ast);
  } break;
  default: Panic();
  }

  Unreachable();

  return (SRef){0};
}

internal b32 binary_op_is_cmp(BinaryOpKind op) {
  switch (op) {
  case Cmp_equal:
  case Cmp_not_equal:
  case Cmp_greater_than:
  case Cmp_greater_equal:
  case Cmp_less_than:
  case Cmp_less_equal:
    return True;
  default:
    return False;
  }
}

internal void binary_op_type_destinations(CodeGenContext *ctx, BinaryOpKind op, SRef type_destination, SRef *lhs_type_destination, SRef *rhs_type_destination) {
  SRef lhs;
  SRef rhs;

  switch (op) {
  case Logical_or:
  case Logical_and: {
    lhs = sref_from_value(ctx->common->val._bool);
    rhs = sref_from_value(ctx->common->val._bool);
  } break;

  case Cmp_equal:
  case Cmp_not_equal:
  case Cmp_greater_than:
  case Cmp_greater_equal:
  case Cmp_less_than:
  case Cmp_less_equal: {
    lhs = (SRef){0};
    rhs = (SRef){0};
  } break;

  case Bit_shift_left:
  case Bit_shift_right: {
    lhs = type_destination;

    // TODO: for now the rhs can be a usize, but at some point it should become log2(bitwidth(lhs))
    rhs = sref_from_value(ctx->common->val.usize);
  } break;

  case Mul:
  case Div:
  case Mod:
  case Sub:
  case Add:
  case Bit_and:
  case Bit_or:
  case Bit_xor: {
    lhs = type_destination;
    rhs = type_destination;
  } break;
  }

  *lhs_type_destination = lhs;
  *rhs_type_destination = rhs;
}

internal InstructionIndex gen_code_for_binary_op(SIrBuilder *builder, BinaryOpKind op, SRef lhs, SRef rhs, SourceIndex source_idx, AstIndex ast_idx) {
  // clang-format on
  u8 sir_op;
  switch (op) {
  case Logical_and: Panic();
  case Logical_or: Panic();

  case Mul: sir_op = SIR_mul; break;
  case Div: sir_op = SIR_div; break;
  case Mod: sir_op = SIR_mod; break;
  case Sub: sir_op = SIR_sub; break;
  case Add: sir_op = SIR_add; break;
  case Cmp_equal: sir_op = SIR_cmp_eq; break;
  case Cmp_not_equal: sir_op = SIR_cmp_ne; break;
  case Cmp_greater_than: sir_op = SIR_cmp_gt; break;
  case Cmp_greater_equal: sir_op = SIR_cmp_ge; break;
  case Cmp_less_than: sir_op = SIR_cmp_lt; break;
  case Cmp_less_equal: sir_op = SIR_cmp_le; break;
  case Bit_shift_left: sir_op = SIR_bitshift_left; break;
  case Bit_shift_right: sir_op = SIR_bitshift_right; break;
  case Bit_and: sir_op = SIR_bit_and; break;
  case Bit_xor: sir_op = SIR_bit_xor; break;
  case Bit_or: sir_op = SIR_bit_or; break;
  }
  // clang-format off

  InstructionIndex inst = sir_builder_add(builder, sir_op, source_idx, ast_idx);
  SIrBinary *data = sir_builder_push_data(builder, inst, SIrBinary);
  *data = (SIrBinary){
    .lhs = lhs,
    .rhs = rhs,
  };

  return inst;
}

SRef gen_code(CodeGen *gen, CodeGenContext *ctx, AstIndex idx_ast, SRef type_destination);

SRef gen_comptime_code(CodeGen *gen, CodeGenContext *ctx, AstIndex idx_ast, SRef type_destination) {
  SIrBuilder *builder = &gen->builder;
  SourceIndex source_idx = gen->source->idx;
  InstructionIndex inst_block = sir_builder_add(builder, SIR_comptime_block, source_idx, idx_ast);
  SRef ref = gen_code(gen, ctx, idx_ast, type_destination);
  sir_builder_end_block_with(builder, inst_block, inst_block, ref, source_idx, idx_ast);
  return sref_from_instruction(inst_block);
}

internal BinaryOpKind compound_assign_kind_to_binary_op_kind(AssignKind kind) {
  switch (kind) {
  case Assign_normal: Panic();
  case Assign_mul: return Mul;
  case Assign_div: return Div;
  case Assign_mod: return Mod;
  case Assign_sub: return Sub;
  case Assign_add: return Add;
  case Assign_bit_shift_left: return Bit_shift_left;
  case Assign_bit_shift_right: return Bit_shift_right;
  case Assign_bit_and: return Bit_and;
  case Assign_bit_xor: return Bit_xor;
  case Assign_bit_or: return Bit_or;
  }
}

SRef gen_code(CodeGen *gen, CodeGenContext *ctx, AstIndex idx_ast, SRef type_destination) {
  String text = gen->source->text;
  Tokens *tokens = &gen->source->tokens;
  AstNodes *ast = &gen->source->ast;
  SIrBuilder *builder = &gen->builder;
  SourceIndex source_idx = gen->source->idx;
  SymbolMap *syms = &gen->syms;
  Source *source = gen->source;

  AstKind kind = ast->kinds[idx_ast];
  switch (kind) {
  case Ast_source:      { PanicMsg("Encountered 'Ast_source' in gen_code, which should never happen."); } break;
  case Ast_mod_section: { PanicMsg("Encountered 'Ast_mod_section' in gen_code, which should never happen."); } break;
  case Ast_param:       { PanicMsg("Encountered 'Ast_param' in gen_code, which should never happen."); } break;
  case Ast_label:       { PanicMsg("Encountered 'Ast_label' in gen_code, which should never happen."); } break;

  case Ast_cast: {
    AstCast *cast = ast_data(ast, idx_ast);

    SRef ref_type = gen_code(gen, ctx, cast->type, sref_from_value(ctx->common->val.type));
    SRef ref_val = gen_code(gen, ctx, cast->value, (SRef){0});

    InstructionIndex inst = sir_builder_add(builder, SIR_cast, source_idx, idx_ast);
    SIrCast *data = sir_builder_push_data(builder, inst, SIrCast);
    *data = (SIrCast){
      .type_dst = ref_type,
      .val = ref_val,
    };

    return sir_builder_add_as(builder, type_destination, sref_from_instruction(inst), source_idx, idx_ast);
  } break;

  case Ast_identifier: {
    TokenIndex *name = ast_data(ast, idx_ast);
    String s = token_string(&gen->source->tokens, gen->source->text, *name);
    StringIndex str = strings_add(ctx->strings, s);

    LookupResult res;
    b32 ok = lookup(gen, ctx, str, &res);
    if (!ok) {
      Todo();
    }

    InstructionIndex inst;
    switch (Cast(LookupResultKind, res.kind)) {
    case LOOKUP_DECL: {
      InstructionIndex inst_lookup_decl = sir_builder_add(builder, SIR_get_decl_value, source_idx, idx_ast);
      sir_builder_set_data(builder, inst_lookup_decl, res.data.decl_idx);

      InstructionIndex inst_load = sir_builder_add(builder, SIR_load, source_idx, idx_ast);
      sir_builder_set_data(builder, inst_load, sref_to_u32(sref_from_instruction(inst_lookup_decl)));

      inst = inst_load;
    } break;
    case LOOKUP_LOCAL: {
      InstructionIndex inst_load = sir_builder_add(builder, SIR_load, source_idx, idx_ast);
      sir_builder_set_data(builder, inst_load, sref_to_u32(sref_from_instruction(res.data.inst)));

      inst = inst_load;
    } break;
    case LOOKUP_PARAM: { inst = res.data.inst; } break;
    }

    return sir_builder_add_as(&gen->builder, type_destination, sref_from_instruction(inst), source_idx, idx_ast);
  } break;

  case Ast_block: {
    AstBlock *block = ast_data(ast, idx_ast);
    u32 count = block->count;
    if (count == 0) {
      return sref_from_value(ctx->common->val.nil);
    }

    InstructionIndex inst_block = sir_builder_add(builder, SIR_block, source_idx, idx_ast);

    StringIndex block_label = intern_ast_label_string(ctx, gen->source, block->label);

    ScopeKey scope_key;
    {
      SymbolsPushScopeResult res = symbols_push_scope(&gen->syms, ScopeEntry_block, inst_block, block_label);
      if (res.err != SYMBOLS_OK) {
        Todo();
      }

      scope_key = res.data.scope_key;
    }

    for (u32 i = 0; i < count-1; i++) {
      gen_code(gen, ctx, block->items[i], (SRef){0});
    }

    SRef res = gen_code(gen, ctx, block->items[count-1], type_destination);

    if (sref_is_terminator_instruction(builder, res)) {
      sir_builder_set_data(builder, inst_block, sir_builder_offset(builder, inst_block));
    } else {
      sir_builder_end_block_with(builder, inst_block, inst_block, res, source_idx, idx_ast);
    }

    symbols_pop_scope(&gen->syms, scope_key);

    return sref_from_instruction(inst_block);
  } break;

  case Ast_if_else: {
    AstIfElse *if_else = ast_data(ast, idx_ast);

    InstructionIndex block = sir_builder_add(builder, SIR_block, source_idx, idx_ast);

    SRef cond_val = gen_code(gen, ctx, if_else->cond, sref_from_value(ctx->common->val._bool));

    InstructionIndex condbr = sir_builder_add(builder, SIR_condbr, source_idx, if_else->cond);
    SIrCondbr *condbr_data = sir_builder_push_data(builder, condbr, SIrCondbr);

    SRef then_val = gen_code(gen, ctx, if_else->then, type_destination);

    InstructionIndex then_br = sir_builder_add(builder, SIR_br, source_idx, if_else->then);
    SIrBr *then_data = sir_builder_push_data(builder, then_br, SIrBr);
    *then_data = (SIrBr){
      .block = block,
      .value = then_val,
    };

    SRef else_val = (SRef){0};
    if (if_else->otherwise) {
      else_val = gen_code(gen, ctx, if_else->otherwise, type_destination);
    }

    InstructionIndex else_br_inst = sir_builder_add(builder, SIR_br, source_idx, if_else->otherwise);
    SIrBr *else_br_data = sir_builder_push_data(builder, else_br_inst, SIrBr);
    *else_br_data = (SIrBr){
      .block = block,
      .value = else_val,
    };

    *condbr_data = (SIrCondbr){
      .cond = cond_val,
      .otherwise = else_br_inst,
    };

    sir_builder_set_data(builder, block, sir_builder_offset(builder, block));

    return sref_from_instruction(block);
  } break;

  case Ast_type_function: {
    AstTypeFunction *func = ast_data(ast, idx_ast);
    SRef ref_ret_type = gen_code(gen, ctx, func->return_type, sref_from_value(ctx->common->val.type));

    SRef *param_types = arena_push_array(SRef, ctx->scratch, func->count);

    for (u32 i = 0; i < func->count; i++) {
      param_types[i] = gen_code(gen, ctx, func->param_types[i], sref_from_value(ctx->common->val.type));
    }

    InstructionIndex inst_type = sir_builder_add(&gen->builder, SIR_type, source_idx, idx_ast);

    u32 arg_count = func->count + 1; // parameters + return type

    SIrType *data_type = sir_builder_push_data_raw(
      &gen->builder,
      inst_type,
      sizeof(SIrType) + arg_count * sizeof(SRef),
      Align_of(SIrType)
    );
    *data_type = (SIrType){
      .kind = Type_function,
      .arg_count = arg_count,
    };

    data_type->args[0] = ref_ret_type;

    for (u32 i = 0; i < func->count; i++) {
      data_type->args[1 + i] = param_types[i];
    }

    return sir_builder_add_as(&gen->builder, type_destination, sref_from_instruction(inst_type), source_idx, idx_ast);
  } break;

  case Ast_function: {
    AstFunction *func = ast_data(ast, idx_ast);

    InstructionIndex inst_func = sir_builder_add(builder, SIR_func, source_idx, idx_ast);
    SIrFunc *data_func = sir_builder_push_data(builder, inst_func, SIrFunc);

    InstructionIndex return_type_inst;
    {
      SRef return_type_ref = {};
      return_type_inst = sir_builder_add(builder, SIR_return_type, source_idx, func->return_type);
      if (func->return_type) {
        return_type_ref = gen_code(gen, ctx, func->return_type, sref_from_value(ctx->common->val.type));
      }
      sir_builder_end_block_with(builder, return_type_inst, return_type_inst, return_type_ref, source_idx, func->return_type);
    }

    for (u32 i = 0; i < func->count; i++) {
      AstParam *ast_param = ast_data(ast, func->params[i]);

      InstructionIndex param_inst = sir_builder_add(builder, SIR_param, source_idx, func->params[i]);

      SRef param_type = {};
      if (ast_param->type) {
        param_type = gen_code(gen, ctx, ast_param->type, sref_from_value(ctx->common->val.type));
      }

      sir_builder_end_block_with(builder, param_inst, param_inst, param_type, source_idx, ast_param->type);

      StringIndex str = strings_add(ctx->strings, token_string(tokens, text, ast_param->name));
      SymbolsPushLocalResult res = symbols_push_local(&gen->syms, ScopeEntry_param, str, param_inst);
      if (res.err != SYMBOLS_OK) {
        Todo();
      }
    }

    ScopeKey scope = symbols_push_scope(&gen->syms, ScopeEntry_function, inst_func, 0).data.scope_key;

    SRef inst_body = gen_code(gen, ctx, func->body, sref_from_instruction(return_type_inst));

    symbols_pop_scope(syms, scope);

    InstructionIndex inst_ret = sir_builder_add(&gen->builder, SIR_ret, source_idx, func->body);
    sir_builder_set_data(&gen->builder, inst_ret, sref_to_u32(inst_body));

    u32 func_instruction_count = sir_builder_offset(&gen->builder, inst_func);

    *data_func = (SIrFunc){
      .type_destination = type_destination,
      .param_count = func->count,
      .instruction_count = func_instruction_count,
    };

    return sref_from_instruction(inst_func);
  } break;

  case Ast_declaration: {
    AstDeclaration *decl = ast_data(ast, idx_ast);

    Assert(decl->type);

    StringIndex str = strings_add(ctx->strings, token_string(&gen->source->tokens, gen->source->text, decl->name));

    SRef ref_type = sref_from_value(ctx->common->val.type);

    SRef ref_decl_type = gen_code(gen, ctx, decl->type, ref_type);

    InstructionIndex inst_var = sir_builder_add(builder, SIR_alloc, source_idx, idx_ast);
    sir_builder_set_data(builder, inst_var, sref_to_u32(ref_decl_type));

    SymbolsPushLocalResult res = symbols_push_local(syms, ScopeEntry_local, str, inst_var);
    if (res.err != SYMBOLS_OK) {
      gen->has_error = True;
      Message_error(
        ctx->msg_sink,
        (MessageLocation){
          .kind = MessageLocation_ast_index,
          .source_idx = source_idx,
          .data.ast_index = idx_ast,
        },
        string_lit("Variable has already been declared")
      );
    }

    SRef ref_decl_val = gen_code(gen, ctx, decl->value, ref_decl_type);

    InstructionIndex inst_store = sir_builder_add(builder, SIR_store, source_idx, decl->value);
    SIrStore *store = sir_builder_push_data(builder, inst_store, SIrStore);
    *store = (SIrStore){
      .dst = sref_from_instruction(inst_var),
      .value = ref_decl_val,
    };

    return sref_from_value(ctx->common->val.nil);
  } break;

  case Ast_literal_u8_string: {
    TokenIndex *tok = ast_data(ast, idx_ast);
    String s = token_string(tokens, text, *tok);

    // s is of the form `u8"x"` and for now we only support strings containing a single character,
    // so no escape codes.
    if (s.len - 4 > 1) {
      Todo();
    }

    Value *v;
    ValueIndex idx = values_alloc(ctx->values, &v);
    u8 *data = values_alloc_data_type(ctx->values, u8);
    *data = s.str[3];
    *v = (Value){
      .type = ctx->types->t_u8,
      .data_size = sizeof(u8),
      .data = data,
    };

    return sir_builder_add_as(&gen->builder, type_destination, sref_from_value(idx), source_idx, idx_ast);
  } break;

  case Ast_literal_int: {
    TokenIndex *tok = ast_data(ast, idx_ast);
    u64 value;
    u32 err = parse_u64(token_string(tokens, text, *tok), &value);
    if (err == PARSE_U64_OVERFLOW) {
      Message_error(
        ctx->msg_sink,
        (MessageLocation){
          .kind = MessageLocation_ast_index,
          .source_idx = source_idx,
          .data.ast_index = idx_ast,
        },
        string_lit("Integer literal does not fit in u64."));
      gen->has_error = True;
      return sref_from_value(ctx->common->val.nil);
    }

    Value *v;
    ValueIndex idx = values_alloc(ctx->values, &v);
    ComptimeInt *data = values_alloc_data(ctx->values, sizeof(ComptimeInt), Align_of(ComptimeInt));
    *data = Cast(i64, value);
    *v = (Value){
      .type = ctx->types->t_comptime_int,
      .data_size = sizeof(ComptimeInt),
      .data = data,
    };

    return sir_builder_add_as(&gen->builder, type_destination, sref_from_value(idx), source_idx, idx_ast);
  } break;

  case Ast_call: {
    AstCall *ast_call = ast_data(ast, idx_ast);

    SRef callee = gen_code(gen, ctx, ast_call->callee, (SRef){0});

    SRef *args = arena_push_array(SRef, ctx->scratch, ast_call->count);

    if (ast_call->count > 0) {
      InstructionIndex callee_type = sir_builder_add(builder, SIR_typeof, source_idx, ast_call->callee);
      sir_builder_set_data(builder, callee_type, sref_to_u32(callee));

      for (u32 i = 0; i < ast_call->count; i++) {
        InstructionIndex inst_param_type = sir_builder_add(builder, SIR_typeof_param, source_idx, ast_call->args[i]);
        SIrTypeofParam *data = sir_builder_push_data(builder, inst_param_type, SIrTypeofParam);
        *data = (SIrTypeofParam){
          .function_type = sref_from_instruction(callee_type),
          .param_index = i,
        };
        args[i] = gen_code(gen, ctx, ast_call->args[i], sref_from_instruction(inst_param_type));
      }
    }

    InstructionIndex inst_call = sir_builder_add(builder, SIR_call, source_idx, idx_ast);

    SIrCall *call = sir_builder_push_data_raw(
      builder,
      inst_call,
      sizeof(SIrCall) + ast_call->count * sizeof(SRef),
      Align_of(SIrCall)
    );
    *call = (SIrCall){
      .func = callee,
      .arg_count = ast_call->count,
    };

    for (u32 i = 0; i < ast_call->count; i++) {
      call->args[i] = args[i];
    }

    return sir_builder_add_as(&gen->builder, type_destination, sref_from_instruction(inst_call), source_idx, idx_ast);
  } break;

  case Ast_builtin: {
    AstBuiltin *builtin = ast_data(ast, idx_ast);
    switch (Cast(BuiltinKind, builtin->kind)) {
    case Builtin_debug: {
      AstIndex e = builtin->args[0];
      SRef val = gen_code(gen, ctx, e, type_destination);

      InstructionIndex inst_debug = sir_builder_add(builder, SIR_builtin_debug, source_idx, idx_ast);
      sir_builder_set_data(builder, inst_debug, sref_to_u32(val));

      return sref_from_instruction(inst_debug);
    } break;

    case Builtin_len: {
      SRef val = gen_code(gen, ctx, builtin->args[0], (SRef){0});

      InstructionIndex inst_len = sir_builder_add(builder, SIR_builtin_len, source_idx, idx_ast);
      sir_builder_set_data(builder, inst_len, sref_to_u32(val));

      return sir_builder_add_as(&gen->builder, type_destination, sref_from_instruction(inst_len), source_idx, idx_ast);
    } break;
    }
  } break;

  case Ast_literal_string: {
    TokenIndex *name = ast_data(ast, idx_ast);
    
    String s = token_string(&gen->source->tokens, gen->source->text, *name);

    void *data = values_alloc_data(ctx->values, s.len, 1);
    u32 len;
    decode_string_literal(s, data, &len);

    TypeKey type = types_intern(ctx->types, &(Type){
      .kind = Type_array,
      .data.array = {
        .base_type = ctx->types->t_u8,
        .size = len,
      },
    });

    Value *v;
    ValueIndex idx = values_alloc(ctx->values, &v);
    *v = (Value){
      .type = type,
      .data = data,
      .data_size = s.len,
    };

    return sir_builder_add_as(&gen->builder, type_destination, sref_from_value(idx), source_idx, idx_ast);
  } break;

  case Ast_type_slice: {
    AstTypeSlice *slice = ast_data(ast, idx_ast);
    SRef base_type = gen_code(gen, ctx, slice->base, sref_from_value(ctx->common->val.type));
    InstructionIndex inst = sir_builder_add(builder, SIR_type, source_idx, idx_ast);
    SIrType *data = sir_builder_push_data_raw(builder, inst, sizeof(SIrType) + sizeof(SRef), Align_of(SIrType));
    data->kind = Type_slice;
    data->arg_count = 1;
    data->args[0] = base_type;
    return sref_from_instruction(inst);
  } break;

  case Ast_type_array: {
    AstTypeArray *array = ast_data(ast, idx_ast);

    SRef array_size = gen_comptime_code(gen, ctx, array->size, sref_from_value(ctx->common->val.usize));

    SRef base_type = gen_code(gen, ctx, array->base, sref_from_value(ctx->common->val.type));

    InstructionIndex inst = sir_builder_add(builder, SIR_type, source_idx, idx_ast);
    SIrType *data = sir_builder_push_data_raw(builder, inst, sizeof(SIrType) + 2 * sizeof(SRef), Align_of(SIrType));
    data->kind = Type_array;
    data->arg_count = 2;
    data->args[0] = array_size;
    data->args[1] = base_type;
    return sref_from_instruction(inst);
  } break;

  case Ast_assign: { 
    AstAssign *assign = ast_data(ast, idx_ast);

    SRef lhs_ptr = gen_code_for_ptr(gen, ctx, assign->lhs, (SRef){0});

    InstructionIndex inst_typeof_lhs = sir_builder_add(builder, SIR_typeof, source_idx, assign->lhs);
    sir_builder_set_data(builder, inst_typeof_lhs, sref_to_u32(lhs_ptr));

    InstructionIndex inst_basetype = sir_builder_add(builder, SIR_base_type, source_idx, assign->lhs);
    sir_builder_set_data(builder, inst_basetype, sref_to_u32(sref_from_instruction(inst_typeof_lhs)));

    SRef value;
    AssignKind kind = assign->kind;
    if (kind != Assign_normal) {
      BinaryOpKind binop = compound_assign_kind_to_binary_op_kind(kind);

      SRef lhs_type_destination, rhs_type_destination;
      binary_op_type_destinations(ctx, binop, sref_from_instruction(inst_basetype), &lhs_type_destination, &rhs_type_destination);

      InstructionIndex lhs = sir_builder_add(builder, SIR_load, source_idx, assign->lhs);
      sir_builder_set_data(builder, lhs, sref_to_u32(lhs_ptr));

      SRef lhs_val = sir_builder_add_as(&gen->builder, lhs_type_destination, sref_from_instruction(lhs), source_idx, idx_ast);

      SRef rhs = gen_code(gen, ctx, assign->value, rhs_type_destination);

      value = sref_from_instruction(gen_code_for_binary_op(&gen->builder, binop, lhs_val, rhs, source_idx, idx_ast));
    } else {
      value = gen_code(gen, ctx, assign->value, sref_from_instruction(inst_basetype));
    }

    InstructionIndex inst_store = sir_builder_add(builder, SIR_store, source_idx, idx_ast);
    SIrStore *store = sir_builder_push_data(builder, inst_store, SIrStore);
    *store = (SIrStore){
      .dst = lhs_ptr,
      .value = value,
    };

    return sref_from_instruction(inst_store);
  } break;

  case Ast_index: {
    AstIndexData *index = ast_data(ast, idx_ast);
    SRef indexable = gen_code(gen, ctx, index->indexable, (SRef){0});
    SRef index_at = gen_code(gen, ctx, index->index_at, sref_from_value(ctx->common->val.usize));

    InstructionIndex inst = sir_builder_add(builder, SIR_index, source_idx, idx_ast);
    SIrBinary *data = sir_builder_push_data(builder, inst, SIrBinary);
    *data = (SIrBinary){
      .lhs = indexable,
      .rhs = index_at,
    };

    return sir_builder_add_as(&gen->builder, type_destination, sref_from_instruction(inst), source_idx, idx_ast);
  } break;

  case Ast_unary_op: {
    AstUnaryOp *unary = ast_data(ast, idx_ast);
    UnaryOpKind op = unary->op_kind;

    SRef unary_type_destination = (SRef){0};
    switch (op) {
    case Negate: unary_type_destination = (SRef){0}; break;
    case Logical_not: unary_type_destination = sref_from_value(ctx->common->val._bool); break;
    case Bit_not: Todo(); break;
    }

    SRef e = gen_code(gen, ctx, unary->value, unary_type_destination);

    u8 sir_op;
    switch (op) {
    case Negate: sir_op = SIR_negate; break;
    case Logical_not: sir_op = SIR_not; break;
    case Bit_not: Todo(); break;
    }

    InstructionIndex inst = sir_builder_add(builder, sir_op, source_idx, idx_ast);
    sir_builder_set_data(builder, inst, sref_to_u32(e));

    return sir_builder_add_as(&gen->builder, type_destination, sref_from_instruction(inst), source_idx, idx_ast);
  } break;

  case Ast_binary_op: {
    AstBinaryOp *binary = ast_data(ast, idx_ast);

    if (binary->op_kind == Logical_and || binary->op_kind == Logical_or) {
      SRef lhs = gen_code(gen, ctx, binary->lhs, sref_from_value(ctx->common->val._bool));
      InstructionIndex block = sir_builder_add(builder, SIR_block, source_idx, idx_ast);

      u8 op = (binary->op_kind == Logical_and) ? SIR_logical_and : SIR_logical_or;

      InstructionIndex inst = sir_builder_add(builder, op, source_idx, idx_ast);
      SIrLogicalAndOr *data = sir_builder_push_data(builder, inst, SIrLogicalAndOr);
      *data = (SIrLogicalAndOr){
        .block = block,
        .lhs = lhs,
      };

      SRef rhs = gen_code(gen, ctx, binary->rhs, sref_from_value(ctx->common->val._bool));

      sir_builder_end_block_with(builder, block, block, rhs, source_idx, binary->rhs);

      return sref_from_instruction(block);
    }

    SRef lhs_type_destination, rhs_type_destination;
    binary_op_type_destinations(ctx, binary->op_kind, type_destination, &lhs_type_destination, &rhs_type_destination);

    SRef lhs = gen_code(gen, ctx, binary->lhs, lhs_type_destination);
    SRef rhs = gen_code(gen, ctx, binary->rhs, rhs_type_destination);

    if (binary_op_is_cmp(binary->op_kind) || sref_is_nil(type_destination)) {
      InstructionIndex inst_typeof_lhs = sir_builder_add(builder, SIR_typeof, source_idx, binary->lhs);
      sir_builder_set_data(builder, inst_typeof_lhs, sref_to_u32(lhs));

      InstructionIndex inst_typeof_rhs = sir_builder_add(builder, SIR_typeof, source_idx, binary->rhs);
      sir_builder_set_data(builder, inst_typeof_rhs, sref_to_u32(rhs));

      InstructionIndex inst_unify = sir_builder_add(builder, SIR_unify, source_idx, idx_ast);
      SIrUnify *unify = sir_builder_push_data(builder, inst_unify, SIrUnify);
      *unify = (SIrUnify){
        .type_lhs = sref_from_instruction(inst_typeof_lhs),
        .type_rhs = sref_from_instruction(inst_typeof_rhs),
      };

      SRef unified_type = sref_from_instruction(inst_unify);

      lhs = sir_builder_add_as(builder, unified_type, lhs, source_idx, binary->lhs);
      rhs = sir_builder_add_as(builder, unified_type, rhs, source_idx, binary->rhs);
    }
    
    InstructionIndex x = gen_code_for_binary_op(&gen->builder, binary->op_kind, lhs, rhs, source_idx, idx_ast);

    return sir_builder_add_as(builder, type_destination, sref_from_instruction(x), source_idx, idx_ast);
  } break;

  case Ast_while: { 
    AstWhile *ast_while = ast_data(ast, idx_ast);

    InstructionIndex loop = sir_builder_add(builder, SIR_loop, source_idx, idx_ast);

    SRef cond_val = gen_code(gen, ctx, ast_while->cond, sref_from_value(ctx->common->val._bool));

    InstructionIndex condbr_inst = sir_builder_add(builder, SIR_condbr, source_idx, ast_while->cond);
    SIrCondbr *condbr_data = sir_builder_push_data(builder, condbr_inst, SIrCondbr);

    ScopeKey scope_key;
    {
      StringIndex loop_label = intern_ast_label_string(ctx, gen->source, ast_while->label);
      SymbolsPushScopeResult res = symbols_push_scope(syms, ScopeEntry_loop, loop, loop_label);
      if (res.err != SYMBOLS_OK) {
        Todo();
      }

      scope_key = res.data.scope_key;
    }

    SRef body = gen_code(gen, ctx, ast_while->body, (SRef){0});
    Unused(body);

    InstructionIndex repeat_inst = sir_builder_add(builder, SIR_repeat, source_idx, idx_ast);
    sir_builder_set_data(builder, repeat_inst, loop);

    InstructionIndex exit_br_inst = sir_builder_add(builder, SIR_br, source_idx, ast_while->cond);
    SIrBr *exit_br_data = sir_builder_push_data(builder, exit_br_inst, SIrBr);
    *exit_br_data = (SIrBr){
      .block = loop,
      .value = (SRef){0},
    };

    sir_builder_set_data(builder, loop, sir_builder_offset(builder, loop));

    *condbr_data = (SIrCondbr){
      .cond = cond_val,
      .otherwise = exit_br_inst,
    };

    symbols_pop_scope(syms, scope_key);

    return sref_from_instruction(loop);
  } break;

  case Ast_break: {
    AstBreak *b = ast_data(ast, idx_ast);

    InstructionIndex target;

    if (b->label) {
      StringIndex label = intern_ast_label_string(ctx, source, b->label);
      ScopeEntry *entry = symbols_find(syms, label);
      if (!entry) {
        Todo();
      }

      target = entry->inst;
    } else {
      ScopeEntry *entry = symbols_get_enclosing_loop(syms);
      if (!entry) {
        Todo();
      }

      target = entry->inst;
    }

    SRef value = b->value ? gen_code(gen, ctx, b->value, (SRef){0}) : (SRef){0};

    InstructionIndex br = sir_builder_add(builder, SIR_br, source_idx, idx_ast);
    SIrBr *data = sir_builder_push_data(builder, br, SIrBr);
    *data = (SIrBr){
      .block = target,
      .value = value,
    };

    return sref_from_instruction(br);
  } break;
  }

  Unreachable();
}

b32 generate_code(CodeGenContext *ctx, Declaration *decl) {
  Source *source = decl->data.decl.source;

  CodeGen gen;
  b32 ok = codegen_init(&gen, ctx, source, decl->data.decl.tree_idx);
  if (!ok) {
    Todo();
  }

  AstIndex ast_idx_decl = source->decls[decl->data.decl.tree_idx].node;
  AstDeclaration *ast_decl = ast_data(&source->ast, ast_idx_decl);

  Assert(gen.source->ast.kinds[ast_idx_decl] == Ast_declaration);

  SIrBuilder *builder = &gen.builder;
  InstructionIndex block_decl = sir_builder_add(builder, SIR_comptime_block, source->idx, ast_idx_decl);

  SRef decl_type;
  {
    InstructionIndex block = sir_builder_add(builder, SIR_comptime_block, source->idx, ast_decl->type);

    SRef ref_decl_type = (SRef){0};
    if (ast_decl->type) {
      ref_decl_type = gen_code(&gen, ctx, ast_decl->type, sref_from_value(ctx->common->val.type));
    }

    sir_builder_end_block_with(builder, block, block, ref_decl_type, source->idx, ast_decl->type);
    decl_type = sref_from_instruction(block);
  }
  
  SRef decl_val;
  {
    InstructionIndex block = sir_builder_add(builder, SIR_comptime_block, source->idx, ast_decl->value);

    SRef ref_decl_val = gen_code(&gen, ctx, ast_decl->value, decl_type);

    sir_builder_end_block_with(builder, block, block, ref_decl_val, source->idx, ast_idx_decl);
    decl_val = sref_from_instruction(block);
  }

  sir_builder_end_block_with(builder, block_decl, block_decl, decl_val, source->idx, ast_idx_decl);

  sir_builder_flatten(builder, ctx->perm, &decl->data.decl.chunk);

  codegen_deinit(&gen, ctx);

  return !gen.has_error;
}
