#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "main.h"

static int depth = 0;

static void push(char *reg) {
    if ((depth++ & 1) == 0) {
        printf("\tsub sp, sp, #16\n");
        printf("\tstr %s, [sp, #8]\n", reg);
    } else {
        printf("\tstr %s, [sp]\n", reg);
    }

    return;
}

static void pop(char *reg) {
    if ((--depth & 1) == 0) {
        printf("\tldr %s, [sp, #8]\n", reg);
        printf("\tadd sp, sp, #16\n");
    } else {
        printf("\tldr %s, [sp]\n", reg);
    }

    return;
}

static void gen_addr(Node *node) {
    if (node == NULL) {
        error("Invalid lvalue");
    }

    if (node->kind == ND_VAR) {
        int offset = (node->name - 'a' + 1) * 8;
        printf("\tsub x0, x29, #%d\n", offset);
        return;
    }

    error("Not an lvalue");
}

static void gen_expr(Node *node) {
    if (node == NULL) {
        error("Invalid expression");
    }

    switch (node->kind) {
    case ND_NEG:
        gen_expr(node->lhs);
        printf("\tneg x0, x0\n");
        return;
    case ND_NUM:
        printf("\tmov x0, #%lld\n", node->val);
        return;
    case ND_VAR:
        gen_addr(node);
        printf("\tldr x0, [x0]\n");
        return;
    case ND_ASSIGN:
        gen_addr(node->lhs);
        push("x0");
        gen_expr(node->rhs);
        pop("x1");
        printf("\tstr x0, [x1]\n");
        return;
    default:
        break;
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
    case ND_EQ:
        printf("\tcmp x1, x0\n");
        printf("\tcset x0, eq\n");
        break;
    case ND_NE:
        printf("\tcmp x1, x0\n");
        printf("\tcset x0, ne\n");
        break;
    case ND_LT:
        printf("\tcmp x1, x0\n");
        printf("\tcset x0, lt\n");
        break;
    case ND_LE:
        printf("\tcmp x1, x0\n");
        printf("\tcset x0, le\n");
        break;
    case ND_GT:
        printf("\tcmp x1, x0\n");
        printf("\tcset x0, gt\n");
        break;
    case ND_GE:
        printf("\tcmp x1, x0\n");
        printf("\tcset x0, ge\n");
        break;
    default:
        error("Invalid expression");
    }

    return;
}

static void gen_stmt(Node *node) {
    if (node == NULL) {
        error("Invalid statement");
    }

    if (node->kind == ND_EXPR_STMT) {
        gen_expr(node->lhs);
        return;
    }

    error("Invalid statement");
}

void codegen(Node *node) {
    printf("\t.global main\n");
    printf("main:\n");

    printf("\tstp x29, x30, [sp, #-16]!\n");
    printf("\tmov x29, sp\n");
    printf("\tsub sp, sp, #208\n");

    Node *n = node;
    while (n != NULL) {
        gen_stmt(n);
        assert(depth == 0);
        n = n->next;
    }

    printf("\tmov sp, x29\n");
    printf("\tldp x29, x30, [sp], #16\n");
    printf("\tret\n");
    return;
}
