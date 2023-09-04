#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include "main.h"

static Node *new_node(NodeKind kind) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    return node;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_unary(NodeKind kind, Node *lhs) {
    Node *node = new_node(kind);
    node->lhs = lhs;
    return node;
}

static Node *new_num(long long val) {
    Node *node = new_node(ND_NUM);
    node->val = val;
    return node;
}

static Node *stmt(Token **rest, Token *tk);
static Node *expr_stmt(Token **rest, Token *tk);
static Node *expr(Token **rest, Token *tk);
static Node *equality(Token **rest, Token *tk);
static Node *relational(Token **rest, Token *tk);
static Node *add(Token **rest, Token *tk);
static Node *mul(Token **rest, Token *tk);
static Node *unary(Token **rest, Token *tk);
static Node *primary(Token **rest, Token *tk);

// stmt = expr-stmt
static Node *stmt(Token **rest, Token *tk) {
    return expr_stmt(rest, tk);
}

// expr-stmt = expr ";"
static Node *expr_stmt(Token **rest, Token *tk) {
    Node *node = expr(&tk, tk);
    *rest = skip(tk, ";");
    return new_unary(ND_EXPR_STMT, node);
}

// expr = equality
static Node *expr(Token **rest, Token *tk) {
    return equality(rest, tk);
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality(Token **rest, Token *tk) {
    Node *lhs = relational(&tk, tk);

    while (true) {
        if (equal(tk, "==")) {
            Node *rhs = relational(&tk, tk->next);
            lhs = new_binary(ND_EQ, lhs, rhs);
            continue;
        }

        if (equal(tk, "!=")) {
            Node *rhs = relational(&tk, tk->next);
            lhs = new_binary(ND_NE, lhs, rhs);
            continue;
        }

        *rest = tk;
        return lhs;
    }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(Token **rest, Token *tk) {
    Node *lhs = add(&tk, tk);

    while (true) {
        if (equal(tk, "<")) {
            Node *rhs = add(&tk, tk->next);
            lhs = new_binary(ND_LT, lhs, rhs);
            continue;
        }

        if (equal(tk, "<=")) {
            Node *rhs = add(&tk, tk->next);
            lhs = new_binary(ND_LE, lhs, rhs);
            continue;
        }

        if (equal(tk, ">")) {
            Node *rhs = add(&tk, tk->next);
            lhs = new_binary(ND_GT, lhs, rhs);
            continue;
        }

        if (equal(tk, ">=")) {
            Node *rhs = add(&tk, tk->next);
            lhs = new_binary(ND_GE, lhs, rhs);
            continue;
        }

        *rest = tk;
        return lhs;
    }
}

// add = mul ("+" mul | "-" mul)*
static Node *add(Token **rest, Token *tk) {
    Node *lhs = mul(&tk, tk);

    while (true) {
        if (equal(tk, "+")) {
            Node *rhs = mul(&tk, tk->next);
            lhs = new_binary(ND_ADD, lhs, rhs);
            continue;
        }

        if (equal(tk, "-")) {
            Node *rhs = mul(&tk, tk->next);
            lhs = new_binary(ND_SUB, lhs, rhs);
            continue;
        }

        *rest = tk;
        return lhs;
    }
}

// mul = unary ("*" unary | "/" unary)*
static Node *mul(Token **rest, Token *tk) {
    Node *lhs = unary(&tk, tk);

    while (true) {
        if (equal(tk, "*")) {
            Node *rhs = unary(&tk, tk->next);
            lhs = new_binary(ND_MUL, lhs, rhs);
            continue;
        }

        if (equal(tk, "/")) {
            Node *rhs = unary(&tk, tk->next);
            lhs = new_binary(ND_DIV, lhs, rhs);
            continue;
        }

        *rest = tk;
        return lhs;
    }
}

// unary = ("+" | "-")? primary
static Node *unary(Token **rest, Token *tk) {
    if (equal(tk, "+")) {
        return unary(rest, tk->next);
    }

    if (equal(tk, "-")) {
        Node *node = unary(rest, tk->next);
        return new_unary(ND_NEG, node);
    }

    return primary(rest, tk);
}

// primary = "(" expr ")" | num
static Node *primary(Token **rest, Token *tk) {
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

Node *parse(Token *tk) {
    return stmt(&tk, tk);
}
