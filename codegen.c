#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "main.h"

static int depth = 0;

static int count(void) {
    static int i = 0;
    return i++;
}

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

static int align_to(int n, int align) {
    return (n + align - 1) / align * align;
}

static void gen_addr(Node *node) {
    if (node == NULL) {
        error("Invalid lvalue");
        return;
    }

    if (node->kind == ND_VAR) {
        printf("\tsub x0, x29, #%d\n", node->var->offset);
        return;
    }

    error("Not an lvalue");
    return;
}

static void gen_expr(Node *node) {
    if (node == NULL) {
        error("Invalid expression");
        return;
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
        return;
    case ND_SUB:
        printf("\tsub x0, x1, x0\n");
        return;
    case ND_MUL:
        printf("\tmul x0, x1, x0\n");
        return;
    case ND_DIV:
        printf("\tsdiv x0, x1, x0\n");
        return;
    case ND_EQ:
        printf("\tcmp x1, x0\n");
        printf("\tcset x0, eq\n");
        return;
    case ND_NE:
        printf("\tcmp x1, x0\n");
        printf("\tcset x0, ne\n");
        return;
    case ND_LT:
        printf("\tcmp x1, x0\n");
        printf("\tcset x0, lt\n");
        return;
    case ND_LE:
        printf("\tcmp x1, x0\n");
        printf("\tcset x0, le\n");
        return;
    case ND_GT:
        printf("\tcmp x1, x0\n");
        printf("\tcset x0, gt\n");
        return;
    case ND_GE:
        printf("\tcmp x1, x0\n");
        printf("\tcset x0, ge\n");
        return;
    default:
        error("Invalid expression");
        return;
    }
}

static void gen_stmt(Node *node) {
    if (node == NULL) {
        error("Invalid statement");
        return;
    }

    switch (node->kind) {
    case ND_IF: {
        int c = count();
        gen_expr(node->cond);
        printf("\tcmp x0, #0\n");
        printf("\tbeq .L.else.%d\n", c);
        gen_stmt(node->then);
        printf("\tb .L.end.%d\n", c);
        printf(".L.else.%d:\n", c);
        if (node->els != NULL) {
            gen_stmt(node->els);
        }
        printf(".L.end.%d:\n", c);
        return;
    }
    case ND_FOR: {
        int c = count();
        if (node->init != NULL) {
            gen_stmt(node->init);
        }
        printf(".L.begin.%d:\n", c);
        if (node->cond != NULL) {
            gen_expr(node->cond);
            printf("\tcmp x0, #0\n");
            printf("\tbeq .L.end.%d\n", c);
        }
        gen_stmt(node->then);
        if (node->inc != NULL) {
            gen_expr(node->inc);
        }
        printf("\tb .L.begin.%d\n", c);
        printf(".L.end.%d:\n", c);
        return;
    }
    case ND_BLOCK:
        for (Node *n = node->body; n != NULL; n = n->next) {
            gen_stmt(n);
        }
        return;
    case ND_RETURN:
        gen_expr(node->lhs);
        printf("\tb .L.return\n");
        return;
    case ND_EXPR_STMT:
        gen_expr(node->lhs);
        return;
    default:
        error("Invalid statement");
        return;
    }
}

static void assign_lvar_offsets(Function *prog) {
    int offset = 0;
    for (Obj *v = prog->locals; v != NULL; v = v->next) {
        v->offset = offset += 8;
    }

    prog->stack_size = align_to(offset, 16);
    return;
}

void codegen(Function *prog) {
    assign_lvar_offsets(prog);

    printf("\t.global main\n");
    printf("main:\n");

    printf("\tstp x29, x30, [sp, #-16]!\n");
    printf("\tmov x29, sp\n");
    printf("\tsub sp, sp, #%d\n", prog->stack_size);

    gen_stmt(prog->body);
    assert(depth == 0);

    printf(".L.return:\n");
    printf("\tmov sp, x29\n");
    printf("\tldp x29, x30, [sp], #16\n");
    printf("\tret\n");
    return;
}
