#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "input.h"

/**
 * @brief Read until EOF
 *
 * @param fd Input file descriptor
 * @param buffer_size The number of bytes to read at once.
 * 64 bytes if 0 specified
 * @param terminator Stop reading when terminator is reached. Not included in
 * the output string
 * @return char* Read string. Null if nothing read
 */
char *read_all(int fd, size_t buffer_size, char terminator) {
    static const size_t default_buffer_size = 64;

    if (buffer_size <= 0) {
        buffer_size = default_buffer_size;
    }

    char buffer[buffer_size];
    
    char* read_str = NULL;
    size_t str_len = 0;
    ssize_t read_len = 0;
    
    bool terminator_found = false;
    
    /* Collect characters until EOF */
    while (!terminator_found && (read_len = read(fd, buffer, buffer_size)) > 0)
    {
        /* Search for our terminator */
        for (ssize_t i = 0; i < read_len; ++i) {
            if (buffer[i] == terminator) {
                terminator_found = true;
                buffer[i] = '\0';
                read_len = i + 1;
                break;
            }
        }

        read_str = realloc(read_str, (str_len + (size_t)read_len));
        memcpy(read_str + str_len, buffer, (size_t)read_len);
        str_len += (size_t)read_len;
    }

    return read_str;
}


/**
 * @brief Split string by linefeed. Not re-entrant because of internal strtok()
 * 
 * @param str Input string to split. Mutates the string
 * @param[out] count Return value of the number of lines parsed
 * @return char** List of strings lines
 * 
 * @note The array returned has to be freed, but no memory is allocated for
 * its items. Deallocate the passed in string argument to free all the items. 
 */
char **split_lines(char *str, size_t *count) {
    if (str == NULL) {
        return NULL;
    }

    char **tokens = NULL;
    size_t n = 0;

    char *token = strtok(str, "\n");
    while (token) {
        n++;
        tokens = realloc(tokens, n * sizeof(char *));
        tokens[n-1] = token;
        token = strtok(NULL, "\n");
    }
    
    *count = n;
    return tokens;
}
