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
    int pipe[2];
    pid_t server_pid;
    process *processes;
    process *last_process;

    process **processes_running;
    size_t processes_running_max;
    size_t processes_running_count;
} procman;

procman *pm_init(size_t max_running_processes);

void pm_shutdown(procman *pm, unsigned int retries);

void pm_execute(procman *pm, const char *cmd_string);

#endif
