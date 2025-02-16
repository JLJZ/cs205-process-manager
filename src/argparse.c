#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "argparse.h"

static size_t count_tokens(const char *bytes) {
    size_t count = 0;

    /* Bytes is ended by a double null terminator */
    while (*bytes || *(bytes + 1)) {
        
        /* Tokens are delimited by a single null terminator */
        if (*bytes && *(bytes + 1) == '\0') {
            count++;
        }
        bytes++;
    }

    return count;
}

static char **to_argv(char *bytes, size_t token_count) {
    char **argv = calloc(token_count + 1, sizeof(char *));
    argv[token_count] = NULL;
    
    size_t i = 0;
    char *iter = bytes;
    
    while (*iter) {
        argv[i++] = iter;
        iter += strlen(iter) + 1;
    }

    return argv;
}

void args_parse(args *a, const char *str) {
    /* Initialise buffer with 2 additional bytes for a double null terminator */
    char *buffer = calloc(strlen(str) + 2, sizeof(char));
    size_t len = 0;
    
    /* Add '\0' after every token and trim additonal whitespace between them. */
    while (*str != '\0') {
        if (*str == ' ') {
            buffer[len++] = '\0';
            while (*str == ' ') {
                str++;
            }

        } else {
            buffer[len++] = *str;
            str++;
        }
    }
    
    /* Length of bytes + 2 additional null terminator */
    len += 2;
    
    buffer = realloc(buffer, len * sizeof(char));
    
    args_cast(a, buffer, len);
}

void args_cast(args *a, char *bytes, size_t size) {
    a->bytes = bytes;
    a->bytes_size = size;
    a->token_count = count_tokens(bytes);
    a->argv = to_argv(bytes, a->token_count);
}

void args_free(args *a) {
    free(a->argv);
    free((void *)a->bytes);
}
