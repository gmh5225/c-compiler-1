#include <stdbool.h>
#include <stdlib.h>
#include "main.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        error("Invalid number of arguments");
    }

    Token *tk = tokenize(argv[1]);
    Function *prog = parse(tk);
    codegen(prog);
    return EXIT_SUCCESS;
}
