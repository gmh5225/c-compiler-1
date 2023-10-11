#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include "main.h"

Type *ty_char = &(Type) { TY_CHAR, 1 };
Type *ty_int  = &(Type) { TY_INT,  8 };

bool is_integer(Type *ty) {
    return ty->kind == TY_INT || ty->kind == TY_CHAR;
}

Type *copy_type(Type *ty) {
    Type *ret = calloc(1, sizeof(Type));
    *ret = *ty;
    return ret;
}

Type *pointer_to(Type *base) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_PTR;
    ty->size = 8;
    ty->base = base;
    return ty;
}

Type *func_type(Type *return_ty) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_FUNC;
    ty->return_ty = return_ty;
    return ty;
}

Type *array_of(Type *base, int len) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_ARRAY;
    ty->size = base->size * len;
    ty->base = base;
    ty->array_len = len;
    return ty;
}

void add_type(Node *node) {
    if (node == NULL) {
        return;
    }

    if (node->ty != NULL) {
        return;
    }

    add_type(node->lhs);
    add_type(node->rhs);
    add_type(node->cond);
    add_type(node->then);
    add_type(node->els);
    add_type(node->init);
    add_type(node->inc);

    for (Node *n = node->body; n != NULL; n = n->next) {
        add_type(n);
    }

    for (Node *n = node->args; n != NULL; n = n->next) {
        add_type(n);
    }

    switch (node->kind) {
    case ND_ADD:
    case ND_SUB:
    case ND_MUL:
    case ND_DIV:
    case ND_NEG:
        node->ty = node->lhs->ty;
        return;
    case ND_ASSIGN:
        if (node->lhs->ty->kind == TY_ARRAY) {
            error_tk(node->tk, "Not an lvalue");
        }
        node->ty = node->lhs->ty;
        return;
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
    case ND_NUM:
    case ND_FUNC_CALL:
        node->ty = ty_int;
        return;
    case ND_VAR:
        node->ty = node->var->ty;
        return;
    case ND_DEREF:
        if (node->lhs->ty->base == NULL) {
            error_tk(node->tk, "Invalid pointer dereference");
        }
        node->ty = node->lhs->ty->base;
        return;
    case ND_ADDR:
        if (node->lhs->ty->kind == TY_ARRAY) {
            node->ty = pointer_to(node->lhs->ty->base);
        } else {
            node->ty = pointer_to(node->lhs->ty);
        }
        return;
    case ND_STMT_EXPR:
        if (node->body) {
            Node *stmt = node->body;
            while (stmt->next != NULL) {
                stmt = stmt->next;
            }
            if (stmt->kind == ND_EXPR_STMT) {
                node->ty = stmt->lhs->ty;
                return;
            }
        }
        error_tk(node->tk, "Statement expression returning void is not supported");
    default:
        return;
    }
}
