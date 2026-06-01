#pragma once

#include "toteload.hh"
#include "vector.hh"
#include "arena_item_pool.hh"
#include "hashmap.hh"
#include "segment_list.hh"
#include "index.hh"
#include "string_interner.hh"

struct Tag_AstIndex {};
struct Tag_ValueIndex {};
struct Tag_TypeIndex {};
struct Tag_TokenIndex {};

using NodeIndexT = u32;
using AstIndex   = Index<NodeIndexT, Tag_AstIndex>;
using ValueIndex = Index<NodeIndexT, Tag_ValueIndex>;
using TypeIndex  = Index<u32, Tag_TypeIndex>;
using TokenIndex = Index<u32, Tag_TokenIndex>;

enum NodeIndexKind : u8 {
  NodeIndex_none,
  NodeIndex_ast,
  NodeIndex_value,
};

struct NodeIndex {
  NodeIndexKind kind;

  // An index value of 0 means not valid.
  union {
    AstIndex   ast;
    ValueIndex value;
  } idx;

  bool is_some() const { return kind != NodeIndex_none; }
  bool is_none() const { return kind == NodeIndex_none; }

  ValueIndex as_value_idx() const {
    Assert(kind == NodeIndex_value);
    return idx.value;
  }

  AstIndex as_ast_idx() const {
    Assert(kind == NodeIndex_ast);
    return idx.ast;
  }

  static NodeIndex none() {
    return {
      .kind = NodeIndex_none,
      .idx  = {},
    };
  }

  static NodeIndex from_value(ValueIndex value) {
    return {
      .kind = NodeIndex_value,
      .idx  = {.value = value},
    };
  }

  static NodeIndex from_ast_index(AstIndex ast) {
    return {
      .kind = NodeIndex_ast,
      .idx  = {.ast = ast},
    };
  }
};

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

  Type *get(TypeIndex idx) { return list[idx.idx]; }

  TypeSizeInfo size_info(TypeIndex idx);

  bool is_coercible_to(TypeIndex src, TypeIndex dst);
  bool unify(TypeIndex lhs, TypeIndex rhs, TypeIndex *result);

  u32 type_to_string(TypeIndex type, char *buf, u32 buf_size);

  b32 is_valid_cast(TypeIndex from, TypeIndex to);
};

template<typename T> struct Span {
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
  Ast_param,
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

struct AstParam {
  TokenIndex name;
  NodeIndex  type;
};

struct AstFunction {
  SegmentList<NodeIndex> params;
  NodeIndex              return_type;
  NodeIndex              body;
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

struct AstIndexData {
  NodeIndex indexable;
  NodeIndex index_at;
};

struct AstDefer {
  NodeIndex value;
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
  TokenIndex         literal_int;
  TokenIndex         literal_string;
  TokenIndex         identifier;
  AstFieldAccess     access;
  AstCall            call;
  AstIndexData       index;
  AstUnaryOp         unary_op;
  AstBinaryOp        binary_op;
  AstFunction        function;
  AstParam           param;
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
  SegmentList<AstKind>          kinds;
  SegmentList<Span<TokenIndex>> spans;
  SegmentList<AstNodeData>      datas;
  SegmentList<TypeIndex>        types;

  Allocator segment_allocator;

  void init(Allocator vector_allocator, Allocator segment_allocator) {
    this->segment_allocator = segment_allocator;

    kinds.init();
    spans.init();
    datas.init();
    types.init();

    kinds.push(segment_allocator);
    spans.push(segment_allocator);
    datas.push(segment_allocator);
    types.push(segment_allocator);
  }

  void deinit() {
    kinds.deinit(segment_allocator);
    spans.deinit(segment_allocator);
    datas.deinit(segment_allocator);
    types.deinit(segment_allocator);

    memset(this, 0, sizeof(*this));
  }

  AstIndex first_valid_index() const { return {.idx = 1}; }

  usize len() { return kinds.len(); }

  AstIndex alloc() {
    AstIndex res = {.idx = cast<u32>(kinds.len())};

    kinds.push(segment_allocator);
    spans.push(segment_allocator);
    datas.push(segment_allocator);
    types.append(segment_allocator, {0});

    return res;
  }

  void set(AstIndex idx, AstNode node) {
    kinds[idx.idx] = node.kind;
    spans[idx.idx] = node.span;
    datas[idx.idx] = node.data;
  }

  AstKind          &kind(AstIndex idx) { return kinds[idx.idx]; }
  Span<TokenIndex> &span(AstIndex idx) { return spans[idx.idx]; }
  AstNodeData      &data(AstIndex idx) { return datas[idx.idx]; }
  TypeIndex        &type(AstIndex idx) { return types[idx.idx]; }
};

constexpr char const *ast_string[Ast_kind_max + 1] = {
  "root",        "block",          "type-slice",
  "type-array",  "type-function",  "builtin",
  "declaration", "assign",         "literal-sequence",
  "literal-int", "literal-string", "identifier",
  "call",        "index",          "unary-op",
  "binary-op",   "function",       "param",
  "if-else",     "while",          "defer",
  "const",       "cast",           "illegal",
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

// How are different types stored in memory?
//
// - Integer types allocate the number of bytes appropriate for their size: 8-bit integers use 1
// byte, etc...
// - A slice allocates a `ValueSlice`: a 64-bit length and a pointer to its items.
// - An array allocates however many bytes are needed for its size N and base type T: stride(T) * N.
// - A bool allocates 1 byte
// - A literal int allocates 8 bytes and internally is a 64-bit integer.
// - nil allocates 0 bytes.
// - never allocates 0 bytes.
// - type allocates sizeof(TypeIndex) bytes.

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

  b32 has(StrKey identifier) {
    if (map.has(identifier)) {
      return true;
    }

    if (parent) {
      return parent->has(identifier);
    }

    return false;
  }

  bool lookup(StrKey identifier, T *out) {
    T   *p;
    auto found = lookup_ptr(identifier, &p);
    if (!found) {
      return false;
    }
    *out = *p;
    return found;
  }

  bool lookup_ptr(StrKey identifier, T **out) {
    T *p = map.get_ptr(identifier);
    if (p) {
      *out = p;
      return true;
    }

    if (parent) {
      return parent->lookup_ptr(identifier, out);
    }

    return false;
  }

  bool find_and_replace(StrKey identifier, T val) {
    T   *p;
    auto found = lookup_ptr(identifier, &p);
    if (!found) {
      return false;
    }
    *p = val;
    return found;
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

enum ResolveStatus : u8 {
  ResolveStatus_unresolved,
  ResolveStatus_resolving_type,
  ResolveStatus_resolving_value,
  ResolveStatus_resolved,
};

struct Declaration {
  ResolveStatus resolve_status;
  u8            is_const;
  AstIndex      ast_index;

  // If `resolve_status == ResolveStatus_type_unresolved`
  // - `value` holds no type.
  // - `value` holds no data.
  //
  // If `resolve_status == ResolveStatus_type_resolving`
  // - `value` holds a type if the declaration has a declared type.
  // - `value` holds no data.
  //
  // If `resolve_status == ResolveStatus_type_resolved`
  // - `value` holds a type.
  // - `value` holds data.

  ValueIndex value;
};

struct TypeHint {
  TypeIndex type;
  NodeIndex location;

  bool is_some() const { return type.is_some(); }
  bool is_none() const { return type.is_none(); }

  static TypeHint none() {
    return {
      .type     = TypeIndex::none(),
      .location = NodeIndex::none(),
    };
  }
};

struct Builder {
  // Non-owning
  EnvManager<Declaration> *envs;

  TypeInterner   *types;
  StringInterner *strings;
  Messages       *messages;
  ValueStore     *values;
  Arena          *arena_tmp;

  Str       text;
  Tokens   *tokens;
  AstNodes *nodes;

  // Owning
  Env<Declaration> *env_builtin;
  Env<Declaration> *env_root;

  struct {
    TypeHint hint_nil;

    struct {
      ValueIndex nil;
    } val;
  } common;

  void init();
  void deinit();

  b32 typecheck_and_eval_const_code();

  // - `check_expression` adds type annotations to the `AstNodes`.
  // - Make sure there are no type violations.
  // - Add explicit casts in place of coercions.
  // - Evaluate const code and insert the value into the AST.
  b32 check_expression(
    Env<Declaration> *env, NodeIndex *node_index, TypeHint hint = TypeHint::none()
  );

  b32 find_declaration(Env<Declaration> *env, TokenIndex identifier, Declaration **decl);
  b32 resolve_declaration_type(Env<Declaration> *env, Declaration *decl, TypeIndex *type);
  b32 resolve_declaration(Env<Declaration> *env, TokenIndex identifier);

  // Only call `eval_expression` on expressions that have been checked.
  b32 eval_expression(Env<Declaration> *env, NodeIndex node_index, ValueIndex *result);
  b32 eval_cast(TypeIndex type_dst, ValueIndex val, ValueIndex *result);
  b32 eval_call(
    Env<Declaration> *env, ValueIndex function, Slice<ValueIndex> args, ValueIndex *result
  );
  b32 eval_binary_op(BinaryOpKind op, ValueIndex lhs, ValueIndex rhs, ValueIndex *result);

  b32 check_and_eval_expression(
    Env<Declaration> *env, NodeIndex *node_index, TypeHint hint, ValueIndex *result
  );

  // 1. Checks that the expression is valid.
  // 2. Evaluates the expression.
  // 3. Checks that the result of the expression is a type.
  b32 check_and_eval_type_expression(
    Env<Declaration> *env, NodeIndex *node_index, ValueIndex *result
  );

  // All functions starting with "ensure" test for a certain condition.
  // If the condition does not hold it will produce an error message describing the problem.
  // They do not evaluate any expressions.
  b32 ensure_is_expected_type(NodeIndex location, TypeIndex expected, TypeIndex actual);
  b32 ensure_is_valid_cast(NodeIndex at, TypeIndex type_dst, TypeIndex type_expr);
  b32 ensure_is_assignable(NodeIndex node_index);
  b32 ensure_is_type_comparable(AstIndex location, TypeIndex type);

  b32 check_and_resolve_coercion(TypeHint expected, NodeIndex *value);
  b32 check_and_resolve_unification(
    NodeIndex *node_lhs, NodeIndex *node_rhs, TypeIndex *type_unified
  );

  void env_populate_with_builtins(Env<Declaration> *env);
  void env_insert_value(Env<Declaration> *env, Str s, ValueIndex value);

  inline void copy_value_data(ValueIndex value_idx, void *dst) {
    Value *v = values->get(value_idx);
    memcpy(dst, v->data, types->size_info(v->type).size);
  }

  void insert_cast(NodeIndex *value, TypeIndex type_dst);

  StrKey    intern_identifier(TokenIndex identifier);
  TypeIndex get_type(NodeIndex node_index);

  // Reads a signed integer value and extends it to i64
  i64 read_value_i64(ValueIndex idx) {
    i64 res;

    Value *v = values->get(idx);
    Type  *t = types->get(v->type);

    // clang-format off
    switch (t->integer.bitwidth) {
    case 8:  { i8  x; memcpy(&x, v->data, 1); res = x; } break;
    case 16: { i16 x; memcpy(&x, v->data, 2); res = x; } break;
    case 32: { i32 x; memcpy(&x, v->data, 4); res = x; } break;
    case 64: { i64 x; memcpy(&x, v->data, 8); res = x; } break;
    default: Unreachable();
    }
    // clang-format on

    return res;
  }

  // Reads an unsigned integer value and extends it to u64
  u64 read_value_u64(ValueIndex idx) {

    Value *v = values->get(idx);
    Type  *t = types->get(v->type);

    u64 res = 0;
    memcpy(&res, v->data, t->integer.bitwidth / 8);

    return res;
  }

  ValueIndex alloc_type(TypeIndex type_idx);
  ValueIndex alloc_bool(u8 val);
};

struct InterpreterContext {
  TypeInterner   *types;
  StringInterner *strings;
  Messages       *messages;

  Str         text;
  Tokens     *tokens;
  AstNodes   *nodes;
  ValueStore *values;
};

struct Interpreter {
  EnvManager<ValueIndex> envs;
  Arena                  work_arena;
  Env<ValueIndex>       *env_root;

  // Externally owned
  TypeInterner   *types;
  StringInterner *strings;
  Messages       *messages;
  Str             text;
  Tokens         *tokens;
  AstNodes       *nodes;
  ValueStore     *values;

  struct {
    ValueIndex nil;
  } common;

  void init(InterpreterContext *context);
  void deinit();

  // - Creates root env and add roots declarations.
  // - Evaluates all the `const` code and inserts computed values into AST.
  bool prepare_code();

  b32 const_walk(Env<ValueIndex> *env, NodeIndex *slot);

  bool run_main(ValueIndex *result);

  // OptionalValueIndex _lookup(Env<ValueIndex> *env, Str identifier) {
  //   ValueIndex idx;
  //   b32        found = env->lookup(strings->add(identifier), &idx);
  //   if (!found) {
  //     return OptionalValueIndex::none();
  //   }

  //  return OptionalValueIndex::from_index(idx);
  //}

  TypeIndex get_type(NodeIndex node_index);

  void copy_value(TypeIndex type, ValueIndex src, void *dst);

  b32 eval_expr(Env<ValueIndex> *env, NodeIndex node_index, ValueIndex *result);
  b32 eval_declaration(Env<ValueIndex> *env, NodeIndex declaration, ValueIndex *result);
  b32 eval_call(
    Env<ValueIndex> *env, Value *function, Slice<ValueIndex> arguments, ValueIndex *result
  );
  b32 eval_binary_op(
    BinaryOpKind op, ValueIndex lhs, ValueIndex rhs, NodeIndex expr, ValueIndex *result
  );
  b32 eval_cast(TypeIndex type_dst, ValueIndex val, ValueIndex *result);

  void builtin_print(Str format, Slice<ValueIndex> args);

  ValueIndex lookup_identifier(Env<ValueIndex> *env, NodeIndex identifier);

  b32 eval_place(Env<ValueIndex> *env, NodeIndex node, void **out_ptr, TypeIndex *out_type);

  // Can read any signed integer type and convert it to i64.
  i64 get_as_i64(ValueIndex idx) {
    i64 res;
    Todo();
    return res;
  }

  // Can read any unsigned integer type and convert it to u64.
  u64 get_as_u64(ValueIndex idx) {
    i64 res;
    Todo();
    return res;
  }

  void populate_root_env(Env<ValueIndex> *env);

  Str get_token_str(TokenIndex idx) { return ::get_token_str(text, tokens, idx); }

  ValueIndex alloc_value_type(TypeIndex type);
  void       insert_cast_to(TypeIndex type_dst, NodeIndex *node);
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
  ValueStore     values;

  Interpreter interpreter;

  Tokens   tokens;
  AstNodes nodes;

  void init();
  void deinit();

  bool tokenize(Str filename, Str text);
  bool parse();
  bool typecheck_and_run_const_code();
  bool run_main(ValueIndex *result);

  void print_messages();
};

struct TypeCheckContext {
  Messages                *messages;
  EnvManager<Declaration> *envs;
  TypeInterner            *types;
  StringInterner          *strings;
  ValueStore              *values;
  Arena                   *work_arena;
};

b32 typecheck(TypeCheckContext *context, ParsedSource *source, NodeIndex idx);

u32 string_literal_byte_size(Str literal);
u32 decode_string_literal(Str literal, char *out);

// ---

void debug_print_type(TypeInterner *types, TypeIndex type);

enum PrettyPrintMode : u32 {
  Print_basic,
  Print_with_types,
};

struct AstPrettyPrintContext {
  Str           text;
  Tokens       *tokens;
  TypeInterner *types;
  AstNodes     *nodes;
  ValueStore   *values;
};

void pretty_print(AstPrettyPrintContext *context, PrettyPrintMode mode, NodeIndex idx);

void table_print_ast(AstPrettyPrintContext *context);
