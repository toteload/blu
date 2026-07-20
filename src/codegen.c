#include "blu.h"
#include "value.h"
#include "tokens.h"
#include "messages.h"
#include "string_interner.h"
#include "source_file.h"
#include "compiler.h"
#include "codegen.h"
#include "ir.h"

#define SEGMENTLIST_NAME            KindList
#define SEGMENTLIST_TYPE            u8
#define SEGMENTLIST_FUNCTION_PREFIX kindlist
#define SEGMENTLIST_LINKAGE         internal
#define SEGMENTLIST_MIN_SIZE_LOG2   8
#define SEGMENTLIST_SEGMENT_COUNT   24
#define SEGMENTLIST_OUTPUT_TYPES
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

typedef union {
  u32   data;
  void *ptr;
} InstructionData;

#define SEGMENTLIST_NAME            DataList
#define SEGMENTLIST_TYPE            InstructionData
#define SEGMENTLIST_FUNCTION_PREFIX datalist
#define SEGMENTLIST_LINKAGE         internal
#define SEGMENTLIST_MIN_SIZE_LOG2   8
#define SEGMENTLIST_SEGMENT_COUNT   24
#define SEGMENTLIST_OUTPUT_TYPES
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

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
  KindList kinds;
  DataList datas;
  DependencyList dependencies; // []DeclarationIndex
  ArenaSnapshot scope_scratch;

  Arena *arena;
  Arena *scratch;
  Common *common;
  MessageSink *msg_sink;
  StringInterner *strings;
  DeclarationInterner *decl_keys;
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
    .decl_keys = context->decl_keys,
    .values = context->values,
    .source = source,
  };

  gen->scope_scratch = arena_scope_begin(context->scratch);

  DeclarationIndex *mods = arena_push_array(DeclarationIndex, context->scratch, Max_module_depth);
  u32 i = source->decls[source->tree_idxs[idx]].parent;
  u32 offset = 0;
  while (source->decls[i].kind != SourceDeclaration_root) {
    mods[offset++] = source->decl_idxs[i];
    i = source->decls[i].parent;
  }

  gen->mod_depth = offset;
  gen->mods = mods;
}

void codegen_deinit(CodeGen *gen) {
  arena_scope_end(gen->scratch, gen->scope_scratch);
}

internal InstructionIndex inst_alloc(CodeGen *gen) {
  InstructionIndex idx = gen->kinds.len;
  kindlist_append(&gen->kinds, gen->scratch, 0);
  datalist_append(&gen->datas, gen->scratch, (InstructionData){ .ptr = Null });
  return idx;
}

internal void inst_set_kind(CodeGen *gen, InstructionIndex idx, u8 kind) {
  *kindlist_ptr_at_unchecked(&gen->kinds, idx) = kind;
}

internal void inst_set_data(CodeGen *gen, InstructionIndex idx, u32 data) {
  *datalist_ptr_at_unchecked(&gen->datas, idx) = (InstructionData){ .data = data };
}

internal void *inst_push_data_raw(CodeGen *gen, InstructionIndex idx, u32 size, u32 align) {
  void *p = arena_push(gen->scratch, size, align);
  *datalist_ptr_at_unchecked(&gen->datas, idx) = (InstructionData){ .ptr = p };
  return p;
}

#define inst_push_data(gen,idx,type) inst_push_data_raw(gen, idx, sizeof(type), Align_of(type))

IrRef inst_add_lookup(CodeGen *gen, TokenIndex name) {
  StringIndex str = strings_add(gen->strings, token_string(&gen->source->tokens, gen->source->text, name));
  DeclarationIndex decl;
  b32 found = lookup_identifier(gen->decl_keys, gen->mods, gen->mod_depth, str, &decl);
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

  InstructionIndex lookup = inst_alloc(gen);
  inst_set_kind(gen, lookup, IR_lookup);
  inst_set_data(gen, lookup, decl);

  depslist_append(&gen->dependencies, gen->scratch, decl);

  return ir_ref_from_instruction_index(lookup);
}

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

internal InstructionIndex inst_add_as(CodeGen *gen, IrRef type_destination, IrRef ref_type_from) {
  InstructionIndex idx_as = inst_alloc(gen);
  inst_set_kind(gen, idx_as, IR_as);

  IrAs *data = inst_push_data(gen, idx_as, IrAs);
  *data = (IrAs){
    .type_to   = type_destination,
    .type_from = ref_type_from,
  };

  return idx_as;
}

IrRef gen_code(CodeGen *gen, AstIndex idx_ast, IrRef type_destination) {
  String    text   = gen->source->text;
  Tokens   *tokens = &gen->source->tokens;
  AstNodes *ast    = &gen->source->ast;

  u8 kind = ast->kinds[idx_ast];
  switch (Cast(enum AstKind, kind)) {
  case Ast_identifier: {
    TokenIndex *name = ast_data(ast, idx_ast);
    IrRef ref_lookup = inst_add_lookup(gen, *name);
    if (type_destination) {
      inst_add_as(gen, type_destination, ref_lookup);
    }
    return ref_lookup;
  } break;
  case Ast_type_function: {
    AstTypeFunction *func = ast_data(ast, idx_ast);
    IrRef ref_ret_type = gen_code(gen, func->return_type, gen->common->type.type);

    for (u32 i = 0; i < func->count; i++) {
      // TODO Add the parameter types
      Panic();
    }

    InstructionIndex inst_type = inst_alloc(gen);

    inst_set_kind(gen, inst_type, IR_type);

    u32 arg_count = func->count + 1; // parameters + return type

    IrType *data_type = inst_push_data_raw(gen, inst_type, sizeof(IrType) + arg_count * sizeof(IrRef), Align_of(IrType));
    *data_type = (IrType){
      .kind = Type_function,
      .arg_count = arg_count,
    };

    data_type->args[0] = ref_ret_type;

    return ir_ref_from_instruction_index(inst_type);
  } break;
  case Ast_function: {
    AstFunction *func = ast_data(ast, idx_ast);

    IrRef ref_ret_type = 0;
    if (func->return_type) {
      ref_ret_type = gen_code(gen, func->return_type, gen->common->val.type);
    }

    for (u32 i = 0; i < func->count; i++) {
      Panic();
    }

    InstructionIndex inst_type = inst_alloc(gen);
    inst_set_kind(gen, inst_type, IR_type);
    u32 arg_count = func->count + 1; // parameters + return type

    IrType *data_type = inst_push_data_raw(gen, inst_type, sizeof(IrType) + arg_count * sizeof(IrRef), Align_of(IrType));
    *data_type = (IrType){
      .kind = Type_function,
      .arg_count = arg_count,
    };

    data_type->args[0] = ref_ret_type;

    InstructionIndex inst_unify = inst_alloc(gen);
    inst_set_kind(gen, inst_unify, IR_unify);
    IrUnify *data_unify = inst_push_data(gen, inst_unify, IrUnify);
    *data_unify = (IrUnify){
      .type_lhs = type_destination,
      .type_rhs = ir_ref_from_instruction_index(inst_type),
    };

    InstructionIndex inst_return_type = inst_alloc(gen);
    inst_set_kind(gen, inst_return_type, IR_function_return_type);
    inst_set_data(gen, inst_return_type, inst_unify);

    // ASSUME: you have no early returns in the function

    InstructionIndex inst_body = gen_code(gen, func->body, ir_ref_from_instruction_index(inst_return_type));

    InstructionIndex inst_ret = inst_alloc(gen);
    inst_set_kind(gen, inst_ret, IR_ret);
    inst_set_data(gen, inst_ret, ir_ref_from_instruction_index(inst_body));

    return inst_ret;
  } break;
  case Ast_declaration: {
    AstDeclaration *decl = ast_data(ast, idx_ast);

    InstructionIndex idx_decl = inst_alloc(gen);

    inst_set_kind(gen, idx_decl, IR_declaration);

    IrDeclaration *data_decl = inst_push_data(gen, idx_decl, IrDeclaration);

    IrRef ref_type = ir_ref_from_value_index(gen->common->val.type);
    IrRef ref_decl_type = 0;
    if (decl->type) {
      ref_decl_type = gen_code(gen, decl->type, ref_type);
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

    if (type_destination) {
      inst_add_as(gen, type_destination, ref_literal);
    }

    return ref_literal;
  } break;
  default: Panic();
  }
}

internal b32 opcode_references_extra(u8 op) {
  switch (op) {
  case IR_func:
  case IR_cond_br:
  case IR_store:
  case IR_call:
  case IR_declaration:
  case IR_cast_int:
  case IR_as:
  case IR_type:
    return True;
  default:
    return False;
  }
}

internal u32 extra_payload_size(u8 op, void *payload) {
  switch (op) {
  case IR_func:        return sizeof(IrFunc);
  case IR_cond_br:     return sizeof(IrCondBr);
  case IR_store:       return sizeof(IrStore);
  case IR_declaration: return sizeof(IrDeclaration);
  case IR_cast_int:    return sizeof(IrCastInt);
  case IR_as:          return sizeof(IrAs);
  case IR_type: {
    IrType *type = payload;
    return sizeof(IrType) + type->arg_count * sizeof(IrRef);
  }
  case IR_call: {
    IrCall *call = payload;
    return sizeof(IrCall) + call->arg_count * sizeof(IrRef);
  }
  }

  Panic();
}

internal u32 extra_payload_align(u8 op) {
  switch (op) {
  case IR_func:        return Align_of(IrFunc);
  case IR_cond_br:     return Align_of(IrCondBr);
  case IR_store:       return Align_of(IrStore);
  case IR_call:        return Align_of(IrCall);
  case IR_declaration: return Align_of(IrDeclaration);
  case IR_cast_int:    return Align_of(IrCastInt);
  case IR_as:          return Align_of(IrAs);
  case IR_type:        return Align_of(IrType);
  }

  Panic();
}

internal void *flatten_push_data(void *extra_base, Arena *arena, u32 *data, InstructionIndex i, u32 size, u32 align) {
  void *p = arena_push(arena, size, align);
  u32 offset = ptr_diff(p, extra_base);
  data[i] = offset;
  return p;
}

b32 source_generate_code(CodeGenContext *context, Source *source, u32 idx) {
  CodeGen gen;
  codegen_init(&gen, context, source, idx);

  // Reserve the zero index so instruction index 0 can be used as a null reference.
  inst_alloc(&gen);

  gen_code(&gen, source->decls[source->tree_idxs[idx]].node, context->common->type.nil);

  u32 count = Cast(u32, gen.kinds.len);

  u8  *opcodes = arena_push_array(u8,  context->arena, count);
  u32 *data    = arena_push_array(u32, context->arena, count);

  kindlist_copy_to_array(&gen.kinds, opcodes);

  void *extra = context->arena->at;

  for (InstructionIndex i = 1; i < count; i++) {
    u8 op = opcodes[i];
    InstructionData entry = datalist_at_unchecked(&gen.datas, i);

    if (!opcode_references_extra(op)) {
      data[i] = entry.data;
      continue;
    }

    u32 size  = extra_payload_size(op, entry.ptr);
    u32 align = extra_payload_align(op);
    void *mem = flatten_push_data(extra, context->arena, data, i, size, align);
    memcpy(mem, entry.ptr, size);
  }

  source->ir_chunks[idx] = (IrChunk){
    .opcode_count = count,
    .opcodes      = opcodes,
    .data         = data,
    .extra        = extra,
  };

  // TODO
  // - output all the dependencies somewhere.

  codegen_deinit(&gen);

  return !gen.has_error;
}
