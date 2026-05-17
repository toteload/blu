#include "blu.hh"
#include "utils/stdlib.hh"

void SourceUnit::init(Str filename, Str text) {
  stage = Stage_tokenize;

  arena.init(MiB(2));
  work_arena.init(MiB(8));

  messages.init(stdlib_alloc);

  this->filename = filename;
  this->text     = text;
}

void SourceUnit::deinit() {
  arena.deinit();
  work_arena.deinit();

  messages.deinit();

  if (stage > Stage_tokenize) {
    tokens.deinit();
  }

  if (stage > Stage_parse) {
    nodes.deinit();
  }

  if (stage > Stage_typecheck) {
    types.deinit();
    strings.deinit();
  }

  if (stage > Stage_run_const_code) {
    interpreter.deinit();
  }

  memset(this, 0, sizeof(*this));
}

bool SourceUnit::tokenize() {
  Assert(stage == Stage_tokenize);

  tokens.init(stdlib_alloc);

  TokenizeContext context{};
  context.messages = &messages;

  bool ok = ::tokenize(&context, text, &tokens);
  if (!ok) {
    return false;
  }

  stage = Stage_parse;

  return true;
}

bool SourceUnit::parse() {
  Assert(stage == Stage_parse);

  nodes.init(stdlib_alloc, arena.as_allocator());

  ParseContext context{};
  context.messages = &messages;

  b32 ok = parse_root(&context, &tokens, &nodes);
  if (!ok) {
    return false;
  }

  stage = Stage_typecheck;

  return true;
}

bool SourceUnit::typecheck() {
  Assert(stage == Stage_typecheck);

  strings.init(arena.as_allocator(), stdlib_alloc, stdlib_alloc);
  types.init(&work_arena, arena.as_allocator(), stdlib_alloc, stdlib_alloc);

  EnvManager<Declaration> envs;
  envs.init(stdlib_alloc, stdlib_alloc);
  defer(envs.deinit());

  TypeCheckContext context{};
  context.messages   = &messages;
  context.envs       = &envs;
  context.types      = &types;
  context.strings    = &strings;
  context.work_arena = &work_arena;

  ParsedSource source = {
    .text   = text,
    .tokens = &tokens,
    .nodes  = &nodes,
  };

  bool ok = ::typecheck(&context, &source, nodes.first_valid_index());
  if (!ok) {
    return false;
  }

  stage = Stage_run_const_code;

  return true;
}

bool SourceUnit::run_const_code() {
  Assert(stage == Stage_run_const_code);

  InterpreterContext context{};
  context.types      = &types;
  context.strings    = &strings;
  context.messages   = &messages;
  context.text       = text;
  context.tokens     = &tokens;
  context.nodes      = &nodes;

  interpreter.init(&context);

  bool ok = interpreter.prepare_code();
  if (!ok) {
    return false;
  }

  stage = Stage_done;

  return true;
}

bool SourceUnit::run_main(ValueIndex *result) {
  Assert(stage == Stage_done);
  return interpreter.run_main(result);
}

void SourceUnit::print_messages() {
  MessageContext context;
  context.text    = text;
  context.tokens  = &tokens;
  context.nodes   = &nodes;
  context.types   = &types;
  context.strings = &strings;

  messages.print_messages(&context);
}
