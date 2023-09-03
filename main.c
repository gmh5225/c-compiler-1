#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

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

int error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(EXIT_FAILURE);
    return EXIT_FAILURE;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        error("Invalid number of arguments");
    }

    char *p = argv[1];

    printf("\t.global main\n");
    printf("main:\n");

    long long n = strtoll(p, &p, 10);
    printf("\tmov x0, %lld\n", n);

    while (*p != '\0') {
        if (*p == '+') {
            long long n = strtoll(p + 1, &p, 10);
            printf("\tadd x0, x0, %lld\n", n);
            continue;
        }

        if (*p == '-') {
            long long n = strtoll(p + 1, &p, 10);
            printf("\tsub x0, x0, %lld\n", n);
            continue;
        }

        error("Unexpected character: '%c'", *p);
    }

    printf("\tret\n");
    return EXIT_SUCCESS;
}
