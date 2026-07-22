#include "ir.h"

#define SEGMENTLIST_NAME            OPCODELIST_NAME
#define SEGMENTLIST_TYPE            OPCODELIST_TYPE
#define SEGMENTLIST_MIN_SIZE_LOG2   OPCODELIST_MIN_SIZE_LOG_2
#define SEGMENTLIST_SEGMENT_COUNT   OPCODELIST_SEGMENT_COUNT
#define SEGMENTLIST_FUNCTION_PREFIX opcodelist
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

#define SEGMENTLIST_NAME            INSTDATALIST_NAME
#define SEGMENTLIST_TYPE            INSTDATALIST_TYPE
#define SEGMENTLIST_MIN_SIZE_LOG2   INSTDATALIST_MIN_SIZE_LOG2
#define SEGMENTLIST_SEGMENT_COUNT   INSTDATALIST_SEGMENT_COUNT
#define SEGMENTLIST_FUNCTION_PREFIX datalist
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

InstructionIndex inst_alloc(IrBuilder *builder) {
  InstructionIndex idx = builder->kinds.len;
  opcodelist_append(&builder->kinds, builder->scratch, 0);
  datalist_append(&builder->data, builder->scratch, (InstData){ .ptr = Null });
  return idx;
}

void inst_set_opcode(IrBuilder *builder, InstructionIndex idx, u8 opcode) {
  *opcodelist_ptr_at_unchecked(&builder->kinds, idx) = opcode;
}

void inst_set_data(IrBuilder *builder, InstructionIndex idx, u32 data) {
  *datalist_ptr_at_unchecked(&builder->data, idx) = (InstData){ .data = data };
}

void *inst_push_data_raw(IrBuilder *builder, InstructionIndex idx, u32 size, u32 align) {
  void *p = arena_push(builder->scratch, size, align);
  *datalist_ptr_at_unchecked(&builder->data, idx) = (InstData){ .ptr = p };
  return p;
}

u32 inst_offset(IrBuilder *builder, InstructionIndex start) {
  u32 at = builder->kinds.len;
  return at - start - 1;
}

InstructionIndex inst_block_begin(IrBuilder *builder) {
  InstructionIndex block = inst_alloc(builder);
  inst_set_opcode(builder, block, IR_block);
  return block;
}

void inst_block_end(IrBuilder *builder, InstructionIndex block, IrRef val) {
  InstructionIndex br = inst_alloc(builder);
  inst_set_opcode(builder, br, IR_br);
  IrBr *data_br = inst_push_data(builder, br, IrBr);
  *data_br = (IrBr){
    .block = block,
    .value = val,
  };

  u32 block_inst_count = inst_offset(builder, block);
  inst_set_data(builder, block, block_inst_count);
}

InstructionIndex inst_as(IrBuilder *builder, IrRef type_destination, IrRef val) {
  InstructionIndex idx = inst_alloc(builder);
  inst_set_opcode(builder, idx, IR_as);

  IrAs *data = inst_push_data(builder, idx, IrAs);
  *data = (IrAs){
    .type_to   = type_destination,
    .val = val,
  };

  return idx;
}

internal b32 opcode_references_extra(u8 op) {
  switch (Cast(enum IrOpcode, op)) {
#define X(k,e,_1,_2) case k: return e;
#include "x_ir.h"
#undef X
  }
}

internal u32 flex_array_size(u8 op, void *payload) {
  switch (Cast(enum IrOpcode, op)) {
  case IR_type: return Cast(IrType*, payload)->arg_count * sizeof(IrRef);
  case IR_call: return Cast(IrCall*, payload)->arg_count * sizeof(IrRef);
  default:      return 0;
  }
}

internal u32 extra_payload_size(u8 op, void *payload) {
  switch (Cast(enum IrOpcode, op)) {
#define X(k,_1,d,_2) case k: return sizeof(d) + flex_array_size(op, payload);
#include "x_ir.h"
#undef X
  }
}

internal u32 extra_payload_align(u8 op) {
  switch (Cast(enum IrOpcode, op)) {
#define X(k,_1,d,_2) case k: return Align_of(d);
#include "x_ir.h"
#undef X
  }
}

internal void *flatten_push_data(void *extra_base, Arena *arena, u32 *data, InstructionIndex i, u32 size, u32 align) {
  void *p = arena_push(arena, size, align);
  u32 offset = ptr_diff(p, extra_base);
  data[i] = offset;
  return p;
}

void irbuilder_flatten(IrBuilder *builder, Arena *arena, IrChunk *chunk) {
  u32 count = Cast(u32, builder->kinds.len);

  u8  *opcodes = arena_push_array(u8,  arena, count);
  u32 *data    = arena_push_array(u32, arena, count);

  opcodelist_copy_to_array(&builder->kinds, opcodes);

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
    .data         = data,
    .extra        = extra,
  };
}

#if 0
extern u32 eval_cast_int(TypeInteger, void*, TypeInteger, void*);

#define SEGMENTLIST_NAME            ChunkList
#define SEGMENTLIST_TYPE            IrChunk
#define SEGMENTLIST_MIN_SIZE_LOG2   ChunkList_min_size_log2
#define SEGMENTLIST_SEGMENT_COUNT   ChunkList_segment_count
#define SEGMENTLIST_FUNCTION_PREFIX chunk_list
#define SEGMENTLIST_LINKAGE         internal
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

IrChunk *get_chunk(IrChunkAllocator *chunks, ChunkIndex idx) {
  return chunk_list_ptr_at_unchecked(&chunks->list, idx);
}

u32 instruction_index_hash(void *context, InstructionIndex idx) {
  Unused(context);

  return idx;
}

b32 instruction_index_eq(void *context, InstructionIndex a, InstructionIndex b) {
  Unused(context);

  return a == b;
}

#define HASHMAP_NAME            InstructionResultMap
#define HASHMAP_FUNCTION_PREFIX map
#define HASHMAP_KEY_TYPE        InstructionIndex
#define HASHMAP_VALUE_TYPE      ValueIndex
#define HASHMAP_HASH_FN         instruction_index_hash
#define HASHMAP_KEY_COMPARE_FN  instruction_index_eq
#define HASHMAP_LINKAGE         internal
#define HASHMAP_OUTPUT_DEFINITIONS
#include "hashmap.h"

#define SEGMENTLIST_NAME            CallStack
#define SEGMENTLIST_TYPE            CallFrame
#define SEGMENTLIST_MIN_SIZE_LOG2   CallStack_min_size_log2
#define SEGMENTLIST_SEGMENT_COUNT   CallStack_segment_count
#define SEGMENTLIST_FUNCTION_PREFIX callstack
#define SEGMENTLIST_LINKAGE         internal
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

#define SEGMENTLIST_NAME            ValueStack
#define SEGMENTLIST_TYPE            ValueStackElement
#define SEGMENTLIST_MIN_SIZE_LOG2   ValueStack_min_size_log2
#define SEGMENTLIST_SEGMENT_COUNT   ValueStack_segment_count
#define SEGMENTLIST_FUNCTION_PREFIX value_stack
#define SEGMENTLIST_LINKAGE         internal
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

internal CallFrame *push_and_init_callframe(IrMachine *machine, IrLocation address) {
  CallFrame *frame = callstack_push(&machine->callstack, machine->arena_callstack);

  *frame = (CallFrame){
    .pc = address,
  };

  map_init(&frame->inst_map, &(HashMapOptions){ .allocator = machine->allocator_inst_map, .initial_size = 8, .context = Null });

  return frame;
}

internal always_inline Value *ref_value(ValueStore *values, IrRef ref) {
  ValueIndex idx = ref_to_value_index(ref);
  return values_get(values, idx);
}

#if 0
u8 opcode(IrChunk *chunk, InstructionIndex idx) {
  return chunk->opcodes[idx];
}

u32 instruction_data(IrChunk *chunk, InstructionIndex idx) {
  return chunk->data[idx];
}

void *instruction_extra(IrChunk *chunk, InstructionIndex idx) {
  return ptr_offset(chunk->extra, chunk->data[idx]);
}
#endif

internal CallFrame *top_frame(IrMachine *machine) {
  return callstack_ptr_at_unchecked(&machine->callstack, machine->callstack.len-1);
}

ValueIndex ref_value_index(IrMachine *machine, IrRef ref) {
  if (ref_is_value_index(ref)) {
    return ref_to_value_index(ref);
  }

  CallFrame *frame = top_frame(machine);

  return *map_find(&frame->inst_map, ref_to_instruction_index(ref));
}

internal void clear_block_values(IrMachine *machine, CallFrame *frame, InstructionIndex block) {
  ValueStack *stack = &machine->value_stack;

  for (u32 i = stack->len; i-- > 0; ) {
    ValueStackElement e = *value_stack_ptr_at_unchecked(stack, i);
    if (e.kind == ValueStackElement_marker_block && e.idx == block) {
      stack->len = i-1;
break;
    }

    Assert(e.kind == ValueStackElement_value);

    ValueIndex val = *map_find(&frame->inst_map, e.idx);

    values_dealloc(machine->values, val);
  }
}

internal void clear_frame_values(IrMachine *machine, CallFrame *frame) {
  ValueStack *stack = &machine->value_stack;

  for (u32 i = stack->len; i-- > 0; ) {
    ValueStackElement e = *value_stack_ptr_at_unchecked(stack, i);

    if (e.kind == ValueStackElement_marker_frame) {
      stack->len = i-1;
      break;
    }

    if (e.kind == ValueStackElement_marker_block) {
      continue;
    }

    Assert(e.kind == ValueStackElement_value);

    ValueIndex val = *map_find(&frame->inst_map, e.idx);

    values_dealloc(machine->values, val);
  }
}

u32 ir_run(IrMachine *machine) {
  ValueStore *values = machine->values;
  TypeInterner *types = machine->types;

  while (True) {
    // OPTIMIZE: fetch the top frame when the frame actually changes, so for call and ret instructions.
    CallFrame *frame = top_frame(machine);
    InstructionIndex pc = frame->pc.instruction_index;
    IrChunk *chunk = get_chunk(machine->chunks, frame->pc.chunk_index);

    u8 op = opcode(chunk, pc);

    switch (op) {
    case IR_alloc: {
      TypeIndex type_idx = instruction_data(chunk, pc);
      Type *type = types_get(types, type_idx);
      TypeSizeInfo size_info = types_size_info(types, type);
      void *mem = values_alloc_data(values, size_info.size, size_info.align);
      Value *val;
      ValueIndex val_idx = values_alloc(values, &val);
      *val = (Value){
        .type = type_idx,
        .data = mem,
      };

      map_insert(&frame->inst_map, pc, val_idx);
      value_stack_append(
        &machine->value_stack,
        machine->arena_value_stack,
        (ValueStackElement){ .kind = ValueStackElement_value, .idx = pc });
    } break;
    case IR_cond_br: {
      IrCondBr *cond_br = instruction_extra(chunk, pc);
      Value *cond = ref_value(values, cond_br->cond);

      // A cond_br can only jump forward into a block, and not backwards to break out of an enclosing block.
      // This way you don't have to branch on whether this is a forwards or backwards jump.
      InstructionIndex target = (*Cast(u8*, cond->data)) ? cond_br->then : cond_br->otherwise;
      pc = target;
    } break;
    case IR_block: {
      value_stack_append(
        &machine->value_stack, 
        machine->arena_value_stack, 
        (ValueStackElement){ .kind = ValueStackElement_marker_block, .idx = pc });
    } break;
    case IR_loop: {
      value_stack_append(
        &machine->value_stack, 
        machine->arena_value_stack,
        (ValueStackElement){ .kind = ValueStackElement_marker_block, .idx = pc });
    } break;
    case IR_br: {
      InstructionIndex block_index = instruction_data(chunk, pc);
      clear_block_values(machine, frame, block_index);
      u32 block_size = instruction_data(chunk, block_index);
      pc = block_index + block_size;
    } break;
    case IR_repeat: {
      InstructionIndex block_index = instruction_data(chunk, pc);
      clear_block_values(machine, frame, block_index);
      pc = block_index;
    } break;
    case IR_ret: {
      IrRef ref = instruction_data(chunk, pc);
      machine->return_value = ref_to_value_index(ref);
      machine->callstack.len -= 1;
      clear_frame_values(machine, frame);
      continue;
    } break;
    case IR_load: {
      Panic();
    } break;
    case IR_store: {
      IrStore *store = instruction_extra(chunk, pc);
      Value *from = ref_value(machine->values, store->value);
      Value *target = ref_value(machine->values, store->dst);
      memcpy(target->data, from->data, target->data_size);
    } break;
    case IR_call: {
      IrCall *call = instruction_extra(chunk, pc);

      CallFrame *frame = push_and_init_callframe(machine, call->func);

      InstructionIndex first_param = call->func.instruction_index + 1;
      for (u32 i = 0; i < call->arg_count; i++) {
        ValueIndex arg = ref_to_value_index(call->args[i]);
        map_insert(
          &frame->inst_map, 
          first_param + i,
          arg
        );
      }
    } break;
    case IR_cast_int: {
      IrCastInt *cast_int = instruction_extra(chunk, pc);
      Type *type_dst = types_get(types, cast_int->type);
      Value *val = ref_value(values, cast_int->value);
      Type *type_src = types_get(types, val->type);
      TypeSizeInfo size_info = types_size_info(types, type_dst);
      void *payload_dst = values_alloc_data(values, size_info.size, size_info.align);
      u32 err = eval_cast_int(type_src->data.integer, val->data, type_dst->data.integer, payload_dst);
      if (err) {
        Panic();
      }
      Value *val_res;
      ValueIndex res = values_alloc(values, &val_res);
      *val_res = (Value){
        .type = cast_int->type,
        .data = payload_dst,
      };

      map_insert(&frame->inst_map, pc, res);
    } break;
    }

    frame->pc.instruction_index = pc + 1;
  }
 
  return IrResult_ok;
}

internal u32 generate_ir_function(Source *source, IrChunk *chunk, TypeFunction type, AstIndex function) {
  u32 comptime_arg_count; // TODO init
  u32 runtime_arg_count; // TODO init

  InstructionIndex i = chunk_alloc_inst(chunk);

  chunk_set_opcode(chunk, i, IR_comptime_func);
  IrComptimeFunc *func = chunk_alloc_extra_typed(IrComptimeFunc, chunk, i);

  // TODO: output all the comptime and runtime args.

  // while generating code: save all the declaration dependencies.
  // it may be the case that the code in this function depends on the value or type of other declarations.

  TypeIndex expected_return_type; // TODO init
  
  struct InstSource {
    AstIndex ast_index;
    TypeIndex type_dst;
  };

  Stack(InstSource) stack;

  AstFunction *ast_func = nodes_data(&source->ast, function);
  stack_push(&stack, ast_func->body);
  
  while (!is_empty(stack)) {
    AstIndex idx = stack_pop();
    u8 kind = ast_kind(idx);
    switch (kind) {
    case Ast_literal_int: {
      InstructionIndex i = chunk_alloc_inst(chunk);
      chunk_set_opcode(chunk, i, IR_const);
      // TODO: create value in value store with this int
      // TODO: add cast and check instruction if the type does not match the expected type
    } break;
    }
  }

  // TODO: return the value of the last instruction generated
}
#endif
