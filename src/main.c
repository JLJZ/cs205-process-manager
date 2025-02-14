#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "procman.h"
#include "argparse.h"

#define MAX_RUNNING_PROCESSES 3
#define SHUTDOWN_TIMEOUT 5

/**
 * @brief Read a command from stdin
 * 
 * @return char* Command string
 */
static char *read_line(void) {
    char *input = NULL;
    size_t len = 0;
    
    ssize_t read_len = getline(&input, &len, stdin);

    if (read_len < 0) {
        exit(EXIT_FAILURE);
    }
    
    /* Delete newline character */
    input[read_len - 1] = '\0';

    return input;
}

int main(void) {
    procman *pm = pm_init(MAX_RUNNING_PROCESSES);

    while (true) {
        printf("cs205$ ");
        char *command = read_line();
        
        if (!strcmp("exit", command)) {
            free(command);
            break;
        }
        
        pm_execute(pm, command);

        free(command);
    }
    
    pm_shutdown(pm, SHUTDOWN_TIMEOUT);

    return 0;
}
