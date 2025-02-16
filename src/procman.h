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

/**
 * @brief Initialise a process manager>
 * 
 * @param pm Target process manager
 * @param max_running_processes Number of processes allowed to be running
 */
void pm_init(procman *pm, size_t max_running_processes);

/**
 * @brief Dispatch a command to the process manager.
 * 
 * @param pm Target process manager
 * @param command The command string
 */
void pm_send_command(procman *pm, const char *command);

/**
 * @brief Run process management procedures. Should be called repeatedly.
 * 
 * @param pm Target process manager
 */
void pm_run(procman *pm);

/**
 * @brief Free all resources and spawned processes.
 * 
 * @param pm Target process manager
 */
void pm_shutdown(procman *pm);

#endif
