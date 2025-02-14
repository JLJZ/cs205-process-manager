#ifndef PROCMAN_H
#define PROCMAN_H

#include <unistd.h>

typedef struct procman procman;

procman *pm_init(size_t max_running_processes);

void pm_shutdown(procman *pm, unsigned int retries);

void pm_execute(procman *pm, const char *cmd_string);

#endif
