#ifndef ARGPARSE_H
#define ARGPARSE_H

#include <stddef.h>

typedef struct args {
    size_t bytes_size;
    size_t token_count;
    char *bytes;
    char **argv;
} args;

/**
 * @brief Collect whitespace delimited words into args
 * 
 * @param a Destination args
 * @param str Source string
 */
void args_parse(args *a, const char *str);

/**
 * @brief Deallocate internally memory used by args
 * 
 * @param a Target args
 */
void args_free(args *a);

#endif
