#include "blu.hh"

void Program::init() {
  arena.init(MiB(2));
  work_arena.init(MiB(8));

  types.init(&work_arena, stdlib_alloc, stdlib_alloc, stdlib_alloc);
  strings.init(arena.as_allocator(), stdlib_alloc, stdlib_alloc);

  // TODO messages.source = &source;
  messages.init(stdlib_alloc, &strings, &types);
}

void Program::deinit() {
  arena.deinit();
  work_arena.deinit();

  types.deinit();
}

bool Program::load(Str source) {
  bool ok = true;

  // - Tokenize

  tokens.init(stdlib_alloc, stdlib_alloc);

  ok = tokenize(&messages, source, &tokens);

  // - Parse

  nodes.init();

  // - Typecheck

  Vector<TypeIndex> type_annotations;

  type_annotations.init(stdlib_alloc);
  type_annotations.set_size(nodes.kinds.len());
  memset(type_annotations.data, 0xff, nodes.kinds.len() * sizeof(TypeIndex));

  {
    EnvManager<Declaration> envs;
    envs.init(stdlib_alloc, stdlib_alloc);
    defer(envs.deinit());

    TypeCheckContext typecheck_context = {
      .messages   = &messages,
      .envs       = &envs,
      .types      = &types,
      .strings    = &strings,
      .values     = &values,
      .work_arena = &work_arena,
    };

    ok = typecheck(&typecheck_context, &source, type_annotations.slice());
  }

  return ok;
}

bool Program::interpret(Str source_name, Str source, ValueIndex *result) {
  bool ok = true;

  // - Tokenize

  tokens.init(stdlib_alloc, stdlib_alloc);

  ok = tokenize(&messages, source, &tokens);

  // - Parse

  nodes.init();

  // - Typecheck

  Vector<TypeIndex> type_annotations;

  type_annotations.init(stdlib_alloc);
  type_annotations.set_size(nodes.kinds.len());
  memset(type_annotations.data, 0xff, nodes.kinds.len() * sizeof(TypeIndex));

  {
    EnvManager<Declaration> envs;
    envs.init(stdlib_alloc, stdlib_alloc);
    defer(envs.deinit());

    TypeCheckContext typecheck_context = {
      .messages   = &messages,
      .envs       = &envs,
      .types      = &types,
      .strings    = &strings,
      .values     = &values,
      .work_arena = &work_arena,
    };

    ok = typecheck(&typecheck_context, &source, type_annotations.slice());
  }

  // - Interpret

  values.init(stdlib_alloc);

  {
    EnvManager<ValueIndex> envs;
    envs.init(stdlib_alloc, stdlib_alloc);
    defer(envs.deinit());

    Interpreter interpreter;
    interpreter.init(&strings, &types, &values, &value_envs, &work_arena, &messages);
    interpreter.type_annotations = type_annotations.slice();

    ok = interpreter.run(source, result);
  }

  return true;
}
