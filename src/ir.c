#include "ir.h"

// -------------------------------------------------------------------------------------------------

#define SEGMENTLIST_NAME            OPCODE_LIST_NAME
#define SEGMENTLIST_TYPE            OPCODE_LIST_TYPE
#define SEGMENTLIST_MIN_SIZE_LOG2   OPCODE_LIST_MIN_SIZE_LOG_2
#define SEGMENTLIST_SEGMENT_COUNT   OPCODE_LIST_SEGMENT_COUNT
#define SEGMENTLIST_FUNCTION_PREFIX opcodelist
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

#define SEGMENTLIST_NAME            INST_DATALIST_NAME
#define SEGMENTLIST_TYPE            INST_DATALIST_TYPE
#define SEGMENTLIST_MIN_SIZE_LOG2   INST_DATALIST_MIN_SIZE_LOG2
#define SEGMENTLIST_SEGMENT_COUNT   INST_DATALIST_SEGMENT_COUNT
#define SEGMENTLIST_FUNCTION_PREFIX datalist
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

#define SEGMENTLIST_NAME AST_SOURCE_LIST_NAME
#define SEGMENTLIST_TYPE AST_SOURCE_LIST_TYPE
#define SEGMENTLIST_MIN_SIZE_LOG2 AST_SOURCE_LIST_MIN_SIZE_LOG2
#define SEGMENTLIST_SEGMENT_COUNT AST_SOURCE_LIST_SEGMENT_COUNT
#define SEGMENTLIST_FUNCTION_PREFIX sourcelist
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

#define SEGMENTLIST_NAME TYPE_LIST_NAME
#define SEGMENTLIST_TYPE TYPE_LIST_TYPE
#define SEGMENTLIST_MIN_SIZE_LOG2 TYPE_LIST_MIN_SIZE_LOG_2
#define SEGMENTLIST_SEGMENT_COUNT TYPE_LIST_SEGMENT_COUNT
#define SEGMENTLIST_FUNCTION_PREFIX typelist
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

// Specializer IR Builder
// -------------------------------------------------------------------------------------------------

InstructionIndex sir_builder_add(SIrBuilder *builder, u8 op, SourceIndex source_idx, AstIndex ast_idx) {
  InstructionIndex idx = builder->kinds.len;
  opcodelist_append(&builder->kinds, builder->scratch, op);
  sourcelist_append(&builder->ast_source, builder->scratch, (AstAndSourceIndex){ .source_idx = source_idx, .ast_idx = ast_idx });
  datalist_append(&builder->data, builder->scratch, (InstData){ .ptr = Null });
  return idx;
}

InstructionIndex sir_builder_add_as(SIrBuilder *builder, SRef type_destination, SRef val, SourceIndex source_idx, AstIndex ast_idx) {
  InstructionIndex idx = sir_builder_add(builder, SIR_as, source_idx, ast_idx);

  SIrAs *data = sir_builder_push_data(builder, idx, SIrAs);
  *data = (SIrAs){
    .type_to = type_destination,
    .val = val,
  };

  return idx;
}

void *sir_builder_push_data_raw(SIrBuilder *builder, InstructionIndex idx, u32 size, u32 align) {
  void *p = arena_push(builder->scratch, size, align);
  *datalist_ptr_at_unchecked(&builder->data, idx) = (InstData){ .ptr = p };
  return p;
}

void sir_builder_set_data(SIrBuilder *builder, InstructionIndex idx, u32 data) {
  *datalist_ptr_at_unchecked(&builder->data, idx) = (InstData){ .data = data };
}

InstructionIndex sir_builder_end_block_with(SIrBuilder *builder, InstructionIndex block, InstructionIndex target, SRef ref, SourceIndex source_idx, AstIndex ast_idx) {
  InstructionIndex br = sir_builder_add(builder, SIR_br, source_idx, ast_idx);

  SIrBr *data = sir_builder_push_data(builder, br, SIrBr);
  *data = (SIrBr){
    .block = target,
    .value = ref,
  };

  u32 block_inst_count = sir_builder_offset(builder, block);
  sir_builder_set_data(builder, block, block_inst_count);

  return br;
}

u32 sir_builder_offset(SIrBuilder *builder, InstructionIndex start) {
  u32 at = builder->kinds.len;
  return at - start;
}

internal b32 sir_opcode_references_extra(u8 op) {
  switch (Cast(SIrOpcode, op)) {
#define X(k,_1,e,_2) case k: return e;
#include "x_sir.h"
#undef X
  }
}

internal u32 sir_flex_array_size(u8 op, void *payload) {
  switch (Cast(SIrOpcode, op)) {
  case SIR_type: return Cast(SIrType*, payload)->arg_count * sizeof(SRef);
  case SIR_call: return Cast(SIrCall*, payload)->arg_count * sizeof(SRef);
  default:      return 0;
  }
}

internal u32 sir_extra_payload_size(u8 op, void *payload) {
  switch (Cast(SIrOpcode, op)) {
#define X(k,_1,_2,d) case k: return sizeof(d) + sir_flex_array_size(op, payload);
#include "x_sir.h"
#undef X
  }
}

internal u32 sir_extra_payload_align(u8 op) {
  switch (Cast(SIrOpcode, op)) {
#define X(k,_1,_2,d) case k: return Align_of(d);
#include "x_sir.h"
#undef X
  }
}

internal void *flatten_push_data(void *extra_base, Arena *arena, u32 *data, InstructionIndex i, u32 size, u32 align) {
  void *p = arena_push(arena, size, align);
  u32 offset = ptr_diff(p, extra_base);
  data[i] = offset;
  return p;
}

void sir_builder_flatten(SIrBuilder *builder, Arena *arena, SIrChunk *chunk) {
  u32 count = Cast(u32, builder->kinds.len);

  u8  *opcodes = arena_push_array(u8,  arena, count);
  AstAndSourceIndex *sources = arena_push_array(AstAndSourceIndex, arena, count);
  u32 *data    = arena_push_array(u32, arena, count);

  opcodelist_copy_to_array(&builder->kinds, opcodes);
  sourcelist_copy_to_array(&builder->ast_source, sources);

  void *extra = arena->at;

  for (InstructionIndex i = 0; i < count; i++) {
    u8 op = opcodes[i];
    InstData entry = datalist_at_unchecked(&builder->data, i);

    if (!sir_opcode_references_extra(op)) {
      data[i] = entry.data;
      continue;
    }

    u32 size  = sir_extra_payload_size(op, entry.ptr);
    u32 align = sir_extra_payload_align(op);
    void *mem = flatten_push_data(extra, arena, data, i, size, align);
    memcpy(mem, entry.ptr, size);
  }

  *chunk = (SIrChunk){
    .opcode_count = count,
    .opcodes      = opcodes,
    .sources   = sources,
    .data         = data,
    .extra        = extra,
  };
}

// Interpreter IR Builder
// -------------------------------------------------------------------------------------------------

InstructionIndex iir_builder_add(IIrBuilder *builder, u8 op) {
  InstructionIndex idx = builder->kinds.len;
  opcodelist_append(&builder->kinds, builder->scratch, op);
  typelist_append(&builder->types, builder->scratch, 0);
  sourcelist_append(&builder->ast_source, builder->scratch, (AstAndSourceIndex){ 0, 0 });
  datalist_append(&builder->data, builder->scratch, (InstData){ .ptr = Null });
  return idx;
}

void *iir_builder_push_data_raw(IIrBuilder *builder, InstructionIndex idx, u32 size, u32 align) {
  void *p = arena_push(builder->scratch, size, align);
  *datalist_ptr_at_unchecked(&builder->data, idx) = (InstData){ .ptr = p };
  return p;
}

void iir_builder_set_source(IIrBuilder *builder, InstructionIndex idx, SourceIndex source_idx, AstIndex ast_idx) {
  *sourcelist_ptr_at_unchecked(&builder->ast_source, idx) = (AstAndSourceIndex){ .source_idx = source_idx, .ast_idx = ast_idx };
}

void iir_builder_set_data(IIrBuilder *builder, InstructionIndex idx, u32 data) {
  *datalist_ptr_at_unchecked(&builder->data, idx) = (InstData){ .data = data };
}

void iir_builder_set_type(IIrBuilder *builder, InstructionIndex idx, TypeIndex type) {
  *typelist_ptr_at_unchecked(&builder->types, idx) = type;
}

u8 iir_builder_get_opcode(IIrBuilder *builder, InstructionIndex idx) {
  return opcodelist_at_unchecked(&builder->kinds, idx);
}

u32 iir_builder_get_data(IIrBuilder *builder, InstructionIndex idx) {
  return datalist_at_unchecked(&builder->data, idx).data;
}

TypeIndex iir_builder_get_type(IIrBuilder *builder, InstructionIndex idx) {
  return typelist_at_unchecked(&builder->types, idx);
}

u32 iir_builder_offset(IIrBuilder *builder, InstructionIndex start) {
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
  case IIR_call: return Cast(IIrCall*, payload)->arg_count * sizeof(SRef);
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
  TypeIndex *types = arena_push_array(TypeIndex, arena, count);

  opcodelist_copy_to_array(&builder->kinds, opcodes);
  sourcelist_copy_to_array(&builder->ast_source, sources);
  typelist_copy_to_array(&builder->types, types);

  void *extra = arena->at;

  for (InstructionIndex i = 0; i < count; i++) {
    u8 op = opcodes[i];
    InstData entry = datalist_at_unchecked(&builder->data, i);

    if (!iir_opcode_references_extra(op)) {
      data[i] = entry.data;
      continue;
    }

    u32 size  = iir_extra_payload_size(op, entry.ptr);
    u32 align = iir_extra_payload_align(op);
    void *mem = flatten_push_data(extra, arena, data, i, size, align);
    memcpy(mem, entry.ptr, size);
  }

  *chunk = (IIrChunk){
    .opcode_count = count,
    .opcodes      = opcodes,
    .sources   = sources,
    .data         = data,
    .types = types,
    .extra        = extra,
  };
}
