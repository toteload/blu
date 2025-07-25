#include "blu.hh"

b32 Compiler::add_source_file_job(Str filename) {
  Str filename = copy_str(&arena, filename);

  Source source;
  source.filename = filename;

  sources.push(source);

  Job job;
  job.kind = Job_read_file;
  job.source_idx;

  jobs.push_back(job);
}

void Compiler::run() {
  while (!jobs.is_empty()) {
    Job job = jobs.pop_front();

    SourceIdx src_idx = job.src_idx;

    Source *src = &sources[src_idx];

    switch (job.kind) {
    case Job_read_file: {
      Str text = read_file(&arena, src->filename);
      if (text.is_empty()) {
        Todo();
      }

      src->text = text;

      Job job;
      job.kind = Job_tokenize;
      job.src_idx = src_idx;

      jobs.push_back(job);
    } break;
    case Job_tokenize: {
      src->tokens.init(stdlib_alloc);

      TokenizeContext ctx;
      ctx.arena = &arena;
      ctx.messages = &messages;

      b32 ok = tokenize(ctx, src->text, &src->tokens);
      if (!ok) {
        Todo();
      }

      Job job;
      job.kind = Job_parse;
      job.src_idx = src_idx;

      jobs.push_back(job);
    } break;
    case Job_parse: {
      ParseContext ctx;
      ctx.arena = &arena;
      ctx.messages = &messages;
      ctx.strings = &strings;
      ctx.nodes = &nodes;

      b32 ok = parse(ctx, src->tokens.slice(), &src->mod);
      if (!ok) {
        Todo();
      }

      // TODO: if the file has includes add more read file jobs

      Job job;
      job.kind = Job_typecheck;
      job.src_idx = src_idx;

      jobs.push_back(job);
    } break;
    case Job_typecheck: {
      TypeCheckContext ctx;
      ctx.arena = &arena;
      ctx.messages = &messages;
      ctx.work_arena = &work_arena;
      ctx.envs = &envs;
      ctx.types = &types;
      ctx.nodes = &nodes;

      b32 ok = type_check_module(ctx, src->mod);
      if (!ok) {
          Todo();
        }
    } break;
    }
  }
}
