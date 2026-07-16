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

InstructionIndex inst_add_lookup(CodeGen *gen, TokenIndex name) {
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

  return lookup;
}

InstructionIndex inst_add_const_type(CodeGen *gen, TypeIndex type) {
  Value *v;
  ValueIndex idx = values_alloc(gen->values, &v);
  TypeIndex *data = values_alloc_data(gen->values, sizeof(TypeIndex), Align_of(TypeIndex));
  *data = type;
  *v = (Value){
    .type = gen->common->type.type,
    .data_size = sizeof(TypeIndex),
    .data = data,
  };

  InstructionIndex idx_const = inst_alloc(gen);
  inst_set_kind(gen, idx_const, IR_const);
  inst_set_data(gen, idx_const, idx);

  return idx_const;
}

internal void inst_add_check_coerce(CodeGen *gen, TypeIndex type_destination, InstructionIndex idx_type_from) {
  InstructionIndex idx_const_type = inst_add_const_type(gen, type_destination);

  InstructionIndex idx_check = inst_alloc(gen);
  inst_set_kind(gen, idx_check, IR_check_coerce);

  IrCheckCoerce *data = inst_push_data(gen, idx_check, IrCheckCoerce);
  *data = (IrCheckCoerce){
    .type_to   = idx_const_type,
    .type_from = idx_type_from,
  };
}

InstructionIndex gen_code(CodeGen *gen, AstIndex idx_ast, TypeIndex type_destination) {
  String    text   = gen->source->text;
  Tokens   *tokens = &gen->source->tokens;
  AstNodes *ast    = &gen->source->ast;

  u8 kind = ast->kinds[idx_ast];
  switch (kind) {
  case Ast_identifier: {
    TokenIndex *name = ast_data(ast, idx_ast);
    InstructionIndex idx_lookup = inst_add_lookup(gen, *name);
    if (type_destination) {
      inst_add_check_coerce(gen, type_destination, idx_lookup);
    }
    return idx_lookup;
  } break;
  case Ast_type_function: {
    AstTypeFunction *func = ast_data(ast, idx_ast);
    InstructionIndex idx_ret_type = gen_code(gen, func->return_type, gen->common->type.type);

    Assert(func->count == 0);

    for (u32 i = 0; i < func->count; i++) {
      // TODO Add the parameter types
    }

    Panic();
  } break;
  case Ast_function: {
    AstFunction *func = ast_data(ast, idx_ast);
    
    InstructionIndex idx_ret_type = 0;
    if (func->return_type) {
      idx_ret_type = gen_code(gen, func->return_type, gen->common->type.type);
    }

    Assert(func->count == 0);

    // - get all the parameter types (if they are present)
    // - construct a function type
    // - check unify with this function type and the type_destination
    // - type destination of the body becomes the return type of the unified type
    // - output emit block?
    // - generate body and return
  } break;
  case Ast_declaration: {
    AstDeclaration *decl = ast_data(ast, idx_ast);

    InstructionIndex idx_decl = inst_alloc(gen);

    inst_set_kind(gen, idx_decl, IR_declaration);

    IrDeclaration *data_decl = inst_push_data(gen, idx_decl, IrDeclaration);

    InstructionIndex idx_type = inst_add_const_type(gen, gen->common->type.type);
    InstructionIndex idx_decl_type = 0;
    if (decl->type) {
      idx_decl_type = gen_code(gen, decl->type, idx_type);
    }
    InstructionIndex idx_decl_val = gen_code(gen, idx_decl_type, idx_decl_type);

    data_decl->declared_type = idx_decl_type;
    data_decl->value = idx_decl_val;

    return idx_decl;
  } break;
  }
}

b32 source_generate_code(CodeGenContext *context, Source *source, u32 idx) {
  CodeGen gen;
  codegen_init(&gen, context, source, idx);

  InstructionIndex inst_idx = gen_code(&gen, source->decls[source->tree_idxs[idx]].node, context->common->type.nil);

  // TODO
  // - flatten all the data and store it somewhere.
  // - output all the dependencies somewhere.

  codegen_deinit(&gen);

  return !gen.has_error;
}
