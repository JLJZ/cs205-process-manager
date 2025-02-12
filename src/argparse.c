#include <stdlib.h>
#include <string.h>

#include "argparse.h"

static char **to_argv(char *tokens, size_t count) {
    char **argv = calloc(count + 1, sizeof(char *));
    argv[count] = NULL;
    
    if (count == 0) {
        return argv;
    }
    
    size_t i = 0;
    char *iter = tokens;
    
    while (*iter) {
        argv[i++] = iter;
        iter += strlen(iter) + 1;
    }
    
    return argv;
}

void args_parse(args *a, const char *str) {
    char *buffer = strdup(str);
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
    
    if (buffer[len] != '\0') {
        buffer[len] = '\0';
        len++;
    }
    
    buffer = realloc(buffer, len * sizeof(char));
    
    args_cast(a, buffer, len);
}

void args_cast(args *a, char *bytes, size_t length) {
    a->bytes = bytes;
    a->length = length;
    a->argv = to_argv(bytes, length);
}

void args_free(args *a) {
    free(a->argv);
    free((void *)a->bytes);
}
