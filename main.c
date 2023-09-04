#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// Tokenizer
//

typedef struct Token Token;

typedef enum {
    TK_PUNCT,
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

char *current_input;

int error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(EXIT_FAILURE);
    return EXIT_FAILURE;
}

int verror_at(char *loc, char *fmt, va_list ap) {
    int pos = loc - current_input;
    fprintf(stderr, "%s\n", current_input);
    fprintf(stderr, "%*s", pos, "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    return EXIT_FAILURE;
}

int error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(loc, fmt, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
    return EXIT_FAILURE;
}

int error_tk(Token *tk, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verror_at(tk->loc, fmt, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
    return EXIT_FAILURE;
}

bool equal(Token *tk, char *op) {
    bool kind = tk->kind == TK_PUNCT;
    bool len = tk->len == strlen(op);
    bool loc = strncmp(tk->loc, op, tk->len) == 0;
    return kind && len && loc;
}

Token *skip(Token *tk, char *op) {
    if (!equal(tk, op)) {
        error_tk(tk, "Expected '%s'", op);
    }

    return tk->next;
}

long long get_number(Token *tk) {
    if (tk->kind != TK_NUM) {
        error_tk(tk, "Expected a number");
    }

    return tk->val;
}

Token *new_token(TokenKind kind, char *start, char *end) {
    Token *tk = calloc(1, sizeof(Token));
    tk->kind = kind;
    tk->loc = start;
    tk->len = end - start;
    return tk;
}

Token *tokenize(void) {
    char *p = current_input;
    Token head = {0};
    Token *cur = &head;

    while (*p != '\0') {
        if (isspace(*p)) {
            p += 1;
            continue;
        }

        if (isdigit(*p)) {
            cur->next = new_token(TK_NUM, p, p);
            cur = cur->next;
            char *q = p;
            cur->val = strtoll(p, &p, 10);
            cur->len = p - q;
            continue;
        }

        if (ispunct(*p)) {
            cur->next = new_token(TK_PUNCT, p, p + 1);
            cur = cur->next;
            p += 1;
            continue;
        }

        error_at(p, "Invalid token");
    }

    cur->next = new_token(TK_EOF, p, p);
    return head.next;
}

//
// Parser
//

typedef struct Node Node;

typedef enum {
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_NUM,
} NodeKind;

struct Node {
    NodeKind kind;
    long long val;
    Node *lhs;
    Node *rhs;
};

Node *new_node(NodeKind kind) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    return node;
}

Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_num(long long val) {
    Node *node = new_node(ND_NUM);
    node->val = val;
    return node;
}

Node *expr(Token **rest, Token *tk);
Node *term(Token **rest, Token *tk);
Node *primary(Token **rest, Token *tk);

Node *expr(Token **rest, Token *tk) {
    Node *lhs = term(&tk, tk);

    while (true) {
        if (equal(tk, "+")) {
            Node *rhs = term(&tk, tk->next);
            lhs = new_binary(ND_ADD, lhs, rhs);
            continue;
        }

        if (equal(tk, "-")) {
            Node *rhs = term(&tk, tk->next);
            lhs = new_binary(ND_SUB, lhs, rhs);
            continue;
        }

        *rest = tk;
        return lhs;
    }
}

Node *term(Token **rest, Token *tk) {
    Node *lhs = primary(&tk, tk);

    while (true) {
        if (equal(tk, "*")) {
            Node *rhs = primary(&tk, tk->next);
            lhs = new_binary(ND_MUL, lhs, rhs);
            continue;
        }

        if (equal(tk, "/")) {
            Node *rhs = primary(&tk, tk->next);
            lhs = new_binary(ND_DIV, lhs, rhs);
            continue;
        }

        *rest = tk;
        return lhs;
    }
}

Node *primary(Token **rest, Token *tk) {
    if (equal(tk, "(")) {
        Node *node = expr(&tk, tk->next);
        *rest = skip(tk, ")");
        return node;
    }

    if (tk->kind == TK_NUM) {
        Node *node = new_num(tk->val);
        *rest = tk->next;
        return node;
    }

    error_tk(tk, "Expected a number");
    return NULL;
}

//
// Code generator
//

int depth = 0;

void push(char *reg) {
    if ((depth++ & 1) == 0) {
        printf("\tsub sp, sp, #16\n");
        printf("\tstr %s, [sp, #8]\n", reg);
    } else {
        printf("\tstr %s, [sp]\n", reg);
    }

    return;
}

void pop(char *reg) {
    if ((--depth & 1) == 0) {
        printf("\tldr %s, [sp, #8]\n", reg);
        printf("\tadd sp, sp, #16\n");
    } else {
        printf("\tldr %s, [sp]\n", reg);
    }

    return;
}

void gen_expr(Node *node) {
    if (node->kind == ND_NUM) {
        printf("\tmov x0, #%lld\n", node->val);
        return;
    }

    gen_expr(node->lhs);
    push("x0");
    gen_expr(node->rhs);
    pop("x1");

    switch (node->kind) {
    case ND_ADD:
        printf("\tadd x0, x1, x0\n");
        break;
    case ND_SUB:
        printf("\tsub x0, x1, x0\n");
        break;
    case ND_MUL:
        printf("\tmul x0, x1, x0\n");
        break;
    case ND_DIV:
        printf("\tsdiv x0, x1, x0\n");
        break;
    default:
        error("Invalid expression");
    }

    return;
}

//
// Main
//

int main(int argc, char **argv) {
    if (argc != 2) {
        error("Invalid number of arguments");
    }

    current_input = argv[1];
    Token *tk = tokenize();
    Node *node = expr(&tk, tk);

    if (tk->kind != TK_EOF) {
        error_tk(tk, "Extra token");
    }

    printf("\t.global main\n");
    printf("main:\n");
    gen_expr(node);
    printf("\tret\n");

    assert(depth == 0);
    return EXIT_SUCCESS;
}
