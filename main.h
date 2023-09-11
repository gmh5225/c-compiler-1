#ifndef MAIN_H
#define MAIN_H

//
// Tokenizer
//

typedef struct Token Token;

typedef enum {
    TK_IDENT,
    TK_PUNCT,
    TK_KEYWORD,
    TK_NUM,
    TK_EOF,
} TokenKind;

struct Token {
    TokenKind kind;
    long long val;
    char *loc;
    size_t len;
    Token *next;
};

int error(char *fmt, ...);
int error_at(char *loc, char *fmt, ...);
int error_tk(Token *tk, char *fmt, ...);
bool equal(Token *tk, char *op);
Token *skip(Token *tk, char *op);
bool consume(Token **rest, Token *tk, char *str);
Token *tokenize(char *p);

//
// Parser
//

typedef struct Node Node;
typedef struct Obj Obj;
typedef struct Function Function;
typedef struct Type Type;

struct Obj {
    char *name;
    Type *ty;
    int offset;
    Obj *next;
};

struct Function {
    Node *body;
    int stack_size;
    Obj *locals;
};

typedef enum {
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_NEG,
    ND_EQ,
    ND_NE,
    ND_LT,
    ND_LE,
    ND_GT,
    ND_GE,
    ND_ASSIGN,
    ND_ADDR,
    ND_DEREF,
    ND_RETURN,
    ND_IF,
    ND_FOR,
    ND_BLOCK,
    ND_FUNC_CALL,
    ND_EXPR_STMT,
    ND_VAR,
    ND_NUM,
} NodeKind;

struct Node {
    NodeKind kind;
    Token *tk;

    Type *ty;
    Obj *var;
    long long val;
    char *funcname;
    Node *lhs;
    Node *rhs;

    Node *cond;
    Node *then;
    Node *els;
    Node *init;
    Node *inc;
    Node *body;
    Node *next;
};

Function *parse(Token *tk);

//
// Types
//

typedef enum {
    TY_INT,
    TY_PTR,
} TypeKind;

struct Type {
    TypeKind kind;
    Type *base;
    Token *name;
};

extern Type *ty_int;

bool is_integer(Type *ty);
Type *pointer_to(Type *base);
void add_type(Node *node);

//
// Code generator
//

void codegen(Function *prog);

#endif
