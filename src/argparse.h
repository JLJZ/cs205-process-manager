#ifndef ARGPARSE_H
#define ARGPARSE_H

#include <stddef.h>

typedef struct args {
    size_t bytes_size;
    size_t token_count;
    char *bytes;
    char **argv;
} args;

void args_parse(args *a, const char *str);

void args_free(args *a);

#endif
