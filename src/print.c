#include <string.h>
#include <inttypes.h>

#include "blu.h"
#include "types.h"
#include "value.h"
#include "ir.h"
#include "compiler.h"
#include "source_file.h"
#include "print.h"

void print_type(FILE *out, TypeInterner *types, TypeIndex idx) {
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
      print_type(out, types, type->data.function.param_types[i]);
    }
    fputs(") ", out);
    print_type(out, types, type->data.function.return_type);
  } break;
  case Type_nil: {
    fputs("nil", out);
  } break;
  case Type_never: {
    fputs("never", out);
  } break;
  case Type_slice: {
    fputs("[]", out);
    print_type(out, types, type->data.slice.base_type);
  } break;
  case Type_pointer: {
    fputs("*", out);
    print_type(out, types, type->data.pointer.base_type);
  } break;
  case Type_array: {
    fprintf(out, "[%llu]", Cast(unsigned long long, type->data.array.size));
    print_type(out, types, type->data.array.base_type);
  } break;
  case Type_type: {
    fputs("type", out);
  } break;
  }
}

internal i64 read_signed(u16 bitwidth, void *data) {
  i64 res = 0;
  switch (bitwidth) {
  case  8: { i8  x = *Cast(i8*,data);  res = x; } break;
  case 16: { i16 x = *Cast(i16*,data); res = x; } break;
  case 32: { i32 x = *Cast(i32*,data); res = x; } break;
  case 64: { i64 x = *Cast(i64*,data); res = x; } break;
  }
  return res;
}

internal u64 read_unsigned(u16 bitwidth, void *data) {
  u64 res = 0;
  memcpy(&res, data, bitwidth / 8);
  return res;
}

void print_value_raw(FILE *out, Compiler *compiler, u32 flags, TypeIndex type, void *data) {
  print_type(out, &compiler->types, type);
  fputs(" ", out);
  Type *t = types_get(&compiler->types, type);
  switch (Cast(TypeKind, t->kind)) {
  case Type_comptime_int: {
    fprintf(out, "%lld", Cast(long long, read_signed(64, data)));
  } break;
  case Type_integer: {
    if (t->data.integer.signedness == Signed) {
      fprintf(out, "%" PRId64, Cast(i64, read_signed(t->data.integer.bitwidth, data)));
    } else {
      fprintf(out, "%" PRIu64, Cast(u64, read_unsigned(t->data.integer.bitwidth, data)));
    }
  } break;
  case Type_bool: {
    fputs((*Cast(u8 *, data)) ? "true" : "false", out);
  } break;
  case Type_type: {
    print_type(out, &compiler->types, *Cast(TypeIndex *, data));
  } break;
  case Type_function: {
    if (flags & PrintFlag_expand_function) {
      ValueFunc *func = data;
      fprintf(out, "0x%p\n", data);
      print_iir_chunk(out, compiler, &func->chunk);
    } else {
      fprintf(out, "0x%p", data);
    }
  } break;
  case Type_pointer:
  case Type_nil:
  case Type_never:
  case Type_slice:
  case Type_array: {
    fprintf(out, "0x%p", data);
  } break;
  }
}

internal void print_sref(FILE *out, Compiler *compiler, SRef ref) {
  if (sref_is_nil(ref)) {
    fprintf(out, "_");
    return;
  }

  if (sref_is_value(ref)) {
    Value *value = values_get(&compiler->values, sref_to_value(ref));
    print_value_raw(out, compiler, 0, value->type, value->data);
  } else {
    fprintf(out, "%%%u", sref_to_instruction(ref));
  }
}

internal void print_iref(FILE *out, Compiler *compiler, IRef ref) {
  if (iref_is_nil(ref)) {
    fprintf(out, "_");
    return;
  }

  if (iref_is_value(ref)) {
    Value *value = values_get(&compiler->values, iref_to_value(ref));
    print_value_raw(out, compiler, 0, value->type, value->data);
  } else {
    fprintf(out, "%%%u", iref_to_instruction(ref));
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
  case Type_pointer:      return "pointer";
  }

  return "<invalid>";
}

internal void print_inst_source(FILE *out, Compiler *compiler, AstAndSourceIndex src) {
  fputs(" ; ", out);

  if (src.source_idx == 0 || src.ast_idx == 0) {
    fputs("_\n", out);
    return;
  }

  Source *source = compiler_get_source(compiler, src.source_idx);

  SpanToken span = source->ast.spans[src.ast_idx];

  u32 start = source->tokens.spans[span.start].start;
  u32 end   = source->tokens.spans[span.end - 1].end;

  u8 const *s = source->text.str + start;

  b32 has_newline = False;
  u32 len = Min(end-start, 40);
  for (u32 j = 0; j < len; j++) {
    if (s[j] == '\n') {
      fputs(" ...", out);
      has_newline = True;
      break;
    }

    fputc(s[j], out);
  }

  if (!has_newline && (end-start) > 40) {
    fputs(" ...", out);
  }

  fputs("\n", out);
}

static char const *sir_opcode_names[] = {
#define X(k, name, a, b) [k] = name,
#include "x_sir.h"
#undef X
};

typedef struct {
  u32 count;
  u32 at;
} BlockPrint;

void print_sir_chunk(FILE *out, Compiler *compiler, SIrChunk *chunk) {
  BlockPrint buf[64];
  Stack(BlockPrint) blocks;
  stack_init(&blocks, buf, 64);

  for (InstructionIndex i = 0; i < chunk->opcode_count; i++) {
    SIrOpcode  op   = sir_chunk_op(chunk, i);

    u32 data = sir_chunk_data(chunk, i);

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

    fprintf(out, "%4u | \033[1m%*s%s\033[22m ", i, depth * 2, "", sir_opcode_names[op]);

    void *extra = sir_chunk_extra(chunk, i);

    switch (op) {
    case SIR_func: {
      SIrFunc *func = extra;
      fprintf(out, "param_count=%u instruction_count=%u return_type=", func->param_count, func->instruction_count);
      print_sref(out, compiler, func->return_type);
      stack_push(&blocks, ((BlockPrint){ .count = func->instruction_count - 1, .at = 0 }));
    } break;

    case SIR_condbr: {
      SIrCondbr *cond_br = extra;
      fputs("cond=", out);
      print_sref(out, compiler, cond_br->cond);
      fprintf(out, " then=%%%u otherwise=%%%u", cond_br->then, cond_br->otherwise);
    } break;

    case SIR_index:
    case SIR_and:
    case SIR_or:
    case SIR_mul:
    case SIR_div:
    case SIR_mod:
    case SIR_sub:
    case SIR_add:
    case SIR_cmp_eq:
    case SIR_cmp_ne:
    case SIR_cmp_gt:
    case SIR_cmp_ge:
    case SIR_cmp_lt:
    case SIR_cmp_le: {
      SIrBinary *binary = extra;
      print_sref(out, compiler, binary->lhs);
      fputs(", ", out);
      print_sref(out, compiler, binary->rhs);
    } break;

    case SIR_negate:
    case SIR_not: {
      print_sref(out, compiler, (SRef){data});
    } break;

    case SIR_block:
    case SIR_comptime_block:
    case SIR_loop: {
      fprintf(out, "count=%u", data);
      stack_push(&blocks, ((BlockPrint){ .count = data - 1, .at = 0 }));
    } break;

    case SIR_br: {
      SIrBr *br = extra;
      fprintf(out, "block=%%%u ", br->block);
      print_sref(out, compiler, br->value);
    } break;

    case SIR_repeat: {
      fprintf(out, "%%%u", data);
    } break;

    case SIR_param_type: {
      SIrParamType *p = extra;
      print_sref(out, compiler, p->function);
      fprintf(out, " %u", p->param_index);
    } break;

    case SIR_param:
    case SIR_builtin_debug:
    case SIR_ret:
    case SIR_load:
    case SIR_typeof:
    case SIR_base_type:
    case SIR_alloc:
    case SIR_comptime_alloc:
    case SIR_return_type: {
      print_sref(out, compiler, (SRef){data});
    } break;

    case SIR_store: {
      SIrStore *store = extra;
      fputs("dst=", out);
      print_sref(out, compiler, store->dst);
      fputs(" value=", out);
      print_sref(out, compiler, store->value);
    } break;

    case SIR_call: {
      SIrCall *call = extra;
      print_sref(out, compiler, call->func);
      if (call->arg_count) {
        fputs(" ", out);
      }
      for (u32 j = 0; j < call->arg_count; j++) {
        if (j != 0) {
          fputs(", ", out);
        }
        print_sref(out, compiler, call->args[j]);
      }
    } break;

    case SIR_lookup_decl_type:
    case SIR_lookup_decl_value: {
      fprintf(out, "decl=%u", data);
    } break;

    case SIR_as: {
      SIrAs *as = extra;
      print_sref(out, compiler, as->type_to);
      fputs(" ", out);
      print_sref(out, compiler, as->val);
    } break;

    case SIR_unify: {
      SIrUnify *unify = extra;
      print_sref(out, compiler, unify->type_lhs);
      fputs(" ", out);
      print_sref(out, compiler, unify->type_rhs);
    } break;

    case SIR_type: {
      SIrType *type = extra;
      fprintf(out, "%s ", typekind_string(type->kind));
      if (type->kind == Type_function) {
        Assert(type->arg_count > 0);

        fputs("(", out);
        if (type->arg_count > 1) {
          for (u32 j = 1; j < type->arg_count; j++) {
            if (j != 1) {
              fputs(", ", out);
            }
            print_sref(out, compiler, type->args[j]);
          }
        }
        fputs(") ", out);
        print_sref(out, compiler, type->args[type->arg_count-1]);
      } else {
        for (u32 j = 0; j < type->arg_count; j++) {
          if (j != 0) {
            fputs(", ", out);
          }
          print_sref(out, compiler, type->args[j]);
        }
      }
    } break;
    }

    print_inst_source(out, compiler, chunk->sources[i]);
  }
}

static char const *iir_opcode_names[] = {
#define X(k, name, a, b) [k] = name,
#include "x_iir.h"
#undef X
};

void print_iir_chunk(FILE *out, Compiler *compiler, IIrChunk *chunk) {
  BlockPrint buf[64];
  Stack(BlockPrint) blocks;
  stack_init(&blocks, buf, 64);

  for (InstructionIndex i = 0; i < chunk->opcode_count; i++) {
    IIrOpcode op = iir_chunk_op(chunk, i);

    u32 data = iir_chunk_data(chunk, i);

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

    fprintf(out, "%4u | \033[1m%*s%s\033[22m", i, depth * 2, "", iir_opcode_names[op]);

    switch (op) {
    case IIR_func:
    case IIR_block:
    case IIR_loop: {
      fprintf(out, " count=%u", data);
      stack_push(&blocks, ((BlockPrint){ .count = data - 1, .at = 0 }));
    } break;

    case IIR_param:
    case IIR_alloc: break;

    case IIR_load:
    case IIR_ret:
    case IIR_builtin_debug: {
      fputs(" ", out);
      print_iref(out, compiler, (IRef){data});
    } break;

    case IIR_int_add:
    case IIR_int_sub:
    case IIR_int_mul:
    case IIR_int_div: {
      IIrBinary *bin = iir_chunk_extra(chunk, i);
      fputs(" ", out);
      print_iref(out, compiler, bin->lhs);
      fputs(", ", out);
      print_iref(out, compiler, bin->rhs);
    } break;

    case IIR_store: {
      IIrStore *store = iir_chunk_extra(chunk, i);
      fputs(" ptr=", out);
      print_iref(out, compiler, store->ptr);
      fputs(" value=", out);
      print_iref(out, compiler, store->value);
    } break;

    case IIR_condbr: {
      IIrCondbr *condbr = iir_chunk_extra(chunk, i);
      fputs(" cond=", out);
      print_iref(out, compiler, condbr->cond);
      fprintf(out, " then=%%%u otherwise=%%%u", condbr->then, condbr->otherwise);
    } break;

    case IIR_br: {
      IIrBr *br = iir_chunk_extra(chunk, i);
      fprintf(out, " block=%%%u ", br->block);
      print_iref(out, compiler, br->value);
    } break;

    case IIR_repeat: {
      fprintf(out, " %%%u", data);
    } break;

    case IIR_call: {
      IIrCall *call = iir_chunk_extra(chunk, i);
      fputs(" ", out);
      print_iref(out, compiler, call->func_ptr);
      if (call->arg_count) {
        fputs(" ", out);
      }
      for (u32 j = 0; j < call->arg_count; j++) {
        if (j != 0) {
          fputs(", ", out);
        }
        print_iref(out, compiler, call->args[j]);
      }
    } break;
    }

    fputs(" : ", out);
    print_type(out, &compiler->types, iir_chunk_type(chunk, i));

    print_inst_source(out, compiler, chunk->sources[i]);
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

