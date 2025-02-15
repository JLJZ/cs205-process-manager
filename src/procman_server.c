#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/prctl.h>

#include "procman.h"
#include "argparse.h"

#define error(msg) do { perror("[procman-server] " msg); } while (0);

#define BUFFER_SIZE 64


/******************************************************************************
 *                                 UTILITIES                                  * 
 ******************************************************************************/


static void pm_list_processes(procman *pm) {
    for (process *p = pm->processes; p != NULL; p = p->next) {
        printf("%d,%d\n", p->pid, p->status);
    }
}

static process *pm_find_process(procman *pm, pid_t pid) {
    for (process *p = pm->processes; p != NULL; p = p->next) {
        /* Assuming there are never duplicated pid stored */
        if (p->pid == pid) {
            return p;
        }
    }

    return NULL;
}


/******************************************************************************
 *                               COMMAND HANDLERS                             *
 ******************************************************************************/


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

static void pm_server_stop_process(procman *pm, process *p) {
    if (p->status == TERMINATED || p->status == STOPPED) {
        return;
    }
    
    pm_server_remove_running_process(pm, p);
    
    kill(p->pid, SIGSTOP);

    p->status = STOPPED;
}

static void pm_server_terminate_process(procman *pm, process *p) {
    if (p->status == TERMINATED) {
        return;
    }
    
    pm_server_remove_running_process(pm, p);

    kill(p->pid, SIGTERM);
    p->status = TERMINATED;
}

static void pm_server_spawn_process(procman *pm, char *const argv[]) {

    pid_t pid = fork();

    switch (pid) {
    case -1:
        error("fork() failed");
        return;

    case 0: {

        /* Send SIGTERM to child when parent dies  */
        if (prctl(PR_SET_PDEATHSIG, SIGTERM) < 0) {
            error("Failed to link parent death signal");
            exit(EXIT_FAILURE);
        }
        
        if (getppid() != pm->server_pid) {
            /* Parent died before prctl() */
            exit(EXIT_FAILURE);
        }

        puts("Child process spawned");

        if (raise(SIGSTOP) == 0) {
            puts("Child process started");

            if (execvp(argv[0], argv) < 0) {
                error("exec() failed");
            }
        }

        exit(EXIT_FAILURE);
    }

    default: {
        /*
         * Sleep for 1 second to allow child to spawn and suspend.
         * Yes, it may suffer race conditions but highly unlikely
         * for my assignment. Simple is best right.
         */
        sleep(1);

        /* Enqueue process */
        process *p = malloc(sizeof(process));
        p->pid = pid;
        p->status = READY;
        p->previous = NULL;
        p->next = NULL;
        if (pm->processes != NULL) {
            pm->processes->previous = p;
            p->next = pm->processes;
        }
        pm->processes = p;
        break;
    }
    }
}


/******************************************************************************
 *                              COMMAND DISPATCHER                            *
 ******************************************************************************/


static bool ensure_args_length(args *a, size_t n, const char *message) {
    if (a->token_count < n) {
        fwrite(message, sizeof(char), strlen(message) + 1, stderr);
        return false;
    }
    
    return true;
}

static pid_t parse_pid(const char *pid) {
    int num = atoi(pid);
    /* atoi() returns 0 on invalid input but 0 is also an invalid pid for us */
    if (num == 0) {
        error("Invalid pid");
    }
    return num;
}

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
            process *p = pm_find_process(pm, pid);
            if (p) {
                pm_server_stop_process(pm, p);
            }
        }

    } else if (!strcmp(command, "kill")) {
        if (ensure_args_length(a, 2, "USAGE: kill [PID]\n")) {
            pid_t pid = parse_pid(a->argv[1]);
            process *p = pm_find_process(pm, pid);
            if (p) {
                pm_server_terminate_process(pm, p);
            }
        }

    } else if (!strcmp(command, "list")) {
        pm_list_processes(pm);

    } else {
        fprintf(stderr, "Unrecognised command\n");
    }
}


/******************************************************************************
 *                             MAIN-LOOP PROCEDURES                           * 
 ******************************************************************************/


static void pm_server_reap_terminated_process(procman *pm) {
    int status = 0;

    pid_t pid = waitpid(-1, &status, WNOHANG | WCONTINUED | WSTOPPED);
    
    switch (pid) {
        case -1:
            if (ECHILD != errno) {  /* Ignore if there's no children */
                perror("wait() failed");
            }
        case 0:
            return;

        default: {
            process *p = pm_find_process(pm, pid);
            
            if (WIFEXITED(status)) {
                p->status = TERMINATED;
            
                for (size_t i = 0; i < pm->processes_running_max; ++i) {
                    process *p = pm->processes_running[i];
                    if (p != NULL && p->pid == pid) {
                        pm->processes_running[i] = NULL;
                        pm->processes_running_count -= 1;
                        printf("Removed from running (%d)\n", p->pid);
                        break;
                    }
                }
                printf("Terminated (%d)\n", p->pid);
            }
        }
    }
}

static void pm_server_reschedule_processes(procman *pm) {
    if (pm->processes_running_count < pm->processes_running_max) {

        /* Find earliest ready process */
        process *p = pm->processes;
        while (p != NULL) {
            if (p->status == READY) {
                break;
            }
            p = p->next;
        }
        
        /* No ready processes */
        if (p == NULL) {
            return;
        }

        printf("Ready found (%d)\n", p->pid);

        /* Run the process */
        for (size_t i = 0; i < pm->processes_running_max; ++i) {
            if (pm->processes_running[i] == NULL) {
                pm->processes_running[i] = p;
                if (kill(p->pid, SIGCONT) < 0) {
                    error("failed to continue process");
                    return;
                }
                p->status = RUNNING;
                pm->processes_running_count += 1;
                puts("REGISTER");
                break;
            }
        }
    }
}


/******************************************************************************
 *                                INITIALISER                                 * 
 ******************************************************************************/


static size_t read_pipe(int fd, char **out) {
    assert(*out == NULL);
    char *str = NULL;
    ssize_t size = 0;

    char buffer[BUFFER_SIZE];
    ssize_t read_size = read(fd, buffer, BUFFER_SIZE);

    while (read_size > 0) {
        str = realloc(str, (size_t)(size + read_size));
        memcpy(str + size, buffer, (size_t)read_size);
        size += read_size;

        /* End of buffer reached */
        if (read_size < BUFFER_SIZE) {
            break;
        }

        read_size = read(fd, buffer, BUFFER_SIZE);
    }
    
    if (read_size == -1) {
        if (errno != EAGAIN) { /* Ignore empty pipe */
            error("read() pipe failed on server");
            if (str != NULL) {
                free(str);
            }
            str = NULL;
            size = 0;
        }
    }
    
    *out = str;
    return (size_t)size;
}

/* Defined for client to link */
extern void pm_server_init(procman *pm) {
    if (close(pm->pipe[1]) < 0) {
        error("close() pipe failed on server init");
        return;
    }
    
    /* Main server loop */

    while (true) {
        char *cmd_str = NULL;
        size_t size = read_pipe(pm->pipe[0], &cmd_str);

        pm_server_reap_terminated_process(pm);
        pm_server_reschedule_processes(pm);

        /* Ignore further processing on errors */
        if (size < 1) {
            continue;
        }
        
        args a;
        args_cast(&a, cmd_str, size);

        dispatch(pm, &a);
        
        args_free(&a);
    }
    
    if (close(pm->pipe[0]) < 0) {
        error("close() pipe failed on server init");
        return;
    }

    exit(EXIT_SUCCESS);
}
