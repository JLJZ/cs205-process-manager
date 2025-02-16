#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/prctl.h>

#include "procman.h"
#include "argparse.h"

#define error(msg) do { perror("[error] " msg); } while (0);

#define USAGE "COMMANDS:\n"                     \
              "    run [program] [arguments]\n" \
              "    stop [PID]\n"                \
              "    kill [PID]\n"                \
              "    resume [PID]\n"              \
              "    list\n"                      \
              "    exit\n"


/******************************************************************************
 *                                 UTILITIES                                  * 
 ******************************************************************************/


/**
 * @brief Search a chain of processes for specified pid.
 * 
 * @param processes Beginning of process chain
 * @param pid Target pid
 * @return process* The process with target pid. NULL if no processes found
 */
static process *find_process(process *processes, pid_t pid) {
    for (process *p = processes; p != NULL; p = p->next) {
        /* Assuming there are never duplicated pid stored */
        if (p->pid == pid) {
            return p;
        }
    }

    return NULL;
}

/**
 * @brief Links a process to the end of process chain. 
 * 
 * @warning No checks are done to prevent cycles in the chain if a duplicate
 * process is added.
 *
 * @param pm Target process manager
 * @param p The process to link
 */
static void pm_enqueue_process(procman *pm, process *p) {
    /* Add process as first in chain */
    if (pm->processes == NULL) {
        p->next = NULL;
        p->previous = NULL;
        pm->processes = p;
        pm->last_process = p;

    } else { /* Add process to the tail of the chain */

        pm->last_process->next = p;
        p->previous = pm->last_process;
        p->next = NULL;
        pm->last_process = p;
    }
}


/******************************************************************************
 *                               COMMAND HANDLERS                             *
 ******************************************************************************/


/**
 * @brief Prints all process status in sequence of the process chain
 * 
 * @param pm Process manager with target process chain
 */
static void pm_list_processes(procman *pm) {
    for (process *p = pm->processes; p != NULL; p = p->next) {
        printf("%d,%d\n", p->pid, p->status);
    }
}

/**
 * @brief Remove a running process from the running process list.
 * 
 * @param pm Process manager with target list
 * @param p Target process with status RUNNING. No-op for other states
 */
static void pm_server_remove_running_process(procman *pm, process *p) {
    if (p->status != RUNNING) {
        return;
    }

    for (size_t i = 0; i < pm->processes_running_max; ++i) {
        if (pm->processes_running[i] && pm->processes_running[i] == p) {
            pm->processes_running[i] = NULL;
            pm->processes_running_count -= 1;
            printf("Removed from running (%d)\n", p->pid);
            break;
        }
    }
}

/**
 * @brief Stop a process and removes it from running list if it is running.
 * 
 * @param pm Process manager with target list
 * @param p Target process. No-op if status is TERMINATED or STOPPED
 */
static void pm_server_stop_process(procman *pm, process *p) {
    if (p->status == TERMINATED || p->status == STOPPED) {
        return;
    }
    
    pm_server_remove_running_process(pm, p);
    kill(p->pid, SIGSTOP);
    p->status = STOPPED;
}

/**
 * @brief Terminate a process and removes it from running list if it is running.
 * 
 * @param pm Process manager with target list
 * @param p Target process. No-op if status is TERMINATED
 */
static void pm_server_terminate_process(procman *pm, process *p) {
    if (p->status == TERMINATED) {
        return;
    }
    
    pm_server_remove_running_process(pm, p);
    kill(p->pid, SIGTERM);
    p->status = TERMINATED;
}

/**
 * @brief Resume a stopped process.
 *
 * @param p Target process that must have status STOPPED
 *
 * @note Unlike other command handlers, only process management state is
 * updated. No signals are sent to the underlying process
 */
static void pm_server_resume_process(process *p) {
    assert(p->status == STOPPED);
    p->status = READY;
    /* Don't send SIGCONT and let the rescheduler decide whether the process
     * should run.
     */
}

/**
 * @brief Spawn a child process that executes a given shell command.
 * 
 * @details Child is spawned and suspended. It will be queued into the
 * process manager in a READY state to be according to the scheduler.
 * 
 * @param pm Target process manager
 * @param argv Array of strings representing tokens of the command. Last token
 * must be NULL to indicate the end of the array
 */
static void pm_server_spawn_process(procman *pm, char *const argv[]) {

    pid_t parent_pid = getpid();
    pid_t child_pid = fork();

    switch (child_pid) {
    case -1:
        error("fork() failed");
        return;

    case 0: {

        /* Send SIGTERM to child when parent dies  */
        if (prctl(PR_SET_PDEATHSIG, SIGTERM) < 0) {
            error("Failed to link parent death signal");
            exit(EXIT_FAILURE);
        }
        
        if (getppid() != parent_pid) {
            /* Parent died before prctl() */
            exit(EXIT_FAILURE);
        }

        puts("Child process spawned");

        /* Suspend process to wait for parent to resume it before execvp() */
        if (raise(SIGSTOP) == 0) {
            puts("Child process started");

            if (execvp(argv[0], argv) < 0) {
                /* Print exec() error, usually due to program not found */
                char *msg = "error running ";
                msg = malloc(1 + (strlen(msg) + strlen(argv[0])));
                strcpy(msg, "error running ");
                strcat(msg, argv[0]);
                perror(msg);
            }
        }

        exit(EXIT_FAILURE);
    }

    default: {
        /* Wait for child process to suspend before enqueuing */
        int status;
        int w_pid = waitpid(child_pid, &status, WSTOPPED);

        while (w_pid < 0) {

            /* Continue waiting when interrupted by other signals */
            if (errno != EINTR) {
                error("error waiting for process to spawn");
                return;
            }

            w_pid = waitpid(child_pid, &status, WSTOPPED);
        }
        
        /* Child suspended with stop signal */
        if (WIFSTOPPED(status)) {
            /* Enqueue process */
            process *p = malloc(sizeof(process));
            p->pid = child_pid;
            p->status = READY;
            pm_enqueue_process(pm, p);
        }

        /* No-op when child is unexpectedly terminated and not stopped */

        break;
    }
    }
}

/**
 * @brief Terminate all managed processes and remove their handle.
 * 
 * @param pm Target process manager
 */
static void pm_clear_processes(procman *pm) {
    /* Iterate process chain to terminate each process and free its pointer */
    process *p = pm->processes;
    while (p != NULL) {
        kill(p->pid, SIGTERM);
        process *next = p->next;
        free(p);
        p = next;
    }
    
    for (size_t i = 0; i < pm->processes_running_max; ++i) {
        pm->processes_running[i] = NULL;
    }
    
    pm->processes = NULL;
    pm->last_process = NULL;
    pm->processes_running_count = 0;
}


/******************************************************************************
 *                              COMMAND DISPATCHER                            *
 ******************************************************************************/


/**
 * @brief Ensure number of required arguments is hit.
 * 
 * @param a Target argument
 * @param n Minimum number of tokens
 * @param message Message to print when lacking tokens
 * @return true Reached token count
 * @return false Less than token count
 */
static bool ensure_args_length(args *a, size_t n, const char *message) {
    if (a->token_count < n) {
        fwrite(message, sizeof(char), strlen(message) + 1, stderr);
        return false;
    }
    
    return true;
}

/**
 * @brief Parse a string representing a pid into a pid_t type
 * 
 * @param pid Target pid
 * @return pid_t Parsed pid. 0 if string was an invalid pid
 */
static pid_t parse_pid(const char *pid) {
    int num = atoi(pid);
    /* atoi() returns 0 on invalid input but 0 is also an invalid pid for us */
    if (num == 0) {
        error("Invalid pid");
    }
    return num;
}

/**
 * @brief Execute command handlers based on received commands.
 * 
 * @param pm Target process manager to run command handlers on
 * @param a Command to dispatch
 */
static void dispatch(procman *pm, args *a) {
    /* No-op when empty command received */
    if (a->token_count == 0) {
        return;
    }

    const char *command = a->argv[0];

    if (!strcmp(command, "run")) {
        if (ensure_args_length(a, 2, "USAGE: run [program] [arguments]\n")) {
            pm_server_spawn_process(pm, a->argv + 1);
        }

    } else if (!strcmp(command, "stop")) {
        if (ensure_args_length(a, 2, "USAGE: stop [PID]\n")) {
            pid_t pid = parse_pid(a->argv[1]);
            process *p = find_process(pm->processes, pid);
            if (p) {
                pm_server_stop_process(pm, p);
            }
        }

    } else if (!strcmp(command, "kill")) {
        if (ensure_args_length(a, 2, "USAGE: kill [PID]\n")) {
            pid_t pid = parse_pid(a->argv[1]);
            process *p = find_process(pm->processes, pid);
            if (p) {
                pm_server_terminate_process(pm, p);
            }
        }

    } else if (!strcmp(command, "resume")) {
        if (ensure_args_length(a, 2, "USAGE: resume [PID]\n")) {
            pid_t pid = parse_pid(a->argv[1]);
            process *p = find_process(pm->processes, pid);
            if (p) {
                pm_server_resume_process(p);
            }
        }

    } else if (!strcmp(command, "list")) {
        pm_list_processes(pm);

    } else if (!strcmp(command, "exit")) {
        pm_clear_processes(pm);

    } else { /* Unrecognised command received */
        fprintf(stderr, USAGE);
    }
}


/******************************************************************************
 *                             MAIN-LOOP PROCEDURES                           * 
 ******************************************************************************/


static void pm_server_reap_terminated_process(procman *pm) {
    int status = 0;

    pid_t pid = waitpid(-1, &status, WNOHANG);
    
    switch (pid) {
        case -1:
            if (ECHILD != errno) {  /* Ignore if there's no children */
                perror("wait() failed");
            }
        case 0:
            return;

        default: {
            process *p = find_process(pm->processes, pid);

            /* The process here should always be managed by us */
            assert(p != NULL);
            
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                pm_server_remove_running_process(pm, p);
                p->status = TERMINATED;
                printf("Terminated (%d)\n", p->pid);
            }
        }
    }
}

static void pm_server_reschedule_processes(procman *pm) {
    
    /* Collect earliest process to be ran */
    process **to_run = malloc(sizeof(process *) * pm->processes_running_max);
    {
        size_t idx = 0;
        process *p = pm->processes;

        while (idx < pm->processes_running_max && p != NULL) {
            if (p->status == RUNNING || p->status == READY) {
                to_run[idx++] = p;
            }
            p = p->next;
        }
        
        while (idx < pm->processes_running_max) {
            to_run[idx++] = NULL;
        }
    }

    /* Stop currently running processes that should not be running */
    for (size_t i = 0; i < pm->processes_running_max; ++i) {
        process *p_running = pm->processes_running[i];

        if (p_running == NULL) {
            continue;
        }

        bool process_should_run = false;
        for (size_t j = 0; j < pm->processes_running_max; ++j) {
            if (p_running == to_run[j]) {
                process_should_run = true;
                break;
            }
        }
            
        if (!process_should_run) {
            p_running->status = READY;
            kill(p_running->pid, SIGSTOP);
            puts("Ready now");
        } else {
            /* Process in running list but not running is a bug */
            assert(p_running->status == RUNNING);
        }
    }
    
    /* Run all processes that needs to run */
    for (size_t i = 0; i < pm->processes_running_max; ++i) {
        process *p_to_run = to_run[i];
        if (p_to_run != NULL && p_to_run->status == READY) {
            p_to_run->status = RUNNING;
            kill(p_to_run->pid, SIGCONT);
            puts("Running now");
        }
    }

    /* Replace running list */
    free(pm->processes_running);
    pm->processes_running = to_run;
}


/******************************************************************************
 *                             PUBLIC INTERFACE                               * 
 ******************************************************************************/


/**
 * @brief Initialise a process manager.
 * 
 * @param pm Target process manager
 * @param max_running_processes Number of processes allowed to be running
 */
void pm_init(procman *pm, size_t max_running_processes) {
    pm->processes = NULL;
    pm->last_process = NULL;
    pm->processes_running_count = 0;
    pm->processes_running_max = max_running_processes;
    pm->processes_running = malloc(max_running_processes * sizeof(process *));
}

/**
 * @brief Dispatch a command to the process manager.
 * 
 * @param pm Target process manager
 * @param command The command string
 */
void pm_send_command(procman *pm, const char *command) {
    if (command != NULL) {
        args a;
        args_parse(&a, command);
        dispatch(pm, &a);
        args_free(&a);
    }
}

/**
 * @brief Run process management procedures. Should be called repeatedly.
 * 
 * @param pm Target process manager
 */
void pm_run(procman *pm) {
    pm_server_reap_terminated_process(pm);
    pm_server_reschedule_processes(pm);
}

/**
 * @brief Free all resources and spawned processes.
 * 
 * @param pm Target process manager
 */
void pm_shutdown(procman *pm) {
    pm_clear_processes(pm);
    if (pm->processes_running != NULL) {
        free(pm->processes_running);
        pm->processes_running = NULL;
    }
    pm->processes_running_max = 0;
}
