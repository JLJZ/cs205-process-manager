#ifndef RUNNER_H
#define RUNNER_H

#include "procman.h"

typedef struct runner {
    int pipe[2];
    int worker_pid;
    procman *pm;
} runner;

/**
 * @brief Initialise the runner with a corresponding background worker process
 * 
 * @param rn Target runner
 * @param pm_max_running_processes Max number of running processes
 * @return int 0 if successful. -1 otherwise
 */
int rn_init(runner *rn, size_t pm_max_running_processes);

/**
 * @brief Send input to the process runner
 * 
 * @param input String input
 * @return int 0 if successful. -1 otherwise
 */
int rn_send_input(runner *rn, const char *input);

/**
 * @brief Free resources used by runner
 * 
 * @param rn Target runner
 * @return int 
 */
int rn_free(runner *rn);

#endif
