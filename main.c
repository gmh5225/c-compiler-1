#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <source>\n", argv[0]);
        return EXIT_FAILURE;
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

        fprintf(stderr, "Unexpected character: '%c'\n", *p);
        return EXIT_FAILURE;
    }

    printf("\tret\n");
    return EXIT_SUCCESS;
}
