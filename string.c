#define _POSIX_C_SOURCE 200809L
#include <stdarg.h>
#include <stdio.h>
#include "main.h"

char *format(char *fmt, ...) {
    char *buf;
    size_t buflen;
    FILE *out = open_memstream(&buf, &buflen);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(out, fmt, ap);
    va_end(ap);
    fclose(out);
    return buf;
}
