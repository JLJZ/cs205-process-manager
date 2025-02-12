#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <stdbool.h>

#include "procman.h"
#include "argparse.h"

#define error(msg) do { perror("[procman] " msg); } while (0);

#define BUFFER_SIZE 64

typedef struct process process;
typedef struct process {
    pid_t pid;
    process *previous;
    process *next;
} process;

struct procman {
    int pipe[2];
    pid_t server_pid;
    process *processes;
};

static char *pack_argv(const char *const argv[], size_t *size) {
    char *bytes = NULL;
    size_t length = 0;
    
    for (int i = 0; argv[i] != NULL; ++i) {
        const char *token = argv[i];
        size_t token_len = strlen(argv[i]) + 1;
        bytes = realloc(bytes, length + token_len);
        memcpy(bytes + length, token, token_len);
        length += token_len;
    }
    
    *size = length;
    
    return bytes;
}

static void handle_sigchld(int signum, siginfo_t *siginfo, void *ucontext) {
    assert(signum == SIGCHLD);

    (void) ucontext;

    switch (siginfo->si_code)
    {
    case CLD_KILLED:
    case CLD_DUMPED:
    case CLD_EXITED:
        printf("Terminated PID (%d)\n", siginfo->si_pid);
        break;

    case CLD_STOPPED:
        printf("Stopped PID (%d)\n", siginfo->si_pid);
        break;

    case CLD_CONTINUED:
    case CLD_TRAPPED:
        break;

    default: /* Unknown code */
        assert(false);
        break;
    }
}

static void pm_server_spawn_process(procman *pm, char *const argv[]) {
    
    /* Block child continuation signal */
    sigset_t setmask;
    sigemptyset(&setmask);
    sigaddset(&setmask, SIGCONT);

    pid_t pid = fork();

    switch (pid) {
    case -1:
        error("fork() failed");
        return;

    case 0: {
        puts("Child process spawned");
        sigprocmask(SIG_BLOCK, &setmask, NULL);
        puts("Child process started");
        if (execvp(argv[0], argv) < 0) {
            error("exec() failed");
        }
        exit(EXIT_FAILURE);
    }

    default: {
        struct sigaction action;
        sigemptyset(&action.sa_mask);
        action.sa_sigaction = handle_sigchld;
        /* Restart reads that are interrupted and fill siginfo for handler */
        action.sa_flags = SA_RESTART | SA_SIGINFO;  
        sigaction(SIGCHLD, &action, NULL);

        /* Enqueue process */
        process *p = calloc(1, sizeof(p));
        p->pid = pid;
        p->previous = NULL;
        p->next = NULL;
        if (pm->processes != NULL) {
            pm->processes->previous = p;
            p->next = pm->processes;
        }
        pm->processes = p;

        /* Continue paused child */
        kill(pid, SIGCONT);
        break;
    }
    }
}

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
        error("read() pipe failed on server");
        if (str != NULL) {
            free(str);
        }
        str = NULL;
        size = 0;
    }
    
    *out = str;
    return (size_t)size;
}

// static void dispatch(const args *a) {
//     const char *command = a->argv[0];
// }

static void pm_server_init(procman *sp) {
    if (close(sp->pipe[1]) < 0) {
        error("close() pipe failed on server init");
        return;
    }
    
    while (true) {
        char *cmd_str = NULL;
        size_t size = read_pipe(sp->pipe[0], &cmd_str);

        /* Ignore further processing on errors */
        if (size < 1) {
            continue;
        }
        
        args a;
        args_cast(&a, cmd_str, size);
        
        printf("Command:");
        for (size_t i = 0; a.argv[i] != NULL; ++i) {
            printf(" %s", a.argv[i]);
        }
        printf("\n");

        pm_server_spawn_process(sp, a.argv);
        
        args_free(&a);
    }
    
    if (close(sp->pipe[0]) < 0) {
        error("close() pipe failed on server init");
        return;
    }

    exit(EXIT_SUCCESS);
}

static void pm_client_init(procman *sp) {
    if (close(sp->pipe[0]) < 0) {
        error("close() pipe failed on client init");
        return;
    }
}

procman *pm_init(void) {
    procman *sp = calloc(1, sizeof(procman));
    sp->processes = NULL;
    
    if (pipe(sp->pipe) < 0) {
        error("pipe() init failed");
        goto err;
    }

    pid_t pid = fork();

    switch (pid) {
    case -1:
        error("fork() failed");
        goto err;

    case 0:
        sp->server_pid = getpid();
        pm_server_init(sp);
        break;

    default:
        sp->server_pid = pid;
        pm_client_init(sp);
        break;
    }
    
    return sp;
err:
    free(sp);
    return NULL;
}

void pm_spawn(procman *sp, const char *const argv[]) {
    size_t size;
    char* data = pack_argv(argv, &size);
    
    if (write(sp->pipe[1], data, size) < 0) {
        error("write() pipe failed on client");
    }

    free(data);
}

void pm_free(procman *sp) {
    if (kill(sp->server_pid, SIGTERM) < 0) {
        if (kill(sp->server_pid, SIGKILL) < 0) {
            error("failed to kill server");
        }
    }
    free(sp);
}
