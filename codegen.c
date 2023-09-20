#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "main.h"

static int depth = 0;
static char *argreg[] = {"x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7"};
static Function *current_fn = NULL;

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

static void gen_expr(Node *node);

static void gen_addr(Node *node) {
    if (node == NULL) {
        error("Invalid lvalue");
        return;
    }

    switch (node->kind) {
    case ND_VAR:
        printf("\tsub x0, x29, #%d\n", node->var->offset);
        return;
    case ND_DEREF:
        gen_expr(node->lhs);
        return;
    default:
        error_tk(node->tk, "Not an lvalue");
        return;
    }
}

static void gen_expr(Node *node) {
    if (node == NULL) {
        error_tk(node->tk, "Invalid expression");
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
        if (node->ty->kind != TY_ARRAY) {
            printf("\tldr x0, [x0]\n");
        }
        return;
    case ND_DEREF:
        gen_expr(node->lhs);
        if (node->ty->kind != TY_ARRAY) {
            printf("\tldr x0, [x0]\n");
        }
        return;
    case ND_ADDR:
        gen_addr(node->lhs);
        return;
    case ND_ASSIGN:
        gen_addr(node->lhs);
        push("x0");
        gen_expr(node->rhs);
        pop("x1");
        printf("\tstr x0, [x1]\n");
        return;
    case ND_FUNC_CALL: {
        int nargs = 0;
        Node *arg = node->args;
        while (arg != NULL) {
            gen_expr(arg);
            push("x0");
            arg = arg->next;
            nargs += 1;
        }
        assert(nargs <= 8);
        for (int i = nargs - 1; i >= 0; --i) {
            pop(argreg[i]);
        }
        printf("\tbl %s\n", node->funcname);
        return;
    }
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
        error_tk(node->tk, "Invalid expression");
        return;
    }
}

static void gen_stmt(Node *node) {
    if (node == NULL) {
        error_tk(node->tk, "Invalid statement");
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
        printf("\tb .L.return.%s\n", current_fn->name);
        return;
    case ND_EXPR_STMT:
        gen_expr(node->lhs);
        return;
    default:
        error_tk(node->tk, "Invalid statement");
        return;
    }
}

static void assign_lvar_offsets(Function *prog) {
    for (Function *fn = prog; fn != NULL; fn = fn->next) {
        int offset = 0;
        for (Obj *v = fn->locals; v != NULL; v = v->next) {
            v->offset = offset += v->ty->size;
        }

        fn->stack_size = align_to(offset, 16);
    }

    return;
}

void codegen(Function *prog) {
    assign_lvar_offsets(prog);

    for (Function *fn = prog; fn != NULL; fn = fn->next) {
        current_fn = fn;
        printf("\t.global %s\n", fn->name);
        printf("%s:\n", fn->name);

        printf("\tstp x29, x30, [sp, #-16]!\n");
        printf("\tmov x29, sp\n");
        printf("\tsub sp, sp, #%d\n", fn->stack_size);

        int i = 0;
        for (Obj *v = fn->params; v != NULL; v = v->next) {
            printf("\tstr %s, [x29, #-%d]\n", argreg[i++], v->offset);
        }

        gen_stmt(fn->body);
        assert(depth == 0);

        printf(".L.return.%s:\n", fn->name);
        printf("\tmov sp, x29\n");
        printf("\tldp x29, x30, [sp], #16\n");
        printf("\tret\n");
    }

    return;
}
