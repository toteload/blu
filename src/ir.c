#include "ir.h"

// -------------------------------------------------------------------------------------------------

u8 chunk_opcode(IrChunk *chunk, InstructionIndex idx) {
  return chunk->opcodes[idx];
}

u32 chunk_data(IrChunk *chunk, InstructionIndex idx) {
  return chunk->data[idx];
}

void *chunk_extra(IrChunk *chunk, InstructionIndex idx) {
  return ptr_offset(chunk->extra, chunk->data[idx]);
}

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

InstructionIndex irbuilder_add(IrBuilder *builder, u8 op) {
  InstructionIndex idx = builder->kinds.len;
  opcodelist_append(&builder->kinds, builder->scratch, op);
  sourcelist_append(&builder->ast_source, builder->scratch, (AstAndSourceIndex){ 0, 0 });
  datalist_append(&builder->data, builder->scratch, (InstData){ .ptr = Null });
  return idx;
}

void inst_set_opcode(IrBuilder *builder, InstructionIndex idx, u8 opcode) {
  *opcodelist_ptr_at_unchecked(&builder->kinds, idx) = opcode;
}

u8 inst_get_opcode(IrBuilder *builder, InstructionIndex idx) {
  return opcodelist_at_unchecked(&builder->kinds, idx);
}

u32 inst_get_data(IrBuilder *builder, InstructionIndex idx) {
  return datalist_at_unchecked(&builder->data, idx).data;
}

void inst_set_source(IrBuilder *builder, InstructionIndex idx, SourceIndex source_idx, AstIndex ast_idx) {
  *sourcelist_ptr_at_unchecked(&builder->ast_source, idx) = (AstAndSourceIndex){ .source_idx = source_idx, .ast_idx = ast_idx };
}

void inst_set_data(IrBuilder *builder, InstructionIndex idx, u32 data) {
  *datalist_ptr_at_unchecked(&builder->data, idx) = (InstData){ .data = data };
}

void *inst_get_extra(IrBuilder *builder, InstructionIndex idx) {
  return datalist_at_unchecked(&builder->data, idx).ptr;
}

void *inst_push_data_raw(IrBuilder *builder, InstructionIndex idx, u32 size, u32 align) {
  void *p = arena_push(builder->scratch, size, align);
  *datalist_ptr_at_unchecked(&builder->data, idx) = (InstData){ .ptr = p };
  return p;
}

u32 inst_offset(IrBuilder *builder, InstructionIndex start) {
  u32 at = builder->kinds.len;
  return at - start;
}

#if 0
InstructionIndex inst_loop_begin(IrBuilder *builder) {
  InstructionIndex block = inst_alloc(builder);
  inst_set_opcode(builder, block, IR_loop);
  return block;
}

InstructionIndex inst_block_begin(IrBuilder *builder) {
  InstructionIndex block = inst_alloc(builder);
  inst_set_opcode(builder, block, IR_block);
  return block;
}

InstructionIndex inst_eval_block_begin(IrBuilder *builder) {
  InstructionIndex block = inst_alloc(builder);
  inst_set_opcode(builder, block, IR_eval_block);
  return block;
}

void inst_block_end(IrBuilder *builder, InstructionIndex block) {
  u32 block_inst_count = inst_offset(builder, block);
  inst_set_data(builder, block, block_inst_count);
}
#endif

void irbuilder_end_sir_block_with(IrBuilder *builder, InstructionIndex block, InstructionIndex target, SRef ref) {
  InstructionIndex br = irbuilder_add(builder, SIR_br);

  SBr *data = inst_push_data(builder, br, SBr);
  *data = (SBr){
    .block = target,
    .value = ref,
  };

  u32 block_inst_count = inst_offset(builder, block);
  inst_set_data(builder, block, block_inst_count);
}

//void inst_block_end_with_value(IrBuilder *builder, InstructionIndex block, IrRef ref) {
//  InstructionIndex br = inst_alloc(builder);
//  inst_set_opcode(builder, br, IR_br);
//
//  IrBr *data = inst_push_data(builder, br, IrBr);
//  *data = (IrBr){
//    .block = block,
//    .value = ref,
//  };
//
//  u32 block_inst_count = inst_offset(builder, block);
//  inst_set_data(builder, block, block_inst_count);
//}
//
//void inst_block_end_with_target(IrBuilder *builder, InstructionIndex block, InstructionIndex target) {
//  InstructionIndex br = inst_alloc(builder);
//  inst_set_opcode(builder, br, IR_br);
//
//  IrBr *data = inst_push_data(builder, br, IrBr);
//  *data = (IrBr){
//    .block = target,
//    .value = { 0 },
//  };
//
//  u32 block_inst_count = inst_offset(builder, block);
//  inst_set_data(builder, block, block_inst_count);
//}
//
//void inst_block_end_with_value_and_target(IrBuilder *builder, InstructionIndex block, InstructionIndex target, IrRef ref) {
//  InstructionIndex br = inst_alloc(builder);
//  inst_set_opcode(builder, br, IR_br);
//
//  IrBr *data = inst_push_data(builder, br, IrBr);
//  *data = (IrBr){
//    .block = target,
//    .value = ref,
//  };
//
//  u32 block_inst_count = inst_offset(builder, block);
//  inst_set_data(builder, block, block_inst_count);
//}
//
//void inst_block_end_repeat(IrBuilder *builder, InstructionIndex block, InstructionIndex target) {
//  InstructionIndex repeat = inst_alloc(builder);
//  inst_set_opcode(builder, repeat, IR_repeat);
//  inst_set_data(builder, repeat, target);
//
//  u32 block_inst_count = inst_offset(builder, block);
//  inst_set_data(builder, block, block_inst_count);
//}
//
//InstructionIndex inst_as(IrBuilder *builder, IrRef type_destination, IrRef val, SourceIndex source_idx, AstIndex ast_idx) {
//  InstructionIndex idx = inst_alloc(builder);
//  inst_set_opcode(builder, idx, IR_as);
//  inst_set_source(builder, idx, source_idx, ast_idx);
//
//  IrAs *data = inst_push_data(builder, idx, IrAs);
//  *data = (IrAs){
//    .type_to = type_destination,
//    .val = val,
//  };
//
//  return idx;
//}

//internal b32 opcode_references_extra(u8 op) {
//  switch (Cast(IrOpcode, op)) {
//#define X(k,e,_1,_2) case k: return e;
//#include "x_ir.h"
//#undef X
//  }
//}
//
//internal u32 flex_array_size(u8 op, void *payload) {
//  switch (Cast(IrOpcode, op)) {
//  case IR_type: return Cast(IrType*, payload)->arg_count * sizeof(IrRef);
//  case IR_call: return Cast(IrCall*, payload)->arg_count * sizeof(IrRef);
//  default:      return 0;
//  }
//}
//
//internal u32 extra_payload_size(u8 op, void *payload) {
//  switch (Cast(IrOpcode, op)) {
//#define X(k,_1,d,_2) case k: return sizeof(d) + flex_array_size(op, payload);
//#include "x_ir.h"
//#undef X
//  }
//}
//
//internal u32 extra_payload_align(u8 op) {
//  switch (Cast(IrOpcode, op)) {
//#define X(k,_1,d,_2) case k: return Align_of(d);
//#include "x_ir.h"
//#undef X
//  }
//}

internal void *flatten_push_data(void *extra_base, Arena *arena, u32 *data, InstructionIndex i, u32 size, u32 align) {
  void *p = arena_push(arena, size, align);
  u32 offset = ptr_diff(p, extra_base);
  data[i] = offset;
  return p;
}

void irbuilder_flatten(IrBuilder *builder, Arena *arena, IrChunk *chunk) {
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

    if (!opcode_references_extra(op)) {
      data[i] = entry.data;
      continue;
    }

    u32 size  = extra_payload_size(op, entry.ptr);
    u32 align = extra_payload_align(op);
    void *mem = flatten_push_data(extra, arena, data, i, size, align);
    memcpy(mem, entry.ptr, size);
  }

  *chunk = (IrChunk){
    .opcode_count = count,
    .opcodes      = opcodes,
    .sources   = sources,
    .data         = data,
    .extra        = extra,
  };
}
