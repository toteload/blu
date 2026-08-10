#include <string.h>

#include "blu.h"
#include "types.h"
#include "value.h"
#include "ir.h"
#include "source_file.h"

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

void value_print(FILE *out, Source *source, TypeInterner *types, ValueStore *values, ValueIndex idx) {
  Value *value = values_get(values, idx);
  Type  *type  = types_get(types, value->type);
  switch (Cast(TypeKind, type->kind)) {
  case Type_comptime_int: {
    fprintf(out, "%lld", Cast(long long, read_signed(64, value->data)));
  } break;
  case Type_integer: {
    if (type->data.integer.signedness == Signed) {
      fprintf(out, "%lld", Cast(long long, read_signed(type->data.integer.bitwidth, value->data)));
    } else {
      fprintf(out, "%llu", Cast(unsigned long long, read_unsigned(type->data.integer.bitwidth, value->data)));
    }
  } break;
  case Type_bool: {
    fputs((*Cast(u8 *, value->data)) ? "true" : "false", out);
  } break;
  case Type_type: {
    type_index_print(out, types, *Cast(TypeIndex *, value->data));
  } break;
  case Type_function: {
    ValueFunc *func = value->data;
    fputs("\n", out);
    ir_chunk_print(out, &func->chunk, source, types, values);
  } break;
  case Type_nil:
  case Type_never:
  case Type_slice:
  case Type_array: {
    fprintf(out, "$%u", idx);
  } break;
  }
}

internal void ir_ref_print(FILE *out, IrRef ref, Source *source, TypeInterner *types, ValueStore *values) {
  if (ref_is_nil(ref)) {
    fprintf(out, "_");
    return;
  }

  if (ref_is_value_index(ref)) {
    value_print(out, source, types, values, ref_to_value_index(ref));
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

typedef struct {
  u32    line;
  u32    col;
  String text;
} AstSourceInfo;

internal b32 is_whitespace(u8 c) {
  return (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n');
}

internal void ast_index_source_info(Source *source, AstIndex idx, AstSourceInfo *out) {
  if (!source || idx == 0 || idx >= source->ast.count) {
    *out = (AstSourceInfo){
      .line = 0,
      .col = 0,
      .text = string_lit("?"),
    };
    return;
  }

  SpanToken span = source->ast.spans[idx];

  u32 start = source->tokens.spans[span.start].start;
  u32 end   = source->tokens.spans[span.end - 1].end;

  while (end > start && is_whitespace(source->text.str[end - 1])) {
    end -= 1;
  }

  LineInfo info = tokens_find_line_info(&source->tokens, start);

  *out = (AstSourceInfo){
    .line = info.line,
    .col  = start - info.offset_start_of_line + 1,
    .text = {.str = source->text.str + start, .len = end - start},
  };
}

internal void ast_source_text_print(FILE *out, String text) {
  usize max = 56;
  usize len = Min(text.len, max);

  for (usize i = 0; i < len; i++) {
    u8 c = text.str[i];
    fputc(is_whitespace(c) ? ' ' : c, out);
  }

  if (text.len > max) {
    fputs("...", out);
  }
}

void ir_chunk_print(FILE *out, IrChunk *chunk, Source *source, TypeInterner *types, ValueStore *values) {
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
    case IR_func: {
      IrFunc *func = extra;
      fprintf(out, "param_count=%u instruction_count=%u return_type=", func->param_count, func->instruction_count);
      ir_ref_print(out, func->return_type, source, types, values);
      stack_push(&blocks, ((BlockPrint){ .count = func->instruction_count - 1, .at = 0 }));
    } break;
    case IR_alloc: {
      type_index_print(out, types, data);
    } break;
    case IR_condbr: {
      IrCondBr *cond_br = extra;
      fputs("cond=", out);
      ir_ref_print(out, cond_br->cond, source, types, values);
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
      ir_ref_print(out, br->value, source, types, values);
    } break;
    case IR_repeat: {
      fprintf(out, "%%%u", data);
    } break;
    case IR_param_type: {
      IrParamType *p = extra;
      ir_ref_print(out, p->function, source, types, values);
      fprintf(out, " %u", p->param_index);
    } break;
    case IR_param:
    case IR_ret:
    case IR_load:
    case IR_typeof:
    case IR_return_type: {
      ir_ref_print(out, (IrRef){data}, source, types, values);
    } break;
    case IR_store: {
      IrStore *store = extra;
      fputs("dst=", out);
      ir_ref_print(out, store->dst, source, types, values);
      fputs(" value=", out);
      ir_ref_print(out, store->value, source, types, values);
    } break;
    case IR_call: {
      IrCall *call = extra;
      ir_ref_print(out, call->func, source, types, values);
      if (call->arg_count) {
        fputs(" ", out);
      }
      for (u32 j = 0; j < call->arg_count; j++) {
        if (j != 0) {
          fputs(", ", out);
        }
        ir_ref_print(out, call->args[j], source, types, values);
      }
    } break;
    case IR_declaration: {
      IrDeclaration *decl = extra;
      fputs("type=", out);
      ir_ref_print(out, decl->declared_type, source, types, values);
      fputs(" value=", out);
      ir_ref_print(out, decl->value, source, types, values);
    } break;
    case IR_lookup_typeof:
    case IR_lookup_value: {
      fprintf(out, "decl=%u", data);
    } break;
    case IR_as: {
      IrAs *as = extra;
      ir_ref_print(out, as->type_to, source, types, values);
      fputs(" ", out);
      ir_ref_print(out, as->val, source, types, values);
    } break;
    case IR_unify: {
      IrUnify *unify = extra;
      ir_ref_print(out, unify->type_lhs, source, types, values);
      fputs(" ", out);
      ir_ref_print(out, unify->type_rhs, source, types, values);
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
            ir_ref_print(out, type->args[j], source, types, values);
          }
        }
        fputs(") ", out);
        ir_ref_print(out, type->args[type->arg_count-1], source, types, values);
      } else {
        for (u32 j = 0; j < type->arg_count; j++) {
          if (j != 0) {
            fputs(", ", out);
          }
          ir_ref_print(out, type->args[j], source, types, values);
        }
      }
    } break;
    }

    b32 has_ast_info = !is_null(chunk->ast_source);
    AstSourceInfo ast_info;
    ast_index_source_info(source, chunk->ast_source[i], &ast_info);

    if (has_ast_info) {
      fputs(" ; ", out);
      ast_source_text_print(out, ast_info.text);
    }

    fputc('\n', out);
  }
}
