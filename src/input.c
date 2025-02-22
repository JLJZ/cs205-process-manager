#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "input.h"

/**
 * @brief Read until EOF
 * 
 * @return char* Read string
 */
char *read_all(int fd, size_t buffer_size) {
    static const size_t default_buffer_size = 64;

    if (buffer_size <= 0) {
        buffer_size = default_buffer_size;
    }

    char buffer[buffer_size];
    
    char* read_str = NULL;
    size_t str_len = 0;
    ssize_t read_len = 0;
    
    /* Collect characters until EOF */
    while ((read_len = read(fd, buffer, buffer_size)) > 0) {
        read_str = realloc(read_str, (str_len + (size_t)read_len + 1) * sizeof(char));
        memcpy(read_str + str_len, buffer, (size_t)read_len);
        str_len += (size_t)read_len;
    }
    
    if (read_str != NULL) {
        /* Set null terminator */
        read_str[str_len] = '\0';

        /* Remove newline character */
        if (read_str[str_len - 1] == '\n') {
            read_str[str_len - 1] = '\0'; 
        }
    }

    return read_str;
}
