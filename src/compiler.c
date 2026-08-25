#include "compiler.h"
#include "source_file.h"
#include "string_interner.h"
#include "codegen.h"
#include "specialize.h"
#include "interpret.h"
#include "ir.h"
#include "print.h"

#include <stdarg.h>

#define SEGMENTLIST_NAME SourceList
#define SEGMENTLIST_TYPE Source
#define SEGMENTLIST_FUNCTION_PREFIX sources
#define SEGMENTLIST_MIN_SIZE_LOG2 SOURCELIST_MIN_SIZE_LOG2
#define SEGMENTLIST_SEGMENT_COUNT SOURCELIST_SEGMENT_COUNT
#define SEGMENTLIST_LINKAGE internal
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

#define SEGMENTLIST_NAME DeclIdxList
#define SEGMENTLIST_TYPE DeclarationIndex
#define SEGMENTLIST_FUNCTION_PREFIX user_decls
#define SEGMENTLIST_MIN_SIZE_LOG2 DECLIDXLIST_MIN_SIZE_LOG2
#define SEGMENTLIST_SEGMENT_COUNT DECLIDXLIST_SEGMENT_COUNT
#define SEGMENTLIST_LINKAGE internal
#define SEGMENTLIST_OUTPUT_DEFINITIONS
#include "segment_list.h"

#define XXH_INLINE_ALL
#include "xxhash.h"

internal u32 hash_decl_key(void *context, DeclarationKey key) {
  Unused(context);
  return XXH32(&key, sizeof(DeclarationKey), 0);
}

internal b32 cmp_decl_key(void *context, DeclarationKey a, DeclarationKey b) {
  Unused(context);
  return a.parent == b.parent && a.name == b.name;
}

#define INTERNER_NAME DeclarationInterner
#define INTERNER_TYPE DeclarationKey
#define INTERNER_INDEX_TYPE DeclarationIndex
#define INTERNER_EXTRA_TYPE Declaration
#define INTERNER_RESERVE_ZERO_INDEX
#define INTERNER_FUNCTION_PREFIX decls
#define INTERNER_HASH_FN hash_decl_key
#define INTERNER_COMPARE_FN cmp_decl_key
#define INTERNER_OUTPUT_DEFINITIONS
#include "interner.h"

internal void *
cstd_alloc_fn(void *ctx, void *p, usize old_byte_size, usize new_byte_size, u32 align) {
  Unused(ctx, old_byte_size, align);

  if (!is_null(p) && new_byte_size == 0) {
    free(p);
    return Null;
  }

  return realloc(p, new_byte_size);
}

Allocator const cstd_allocator = {
  .fn = cstd_alloc_fn,
};

internal void
compiler_add_message(void *user, u8 severity, MessageLocation location, String format, ...) {
  Compiler *compiler = user;

  u32 arg_count = message_format_arg_count(format);

  Message *msg = arena_push(
    &compiler->arena,
    sizeof(Message) + arg_count * sizeof(MessageArg),
    Align_of(Message)
  );

  msg->severity = severity;
  msg->location = location;
  msg->format = arena_copy_string(&compiler->arena, format);

  va_list vl;
  va_start(vl, format);
  message_collect_args(format, vl, msg->args, arg_count);
  va_end(vl);

  msglist_append(&compiler->msg_list, &compiler->arena, msg);
}

internal ValueIndex add_type_value(Compiler *compiler, TypeIndex t) {
  Value *v;
  ValueIndex res = values_alloc(&compiler->values, &v);
  TypeIndex *data = values_alloc_data_type(&compiler->values, TypeIndex);
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
    (DeclarationKey){.parent = 0, .name = strings_add(&compiler->strings, name)}
  );

  decls_set_extra(
    &compiler->decls,
    idx,
    (Declaration){.idx = idx, .kind = Declaration_primitive, .data.primitive = val}
  );
}

void compiler_init(Compiler *compiler, CLIOptions *options) {
  zero_struct(Compiler, compiler);

  compiler->options = options;

  arena_init(
    &compiler->arena,
    &(ArenaOptions){
      .reserve_size = MiB(16),
      .initial_commit_size = MiB(1),
    }
  );

  arena_init(
    &compiler->scratch,
    &(ArenaOptions){
      .reserve_size = MiB(16),
      .initial_commit_size = MiB(1),
    }
  );

  compiler->msg_sink = (MessageSink){
    .user = compiler,
    .add_message = compiler_add_message,
  };

  // Reserve 0 index to be nil value
  sources_push(&compiler->sources, &compiler->arena);

  strings_init(
    &compiler->strings,
    &(InternerOptions){
      .arena = &compiler->arena,
      .map_allocator = cstd_allocator,
      .map_initial_size = 32,
    }
  );

  types_init(
    &compiler->types,
    &(InternerOptions){
      .arena = &compiler->arena,
      .map_allocator = cstd_allocator,
      .map_initial_size = 32,
      .context = &compiler->scratch,
    }
  );

  values_init(
    &compiler->values,
    &(ValueStoreOptions){
      .arena = &compiler->arena,
      .payload_allocator = cstd_allocator,
    }
  );

  decls_init(
    &compiler->decls,
    &(InternerOptions){
      .arena = &compiler->arena,
      .map_allocator = cstd_allocator,
      .map_initial_size = 32,
    }
  );

  // clang-format off

  compiler->common.type.comptime_int = types_add(&compiler->types, &(Type){.kind = Type_comptime_int});
  compiler->common.type.type = types_add(&compiler->types, &(Type){.kind = Type_type});
  compiler->common.type.nil = types_add(&compiler->types, &(Type){.kind = Type_nil});
  compiler->common.type.bool = types_add(&compiler->types, &(Type){.kind = Type_bool});
  compiler->common.type.never = types_add(&compiler->types, &(Type){.kind = Type_never});
  compiler->common.type.u8 = types_add( &compiler->types, &(Type){.kind = Type_integer, .data.integer = {.signedness = Unsigned, .bitwidth = 8}});
  compiler->common.type.i32 = types_add( &compiler->types, &(Type){.kind = Type_integer, .data.integer = {.signedness = Signed, .bitwidth = 32}});

  TypeIndex ti_i8 = types_add( &compiler->types, &(Type){.kind = Type_integer, .data.integer = {.signedness = Signed, .bitwidth = 8}});

  compiler->common.val.type = add_type_value(compiler, compiler->common.type.type);
  compiler->common.val.nil = add_type_value(compiler, compiler->common.type.nil);
  compiler->common.val.bool = add_type_value(compiler, compiler->common.type.bool);
  compiler->common.val.never = add_type_value(compiler, compiler->common.type.never);
  compiler->common.val.i32 = add_type_value(compiler, compiler->common.type.i32);
  compiler->common.val.i8 = add_type_value(compiler, ti_i8);
  compiler->common.val.u8 = add_type_value(compiler, compiler->common.type.u8);

  // clang-format on

  {
    Value *v;
    compiler->common.val.true = values_alloc(&compiler->values, &v);
    u8 *data = values_alloc_data(&compiler->values, 1, 1);
    *data = 1;
    *v = (Value){
      .type = compiler->common.type.bool,
      .data_size = 1,
      .data = data,
    };
    add_primitive(compiler, string_lit("true"), compiler->common.val.true);
  }

  {
    Value *v;
    compiler->common.val.false = values_alloc(&compiler->values, &v);
    u8 *data = values_alloc_data(&compiler->values, 1, 1);
    *data = 0;
    *v = (Value){
      .type = compiler->common.type.bool,
      .data_size = 1,
      .data = data,
    };
    add_primitive(compiler, string_lit("false"), compiler->common.val.false);
  }

  add_primitive(compiler, string_lit("type"), compiler->common.val.type);
  add_primitive(compiler, string_lit("nil"), compiler->common.val.nil);
  add_primitive(compiler, string_lit("bool"), compiler->common.val.bool);
  add_primitive(compiler, string_lit("never"), compiler->common.val.never);
  add_primitive(compiler, string_lit("i32"), compiler->common.val.i32);
  add_primitive(compiler, string_lit("i8"), compiler->common.val.i8);
  add_primitive(compiler, string_lit("u8"), compiler->common.val.u8);
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

Source *compiler_get_source(Compiler *compiler, SourceIndex source_idx) {
  return sources_ptr_at_unchecked(&compiler->sources, source_idx);
}

void compiler_print_all_messages(Compiler *compiler) {
  for (u32 i = 1; i < compiler->sources.len; i++) {
    Source *source = sources_ptr_at_unchecked(&compiler->sources, i);
    source_print_all_messages(source, &compiler->scratch);
  }

  u32 count = compiler->msg_list.len;
  for (u32 i = 0; i < count; i++) {
    Message *msg = msglist_at_unchecked(&compiler->msg_list, i);
    Source *source = Null;
    Declaration *decl = Null;

    if (msg->location.decl_idx) {
      decl = decls_extra_get_ptr(&compiler->decls, msg->location.decl_idx);
    }

    if (msg->location.source_idx) {
      source = sources_ptr_at_unchecked(&compiler->sources, msg->location.source_idx);
    }

    print_message(&compiler->scratch, msg, source, decl);
  }
}

b32 lookup_identifier(
  DeclarationInterner *decl_keys,
  DeclarationIndex *mods,
  u32 mod_count,
  StringIndex name,
  DeclarationIndex *out
) {
  for (u32 i = 0; i < mod_count; i++) {
    DeclarationKey key = {
      .parent = mods[i],
      .name = name,
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
  RunState state;
  u8 min_required_resolve_status;
} ResolveEntry;

typedef struct {
  b32 ok;

  Arena *scratch;
  MessageSink *msg_sink;

  // List of user defined declarations.
  u32 user_declaration_count;
  Declaration **user_declarations;

  DeclarationInterner *decls;

  Specializer *in;
  Stack(ResolveEntry) resolve_stack;
} Resolver;

internal void
push_resolve_entry(Resolver *resolver, Declaration *decl, u8 min_required_resolve_status) {
  ResolveEntry *entry = stack_push_ptr_unchecked(&resolver->resolve_stack);

  entry->decl = decl;
  entry->min_required_resolve_status = min_required_resolve_status;

  runstate_init(&entry->state, resolver->scratch);

  frame_push(&entry->state, resolver->scratch, decl);
}

internal b32 resolve_entry(Resolver *resolver) {
  ResolveEntry *entry = stack_peek_ptr_unchecked(&resolver->resolve_stack);
  Declaration *decl = entry->decl;
  SIrChunk *chunk = &decl->data.decl.chunk;

  u8 resolve_status = decl->resolve_status;

  if (!entry->state.requested_resolution) {
    decl->resolve_status = (resolve_status + 1);

    CallFrame *frame = top_frame(&entry->state);
    ScopeSpan *scope = push_scope(frame);

    if (resolve_status < ResolveStatus_type_resolved) {
      InstructionIndex block = decl->data.decl.block_type;
      u32 count = sir_chunk_data(chunk, block);

      *scope = (ScopeSpan){
        .scope_kind = Scope_eval_block,
        .start = block,
        .end = block + count,
        .pc = block + 1,
      };
    } else {
      InstructionIndex block = decl->data.decl.block_val;
      u32 count = sir_chunk_data(chunk, block);

      *scope = (ScopeSpan){
        .scope_kind = Scope_eval_block,
        .start = block,
        .end = block + count,
        .pc = block + 1,
      };
    }
  }

  u32 err = run_toplevel_block(resolver->in, &entry->state);

  if (err == Run_ok) {
    if (resolve_status < ResolveStatus_type_resolved) {
      decl->resolve_status = ResolveStatus_type_resolved;
      CallFrame *f = top_frame(&entry->state);

      IRef ref = f->inst_map[decl->data.decl.block_type];
      Assert(iref_is_some_value(ref));

      Value *v = values_get(resolver->in->values, iref_to_value(ref));
      Assert(v->type == resolver->in->common->type.type);

      TypeIndex type = *Cast(TypeIndex*,v->data);

      Assert(type != 0);

      decl->data.decl.type = type;
    } else {
      decl->resolve_status = ResolveStatus_fully_resolved;

      CallFrame *f = top_frame(&entry->state);

      IRef ref = f->inst_map[decl->data.decl.block_val];
      Assert(iref_is_some_value(ref));

      decl->data.decl.val = values_copy(resolver->in->values, iref_to_value(ref));
    }

    return True;
  }

  if (err == Run_encountered_error) {
    return False;
  }

  if (err == Run_resolve_declaration_type || err == Run_resolve_declaration_value) {
    CallFrame *f = top_frame(&entry->state);
    ScopeSpan *s = stack_peek_ptr(&f->scopes);
    DeclarationIndex idx = sir_chunk_data(f->chunk, s->pc);

    Declaration *decl_to_resolve = decls_extra_get_ptr(resolver->decls, idx);

    u8 min_required_resolve_status;
    switch (err) {
    case Run_resolve_declaration_type:
      min_required_resolve_status = ResolveStatus_type_resolved;
      break;
    case Run_resolve_declaration_value:
      min_required_resolve_status = ResolveStatus_fully_resolved;
      break;
    default:
      Unreachable();
    }

    push_resolve_entry(resolver, decl_to_resolve, min_required_resolve_status);

    return True;
  }

  Unreachable();
}

internal void clear_resolve_stack_with_error(Resolver *resolver) {
  resolver->ok = False;

  while (!stack_is_empty(&resolver->resolve_stack)) {
    ResolveEntry *entry = stack_peek_ptr_unchecked(&resolver->resolve_stack);
    entry->decl->resolve_status = ResolveStatus_error;
    stack_pop_unchecked(&resolver->resolve_stack);
  }
}

b32 resolve_declarations(Resolver *resolver) {
  for (u32 i = 0; i < resolver->user_declaration_count; i++) {
    {
      Declaration *decl = resolver->user_declarations[i];

      u8 resolve_status = decl->resolve_status;

      if (resolve_status == ResolveStatus_fully_resolved || resolve_status == ResolveStatus_error) {
        continue;
      }

      Assert(
        resolve_status == ResolveStatus_unresolved || resolve_status == ResolveStatus_type_resolved
      );

      push_resolve_entry(resolver, decl, ResolveStatus_fully_resolved);
    }

    while (!stack_is_empty(&resolver->resolve_stack)) {
      ResolveEntry *entry = stack_peek_ptr_unchecked(&resolver->resolve_stack);

      u8 resolve_status = entry->decl->resolve_status;

      if (resolve_status >= entry->min_required_resolve_status) {
        stack_pop_unchecked(
          &resolver->resolve_stack
        ); // TODO: free callstack of entry? yeah, sounds like a good idea
        continue;
      }

      if (!entry->state.requested_resolution && resolve_status == ResolveStatus_resolving_type) {
        Message_error(
          resolver->msg_sink,
          (MessageLocation){
            .kind = MessageLocation_unspecified,
            .decl_idx = entry->decl->idx,
          },
          string_lit("Encountered circular declaration")
        );

        clear_resolve_stack_with_error(resolver);
        break;
      }

      if (!entry->state.requested_resolution && resolve_status == ResolveStatus_resolving_value) {
        Message_error(
          resolver->msg_sink,
          (MessageLocation){
            .kind = MessageLocation_unspecified,
            .decl_idx = entry->decl->idx,
          },
          string_lit("Encountered circular declaration")
        );

        clear_resolve_stack_with_error(resolver);
        break;
      }

      b32 ok = resolve_entry(resolver);
      if (!ok) {
        clear_resolve_stack_with_error(resolver);
      }
    }
  }

  return resolver->ok;
}

// -------------------------------------------------------------------------------------------------

typedef struct {
  DeclarationIndex mod;
  u32 n;
} DeclFrame;

b32 compile(Compiler *compiler) {
  b32 is_ok = True;
  for (u32 i = 1; i < compiler->sources.len; i++) {
    Source *source = sources_ptr_at_unchecked(&compiler->sources, i);

    b32 ok;
    ok = source_read_file(source);
    if (!ok) {
      is_ok = False;
      source->status = SourceStatus_failed_to_parse;
      continue;
    }

    ok = source_tokenize(source, &compiler->scratch);
    if (!ok) {
      is_ok = False;
      source->status = SourceStatus_failed_to_parse;
      continue;
    }

    if (compiler->options->print_tokens) {
      print_tokens(&source->tokens, source->text);
    }

    ok = source_parse(source, &compiler->scratch);
    if (!ok) {
      is_ok = False;
      source->status = SourceStatus_failed_to_parse;
      continue;
    }

    source_index_declarations(source, &compiler->strings);

    source->status = SourceStatus_parsed;
  }

  if (compiler->options->print_ast) {
    for (u32 i = 1; i < compiler->sources.len; i++) {
      Source *source = sources_ptr_at_unchecked(&compiler->sources, i);
      print_ast_nodes(&source->ast, &source->tokens, source->text);
    }
  }

  if (!is_ok) {
    return False;
  }

  // Add all source file declarations to the global declaration map
  for (u32 i = 1; i < compiler->sources.len; i++) {
    Source *source = sources_ptr_at_unchecked(&compiler->sources, i);

    // At the moment it is not possible to nest modules, so this stack will never grow beyond 2 (1.
    // root, 2. mod). BUT this will likely change in the future. At that point some checks need to
    // be inserted to ensure the stack doesn't overflow (and also make the stack a bit bigger :)).

    DeclFrame stackmem[4];
    Stack(DeclFrame) stack;
    stack_init(&stack, stackmem, 4);
    stack_push(&stack, ((DeclFrame){.mod = 0, .n = source->decls[0].child_count}));

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
          (DeclarationKey){.parent = top->mod, .name = decl->name},
          &already_exists
        );

        if (already_exists) {
          Declaration val = decls_get_extra(&compiler->decls, idx);
          if (val.kind != Declaration_mod) {
            is_ok = False;
            Message_error(
              &compiler->msg_sink,
              (MessageLocation){
                .kind = MessageLocation_ast_index,
                .source_idx = source->idx,
                .data.ast_index = decl->node,
              },
              string_lit("Declaration already exists.")
            );
            goto next_iter;
          }
        }

        stack_push(&stack, ((DeclFrame){.mod = idx, .n = decl->child_count}));

        source->decl_idxs[offset] = idx;

        decls_set_extra(
          &compiler->decls,
          idx,
          (Declaration){
            .idx = idx,
            .kind = Declaration_mod,
            .resolve_status = ResolveStatus_fully_resolved,
            .data.mod = {.source = source, .tree_idx = offset},
          }
        );
      } else if (decl->kind == SourceDeclaration_declaration) {
        b32 already_exists;
        DeclarationIndex idx = decls_add_checked(
          &compiler->decls,
          (DeclarationKey){.parent = top->mod, .name = decl->name},
          &already_exists
        );

        if (already_exists) {
          is_ok = False;
          Message_error(
            &compiler->msg_sink,
            (MessageLocation){
              .kind = MessageLocation_ast_index,
              .source_idx = source->idx,
              .data.ast_index = decl->node,
            },
            string_lit("Declaration already exists.")
          );
          goto next_iter;
        }

        user_decls_append(&compiler->user_decls, &compiler->arena, idx);

        source->decl_idxs[offset] = idx;

        decls_set_extra(
          &compiler->decls,
          idx,
          (Declaration){
            .idx = idx,
            .kind = Declaration_decl,
            .resolve_status = ResolveStatus_unresolved,
            .data.decl = {
              .source = source,
              .tree_idx = offset,
            },
          }
        );
      }

    next_iter:
      offset += 1;
    }
  }

  Declaration **user_decls =
    arena_push_array(Declaration *, &compiler->scratch, compiler->user_decls.len);

  {
    CodeGenContext context = {
      .perm = &compiler->arena,
      .scratch = &compiler->scratch,
      .gpa = cstd_allocator,
      .common = &compiler->common,
      .msg_sink = &compiler->msg_sink,
      .strings = &compiler->strings,
      .decls = &compiler->decls,
      .values = &compiler->values,
      .types = &compiler->types,
    };

    for (u32 i = 0; i < compiler->user_decls.len; i++) {
      Declaration *decl =
        decls_extra_get_ptr(&compiler->decls, user_decls_at_unchecked(&compiler->user_decls, i));
      user_decls[i] = decl;

      is_ok &= generate_code(&context, decl);

      if (compiler->options->print_decl_ir) {
        print_sir_chunk(stdout, compiler, &decl->data.decl.chunk);
      }
    }
  }

  if (!is_ok) {
    return False;
  }

  {
    Specializer specializer = {
      .perm = &compiler->arena,
      .scratch = &compiler->scratch,
      .msg_sink = &compiler->msg_sink,
      .declarations = &compiler->decls,
      .types = &compiler->types,
      .values = &compiler->values,
      .common = &compiler->common,
    };

    IIrBuilder *builders = arena_push_array(IIrBuilder, &compiler->scratch, MAX_BUILDERS);
    stack_init(&specializer.builders, builders, MAX_BUILDERS);

    Resolver resolver = {
      .ok = True,
      .scratch = &compiler->scratch,
      .user_declaration_count = compiler->user_decls.len,
      .user_declarations = user_decls,
      .decls = &compiler->decls,
      .in = &specializer,
      .msg_sink = &compiler->msg_sink,
    };

    ResolveEntry *entries = arena_push_array(ResolveEntry, &compiler->scratch, MAX_RESOLVE_DEPTH);
    stack_init(&resolver.resolve_stack, entries, MAX_RESOLVE_DEPTH);

    b32 ok = resolve_declarations(&resolver);

    if (!ok) {
      return False;
    }
  }

  if (compiler->options->print_residual) {
    for (u32 i = 0; i < compiler->user_decls.len; i++) {
      DeclarationIndex idx = user_decls_at_unchecked(&compiler->user_decls, i);
      DeclarationKey key = decls_get(&compiler->decls, idx);
      Declaration *decl = decls_extra_get_ptr(&compiler->decls, idx);
      String name = strings_get(&compiler->strings, key.name);

      ValueIndex val = decl->data.decl.val;

      printf("%.*s : ", Cast(int, name.len), name.str);
      print_type(stdout, &compiler->types, values_get(&compiler->values, val)->type);
      printf(" = ");
      Value *v = values_get(&compiler->values, val);
      print_value_raw(stdout, compiler, PrintFlag_expand_function, v->type, v->data);
      printf("\n");
    }
  }

  return is_ok;
}

b32 run_main(Compiler *compiler) {
  DeclarationIndex mod_main;
  b32 ok = lookup_identifier(
    &compiler->decls,
    (DeclarationIndex[]){0},
    1,
    strings_add(&compiler->strings, string_lit("main")),
    &mod_main
  );
  if (!ok) {
    Message_error(
      &compiler->msg_sink,
      (MessageLocation){
        .kind = MessageLocation_unspecified,
      },
      string_lit("Module 'main' not found")
    );

    return False;
  }

  DeclarationIndex fn_main;
  ok = lookup_identifier(
    &compiler->decls,
    (DeclarationIndex[]){mod_main},
    1,
    strings_add(&compiler->strings, string_lit("main")),
    &fn_main
  );
  if (!ok) {
    Message_error(
      &compiler->msg_sink,
      (MessageLocation){
        .kind = MessageLocation_unspecified,
      },
      string_lit("Declaration 'main' not found in module 'main'")
    );
    return False;
  }

  Declaration *decl_main = decls_extra_get_ptr(&compiler->decls, fn_main);

  // TODO: make sure main has the correct type

  Value *v = values_get(&compiler->values, decl_main->data.decl.val);
  IIrChunk *chunk = &Cast(ValueFunc *, v->data)->chunk;

  Interpreter in = {
    .scratch = &compiler->scratch,
    .msg_sink = &compiler->msg_sink,
    .compiler = compiler,
  };

  stack_init(&in.call_stack, arena_push_array(CallFrame2, &compiler->scratch, 64), 64);

  u8 buf[16];
  u32 err = interpreter_call(&in, chunk, (ValueIndex[]){0}, 0, &buf);
  if (err) {
    return False;
  }

  return True;
}
