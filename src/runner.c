#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>

#include "runner.h"
#include "input.h"
#include "procman.h"

#define BUFFER_SIZE 64
#define error(msg) do { perror("[error] " msg); } while (0);

/**
 * @brief Start the main loop of a worker runner
 * 
 * @param rn Target runner
 */
static void rn_start_worker(runner *rn) {
    bool is_running = true;
    while (is_running) {
        const char *input = read_all(rn->pipe[1], BUFFER_SIZE);
        pm_send_command(rn->pm, input);
        is_running = strcmp("exit", input) != 0;
        free(input);
    }
    
    close(rn->pipe[0]);
    pm_shutdown(rn->pm);
    free(rn->pm);
    rn->pm = NULL;
}

/**
 * @brief Initialise the runner with a corresponding background worker process
 * 
 * @param rn Target runner
 * @param pm_max_running_processes Max number of running processes
 * @return int 0 if successful. 1 otherwise
 */
int rn_init(runner *rn, int pm_max_running_processes) {
    rn->pm = NULL;
    if (pipe(rn->pipe) < 0) {
        return -1;
    }
    
    rn->worker_pid = fork();
    switch (rn->worker_pid) {
        case -1:
            return -1;

        case 0: /* Initialise background worker process */
            rn->pm = malloc(sizeof(procman));
            pm_init(rn->pm, pm_max_running_processes);

            /* Setup non-blocking pipe read-end */
            close(rn->pipe[1]);
            fcntl(rn->pipe[0], F_SETFL, O_NONBLOCK);

            rn_start_worker(rn);
            exit(EXIT_SUCCESS);

        default: /* Prepare pipe write-end */
            close(rn->pipe[0]);
            return 0;
    }
}

/**
 * @brief Send input to the process runner
 * 
 * @param input 
 */
void rn_send_input(runner *rn, const char *input) {
}
