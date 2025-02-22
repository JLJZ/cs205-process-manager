#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>

#include "input.h"
#include "runner.h"

#define MAX_RUNNING_PROCESSES 3
#define POLLING_INTERVAL 1
#define BUFFER_SIZE 64


int main(void) {
    runner rn;
    if (rn_init(&rn, MAX_RUNNING_PROCESSES) < 0) {
        perror("failed to start");
        exit(EXIT_FAILURE);
    };
    
    bool is_running = true;

    while (is_running) {
        printf("cs205$ ");
        fflush(stdout);

        /* Get input from stdin */
        char *input = read_all(STDIN_FILENO, BUFFER_SIZE, '\n');

        /* Split input by \n and process them as separate commands.
         * This may happen when piping a file into the program or
         * a user pasting a recipe into the prompt.
         */
        size_t command_count = 0;
        char **commands = split_lines(input, &command_count);
        
        for (size_t i = 0; i < command_count; ++i) {
            rn_send_input(&rn, commands[i]);
            is_running = strcmp("exit", commands[i]) != 0;
            if (!is_running) {
                break;
            }
        }

        if (command_count > 0) {
            free(input);
            free(commands);
        }

        if (is_running) {
            sleep(POLLING_INTERVAL);
        }
    }

    rn_free(&rn);

    return EXIT_SUCCESS;
}
