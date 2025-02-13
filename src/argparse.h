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

/**
 * @brief Consumes a string of bytes and casts it into args
 * 
 * @param a destination
 * @param bytes source
 * @param size number of bytes
 */
void args_cast(args *a, char *bytes, size_t size);

void args_free(args *a);

#endif
