#include "iir.h"

#define SEGMENTLIST_NAME TYPE_LIST_NAME
#define SEGMENTLIST_TYPE TYPE_LIST_TYPE
#define SEGMENTLIST_MIN_SIZE_LOG2 TYPE_LIST_MIN_SIZE_LOG_2
#define SEGMENTLIST_SEGMENT_COUNT TYPE_LIST_SEGMENT_COUNT
#define SEGMENTLIST_FUNCTION_PREFIX typelist
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

// Interpreter IR Builder
// -------------------------------------------------------------------------------------------------

InstructionOffset iir_builder_add(IIrBuilder *builder, u8 op, AstAndSourceIndex source) {
  InstructionOffset idx = builder->kinds.len;
  opcodelist_append(&builder->kinds, builder->scratch, op);
  typelist_append(&builder->types, builder->scratch, (TypeKey){0});
  sourcelist_append(&builder->ast_source, builder->scratch, source);
  datalist_append(&builder->data, builder->scratch, (InstData){ .ptr = Null });
  return idx;
}

void *iir_builder_push_data_raw(IIrBuilder *builder, InstructionOffset idx, u32 size, u32 align) {
  void *p = arena_push(builder->scratch, size, align);
  *datalist_ptr_at_unchecked(&builder->data, idx) = (InstData){ .ptr = p };
  return p;
}

void iir_builder_set_data(IIrBuilder *builder, InstructionOffset idx, u32 data) {
  *datalist_ptr_at_unchecked(&builder->data, idx) = (InstData){ .data = data };
}

void iir_builder_set_type(IIrBuilder *builder, InstructionOffset idx, TypeKey type) {
  *typelist_ptr_at_unchecked(&builder->types, idx) = type;
}

u8 iir_builder_get_opcode(IIrBuilder *builder, InstructionOffset idx) {
  return opcodelist_at_unchecked(&builder->kinds, idx);
}

u32 iir_builder_get_data(IIrBuilder *builder, InstructionOffset idx) {
  return datalist_at_unchecked(&builder->data, idx).data;
}

TypeKey iir_builder_get_type(IIrBuilder *builder, InstructionOffset idx) {
  return typelist_at_unchecked(&builder->types, idx);
}

u32 iir_builder_offset(IIrBuilder *builder, InstructionOffset start) {
  u32 at = builder->kinds.len;
  return at - start;
}

internal b32 iir_opcode_references_extra(u8 op) {
  switch (Cast(IIrOpcode, op)) {
#define X(k,_1,e,_2) case k: return e;
#include "x_iir.h"
#undef X
  }
}

internal u32 iir_flex_array_size(u8 op, void *payload) {
  switch (Cast(IIrOpcode, op)) {
  case IIR_call: return Cast(IIrCall*, payload)->arg_count * sizeof(IRef);
  default:       return 0;
  }
}

internal u32 iir_extra_payload_size(u8 op, void *payload) {
  switch (Cast(IIrOpcode, op)) {
#define X(k,_1,_2,d) case k: return sizeof(d) + iir_flex_array_size(op, payload);
#include "x_iir.h"
#undef X
  }
}

internal u32 iir_extra_payload_align(u8 op) {
  switch (Cast(IIrOpcode, op)) {
#define X(k,_1,_2,d) case k: return Align_of(d);
#include "x_iir.h"
#undef X
  }
}

internal IIrOpSizeInfo iir_size_info_of_type(TypeInterner *types, TypeKey type) {
  if (key_is_nil(type)) {
    return (IIrOpSizeInfo){ .size = 0, .align = 1 };
  }

  TypeSizeInfo info = types_size_info_by_key(types, type);

  return (IIrOpSizeInfo){
    .size = info.size,
    .align = Max(info.align, 1),
  };
}

void iir_builder_flatten(IIrBuilder *builder, Arena *arena, TypeInterner *types, IIrChunk *chunk) {
  u32 count = Cast(u32, builder->kinds.len);

  u8  *opcodes = arena_push_array(u8,  arena, count);
  AstAndSourceIndex *sources = arena_push_array(AstAndSourceIndex, arena, count);
  u32 *data    = arena_push_array(u32, arena, count);
  TypeKey *type_keys = arena_push_array(TypeKey, arena, count);
  IIrOpSizeInfo *size_infos = arena_push_array(IIrOpSizeInfo, arena, count);

  opcodelist_copy_to_array(&builder->kinds, opcodes);
  sourcelist_copy_to_array(&builder->ast_source, sources);
  typelist_copy_to_array(&builder->types, type_keys);

  for (InstructionOffset i = 0; i < count; i++) {
    size_infos[i] = iir_size_info_of_type(types, type_keys[i]);
  }

  void *extra = arena->at;

  for (InstructionOffset i = 0; i < count; i++) {
    u8 op = opcodes[i];
    InstData entry = datalist_at_unchecked(&builder->data, i);

    if (!iir_opcode_references_extra(op)) {
      data[i] = entry.data;
      continue;
    }

    u32 size  = iir_extra_payload_size(op, entry.ptr);
    u32 align = iir_extra_payload_align(op);
    void *mem = ir_flatten_push_data(extra, arena, data, i, size, align);
    memcpy(mem, entry.ptr, size);
  }

  *chunk = (IIrChunk){
    .opcode_count = count,
    .opcodes      = opcodes,
    .sources      = sources,
    .data         = data,
    .types        = type_keys,
    .size_infos   = size_infos,
    .extra        = extra,
  };
}

internal CRef convert_ref(IRef ref, IIrConstantFn fn, void *context) {
  Assert(!iref_is_nil(ref));

  if (iref_is_instruction(ref)) {
    return cref_from_instruction(iref_to_instruction(ref));
  }

  return cref_from_constant(fn(context, iref_to_value(ref)));
}

internal void convert_data_ref(IIrChunk *chunk, InstructionOffset i, IIrConstantFn fn, void *context) {
  chunk->data[i] = convert_ref((IRef){chunk->data[i]}, fn, context).x;
}

void iir_chunk_convert_refs(IIrChunk *chunk, IIrConstantFn fn, void *context) {
  for (InstructionOffset i = 0; i < chunk->opcode_count; i++) {
    switch (Cast(IIrOpcode, chunk->opcodes[i])) {
    case IIR_func:
    case IIR_param:
    case IIR_alloc:
    case IIR_block:
    case IIR_loop:
    case IIR_repeat:
    case IIR_br_void:
    case IIR_ret_void:
      break;

    case IIR_load:
    case IIR_ret:
    case IIR_int_cast:
    case IIR_bit_not:
    case IIR_builtin_debug:
    case IIR_builtin_len:
      convert_data_ref(chunk, i, fn, context);
      break;

    case IIR_store: {
      void *p = iir_chunk_extra(chunk, i);
      IIrStore src = *Cast(IIrStore *, p);
      *Cast(IIrChunkStore *, p) = (IIrChunkStore){
        .ptr   = convert_ref(src.ptr, fn, context),
        .value = convert_ref(src.value, fn, context),
      };
    } break;

    case IIR_condbr: {
      void *p = iir_chunk_extra(chunk, i);
      IIrCondbr src = *Cast(IIrCondbr *, p);
      *Cast(IIrChunkCondbr *, p) = (IIrChunkCondbr){
        .cond      = convert_ref(src.cond, fn, context),
        .otherwise = src.otherwise,
      };
    } break;

    case IIR_br: {
      void *p = iir_chunk_extra(chunk, i);
      IIrBr src = *Cast(IIrBr *, p);
      *Cast(IIrChunkBr *, p) = (IIrChunkBr){
        .value = convert_ref(src.value, fn, context),
        .block = src.block,
      };
    } break;

    case IIR_call: {
      void *p = iir_chunk_extra(chunk, i);
      IIrCall *src = p;
      IIrChunkCall *dst = p;
      u32 arg_count = src->arg_count;
      IRef func_ptr = src->func_ptr;
      dst->func_ptr = convert_ref(func_ptr, fn, context);
      dst->arg_count = arg_count;
      for (u32 j = 0; j < arg_count; j++) {
        IRef arg = src->args[j];
        dst->args[j] = convert_ref(arg, fn, context);
      }
    } break;

    case IIR_int_add:
    case IIR_int_sub:
    case IIR_int_mul:
    case IIR_int_div:
    case IIR_int_mod:
    case IIR_bit_and:
    case IIR_bit_or:
    case IIR_bit_xor:
    case IIR_int_cmp_eq:
    case IIR_int_cmp_ne:
    case IIR_int_cmp_gt:
    case IIR_int_cmp_ge:
    case IIR_int_cmp_lt:
    case IIR_int_cmp_le:
    case IIR_index: {
      void *p = iir_chunk_extra(chunk, i);
      IIrBinary src = *Cast(IIrBinary *, p);
      *Cast(IIrChunkBinary *, p) = (IIrChunkBinary){
        .lhs = convert_ref(src.lhs, fn, context),
        .rhs = convert_ref(src.rhs, fn, context),
      };
    } break;
    }
  }
}
