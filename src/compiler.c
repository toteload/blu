#include "compiler.h"
#include "source_file.h"
#include "string_interner.h"

#include <stdarg.h>

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

internal void *cstd_alloc_fn(void *ctx, void *p, usize old_byte_size, usize new_byte_size, u32 align) {
  if (!is_null(p) && new_byte_size == 0) {
    free(p);
    return Null;
  }

  return realloc(p, new_byte_size);
}

Allocator const cstd_allocator = { .fn = cstd_alloc_fn, };

internal void compiler_add_message(void *user, u8 severity, SourceIndex idx, MessageLocation location, String format, ...) {
  Compiler *compiler = user;

  u32 arg_count = message_format_arg_count(format);

  Message *msg = arena_push(&compiler->arena, sizeof(Message) + arg_count * sizeof(MessageArg), Align_of(Message));

  msg->severity = severity;
  msg->source   = idx;
  msg->location = location;
  msg->format   = arena_copy_string(&compiler->arena, format);

  va_list vl;
  va_start(vl, format);

  for (u32 i = 0; i < arg_count; i++) {
    msg->args[i] = va_arg(vl, MessageArg);
  }

  va_end(vl);

  msglist_append(&compiler->msg_list, &compiler->arena, msg);
}

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

  compiler->msg_sink = (MessageSink){
    .user = compiler,
    .add_message = compiler_add_message,
  };

  strings_init(&compiler->strings, &(InternerOptions){
    .arena = &compiler->arena,
    .map_allocator = cstd_allocator,
    .map_initial_size = 32,
  });

  decl_keys_init(&compiler->decl_keys, &(InternerOptions){
    .arena            = &compiler->arena,
    .map_allocator    = cstd_allocator,
    .map_initial_size = 16,
  });

  b32 ignore;

  // DeclarationIndex 0 is reserved for the root
  add_declaration(
    compiler,
    (DeclarationKey){ .parent = UINT32_MAX, .name = UINT32_MAX },
    &ignore
  );

  add_declaration(
    compiler,
    (DeclarationKey){ .parent = 0, .name = strings_add(&compiler->strings, string_lit("i32")) },
    &ignore
  );
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

DeclarationIndex add_declaration(Compiler *compiler, DeclarationKey key, b32 *already_exists) {
  return decl_keys_add_checked(&compiler->decl_keys, key, already_exists);
}

void set_declaration_value(Compiler *compiler, DeclarationIndex idx, Declaration val) {
  // TODO
}

void compiler_print_all_messages(Compiler *compiler) {
  for (u32 i = 0; i < compiler->sources.len; i++) {
    Source *source = sources_ptr_at_unchecked(&compiler->sources, i);
    source_print_all_messages(source);
  }

  u32 count = compiler->msg_list.len;
  for (u32 i = 0; i < count; i++) {
    Message *msg = msglist_at_unchecked(&compiler->msg_list, i);
    Source *source = sources_ptr_at_unchecked(&compiler->sources, msg->source);
    print_message(msg, source);
  }
}

typedef struct {
  DeclarationIndex mod;
  u32 n;
} DeclFrame;

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

    source_index_declarations(source);

    source->status = SourceStatus_parsed;
  }

  if (!is_ok) {
    return False;
  }

  // Add all source file declarations to the global declaration map
  for (u32 i = 0; i < compiler->sources.len; i++) {
    Source *source = sources_ptr_at_unchecked(&compiler->sources, i);

    DeclFrame stackmem[64];
    Stack(DeclFrame) stack;
    stack_init(stack, stackmem, 64);
    stack_push(stack, ((DeclFrame){ .mod = 0, .n = source->decls[0].child_count }));

    u32 offset = 1;

    while (!stack_is_empty(stack)) {
      DeclFrame *top = stack_peek_ptr(stack);

      if (top->n == 0) {
        stack_pop(stack);
        continue;
      }

      top->n -= 1;

      SourceDeclaration const *decl = &source->decls[offset];

      offset += 1;

      if (decl->kind == SourceDeclaration_mod) {
        b32 ignore_already_exists;
        DeclarationIndex mod = add_declaration(compiler, (DeclarationKey){ .parent = top->mod, .name = strings_add(&compiler->strings, decl->name) }, &ignore_already_exists);
        stack_push(stack, ((DeclFrame){ .mod = mod, .n = decl->child_count }));
      } else if (decl->kind == SourceDeclaration_declaration) {
        b32 already_exists;
        DeclarationIndex idx = add_declaration(
          compiler,
          (DeclarationKey){ .parent = top->mod, .name = strings_add(&compiler->strings, decl->name) },
          &already_exists
        );

        if (already_exists) {
          Message_error(
            &compiler->msg_sink,
            source->idx,
            (MessageLocation){ .kind = MessageLocation_ast_index, .data.ast_index = decl->node },
            string_lit("Declaration already exists.")
          );

          is_ok = False;

          continue;
        }

        set_declaration_value(compiler, idx, (Declaration){
          .kind = Declaration_decl,
          .data.decl = { .source = source->idx, .ast = decl->node },
        });
      }
    }
  }

  return is_ok;
}
