#include <stdbool.h>
#include <stdlib.h>
#include "main.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        error("Invalid number of arguments");
        return EXIT_FAILURE;
    }

    Token *tk = tokenize_file(argv[2]);
    Obj *prog = parse(tk);

    FILE *out = fopen(argv[1], "w");
    codegen(prog, out);
    return EXIT_SUCCESS;
}
