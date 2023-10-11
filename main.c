#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"

static char *opt_o;
static char *input_file;

static void usage(int status) {
    fprintf(stderr, "Usage: ./main [-o <path>] <file>\n");
    exit(status);
}

static void parse_args(int argc, char **argv) {
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0) {
            usage(EXIT_SUCCESS);
        }

        if (strcmp(argv[i], "-o") == 0) {
            if (argv[++i] == NULL) {
                usage(EXIT_FAILURE);
            }

            opt_o = argv[i];
            continue;
        }

        if (strncmp(argv[i], "-o", 2) == 0) {
            opt_o = argv[i] + 2;
            continue;
        }

        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            error("Unknown argument: %s", argv[i]);
        }

        input_file = argv[i];
    }

    if (input_file == NULL) {
        error("No input files");
    }
}

static FILE *open_file(char *path) {
    if (path == NULL || strcmp(path, "-") == 0) {
        return stdout;
    }

    FILE *out = fopen(path, "w");
    if (out == NULL) {
        error("Cannot open output file: %s: %s", path, strerror(errno));
    }

    return out;
}

int main(int argc, char **argv) {
    parse_args(argc, argv);

    Token *tk = tokenize_file(input_file);
    Obj *prog = parse(tk);

    FILE *out = open_file(opt_o);
    codegen(prog, out);
    return EXIT_SUCCESS;
}
