#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>

#include "input.h"
#include "procman.h"

#define MAX_RUNNING_PROCESSES 3
#define BUFFER_SIZE 64


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
static char **split_lines(char *str, size_t *count) {
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
        }

        /* Get input from stdin */
        char *input = read_all(stdin, BUFFER_SIZE);
        prompt_needed = input != NULL;

        /* Split input by \n and process them as separate commands.
         * This may happen when piping a file into the program or
         * a user pasting a recipe into the prompt.
         */
        size_t command_count = 0;
        char **commands = split_lines(input, &command_count);
        
        for (size_t i = 0; i < command_count; ++i) {
            is_running = strcmp("exit", commands[i]) != 0;
            pm_send_command(&pm, commands[i]);
            if (!is_running) {
                break;
            }
        }

        pm_run(&pm);

        if (command_count > 0) {
            free(input);
            free(commands);
        }
    }

    pm_shutdown(&pm);

    return 0;
}
