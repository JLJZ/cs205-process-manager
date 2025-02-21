#ifndef INPUT_H
#define INPUT_H

#include <stdio.h>

/**
 * @brief Read until EOF
 * 
 * @return char* Read string
 */
char *read_all(FILE *stream, int buffer_size) {
    static const int default_buffer_size = 64;

    if (buffer_size <= 0) {
        buffer_size = default_buffer_size;
    }

    char buffer[buffer_size];
    
    char* read_str = NULL;
    size_t str_len = 0;
    
    /* Collect characters until EOF */
    while (fgets(buffer, buffer_size, stream) != NULL) {
        size_t read_count = strlen(buffer);
        read_str = realloc(read_str, (str_len + read_count + 1) * sizeof(char));
        memcpy(read_str + str_len, buffer, read_count * sizeof(char));
        str_len += read_count;
    }
    
    if (read_str != NULL) {
        /* Remove newline character */
        read_str[str_len - 1] = '\0'; 
    }

    return read_str;
}

#endif
