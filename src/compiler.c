#include "compiler.h"
#include "source_file.h"
#include "string_interner.h"
#include "codegen.h"
#include "interpret.h"

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
#define INTERNER_EXTRA_TYPE      Declaration
#define INTERNER_FUNCTION_PREFIX decls
#define INTERNER_HASH_FN         hash_decl_key
#define INTERNER_COMPARE_FN      cmp_decl_key
#define INTERNER_OUTPUT_DEFINITIONS
#include "interner.h"

internal void *cstd_alloc_fn(void *ctx, void *p, usize old_byte_size, usize new_byte_size, u32 align) {
  Unused(ctx, old_byte_size, align);

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
    .arena              = &compiler->arena,
    .map_allocator      = cstd_allocator,
    .map_initial_size   = 32,
    .reserve_zero_index = True,
  });

  types_init(&compiler->types, &(InternerOptions){
    .arena              = &compiler->arena,
    .map_allocator      = cstd_allocator,
    .map_initial_size   = 32,
    .reserve_zero_index = True,
    .context            = &compiler->scratch,
  });

  values_init(&compiler->values, &(ValueStoreOptions){
    .arena = &compiler->arena,
    .payload_allocator = cstd_allocator,
  });

  compiler->common.type.comptime_int  = types_add(&compiler->types, &(Type){ .kind = Type_comptime_int });
  compiler->common.type.nil  = types_add(&compiler->types, &(Type){ .kind = Type_nil });
  compiler->common.type.type = types_add(&compiler->types, &(Type){ .kind = Type_type });
  {
    Value *v;
    compiler->common.val.nil  = values_alloc(&compiler->values, &v);
    TypeIndex *data = values_alloc_data(&compiler->values, sizeof(TypeIndex), Align_of(TypeIndex));
    *data = compiler->common.type.nil;
    *v = (Value){
      .type = compiler->common.type.type,
      .data_size = sizeof(TypeIndex),
      .data = data,
    };
  }
  {
    Value *v;
    compiler->common.val.type  = values_alloc(&compiler->values, &v);
    TypeIndex *data = values_alloc_data(&compiler->values, sizeof(TypeIndex), Align_of(TypeIndex));
    *data = compiler->common.type.type;
    *v = (Value){
      .type = compiler->common.type.type,
      .data_size = sizeof(TypeIndex),
      .data = data,
    };
  }

  decls_init(&compiler->decls, &(InternerOptions){
    .arena            = &compiler->arena,
    .map_allocator    = cstd_allocator,
    .map_initial_size = 16,
  });

  // DeclarationIndex 0 is reserved for the root
  decls_add(&compiler->decls, (DeclarationKey){ .parent = UINT32_MAX, .name = UINT32_MAX });
  decls_add(&compiler->decls, (DeclarationKey){ .parent = 0, .name = strings_add(&compiler->strings, string_lit("i32")) });
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

b32 lookup_identifier(DeclarationInterner *decl_keys, DeclarationIndex *mods, u32 mod_count, StringIndex name, DeclarationIndex *out) {
  for (u32 i = 0; i < mod_count; i++) {
    DeclarationKey key = {
      .parent = mods[i],
      .name   = name,
    };

    DeclarationIndex idx;
    b32 found = decls_find(decl_keys, key, &idx);
    if (found) {
      *out = idx;
      return True;
    }
  }

  return False;
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

    ok = source_tokenize(source, &compiler->scratch);
    if (!ok) { is_ok = False; source->status = SourceStatus_failed_to_parse; continue; }

    ok = source_parse(source, &compiler->scratch);
    if (!ok) { is_ok = False; source->status = SourceStatus_failed_to_parse; continue; }

    source_index_declarations(source, &compiler->strings);

    source->status = SourceStatus_parsed;
  }

  if (!is_ok) {
    return False;
  }

  // Add all source file declarations to the global declaration map
  for (u32 i = 0; i < compiler->sources.len; i++) {
    Source *source = sources_ptr_at_unchecked(&compiler->sources, i);

    // At the moment it is not possible to nest modules, so this stack will never grow beyond 2 (1. root, 2. mod).
    // BUT this will likely change in the future. At that point some checks need to be inserted to
    // ensure the stack doesn't overflow (and also make the stack a bit bigger :)).

    DeclFrame stackmem[4];
    Stack(DeclFrame) stack;
    stack_init(stack, stackmem, 4);
    stack_push(stack, ((DeclFrame){ .mod = 0, .n = source->decls[0].child_count }));

    source->decl_idxs[0] = 0;

    u32 offset = 1;

    while (!stack_is_empty(stack)) {
      DeclFrame *top = stack_peek_ptr(stack);

      if (top->n == 0) {
        stack_pop(stack);
        continue;
      }

      top->n -= 1;

      SourceDeclaration const *decl = &source->decls[offset];

      if (decl->kind == SourceDeclaration_mod) {
        b32 already_exists;
        DeclarationIndex idx = decls_add_checked(
          &compiler->decls,
          (DeclarationKey){ .parent = top->mod, .name = decl->name },
          &already_exists
        );

        if (already_exists) {
          Declaration val = decls_get_extra(&compiler->decls, idx);
          if (val.kind != Declaration_mod) {
            is_ok = False;
            Message_error(
              &compiler->msg_sink,
              source->idx,
              (MessageLocation){ .kind = MessageLocation_ast_index, .data.ast_index = decl->node },
              string_lit("Declaration already exists.")
            );
            goto next_iter;
          }
        }

        stack_push(stack, ((DeclFrame){ .mod = idx, .n = decl->child_count }));

        source->decl_idxs[offset] = idx;

        decls_set_extra(
          &compiler->decls,
          idx,
          (Declaration){ .kind = Declaration_mod, .data.loc = { .source = source->idx, .source_decl_idx = offset }}
        );
      } else if (decl->kind == SourceDeclaration_declaration) {
        b32 already_exists;
        DeclarationIndex idx = decls_add_checked(
          &compiler->decls,
          (DeclarationKey){ .parent = top->mod, .name = decl->name },
          &already_exists
        );

        if (already_exists) {
          is_ok = False;
          Message_error(
            &compiler->msg_sink,
            source->idx,
            (MessageLocation){ .kind = MessageLocation_ast_index, .data.ast_index = decl->node },
            string_lit("Declaration already exists.")
          );
          goto next_iter;
        }

        source->decl_idxs[offset] = idx;

        decls_set_extra(&compiler->decls, idx, (Declaration){
          .kind = Declaration_decl,
          .data.loc = { .source = source->idx, .source_decl_idx = offset },
        });
      }

next_iter:
      offset += 1;
    }
  }

  {
    CodeGenContext context = {
      .arena    = &compiler->arena,
      .scratch  = &compiler->scratch,
      .common   = &compiler->common,
      .msg_sink = &compiler->msg_sink,
      .strings  = &compiler->strings,
      .decls    = &compiler->decls,
      .values   = &compiler->values,
    };

    for (u32 i = 0; i < compiler->sources.len; i++) {
      Source *source = sources_ptr_at_unchecked(&compiler->sources, i);
      for (u32 j = 0; j < source->decl_count; j++) {
        is_ok &= source_generate_code(&context, source, j);
        ir_chunk_print(stdout, &source->ir_chunks[j], &compiler->types, &compiler->values);
      }
    }
  }

  // TODO: At this stage all the declarations in all the source files have generated code and
  // have output their dependencies on other declarations. Use these dependencies to determine
  // an evaluation order.
  //
  // For now that is not necessary (just one function).

  {
    for (u32 i = 0; i < compiler->sources.len; i++) {
      Source *source = sources_ptr_at_unchecked(&compiler->sources, i);

      InterpretContext context = {
        .perm = &source->arena,
        .scratch = &compiler->scratch,
        .common = &compiler->common,
        .msg_sink = &compiler->msg_sink,
        .decls = &compiler->decls,
        .values = &compiler->values,
      };

      for (u32 j = 0; j < source->decl_count; j++) {
        is_ok &= source_interpret_declaration(&context, source, j);
        ir_chunk_print(stdout, &source->runtime_chunks[j], &compiler->types, &compiler->values);
      }
    }
  }

  return is_ok;
}
