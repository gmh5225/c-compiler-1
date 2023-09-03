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

Token skip(Token *tk, char *op) {
    if (!equal(tk, op)) {
        error_tk(tk, "Expected '%s'", op);
    }

    return *tk->next;
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
Node *primary(Token **rest, Token *tk);

Node *expr(Token **rest, Token *tk) {
    Node *lhs = primary(&tk, tk);

    while (true) {
        if (equal(tk, "+")) {
            Node *rhs = primary(&tk, tk->next);
            lhs = new_binary(ND_ADD, lhs, rhs);
            continue;
        }

        if (equal(tk, "-")) {
            Node *rhs = primary(&tk, tk->next);
            lhs = new_binary(ND_SUB, lhs, rhs);
            continue;
        }

        *rest = tk;
        return lhs;
    }
}

Node *primary(Token **rest, Token *tk) {
    if (tk->kind == TK_NUM) {
        Node *node = new_num(tk->val);
        *rest = tk->next;
        return node;
    }

    error_tk(tk, "Expected a number");
    return NULL;
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

    printf("\t.global main\n");
    printf("main:\n");

    printf("\tmov x0, #%lld\n", get_number(tk));
    tk = tk->next;

    while (tk->kind != TK_EOF) {
        if (equal(tk, "+")) {
            printf("\tadd x0, x0, #%lld\n", get_number(tk->next));
            tk = tk->next->next;
            continue;
        }

        if (equal(tk, "-")) {
            printf("\tsub x0, x0, #%lld\n", get_number(tk->next));
            tk = tk->next->next;
            continue;
        }

        error_tk(tk, "Unexpected token");
    }

    printf("\tret\n");
    return EXIT_SUCCESS;
}
