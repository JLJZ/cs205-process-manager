#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

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
        
        is_running = strcmp("exit", input) != 0;

        if (input && strlen(input) > 0) {
            rn_send_input(&rn, input);
            free(input);
            if (is_running) {
                sleep(POLLING_INTERVAL);
            }
        }
    }

    rn_free(&rn);

    return EXIT_SUCCESS;
}
