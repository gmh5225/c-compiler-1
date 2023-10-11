#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "main.h"

static FILE *output_file = NULL;
static int depth = 0;
static char *argreg32[] = {"w0", "w1", "w2", "w3", "w4", "w5", "w6", "w7"};
static char *argreg64[] = {"x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7"};
static Obj *current_fn = NULL;

static void println(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(output_file, fmt, ap);
    fprintf(output_file, "\n");
    va_end(ap);
    return;
}

static int count(void) {
    static int i = 0;
    return i++;
}

static void push(char *reg) {
    if ((depth++ & 1) == 0) {
        println("\tsub sp, sp, #16");
        println("\tstr %s, [sp, #8]", reg);
    } else {
        println("\tstr %s, [sp]", reg);
    }

    return;
}

static void pop(char *reg) {
    if ((--depth & 1) == 0) {
        println("\tldr %s, [sp, #8]", reg);
        println("\tadd sp, sp, #16");
    } else {
        println("\tldr %s, [sp]", reg);
    }

    return;
}

static void store(char *dst, char *src, Type *ty) {
    if (ty->kind == TY_ARRAY) {
        return;
    }

    switch (ty->size) {
    case 1:
        println("\tstrb w%s, [%s]", &dst[1], src);
        return;
    case 8:
        println("\tstr x%s, [%s]", &dst[1], src);
        return;
    default:
        assert(false);
        return;
    }
}

static void load(char *dst, char *src, Type *ty) {
    if (ty->kind == TY_ARRAY) {
        return;
    }

    switch (ty->size) {
    case 1:
        println("\tldrb w%s, [%s]", &dst[1], src);
        return;
    case 8:
        println("\tldr x%s, [%s]", &dst[1], src);
        return;
    default:
        assert(false);
        return;
    }
}

static int align_to(int n, int align) {
    return (n + align - 1) / align * align;
}

static void gen_expr(Node *node);
static void gen_stmt(Node *node);

static void gen_addr(Node *node) {
    if (node == NULL) {
        error("Invalid lvalue");
        return;
    }

    switch (node->kind) {
    case ND_VAR:
        if (node->var->is_local) {
            println("\tsub x0, x29, #%d", node->var->offset);
        } else {
            println("\tadr x0, %s", node->var->name);
        }
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
        println("\tneg x0, x0");
        return;
    case ND_NUM:
        println("\tmov x0, #%lld", node->val);
        return;
    case ND_VAR:
        gen_addr(node);
        load("x0", "x0", node->ty);
        return;
    case ND_DEREF:
        gen_expr(node->lhs);
        load("x0", "x0", node->ty);
        return;
    case ND_ADDR:
        gen_addr(node->lhs);
        return;
    case ND_ASSIGN:
        gen_addr(node->lhs);
        push("x0");
        gen_expr(node->rhs);
        pop("x1");
        store("x0", "x1", node->ty);
        return;
    case ND_STMT_EXPR:
        for (Node *n = node->body; n != NULL; n = n->next) {
            gen_stmt(n);
        }
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
            pop(argreg64[i]);
        }
        println("\tbl %s", node->funcname);
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
        println("\tadd x0, x1, x0");
        return;
    case ND_SUB:
        println("\tsub x0, x1, x0");
        return;
    case ND_MUL:
        println("\tmul x0, x1, x0");
        return;
    case ND_DIV:
        println("\tsdiv x0, x1, x0");
        return;
    case ND_EQ:
        println("\tcmp x1, x0");
        println("\tcset x0, eq");
        return;
    case ND_NE:
        println("\tcmp x1, x0");
        println("\tcset x0, ne");
        return;
    case ND_LT:
        println("\tcmp x1, x0");
        println("\tcset x0, lt");
        return;
    case ND_LE:
        println("\tcmp x1, x0");
        println("\tcset x0, le");
        return;
    case ND_GT:
        println("\tcmp x1, x0");
        println("\tcset x0, gt");
        return;
    case ND_GE:
        println("\tcmp x1, x0");
        println("\tcset x0, ge");
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
        println("\tcmp x0, #0");
        println("\tbeq .L.else.%d", c);
        gen_stmt(node->then);
        println("\tb .L.end.%d", c);
        println(".L.else.%d:", c);
        if (node->els != NULL) {
            gen_stmt(node->els);
        }
        println(".L.end.%d:", c);
        return;
    }
    case ND_FOR: {
        int c = count();
        if (node->init != NULL) {
            gen_stmt(node->init);
        }
        println(".L.begin.%d:", c);
        if (node->cond != NULL) {
            gen_expr(node->cond);
            println("\tcmp x0, #0");
            println("\tbeq .L.end.%d", c);
        }
        gen_stmt(node->then);
        if (node->inc != NULL) {
            gen_expr(node->inc);
        }
        println("\tb .L.begin.%d", c);
        println(".L.end.%d:", c);
        return;
    }
    case ND_BLOCK:
        for (Node *n = node->body; n != NULL; n = n->next) {
            gen_stmt(n);
        }
        return;
    case ND_RETURN:
        gen_expr(node->lhs);
        println("\tb .L.return.%s", current_fn->name);
        return;
    case ND_EXPR_STMT:
        gen_expr(node->lhs);
        return;
    default:
        error_tk(node->tk, "Invalid statement");
        return;
    }
}

static void assign_lvar_offsets(Obj *prog) {
    for (Obj *fn = prog; fn != NULL; fn = fn->next) {
        if (!fn->is_function) {
            continue;
        }

        int offset = 0;
        for (Obj *v = fn->locals; v != NULL; v = v->next) {
            v->offset = offset += v->ty->size;
        }

        fn->stack_size = align_to(offset, 16);
    }

    return;
}

static void gen_data(Obj *prog) {
    for (Obj *v = prog; v != NULL; v = v->next) {
        if (v->is_function) {
            continue;
        }

        println("\t.data");
        println("\t.global %s", v->name);
        println("%s:", v->name);

        if (v->init_data != NULL) {
            for (int i = 0; i < v->ty->size; ++i) {
                println("\t.byte %d", v->init_data[i]);
            }
        } else {
            println("\t.zero %d", v->ty->size);
        }
    }

    return;
}

static void gen_text(Obj *prog) {
    println("\t.text");

    for (Obj *fn = prog; fn != NULL; fn = fn->next) {
        if (!fn->is_function) {
            continue;
        }

        current_fn = fn;
        println("\t.global %s", fn->name);
        println("%s:", fn->name);

        println("\tstp x29, x30, [sp, #-16]!");
        println("\tmov x29, sp");
        println("\tsub sp, sp, #%d", fn->stack_size);

        int i = 0;
        for (Obj *v = fn->params; v != NULL; v = v->next) {
            if (v->ty->size == 1) {
                println("\tstrb %s, [x29, #-%d]", argreg32[i++], v->offset);
            } else {
                println("\tstr %s, [x29, #-%d]", argreg64[i++], v->offset);
            }
        }

        gen_stmt(fn->body);
        assert(depth == 0);

        println(".L.return.%s:", fn->name);
        println("\tmov sp, x29");
        println("\tldp x29, x30, [sp], #16");
        println("\tret");
    }

    return;
}

void codegen(Obj *prog, FILE *out) {
    output_file = out;

    assign_lvar_offsets(prog);
    gen_data(prog);
    gen_text(prog);
    return;
}
