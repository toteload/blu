#ifndef ENV_H
#define ENV_H

#include "blu.h"

enum ResolveStatus {
  ResolveStatus_unresolved,
  ResolveStatus_resolving_type,
  ResolveStatus_resolved_type,
  ResolveStatus_resolving_value,
  ResolveStatus_resolved,
};

enum DeclarationScope {
  Scope_builtin,
  Scope_module,
  Scope_parameter,
  Scope_local,
};

typedef struct {
  // `resolve_status` is only relevant to module scoped declarations.
  u8 resolve_status;
  u8 scope;
  u32 attributes;
  AstIndex       ast_index;
  ValueIndex     value;
} Declaration;

#define HASHMAP_NAME       DeclarationMap
#define HASHMAP_KEY_TYPE   StringIndex
#define HASHMAP_VALUE_TYPE Declaration
#define HASHMAP_OUTPUT_TYPES
#include "hashmap.h"

typedef struct Env {
  struct Env     *parent;
  DeclarationMap  map;
} Env;

void env_insert(Env *env, StringIndex idx, Declaration decl);
b32  env_contains(Env *env, StringIndex idx);
b32  env_lookup(Env *env, StringIndex idx, Declaration *decl);

typedef struct {
  Arena *arena;
  void *freelist;
  Allocator map_allocator;
} EnvAllocator;

typedef struct {
  Arena *arena;
  Allocator map_allocator;
} EnvAllocatorOptions;

void  envs_init(EnvAllocator *envs, EnvAllocatorOptions *options);
Env  *envs_alloc(EnvAllocator *envs, Env *parent);
void  envs_dealloc(EnvAllocator *envs, Env *env);

#endif // ENV_H
