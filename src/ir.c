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

void *ir_flatten_push_data(void *extra_base, Arena *arena, u32 *data, InstructionOffset i, u32 size, u32 align) {
  void *p = arena_push(arena, size, align);
  u32 offset = ptr_diff(p, extra_base);
  data[i] = offset;
  return p;
}
