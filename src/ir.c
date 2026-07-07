#include "ir.h"
#include "value.h"
#include "types.h"

#define Bitmask_ir_ref_is_value_index (Cast(u32, 1) << 31)

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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#define HASHMAP_NAME            InstructionResultMap
#define HASHMAP_FUNCTION_PREFIX map
#define HASHMAP_KEY_TYPE        InstructionIndex
#define HASHMAP_VALUE_TYPE      ValueIndex
#define HASHMAP_HASH_FN         instruction_index_hash
#define HASHMAP_KEY_COMPARE_FN  instruction_index_eq
#define HASHMAP_LINKAGE         internal
#define HASHMAP_OUTPUT_DEFINITIONS
#include "hashmap.h"
#pragma clang diagnostic pop

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

internal always_inline InstructionIndex ref_to_instruction_index(IrRef ref) {
  return ref;
}

internal always_inline ValueIndex ref_to_value_index(IrRef ref) {
  return ref & ~Bitmask_ir_ref_is_value_index;
}

internal always_inline Value *ref_value(ValueStore *values, IrRef ref) {
  ValueIndex idx = ref_to_value_index(ref);
  return values_get(values, idx);
}

internal always_inline b32 ref_is_value_index(IrRef ref) {
  return (ref & Bitmask_ir_ref_is_value_index) != 0;
}

u8 opcode(IrChunk *chunk, InstructionIndex idx) {
  return chunk->opcodes[idx];
}

u32 instruction_data(IrChunk *chunk, InstructionIndex idx) {
  return chunk->data[idx];
}

void *instruction_extra(IrChunk *chunk, InstructionIndex idx) {
  return ptr_offset(chunk->extra, chunk->data[idx]);
}

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

u32 generate_ir(Source *source) {
  // foreach mod-section
  //   foreach declaration
  //     generate ir and save the offset somewhere

  return 0;
}
