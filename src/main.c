#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include <assert.h>

#include "procman.h"

#define MAX_RUNNING_PROCESSES 3
#define BUFFER_SIZE 64

/**
 * @brief Read a command from stdin
 * 
 * @return char* Command string
 */
static char *read_line(void) {
    char buffer[BUFFER_SIZE];
    
    char* read_str = NULL;
    size_t str_len = 0;
    
    /* Collect characters until EOL */
    while (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
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

int main(void) {
    /* Set non-blocking reads from standard input */
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

    procman pm;
    pm_init(&pm, MAX_RUNNING_PROCESSES);
    
    bool prompt_needed = true;
    bool is_running = true;

    while (is_running) {
        if (prompt_needed) {
            printf("cs205$ ");
            prompt_needed = false;
        }

        char *command = read_line();
        
        if (command != NULL) {
            is_running = strcmp("exit", command) != 0;
            prompt_needed = true;
        }
        
        pm_run(&pm, command);

        if (command != NULL) {
            free(command);
        }
    }
    
    pm_shutdown(&pm);

    return 0;
}
