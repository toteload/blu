#pragma once

#include "toteload.hh"
#include "vector.hh"
#include "arena_item_pool.hh"
#include "hashmap.hh"
#include "segment_list.hh"
#include "index.hh"
#include "string_interner.hh"

typedef u32 SourceIdx;

struct ValueIndexTag {};
using ValueIndex         = Index<u32, ValueIndexTag>;
using OptionalValueIndex = OptionalIndex<u32, ValueIndexTag>;

enum NodeIndexKind : u8 {
  NodeIndex_ast_node,
  NodeIndex_value,
};

struct NodeIndex {
  NodeIndexKind kind;

  // A value of 0 means not valid or nil.
  u32 idx;

  bool is_some() const { return idx != 0; }
  bool is_none() const { return idx == 0; }

  static NodeIndex none() {
    return {
      .kind = NodeIndex_ast_node, // The kind is invalid if `idx == 0`.
      .idx  = 0,
    };
  }
};

struct TypeIndexTag {};
using TypeIndex = Index<u32, TypeIndexTag>;

struct SourceLocation {
  u32 line = 0;
  u32 col  = 0;
};

struct SourceSpan {
  SourceLocation start;
  SourceLocation end;

  static SourceSpan from_single_location(SourceLocation loc) {
    return {
      loc,
      {
        loc.line,
        loc.col + 1,
      }
    };
  }
};

enum TypeKind : u8 {
  Type_literal_int,
  Type_literal_function,
  Type_integer,
  Type_boolean,
  Type_function,
  Type_nil,
  Type_never,
  Type_slice,
  Type_array,
  Type_distinct,
  Type_sequence,
  Type_type,
};

enum Signedness : u8 {
  Signed,
  Unsigned,
};

struct IntInfo {
  Signedness signedness;
  u16        bitwidth;
};

struct TypeSizeInfo {
  u32 size;
  u32 stride;
  u32 align;

  template<typename T> static TypeSizeInfo of_type() {
    return {
      .size   = sizeof(T),
      .stride = sizeof(T),
      .align  = Align_of(T),
    };
  }
};

ttld_inline i64 int_value_min(u16 bitwidth) {
  // clang-format off
  switch (bitwidth) {
  case 8:  return INT8_MIN;
  case 16: return INT16_MIN;
  case 32: return INT32_MIN;
  case 64: return INT64_MIN;
  default: Unreachable();
  }
  // clang-format on
}

ttld_inline i64 int_value_max(u16 bitwidth) {
  // clang-format off
  switch (bitwidth) {
  case 8:  return INT8_MAX;
  case 16: return INT16_MAX;
  case 32: return INT32_MAX;
  case 64: return INT64_MAX;
  default: Unreachable();
  }
  // clang-format on
}

ttld_inline u64 uint_value_max(u16 bitwidth) {
  // clang-format off
  switch (bitwidth) {
  case 8:  return UINT8_MAX;
  case 16: return UINT16_MAX;
  case 32: return UINT32_MAX;
  case 64: return UINT64_MAX;
  default: Unreachable();
  }
  // clang-format on
}

struct Type {
  TypeKind kind;

  union {
    IntInfo integer;
    struct {
      TypeIndex base_type;
    } slice;
    struct {
      TypeIndex base_type;
      i64       size;
    } array;
    struct {
      TypeIndex return_type;
      u32       param_count;
    } literal_function;
    struct {
      TypeIndex return_type;
      u32       param_count;
      TypeIndex param_types[0];
    } function;
    struct {
      u32       uid;
      TypeIndex base_type;
    } distinct;
    struct {
      u32       count;
      TypeIndex item_types[0];
    } sequence;
  };

  u32 byte_size() {
    switch (kind) {
    case Type_integer:
    case Type_literal_int:
    case Type_literal_function:
    case Type_nil:
    case Type_never:
    case Type_slice:
    case Type_array:
    case Type_distinct:
    case Type_type:
    case Type_boolean:
      return sizeof(*this);
    case Type_function: {
      return sizeof(*this) + function.param_count * sizeof(TypeIndex);
    } break;
    case Type_sequence:
      return sizeof(*this) + sequence.count * sizeof(TypeIndex);
    default:
      Unreachable();
      return 0;
    }
  }

  bool is_integer_or_literal_int() { return kind == Type_integer || kind == Type_literal_int; }
};

ttld_inline Type *alloc_type_function(Arena *arena, u32 param_count) {
  return cast<Type *>(
    arena->raw_alloc(sizeof(Type) + sizeof(TypeIndex) * param_count, Align_of(Type))
  );
}

ttld_inline Type *alloc_type_sequence(Arena *arena, u32 count) {
  return cast<Type *>(arena->raw_alloc(sizeof(Type) + sizeof(TypeIndex) * count, Align_of(Type)));
}

b32 type_eq(void *context, Type *a, Type *b);
u32 type_hash(void *context, Type *x);

struct TypeInterner {
  Allocator                                      storage;
  HashMap<Type *, TypeIndex, type_eq, type_hash> map;
  Vector<Type *>                                 list;
  u32                                            distinct_uid_gen = 1;

  // Often used and always available types

  struct {
    TypeIndex bool_;

    TypeIndex u8_;
    TypeIndex u16_;
    TypeIndex u32_;
    TypeIndex u64_;

    TypeIndex i8_;
    TypeIndex i16_;
    TypeIndex i32_;
    TypeIndex i64_;

    TypeIndex uint;

    TypeIndex literal_int;
    TypeIndex literal_string;

    TypeIndex nil;
    TypeIndex never;

    TypeIndex type;

    TypeIndex slice_u8;
  } type;

  void init(
    Arena    *work_arena,
    Allocator storage_allocator,
    Allocator map_allocator,
    Allocator vector_allocator
  );
  void deinit();

  u32 _get_new_distinct_uid() {
    u32 uid           = distinct_uid_gen;
    distinct_uid_gen += 1;
    return uid;
  }

  TypeIndex _intern_type(Type *type);

  TypeIndex add(Type *type);

  // Intern the provided type, but make it distinct as well.
  // Two calls to this function with the same pointer will result in two different types returned.
  TypeIndex add_as_distinct(Type *type);

  Type *get(TypeIndex idx) { return list[idx.idx]; }

  TypeSizeInfo size_info(TypeIndex idx);

  bool is_coercible_to(TypeIndex src, TypeIndex dst);
  bool unify(TypeIndex lhs, TypeIndex rhs, TypeIndex *result);

  u32 type_to_string(TypeIndex type, char *buf, u32 buf_size);
};template<typename T> struct Span {
  T start;
  T end;
};

struct Messages;

enum TokenKind : u8 {
  Tok_colon,
  Tok_semicolon,
  Tok_comma,
  Tok_dot,

  Tok_equals,
  Tok_minus,
  Tok_plus,
  Tok_star,
  Tok_slash,
  Tok_percent,
  Tok_plus_equals,
  Tok_exclamation,
  Tok_ampersand,
  Tok_bar,
  Tok_caret,
  Tok_tilde,
  Tok_left_shift,
  Tok_right_shift,
  Tok_cmp_eq,
  Tok_cmp_ne,
  Tok_cmp_gt,
  Tok_cmp_ge,
  Tok_cmp_lt,
  Tok_cmp_le,

  Tok_literal_int,
  Tok_literal_string,

  Tok_brace_open,
  Tok_brace_close,
  Tok_paren_open,
  Tok_paren_close,
  Tok_bracket_open,
  Tok_bracket_close,

  Tok_keyword_if,
  Tok_keyword_else,
  Tok_keyword_for,
  Tok_keyword_do,
  Tok_keyword_break,
  Tok_keyword_continue,
  Tok_keyword_return,
  Tok_keyword_and,
  Tok_keyword_or,
  Tok_keyword_defer,
  Tok_keyword_const,
  Tok_keyword_cast,

  Tok_identifier,

  Tok_builtin_print,

  Tok_line_comment,

  Tok_kind_max,
};

// TODO: use Index type.
struct TokenIndex {
  u32 idx;
};

struct Tokens {
  Vector<TokenKind> kinds;

  // Each span denotes what bytes in the source the token spans.
  Vector<Span<u32>> spans;

  void init(Allocator vector_allocator) {
    kinds.init(vector_allocator);
    spans.init(vector_allocator);
  }

  void deinit() {
    kinds.deinit();
    spans.deinit();
  }

  u32        len() { return kinds.len(); }
  TokenIndex end() { return {cast<u32>(kinds.len())}; }

  TokenIndex alloc() {
    TokenIndex res = end();
    kinds.push_empty();
    spans.push_empty();
    return res;
  }

  TokenKind &kind(TokenIndex idx) { return kinds[idx.idx]; }
  Span<u32> &span(TokenIndex idx) { return spans[idx.idx]; }
};

constexpr char const *token_string[Tok_kind_max] = {
  "colon",        "semicolon",
  "comma",        "dot",
  "equals",       "minus",
  "plus",         "star",
  "slash",        "percent",
  "plus_equals",  "exclamation",
  "ampersand",    "bar",
  "caret",        "tilde",
  "left-shift",   "right-shift",
  "cmp-eq",       "cmp-ne",
  "cmp-gt",       "cmp-ge",
  "cmp-lt",       "cmp-le",
  "literal-int",  "literal-string",
  "brace-open",   "brace-close",
  "paren-open",   "paren-close",
  "bracket-open", "bracket-close",
  "if",           "else",
  "for",          "do",
  "break",        "continue",
  "return",       "and",
  "or",           "defer",
  "const",        "cast",
  "identifier",   "#print",
  "line-comment",
};

ttld_inline char const *token_kind_string(u32 kind) {
  if (kind >= Tok_kind_max) {
    return "<illegal>";
  }

  return token_string[kind];
}
struct TokenizeContext {
  Messages *messages;
};

b32 tokenize(TokenizeContext *context, Str source, Tokens *out);

void write_tokens(Tokens *tokens, Str source, Arena *out);

enum AstKind : u8 {
  Ast_root,
  Ast_block,

  Ast_type_slice,
  Ast_type_array,
  Ast_type_function,

  Ast_builtin,

  Ast_declaration,
  Ast_assign,
  Ast_literal_sequence,
  Ast_literal_int,
  Ast_literal_string,
  Ast_identifier,
  Ast_call,
  Ast_index,
  Ast_unary_op,
  Ast_binary_op,
  Ast_function,
  Ast_if_else,
  Ast_for,
  Ast_defer,
  Ast_const,
  Ast_cast,

  Ast_kind_max,
};

enum BuiltinKind : u8 {
  Builtin_print,
};

enum BinaryOpKind : u8 {
  Mul,
  Div,
  Mod,

  Sub,
  Add,

  Bit_shift_left,
  Bit_shift_right,

  Bit_and,
  Bit_or,
  Bit_xor,

  Cmp_equal,
  Cmp_not_equal,
  Cmp_greater_than,
  Cmp_greater_equal,
  Cmp_less_than,
  Cmp_less_equal,

  Logical_and,
  Logical_or,

  BinaryOpKind_max,
};

enum AssignKind : u8 {
  Assign_normal,

  AssignKind_max,
};

enum UnaryOpKind : u8 {
  Negate,
  Not,

  UnaryOpKind_max,
};

struct AstBuiltin {
  BuiltinKind kind;
  union {
    SegmentList<NodeIndex> args;
  };
};

struct AstTypeFunction {
  NodeIndex              return_type;
  SegmentList<NodeIndex> param_types;
};

struct AstTypeSlice {
  NodeIndex base;
};

struct AstTypeArray {
  NodeIndex size;
  NodeIndex base;
};

struct AstDeclaration {
  TokenIndex name;
  NodeIndex  type;
  NodeIndex  value;
};

struct AstRoot {
  SegmentList<NodeIndex> items;
};

struct AstBlock {
  SegmentList<NodeIndex> items;
};

struct AstLiteralSequence {
  SegmentList<NodeIndex> items;
};

struct AstFunction {
  SegmentList<NodeIndex> param_names;

  // NOTE: `NodeIndex body` could be removed.
  // You can say that the next node is the body and then you no longer have to
  // explicitly reference it.
  NodeIndex body; // always a block
};

struct AstIfElse {
  NodeIndex cond;
  NodeIndex then;
  NodeIndex otherwise;
};

struct AstFor {
  NodeIndex iterable;
  NodeIndex iterator;
  NodeIndex body;
};

struct AstUnaryOp {
  UnaryOpKind kind;
  NodeIndex   value;
};

struct AstBinaryOp {
  BinaryOpKind kind;
  NodeIndex    lhs;
  NodeIndex    rhs;
};

struct AstFieldAccess {
  NodeIndex base;
  NodeIndex field;
};

struct AstCall {
  NodeIndex              callee;
  SegmentList<NodeIndex> args;
};

struct AstIndex {
  NodeIndex indexable;
  NodeIndex index_at;
};

struct AstDefer {
  NodeIndex value;
};

struct AstAtom {
  TokenIndex token_index;
};

struct AstAssign {
  AssignKind kind;
  NodeIndex  lhs;
  NodeIndex  value;
};

struct AstConst {
  NodeIndex expr;
};

struct AstCast {
  NodeIndex type_dst;
  NodeIndex value;
};

union AstNodeData {
  AstRoot            root;
  AstBlock           block;
  AstBuiltin         builtin;
  AstTypeFunction    type_function;
  AstTypeSlice       type_slice;
  AstTypeArray       type_array;
  AstDeclaration     declaration;
  AstAssign          assign;
  AstLiteralSequence literal_sequence;
  AstAtom            literal_int;
  AstAtom            literal_string;
  AstAtom            identifier;
  AstFieldAccess     access;
  AstCall            call;
  AstIndex           index;
  AstUnaryOp         unary_op;
  AstBinaryOp        binary_op;
  AstFunction        function;
  AstIfElse          if_else;
  AstFor             for_;
  AstDefer           defer;
  AstConst           const_;
  AstCast            cast;
};

struct AstNode {
  AstKind          kind;
  Span<TokenIndex> span;
  AstNodeData      data;
};

struct AstNodes {
  Vector<AstKind>          kinds;
  Vector<Span<TokenIndex>> spans;
  Vector<AstNodeData>      datas;

  Allocator segment_allocator;

  void init(Allocator vector_allocator, Allocator segment_allocator) {
    kinds.init(vector_allocator);
    spans.init(vector_allocator);
    datas.init(vector_allocator);

    kinds.push_empty();
    spans.push_empty();
    datas.push_empty();

    this->segment_allocator = segment_allocator;
  }

  void deinit() {
    kinds.deinit();
    spans.deinit();
    datas.deinit();

    memset(this, 0, sizeof(*this));
  }

  NodeIndex first_valid_index() const {
    return {
      .kind = NodeIndex_ast_node,
      .idx  = 1,
    };
  }

  usize len() { return kinds.len(); }

  NodeIndex alloc() {
    NodeIndex res = {
      .kind = NodeIndex_ast_node,
      .idx  = cast<u32>(kinds.len()),
    };

    kinds.push_empty();
    spans.push_empty();
    datas.push_empty();
    return res;
  }

  void set(NodeIndex idx, AstNode node) {
    Debug_assert(idx.kind == NodeIndex_ast_node && idx.is_some());

    kinds[idx.idx] = node.kind;
    spans[idx.idx] = node.span;
    datas[idx.idx] = node.data;
  }

  NodeIndex add(AstNode node) {
    NodeIndex res = {
      .kind = NodeIndex_ast_node,
      .idx  = cast<u32>(kinds.len()),
    };

    kinds.push(node.kind);
    spans.push(node.span);
    datas.push(node.data);

    return res;
  }

  AstKind          kind(NodeIndex idx) { return kinds[idx.idx]; }
  Span<TokenIndex> span(NodeIndex idx) { return spans[idx.idx]; }
  AstNodeData      data(NodeIndex idx) { return datas[idx.idx]; }
};

constexpr char const *ast_string[Ast_kind_max + 1] = {
  "root",        "block",   "type-slice",       "type-array",  "type-function",  "builtin",
  "declaration", "assign",  "literal-sequence", "literal-int", "literal-string", "identifier",
  "call",        "index",   "unary-op",         "binary-op",   "function",       "if-else",
  "while",       "defer",          "const",
  "cast",        "illegal",
};

ttld_inline char const *ast_kind_string(u32 kind) {
  if (kind >= Ast_kind_max) {
    return ast_string[Ast_kind_max];
  }

  return ast_string[kind];
}

constexpr char const *binary_op_string[BinaryOpKind_max + 1] = {
  "* (Mul)",
  "/ (Div)",
  "% (Mod)",
  "- (Sub)",
  "+ (Add)",
  "<< (Bit_shift_left)",
  ">> (Bit_shift_right)",
  "& (Bit_and)",
  "| (Bit_or)",
  "~ (Bit_xor)",
  "== (CmpEq)",
  "!= (CmpNe)",
  "> (CmpGt)",
  ">= (CmpGe)",
  "< (CmpLt)",
  "<= (CmpLe)",
  "and (Logical_and)",
  "or (Logical_or)",
  "illegal",
};

ttld_inline char const *binary_op_kind_string(u32 kind) {
  if (kind >= BinaryOpKind_max) {
    return binary_op_string[BinaryOpKind_max];
  }

  return binary_op_string[kind];
}
struct ParseContext {
  Messages *messages;
};

b32 parse_root(ParseContext *ctx, Tokens *tokens, AstNodes *out);

ttld_inline b32 eq_node_index(void *context, NodeIndex a, NodeIndex b) {
  return a.kind == b.kind && a.idx == b.idx;
}
ttld_inline u32 hash_node_index(void *context, NodeIndex a) {
  return (cast<u32>(a.kind) << 30) | a.idx;
}

struct ValueSlice {
  u64   len;
  void *items;
};

struct Value {
  TypeIndex type;
  void     *data;
};

struct ValueStore {
  ArenaItemPool<Value> pool;
  Allocator            payload_allocator;

  void init(Allocator payload_allocator) {
    pool.init(MiB(64));
    this->payload_allocator = payload_allocator;
  }

  void deinit() {
    pool.deinit();
    memset(this, 0, sizeof(*this));
  }

  ValueIndex alloc_value(Value **out) {
    u32 idx = pool.reserve_index();
    *out    = pool.get(idx);
    return {idx};
  }

  // TODO: maybe it doesn't make sense that this function has a count parameter and that information
  // should just be encoded in the TypeSizeInfo.
  void *alloc_data(TypeSizeInfo info, u32 count = 1) {
    usize byte_size;
    if (count == 1) {
      byte_size = info.size;
    } else {
      byte_size = info.stride * count;
    }

    return payload_allocator.raw_alloc(byte_size, info.align);
  }

  template<typename T> T *alloc_data(u32 count = 1) {
    return cast<T *>(alloc_data(TypeSizeInfo::of_type<T>(), count));
  }

  Value *get(ValueIndex idx) { return pool.get(idx.idx); }

  // Returns the number of bytes written to `buf`.
  u32 value_to_string(TypeInterner *types, ValueIndex idx, char *buf, u32 buf_size);
};

ttld_inline b32 str_key_eq(void *context, StrKey a, StrKey b) { return a.idx == b.idx; }
ttld_inline u32 str_key_hash(void *context, StrKey x) { return x.idx; }

template<typename T> struct Env {
  Env                                         *parent;
  HashMap<StrKey, T, str_key_eq, str_key_hash> map;

  void init(Allocator allocator, Env *parent = nullptr) {
    map.init(allocator);
    this->parent = parent;
  }

  void deinit() {
    parent = nullptr;
    map.deinit();
  }

  void insert(StrKey identifier, T val) { map.insert(identifier, val); }
  bool lookup(StrKey identifier, T *out) {
    T *p = map.get_ptr(identifier);
    if (p) {
      *out = *p;
      return true;
    }

    if (parent) {
      return parent->lookup(identifier, out);
    }

    return false;
  }
};

template<typename T> struct EnvManager {
  ObjectPool<Env<T>> pool;

  // Each Env uses the env_allocator for its own allocations.
  Allocator env_allocator;

  void init(Allocator pool_allocator, Allocator env_allocator) {
    pool.init(pool_allocator);
    this->env_allocator = env_allocator;
  }

  void deinit() {
    pool.deinit();
    memset(this, 0, sizeof(*this));
  }

  Env<T> *alloc(Env<T> *parent) {
    Env<T> *env = pool.alloc();
    env->init(env_allocator, parent);
    return env;
  }

  void dealloc(Env<T> *env) {
    env->deinit();
    pool.dealloc(env);
  }
};

enum MessageLocationKind : u32 {
  MessageLocation_none,
  MessageLocation_token_index,
  MessageLocation_node_index,
};

struct MessageLocation {
  MessageLocationKind kind;
  union {
    TokenIndex token_index;
    NodeIndex  node_index;
  } data;
};

enum MessageSeverity : u8 {
  Error,
  Warning,
  Info,
};

union MessageArg {
  TokenKind token_kind;
  StrKey    strkey;
  TypeIndex type;
  Span<u32> span;
};

struct Message {
  MessageLocation location;
  MessageSeverity severity;
  u32             format_len; // excluding null terminator
  char const     *format;     // null terminated
  MessageArg     *args;
};

struct MessageContext {
  Str             text;
  Tokens         *tokens;
  AstNodes       *nodes;
  TypeInterner   *types;
  StringInterner *strings;
};

struct Messages {
  Arena             arena;
  Vector<Message *> messages;

  void init(Allocator vector_alloc) {
    arena.init(MiB(16));
    messages.init(vector_alloc);
  }

  void deinit() {
    messages.deinit();
    arena.deinit();
  }

  void print_message(MessageContext *context, Message *message);
  void print_messages(MessageContext *context);

  void error(char const *format, ...);
  void error(TokenIndex location, char const *format, ...);
  void error(NodeIndex location, char const *format, ...);

  void _error(MessageLocation location, char const *format, va_list varargs);
};

// ---

struct ParsedSource {
  Str       text;
  Tokens   *tokens;
  AstNodes *nodes;
};

ttld_inline Str get_token_str(Str text, Tokens *tokens, TokenIndex idx) {
  auto span = tokens->span(idx);
  return text.sub(span.start, span.end);
}

struct InterpreterContext {
  TypeInterner   *types;
  StringInterner *strings;
  Messages       *messages;

  Str              text;
  Tokens          *tokens;
  AstNodes        *nodes;
  Slice<TypeIndex> node_types;
};

struct Interpreter {
  EnvManager<ValueIndex> envs;
  ValueStore             values;
  Arena                  work_arena;
  Env<ValueIndex>       *env_root;

  // Externally owned
  TypeInterner    *types;
  StringInterner  *strings;
  Messages        *messages;
  Str              text;
  Tokens          *tokens;
  AstNodes        *nodes;
  Slice<TypeIndex> node_types;

  struct {
    ValueIndex nil;
  } common;

  void init(InterpreterContext *context);
  void deinit();

  // - Creates root env and add roots declarations.
  // - Inserts explicit casts for all type coercions.
  // - Evaluates all the `const` code and inserts computed values into AST.
  bool prepare_code();

  // Replaces type coercions with explicit casts. 
  b32 coercion_resolve_walk(NodeIndex *node);
  void resolve_possible_coercion(TypeIndex type_dst, NodeIndex *node);
  b32 const_walk(Env<ValueIndex> *env, NodeIndex *slot);

  bool run_main(ValueIndex *result);

  OptionalValueIndex _lookup(Env<ValueIndex> *env, Str identifier) {
    ValueIndex idx;
    b32        found = env->lookup(strings->add(identifier), &idx);
    if (!found) {
      return OptionalValueIndex::none();
    }

    return OptionalValueIndex::from_index(idx);
  }

  TypeIndex get_type(NodeIndex node_index);

  b32 call_function(
    Env<ValueIndex> *env, Value *function, Slice<ValueIndex> arguments, ValueIndex *result
  );
  b32 eval_expr(Env<ValueIndex> *env, NodeIndex node_index, ValueIndex *result);
  b32 eval_binary_op(
    BinaryOpKind op, ValueIndex lhs, ValueIndex rhs, NodeIndex expr, ValueIndex *result
  );

  void builtin_print(Str format, Slice<ValueIndex> args);

  ValueIndex lookup_identifier(Env<ValueIndex> *env, NodeIndex identifier);

  b32 eval_place(Env<ValueIndex> *env, NodeIndex node, void **out_ptr, TypeIndex *out_type);
  b32 eval_cast(TypeIndex type_dst, ValueIndex val, ValueIndex *result);

  b32       add_declaration(Env<ValueIndex> *env, NodeIndex declaration);
  bool      coerce_value(TypeIndex type_dst, ValueIndex src, void *out);
  b32       coerce_slot_to(NodeIndex *slot, TypeIndex dst_type);
  TypeIndex slot_type(NodeIndex slot);

  u64 get_uint(ValueIndex idx);

  // Can read any signed integer type and convert it to i64.
  i64 get_as_i64(ValueIndex idx) {
    i64 res;
    coerce_value(types->type.i64_, idx, &res);
    return res;
  }

  // Can read any unsigned integer type and convert it to u64.
  u64 get_as_u64(ValueIndex idx) {
    i64 res;
    coerce_value(types->type.u64_, idx, &res);
    return res;
  }

  void populate_root_env(Env<ValueIndex> *env);

  Str get_token_str(TokenIndex idx) { return ::get_token_str(text, tokens, idx); }

  ValueIndex alloc_value_type(TypeIndex type);
  void insert_cast_to(TypeIndex type_dst, NodeIndex *node);
};

enum SourceUnitStage : u8 {
  Stage_init = 0,
  Stage_tokenize,
  Stage_parse,
  Stage_typecheck,
  Stage_run_const_code,
  Stage_done,
};

struct SourceUnit {
  SourceUnitStage stage;

  Str filename;
  Str text;

  Arena arena;
  Arena work_arena;

  Messages messages;

  StringInterner strings;
  TypeInterner   types;

  Interpreter interpreter;

  Tokens            tokens;
  AstNodes          nodes;
  Vector<TypeIndex> node_types;

  void init(Str filename, Str text);
  void deinit();

  bool tokenize();
  bool parse();
  bool typecheck();
  bool run_const_code();
  bool run_main(ValueIndex *result);

  void print_messages();
};

enum DeclarationKind {
  Declaration_of_type,
  Declaration_of_value,

  Declaration_unresolved,
  Declaration_resolving,
};

struct Declaration {
  DeclarationKind kind;
  union {
    TypeIndex type;
    NodeIndex node_index;
  };
};

struct TypeCheckContext {
  Messages                *messages;
  EnvManager<Declaration> *envs;
  TypeInterner            *types;
  StringInterner          *strings;
  Arena                   *work_arena;
};

b32 typecheck(
  TypeCheckContext *context, ParsedSource *source, Slice<TypeIndex> node_types, NodeIndex idx
);

void debug_print_type(TypeInterner *types, TypeIndex type);
u32  string_literal_byte_size(Str literal);
u32  decode_string_literal(Str literal, char *out);
void ast_pretty_print(
  Str text, Tokens *tokens, TypeInterner *types, ValueStore *values, AstNodes *nodes, NodeIndex idx
);
