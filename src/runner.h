#ifndef RUNNER_H
#define RUNNER_H

#include "procman.h"

typedef struct runner {
    int pipe[2];
    int worker_pid;
    procman *pm;
} runner;

int rn_init(runner *rn, int pm_max_running_processes);

void rn_send_input(runner *rn, const char *input);

#endif
