#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"

static char *current_input;

int error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(EXIT_FAILURE);
    return EXIT_FAILURE;
}

static int verror_at(char *loc, char *fmt, va_list ap) {
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
    bool len = tk->len == strlen(op);
    bool loc = strncmp(tk->loc, op, tk->len) == 0;
    return len && loc;
}

Token *skip(Token *tk, char *op) {
    if (!equal(tk, op)) {
        error_tk(tk, "Expected '%s'", op);
    }

    return tk->next;
}

static Token *new_token(TokenKind kind, char *start, char *end) {
    Token *tk = calloc(1, sizeof(Token));
    tk->kind = kind;
    tk->loc = start;
    tk->len = end - start;
    return tk;
}

static bool starts_with(char *p, char *q) {
    return strncmp(p, q, strlen(q)) == 0;
}

static bool is_ident1(char c) {
    return isalpha(c) || c == '_';
}

static bool is_ident2(char c) {
    return isalnum(c) || c == '_';
}

static int read_punct(char *p) {
    if (starts_with(p, "==")) return 2;
    if (starts_with(p, "!=")) return 2;
    if (starts_with(p, "<=")) return 2;
    if (starts_with(p, ">=")) return 2;
    if (starts_with(p, "+")) return 1;
    if (starts_with(p, "-")) return 1;
    if (starts_with(p, "*")) return 1;
    if (starts_with(p, "/")) return 1;
    if (starts_with(p, "(")) return 1;
    if (starts_with(p, ")")) return 1;
    if (starts_with(p, "<")) return 1;
    if (starts_with(p, ">")) return 1;
    if (starts_with(p, ";")) return 1;
    if (starts_with(p, "=")) return 1;
    if (starts_with(p, "{")) return 1;
    if (starts_with(p, "}")) return 1;
    return 0;
}

static bool is_keyword(Token *tk) {
    static char *kw[] = {"return", "if", "else"};

    for (int i = 0, n = sizeof(kw) / sizeof(*kw); i < n; ++i) {
        if (equal(tk, kw[i])) {
            return true;
        }
    }

    return false;
}

static void convert_keywords(Token *tk) {
    for (Token *t = tk; t->kind != TK_EOF; t = t->next) {
        if (is_keyword(t)) {
            t->kind = TK_KEYWORD;
        }
    }

    return;
}

Token *tokenize(char *p) {
    current_input = p;
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

        if (is_ident1(*p)) {
            char *start = p++;
            while (is_ident2(*p)) {
                p += 1;
            }
            cur->next = new_token(TK_IDENT, start, p);
            cur = cur->next;
            continue;
        }

        int punct_len = read_punct(p);
        if (punct_len > 0) {
            cur->next = new_token(TK_PUNCT, p, p + punct_len);
            cur = cur->next;
            p += punct_len;
            continue;
        }

        error_at(p, "Invalid token");
    }

    cur->next = new_token(TK_EOF, p, p);
    convert_keywords(head.next);
    return head.next;
}
