#ifndef MAIN_H
#define MAIN_H

#include <stdbool.h>

typedef struct Token Token;
typedef struct Node Node;
typedef struct Obj Obj;
typedef struct Type Type;

//
// String
//

char *format(char *fmt, ...);

//
// Tokenizer
//

// Token kind
typedef enum {
    TK_IDENT,
    TK_PUNCT,
    TK_KEYWORD,
    TK_STR,
    TK_NUM,
    TK_EOF,
} TokenKind;

// Token
struct Token {
    TokenKind kind;
    Token *next;

    // Integer literal
    long long val;

    // String literal
    Type *ty;
    char *str;

    // Identifier
    char *loc;
    size_t len;
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

// Variable or function
struct Obj {
    char *name;
    Type *ty;
    Obj *next;

    // Local variable
    bool is_local;
    int offset;

    // Global variable
    char *init_data;

    // Function
    bool is_function;
    Obj *params;
    Node *body;
    Obj *locals;
    int stack_size;
};

// AST node kind
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
    ND_STMT_EXPR,
    ND_VAR,
    ND_NUM,
} NodeKind;

// AST node
struct Node {
    NodeKind kind;
    Token *tk;

// Expression
    Type *ty;
    Node *lhs;
    Node *rhs;

    // Variable
    Obj *var;

    // Integer literal
    long long val;

    // Function call
    char *funcname;
    Node *args;

// Statement
    Node *next;

    // Block or statement expression
    Node *body;

    // if or for
    Node *cond;
    Node *then;
    Node *els;
    Node *init;
    Node *inc;
};

Obj *parse(Token *tk);

//
// Types
//

// Type node kind
typedef enum {
    TY_CHAR,
    TY_INT,
    TY_PTR,
    TY_FUNC,
    TY_ARRAY,
} TypeKind;

// Type node
struct Type {
    TypeKind kind;
    int size;

    // Declaration
    Token *name;

    // Pointer
    Type *base;

    // Array
    int array_len;

    // Function type
    Type *return_ty;
    Type *params;
    Type *next;
};

extern Type *ty_char;
extern Type *ty_int;

bool is_integer(Type *ty);
Type *copy_type(Type *ty);
Type *pointer_to(Type *base);
Type *func_type(Type *return_ty);
Type *array_of(Type *base, int len);
void add_type(Node *node);

//
// Code generator
//

void codegen(Obj *prog);

#endif
