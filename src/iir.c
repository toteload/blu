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

void iir_builder_flatten(IIrBuilder *builder, Arena *arena, IIrChunk *chunk) {
  u32 count = Cast(u32, builder->kinds.len);

  u8  *opcodes = arena_push_array(u8,  arena, count);
  AstAndSourceIndex *sources = arena_push_array(AstAndSourceIndex, arena, count);
  u32 *data    = arena_push_array(u32, arena, count);
  TypeKey *type_keys = arena_push_array(TypeKey, arena, count);

  opcodelist_copy_to_array(&builder->kinds, opcodes);
  sourcelist_copy_to_array(&builder->ast_source, sources);
  typelist_copy_to_array(&builder->types, type_keys);

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
    .extra        = extra,
  };
}
