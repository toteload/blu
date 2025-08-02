#include "blu.hh"

void *stdlib_alloc_fn(void *ctx, void *p, usize old_byte_size, usize new_byte_size, u32 align) {
  if (!Is_null(p) && new_byte_size == 0) {
    free(p);
    return nullptr;
  }

  return realloc(p, new_byte_size);
}

static Allocator stdlib_alloc{stdlib_alloc_fn, nullptr};

Str read_file_aux(Arena *arena, Str filename) {
  char *buf = arena->alloc<char>(filename.len() + 1);
  memcpy(buf, filename.str, filename.len());
  buf[filename.len()] = '\0';

  FILE *f = fopen(buf, "rb");
  if (!f) {
    return Str::empty();
  }

  fseek(f, 0, SEEK_END);

  u32 size   = ftell(f);
  char *data = Cast(char *, malloc(size + 1));
  if (!data) {
    return Str::empty();
  }

  fseek(f, 0, SEEK_SET);

  u64 bytes_read = fread(data, 1, size, f);
  if (bytes_read != size) {
    return Str::empty();
  }

  // Set a zero-terminated byte, but don't include it in the length.
  data[size] = '\0';

  return {data, size};
}

Str read_file(Arena *arena, Str filename) {
  auto snapshot = arena->take_snapshot();
  auto res      = read_file_aux(arena, filename);
  arena->restore(snapshot);
  return res;
}

void Compiler::init() {
  arena.init(MiB(32));
  work_arena.init(MiB(32));
  strings.init(stdlib_alloc, stdlib_alloc, stdlib_alloc);
  messages.init(stdlib_alloc);
  types.init(&arena, stdlib_alloc, stdlib_alloc);
  nodes.init(stdlib_alloc);
  envs.init(stdlib_alloc, stdlib_alloc);
  envs.init_global_env(&strings, &types);
  sources.init(stdlib_alloc);
  jobs.init(stdlib_alloc);
}

Str copy_str(Arena *arena, Str s) {
  char *data = arena->alloc<char>(s.len());
  memcpy(data, s.str, s.len());
  return {data, s.len()};
}

SourceIdx Compiler::add_read_file_job(Str filename) {
  Str own_filename = copy_str(&arena, filename);

  Source source;
  source.filename = own_filename;

  SourceIdx src_idx = cast<SourceIdx>(sources.len());
  sources.push(source);

  Job job;
  job.kind    = Job_read_file;
  job.src_idx = src_idx;

  jobs.push_back(job);

  return src_idx;
}

void Compiler::compile_file(Str filename) {
  SourceIdx root_src_idx = add_read_file_job(filename);

  while (!jobs.is_empty()) {
    Job job = jobs.pop_front();

    SourceIdx src_idx = job.src_idx;

    Source *src = &sources[src_idx];

    switch (job.kind) {
    case Job_read_file: {
      //printf("- %.*s - Reading\n", cast<int>(src->filename.len()), src->filename.str);
      Str text = read_file(&arena, src->filename);
      if (text.is_empty()) {
        Todo();
      }

      src->text = text;

      Job job;
      job.kind    = Job_tokenize;
      job.src_idx = src_idx;

      jobs.push_back(job);
    } break;
    case Job_tokenize: {
      //printf("- %.*s - Tokenizing\n", cast<int>(src->filename.len()), src->filename.str);
      src->tokens.init(stdlib_alloc);

      TokenizeContext ctx;
      ctx.messages = &messages;

      b32 ok = tokenize(ctx, src->text, &src->tokens);
      if (!ok) {
        printf("Error encountered during tokenizing\n");
        Todo();
      }

      Job job;
      job.kind    = Job_parse;
      job.src_idx = src_idx;

      jobs.push_back(job);
    } break;
    case Job_parse: {
      //printf("- %.*s - Parsing\n", cast<int>(src->filename.len()), src->filename.str);
      ParseContext ctx;
      ctx.src_idx  = src_idx;
      ctx.messages = &messages;
      ctx.strings  = &strings;
      ctx.nodes    = &nodes;

      b32 ok = parse(ctx, src->tokens.slice(), &src->mod);
      if (!ok) {
        printf("Error encountered during parsing\n");
        messages.print_messages(sources.slice());
        Todo();
      }

      if (listener) {
        listener(this, Job_parse, src);
      }

      ForEachAstNode(n, src->mod->module.items) {
        if (n->kind == Ast_builtin && n->builtin.kind == Builtin_include) {
          Str filename_string = get_ast_str(n->builtin.value, src->tokens.slice());
          Str filename        = {filename_string.str + 1, filename_string.len() - 2};
          // printf("Adding source file job for %.*s\n", cast<int>(filename.len()), filename.str);
          SourceIdx src_idx  = add_read_file_job(filename);
          n->builtin.src_idx = src_idx;
        }
      }
    } break;
    }
  }

  Source *src = &sources[root_src_idx];

  //printf("- %.*s - Type checking\n", cast<int>(src->filename.len()), src->filename.str);

  TypeCheckContext ctx;
  ctx.arena      = &arena;
  ctx.messages   = &messages;
  ctx.work_arena = &work_arena;
  ctx.envs       = &envs;
  ctx.types      = &types;
  ctx.nodes      = &nodes;
  ctx.sources    = &sources;

  b32 ok = type_check_module(ctx, src->mod);
  if (!ok) {
    Todo();
  }
}
