#define _POSIX_C_SOURCE 200809L
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"

Obj *locals;

static Obj *find_var(Token *tk) {
    for (Obj *v = locals; v != NULL; v = v->next) {
        if (strlen(v->name) == tk->len && !strncmp(tk->loc, v->name, tk->len)) {
            return v;
        }
    }

    return NULL;
}

static Node *new_node(NodeKind kind, Token *tk) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->tk = tk;
    return node;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tk) {
    Node *node = new_node(kind, tk);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_unary(NodeKind kind, Node *lhs, Token *tk) {
    Node *node = new_node(kind, tk);
    node->lhs = lhs;
    return node;
}

static Node *new_num(long long val, Token *tk) {
    Node *node = new_node(ND_NUM, tk);
    node->val = val;
    return node;
}

static Node *new_var_node(Obj *var, Token *tk) {
    Node *node = new_node(ND_VAR, tk);
    node->var = var;
    return node;
}

static Obj *new_lvar(char *name, Type *ty) {
    Obj *var = calloc(1, sizeof(Obj));
    var->name = name;
    var->ty = ty;
    var->next = locals;
    locals = var;
    return var;
}

static Node *new_add(Node *lhs, Node *rhs, Token *tk) {
    add_type(lhs);
    add_type(rhs);

    if (is_integer(lhs->ty) && is_integer(rhs->ty)) {
        return new_binary(ND_ADD, lhs, rhs, tk);
    }

    if (lhs->ty->base && is_integer(rhs->ty)) {
        rhs = new_binary(ND_MUL, rhs, new_num(8, tk), tk);
        return new_binary(ND_ADD, lhs, rhs, tk);
    }

    if (is_integer(lhs->ty) && rhs->ty->base) {
        lhs = new_binary(ND_MUL, lhs, new_num(8, tk), tk);
        return new_binary(ND_ADD, lhs, rhs, tk);
    }

    error_tk(tk, "Invalid operands");
    return NULL;
}

static Node *new_sub(Node *lhs, Node *rhs, Token *tk) {
    add_type(lhs);
    add_type(rhs);

    if (is_integer(lhs->ty) && is_integer(rhs->ty)) {
        return new_binary(ND_SUB, lhs, rhs, tk);
    }

    if (lhs->ty->base && is_integer(rhs->ty)) {
        rhs = new_binary(ND_MUL, rhs, new_num(8, tk), tk);
        return new_binary(ND_SUB, lhs, rhs, tk);
    }

    if (lhs->ty->base && rhs->ty->base) {
        Node *node = new_binary(ND_SUB, lhs, rhs, tk);
        node->ty = ty_int;
        return new_binary(ND_DIV, node, new_num(8, tk), tk);
    }

    error_tk(tk, "Invalid operands");
    return NULL;
}

static Node *stmt(Token **rest, Token *tk);
static Node *compound_stmt(Token **rest, Token *tk);
static Node *expr_stmt(Token **rest, Token *tk);
static Node *expr(Token **rest, Token *tk);
static Node *assign(Token **rest, Token *tk);
static Node *equality(Token **rest, Token *tk);
static Node *relational(Token **rest, Token *tk);
static Node *add(Token **rest, Token *tk);
static Node *mul(Token **rest, Token *tk);
static Node *unary(Token **rest, Token *tk);
static Node *primary(Token **rest, Token *tk);

static Type *type_suffix(Token **rest, Token *tk, Type *ty);

static char *get_ident(Token *tk) {
    if (tk->kind != TK_IDENT) {
        error_tk(tk, "Expected an identifier");
        return NULL;
    }

    return strndup(tk->loc, tk->len);
}

// declspec = "int"
static Type *declspec(Token **rest, Token *tk) {
    *rest = skip(tk, "int");
    return ty_int;
}

// declarator = "*"* ident type-suffix
static Type *declarator(Token **rest, Token *tk, Type *ty) {
    while (consume(&tk, tk, "*")) {
        ty = pointer_to(ty);
    }

    if (tk->kind != TK_IDENT) {
        error_tk(tk, "Expected a variable name");
        return NULL;
    }

    ty = type_suffix(rest, tk->next, ty);
    ty->name = tk;
    return ty;
}

// type-suffix = ("(" func-params? ")")?
// func-params = param ("," param)*
// param       = declspec declarator
static Type *type_suffix(Token **rest, Token *tk, Type *ty) {
    if (equal(tk, "(")) {
        tk = tk->next;

        Type head = {0};
        Type *cur = &head;

        while (!equal(tk, ")")) {
            if (cur != &head) {
                tk = skip(tk, ",");
            }

            Type *basety = declspec(&tk, tk);
            Type *ty = declarator(&tk, tk, basety);

            cur->next = ty;
            cur = cur->next;
        }

        ty = func_type(ty);
        ty->params = head.next;
        *rest = tk->next;
        return ty;
    }

    *rest = tk;
    return ty;
}

// declaration = declspec (declarator ("=" expr)? ("," declarator ("=" expr)?)*)? ";"
static Node *declaration(Token **rest, Token *tk) {
    Type *basety = declspec(&tk, tk);

    Node head = {0};
    Node *cur = &head;
    int i = 0;

    while (!equal(tk, ";")) {
        if (i++ > 0) {
            tk = skip(tk, ",");
        }

        Type *ty = declarator(&tk, tk, basety);
        Obj *var = new_lvar(get_ident(ty->name), ty);

        if (!equal(tk, "=")) {
            continue;
        }

        Node *lhs = new_var_node(var, tk);
        Node *rhs = expr(&tk, tk->next);
        Node *node = new_binary(ND_ASSIGN, lhs, rhs, tk);
        cur->next = new_unary(ND_EXPR_STMT, node, tk);
        cur = cur->next;
    }

    Node *node = new_node(ND_BLOCK, tk);
    node->body = head.next;
    *rest = skip(tk, ";");
    return node;
}

// stmt = "return" expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//      | "while" "(" expr ")" stmt
//      | "{" compound-stmt
//      | expr-stmt
static Node *stmt(Token **rest, Token *tk) {
    if (equal(tk, "return")) {
        Node *node = new_node(ND_RETURN, tk);
        node->lhs = expr(&tk, tk->next);
        *rest = skip(tk, ";");
        return node;
    }

    if (equal(tk, "if")) {
        Node *node = new_node(ND_IF, tk);
        tk = skip(tk->next, "(");
        node->cond = expr(&tk, tk);
        tk = skip(tk, ")");
        node->then = stmt(&tk, tk);
        if (equal(tk, "else")) {
            node->els = stmt(&tk, tk->next);
        }
        *rest = tk;
        return node;
    }

    if (equal(tk, "for")) {
        Node *node = new_node(ND_FOR, tk);
        tk = skip(tk->next, "(");
        node->init = expr_stmt(&tk, tk);
        if (!equal(tk, ";")) {
            node->cond = expr(&tk, tk);
        }
        tk = skip(tk, ";");
        if (!equal(tk, ")")) {
            node->inc = expr(&tk, tk);
        }
        tk = skip(tk, ")");
        node->then = stmt(&tk, tk);
        *rest = tk;
        return node;
    }

    if (equal(tk, "while")) {
        Node *node = new_node(ND_FOR, tk);
        tk = skip(tk->next, "(");
        node->cond = expr(&tk, tk);
        tk = skip(tk, ")");
        node->then = stmt(&tk, tk);
        *rest = tk;
        return node;
    }

    if (equal(tk, "{")) {
        return compound_stmt(rest, tk->next);
    }

    return expr_stmt(rest, tk);
}

// compound-stmt = (declaration | stmt)* "}"
static Node *compound_stmt(Token **rest, Token *tk) {
    Node head = {0};
    Node *cur = &head;

    while (!equal(tk, "}")) {
        Node *node;
        if (equal(tk, "int")) {
            node = declaration(&tk, tk);
        } else {
            node = stmt(&tk, tk);
        }

        cur->next = node;
        cur = cur->next;
        add_type(cur);
    }

    Node *node = new_node(ND_BLOCK, tk);
    node->body = head.next;
    *rest = tk->next;
    return node;
}

// expr-stmt = expr? ";"
static Node *expr_stmt(Token **rest, Token *tk) {
    if (equal(tk, ";")) {
        *rest = tk->next;
        return new_node(ND_BLOCK, tk);
    }

    Node *node = new_node(ND_EXPR_STMT, tk);
    node->lhs = expr(&tk, tk);
    *rest = skip(tk, ";");
    return node;
}

// expr = assign
static Node *expr(Token **rest, Token *tk) {
    return assign(rest, tk);
}

// assign = equality ("=" assign)?
static Node *assign(Token **rest, Token *tk) {
    Node *lhs = equality(&tk, tk);

    if (equal(tk, "=")) {
        Node *rhs = assign(&tk, tk->next);
        lhs = new_binary(ND_ASSIGN, lhs, rhs, tk);
    }

    *rest = tk;
    return lhs;
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality(Token **rest, Token *tk) {
    Node *lhs = relational(&tk, tk);

    while (true) {
        Token *start = tk;

        if (equal(tk, "==")) {
            Node *rhs = relational(&tk, tk->next);
            lhs = new_binary(ND_EQ, lhs, rhs, start);
            continue;
        }

        if (equal(tk, "!=")) {
            Node *rhs = relational(&tk, tk->next);
            lhs = new_binary(ND_NE, lhs, rhs, start);
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
        Token *start = tk;

        if (equal(tk, "<")) {
            Node *rhs = add(&tk, tk->next);
            lhs = new_binary(ND_LT, lhs, rhs, start);
            continue;
        }

        if (equal(tk, "<=")) {
            Node *rhs = add(&tk, tk->next);
            lhs = new_binary(ND_LE, lhs, rhs, start);
            continue;
        }

        if (equal(tk, ">")) {
            Node *rhs = add(&tk, tk->next);
            lhs = new_binary(ND_GT, lhs, rhs, start);
            continue;
        }

        if (equal(tk, ">=")) {
            Node *rhs = add(&tk, tk->next);
            lhs = new_binary(ND_GE, lhs, rhs, start);
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
        Token *start = tk;

        if (equal(tk, "+")) {
            Node *rhs = mul(&tk, tk->next);
            lhs = new_add(lhs, rhs, start);
            continue;
        }

        if (equal(tk, "-")) {
            Node *rhs = mul(&tk, tk->next);
            lhs = new_sub(lhs, rhs, start);
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
        Token *start = tk;

        if (equal(tk, "*")) {
            Node *rhs = unary(&tk, tk->next);
            lhs = new_binary(ND_MUL, lhs, rhs, start);
            continue;
        }

        if (equal(tk, "/")) {
            Node *rhs = unary(&tk, tk->next);
            lhs = new_binary(ND_DIV, lhs, rhs, start);
            continue;
        }

        *rest = tk;
        return lhs;
    }
}

// unary = ("+" | "-" | "*" | "&") unary
//       | primary
static Node *unary(Token **rest, Token *tk) {
    if (equal(tk, "+")) {
        return unary(rest, tk->next);
    }

    if (equal(tk, "-")) {
        Node *node = unary(rest, tk->next);
        return new_unary(ND_NEG, node, tk);
    }

    if (equal(tk, "*")) {
        Node *node = unary(rest, tk->next);
        return new_unary(ND_DEREF, node, tk);
    }

    if (equal(tk, "&")) {
        Node *node = unary(rest, tk->next);
        return new_unary(ND_ADDR, node, tk);
    }

    return primary(rest, tk);
}

// funccall = ident "(" ")"
static Node *funccall(Token **rest, Token *tk) {
    Token *start = tk;
    tk = tk->next->next;

    Node head = {0};
    Node *cur = &head;

    while (!equal(tk, ")")) {
        if (cur != &head) {
            tk = skip(tk, ",");
        }

        cur->next = assign(&tk, tk);
        cur = cur->next;
    }

    *rest = skip(tk, ")");

    Node *node = new_node(ND_FUNC_CALL, start);
    node->funcname = strndup(start->loc, start->len);
    node->args = head.next;
    return node;
}

// primary = "(" expr ")"
//         | funccall
//         | ident
//         | num
static Node *primary(Token **rest, Token *tk) {
    if (equal(tk, "(")) {
        Node *node = expr(&tk, tk->next);
        *rest = skip(tk, ")");
        return node;
    }

    if (tk->kind == TK_IDENT) {
        if (equal(tk->next, "(")) {
            return funccall(rest, tk);
        }

        Obj *var = find_var(tk);
        if (var == NULL) {
            error_tk(tk, "Undefined variable");
            return NULL;
        }
        *rest = tk->next;
        return new_var_node(var, tk);
    }

    if (tk->kind == TK_NUM) {
        Node *node = new_num(tk->val, tk);
        *rest = tk->next;
        return node;
    }

    error_tk(tk, "Expected a number");
    return NULL;
}

static void create_param_lvars(Type *params) {
    if (params == NULL) {
        return;
    }

    create_param_lvars(params->next);
    new_lvar(get_ident(params->name), params);
    return;
}

// function = declspec declarator "{" compound-stmt
Function *function(Token **rest, Token *tk) {
    Type *ty = declspec(&tk, tk);
    ty = declarator(&tk, tk, ty);
    locals = NULL;

    Function *fn = calloc(1, sizeof(Function));
    fn->name = get_ident(ty->name);
    create_param_lvars(ty->params);
    fn->params = locals;

    tk = skip(tk, "{");
    fn->body = compound_stmt(rest, tk);
    fn->locals = locals;
    return fn;
}

// parse = function*
Function *parse(Token *tk) {
    Function head = {0};
    Function *cur = &head;

    while (tk->kind != TK_EOF) {
        cur->next = function(&tk, tk);
        cur = cur->next;
    }

    return head.next;
}
