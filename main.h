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
Token *tokenize(char *p);

//
// Parser
//

typedef struct Node Node;
typedef struct Obj Obj;
typedef struct Function Function;

struct Obj {
    char *name;
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
    ND_RETURN,
    ND_BLOCK,
    ND_EXPR_STMT,
    ND_VAR,
    ND_NUM,
} NodeKind;

struct Node {
    NodeKind kind;
    Obj *var;
    long long val;
    Node *body;
    Node *lhs;
    Node *rhs;
    Node *next;
};

Function *parse(Token *tk);

//
// Code generator
//

void codegen(Function *prog);

#endif
