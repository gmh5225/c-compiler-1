#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <source>\n", argv[0]);
        return EXIT_FAILURE;
    }

    printf("\t.global main\n");
    printf("main:\n");
    printf("\tmov x0, %s\n", argv[1]);
    printf("\tret\n");
    return EXIT_SUCCESS;
}
