#ifndef PROCMAN_H
#define PROCMAN_H

#include <unistd.h>

typedef enum pstatus {
    RUNNING,
    READY,
    STOPPED,
    TERMINATED,
} pstatus;

typedef struct process process;
struct process {
    pid_t pid;
    pstatus status;
    process *previous;
    process *next;
};

typedef struct procman {
    process *processes;
    process *last_process;

    process **processes_running;
    size_t processes_running_max;
    size_t processes_running_count;
} procman;

void pm_init(procman *pm, size_t max_running_processes);

void pm_run(procman *pm, const char *command);

void pm_shutdown(procman *pm);

#endif
