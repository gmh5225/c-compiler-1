#ifndef MAIN_H
#define MAIN_H

//
// Tokenizer
//

typedef struct Token Token;

// Token kind
typedef enum {
    TK_IDENT,
    TK_PUNCT,
    TK_KEYWORD,
    TK_NUM,
    TK_EOF,
} TokenKind;

// Token
struct Token {
    TokenKind kind;
    Token *next;

    // Integer literal
    long long val;

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

typedef struct Node Node;
typedef struct Obj Obj;
typedef struct Function Function;
typedef struct Type Type;

// Local variable
struct Obj {
    char *name;
    Type *ty;
    int offset;
    Obj *next;
};

// Function
struct Function {
    Function *next;
    char *name;
    Obj *params;

    Node *body;
    int stack_size;
    Obj *locals;
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

    // Block
    Node *body;

    // if or for
    Node *cond;
    Node *then;
    Node *els;
    Node *init;
    Node *inc;
};

Function *parse(Token *tk);

//
// Types
//

// Type node kind
typedef enum {
    TY_INT,
    TY_PTR,
    TY_FUNC,
    TY_ARRAY,
} TypeKind;

// Type node
struct Type {
    TypeKind kind;

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

void codegen(Function *prog);

#endif
