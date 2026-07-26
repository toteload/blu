#include <string.h>

#include "blu.h"
#include "types.h"
#include "value.h"
#include "ir.h"

static char const *ir_opcode_names[] = {
#define X(k, e, d, name) [k] = name,
#include "x_ir.h"
#undef X
};

internal void type_index_print(FILE *out, TypeInterner *types, TypeIndex idx) {
  if (idx == 0) {
    fputc('?', out);
    return;
  }

  Type *type = types_get(types, idx);
  switch (Cast(enum TypeKind, type->kind)) {
  case Type_comptime_int: {
    fputs("comptime_int", out);
  } break;
  case Type_integer: {
    fprintf(out, "%c%u", (type->data.integer.signedness == Signed) ? 'i' : 'u', type->data.integer.bitwidth);
  } break;
  case Type_boolean: {
    fputs("bool", out);
  } break;
  case Type_function: {
    fputs("function(", out);
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

internal void value_print(FILE *out, TypeInterner *types, ValueStore *values, ValueIndex idx) {
  Value *value = values_get(values, idx);
  Type  *type  = types_get(types, value->type);
  switch (Cast(enum TypeKind, type->kind)) {
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
  case Type_boolean: {
    fputs((*Cast(u8 *, value->data)) ? "true" : "false", out);
  } break;
  case Type_type: {
    type_index_print(out, types, *Cast(TypeIndex *, value->data));
  } break;
  case Type_function:
  case Type_nil:
  case Type_never:
  case Type_slice:
  case Type_array: {
    fprintf(out, "$%u", idx);
  } break;
  }
}

internal void ir_ref_print(FILE *out, IrRef ref, TypeInterner *types, ValueStore *values) {
  if (ir_ref_is_nil(ref)) {
    fprintf(out, "nil");
    return;
  }

  if (ref_is_value_index(ref)) {
    value_print(out, types, values, ref_to_value_index(ref));
  } else {
    fprintf(out, "%%%u", ref_to_instruction_index(ref));
  }
}

internal char const* typekind_string(u8 kind) {
  switch (Cast(enum TypeKind, kind)) {
  case Type_comptime_int: return "comptime_int";
  case Type_integer:      return "integer";
  case Type_boolean:      return "boolean";
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

void ir_chunk_print(FILE *out, IrChunk *chunk, TypeInterner *types, ValueStore *values) {
  BlockPrint buf[64];
  Stack(BlockPrint) blocks;
  stack_init(&blocks, buf, 64);

  for (InstructionIndex i = 0; i < chunk->opcode_count; i++) {
    u8  op   = chunk->opcodes[i];
    u32 data = chunk->data[i];

    u32 depth = blocks.len;

    if (!stack_is_empty(&blocks)) {
      BlockPrint *b = stack_peek_ptr_unsafe(&blocks);
      b->at += 1;
    }

    while (!stack_is_empty(&blocks)) {
      BlockPrint *b = stack_peek_ptr_unsafe(&blocks);
      if (b->at < b->count) {
        break;
      }

      u32 count = b->count;
      stack_pop_unsafe(&blocks);

      if (!stack_is_empty(&blocks)) {
        stack_peek_ptr_unsafe(&blocks)->at += count;
      }
    }

    fprintf(out, "%4u | %*s%s ", i, depth * 2, "", ir_opcode_names[op]);

    void *extra = ptr_offset(chunk->extra, data);

    switch (Cast(enum IrOpcode, op)) {
    case IR_func: {
      IrFunc *func = extra;
      fprintf(out, "first_param_or_body=%u instruction_count=%u", i + func->offset_first_param_or_body, func->instruction_count);
      stack_push(&blocks, ((BlockPrint){ .count = func->instruction_count - 1, .at = 0 }));
    } break;
    case IR_alloc: {
      type_index_print(out, types, data);
    } break;
    case IR_cond_br: {
      IrCondBr *cond_br = extra;
      fputs("cond=", out);
      ir_ref_print(out, cond_br->cond, types, values);
      fprintf(out, " then=%%%u otherwise=%%%u", cond_br->then, cond_br->otherwise);
    } break;
    case IR_block:
    case IR_loop: {
      fprintf(out, "count=%u", data);
      stack_push(&blocks, ((BlockPrint){ .count = data - 1, .at = 0 }));
    } break;
    case IR_br: {
      IrBr *br = extra;
      fprintf(out, "block=%%%u ", br->block);
      ir_ref_print(out, br->value, types, values);
    } break;
    case IR_repeat: {
      fprintf(out, "%%%u", data);
    } break;
    case IR_param:
    case IR_ret:
    case IR_load:
    case IR_typeof:
    case IR_typeinfo:
    case IR_function_return_type: {
      ir_ref_print(out, data, types, values);
    } break;
    case IR_store: {
      IrStore *store = extra;
      fputs("dst=", out);
      ir_ref_print(out, store->dst, types, values);
      fputs(" value=", out);
      ir_ref_print(out, store->value, types, values);
    } break;
    case IR_call: {
      IrCall *call = extra;
      fprintf(out, "func=(TODO) args=[");
      for (u32 j = 0; j < call->arg_count; j++) {
        if (j != 0) {
          fputs(", ", out);
        }
        ir_ref_print(out, call->args[j], types, values);
      }
      fputc(']', out);
    } break;
    case IR_declaration: {
      IrDeclaration *decl = extra;
      fputs("type=", out);
      ir_ref_print(out, decl->declared_type, types, values);
      fputs(" value=", out);
      ir_ref_print(out, decl->value, types, values);
    } break;
    case IR_lookup: {
      fprintf(out, "decl=%u", data);
    } break;
    case IR_cast_int:
    case IR_cast_int_safe: {
      IrCastInt *cast = extra;
      fputs("to=", out);
      type_index_print(out, types, cast->type);
      fputs(" value=", out);
      ir_ref_print(out, cast->value, types, values);
    } break;
    case IR_as: {
      IrAs *as = extra;
      ir_ref_print(out, as->type_to, types, values);
      fputs(" ", out);
      ir_ref_print(out, as->val, types, values);
    } break;
    case IR_unify: {
      IrUnify *unify = extra;
      ir_ref_print(out, unify->type_lhs, types, values);
      fputs(" ", out);
      ir_ref_print(out, unify->type_rhs, types, values);
    } break;
    case IR_type: {
      IrType *type = extra;
      fprintf(out, "%s args=[", typekind_string(type->kind));
      for (u32 j = 0; j < type->arg_count; j++) {
        if (j != 0) {
          fputs(", ", out);
        }
        ir_ref_print(out, type->args[j], types, values);
      }
      fputc(']', out);
    } break;
    }

    fputc('\n', out);
  }
}
