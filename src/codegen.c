#include "blu.h"

typedef struct {
  IrKindList kinds;
  IrDataList datas;
  PtrList    extras;

  Arena *scratch;

  StringInterner *strings;

  MessageSink *sink;

  Source *source;

  u32               mod_depth;
  DeclarationIndex *mods;
} CodeGen;

internal InstructionIndex inst_alloc(CodeGen *gen) {
  InstructionIndex idx = 0; // TODO
  kindlist_append();
  datalist_append();
  ptrlist_append();
  return idx;
}

InstructionIndex gen_code_for_type(CodeGen *code, AstIndex idx_ast) {
  String    text   = code->source->text;
  Tokens   *tokens = &code->source->tokens;
  AstNodes *ast    = &code->source->ast;

  u8 kind = ast->kinds[idx_ast];
  switch (kind) {
  case Ast_identifier: {
    StringIndex str = strings_add(&code->strings, token_string(tokens, text, ...));
    DeclarationIndex decl;
    b32 found = lookup_identifier(compiler, code->mods, code->mod_depth, str, &decl);
    if (!found) {
      Message_error();
      // mark in the codegen that we have encountered an error
      Panic();
    }

    // output a lookup instruction with the found DeclarationIndex
    // record in the codegen a dependency on this declaration

    Panic();
  } break;
  case Ast_type_function: {
    AstTypeFunction *func = ast_data(ast, idx_ast);
    InstructionIndex idx_ret_type = gen_code(code, func->return_type);

    {
      InstructionIndex idx_check = inst_alloc(gen);
      set_kind(gen, idx_check, IR_check_is_coercible);
      IrCheckCoercible *data = ir_push_data(gen, IrCheckCoercible, idx_check);
      *data = (IrCheckCoercible){
        .type_to = 0, /* TODO Add literal type */
        .type_from = idx_ret_type,
      };
    }

    for (u32 i = 0; i < func->count; i++) {
      // TODO Add the parameter types
    }

    


  } break;
  }
}

void source_generate_code_for_declaration(Source *source, u32 idx, Arena *scratch) {
  ArenaSnapshot scope = arena_scope_begin(scratch);

  u32 mod_depth;
  DeclarationIndex *mods = arena_push_array(DeclarationIndex, scratch, Max_module_depth);
  {
    u32 i = source->decls[source->tree_idx[idx]].parent;
    u32 offset = 0;
    while (source->decls[i].kind != SourceDeclaration_root) {
      mods[offset++] = source->decl_idxs[i];
      i = source->decls[i].parent;
    }
    mod_depth = offset;
  }

  InstructionIndex idx_decl = inst_alloc(gen);

  set_kind(gen, idx_decl, IR_declaration);

  InstructionIndex idx_decl_type = gen_code_for_type();
  InstructionIndex idx_decl_val = gen_code();

  IrDeclaration *data_decl = ir_push_data(gen, IrDeclaration, idx_decl);
  data_decl->declared_type = idx_decl_type;
  data_decl->value = idx_decl_val;

  // TODO
  // - flatten all the data and store it in the source arena. write out to the passed in chunk.
  // - output all the dependencies somewhere.

  arena_scope_end(scratch, scope);
}
