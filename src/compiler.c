#include "compiler.h"
#include "source_file.h"
#include "string_interner.h"
#include "codegen.h"
#include "interpret.h"

#include <stdarg.h>

#define SEGMENTLIST_NAME            SourceList
#define SEGMENTLIST_TYPE            Source
#define SEGMENTLIST_FUNCTION_PREFIX sources
#define SEGMENTLIST_MIN_SIZE_LOG2   SOURCELIST_MIN_SIZE_LOG2
#define SEGMENTLIST_SEGMENT_COUNT   SOURCELIST_SEGMENT_COUNT
#define SEGMENTLIST_LINKAGE         internal
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

#define SEGMENTLIST_NAME            DeclIdxList
#define SEGMENTLIST_TYPE            DeclarationIndex
#define SEGMENTLIST_FUNCTION_PREFIX user_decls
#define SEGMENTLIST_MIN_SIZE_LOG2   DECLIDXLIST_MIN_SIZE_LOG2
#define SEGMENTLIST_SEGMENT_COUNT   DECLIDXLIST_SEGMENT_COUNT
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

internal ValueIndex add_type_value(Compiler *compiler, TypeIndex t) {
  Value *v;
  ValueIndex res = values_alloc(&compiler->values, &v);
  TypeIndex *data = values_alloc_data(&compiler->values, sizeof(TypeIndex), Align_of(TypeIndex));
  *data = t;
  *v = (Value){
    .type = compiler->common.type.type,
    .data_size = sizeof(TypeIndex),
    .data = data,
  };
  return res;
}

internal void add_primitive(Compiler *compiler, String name, ValueIndex val) {
  DeclarationIndex idx = decls_add(
    &compiler->decls,
    (DeclarationKey){ .parent = 0, .name = strings_add(&compiler->strings, name) }
  );

  decls_set_extra(
    &compiler->decls,
    idx,
    (Declaration){ .kind = Declaration_primitive, .data.primitive = val }
  );
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
  });

  types_init(&compiler->types, &(InternerOptions){
    .arena              = &compiler->arena,
    .map_allocator      = cstd_allocator,
    .map_initial_size   = 32,
    .context            = &compiler->scratch,
  });

  values_init(&compiler->values, &(ValueStoreOptions){
    .arena = &compiler->arena,
    .payload_allocator = cstd_allocator,
  });

  decls_init(&compiler->decls, &(InternerOptions){
    .arena            = &compiler->arena,
    .map_allocator    = cstd_allocator,
    .map_initial_size = 32,
  });

  compiler->common.type.comptime_int = types_add(&compiler->types, &(Type){ .kind = Type_comptime_int });
  compiler->common.type.type         = types_add(&compiler->types, &(Type){ .kind = Type_type });
  compiler->common.type.nil          = types_add(&compiler->types, &(Type){ .kind = Type_nil });
  compiler->common.type.bool         = types_add(&compiler->types, &(Type){ .kind = Type_bool });
  compiler->common.type.never        = types_add(&compiler->types, &(Type){ .kind = Type_never });
  compiler->common.type.i32          = types_add(&compiler->types, &(Type){ .kind = Type_integer, .data.integer = { .signedness = Signed, .bitwidth = 32 } });

  compiler->common.val.type  = add_type_value(compiler, compiler->common.type.type);
  compiler->common.val.nil   = add_type_value(compiler, compiler->common.type.nil);
  compiler->common.val.bool  = add_type_value(compiler, compiler->common.type.bool);
  compiler->common.val.never = add_type_value(compiler, compiler->common.type.never);
  compiler->common.val.i32   = add_type_value(compiler, compiler->common.type.i32);

  // DeclarationIndex 0 is reserved for the root
  decls_add(&compiler->decls, (DeclarationKey){ .parent = 0, .name = 0 });

  add_primitive(compiler, string_lit("type"),  compiler->common.val.type);
  add_primitive(compiler, string_lit("nil"),   compiler->common.val.nil);
  add_primitive(compiler, string_lit("bool"),  compiler->common.val.bool);
  add_primitive(compiler, string_lit("never"), compiler->common.val.never);
  add_primitive(compiler, string_lit("i32"),   compiler->common.val.i32);
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

// -------------------------------------------------------------------------------------------------

#define MAX_RESOLVE_DEPTH 64

typedef struct {
  Declaration *decl;
  CallStack    call_stack;
  u8           min_required_resolve_status;
} ResolveEntry;

typedef struct {
  Arena *scratch;
  u32 declaration_count;
  Declaration **declarations;
  Interpreter *in;
  Stack(ResolveEntry) resolve_stack;
  b32 is_new_entry;
} Resolver;

internal CallStack alloc_callstack(Arena *arena) {
  CallStack stack;
  stack_init(&stack, arena_push_array(CallFrame, arena, MAX_CALL_DEPTH), MAX_CALL_DEPTH);
  return stack;
}

internal b32 resolve_run_entry_until(Resolver *resolver, ResolveEntry *entry, u32 end) {
  u32 err = run_until(resolver->in, &entry->call_stack, end, !resolver->is_new_entry);

  if (err == Run_resolve_declaration_type || err == Run_resolve_declaration_value) {
    CallFrame *f = stack_peek_ptr_unchecked(&entry->call_stack);
    DeclarationIndex idx = chunk_data(f->chunk, f->pc);

    Declaration *decl = resolver->declarations[idx];

    CallStack call_stack = alloc_callstack(resolver->scratch);
    frame_push(&call_stack, resolver->scratch, &decl->data.decl.chunk);

    u8 min_required_resolve_status;
    switch (err) {
    case Run_resolve_declaration_type:  min_required_resolve_status = ResolveStatus_type_resolved;  break;
    case Run_resolve_declaration_value: min_required_resolve_status = ResolveStatus_fully_resolved; break;
    default: Unreachable();
    }

    stack_push_unchecked(
      &resolver->resolve_stack,
      ((ResolveEntry){
        .decl       = decl,
        .call_stack = call_stack, 
        .min_required_resolve_status = min_required_resolve_status,
      })
    );

    resolver->is_new_entry = True;

    return True;
  }

  return False;
}

b32 resolve_declarations(Resolver *resolver) {
  for (u32 i = 0; i < resolver->declaration_count; i++) {
    {
      Declaration *decl = resolver->declarations[i];

      u8 resolve_status = decl->resolve_status;

      if (resolve_status == ResolveStatus_fully_resolved) {
        continue;
      }

      Assert(resolve_status == ResolveStatus_unresolved || resolve_status == ResolveStatus_type_resolved);

      CallStack call_stack = alloc_callstack(resolver->scratch);
      frame_push(&call_stack, resolver->scratch, &decl->data.decl.chunk);

      stack_push(
        &resolver->resolve_stack,
        ((ResolveEntry){ 
          .decl       = decl,
          .call_stack = call_stack, 
          .min_required_resolve_status = ResolveStatus_fully_resolved,
        })
      );

      resolver->is_new_entry = True;
    }

    while (!stack_is_empty(&resolver->resolve_stack)) {
      ResolveEntry *entry = stack_peek_ptr_unchecked(&resolver->resolve_stack);
      Declaration *decl = entry->decl;

      u8 resolve_status = decl->resolve_status;

      if (resolve_status >= entry->min_required_resolve_status) {
        stack_pop_unchecked(&resolver->resolve_stack); // free callstack of entry?
        resolver->is_new_entry = False;
        continue;
      }

      if (resolver->is_new_entry && resolve_status == ResolveStatus_resolving_type) {
        Panic();
      }

      if (resolver->is_new_entry && resolve_status == ResolveStatus_resolving_value) {
        Panic();
      }

      if (resolve_status < ResolveStatus_type_resolved) {
        decl->resolve_status = ResolveStatus_resolving_type;
        b32 has_pushed = resolve_run_entry_until(resolver, entry, decl->data.decl.typecheck_end);
        if (has_pushed) {
          continue;
        }

        decl->resolve_status = ResolveStatus_type_resolved;
      }

      if (resolve_status < ResolveStatus_fully_resolved) {
        decl->resolve_status = ResolveStatus_resolving_value;
        b32 has_pushed = resolve_run_entry_until(resolver, entry, decl->data.decl.chunk.opcode_count);
        if (has_pushed) {
          continue;
        }

        CallFrame *f = stack_peek_ptr_unchecked(&entry->call_stack);
        decl->data.decl.val = values_copy(resolver->in->values, f->inst_map[0]);
        decl->resolve_status = ResolveStatus_fully_resolved;
      }
    }
  }

  return True;
}

// -------------------------------------------------------------------------------------------------

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
    stack_init(&stack, stackmem, 4);
    stack_push(&stack, ((DeclFrame){ .mod = 0, .n = source->decls[0].child_count }));

    source->decl_idxs[0] = 0;

    u32 offset = 1;

    while (!stack_is_empty(&stack)) {
      DeclFrame *top = stack_peek_ptr_unchecked(&stack);

      if (top->n == 0) {
        stack_pop_unchecked(&stack);
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

        stack_push(&stack, ((DeclFrame){ .mod = idx, .n = decl->child_count }));

        source->decl_idxs[offset] = idx;

        decls_set_extra(
          &compiler->decls,
          idx,
          (Declaration){ .kind = Declaration_mod, .data.mod = { .source = source, .tree_idx = offset }}
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

        user_decls_append(&compiler->user_decls, &compiler->arena, idx);

        source->decl_idxs[offset] = idx;

        decls_set_extra(&compiler->decls, idx, (Declaration){
          .kind = Declaration_decl,
          .data.decl = {
            .source = source,
            .tree_idx = offset,
          },
        });
      }

next_iter:
      offset += 1;
    }
  }

  Declaration **decls = arena_push_array(Declaration*, &compiler->scratch, compiler->user_decls.len);

  {
    CodeGenContext context = {
      .perm     = &compiler->arena,
      .scratch  = &compiler->scratch,
      .common   = &compiler->common,
      .msg_sink = &compiler->msg_sink,
      .strings  = &compiler->strings,
      .decls    = &compiler->decls,
      .values   = &compiler->values,
    };

    for (u32 i = 0; i < compiler->user_decls.len; i++) {
      Declaration *decl = decls_extra_get_ptr(&compiler->decls, user_decls_at_unchecked(&compiler->user_decls, i));
      decls[i] = decl;
      is_ok &= generate_code(&context, decl);
      ir_chunk_print(stdout, &decl->data.decl.chunk, &compiler->types, &compiler->values);
    }
  }

  {
    Interpreter interpreter = {
      .perm = &compiler->arena,
      .scratch = &compiler->scratch,
      .msg_sink = &compiler->msg_sink,
      .declarations = &compiler->decls,
      .types = &compiler->types,
      .values = &compiler->values,
      .common = &compiler->common,
    };

    IrBuilder *builders = arena_push_array(IrBuilder, &compiler->scratch, MAX_BUILDERS);
    stack_init(&interpreter.builders, builders, MAX_BUILDERS);

    Resolver resolver = {
      .scratch = &compiler->scratch,
      .declaration_count = compiler->user_decls.len,
      .declarations = decls,
      .in = &interpreter,
    };

    ResolveEntry *entries = arena_push_array(ResolveEntry, &compiler->scratch, MAX_RESOLVE_DEPTH);
    stack_init(&resolver.resolve_stack, entries, MAX_RESOLVE_DEPTH);

    b32 ok = resolve_declarations(&resolver);

    if (!ok) {
      return False;
    }
  }

  for (u32 i = 0; i < compiler->user_decls.len; i++) {
    DeclarationIndex idx  = user_decls_at_unchecked(&compiler->user_decls, i);
    DeclarationKey   key  = decls_get(&compiler->decls, idx);
    Declaration     *decl = decls_extra_get_ptr(&compiler->decls, idx);
    String           name = strings_get(&compiler->strings, key.name);

    ValueIndex val = decl->data.decl.val;

    printf("%.*s : ", Cast(int, name.len), name.str);
    type_index_print(stdout, &compiler->types, values_get(&compiler->values, val)->type);
    printf(" = ");
    value_print(stdout, &compiler->types, &compiler->values, val);
    printf("\n");
  }

  return is_ok;
}
