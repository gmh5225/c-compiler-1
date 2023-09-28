#include <stdbool.h>
#include <stdlib.h>
#include "main.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        error("Invalid number of arguments");
        return EXIT_FAILURE;
    }

    Token *tk = tokenize_file(argv[1]);
    Obj *prog = parse(tk);
    codegen(prog);
    return EXIT_SUCCESS;
}
