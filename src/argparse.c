#include <stdlib.h>
#include <string.h>

#include "argparse.h"


/******************************************************************************
 *                                 UTILITIES                                  * 
 ******************************************************************************/


/**
 * @brief Count number of null delimited strings in bytes.
 * 
 * @param bytes Source data ended with a double null character
 * @return size_t Number of tokens
 */
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

/**
 * @brief 
 * 
 * @param bytes Source data ended with a null character
 * @param token_count Number of tokens in bytes
 * @return char** Array of string tokens parsed. Last item in array is NULL
 */
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

/**
 * @brief Consumes a string of bytes and casts it into args
 * 
 * @param a destination
 * @param bytes source
 * @param size number of bytes
 */
static void args_cast(args *a, char *bytes, size_t size) {
    a->bytes = bytes;
    a->bytes_size = size;
    a->token_count = count_tokens(bytes);
    a->argv = to_argv(bytes, a->token_count);
}


/******************************************************************************
 *                             PUBLIC INTERFACE                               * 
 ******************************************************************************/


/**
 * @brief Collect whitespace delimited words into args
 * 
 * @param a Destination args
 * @param str Source string
 */
void args_parse(args *a, const char *str) {
    /* Initialise buffer with 2 additional bytes for a double null terminator */
    char *buffer = calloc(strlen(str) + 2, sizeof(char));
    size_t len = 0;
    
    /* Add '\0' after every token and trim additonal whitespace between them. */
    while (*str != '\0') {
        if (*str == ' ') {
            buffer[len++] = '\0';
            /* Skip contiguous whitespaces */
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

/**
 * @brief Deallocated internally memory used by args
 * 
 * @param a Target args
 */
void args_free(args *a) {
    free(a->argv);
    a->argv = NULL;

    free(a->bytes);
    a->bytes = NULL;
}
