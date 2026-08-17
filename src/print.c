#include <string.h>
#include <inttypes.h>

#include "blu.h"
#include "types.h"
#include "value.h"
#include "ir.h"
#include "compiler.h"
#include "source_file.h"
#include "print.h"

static char const *ir_opcode_names[] = {
#define X(k, e, d, name) [k] = name,
#include "x_ir.h"
#undef X
};

void type_index_print(FILE *out, TypeInterner *types, TypeIndex idx) {
  if (idx == 0) {
    fputc('?', out);
    return;
  }

  Type *type = types_get(types, idx);
  switch (Cast(TypeKind, type->kind)) {
  case Type_comptime_int: {
    fputs("comptime_int", out);
  } break;
  case Type_integer: {
    fprintf(out, "%c%u", (type->data.integer.signedness == Signed) ? 'i' : 'u', type->data.integer.bitwidth);
  } break;
  case Type_bool: {
    fputs("bool", out);
  } break;
  case Type_function: {
    fputs("(", out);
    for (u32 i = 0; i < type->data.function.param_count; i++) {
      if (i != 0) {
        fputs(", ", out);
      }
      type_index_print(out, types, type->data.function.param_types[i]);
    }
    fputs(") ", out);
    type_index_print(out, types, type->data.function.return_type);
  } break;
  case Type_nil: {
    fputs("nil", out);
  } break;
  case Type_never: {
    fputs("never", out);
  } break;
  case Type_slice: {
    fputs("[]", out);
    type_index_print(out, types, type->data.slice.base_type);
  } break;
  case Type_array: {
    fprintf(out, "[%llu]", Cast(unsigned long long, type->data.array.size));
    type_index_print(out, types, type->data.array.base_type);
  } break;
  case Type_type: {
    fputs("type", out);
  } break;
  }
}

internal i64 read_signed(u16 bitwidth, void *data) {
  i64 res = 0;
  switch (bitwidth) {
  case  8: { i8  x; memcpy(&x, data, 1); res = x; } break;
  case 16: { i16 x; memcpy(&x, data, 2); res = x; } break;
  case 32: { i32 x; memcpy(&x, data, 4); res = x; } break;
  case 64: { i64 x; memcpy(&x, data, 8); res = x; } break;
  }
  return res;
}

internal u64 read_unsigned(u16 bitwidth, void *data) {
  u64 res = 0;
  memcpy(&res, data, bitwidth / 8);
  return res;
}

void value_print(FILE *out, Compiler *compiler, ValueIndex idx) {
  Value *value = values_get(&compiler->values, idx);
  Type  *type  = types_get(&compiler->types, value->type);
  switch (Cast(TypeKind, type->kind)) {
  case Type_comptime_int: {
    fprintf(out, "%lld", Cast(long long, read_signed(64, value->data)));
  } break;
  case Type_integer: {
    if (type->data.integer.signedness == Signed) {
      fprintf(out, "%" PRId64, Cast(i64, read_signed(type->data.integer.bitwidth, value->data)));
    } else {
      fprintf(out, "%" PRIu64, Cast(u64, read_unsigned(type->data.integer.bitwidth, value->data)));
    }
  } break;
  case Type_bool: {
    fputs((*Cast(u8 *, value->data)) ? "true" : "false", out);
  } break;
  case Type_type: {
    type_index_print(out, &compiler->types, *Cast(TypeIndex *, value->data));
  } break;
  case Type_function: {
    ValueFunc *func = value->data;
    fputs("\n", out);
    ir_chunk_print(out, compiler, &func->chunk);
  } break;
  case Type_nil:
  case Type_never:
  case Type_slice:
  case Type_array: {
    fprintf(out, "$%u", idx);
  } break;
  }
}

internal void ir_ref_print(FILE *out, Compiler *compiler, IrRef ref) {
  if (ref_is_nil(ref)) {
    fprintf(out, "_");
    return;
  }

  if (ref_is_value_index(ref)) {
    value_print(out, compiler, ref_to_value_index(ref));
  } else {
    fprintf(out, "%%%u", ref_to_instruction_index(ref));
  }
}

internal char const* typekind_string(u8 kind) {
  switch (Cast(TypeKind, kind)) {
  case Type_comptime_int: return "comptime_int";
  case Type_integer:      return "integer";
  case Type_bool:         return "bool";
  case Type_function:     return "function";
  case Type_nil:          return "nil";
  case Type_never:        return "never";
  case Type_slice:        return "slice";
  case Type_array:        return "array";
  case Type_type:         return "type";
  }

  return "<invalid>";
}

typedef struct {
  u32 count;
  u32 at;
} BlockPrint;

void ir_chunk_print(FILE *out, Compiler *compiler, IrChunk *chunk) {
  BlockPrint buf[64];
  Stack(BlockPrint) blocks;
  stack_init(&blocks, buf, 64);

  for (InstructionIndex i = 0; i < chunk->opcode_count; i++) {
    u8  op   = chunk->opcodes[i];
    u32 data = chunk->data[i];

    u32 depth = blocks.len;

    if (!stack_is_empty(&blocks)) {
      BlockPrint *b = stack_peek_ptr_unchecked(&blocks);
      b->at += 1;
    }

    while (!stack_is_empty(&blocks)) {
      BlockPrint *b = stack_peek_ptr_unchecked(&blocks);
      if (b->at < b->count) {
        break;
      }

      u32 count = b->count;
      stack_pop_unchecked(&blocks);

      if (!stack_is_empty(&blocks)) {
        stack_peek_ptr_unchecked(&blocks)->at += count;
      }
    }

    fprintf(out, "%4u | %*s%s ", i, depth * 2, "", ir_opcode_names[op]);

    void *extra = ptr_offset(chunk->extra, data);

    switch (Cast(IrOpcode, op)) {
    case IR_nop: break;
    case IR_builtin_debug: {
      ir_ref_print(out, compiler, (IrRef){data});
    } break;
    case IR_func: {
      IrFunc *func = extra;
      fprintf(out, "param_count=%u instruction_count=%u return_type=", func->param_count, func->instruction_count);
      ir_ref_print(out, compiler, func->return_type);
      stack_push(&blocks, ((BlockPrint){ .count = func->instruction_count - 1, .at = 0 }));
    } break;
    case IR_alloc: {
      type_index_print(out, &compiler->types, data);
    } break;
    case IR_condbr: {
      IrCondBr *cond_br = extra;
      fputs("cond=", out);
      ir_ref_print(out, compiler, cond_br->cond);
      fprintf(out, " then=%%%u otherwise=%%%u", cond_br->then, cond_br->otherwise);
    } break;
    case IR_block:
    case IR_eval_block:
    case IR_loop: {
      fprintf(out, "count=%u", data);
      stack_push(&blocks, ((BlockPrint){ .count = data - 1, .at = 0 }));
    } break;
    case IR_br: {
      IrBr *br = extra;
      fprintf(out, "block=%%%u ", br->block);
      ir_ref_print(out, compiler, br->value);
    } break;
    case IR_repeat: {
      fprintf(out, "%%%u", data);
    } break;
    case IR_param_type: {
      IrParamType *p = extra;
      ir_ref_print(out, compiler, p->function);
      fprintf(out, " %u", p->param_index);
    } break;
    case IR_param:
    case IR_ret:
    case IR_load:
    case IR_typeof:
    case IR_return_type: {
      ir_ref_print(out, compiler, (IrRef){data});
    } break;
    case IR_store: {
      IrStore *store = extra;
      fputs("dst=", out);
      ir_ref_print(out, compiler, store->dst);
      fputs(" value=", out);
      ir_ref_print(out, compiler, store->value);
    } break;
    case IR_call: {
      IrCall *call = extra;
      fprintf(out, "$%u", ref_to_u32(call->func));
      if (call->arg_count) {
        fputs(" ", out);
      }
      for (u32 j = 0; j < call->arg_count; j++) {
        if (j != 0) {
          fputs(", ", out);
        }
        ir_ref_print(out, compiler, call->args[j]);
      }
    } break;
    case IR_declaration: {
      IrDeclaration *decl = extra;
      fputs("type=", out);
      ir_ref_print(out, compiler, decl->declared_type);
      fputs(" value=", out);
      ir_ref_print(out, compiler, decl->value);
    } break;
    case IR_lookup_typeof:
    case IR_lookup_value: {
      fprintf(out, "decl=%u", data);
    } break;
    case IR_as: {
      IrAs *as = extra;
      ir_ref_print(out, compiler, as->type_to);
      fputs(" ", out);
      ir_ref_print(out, compiler, as->val);
    } break;
    case IR_unify: {
      IrUnify *unify = extra;
      ir_ref_print(out, compiler, unify->type_lhs);
      fputs(" ", out);
      ir_ref_print(out, compiler, unify->type_rhs);
    } break;
    case IR_type: {
      IrType *type = extra;
      fprintf(out, "%s ", typekind_string(type->kind));
      if (type->kind == Type_function) {
        Assert(type->arg_count > 0);

        fputs("(", out);
        if (type->arg_count > 1) {
          for (u32 j = 1; j < type->arg_count; j++) {
            if (j != 1) {
              fputs(", ", out);
            }
            ir_ref_print(out, compiler, type->args[j]);
          }
        }
        fputs(") ", out);
        ir_ref_print(out, compiler, type->args[type->arg_count-1]);
      } else {
        for (u32 j = 0; j < type->arg_count; j++) {
          if (j != 0) {
            fputs(", ", out);
          }
          ir_ref_print(out, compiler, type->args[j]);
        }
      }
    } break;
    }

    fputs(" ; ", out);
    if (chunk->sources[i].source_idx != 0 && chunk->sources[i].ast_idx != 0) {
      AstIndex ast_idx = chunk->sources[i].ast_idx;
      Source *source = compiler_get_source(compiler, chunk->sources[i].source_idx);

      SpanToken span = source->ast.spans[ast_idx];

      u32 start = source->tokens.spans[span.start].start;
      u32 end   = source->tokens.spans[span.end - 1].end;

      u8 const *s = source->text.str + start;

      b32 has_newline = False;
      u32 len = Min(end-start, 40);
      for (u32 j = 0; j < len; j++) {
        if (s[j] == '\n') {
          fputs("...", out);
          has_newline = True;
          break;
        }

        fputc(s[j], out);
      }

      if (!has_newline && (end-start) > 40) {
        fputs("...", out);
      }

      fputs("\n", out);
    } else {
      fputs("_\n", out);
    }
  }
}

void print_tokens(Tokens *tokens, String source) {
  for (u32 i = 0; i < tokens->tok_count; i++) {
    u8      kind = tokens->kinds[i];
    SpanU32 span = tokens->spans[i];

    char const *s = Cast(char const*, source.str + span.start);
    int len = Cast(int, span.end - span.start);

    char const *kind_string = token_kind_string(kind);

    printf("%5u | %5u:%5u - %s - \"%.*s\"\n", i, span.start, span.end, kind_string, len, s);
  }
}

void print_ast_nodes(AstNodes *nodes, Tokens *tokens, String source) {
  Unused(tokens, source);

  for (u32 i = 1; i < nodes->count; i++) {
    u8 kind = nodes->kinds[i];
    String kind_string = ast_kind_string(kind);

    printf("%.*s\n", Cast(int, kind_string.len), kind_string.str);
  }
}
