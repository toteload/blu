#include "compiler.h"
#include "source_file.h"

#define SEGMENTLIST_NAME            SourceList
#define SEGMENTLIST_TYPE            Source
#define SEGMENTLIST_FUNCTION_PREFIX sources
#define SEGMENTLIST_MIN_SIZE_LOG2   SourceList_min_size_log2
#define SEGMENTLIST_SEGMENT_COUNT   SourceList_segment_count
#define SEGMENTLIST_LINKAGE         internal
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

#define XXH_INLINE_ALL
#include "xxhash.h"

u32 hash_decl_key(void *context, DeclarationKey key) {
  Unused(context);
  return XXH32(&key, sizeof(DeclarationKey), 0);
}

b32 cmp_decl_key(void *context, DeclarationKey a, DeclarationKey b) {
  Unused(context);
  return a.parent == b.parent && a.name == b.name;
}

#define INTERNER_NAME            DeclarationInterner
#define INTERNER_TYPE            DeclarationKey
#define INTERNER_INDEX_TYPE      DeclarationIndex
#define INTERNER_FUNCTION_PREFIX decl_keys
#define INTERNER_HASH_FN         hash_decl_key
#define INTERNER_COMPARE_FN      cmp_decl_key
#define INTERNER_OUTPUT_DEFINITIONS
#include "interner.h"

#define SEGMENTLIST_NAME            Declarations
#define SEGMENTLIST_TYPE            Declaration
#define SEGMENTLIST_MIN_SIZE_LOG2   Declarations_min_size_log2
#define SEGMENTLIST_SEGMENT_COUNT   Declarations_segment_count
#define SEGMENTLIST_FUNCTION_PREFIX decls
#define SEGMENTLIST_LINKAGE         internal
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

void compiler_init(Compiler *compiler) {
  zero_struct(Compiler, compiler);

  arena_init(&compiler->arena, &(ArenaOptions){
    .reserve_size        = MiB(16),
    .initial_commit_size = MiB(1),
  });

  arena_init(&compiler->scratch, &(ArenaOptions){
    .reserve_size        = MiB(16),
    .initial_commit_size = MiB(1),
  });

  decl_keys_init(&compiler->decl_keys, &(InternerOptions){
    .arena            = &compiler->arena,
    .map_allocator    = {0},
    .map_initial_size = 16,
  });

  add_declaration(
    compiler,
    (DeclarationKey){ .parent = 0, .name = 0 },
    (Declaration){ .kind = Declaration_root }
  );
  decls_push(&compiler->decls, &compiler->arena);
}

void compiler_deinit(Compiler *compiler) {
  arena_deinit(&compiler->arena);
  arena_deinit(&compiler->scratch);
}

void compiler_add_sourcefile(Compiler *compiler, String filename) {
  SourceIndex idx = compiler->sources.len;
  Source *source = sources_push(&compiler->sources, &compiler->arena);
  source_file_init(source, idx, filename);
}

DeclarationIndex add_declaration(Compiler *compiler, DeclarationKey key, Declaration decl) {
  DeclarationIndex idx = decl_keys_add(&compiler->decl_keys, key);
  decls_append(&compiler->decls, &compiler->arena, decl);
  return idx;
}

void compiler_print_all_messages(Compiler *compiler) {
  for (u32 i = 0; i < compiler->sources.len; i++) {
    Source *source = sources_ptr_at_unchecked(&compiler->sources, i);
    source_print_all_messages(source);
  }
}

b32 compile(Compiler *compiler) {
  b32 is_ok = True;
  for (u32 i = 0; i < compiler->sources.len; i++) {
    Source *source = sources_ptr_at_unchecked(&compiler->sources, i);

    b32 ok;
    ok = source_read_file(source);
    if (!ok) { is_ok = False; source->status = SourceStatus_failed_to_parse; continue; }

    ok = source_tokenize(source);
    if (!ok) { is_ok = False; source->status = SourceStatus_failed_to_parse; continue; }

    ok = source_parse(source);
    if (!ok) { is_ok = False; source->status = SourceStatus_failed_to_parse; continue; }

    //source_index_declarations(source);

    source->status = SourceStatus_parsed;
  }

  if (!is_ok) {
    return False;
  }

  return is_ok;
}
