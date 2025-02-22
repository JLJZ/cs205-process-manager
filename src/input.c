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
 * @return char* Read string
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
